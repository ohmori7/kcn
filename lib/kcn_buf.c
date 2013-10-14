#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "kcn.h"
#include "kcn_buf.h"

void
kcn_buf_init(struct kcn_buf *kb)
{

	kb->kb_ptr = NULL;
	kb->kb_size = 0;
}

void
kcn_buf_finish(struct kcn_buf *kb)
{

	if (kb->kb_ptr != NULL)
		free(kb->kb_ptr);
	kcn_buf_init(kb);
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
