#include <sys/queue.h>

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <netinet/in.h>

#include <event.h>

#include "kcn.h"
#include "kcn_log.h"
#include "kcn_socket.h"
#include "kcn_pkt.h"
#include "kcn_net.h"

struct kcn_net {
	struct event kn_ev;
	struct kcn_pkt_data *kn_ipktdata;
	struct kcn_pkt_data *kn_opktdata;
	struct kcn_pkt_queue kn_opktq;
	void *kn_data;
};
#define kn_base	kn_ev.ev_base
#define kn_fd	kn_ev.ev_fd

static void kcn_net_event(int, short, void *);

struct kcn_net *
kcn_net_new(struct event_base *evb, int fd, size_t size, void *data)
{
	struct kcn_net *kn;
#define KCN_NET_TIMEOUT	5
	struct timeval tv = {
		.tv_sec = KCN_NET_TIMEOUT,
		.tv_usec = 0
	};

	kn = malloc(sizeof(*kn));
	if (kn == NULL)
		goto bad;
	event_set(&kn->kn_ev, fd, EV_READ | EV_TIMEOUT, kcn_net_event, kn);
	kn->kn_ipktdata = kcn_pkt_data_new(size);
	kn->kn_opktdata = kcn_pkt_data_new(size);
	kcn_pkt_queue_init(&kn->kn_opktq);
	kn->kn_data = data;
	if (event_base_set(evb, &kn->kn_ev) == -1)
		goto bad;
	if (event_add(&kn->kn_ev, &tv) == -1)
		goto bad;
	if (kn->kn_ipktdata == NULL || kn->kn_opktdata == NULL)
		goto bad;
	return kn;
  bad:
	kcn_net_destroy(kn);
	return NULL;
}

void
kcn_net_destroy(struct kcn_net *kn)
{

	if (kn == NULL)
		return;
	event_del(&kn->kn_ev);
	kcn_socket_close(&kn->kn_fd);
	kcn_pkt_data_destroy(kn->kn_ipktdata);
	kcn_pkt_data_destroy(kn->kn_opktdata);
	kcn_pkt_purge(&kn->kn_opktq);
	free(kn);
}

void
kcn_net_ipkt(struct kcn_net *kn, struct kcn_pkt *kp)
{

	kcn_pkt_init(kp, kn->kn_ipktdata);
}

void
kcn_net_opkt(struct kcn_net *kn, struct kcn_pkt *kp)
{

	kcn_pkt_init(kp, kn->kn_opktdata);
}

void *
kcn_net_data(const struct kcn_net *kn)
{

	return kn->kn_data;
}

static bool
kcn_net_read_cb(struct kcn_net *kn)
{
	struct kcn_pkt kp;
	int error;

	kcn_pkt_init(&kp, kn->kn_ipktdata);
	kcn_pkt_end(&kp);

	error = kcn_pkt_read(kn->kn_fd, &kp);
	if (error == EAGAIN)
		goto out;
	if (error != 0) {
		KCN_LOG(WARN, "read() failed: %s", strerror(errno));
		return false;
	}
#if 1
	while (kcn_pkt_trailingdata(&kp) > 0)
		KCN_LOG(DEBUG, "%02x", kcn_pkt_get8(&kp));
	kcn_pkt_reset(&kp);
#endif /* 1 */
  out:
	return true;
}

bool
kcn_net_write(struct kcn_net *kn)
{

	/* XXX */
	(void)kn;
	return true;
}

static bool
kcn_net_write_cb(struct kcn_net *kn)
{
	struct kcn_pkt kp;
	int error;

	if (! kcn_pkt_fetch(&kp, &kn->kn_opktq))
		return false;
	error = kcn_pkt_write(kn->kn_fd, &kp);
	if (error != 0) {
		KCN_LOG(WARN, "write() failed: %s", strerror(errno));
		return false;
	}
	if (kcn_pkt_trailingdata(&kp) == 0)
		kcn_pkt_drop(&kp, &kn->kn_opktq);
	return true;
}

static void
kcn_net_timeout(struct kcn_net *kn)
{

	KCN_LOG(DEBUG, "disconnected");
	kcn_net_destroy(kn);
}

static void
kcn_net_event(int fd, short event, void *arg)
{
	struct kcn_net *kn = arg;

	(void)fd;
	if (event & EV_READ)
		kcn_net_read_cb(kn);
	if (event & EV_WRITE)
		while (kcn_net_write_cb(kn))
			;
	if (event & EV_TIMEOUT)
		kcn_net_timeout(kn);
}
