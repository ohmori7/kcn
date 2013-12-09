int kcn_sockaddr_pf2af(int);
bool kcn_sockaddr_init(struct sockaddr_storage *, socklen_t *,
    sa_family_t, in_port_t);
