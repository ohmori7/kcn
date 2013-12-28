struct event_base;
struct kcn_client;

struct kcn_client *kcn_client_new(struct event_base *);
void kcn_client_destroy(struct kcn_client *);
bool kcn_client_add_send(struct kcn_client *, const struct kcn_msg_add *);
bool kcn_client_loop(struct kcn_client *);
bool kcn_client_search(struct kcn_info *, const struct kcn_msg_query *);
