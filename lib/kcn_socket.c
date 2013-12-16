#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include <netinet/in.h>

#include "kcn.h"
#include "kcn_log.h"
#include "kcn_socket.h"
#include "kcn_sockaddr.h"

static bool
kcn_socket_nonblock(int s)
{
	int flags;

	flags = fcntl(s, F_GETFL);
	if (flags == -1) {
		KCN_LOG(ERR, "fcntl(F_GETFL) failed for %d: %s",
		    s, strerror(errno));
		return false;
	}
	flags |= O_NONBLOCK;
	if (fcntl(s, F_SETFL, flags) == -1) {
		KCN_LOG(ERR, "fcntl(F_SETFL) failed for %d: %s",
		    s, strerror(errno));
		return false;
	}
	return true;
}

int
kcn_socket_listen(int domain, in_port_t port)
{
	struct sockaddr_storage ss;
	socklen_t sslen;
	sa_family_t family;
	int s, on;

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
	on = 0;
	if (domain == PF_INET6 &&
	    setsockopt(s, IPPROTO_IPV6, IPV6_V6ONLY, &on, sizeof(on)) == -1) {
		KCN_LOG(ERR, "setsockopt(IPV6_V6ONLY) failed for %d: %s",
		    domain, strerror(errno));
		goto bad;
	}
#endif /* HAVE_IPV6 */

	on = 1;
	if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) == -1)
		KCN_LOG(WARN, "setsockopt(SO_REUSEADDR) failed for %d: %s",
		    domain, strerror(errno));
	if (setsockopt(s, SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on)) == -1)
		KCN_LOG(WARN, "setsockopt(SO_REUSEPORT) failed for %d: %s",
		    domain, strerror(errno));

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

#if 0 /* XXX */
	if (! kcn_socket_nonblock(s))
		goto bad;
#endif /* 0 */

	KCN_LOG(DEBUG, "listen on port %hu with fd %d", ntohs(port), s);

	return s;
  bad:
	kcn_socket_close(&s);
	return -1;
}

int
kcn_socket_accept(int ls)
{
	struct sockaddr_storage ss;
	socklen_t sslen;
	int s;

	sslen = sizeof(ss);
	while ((s = accept(ls, (struct sockaddr *)&ss, &sslen)) == -1)
		if (errno != EINTR) {
			KCN_LOG(WARN, "accept() failed on %d: %s",
			    ls, strerror(errno));
			goto bad;
		}
	if (! kcn_socket_nonblock(s))
		goto bad;
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
