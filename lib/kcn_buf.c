#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "kcn.h"
#include "kcn_log.h"
#include "kcn_buf.h"

struct kcn_buf_data {
	STAILQ_ENTRY(kcn_buf_data) kbd_chain;
	size_t kbd_size;
	size_t kbd_sp;
	size_t kbd_ep;
	uint8_t kbd_buf[1];
};

#define kb_size		kb_kbd->kbd_size
#define kb_sp		kb_kbd->kbd_sp
#define kb_ep		kb_kbd->kbd_ep
#define kb_buf		kb_kbd->kbd_buf

void
kcn_buf_queue_init(struct kcn_buf_queue *kbq)
{

	STAILQ_INIT(kbq);
}

void
kcn_buf_init(struct kcn_buf *kb, struct kcn_buf_data *kbd)
{

	kb->kb_kbd = kbd;
	if (kbd != NULL)
		kcn_buf_start(kb);
	else
		kb->kb_cp = 0;
}

struct kcn_buf_data *
kcn_buf_data_new(size_t size)
{
	struct kcn_buf_data *kbd;

	/* XXX: should consider real size of allocated space. */
	kbd = malloc(offsetof(struct kcn_buf_data, kbd_buf[size]));
	if (kbd == NULL)
		return NULL;
	kbd->kbd_size = size;
	kbd->kbd_sp = kbd->kbd_ep = 0;
	return kbd;
}

static struct kcn_buf_data *
kcn_buf_data_dup(const struct kcn_buf *kb)
{
	struct kcn_buf_data *kbd;
	size_t len;

	assert(kb->kb_kbd != NULL);
	len = kcn_buf_len(kb);
	kbd = kcn_buf_data_new(len);
	if (kbd == NULL)
		return NULL;
	memcpy(kbd->kbd_buf, kcn_buf_head(kb), len);
	kbd->kbd_ep = len;
	return kbd;
}

void
kcn_buf_data_destroy(struct kcn_buf_data *kbd)
{

	if (kbd != NULL)
		free(kbd);
}

size_t
kcn_buf_len(const struct kcn_buf *kb)
{

	return kb->kb_ep - kb->kb_sp;
}

void
kcn_buf_reset(struct kcn_buf *kb, size_t headingspace)
{

	kb->kb_cp = kb->kb_sp = kb->kb_ep = headingspace;
}

#ifndef NDEBUG
static size_t
kcn_buf_headingspace(const struct kcn_buf *kb)
{

	return kb->kb_sp;
}
#endif /* ! NDEBUG */

static size_t
kcn_buf_trailingspace(const struct kcn_buf *kb)
{

	assert(kb->kb_size >= kb->kb_ep);
	return kb->kb_size - kb->kb_ep;
}

size_t
kcn_buf_headingdata(const struct kcn_buf *kb)
{

	assert(kb->kb_cp >= kb->kb_sp);
	return kb->kb_cp - kb->kb_sp;
}

size_t
kcn_buf_trailingdata(const struct kcn_buf *kb)
{

	assert(kb->kb_ep >= kb->kb_cp);
	return kb->kb_ep - kb->kb_cp;
}

void
kcn_buf_forward(struct kcn_buf *kb, size_t len)
{

	assert(kcn_buf_trailingdata(kb) >= len);
	kb->kb_cp += len;
}

void
kcn_buf_backward(struct kcn_buf *kb, size_t len)
{

	assert(kcn_buf_headingdata(kb) >= len);
	kb->kb_cp -= len;
}

void
kcn_buf_start(struct kcn_buf *kb)
{

	kb->kb_cp = kb->kb_sp;
}

void
kcn_buf_end(struct kcn_buf *kb)
{

	kb->kb_cp = kb->kb_ep;
}

void
kcn_buf_trim_head(struct kcn_buf *kb, size_t len)
{

	assert(kcn_buf_len(kb) >= len);
	kb->kb_sp += len;
	if (kb->kb_sp > kb->kb_cp)
		kcn_buf_start(kb);
}

void
kcn_buf_realign(struct kcn_buf *kb)
{
	size_t len;

	len = kcn_buf_len(kb);
	memmove(kb->kb_buf, kcn_buf_head(kb), len);
	kb->kb_cp -= kb->kb_sp;
	kb->kb_sp = 0;
	kb->kb_ep = len;
}

void *
kcn_buf_head(const struct kcn_buf *kb)
{

	return kb->kb_buf + kb->kb_sp;
}

void *
kcn_buf_current(const struct kcn_buf *kb)
{

	return kb->kb_buf + kb->kb_cp;
}

