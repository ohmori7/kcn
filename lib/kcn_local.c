#include <errno.h>
#include <stdbool.h>
#include <stddef.h>

#include "kcn.h"
#include "kcn_info.h"
#include "kcn_local.h"

bool
kcn_local_search(int keyc, char * const keyv[], struct kcn_info *ki)
{

	(void)keyc;
	(void)keyv;
	(void)ki;
	errno = EPROTONOSUPPORT;
	return false;
}
