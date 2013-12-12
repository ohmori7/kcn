#include <sys/queue.h>

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "kcn_pkt.h"

struct kcn_pkt {
	STAILQ_ENTRY(kcn_pkt) kp_chain;
	size_t kp_size;
	size_t kp_sp;
	size_t kp_ep;
	uint8_t kp_buf[1];
};

void
kcn_pkt_queue_init(struct kcn_pkt_queue *kpq)
{

	STAILQ_INIT(kpq);
}

void
kcn_pkt_handle_init(struct kcn_pkt_handle *kph, struct kcn_pkt_queue *kpq)
{

	kph->kph_q = kpq;
	/* these are just for debug purpose. */
	kph->kph_kp = NULL;
	kph->kph_cp = 0;
}

static struct kcn_pkt *
_kcn_pkt_new(size_t size)
{
	struct kcn_pkt *kp;

	/* XXX: should consider real size of allocated space. */
	kp = malloc(offsetof(struct kcn_pkt, kp_buf[size]));
	if (kp == NULL)
		return NULL;
	kp->kp_size = size;
	return kp;
}

static struct kcn_pkt *
_kcn_pkt_dup(const struct kcn_pkt_handle *kph)
{
	struct kcn_pkt *kp;
	size_t len;

	assert(kph->kph_kp != NULL);
	len = kcn_pkt_len(kph);
	kp = _kcn_pkt_new(len);
	if (kp == NULL)
		return NULL;
	memcpy(kp->kp_buf, kcn_pkt_sp(kph), len);
	kp->kp_ep = len;
	return kp;
}

bool
kcn_pkt_new(struct kcn_pkt_handle *kph, size_t size)
{

	kph->kph_kp = _kcn_pkt_new(size);
	if (kph->kph_kp == NULL)
		return false;
	kcn_pkt_reset(kph);
	return true;
}

void
kcn_pkt_destroy(struct kcn_pkt_handle *kph)
{

	if (kph->kph_kp == NULL)
		return;
	free(kph->kph_kp);
	kph->kph_kp = NULL;
}

size_t
kcn_pkt_len(const struct kcn_pkt_handle *kph)
{
	struct kcn_pkt *kp = kph->kph_kp;

	return kp->kp_ep - kp->kp_sp;
}

void
kcn_pkt_reset(struct kcn_pkt_handle *kph)
{
	struct kcn_pkt *kp = kph->kph_kp;

	kp->kp_sp = kp->kp_ep = 0;
	kph->kph_cp = 0;
}

size_t
kcn_pkt_trailingspace(const struct kcn_pkt_handle *kph)
{

	assert(kph->kph_kp->kp_size >= kph->kph_cp);
	return kph->kph_kp->kp_size - kph->kph_cp;
}

size_t
kcn_pkt_trailingdata(const struct kcn_pkt_handle *kph)
{

	assert(kph->kph_kp->kp_ep >= kph->kph_cp);
	return kph->kph_kp->kp_ep - kph->kph_cp;
}

void
kcn_pkt_trim_head(struct kcn_pkt_handle *kph, size_t len)
{
	struct kcn_pkt *kp = kph->kph_kp;

	assert(kcn_pkt_len(kph) >= len);
	kp->kp_sp += len;
	if (kp->kp_sp > kph->kph_cp)
		kph->kph_cp = kp->kp_sp;
}

void *
kcn_pkt_sp(const struct kcn_pkt_handle *kph)
{
	struct kcn_pkt *kp = kph->kph_kp;

	return kp->kp_buf + kp->kp_sp;
}

void *
kcn_pkt_cp(const struct kcn_pkt_handle *kph)
{

	return kph->kph_kp->kp_buf + kph->kph_cp;
}

static void
kcn_pkt_append(struct kcn_pkt_handle *kph, size_t len)
{
	struct kcn_pkt *kp = kph->kph_kp;
	size_t trailingspace;

	trailingspace = kcn_pkt_trailingspace(kph);
	if (trailingspace >= len)
		return;
	kp->kp_ep += len - trailingspace;
	assert(kp->kp_ep <= kp->kp_size);
}

uint8_t
kcn_pkt_get8(struct kcn_pkt_handle *kph)
{
	struct kcn_pkt *kp = kph->kph_kp;

	assert(kcn_pkt_trailingdata(kph) >= 1);
	return kp->kp_buf[kph->kph_cp++];
}

uint16_t
kcn_pkt_get16(struct kcn_pkt_handle *kph)
{
	struct kcn_pkt *kp = kph->kph_kp;
	uint16_t v;

	assert(kcn_pkt_trailingdata(kph) >= 2);
	v =  (uint16_t)kp->kp_buf[kph->kph_cp++] << 8;
	v |= (uint16_t)kp->kp_buf[kph->kph_cp++];
	return v;
}

