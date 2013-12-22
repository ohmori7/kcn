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

enum kcn_net_state {
	KCN_NET_STATE_INIT,
	KCN_NET_STATE_PENDING,
	KCN_NET_STATE_ESTABLISHED,
	KCN_NET_STATE_DISCONNECTED
};

struct kcn_net {
	enum kcn_net_state kn_state;
	char kn_name[KCN_SOCKNAMELEN];
	struct event kn_evread;
	struct event kn_evwrite;
	struct kcn_pkt_data *kn_ipktdata;
	struct kcn_pkt_data *kn_opktdata;
	struct kcn_pkt_queue kn_opktq;
	int (*kn_readcb)(struct kcn_net *, struct kcn_pkt *, void *);
	void *kn_data;
};
#define kn_evb	kn_evread.ev_base
#define kn_fd	kn_evread.ev_fd

#define LOG(p, fmt, ...)						\
	KCN_LOG(p, "%s: " fmt, kn->kn_name, __VA_ARGS__)

#define KCN_NET_TIMEOUT	5
struct timeval kcn_net_timeouttv = {
	.tv_sec = KCN_NET_TIMEOUT,
	.tv_usec = 0
};

static bool kcn_net_write_enable(struct kcn_net *);
static void kcn_net_disconnect(struct kcn_net *);
static void kcn_net_read_cb(int, short, void *);
static void kcn_net_write_cb(int, short, void *);

struct kcn_net *
kcn_net_new(struct event_base *evb, int fd, size_t size, const char *name,
    int (*readcb)(struct kcn_net *, struct kcn_pkt *, void *), void *data)
{
	struct kcn_net *kn;

	kn = malloc(sizeof(*kn));
	if (kn == NULL)
		goto bad;
	kn->kn_state = KCN_NET_STATE_INIT;
	strlcpy(kn->kn_name, name, sizeof(kn->kn_name));
	event_set(&kn->kn_evread, fd, EV_READ | EV_TIMEOUT,
	    kcn_net_read_cb, kn);
	event_set(&kn->kn_evwrite, fd, EV_WRITE | EV_TIMEOUT,
	    kcn_net_write_cb, kn);
	kn->kn_ipktdata = kcn_pkt_data_new(size);
	kn->kn_opktdata = kcn_pkt_data_new(size);
	kcn_pkt_queue_init(&kn->kn_opktq);
	kn->kn_readcb = readcb;
	kn->kn_data = data;
	if (event_base_set(evb, &kn->kn_evread) == -1)
		goto bad;
	if (event_base_set(evb, &kn->kn_evwrite) == -1)
		goto bad;
	if (kn->kn_ipktdata == NULL || kn->kn_opktdata == NULL)
		goto bad;
	if (! kcn_net_write_enable(kn))
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
	kcn_net_disconnect(kn);
	kcn_pkt_data_destroy(kn->kn_ipktdata);
	kcn_pkt_data_destroy(kn->kn_opktdata);
	kcn_pkt_purge(&kn->kn_opktq);
	free(kn);
}

static const char *
kcn_net_state_ntoa(enum kcn_net_state state)
{

	switch (state) {
	case KCN_NET_STATE_INIT:		return "Init";
	case KCN_NET_STATE_PENDING:		return "Pending";
	case KCN_NET_STATE_ESTABLISHED:		return "Established";
	case KCN_NET_STATE_DISCONNECTED:	return "Disconnected";
	}
	return NULL; /* just in case. */
}

static void
kcn_net_state_change(struct kcn_net *kn, enum kcn_net_state nstate)
{
	enum kcn_net_state ostate;

	if (kn->kn_state == nstate)
		return;
	ostate = kn->kn_state;
	kn->kn_state = nstate;
	LOG(DEBUG, "state change from %s to %s",
	    kcn_net_state_ntoa(ostate), kcn_net_state_ntoa(nstate));
	if (ostate == KCN_NET_STATE_PENDING &&
	    nstate == KCN_NET_STATE_ESTABLISHED)
		kcn_net_read_enable(kn);
}

void
kcn_net_opkt(struct kcn_net *kn, struct kcn_pkt *kp)
{

	kcn_pkt_init(kp, kn->kn_opktdata);
}

