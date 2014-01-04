#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>

#include <pthread.h>

#include <netinet/in.h>

#include <event.h>

#include "kcn.h"
#include "kcn_eq.h"
#include "kcn_log.h"
#include "kcn_socket.h"
#include "kcn_sockaddr.h"
#include "kcn_buf.h"
#include "kcn_net.h"
#include "kcn_msg.h"
#include "kcn_netstat.h"
#include "kcndb_db.h"
#include "kcndb_server.h"

#define LOG(p, fmt, ...)						\
	KCN_LOG(p, "thread[%hu]: " fmt, kt->kt_number, __VA_ARGS__)

enum kcndb_thread_status {
	KCNDB_THREAD_STATUS_DEAD,
	KCNDB_THREAD_STATUS_RUN
};

struct kcndb_thread {
	pthread_t kt_id;
	enum kcndb_thread_status kt_status;
	unsigned short kt_number;
	int kt_listenfd;
	struct event_base *kt_evb;
	struct kcndb_db *kt_db;
};

#define KCNDB_THREAD_NUMBER_MAIN	0

static int kcndb_server_socket = -1;
static in_port_t kcndb_server_port = KCN_HTONS(KCN_NETSTAT_PORT_DEFAULT);

#define KCNDB_SERVER_NWORKERS		8
static struct kcndb_thread kcndb_server_threads[KCNDB_SERVER_NWORKERS];

static void *kcndb_server_main(void *);

static void
kcndb_thread_init(struct kcndb_thread *kt, unsigned short n)
{

	kt->kt_number = n;
	kt->kt_listenfd = kcndb_server_socket;
	kt->kt_evb = NULL;
}

static void
kcndb_thread_finish(struct kcndb_thread *kt)
{

	if (kt->kt_evb != NULL)
		event_base_free(kt->kt_evb);
	kcndb_db_destroy(kt->kt_db);
}

void
kcndb_server_port_set(uint16_t port)
{

	assert(port >= KCNDB_NET_PORT_MIN);
	kcndb_server_port = htons(port);
}

bool
kcndb_server_start(void)
{
	int i, domain, error;

	assert(kcndb_server_socket == -1);

#ifdef HAVE_IPV6
	domain = PF_INET6;
#else /* HAVE_IPV6 */
	domain = PF_INET;
#endif /* ! HAVE_IPV6 */
	kcndb_server_socket = kcn_socket_listen(domain, kcndb_server_port);
	if (kcndb_server_socket == -1)
		goto bad;
	KCN_LOG(INFO, "server listen on %u with fd %d.",
	    ntohs(kcndb_server_port), kcndb_server_socket);

	for (i = KCNDB_THREAD_NUMBER_MAIN + 1; i < KCNDB_SERVER_NWORKERS; i++) {
		kcndb_thread_init(&kcndb_server_threads[i], i);
		error = pthread_create(&kcndb_server_threads[i].kt_id, NULL,
		    kcndb_server_main, &kcndb_server_threads[i]);
		if (error != 0) {
			KCN_LOG(EMERG, "cannot create server thread: %s",
			    strerror(error));
			goto bad;
		}
		kcndb_server_threads[i].kt_status = KCNDB_THREAD_STATUS_RUN;
	}

	return true;
  bad:
	kcndb_server_stop();
	return false;
}

void
kcndb_server_stop(void)
{

	/* XXX */
	kcn_socket_close(&kcndb_server_socket);
}

void
kcndb_server_loop(void)
{
	struct kcndb_thread *kt;
	
	kt = &kcndb_server_threads[KCNDB_THREAD_NUMBER_MAIN];
	kcndb_thread_init(kt, KCNDB_THREAD_NUMBER_MAIN);
	kt->kt_id = pthread_self();
	kt->kt_status = KCNDB_THREAD_STATUS_RUN;
	kcndb_server_main(kt);
}

static bool
kcndb_server_response_send(const struct kcndb_db_record *kdr, size_t score,
    void *arg)
{
	struct kcn_net *kn = arg;
	struct kcn_buf okb;
	struct kcn_msg_response kmr;

	kcn_net_obuf(kn, &okb);
#define KCNDB_SERVER_RESPONSE_MAGIC	((void *)1U)
	if (kdr != KCNDB_SERVER_RESPONSE_MAGIC) {
		kmr.kmr_error = 0;
		kmr.kmr_score = score;
		kmr.kmr_loc = kdr->kdr_loc;
		kmr.kmr_loclen = kdr->kdr_loclen;
	} else {
		kmr.kmr_error = score;
		kmr.kmr_score = 0;
		kmr.kmr_loc = 0;
		kmr.kmr_loclen = 0;
	}
	kcn_msg_response_encode(&okb, &kmr);
	return kcn_net_write(kn, &okb);
}

