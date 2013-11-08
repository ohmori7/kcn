#include <errno.h>
#include <stdbool.h>
#include <stddef.h>

#include "kcn.h"
#include "kcn_info.h"
#include "kcn_google.h"
#include "kcn_local.h"
#include "kcn_search.h"

bool
kcn_search(struct kcn_info *ki, const char *keys)
{

	switch (kcn_info_type(ki)) {
	case KCN_TYPE_GOOGLE:
		return kcn_google_search(ki, keys);
	case KCN_TYPE_LOCAL:
		return kcn_local_search(ki, keys);
	default:
		errno = EPROTONOSUPPORT;
		return false;
	}
}
