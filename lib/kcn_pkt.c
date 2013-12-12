#include <sys/queue.h>

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "kcn_pkt.h"

struct kcn_pkt {
	STAILQ_ENTRY(kcn_pkt) kp_chain;
	size_t kp_size;
	size_t kp_len;
	uint8_t kp_buf[1];
};

struct kcn_pkt *
kcn_pkt_new(struct kcn_pkt_handle *kph, size_t size)
{
	struct kcn_pkt *kp;

	kp = malloc(offsetof(struct kcn_pkt, kp_buf[size]));
	if (kp == NULL)
		return NULL;
	kp->kp_size = size;
	kph->kph_kp = kp;
	kcn_pkt_reset(kph);
	return kp;
}

void
kcn_pkt_destroy(struct kcn_pkt *kp)
{

	free(kp);
}

size_t
kcn_pkt_len(const struct kcn_pkt_handle *kph)
{

	return kph->kph_kp->kp_len;
}

void
kcn_pkt_reset(struct kcn_pkt_handle *kph)
{

	kph->kph_kp->kp_len = 0;
	kph->kph_cp = 0;
}

size_t
kcn_pkt_leftlen(struct kcn_pkt_handle *kph)
{

	assert(kph->kph_kp->kp_size >= kph->kph_cp);
	return kph->kph_kp->kp_size - kph->kph_cp;
}

size_t
kcn_pkt_unreadlen(struct kcn_pkt_handle *kph)
{

	assert(kph->kph_kp->kp_len >= kph->kph_cp);
	return kph->kph_kp->kp_len - kph->kph_cp;
}

void *
kcn_pkt_ptr(struct kcn_pkt_handle *kph)
{

	return kph->kph_kp->kp_buf + kph->kph_cp;
}

static void
kcn_pkt_append(struct kcn_pkt_handle *kph, size_t len)
{
	struct kcn_pkt *kp = kph->kph_kp;
	size_t leftlen;

	leftlen = kcn_pkt_leftlen(kph);
	if (leftlen >= len)
		return;
	kp->kp_len += len - leftlen;
	assert(kp->kp_size >= kp->kp_len);
}

uint8_t
kcn_pkt_get8(struct kcn_pkt_handle *kph)
{
	struct kcn_pkt *kp = kph->kph_kp;

	assert(kcn_pkt_unreadlen(kph) >= 1);
	return kp->kp_buf[kph->kph_cp++];
}

uint16_t
kcn_pkt_get16(struct kcn_pkt_handle *kph)
{
	struct kcn_pkt *kp = kph->kph_kp;
	uint16_t v;

	assert(kcn_pkt_unreadlen(kph) >= 2);
	v =  (uint16_t)kp->kp_buf[kph->kph_cp++] << 8;
	v |= (uint16_t)kp->kp_buf[kph->kph_cp++];
	return v;
}

uint32_t
kcn_pkt_get32(struct kcn_pkt_handle *kph)
{
	struct kcn_pkt *kp = kph->kph_kp;
	uint32_t v;

	assert(kcn_pkt_unreadlen(kph) >= 4);
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

	assert(kcn_pkt_unreadlen(kph) >= 8);
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
	memcpy(kcn_pkt_ptr(kph), p, len);
}
