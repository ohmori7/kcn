#include <stdbool.h>
#include <stddef.h>

#include "kcn.h"
#include "kcn_info.h"
#include "kcn_google.h"
#include "kcn_search.h"

bool
kcn_search(int keyc, char * const keyv[], struct kcn_info *ki)
{

	switch (kcn_info_type(ki)) {
	case KCN_TYPE_GOOGLE:	return kcn_google(keyc, keyv, ki);
	case KCN_TYPE_LOCAL:
	default:		return false;
	}
}
