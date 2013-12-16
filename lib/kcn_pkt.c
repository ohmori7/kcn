#include <sys/queue.h>

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "kcn_pkt.h"

struct kcn_pkt_data {
	STAILQ_ENTRY(kcn_pkt_data) kpd_chain;
	size_t kpd_size;
	size_t kpd_sp;
	size_t kpd_ep;
	uint8_t kpd_buf[1];
};

#define kp_size		kp_kpd->kpd_size
#define kp_sp		kp_kpd->kpd_sp
#define kp_ep		kp_kpd->kpd_ep
#define kp_buf		kp_kpd->kpd_buf

void
kcn_pkt_queue_init(struct kcn_pkt_queue *kpq)
{

	STAILQ_INIT(kpq);
}

void
kcn_pkt_init(struct kcn_pkt *kp, struct kcn_pkt_data *kpd)
{

	kp->kp_kpd = kpd;
	if (kpd != NULL)
		kcn_pkt_start(kp);
	else
		kp->kp_cp = 0;
}

struct kcn_pkt_data *
kcn_pkt_data_new(size_t size)
{
	struct kcn_pkt_data *kpd;

	/* XXX: should consider real size of allocated space. */
	kpd = malloc(offsetof(struct kcn_pkt_data, kpd_buf[size]));
	if (kpd == NULL)
		return NULL;
	kpd->kpd_size = size;
	kpd->kpd_sp = kpd->kpd_ep = 0;
	return kpd;
}

static struct kcn_pkt_data *
kcn_pkt_data_dup(const struct kcn_pkt *kp)
{
	struct kcn_pkt_data *kpd;
	size_t len;

	assert(kp->kp_kpd != NULL);
	len = kcn_pkt_len(kp);
	kpd = kcn_pkt_data_new(len);
	if (kpd == NULL)
		return NULL;
	memcpy(kpd->kpd_buf, kcn_pkt_head(kp), len);
	kpd->kpd_ep = len;
	return kpd;
}

void
kcn_pkt_data_destroy(struct kcn_pkt_data *kpd)
{

	free(kpd);
}

size_t
kcn_pkt_len(const struct kcn_pkt *kp)
{

	return kp->kp_ep - kp->kp_sp;
}

void
kcn_pkt_reset(struct kcn_pkt *kp, size_t headingspace)
{

	kp->kp_cp = kp->kp_sp = kp->kp_ep = headingspace;
}

#ifndef NDEBUG
static size_t
kcn_pkt_headingspace(const struct kcn_pkt *kp)
{

	return kp->kp_sp;
}
#endif /* ! NDEBUG */

static size_t
kcn_pkt_trailingspace(const struct kcn_pkt *kp)
{

	assert(kp->kp_size >= kp->kp_ep);
	return kp->kp_size - kp->kp_ep;
}

size_t
kcn_pkt_trailingdata(const struct kcn_pkt *kp)
{

	assert(kp->kp_ep >= kp->kp_cp);
	return kp->kp_ep - kp->kp_cp;
}

void
kcn_pkt_start(struct kcn_pkt *kp)
{

	kp->kp_cp = kp->kp_sp;
}

void
kcn_pkt_end(struct kcn_pkt *kp)
{

	kp->kp_cp = kp->kp_ep;
}

static void
kcn_pkt_trim_head(struct kcn_pkt *kp, size_t len)
{

	assert(kcn_pkt_len(kp) >= len);
	kp->kp_sp += len;
	if (kp->kp_sp > kp->kp_cp)
		kcn_pkt_start(kp);
}

void *
kcn_pkt_head(const struct kcn_pkt *kp)
{

	return kp->kp_buf + kp->kp_sp;
}

void *
kcn_pkt_current(const struct kcn_pkt *kp)
{

	return kp->kp_buf + kp->kp_cp;
}

void *
kcn_pkt_tail(const struct kcn_pkt *kp)
{

	return kp->kp_buf + kp->kp_ep;
}

void
kcn_pkt_prepend(struct kcn_pkt *kp, size_t len)
{

	assert(kcn_pkt_headingspace(kp) >= len);
	kp->kp_sp -= len;
	kp->kp_cp = kp->kp_sp;
}

static void
kcn_pkt_append(struct kcn_pkt *kp, size_t len)
{
	size_t trailingdata;

	trailingdata = kcn_pkt_trailingdata(kp);
	if (trailingdata >= len)
		return;
	kp->kp_ep += len - trailingdata;
	assert(kp->kp_ep <= kp->kp_size);
}

uint8_t
kcn_pkt_get8(struct kcn_pkt *kp)
{

	assert(kcn_pkt_trailingdata(kp) >= 1);
	return kp->kp_buf[kp->kp_cp++];
}

uint16_t
kcn_pkt_get16(struct kcn_pkt *kp)
{
	uint16_t v;

	assert(kcn_pkt_trailingdata(kp) >= 2);
	v =  (uint16_t)kp->kp_buf[kp->kp_cp++] << 8;
	v |= (uint16_t)kp->kp_buf[kp->kp_cp++];
	return v;
}

uint32_t
kcn_pkt_get32(struct kcn_pkt *kp)
{
	uint32_t v;

	assert(kcn_pkt_trailingdata(kp) >= 4);
	v =  (uint32_t)kp->kp_buf[kp->kp_cp++] << 24;
	v |= (uint32_t)kp->kp_buf[kp->kp_cp++] << 16;
	v |= (uint32_t)kp->kp_buf[kp->kp_cp++] <<  8;
	v |= (uint32_t)kp->kp_buf[kp->kp_cp++];
	return v;
}

