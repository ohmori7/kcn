int kcn_socket_listen(int, in_port_t);
int kcn_socket_connect(struct sockaddr_storage *);
int kcn_socket_accept(int, char *, size_t);
void kcn_socket_close(int *);
