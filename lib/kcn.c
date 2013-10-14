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
