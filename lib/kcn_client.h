struct event_base;

struct kcn_net *kcn_client_init(struct event_base *, void *);
void kcn_client_finish(struct kcn_net *);
void kcn_client_server_name_set(const char *);
bool kcn_client_add_send(struct kcn_net *, const struct kcn_msg_add *);
bool kcn_client_search(struct kcn_info *, const struct kcn_msg_query *);