void *
kcn_buf_tail(const struct kcn_buf *kb)
{

	return kb->kb_buf + kb->kb_ep;
}

void
kcn_buf_prepend(struct kcn_buf *kb, size_t len)
{

	assert(kcn_buf_headingspace(kb) >= len);
	kb->kb_sp -= len;
	kb->kb_cp = kb->kb_sp;
}

static void
kcn_buf_append(struct kcn_buf *kb, size_t len)
{
	size_t trailingdata;

	trailingdata = kcn_buf_trailingdata(kb);
	if (trailingdata >= len)
		return;
	kb->kb_ep += len - trailingdata;
	assert(kb->kb_ep <= kb->kb_size);
}

uint8_t
kcn_buf_get8(struct kcn_buf *kb)
{

	assert(kcn_buf_trailingdata(kb) >= 1);
	return kb->kb_buf[kb->kb_cp++];
}

uint16_t
kcn_buf_get16(struct kcn_buf *kb)
{
	uint16_t v;

	assert(kcn_buf_trailingdata(kb) >= 2);
	v =  (uint16_t)kb->kb_buf[kb->kb_cp++] << 8;
	v |= (uint16_t)kb->kb_buf[kb->kb_cp++];
	return v;
}

uint32_t
kcn_buf_get32(struct kcn_buf *kb)
{
	uint32_t v;

	assert(kcn_buf_trailingdata(kb) >= 4);
	v =  (uint32_t)kb->kb_buf[kb->kb_cp++] << 24;
	v |= (uint32_t)kb->kb_buf[kb->kb_cp++] << 16;
	v |= (uint32_t)kb->kb_buf[kb->kb_cp++] <<  8;
	v |= (uint32_t)kb->kb_buf[kb->kb_cp++];
	return v;
}

uint64_t
kcn_buf_get64(struct kcn_buf *kb)
{
	uint64_t v;

	assert(kcn_buf_trailingdata(kb) >= 8);
	v =  (uint64_t)kb->kb_buf[kb->kb_cp++] << 56;
	v |= (uint64_t)kb->kb_buf[kb->kb_cp++] << 48;
	v |= (uint64_t)kb->kb_buf[kb->kb_cp++] << 40;
	v |= (uint64_t)kb->kb_buf[kb->kb_cp++] << 32;
	v |= (uint64_t)kb->kb_buf[kb->kb_cp++] << 24;
	v |= (uint64_t)kb->kb_buf[kb->kb_cp++] << 16;
	v |= (uint64_t)kb->kb_buf[kb->kb_cp++] <<  8;
	v |= (uint64_t)kb->kb_buf[kb->kb_cp++];
	return v;
}

void
kcn_buf_put8(struct kcn_buf *kb, uint8_t v)
{

	kcn_buf_append(kb, 1);
	kb->kb_buf[kb->kb_cp++] = v;
}

void
kcn_buf_put16(struct kcn_buf *kb, uint16_t v)
{

	kcn_buf_append(kb, 2);
	kb->kb_buf[kb->kb_cp++] = (v >>  8) & 0xffU;
	kb->kb_buf[kb->kb_cp++] = v         & 0xffU;
}

void
kcn_buf_put32(struct kcn_buf *kb, uint32_t v)
{

	kcn_buf_append(kb, 4);
	kb->kb_buf[kb->kb_cp++] = (v >> 24) & 0xffUL;
	kb->kb_buf[kb->kb_cp++] = (v >> 16) & 0xffUL;
	kb->kb_buf[kb->kb_cp++] = (v >>  8) & 0xffUL;
	kb->kb_buf[kb->kb_cp++] = v         & 0xffUL;
}

void
kcn_buf_put64(struct kcn_buf *kb, uint64_t v)
{

	kcn_buf_append(kb, 8);
	kb->kb_buf[kb->kb_cp++] = (v >> 56) & 0xffULL;
	kb->kb_buf[kb->kb_cp++] = (v >> 48) & 0xffULL;
	kb->kb_buf[kb->kb_cp++] = (v >> 40) & 0xffULL;
	kb->kb_buf[kb->kb_cp++] = (v >> 32) & 0xffULL;
	kb->kb_buf[kb->kb_cp++] = (v >> 24) & 0xffULL;
	kb->kb_buf[kb->kb_cp++] = (v >> 16) & 0xffULL;
	kb->kb_buf[kb->kb_cp++] = (v >>  8) & 0xffULL;
	kb->kb_buf[kb->kb_cp++] = v         & 0xffULL;
}

