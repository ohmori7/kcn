#include <sys/queue.h>

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "kcn_pkt.h"

struct kcn_pkt_data {
	STAILQ_ENTRY(kcn_pkt_data) kpd_chain;
	size_t kpd_size;
	size_t kpd_sp;
	size_t kpd_ep;
	uint8_t kpd_buf[1];
};

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
		kp->kp_cp = kpd->kpd_sp;
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
	memcpy(kpd->kpd_buf, kcn_pkt_sp(kp), len);
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
	struct kcn_pkt_data *kpd = kp->kp_kpd;

	return kpd->kpd_ep - kpd->kpd_sp;
}

void
kcn_pkt_reset(struct kcn_pkt *kp)
{
	struct kcn_pkt_data *kpd = kp->kp_kpd;

	kpd->kpd_sp = kpd->kpd_ep = 0;
	kp->kp_cp = 0;
}

size_t
kcn_pkt_trailingspace(const struct kcn_pkt *kp)
{

	assert(kp->kp_kpd->kpd_size >= kp->kp_cp);
	return kp->kp_kpd->kpd_size - kp->kp_cp;
}

size_t
kcn_pkt_trailingdata(const struct kcn_pkt *kp)
{

	assert(kp->kp_kpd->kpd_ep >= kp->kp_cp);
	return kp->kp_kpd->kpd_ep - kp->kp_cp;
}

void
kcn_pkt_trim_head(struct kcn_pkt *kp, size_t len)
{
	struct kcn_pkt_data *kpd = kp->kp_kpd;

	assert(kcn_pkt_len(kp) >= len);
	kpd->kpd_sp += len;
	if (kpd->kpd_sp > kp->kp_cp)
		kp->kp_cp = kpd->kpd_sp;
}

void *
kcn_pkt_sp(const struct kcn_pkt *kp)
{
	struct kcn_pkt_data *kpd = kp->kp_kpd;

	return kpd->kpd_buf + kpd->kpd_sp;
}

void *
kcn_pkt_cp(const struct kcn_pkt *kp)
{

	return kp->kp_kpd->kpd_buf + kp->kp_cp;
}

static void
kcn_pkt_append(struct kcn_pkt *kp, size_t len)
{
	struct kcn_pkt_data *kpd = kp->kp_kpd;
	size_t trailingspace;

	trailingspace = kcn_pkt_trailingspace(kp);
	if (trailingspace >= len)
		return;
	kpd->kpd_ep += len - trailingspace;
	assert(kpd->kpd_ep <= kpd->kpd_size);
}

uint8_t
kcn_pkt_get8(struct kcn_pkt *kp)
{
	struct kcn_pkt_data *kpd = kp->kp_kpd;

	assert(kcn_pkt_trailingdata(kp) >= 1);
	return kpd->kpd_buf[kp->kp_cp++];
}

uint16_t
kcn_pkt_get16(struct kcn_pkt *kp)
{
	struct kcn_pkt_data *kpd = kp->kp_kpd;
	uint16_t v;

	assert(kcn_pkt_trailingdata(kp) >= 2);
	v =  (uint16_t)kpd->kpd_buf[kp->kp_cp++] << 8;
	v |= (uint16_t)kpd->kpd_buf[kp->kp_cp++];
	return v;
}

uint32_t
kcn_pkt_get32(struct kcn_pkt *kp)
{
	struct kcn_pkt_data *kpd = kp->kp_kpd;
	uint32_t v;

	assert(kcn_pkt_trailingdata(kp) >= 4);
	v =  (uint32_t)kpd->kpd_buf[kp->kp_cp++] << 24;
	v |= (uint32_t)kpd->kpd_buf[kp->kp_cp++] << 16;
	v |= (uint32_t)kpd->kpd_buf[kp->kp_cp++] <<  8;
	v |= (uint32_t)kpd->kpd_buf[kp->kp_cp++];
	return v;
}

uint64_t
kcn_pkt_get64(struct kcn_pkt *kp)
{
	struct kcn_pkt_data *kpd = kp->kp_kpd;
	uint64_t v;

	assert(kcn_pkt_trailingdata(kp) >= 8);
	v =  (uint64_t)kpd->kpd_buf[kp->kp_cp++] << 56;
	v |= (uint64_t)kpd->kpd_buf[kp->kp_cp++] << 48;
	v |= (uint64_t)kpd->kpd_buf[kp->kp_cp++] << 40;
	v |= (uint64_t)kpd->kpd_buf[kp->kp_cp++] << 32;
	v |= (uint64_t)kpd->kpd_buf[kp->kp_cp++] << 24;
	v |= (uint64_t)kpd->kpd_buf[kp->kp_cp++] << 16;
	v |= (uint64_t)kpd->kpd_buf[kp->kp_cp++] <<  8;
	v |= (uint64_t)kpd->kpd_buf[kp->kp_cp++];
	return v;
}

void
kcn_pkt_put8(struct kcn_pkt *kp, uint8_t v)
{
	struct kcn_pkt_data *kpd = kp->kp_kpd;

	kcn_pkt_append(kp, 1);
	kpd->kpd_buf[kp->kp_cp++] = v;
}

void
kcn_pkt_put16(struct kcn_pkt *kp, uint16_t v)
{
	struct kcn_pkt_data *kpd = kp->kp_kpd;

	kcn_pkt_append(kp, 2);
	kpd->kpd_buf[kp->kp_cp++] = (v >>  8) & 0xffU;
	kpd->kpd_buf[kp->kp_cp++] = v         & 0xffU;
}

void
kcn_pkt_put32(struct kcn_pkt *kp, uint32_t v)
{
	struct kcn_pkt_data *kpd = kp->kp_kpd;

	kcn_pkt_append(kp, 4);
	kpd->kpd_buf[kp->kp_cp++] = (v >> 24) & 0xffUL;
	kpd->kpd_buf[kp->kp_cp++] = (v >> 16) & 0xffUL;
	kpd->kpd_buf[kp->kp_cp++] = (v >>  8) & 0xffUL;
	kpd->kpd_buf[kp->kp_cp++] = v         & 0xffUL;
}

void
kcn_pkt_put64(struct kcn_pkt *kp, uint64_t v)
{
	struct kcn_pkt_data *kpd = kp->kp_kpd;

	kcn_pkt_append(kp, 8);
	kpd->kpd_buf[kp->kp_cp++] = (v >> 56) & 0xffULL;
	kpd->kpd_buf[kp->kp_cp++] = (v >> 48) & 0xffULL;
	kpd->kpd_buf[kp->kp_cp++] = (v >> 40) & 0xffULL;
	kpd->kpd_buf[kp->kp_cp++] = (v >> 32) & 0xffULL;
	kpd->kpd_buf[kp->kp_cp++] = (v >> 24) & 0xffULL;
	kpd->kpd_buf[kp->kp_cp++] = (v >> 16) & 0xffULL;
	kpd->kpd_buf[kp->kp_cp++] = (v >>  8) & 0xffULL;
	kpd->kpd_buf[kp->kp_cp++] = v         & 0xffULL;
}

void
kcn_pkt_put(struct kcn_pkt *kp, void *p, size_t len)
{

	kcn_pkt_append(kp, len);
	memcpy(kcn_pkt_cp(kp), p, len);
}

bool
kcn_pkt_enqueue(struct kcn_pkt *kp, struct kcn_pkt_queue *kpq)
{
	struct kcn_pkt_data *kpd;

	assert(kp->kp_kpd != NULL);
	kpd = kcn_pkt_data_dup(kp);
	kcn_pkt_reset(kp);
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
	kp->kp_cp = kpd->kpd_sp;
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
