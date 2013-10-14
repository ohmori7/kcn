#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H_ */

#ifndef HAVE_STRLCPY
size_t strlcpy(char *, const char *, size_t);
#endif /* ! HAVE_STRLCPY */

enum kcn_loc_type {
	KCN_LOC_TYPE_DOMAINNAME,
	KCN_LOC_TYPE_URI
};
