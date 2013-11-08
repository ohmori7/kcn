#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "kcn.h"

#ifndef HAVE_STRLCPY
size_t
strlcpy(char *dst, const char *src, size_t size)
{

	strncpy(dst, src, size);
	dst[size - 1] = '\0';
	return strlen(dst);
}
#endif /* ! HAVE_STRLCPY */

char *
kcn_key_concat(int keyc, char * const keyv[])
{
	char *s;
	size_t size;
	int i;

	assert(keyc > 0);
	size = keyc; /* the number of separators and terminator */
	for (i = 0; i < keyc; i++)
		size += strlen(keyv[i]);
	s = malloc(size);
	if (s == NULL)
		return NULL;
	i = 0;
	strlcpy(s, keyv[i++], size);
	for (; i < keyc; i++) {
		(void)strncat(s, " ", size);
		(void)strncat(s, keyv[i], size);
	}
	return s;
}
