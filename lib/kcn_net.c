#include <sys/queue.h>

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <netinet/in.h>

#include "kcn.h"
#include "kcn_log.h"
#include "kcn_socket.h"
#include "kcn_pkt.h"
#include "kcn_net.h"

struct kcn_net {
	int kn_fd;
	struct kcn_pkt_data *kn_ipktdata;
	struct kcn_pkt_data *kn_opktdata;
	struct kcn_pkt_queue kn_opktq;
	void *kn_data;
};

struct kcn_net *
kcn_net_new(int fd, size_t size, void *data)
{
	struct kcn_net *kn;

	kn = malloc(sizeof(*kn));
	if (kn == NULL)
		return NULL;
	kn->kn_fd = fd;
	kn->kn_ipktdata = kcn_pkt_data_new(size);
	kn->kn_opktdata = kcn_pkt_data_new(size);
	kcn_pkt_queue_init(&kn->kn_opktq);
	kn->kn_data = data;
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

	kcn_socket_close(&kn->kn_fd);
	kcn_pkt_data_destroy(kn->kn_ipktdata);
	kcn_pkt_data_destroy(kn->kn_opktdata);
	kcn_pkt_purge(&kn->kn_opktq);
	free(kn);
}

int
kcn_net_fd(const struct kcn_net *kn)
{

	return kn->kn_fd;
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

bool
kcn_net_read(struct kcn_net *kn)
{
	struct kcn_pkt kp;
	ssize_t len;

	kcn_pkt_init(&kp, kn->kn_ipktdata);
	kcn_pkt_end(&kp);

	len = read(kn->kn_fd, kcn_pkt_tail(&kp), kcn_pkt_trailingspace(&kp));
	if (len == 0) {
		KCN_LOG(WARN, "disconnected");
		return false;
	}
	if (len == -1) {
		if (errno == EINTR ||
#if EWOULDBLOCK != EAGAIN
		    errno == EWOULDBLOCK ||
#endif /* EWOULDBLOCK != EAGAIN */
		    errno == EAGAIN)
			return true;
		KCN_LOG(WARN, "read() failed: %s", strerror(errno));
		return false;
	}
	KCN_LOG(DEBUG, "read %zu bytes", len);
	return true;
}

bool
kcn_net_write(struct kcn_net *kn)
{
	struct kcn_pkt kp;
	ssize_t len;

	if (! kcn_pkt_fetch(&kp, &kn->kn_opktq)) {
		assert(0);
		KCN_LOG(EMERG, "BUG: no packet to write, but called!!");
		return false;
	}
	len = write(kn->kn_fd, kcn_pkt_head(&kp), kcn_pkt_len(&kp));
	if (len == -1) {
		KCN_LOG(WARN, "write() failed: %s", strerror(errno));
		return false;
	}
	KCN_LOG(DEBUG, "write %zu bytes", len);
	kcn_pkt_trim_head(&kp, len);
	if (kcn_pkt_trailingdata(&kp) == 0)
		kcn_pkt_drop(&kp, &kn->kn_opktq);
	return true;
}
