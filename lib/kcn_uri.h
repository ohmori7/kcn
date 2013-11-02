struct kcn_uri;

struct kcn_uri *kcn_uri_new(const char *);
void kcn_uri_destroy(struct kcn_uri *);
size_t kcn_uri_len(const struct kcn_uri *);
const char *kcn_uri(const struct kcn_uri *);
void kcn_uri_resize(struct kcn_uri *, size_t);
bool kcn_uri_param_puts(struct kcn_uri *, const char *, const char *);
bool kcn_uri_param_putv(struct kcn_uri *, const char *, int, char * const []);