static bool
kcndb_server_query_process(struct kcn_net *kn, struct kcn_buf *ikb,
    const struct kcn_msg_header *kmh)
{
	struct kcndb_thread *kt;
	struct kcn_msg_query kmq;

	kt = kcn_net_data(kn);
	if (kcn_msg_query_decode(ikb, kmh, &kmq) &&
	    kcndb_db_search(kt->kt_db, &kmq.kmq_eq, kmq.kmq_maxcount,
	    kcndb_server_response_send, kn))
		errno = 0;
	kcndb_server_response_send(KCNDB_SERVER_RESPONSE_MAGIC, errno, kn);
	/* always return 0 in order to return a response with an error. */
	return true;
}

static bool
kcndb_server_add_process(struct kcn_net *kn, struct kcn_buf *kb,
    const struct kcn_msg_header *kmh)
{
	struct kcndb_thread *kt;
	struct kcn_msg_add kma;
	struct kcndb_db_record kdr;

	kt = kcn_net_data(kn);
	if (! kcn_msg_add_decode(kb, kmh, &kma))
		return false;
	kdr.kdr_time = kma.kma_time;
	kdr.kdr_val = kma.kma_val;
	kdr.kdr_loc = kma.kma_loc;
	kdr.kdr_loclen = kma.kma_loclen;
	return kcndb_db_record_add(kt->kt_db, kma.kma_type, &kdr);
}

static int
kcndb_server_recv(struct kcn_net *kn, struct kcn_buf *kb, void *arg)
{
	struct kcndb_thread *kt = arg;
	struct kcn_msg_header kmh;
	bool rc;

	LOG(DEBUG, "recv %u bytes", kcn_buf_trailingdata(kb));

	while (kcn_buf_trailingdata(kb) > 0) {
		if (! kcn_msg_header_decode(kb, &kmh))
			goto bad;

		switch (kmh.kmh_type) {
		case KCN_MSG_TYPE_QUERY:
			rc = kcndb_server_query_process(kn, kb, &kmh);
			break;
		case KCN_MSG_TYPE_ADD:
			rc = kcndb_server_add_process(kn, kb, &kmh);
			break;
		case KCN_MSG_TYPE_DEL: /* XXX */
		default:
			rc = false;
			errno = EOPNOTSUPP;
		}
		if (! rc) {
			if (errno == EAGAIN)
				kcn_buf_start(kb);
			goto bad;
		}
	}

	return EAGAIN;
  bad:
	if (errno != EAGAIN)
		LOG(ERR, "recv invalid message: %s", strerror(errno));
	return errno;
}

static void
kcndb_server_session(struct kcndb_thread *kt, int fd, const char *name)
{
	struct kcn_net *kn;

	LOG(INFO, "[%s] connected", name);

	kn = kcn_net_new(kt->kt_evb, fd, KCN_MSG_MAXSIZ, name,
	    kcndb_server_recv, kt);
	if (kn == NULL) {
		kcn_socket_close(&fd);
		LOG(ERR, "[%s] cannot allocate network structure: %s",
		    name, strerror(errno));
		goto out;
	}
	kcn_net_read_enable(kn);
	kcn_net_loop(kn);
	kcn_net_destroy(kn);
  out:
	LOG(INFO, "[%s] disconnected", name);
}

static void *
kcndb_server_main(void *arg)
{
	struct kcndb_thread *kt = arg;
	char name[KCN_SOCKNAMELEN];
	int fd;

	LOG(DEBUG, "start%s", "");

	kt->kt_evb = event_init();
	kt->kt_db = kcndb_db_new();
	if (kt->kt_evb == NULL) {
		LOG(ERR, "event_init() failed: %s", strerror(errno));
		goto out;
	}
	if (kt->kt_db == NULL) {
		LOG(ERR, "cannot allocate database: %s", strerror(errno));
		goto out;
	}

	for (;;) {
		fd = kcn_socket_accept(kt->kt_listenfd, name, sizeof(name));
		if (fd == -1) {
			LOG(ERR, "accept() failed: %s", strerror(errno));
			continue;
		}
		kcndb_server_session(kt, fd, name);
	}
  out:
	kcndb_thread_finish(kt);
	LOG(DEBUG, "finish%s", "");
	return NULL;
}
