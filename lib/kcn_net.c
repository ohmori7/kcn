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
#include "kcn_buf.h"
#include "kcn_net.h"

/* hack for libevent 1.4.13-4.el6 on CentOS 6.x. */
#ifdef __linux__
#define HAVE_LIBEVENT_RAISE_EV_READ_WITH_EINPROGRESS
#define HAVE_LIBEVENT_RAISE_EV_TIMEOUT_BEFORE_TIMEOUT
#endif /* __Linux__ */

enum kcn_net_state {
	KCN_NET_STATE_INIT,
#ifdef HAVE_LIBEVENT_RAISE_EV_READ_WITH_EINPROGRESS
	KCN_NET_STATE_PENDING,
#endif /* HAVE_LIBEVENT_RAISE_EV_READ_WITH_EINPROGRESS */
	KCN_NET_STATE_ESTABLISHED,
	KCN_NET_STATE_DISCONNECTED
};

struct kcn_net {
	enum kcn_net_state kn_state;
	char kn_name[KCN_SOCKNAMELEN];
	struct event kn_evread;
	struct event kn_evwrite;
	struct kcn_buf kn_ibuf;
	struct kcn_buf_data *kn_ibufdata;
	struct kcn_buf_data *kn_obufdata;
	struct kcn_buf_queue kn_obufq;
	int (*kn_readcb)(struct kcn_net *, struct kcn_buf *, void *);
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
    int (*readcb)(struct kcn_net *, struct kcn_buf *, void *), void *data)
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
	kn->kn_ibufdata = kcn_buf_data_new(size);
	kcn_buf_init(&kn->kn_ibuf, kn->kn_ibufdata);
	kn->kn_obufdata = kcn_buf_data_new(size);
	kcn_buf_queue_init(&kn->kn_obufq);
	kn->kn_readcb = readcb;
	kn->kn_data = data;
	if (event_base_set(evb, &kn->kn_evread) == -1)
		goto bad;
	if (event_base_set(evb, &kn->kn_evwrite) == -1)
		goto bad;
	if (kn->kn_ibufdata == NULL || kn->kn_obufdata == NULL)
		goto bad;
#ifdef HAVE_LIBEVENT_RAISE_EV_READ_WITH_EINPROGRESS
	if (! kcn_net_write_enable(kn))
		goto bad;
#endif /* HAVE_LIBEVENT_RAISE_EV_READ_WITH_EINPROGRESS */
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
	kcn_buf_data_destroy(kn->kn_ibufdata);
	kcn_buf_data_destroy(kn->kn_obufdata);
	kcn_buf_purge(&kn->kn_obufq);
	free(kn);
}

static const char *
kcn_net_state_ntoa(enum kcn_net_state state)
{

	switch (state) {
	case KCN_NET_STATE_INIT:		return "Init";
#ifdef HAVE_LIBEVENT_RAISE_EV_READ_WITH_EINPROGRESS
	case KCN_NET_STATE_PENDING:		return "Pending";
#endif /* HAVE_LIBEVENT_RAISE_EV_READ_WITH_EINPROGRESS */
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
#ifdef HAVE_LIBEVENT_RAISE_EV_READ_WITH_EINPROGRESS
	if (ostate == KCN_NET_STATE_PENDING &&
	    nstate == KCN_NET_STATE_ESTABLISHED)
		kcn_net_read_enable(kn);
#endif /* HAVE_LIBEVENT_RAISE_EV_READ_WITH_EINPROGRESS */
}

struct event_base *
kcn_net_event_base(const struct kcn_net *kn)
{

	return kn->kn_evb;
}

void
kcn_net_obuf(struct kcn_net *kn, struct kcn_buf *kb)
{

	kcn_buf_init(kb, kn->kn_obufdata);
}

void *
kcn_net_data(const struct kcn_net *kn)
{

	return kn->kn_data;
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

	assert(kn->kn_state != KCN_NET_STATE_DISCONNECTED);
#ifdef HAVE_LIBEVENT_RAISE_EV_READ_WITH_EINPROGRESS
	if (kn->kn_state != KCN_NET_STATE_ESTABLISHED) {
		kcn_net_state_change(kn, KCN_NET_STATE_PENDING);
		return true;
	}
#endif /* HAVE_LIBEVENT_RAISE_EV_READ_WITH_EINPROGRESS */
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
	kcn_net_state_change(kn, KCN_NET_STATE_ESTABLISHED);
	return true;
}

static void
kcn_net_read_cb(int fd, short events, void *arg)
{
	struct kcn_net *kn = arg;
	int error;

	assert(kn->kn_fd == fd);
	(void)fd;
	if (! kcn_net_event_check(kn, events, EV_READ))
		return;

	error = kcn_buf_read(kn->kn_fd, &kn->kn_ibuf);
	if (error == EAGAIN)
		goto out;
	if (error != 0) {
		LOG(DEBUG, "read() failed: %s", strerror(error));
		goto out;
	}
	error = (*kn->kn_readcb)(kn, &kn->kn_ibuf, kn->kn_data);
  out:
	if (error == EAGAIN)
		kcn_net_read_enable(kn);
	else if (error != 0)
		kcn_net_disconnect(kn);
}

bool
kcn_net_write(struct kcn_net *kn, struct kcn_buf *kb)
{

	if (kn->kn_state == KCN_NET_STATE_DISCONNECTED) {
		errno = ENETDOWN;
		LOG(ERR, "cannot enqueue packet", strerror(errno));
		return false;
	}
	if (! kcn_buf_enqueue(kb, &kn->kn_obufq)) {
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
	struct kcn_buf kb;
	int error;

	assert(kn->kn_fd == fd);
	(void)fd;
#ifdef HAVE_LIBEVENT_RAISE_EV_TIMEOUT_BEFORE_TIMEOUT
	if (kn->kn_state == KCN_NET_STATE_PENDING &&
	    (events & (EV_WRITE | EV_TIMEOUT)) == EV_TIMEOUT) {
		LOG(DEBUG, "fix wrong timeout event without write event", "");
		kcn_net_state_change(kn, KCN_NET_STATE_ESTABLISHED);
		return;
	}
#endif /* HAVE_LIBEVENT_RAISE_EV_TIMEOUT_BEFORE_TIMEOUT */
	if (! kcn_net_event_check(kn, events, EV_WRITE))
		return;

	while (kcn_buf_fetch(&kb, &kn->kn_obufq)) {
		error = kcn_buf_write(kn->kn_fd, &kb);
		if (error == EAGAIN) {
			kcn_net_write_enable(kn);
			return;
		}
		if (error != 0) {
			LOG(WARN, "write() failed: %s", strerror(errno));
			kcn_net_disconnect(kn);
			return;
		}
		if (kcn_buf_trailingdata(&kb) == 0)
			kcn_buf_drop(&kb, &kn->kn_obufq);
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
