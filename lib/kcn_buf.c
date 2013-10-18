#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "kcn.h"
#include "kcn_buf.h"

struct kcn_buf {
	void *kb_ptr;
	size_t kb_size;
};

struct kcn_buf *
kcn_buf_new(void)
{
	struct kcn_buf *kb;

	kb = malloc(sizeof(*kb));
	if (kb == NULL)
		return NULL;
	kb->kb_ptr = NULL;
	kb->kb_size = 0;
	return kb;
}

void
kcn_buf_destroy(struct kcn_buf *kb)
{

	if (kb->kb_ptr != NULL)
		free(kb->kb_ptr);
	free(kb);
}

void *
kcn_buf_get(struct kcn_buf *kb)
{
	void *p;

	p = kb->kb_ptr;
	kb->kb_ptr = NULL;
	kb->kb_size = 0;
	return p;
}

bool
kcn_buf_append(struct kcn_buf *kb, const void *p, size_t size)
{
	void *np;

	np = realloc(kb->kb_ptr, kb->kb_size + size);
	if (np == NULL)
		return false;
	memcpy(np + kb->kb_size, p, size);
	kb->kb_ptr = np;
	kb->kb_size += size;

	return true;
}
