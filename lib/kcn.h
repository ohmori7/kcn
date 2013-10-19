#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H_ */

/* XXX: should detect with autoconf but... */
#ifdef __linux__
#include <sys/param.h>
#endif /* __linux__ */

#ifndef HAVE_STRLCPY
size_t strlcpy(char *, const char *, size_t);
#endif /* ! HAVE_STRLCPY */

enum kcn_loc_type {
	KCN_LOC_TYPE_DOMAINNAME,
	KCN_LOC_TYPE_URI
};

#ifdef HAVE_IPV6
#define KCN_INET_ADDRSTRLEN	INET6_ADDRSTRLEN
#else /* HAVE_IPV6 */
#define KCN_INET_ADDRSTRLEN	INET_ADDRSTRLEN
#endif /* HAVE_IPV6 */
