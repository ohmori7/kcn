#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include <netinet/in.h>

#include "kcn.h"
#include "kcn_log.h"
#include "kcn_sockaddr.h"
#include "kcndb_net.h"

static int kcndb_net_socket4 = -1;
#ifdef HAVE_IPV6
static int kcndb_net_socket6 = -1;
#endif /* HAVE_IPV6 */
static in_port_t kcndb_net_port = KCN_HTONS(KCNDB_NET_PORT_DEFAULT);

static void kcndb_net_close(int *);

bool
kcndb_net_port_set(uint16_t port)
{

	kcndb_net_port = htons(port);
	return true;
}

static bool
kcndb_net_listen(int domain, in_port_t port, int *sockp)
{
	struct sockaddr_storage ss;
	socklen_t sslen;
	int s;

	if (! kcn_sockaddr_init(&ss, &sslen, domain /* XXX */, port))
		goto bad;

	s = socket(domain, SOCK_STREAM, 0);
	if (s == -1) {
		KCN_LOG(ERR, "socket() failed for %d: %s",
		    domain, strerror(errno));
		goto bad;
	}

	if (bind(s, (struct sockaddr *)&ss, sslen) == -1) {
		KCN_LOG(ERR, "bind() failed for %d: %s",
		    domain, strerror(errno));
		goto bad;
	}

#define KCNDB_LISTEN_BACKLOG	5
	if (listen(s, KCNDB_LISTEN_BACKLOG) == -1) {
		KCN_LOG(ERR, "listen() failed for %d: %s",
		    domain, strerror(errno));
		goto bad;
	}

	*sockp = s;
	return true;
  bad:
	kcndb_net_close(&s);
	return false;
}

static void
kcndb_net_close(int *sockp)
{
	int oerrno;

	if (*sockp < 0)
		return;
	oerrno = errno;
	(void)close(*sockp);
	*sockp = -1;
	errno = oerrno;
}

bool
kcndb_net_start(void)
{

	assert(kcndb_net_socket4 == -1);
#ifdef HAVE_IPV6
	assert(kcndb_net_socket6 == -1);
#endif /* HAVE_IPV6 */

	if (! kcndb_net_listen(PF_INET, kcndb_net_port, &kcndb_net_socket4))
		goto bad;
	KCN_LOG(INFO, "server listen on %u for IPv4.", ntohs(kcndb_net_port));
#ifdef HAVE_IPV6
	if (! kcndb_net_listen(PF_INET6, kcndb_net_port, &kcndb_net_socket6))
		goto bad;
	KCN_LOG(INFO, "server listen on %u for IPv6.", ntohs(kcndb_net_port));
#endif /* HAVE_IPV6 */
	return true;
  bad:
	kcndb_net_stop();
	return false;
}

void
kcndb_net_stop(void)
{

	kcndb_net_close(&kcndb_net_socket4);
#ifdef HAVE_IPV6
	kcndb_net_close(&kcndb_net_socket6);
#endif /* HAVE_IPV6 */
}
