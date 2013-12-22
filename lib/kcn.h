#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H_ */

#ifndef HAVE_STRLCPY
size_t strlcpy(char *, const char *, size_t);
#endif /* ! HAVE_STRLCPY */
#ifndef HAVE_STRLCAT
size_t strlcat(char *, const char *, size_t);
#endif /* ! HAVE_STRLCAT */

#ifdef HAVE_IPV6
#define KCN_INET_ADDRSTRLEN	INET6_ADDRSTRLEN
#else /* HAVE_IPV6 */
#define KCN_INET_ADDRSTRLEN	INET_ADDRSTRLEN
#endif /* HAVE_IPV6 */

#define KCN_SOCKNAMELEN		(KCN_INET_ADDRSTRLEN + sizeof("/65535") - 1)

/* for JANSSON 2.4 */
#ifndef json_array_foreach
#define json_array_foreach(a, i, v)					\
	for ((i) = 0;							\
	     (i) < json_array_size(a) &&				\
	     ((v) = json_array_get((a), (i))) != NULL;			\
	     (i)++)
#endif /* ! json_array_foreach */

#if BYTE_ORDER == LITTLE_ENDIAN
#define KCN_HTONS(v)							\
	((((unsigned int)(v) << 8) & 0xff00U) |				\
	 (((unsigned int)(v) >> 8) & 0xffU))
#elif BYTE_ORDER == BIG_ENDIAN /* BYTE_ORDER == LITTLE_ENDIAN */
#define KCN_HTONS(v)	v
#else /* BYTE_ORDER == BIG_ENDIAN */
#error unknown endianness
#endif /* BYTE_ORDER != LITTLE_ENDIAN && BYTE_ORDER != BIG_ENDIAN */

struct kcn_info; /* XXX */

char *kcn_key_concat(int, char * const []);
char **kcn_key_split(const char *, size_t *);
void kcn_key_free(size_t, char **);
void kcn_init(void);
bool kcn_search(struct kcn_info *, const char *);
bool kcn_searchv(struct kcn_info *, int, char * const []);
