struct kcn_net;

extern struct timeval kcn_net_timeouttv;

struct kcn_net *kcn_net_new(struct event_base *, int, size_t, const char *,
    int (*)(struct kcn_net *, struct kcn_buf *, void *), void *);
void kcn_net_destroy(struct kcn_net *);
struct event_base *kcn_net_event_base(const struct kcn_net *);
void kcn_net_obuf(struct kcn_net *, struct kcn_buf *);
void *kcn_net_data(const struct kcn_net *);
bool kcn_net_read_enable(struct kcn_net *);
bool kcn_net_write(struct kcn_net *, struct kcn_buf *);
bool kcn_net_loop(struct kcn_net *);
