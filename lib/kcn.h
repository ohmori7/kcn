#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H_ */

#ifndef HAVE_STRLCPY
size_t strlcpy(char *, const char *, size_t);
#endif /* ! HAVE_STRLCPY */

#ifdef HAVE_IPV6
#define KCN_INET_ADDRSTRLEN	INET6_ADDRSTRLEN
#else /* HAVE_IPV6 */
#define KCN_INET_ADDRSTRLEN	INET_ADDRSTRLEN
#endif /* HAVE_IPV6 */

struct kcn_info; /* XXX */

char *kcn_key_concat(int, char * const []);
char **kcn_key_split(const char *, size_t *);
void kcn_key_free(size_t, char **);
bool kcn_search(struct kcn_info *, const char *);
bool kcn_searchv(struct kcn_info *, int, char * const []);
