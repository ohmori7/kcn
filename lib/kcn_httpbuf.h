struct kcn_buf;

struct kcn_buf *kcn_buf_new(void);
void kcn_buf_destroy(struct kcn_buf *);
void *kcn_buf_get(struct kcn_buf *);
bool kcn_buf_append(struct kcn_buf *, const void *, size_t);