static bool
kcn_net_event_enable(struct kcn_net *kn, struct event *ev, const char *name)
{

	if (event_pending(ev, EV_TIMEOUT, NULL))
		return true;
	if (event_add(ev, &kcn_net_timeouttv) == -1) {
		LOG(WARN, "cannot enable %s event", name);
		return false;
	}
	LOG(DEBUG, "enable %s event with %lld sec timeout",
	    name, (long long)kcn_net_timeouttv.tv_sec);
	return true;
}

bool
kcn_net_read_enable(struct kcn_net *kn)
{

	if (kn->kn_state != KCN_NET_STATE_ESTABLISHED) {
		kcn_net_state_change(kn, KCN_NET_STATE_PENDING);
		return true;
	}
	return kcn_net_event_enable(kn, &kn->kn_evread, "read");
}

static bool
kcn_net_write_enable(struct kcn_net *kn)
{

	return kcn_net_event_enable(kn, &kn->kn_evwrite, "write");
}

static void
kcn_net_disconnect(struct kcn_net *kn)
{

	if (kn->kn_state == KCN_NET_STATE_DISCONNECTED)
		return;
	event_del(&kn->kn_evread);
	event_del(&kn->kn_evwrite);
	kcn_socket_close(&kn->kn_fd);
	kcn_net_state_change(kn, KCN_NET_STATE_DISCONNECTED);
}

static void
kcn_net_timeout(struct kcn_net *kn, short event)
{

	LOG(DEBUG, "%s timeout", event & EV_READ ? "read" : "write");
	kcn_net_disconnect(kn);
}

static bool
kcn_net_event_check(struct kcn_net *kn, short events, short event)
{

	if ((events & event) == 0) {
		if (events & EV_TIMEOUT)
			kcn_net_timeout(kn, event);
		return false;
	}
	return true;
}

static void
kcn_net_read_cb(int fd, short events, void *arg)
{
	struct kcn_net *kn = arg;
	struct kcn_pkt kp;
	int error;

	assert(kn->kn_fd == fd);
	(void)fd;
	if (! kcn_net_event_check(kn, events, EV_READ))
		return;

	kcn_pkt_init(&kp, kn->kn_ipktdata);

	error = kcn_pkt_read(kn->kn_fd, &kp);
	if (error == EAGAIN)
		goto out;
	if (error != 0) {
		LOG(DEBUG, "read() failed: %s", strerror(error));
		goto out;
	}
	error = (*kn->kn_readcb)(kn, &kp, kn->kn_data);
  out:
	if (error == EAGAIN)
		kcn_net_read_enable(kn);
	else if (error != 0)
		kcn_net_disconnect(kn);
}

bool
kcn_net_write(struct kcn_net *kn, struct kcn_pkt *kp)
{

	if (! kcn_pkt_enqueue(kp, &kn->kn_opktq)) {
		LOG(ERR, "cannot enqueue packet: %s", strerror(errno));
		return false;
	}
	if (! kcn_net_write_enable(kn))
		return false;
	return true;
}

static void
kcn_net_write_cb(int fd, short events, void *arg)
{
	struct kcn_net *kn = arg;
	struct kcn_pkt kp;
	int error;

	assert(kn->kn_fd == fd);
	(void)fd;
	if (! kcn_net_event_check(kn, events, EV_WRITE))
		return;

	kcn_net_state_change(kn, KCN_NET_STATE_ESTABLISHED);

	while (kcn_pkt_fetch(&kp, &kn->kn_opktq)) {
		error = kcn_pkt_write(kn->kn_fd, &kp);
		if (error == EAGAIN) {
			kcn_net_write_enable(kn);
			return;
		}
		if (error != 0) {
			LOG(WARN, "write() failed: %s", strerror(errno));
			kcn_net_disconnect(kn);
			return;
		}
		if (kcn_pkt_trailingdata(&kp) == 0)
			kcn_pkt_drop(&kp, &kn->kn_opktq);
	}
}

bool
kcn_net_loop(struct kcn_net *kn)
{

	if (event_base_dispatch(kn->kn_evb) == -1) {
		LOG(ERR, "event dispatch failed: %s", strerror(errno));
		return false;
	}
	LOG(DEBUG, "event dispatch finishes", "");
	return true;
}
