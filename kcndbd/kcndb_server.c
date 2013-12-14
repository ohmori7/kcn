#include <sys/queue.h>

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include <pthread.h>

#include <netinet/in.h>

#include <event.h>

#include "kcn.h"
#include "kcn_log.h"
#include "kcn_socket.h"
#include "kcn_sockaddr.h"
#include "kcn_pkt.h"
#include "kcn_net.h"
#include "kcndb_server.h"

enum kcndb_thread_status {
	KCNDB_THREAD_STATUS_DEAD,
	KCNDB_THREAD_STATUS_RUN
};

struct kcndb_thread {
	enum kcndb_thread_status kt_status;
	pthread_t kt_id;
};

static int kcndb_server_socket = -1;
static in_port_t kcndb_server_port = KCN_HTONS(KCNDB_NET_PORT_DEFAULT);

#define KCNDB_SERVER_NWORKERS	8
static struct kcndb_thread kcndb_server_threads[KCNDB_SERVER_NWORKERS];

static void *kcndb_server_main(void *);
static bool kcndb_server_accept_set(struct event_base *, int);
static bool kcndb_server_read_set(struct kcn_net *);

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
	KCN_LOG(INFO, "server listen on %u.", ntohs(kcndb_server_port));

	for (i = 0; i < KCNDB_SERVER_NWORKERS; i++) {
		error = pthread_create(&kcndb_server_threads[i].kt_id, NULL,
		    kcndb_server_main, &kcndb_server_socket);
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

	kcn_socket_close(&kcndb_server_socket);
}

void
kcndb_server_loop(void)
{

	kcndb_server_main(&kcndb_server_socket);
}

static void
kcndb_server_disconnect(struct kcn_net *kn)
{

	kcn_net_destroy(kn);
	KCN_LOG(DEBUG, "disconnected from (host name: notyet)");
}

static void
kcndb_server_accept(int ls, short event, void *arg)
{
	struct sockaddr_storage ss;
	socklen_t sslen;
	struct event_base *evb;
	struct kcn_net *kn;
	int s;

	evb = arg;
	kcndb_server_accept_set(evb, ls);
	if ((event & EV_READ) == 0)
		return;
	s = accept(ls, (struct sockaddr *)&ss, &sslen);
	if (s == -1) {
		KCN_LOG(WARN, "accept() failed: %s", strerror(errno));
		return;
	}
	KCN_LOG(DEBUG, "connected from (host name: notyet)");

#define KCNDB_MSGSIZ	4096
	kn = kcn_net_new(s, KCNDB_MSGSIZ, evb);
	if (kn == NULL) {
		KCN_LOG(ERR, "cannot allocate net structure");
		goto bad;
	}

	if (! kcndb_server_read_set(kn))
		goto bad;
	return;
  bad:
	if (kn != NULL)
		kcn_net_destroy(kn);
	else
		kcn_socket_close(&s);
}

static void
kcndb_server_read(int s, short event, void *arg)
{
	struct kcn_net *kn;
	struct kcn_pkt kp;

	(void)s;
	kn = arg;
	if (event & EV_TIMEOUT && (event & EV_READ) == 0) {
		KCN_LOG(WARN, "connection timed out");
		goto bad;
	}
	if ((event & EV_READ) == 0) {
		KCN_LOG(WARN, "no read event but called. libevent bug???");
		goto out;
	}
	if (! kcn_net_read(kn))
		goto bad;
	kcn_net_ipkt(kn, &kp);
	while (kcn_pkt_trailingdata(&kp) > 0)
		KCN_LOG(DEBUG, "%02x\n", kcn_pkt_get8(&kp));
	kcn_pkt_reset(&kp);
  out:
	kcndb_server_read_set(kn);
	return;
  bad:
	kcndb_server_disconnect(kn);
}

static bool
kcndb_server_event_set(struct event_base *evb, int s,
    void (*cb)(int, short, void *), void *arg, struct timeval *tv,
    const char *name)
{
	short event;

	event = EV_READ;
	if (tv != NULL)
		event |= EV_TIMEOUT;
	if (event_base_once(evb, s, event, cb, arg, tv) == -1) {
		KCN_LOG(ERR, "cannot set %s event", name);
		return false;
	}
	KCN_LOG(DEBUG, "set %s event", name);
	return true;
}

static bool
kcndb_server_accept_set(struct event_base *evb, int ls)
{

	return kcndb_server_event_set(evb, ls, kcndb_server_accept, evb, NULL,
	    "accept");
}

static bool
kcndb_server_read_set(struct kcn_net *kn)
{
#define KCNDB_SERVER_READ_TIMEOUT	5
	struct timeval tv = {
		.tv_sec = KCNDB_SERVER_READ_TIMEOUT,
		.tv_usec = 0
	};

	return kcndb_server_event_set(kcn_net_data(kn), kcn_net_fd(kn),
	    kcndb_server_read, kn, &tv, "read");
}

static void *
kcndb_server_main(void *arg)
{
	int ls;
	struct event_base *evb;

	evb = event_init();
	if (evb == NULL) {
		KCN_LOG(ERR, "event_init() failed: %s", strerror(errno));
		goto out;
	}

	ls = *(int *)arg;
	if (! kcndb_server_accept_set(evb, ls))
		goto out;

	while (event_base_dispatch(evb) == -1) {
		if (errno != EINTR) {
			KCN_LOG(ERR, "event_base_dispatch() failed: %s",
			    strerror(errno));
			goto out;
		}
	}
  out:
	if (evb != NULL)
		event_base_free(evb);
	return NULL;
}
