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
kcndb_server_accept(int ls, short event, void *arg)
{
	struct sockaddr_storage ss;
	socklen_t sslen;
	struct event_base *evb;
	struct kcn_net *kn;
	int s;

	if ((event & EV_READ) == 0) {
		KCN_LOG(DEBUG, "no read event but called. libevent bug???");
		return;
	}

	evb = arg;
	s = accept(ls, (struct sockaddr *)&ss, &sslen);
	if (s == -1) {
		KCN_LOG(WARN, "accept() failed: %s", strerror(errno));
		return;
	}
	KCN_LOG(DEBUG, "connected from (host name: notyet)");

#define KCNDB_MSGSIZ	4096
	kn = kcn_net_new(evb, s, KCNDB_MSGSIZ, NULL);
	if (kn == NULL)
		KCN_LOG(ERR, "cannot allocate net structure");
}

static void *
kcndb_server_main(void *arg)
{
	int ls;
	struct event_base *evb;
	struct event ev;

	evb = event_init();
	if (evb == NULL) {
		KCN_LOG(ERR, "event_init() failed: %s", strerror(errno));
		goto out;
	}

	ls = *(int *)arg;
	event_set(&ev, ls, EV_READ | EV_PERSIST, kcndb_server_accept, evb);
	if (event_base_set(evb, &ev) == -1) {
		KCN_LOG(ERR, "cannot set base for accept event");
		goto out;
	}
	if (event_add(&ev, NULL) == -1) {
		KCN_LOG(ERR, "cannot set accept event");
		goto out;
	}

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
