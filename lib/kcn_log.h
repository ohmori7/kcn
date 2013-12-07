#include <syslog.h>

#define KCN_LOG(p, fmt, ...)	kcn_log(LOG_ ## p, fmt "\n" __VA_ARGS__)

void kcn_log_priority_increment(void);
void kcn_log(int, const char *, ...);
