#include <stdarg.h>
#include <stdio.h>

#include "kcn_log.h"

static int kcn_log_priority = LOG_EMERG - 1;

void
kcn_log_priority_increment(void)
{

	if (kcn_log_priority + 1 > kcn_log_priority)
		++kcn_log_priority;
}

void
kcn_log(int priority, const char *fmt, ...)
{
	va_list ap;

	if (kcn_log_priority < priority)
		return;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
}
