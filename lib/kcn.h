#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H_ */

#ifndef HAVE_STRLCPY
size_t strlcpy(char *, const char *, size_t);
#endif /* ! HAVE_STRLCPY */

#ifdef HAVE_IPV6
#define KCN_INET_ADDRSTRLEN	INET6_ADDRSTRLEN
#else /* HAVE_IPV6 */
#define KCN_INET_ADDRSTRLEN	INET_ADDRSTRLEN
#endif /* HAVE_IPV6 */
