#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "kcn.h"

#ifndef HAVE_STRLCPY
size_t
strlcpy(char *dst, const char *src, size_t size)
{

	strncpy(dst, src, size);
	dst[size - 1] = '\0';
	return strlen(dst);
}
#endif /* ! HAVE_STRLCPY */

char *
kcn_key_concat(int keyc, char * const keyv[])
{
	char *s;
	size_t size;
	int i;

	assert(keyc > 0);
	size = keyc; /* the number of separators and terminator */
	for (i = 0; i < keyc; i++)
		size += strlen(keyv[i]);
	s = malloc(size);
	if (s == NULL)
		return NULL;
	i = 0;
	strlcpy(s, keyv[i++], size);
	for (; i < keyc; i++) {
		(void)strncat(s, " ", size);
		(void)strncat(s, keyv[i], size);
	}
	return s;
}

char **
kcn_key_split(const char *keys0, size_t *keycp)
{
	char *k, *kp, *keys, **keyv, **tmp;
	size_t keyc;

	keys = strdup(keys0);
	if (keys == NULL)
		return NULL;
	keyc = 0;
	keyv = NULL;
	for (k = strtok_r(keys, " \t", &kp);
	     k != NULL;
	     k = strtok_r(NULL, " \t", &kp)) {
		tmp = realloc(keyv, sizeof(char *) * (keyc + 1));
		if (tmp == NULL)
			goto bad;
		keyv = tmp;
		keyv[keyc++] = strdup(k);
		if (keyv[keyc - 1] == NULL)
			goto bad;
	}
	free(keys);
	*keycp = keyc;
	return keyv;
  bad:
	free(keys);
	kcn_key_free(keyc, keyv);
	return NULL;
}

void
kcn_key_free(size_t keyc, char *keyv[])
{
	size_t i;

	if (keyv == NULL)
		return;
	for (i = 0; i < keyc; i++)
		if (keyv[i] != NULL)
			free(keyv[i]);
	free(keyv);
}
