#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "kcn_str.h"

bool
kcn_strtoull(const char *s, unsigned long long min, unsigned long long max,
    unsigned long long *p)
{
	char *ep;
	int oerrno;
	unsigned long long llval;

	oerrno = errno;
	errno = 0;
	llval = strtoll(s, &ep, 10);
	if (s[0] == '\0' || *ep != '\0') {
		errno = EINVAL;
		return false;
	}
	if (errno == ERANGE && (llval == ULLONG_MAX || llval == 0))
		return false;
	if (llval < min || llval > max) {
		errno = ERANGE;
		return false;
	}
	errno = oerrno;
	*p = llval;
	return true;
}

char *
kcn_str_dup(const char *s0, size_t len)
{
	char *s;

	s = malloc(len + 1);
	if (s == NULL)
		return NULL;
	memcpy(s, s0, len);
	s[len] = '\0';
	return s;
}

unsigned int
kcn_str_hash(const char *s, size_t slen, size_t hsize)
{
#define KCN_STR_HASH_DJB2_INIT	5381
	unsigned int h = KCN_STR_HASH_DJB2_INIT;
	const uint8_t *cp, *limit;

	for (cp = (const unsigned char *)s, limit = cp + slen; cp < limit; cp++)
		h = (h << 5) + h + *cp;
	return (h + (h >> 5)) % hsize;
}