uint64_t
kcn_pkt_get64(struct kcn_pkt *kp)
{
	uint64_t v;

	assert(kcn_pkt_trailingdata(kp) >= 8);
	v =  (uint64_t)kp->kp_buf[kp->kp_cp++] << 56;
	v |= (uint64_t)kp->kp_buf[kp->kp_cp++] << 48;
	v |= (uint64_t)kp->kp_buf[kp->kp_cp++] << 40;
	v |= (uint64_t)kp->kp_buf[kp->kp_cp++] << 32;
	v |= (uint64_t)kp->kp_buf[kp->kp_cp++] << 24;
	v |= (uint64_t)kp->kp_buf[kp->kp_cp++] << 16;
	v |= (uint64_t)kp->kp_buf[kp->kp_cp++] <<  8;
	v |= (uint64_t)kp->kp_buf[kp->kp_cp++];
	return v;
}

void
kcn_pkt_put8(struct kcn_pkt *kp, uint8_t v)
{

	kcn_pkt_append(kp, 1);
	kp->kp_buf[kp->kp_cp++] = v;
}

void
kcn_pkt_put16(struct kcn_pkt *kp, uint16_t v)
{

	kcn_pkt_append(kp, 2);
	kp->kp_buf[kp->kp_cp++] = (v >>  8) & 0xffU;
	kp->kp_buf[kp->kp_cp++] = v         & 0xffU;
}

void
kcn_pkt_put32(struct kcn_pkt *kp, uint32_t v)
{

	kcn_pkt_append(kp, 4);
	kp->kp_buf[kp->kp_cp++] = (v >> 24) & 0xffUL;
	kp->kp_buf[kp->kp_cp++] = (v >> 16) & 0xffUL;
	kp->kp_buf[kp->kp_cp++] = (v >>  8) & 0xffUL;
	kp->kp_buf[kp->kp_cp++] = v         & 0xffUL;
}

void
kcn_pkt_put64(struct kcn_pkt *kp, uint64_t v)
{

	kcn_pkt_append(kp, 8);
	kp->kp_buf[kp->kp_cp++] = (v >> 56) & 0xffULL;
	kp->kp_buf[kp->kp_cp++] = (v >> 48) & 0xffULL;
	kp->kp_buf[kp->kp_cp++] = (v >> 40) & 0xffULL;
	kp->kp_buf[kp->kp_cp++] = (v >> 32) & 0xffULL;
	kp->kp_buf[kp->kp_cp++] = (v >> 24) & 0xffULL;
	kp->kp_buf[kp->kp_cp++] = (v >> 16) & 0xffULL;
	kp->kp_buf[kp->kp_cp++] = (v >>  8) & 0xffULL;
	kp->kp_buf[kp->kp_cp++] = v         & 0xffULL;
}

void
kcn_pkt_put(struct kcn_pkt *kp, void *p, size_t len)
{

	kcn_pkt_append(kp, len);
	memcpy(kcn_pkt_current(kp), p, len);
}

bool
kcn_pkt_enqueue(struct kcn_pkt *kp, struct kcn_pkt_queue *kpq)
{
	struct kcn_pkt_data *kpd;

	assert(kp->kp_kpd != NULL);
	kpd = kcn_pkt_data_dup(kp);
	if (kpd == NULL)
		return false;
	STAILQ_INSERT_TAIL(kpq, kpd, kpd_chain);
	return true;
}

bool
kcn_pkt_fetch(struct kcn_pkt *kp, struct kcn_pkt_queue *kpq)
{
	struct kcn_pkt_data *kpd;

	kpd = STAILQ_FIRST(kpq);
	if (kpd == NULL)
		return false;
	kp->kp_kpd = kpd;
	kcn_pkt_start(kp);
	return true;
}

void
kcn_pkt_drop(struct kcn_pkt *kp, struct kcn_pkt_queue *kpq)
{

	assert(kp->kp_kpd != NULL);
	assert(kp->kp_kpd == STAILQ_FIRST(kpq));
	STAILQ_REMOVE_HEAD(kpq, kpd_chain);
	kcn_pkt_data_destroy(kp->kp_kpd);
	kcn_pkt_init(kp, NULL);
}

void
kcn_pkt_purge(struct kcn_pkt_queue *kpq)
{
	struct kcn_pkt kp;

	while (kcn_pkt_fetch(&kp, kpq))
		kcn_pkt_drop(&kp, kpq);
}

static int
kcn_pkt_error(ssize_t len, int error)
{

	if (len == 0)
		return ESHUTDOWN;
	if (len != -1)
		return 0;
#if EWOULDBLOCK != EAGAIN
	if (error == EWOULDBLOCK)
		error = EAGAIN;
#endif /* EWOULDBLOCK != EAGAIN */
	return error;
}

int
kcn_pkt_read(int fd, struct kcn_pkt *kp)
{
	ssize_t len;
	int error;

	for (;;) {
		len = read(fd, kcn_pkt_tail(kp), kcn_pkt_trailingspace(kp));
		error = kcn_pkt_error(len, errno);
		if (error != EINTR)
			break;
	}
	if (error == 0)
		kcn_pkt_append(kp, len);
	return error;
}

int
kcn_pkt_write(int fd, struct kcn_pkt *kp)
{
	ssize_t len;
	int error;

	for (;;) {
		len = write(fd, kcn_pkt_head(kp), kcn_pkt_trailingdata(kp));
		error = kcn_pkt_error(len, errno);
		if (error != EINTR)
			break;
	}
	if (error == 0)
		kcn_pkt_trim_head(kp, len);
	return error;
}
