int kcn_sockaddr_pf2af(int);
int kcn_sockaddr_af2pf(int);
socklen_t kcn_sockaddr_len(const struct sockaddr_storage *);
bool kcn_sockaddr_init(struct sockaddr_storage *, socklen_t *,
    sa_family_t, in_port_t);
