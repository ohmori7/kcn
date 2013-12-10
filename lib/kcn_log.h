#include <syslog.h>

#if HAVE_GCC_VA_ARGS
#define KCN_LOG(p, fmt, ...)	kcn_log(LOG_ ## p, fmt, ## __VA_ARGS__)
#elif HAVE_C99_VA_ARGS /* HAVE_GCC_VA_ARGS */
#define _KCN_LOG(p, fmt, ...)	kcn_log(LOG_ ## p, fmt "%s", __VA_ARGS__)
#define KCN_LOG(...)		_KCN_LOG(__VA_ARGS__, "")
#else /* HAVE_C99_VA_ARGS */
#error cpp does not support __VA_ARGS__ or kcn.h is not properly included.
#endif /* ! HAVE_GCC_VA_ARGS && ! HAVE_C99_VA_ARGS */

#define LOG_WARN	LOG_WARNING

void kcn_log_priority_increment(void);
void kcn_log(int, const char *, ...);
