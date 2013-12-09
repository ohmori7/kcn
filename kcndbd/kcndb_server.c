#include <assert.h>
#include <stdbool.h>

#include <netinet/in.h>

#include "kcn.h"
#include "kcn_log.h"
#include "kcn_socket.h"
#include "kcn_sockaddr.h"
#include "kcndb_net.h"

static int kcndb_net_socket = -1;
static in_port_t kcndb_net_port = KCN_HTONS(KCNDB_NET_PORT_DEFAULT);

bool
kcndb_net_port_set(uint16_t port)
{

	kcndb_net_port = htons(port);
	return true;
}

bool
kcndb_net_start(void)
{
	int domain;

	assert(kcndb_net_socket == -1);

#ifdef HAVE_IPV6
	domain = PF_INET6;
#else /* HAVE_IPV6 */
	domain = PF_INET;
#endif /* ! HAVE_IPV6 */
	kcndb_net_socket = kcn_socket_listen(domain, kcndb_net_port);
	if (kcndb_net_socket == -1)
		goto bad;
	KCN_LOG(INFO, "server listen on %u.", ntohs(kcndb_net_port));
	return true;
  bad:
	kcndb_net_stop();
	return false;
}

void
kcndb_net_stop(void)
{

	kcn_socket_close(&kcndb_net_socket);
}
