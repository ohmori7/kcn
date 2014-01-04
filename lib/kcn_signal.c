#include <stdbool.h>
#include <signal.h>

#include <kcn_signal.h>

bool
kcn_signal_init(void)
{

	if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
		return false;
	return true;
}
