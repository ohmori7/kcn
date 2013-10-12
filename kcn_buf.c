#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "kcn_buf.h"

void
buf_init(struct buf *b)
{

	b->b_ptr = NULL;
	b->b_size = 0;
}

void
buf_finish(struct buf *b)
{

	if (b->b_ptr != NULL)
		free(b->b_ptr);
	buf_init(b);
}

bool
buf_append(struct buf *b, const void *p, size_t size)
{
	void *np;

	np = realloc(b->b_ptr, b->b_size + size);
	if (np == NULL)
		return false;
	memcpy(np + b->b_size, p, size);
	b->b_ptr = np;
	b->b_size += size;

	return true;
}
