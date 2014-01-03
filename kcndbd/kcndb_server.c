#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>

#include <pthread.h>

#include <netinet/in.h>

#include <event.h>

#include "kcn.h"
#include "kcn_info.h"
#include "kcn_eq.h"
#include "kcn_log.h"
#include "kcn_socket.h"
#include "kcn_sockaddr.h"
#include "kcn_pkt.h"
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
kcndb_server_query_process(struct kcn_net *kn, struct kcn_pkt *ikp,
    const struct kcn_msg_header *kmh)
{
	struct kcn_pkt okp;
	struct kcn_msg_query kmq;
	struct kcn_msg_response kmr;
	struct kcn_info *ki;

	if (! kcn_msg_query_decode(ikp, kmh, &kmq))
		ki = NULL;
	else
		/* XXX */
		ki = kcn_info_new(kmq.kmq_loctype, kmq.kmq_maxcount);

	kcn_msg_response_init(&kmr, ki);
	if (ki == NULL || ! kcndb_db_search(ki, &kmq.kmq_eq))
		kmr.kmr_error = errno;
	else  {
		assert(kcn_info_nlocs(ki) > 0);
		kmr.kmr_leftcount = kcn_info_nlocs(ki) - 1;
	}

	kcn_net_opkt(kn, &okp);
	do {
		kcn_msg_response_encode(&okp, &kmr);
		if (! kcn_net_write(kn, &okp))
			break;
	} while (kmr.kmr_leftcount-- > 0);

	kcn_info_destroy(ki); /* XXX */

	return true;
}

static bool
kcndb_server_add_process(struct kcn_net *kn, struct kcn_pkt *kp,
    const struct kcn_msg_header *kmh)
{
	struct kcn_msg_add kma;
	struct kcndb_db_record kdr;
	bool rc;

	(void)kn;
	if (! kcn_msg_add_decode(kp, kmh, &kma))
		return false;
	kdr.kdr_time = kma.kma_time;
	kdr.kdr_val = kma.kma_val;
	kdr.kdr_loc = kma.kma_loc;
	kdr.kdr_loclen = kma.kma_loclen;
	rc = kcndb_db_record_add(kma.kma_type, &kdr);
	return rc;
}

static int
kcndb_server_recv(struct kcn_net *kn, struct kcn_pkt *kp, void *arg)
{
	struct kcndb_thread *kt = arg;
	struct kcn_msg_header kmh;
	bool rc;

	LOG(DEBUG, "recv %u bytes", kcn_pkt_trailingdata(kp));

	while (kcn_pkt_trailingdata(kp) > 0) {
		if (! kcn_msg_header_decode(kp, &kmh))
			goto bad;

		switch (kmh.kmh_type) {
		case KCN_MSG_TYPE_QUERY:
			rc = kcndb_server_query_process(kn, kp, &kmh);
			break;
		case KCN_MSG_TYPE_ADD:
			rc = kcndb_server_add_process(kn, kp, &kmh);
			break;
		case KCN_MSG_TYPE_DEL: /* XXX */
		default:
			rc = false;
			errno = EOPNOTSUPP;
		}
		if (! rc) {
			if (errno == EAGAIN)
				kcn_pkt_start(kp);
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
	if (kt->kt_evb == NULL) {
		LOG(ERR, "event_init() failed: %s", strerror(errno));
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
