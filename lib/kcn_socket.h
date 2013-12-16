int kcn_socket_listen(int domain, in_port_t);
int kcn_socket_connect(struct sockaddr_storage *);
int kcn_socket_accept(int);
void kcn_socket_close(int *);
