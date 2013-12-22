#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
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

socklen_t
kcn_sockaddr_len(const struct sockaddr_storage *ss)
{
#ifdef HAVE_STRUCT_SOCKADDR_STORAGE_SS_LEN
	return ss->ss_len;
#else /* HAVE_STRUCT_SOCKADDR_STORAGE_SS_LEN */
	switch (ss->ss_family) {
	case AF_INET:	return sizeof(struct sockaddr_in);
#ifdef HAVE_IPV6
	case AF_INET6:	return sizeof(struct sockaddr_in6);
#endif /* HAVE_IPV6 */
	default:	errno = EAFNOSUPPORT; return -1;
	}
#endif /* ! HAVE_STRUCT_SOCKADDR_STORAGE_SS_LEN */
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

bool
kcn_sockaddr_aton(struct sockaddr_storage *ss,
    const char *nodename, const char *servname)
{
	struct addrinfo hai, *rai;
	int error;

	memset(&hai, 0, sizeof(hai));
#ifdef HAVE_IPV6
	hai.ai_family = AF_UNSPEC;
#else /* HAVE_IPV6 */
	hai.ai_family = AF_INET;
#endif /* ! HAVE_IPV6 */
	hai.ai_socktype = SOCK_STREAM;
	hai.ai_flags = 0;
	hai.ai_protocol = 0;
	error = getaddrinfo(nodename, servname, &hai, &rai);
	if (error != 0)
		goto out;
	assert(rai != NULL);
	assert(sizeof(*ss) >= rai->ai_addrlen);
	memcpy(ss, rai->ai_addr, rai->ai_addrlen);
	freeaddrinfo(rai);
  out:
	return error == 0 ? true : false;
}

bool
kcn_sockaddr_ntoa(char *name, size_t namelen, const struct sockaddr_storage *ss)
{
	char sbuf[NI_MAXSERV + 1] = { '/', 0 };
	int error;

	error = getnameinfo((struct sockaddr *)ss, sizeof(*ss), name, namelen,
	    sbuf + 1, sizeof(sbuf - 1), NI_NUMERICHOST | NI_NUMERICSERV);
	if (error == -1)
		goto bad;
	strlcat(name, sbuf, namelen);
	return true;
  bad:
	if (namelen > 0)
		name[0] = '\0';
	return false;
}
