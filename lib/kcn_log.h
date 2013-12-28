#include <syslog.h>

#define KCN_LOG_ISLOGGING(p)	(kcn_log_priority >= LOG_ ## p)
#define _KCN_LOG(p, ...)						\
do {									\
	if (KCN_LOG_ISLOGGING(p))					\
		kcn_log(__VA_ARGS__);					\
} while (0/*CONSTCOND*/)

#if HAVE_GCC_VA_ARGS
#define KCN_LOG(p, fmt, ...)	_KCN_LOG(p, "%s: " fmt, #p, ## __VA_ARGS__)
#elif HAVE_C99_VA_ARGS /* HAVE_GCC_VA_ARGS */
#define __KCN_LOG(p, fmt, ...)	_KCN_LOG(p, "%s: " fmt "%s", #p, __VA_ARGS__)
#define KCN_LOG(...)		__KCN_LOG(__VA_ARGS__, "")
#else /* HAVE_C99_VA_ARGS */
#error cpp does not support __VA_ARGS__ or kcn.h is not properly included.
#endif /* ! HAVE_GCC_VA_ARGS && ! HAVE_C99_VA_ARGS */

#define LOG_WARN	LOG_WARNING

extern int kcn_log_priority;

void kcn_log_priority_increment(void);
void kcn_log(const char *, ...);
