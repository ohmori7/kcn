#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "kcn.h"
#include "kcn_httpbuf.h"

struct kcn_httpbuf {
	void *khb_ptr;
	size_t khb_size;
};

struct kcn_httpbuf *
kcn_httpbuf_new(void)
{
	struct kcn_httpbuf *khb;

	khb = malloc(sizeof(*khb));
	if (khb == NULL)
		return NULL;
	khb->khb_ptr = NULL;
	khb->khb_size = 0;
	return khb;
}

void
kcn_httpbuf_destroy(struct kcn_httpbuf *khb)
{

	if (khb == NULL)
		return;
	if (khb->khb_ptr != NULL)
		free(khb->khb_ptr);
	free(khb);
}

void *
kcn_httpbuf_get(struct kcn_httpbuf *khb)
{
	void *p;

	p = khb->khb_ptr;
	khb->khb_ptr = NULL;
	khb->khb_size = 0;
	return p;
}

bool
kcn_httpbuf_append(struct kcn_httpbuf *khb, const void *p, size_t size)
{
	void *np;

	if (khb->khb_size > khb->khb_size + size)
		return false;
	np = realloc(khb->khb_ptr, khb->khb_size + size);
	if (np == NULL)
		return false;
	memcpy(np + khb->khb_size, p, size);
	khb->khb_ptr = np;
	khb->khb_size += size;

	return true;
}
