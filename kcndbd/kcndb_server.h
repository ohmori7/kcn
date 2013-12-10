#define KCNDB_NET_PORT_MIN	1
#define KCNDB_NET_PORT_MAX	65535
#define KCNDB_NET_PORT_DEFAULT	9410

bool kcndb_server_port_set(uint16_t);
bool kcndb_server_start(void);
void kcndb_server_stop(void);
void kcndb_server_loop(void);
