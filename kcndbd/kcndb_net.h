#define KCNDB_NET_PORT_MIN	1
#define KCNDB_NET_PORT_MAX	65535
#define KCNDB_NET_PORT_DEFAULT	9410

bool kcndb_net_port_set(uint16_t);
bool kcndb_net_start(void);
void kcndb_net_stop(void);
