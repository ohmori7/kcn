#include <errno.h>
#include <stdbool.h>	/* just for kcn.h */
#include <string.h>
#include <unistd.h>

#include <netinet/in.h>

#include "kcn.h"
#include "kcn_log.h"
#include "kcn_socket.h"
#include "kcn_sockaddr.h"

int
kcn_socket_listen(int domain, in_port_t port)
{
	struct sockaddr_storage ss;
	socklen_t sslen;
	sa_family_t family;
	int s;
#ifdef HAVE_IPV6
	int on = 0;
#endif /* HAVE_IPV6 */

	family = kcn_sockaddr_pf2af(domain);
	if (! kcn_sockaddr_init(&ss, &sslen, family, port))
		goto bad;

	s = socket(domain, SOCK_STREAM, 0);
	if (s == -1) {
		KCN_LOG(ERR, "socket() failed for %d: %s",
		    domain, strerror(errno));
		goto bad;
	}
#ifdef HAVE_IPV6
	if (domain == PF_INET6 &&
	    setsockopt(s, IPPROTO_IPV6, IPV6_V6ONLY, &on, sizeof(on)) == -1) {
		KCN_LOG(ERR, "setsockopt() failed %d: %s",
		    domain, strerror(errno));
		goto bad;
	}
#endif /* HAVE_IPV6 */

	if (bind(s, (struct sockaddr *)&ss, sslen) == -1) {
		KCN_LOG(ERR, "bind() failed for %d: %s",
		    domain, strerror(errno));
		goto bad;
	}

#define KCN_SOCKET_LISTEN_BACKLOG	5
	if (listen(s, KCN_SOCKET_LISTEN_BACKLOG) == -1) {
		KCN_LOG(ERR, "listen() failed for %d: %s",
		    domain, strerror(errno));
		goto bad;
	}

	return s;
  bad:
	kcn_socket_close(&s);
	return -1;
}

void
kcn_socket_close(int *sp)
{
	int oerrno;

	if (*sp < 0)
		return;
	oerrno = errno;
	(void)close(*sp);
	*sp = -1;
	errno = oerrno;
}
