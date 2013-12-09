#include <errno.h>
#include <stdbool.h>
#include <string.h>

#include <netinet/in.h>

#include "kcn.h"
#include "kcn_sockaddr.h"

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
