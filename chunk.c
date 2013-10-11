#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "chunk.h"

void
chunk_init(struct chunk *c)
{

	c->c_ptr = NULL;
	c->c_size = 0;
}

void
chunk_finish(struct chunk *c)
{

	if (c->c_ptr != NULL)
		free(c->c_ptr);
	chunk_init(c);
}

bool
chunk_append(struct chunk *c, const void *p, size_t size)
{
	void *np;

	np = realloc(c->c_ptr, c->c_size + size);
	if (np == NULL)
		return false;
	memcpy(np + c->c_size, p, size);
	c->c_ptr = np;
	c->c_size += size;

	return true;
}