uint32_t
kcn_pkt_get32(struct kcn_pkt_handle *kph)
{
	struct kcn_pkt *kp = kph->kph_kp;
	uint32_t v;

	assert(kcn_pkt_trailingdata(kph) >= 4);
	v =  (uint32_t)kp->kp_buf[kph->kph_cp++] << 24;
	v |= (uint32_t)kp->kp_buf[kph->kph_cp++] << 16;
	v |= (uint32_t)kp->kp_buf[kph->kph_cp++] <<  8;
	v |= (uint32_t)kp->kp_buf[kph->kph_cp++];
	return v;
}

uint64_t
kcn_pkt_get64(struct kcn_pkt_handle *kph)
{
	struct kcn_pkt *kp = kph->kph_kp;
	uint64_t v;

	assert(kcn_pkt_trailingdata(kph) >= 8);
	v =  (uint64_t)kp->kp_buf[kph->kph_cp++] << 56;
	v |= (uint64_t)kp->kp_buf[kph->kph_cp++] << 48;
	v |= (uint64_t)kp->kp_buf[kph->kph_cp++] << 40;
	v |= (uint64_t)kp->kp_buf[kph->kph_cp++] << 32;
	v |= (uint64_t)kp->kp_buf[kph->kph_cp++] << 24;
	v |= (uint64_t)kp->kp_buf[kph->kph_cp++] << 16;
	v |= (uint64_t)kp->kp_buf[kph->kph_cp++] <<  8;
	v |= (uint64_t)kp->kp_buf[kph->kph_cp++];
	return v;
}

void
kcn_pkt_put8(struct kcn_pkt_handle *kph, uint8_t v)
{
	struct kcn_pkt *kp = kph->kph_kp;

	kcn_pkt_append(kph, 1);
	kp->kp_buf[kph->kph_cp++] = v;
}

void
kcn_pkt_put16(struct kcn_pkt_handle *kph, uint16_t v)
{
	struct kcn_pkt *kp = kph->kph_kp;

	kcn_pkt_append(kph, 2);
	kp->kp_buf[kph->kph_cp++] = (v >>  8) & 0xffU;
	kp->kp_buf[kph->kph_cp++] = v         & 0xffU;
}

void
kcn_pkt_put32(struct kcn_pkt_handle *kph, uint32_t v)
{
	struct kcn_pkt *kp = kph->kph_kp;

	kcn_pkt_append(kph, 4);
	kp->kp_buf[kph->kph_cp++] = (v >> 24) & 0xffUL;
	kp->kp_buf[kph->kph_cp++] = (v >> 16) & 0xffUL;
	kp->kp_buf[kph->kph_cp++] = (v >>  8) & 0xffUL;
	kp->kp_buf[kph->kph_cp++] = v         & 0xffUL;
}

void
kcn_pkt_put64(struct kcn_pkt_handle *kph, uint64_t v)
{
	struct kcn_pkt *kp = kph->kph_kp;

	kcn_pkt_append(kph, 8);
	kp->kp_buf[kph->kph_cp++] = (v >> 56) & 0xffULL;
	kp->kp_buf[kph->kph_cp++] = (v >> 48) & 0xffULL;
	kp->kp_buf[kph->kph_cp++] = (v >> 40) & 0xffULL;
	kp->kp_buf[kph->kph_cp++] = (v >> 32) & 0xffULL;
	kp->kp_buf[kph->kph_cp++] = (v >> 24) & 0xffULL;
	kp->kp_buf[kph->kph_cp++] = (v >> 16) & 0xffULL;
	kp->kp_buf[kph->kph_cp++] = (v >>  8) & 0xffULL;
	kp->kp_buf[kph->kph_cp++] = v         & 0xffULL;
}

void
kcn_pkt_put(struct kcn_pkt_handle *kph, void *p, size_t len)
{

	kcn_pkt_append(kph, len);
	memcpy(kcn_pkt_cp(kph), p, len);
}

bool
kcn_pkt_enqueue(struct kcn_pkt_handle *kph)
{
	struct kcn_pkt *kp;

	assert(kph->kph_kp != NULL);
	kp = _kcn_pkt_dup(kph);
	if (kp == NULL)
		return false;
	STAILQ_INSERT_TAIL(kph->kph_q, kp, kp_chain);
	return true;
}

bool
kcn_pkt_fetch(struct kcn_pkt_handle *kph)
{

	kph->kph_kp = STAILQ_FIRST(kph->kph_q);
	kph->kph_cp = kph->kph_kp->kp_sp;
	return kph->kph_kp != NULL;
}

void
kcn_pkt_dequeue(struct kcn_pkt_handle *kph)
{

	assert(kph->kph_kp != NULL);
	assert(kph->kph_kp == STAILQ_FIRST(kph->kph_q));
	STAILQ_REMOVE_HEAD(kph->kph_q, kp_chain);
	kcn_pkt_destroy(kph);
}
