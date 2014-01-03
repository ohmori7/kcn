struct kcn_httpbuf;

struct kcn_httpbuf *kcn_httpbuf_new(void);
void kcn_httpbuf_destroy(struct kcn_httpbuf *);
void *kcn_httpbuf_get(struct kcn_httpbuf *);
bool kcn_httpbuf_append(struct kcn_httpbuf *, const void *, size_t);
