struct kcn_net;

struct kcn_net *kcn_net_new(int, size_t, void *);
void kcn_net_destroy(struct kcn_net *);
int kcn_net_fd(const struct kcn_net *);
void kcn_net_ipkt(struct kcn_net *, struct kcn_pkt *);
void kcn_net_opkt(struct kcn_net *, struct kcn_pkt *);
void *kcn_net_data(const struct kcn_net *);
bool kcn_net_read(struct kcn_net *kn);
bool kcn_net_write(struct kcn_net *);
