struct kcn_token;

struct kcn_token *kcn_token_new(const char *);
void kcn_token_destroy(struct kcn_token *);
const char *kcn_token_get(struct kcn_token *, size_t *);