void
kcn_buf_put(struct kcn_buf *kb, const void *p, size_t len)
{

	kcn_buf_append(kb, len);
	memcpy(kcn_buf_current(kb), p, len);
	kcn_buf_forward(kb, len);
}

void
kcn_buf_putnull(struct kcn_buf *kb, size_t len)
{

	kcn_buf_append(kb, len);
	memset(kcn_buf_current(kb), 0, len);
	kcn_buf_forward(kb, len);
}

bool
kcn_buf_enqueue(struct kcn_buf *kb, struct kcn_buf_queue *kbq)
{
	struct kcn_buf_data *kbd;

	assert(kb->kb_kbd != NULL);
	kbd = kcn_buf_data_dup(kb);
	if (kbd == NULL)
		return false;
	STAILQ_INSERT_TAIL(kbq, kbd, kbd_chain);
	return true;
}

bool
kcn_buf_fetch(struct kcn_buf *kb, struct kcn_buf_queue *kbq)
{
	struct kcn_buf_data *kbd;

	kbd = STAILQ_FIRST(kbq);
	if (kbd == NULL)
		return false;
	kb->kb_kbd = kbd;
	kcn_buf_start(kb);
	return true;
}

void
kcn_buf_drop(struct kcn_buf *kb, struct kcn_buf_queue *kbq)
{

	assert(kb->kb_kbd != NULL);
	assert(kb->kb_kbd == STAILQ_FIRST(kbq));
	STAILQ_REMOVE_HEAD(kbq, kbd_chain);
	kcn_buf_data_destroy(kb->kb_kbd);
	kcn_buf_init(kb, NULL);
}

void
kcn_buf_purge(struct kcn_buf_queue *kbq)
{
	struct kcn_buf kb;

	while (kcn_buf_fetch(&kb, kbq))
		kcn_buf_drop(&kb, kbq);
}

static int
kcn_buf_error(ssize_t len, int error)
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
kcn_buf_read(int fd, struct kcn_buf *kb)
{
	ssize_t len;
	int error;

	if (kcn_buf_trailingspace(kb) == 0) {
		kcn_buf_realign(kb);
		assert(kcn_buf_trailingspace(kb) > 0);
	}
	for (;;) {
		len = read(fd, kcn_buf_tail(kb), kcn_buf_trailingspace(kb));
		error = kcn_buf_error(len, errno);
		if (error != EINTR)
			break;
	}
	if (error == 0)
		kcn_buf_append(kb, kcn_buf_trailingdata(kb) + len);
	return error;
}

int
kcn_buf_write(int fd, struct kcn_buf *kb)
{
	ssize_t len;
	int error;

	for (;;) {
		len = write(fd, kcn_buf_head(kb), kcn_buf_len(kb));
		error = kcn_buf_error(len, errno);
		if (error != EINTR)
			break;
	}
	if (error == 0)
		kcn_buf_trim_head(kb, len);
	return error;
}

#define KCN_BUF_DUMP_MAXOCTETS	16
#define KCN_BUF_DUMP_LINESIZ	(KCN_BUF_DUMP_MAXOCTETS * sizeof("ff") + 1)

static void
kcn_buf_dump_line(struct kcn_buf *kb, size_t maxoctets)
{
	char buf[KCN_BUF_DUMP_LINESIZ], *cp;
	static const char ntoa[] = "0123456789abcdef";
	size_t i;
	uint8_t v;

	if (maxoctets == 0)
		return;
	for (i = 0, cp = buf; i < maxoctets; i++) {
		v = kcn_buf_get8(kb);
		*cp++ = ntoa[(v >> 4) & 0xfU];
		*cp++ = ntoa[(v     ) & 0xfU];
		*cp++ = ' ';
		if (i + 1 == KCN_BUF_DUMP_MAXOCTETS / 2)
			*cp++ = ' ';
	}
	*--cp = '\0';
	KCN_LOG(DEBUG, "%s", buf);
}

void
kcn_buf_dump(const struct kcn_buf *kb0, size_t len)
{
	struct kcn_buf kb;
	int i, n;

	if (! KCN_LOG_ISLOGGING(DEBUG))
		return;
	kb = *kb0;
	kcn_buf_start(&kb);
	n = len / KCN_BUF_DUMP_MAXOCTETS;
	for (i = 0; i < n; i++)
		kcn_buf_dump_line(&kb, KCN_BUF_DUMP_MAXOCTETS);
	kcn_buf_dump_line(&kb, len % KCN_BUF_DUMP_MAXOCTETS);
}
