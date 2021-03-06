#include <errno.h>
#include <stdarg.h>
#include <stdbool.h> /* XXX: just for bool in kcn.h */
#include <stdio.h>

#include "kcn.h"
#include "kcn_log.h"

int kcn_log_priority = LOG_EMERG - 1;

void
kcn_log_priority_increment(void)
{

	if (kcn_log_priority + 1 > kcn_log_priority)
		++kcn_log_priority;
}

void
kcn_log(const char *fmt, ...)
{
	va_list ap;
	int oerrno;

	oerrno = errno;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fprintf(stderr, "\n");
	errno = oerrno;
}
