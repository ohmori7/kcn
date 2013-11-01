/* XXX: is there any appropriate library that defines HTTP response code? */
#define KCN_HTTP_OK		200
#define KCN_HTTP_FORBIDDEN	403

char *kcn_http_response_get(const char *);
void kcn_http_response_free(char *);

