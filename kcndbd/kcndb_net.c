#include <stdbool.h>
#include <stdint.h>

#include <netinet/in.h>

#include "kcn.h"
#include "kcndb_net.h"

static in_port_t kcndb_net_port = KCN_HTONS(KCNDB_NET_PORT_DEFAULT);

bool
kcndb_net_port_set(uint16_t port)
{

	kcndb_net_port = htons(port);
	return true;
}
