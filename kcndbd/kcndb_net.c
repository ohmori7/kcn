#include <stdbool.h>
#include <stdint.h>

#include "kcndb_net.h"

static uint16_t kcndb_net_port = KCNDB_NET_PORT_DEFAULT;

bool
kcndb_net_port_set(uint16_t port)
{

	kcndb_net_port = port;
	return true;
}
