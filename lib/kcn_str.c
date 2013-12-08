#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>

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
		errno = ERANGE;
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
