#include <sys/queue.h>

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>

#include <pthread.h>

#include <netinet/in.h>

#include <event.h>

#include "kcn.h"
#include "kcn_info.h"
#include "kcn_formula.h"
#include "kcn_log.h"
#include "kcn_socket.h"
#include "kcn_sockaddr.h"
#include "kcn_pkt.h"
#include "kcn_net.h"
#include "kcn_msg.h"
#include "kcn_netstat.h"
#include "kcndb_db.h"
#include "kcndb_server.h"

enum kcndb_thread_status {
	KCNDB_THREAD_STATUS_DEAD,
	KCNDB_THREAD_STATUS_RUN
};

struct kcndb_thread {
	enum kcndb_thread_status kt_status;
	pthread_t kt_id;
	int kt_listenfd;
	struct event_base *kt_evb;
};

static int kcndb_server_socket = -1;
static in_port_t kcndb_server_port = KCN_HTONS(KCN_NETSTAT_PORT_DEFAULT);

#define KCNDB_SERVER_NWORKERS	8
static struct kcndb_thread kcndb_server_threads[KCNDB_SERVER_NWORKERS];

static void *kcndb_server_main(void *);

static void
kcndb_thread_init(struct kcndb_thread *kt)
{

	kt->kt_listenfd = kcndb_server_socket;
}

bool
kcndb_server_port_set(uint16_t port)
{

	kcndb_server_port = htons(port);
	return true;
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

	for (i = 0; i < KCNDB_SERVER_NWORKERS; i++) {
		kcndb_thread_init(&kcndb_server_threads[i]);
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
	struct kcndb_thread kt;

	kcndb_thread_init(&kt);
	kt.kt_id = pthread_self();
	kt.kt_status = KCNDB_THREAD_STATUS_RUN;
	kcndb_server_main(&kt);
}

static bool
kcndb_server_query_process(struct kcn_net *kn, struct kcn_pkt *ikp)
{
	struct kcn_pkt okp;
	struct kcn_msg_query kmq;
	struct kcn_msg_response kmr;
	struct kcn_info *ki;

	if (! kcn_msg_query_decode(ikp, &kmq))
		ki = NULL;
	else
		/* XXX */
		ki = kcn_info_new(KCN_LOC_TYPE_DOMAINNAME, kmq.kmq_maxcount);

	kcn_msg_response_init(&kmr, ki);
	if (ki == NULL || ! kcndb_db_search_all(ki, &kmq.kmq_formula))
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

static int
kcndb_server_recv(struct kcn_net *kn, struct kcn_pkt *kp, void *arg)
{
	struct kcndb_thread *kt = arg;
	struct kcn_msg_header kmh;
	bool rc;

	KCN_LOG(DEBUG, "%u: recv %u bytes", kt->kt_id,
	    kcn_pkt_trailingdata(kp));

	if (! kcn_msg_header_decode(kp, &kmh))
		goto bad;

	switch (kmh.kmh_type) {
	case KCN_MSG_TYPE_QUERY:
		rc = kcndb_server_query_process(kn, kp);
		break;
	case KCN_MSG_TYPE_ADD: /* XXX */
	case KCN_MSG_TYPE_DEL: /* XXX */
	default:
		rc = false;
		errno = EOPNOTSUPP;
	}
	if (! rc)
		goto bad;

	return EAGAIN;
  bad:
	if (errno != EAGAIN)
		KCN_LOG(ERR, "recv invalid message: %s", strerror(errno));
	return errno;
}

static void
kcndb_server_session(struct kcndb_thread *kt, int fd)
{
	struct kcn_net *kn;

	kn = kcn_net_new(kt->kt_evb, fd, KCN_MSG_MAXSIZ, kcndb_server_recv, kt);
	if (kn == NULL) {
		KCN_LOG(ERR, "cannot allocate network structure");
		kcn_socket_close(&fd);
		return;
	}
	kcn_net_read_enable(kn);
	kcn_net_loop(kn);
	kcn_net_destroy(kn);
}

static void *
kcndb_server_main(void *arg)
{
	struct kcndb_thread *kt;
	int fd;

	kt = arg;
	kt->kt_evb = event_init();
	if (kt->kt_evb == NULL) {
		KCN_LOG(ERR, "event_init() failed: %s", strerror(errno));
		goto out;
	}

	for (;;) {
		fd = kcn_socket_accept(kt->kt_listenfd);
		if (fd == -1)
			continue;
		kcndb_server_session(kt, fd);
	}
  out:
	if (kt->kt_evb != NULL)
		event_base_free(kt->kt_evb);
	return NULL;
}
