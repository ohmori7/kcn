#include <errno.h>
#include <stdbool.h>
#include <string.h>

#include <netinet/in.h>

#include "kcn.h"
#include "kcn_sockaddr.h"

int
kcn_sockaddr_pf2af(int domain)
{

	switch (domain) {
	case PF_INET:	return AF_INET;
#ifdef HAVE_IPV6
	case PF_INET6:	return AF_INET6;
#endif /* HAVE_IPV6 */
	default:	errno = EPFNOSUPPORT; return -1;
	}
}

int
kcn_sockaddr_af2pf(int af)
{

	switch (af) {
	case AF_INET:	return PF_INET;
#ifdef HAVE_IPV6
	case AF_INET6:	return PF_INET6;
#endif /* HAVE_IPV6 */
	default:	errno = EAFNOSUPPORT; return -1;
	}
}

bool
kcn_sockaddr_init(struct sockaddr_storage *ss, socklen_t *sslenp,
    sa_family_t family, in_port_t port)
{
	struct sockaddr_in *sin = (struct sockaddr_in *)ss;
#ifdef HAVE_IPV6
	struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)ss;
#endif /* HAVE_IPV6 */

	memset(ss, 0, sizeof(*ss));
	switch (family) {
	case AF_INET:
		sin->sin_family = AF_INET;
		*sslenp = sizeof(*sin);
#ifdef HAVE_STRUCT_SOCKADDR_IN_SIN_LEN
		sin->sin_len = *sslenp;
#endif /* HAVE_STRUCT_SOCKADDR_IN_SIN_LEN */
		sin->sin_port = port;
		break;
#ifdef HAVE_IPV6
	case AF_INET6:
		sin6->sin6_family = AF_INET6;
		*sslenp = sizeof(*sin6);
#ifdef HAVE_STRUCT_SOCKADDR_IN6_SIN6_LEN
		sin6->sin6_len = *sslenp;
#endif /* HAVE_STRUCT_SOCKADDR_IN6_SIN6_LEN */
		sin6->sin6_port = port;
		break;
#endif /* HAVE_IPV6 */
	default:
		errno = EPFNOSUPPORT;
		return false;
	}
	return true;
}
