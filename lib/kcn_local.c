#include <errno.h>
#include <stdbool.h>
#include <stddef.h>

#include "kcn.h"
#include "kcn_info.h"
#include "kcn_local.h"

bool
kcn_local_search(struct kcn_info *ki, const char *keys)
{

	(void)ki;
	(void)keys;
	errno = EPROTONOSUPPORT;
	return false;
}
