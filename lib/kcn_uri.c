#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "kcn.h"
#include "kcn_uri.h"

/* one original character is encoded with 3 characters at maximum. */
#define KCN_URI_ENCODING_FACTOR		3

static char *
kcn_uri_encputs(char *d, const char *s0)
{
	static const char *hexstr = "0123456789ABCDEF";
	const unsigned char *s;

	for (s = (const unsigned char *)s0; *s != '\0'; s++) {
		if (isalnum(*s) || *s == '.' || *s == '-' || *s == '_')
			*d++ = *s;
		else if (*s == ' ')
			*d++ = '+';
		else {
			*d++ = '%';
			*d++ = hexstr[(*s >> 4) & 0xfU];
			*d++ = hexstr[(*s     ) & 0xfU];
		}
	}
	*d = '\0';
	return d;
}

bool
kcn_uri_puts(char **urip, const char *s)
{
	char *uri;
	size_t olen, slen;

	assert(*urip != NULL);
	olen = strlen(*urip);
	slen = strlen(s);
	uri = realloc(*urip, olen + slen + 1);
	if (uri == NULL)
		return false;
	memmove(&uri[olen], s, slen);
	uri[olen + slen] = '\0';
	*urip = uri;
	return true;
}

char *
kcn_uri_build(const char *base, int argc, char * const argv[])
{
	int i;
	size_t baselen, querylen, totallen;
	char *sp, *cp;

	assert(base != NULL);
	assert(argc > 0);

	/*
	 * XXX: should convert to utf-8 when character encodings
	 *	of input string is different.
	 */
	querylen = argc - 1; /* separator, i.e., space character */
	for (i = 0; i < argc; i++) {
		assert(argv[i] != NULL);
		querylen += strlen(argv[i]) * KCN_URI_ENCODING_FACTOR;
	}
	baselen = strlen(base);
	totallen = baselen + querylen + 1 /* terminator */;

	sp = malloc(totallen);
	if (sp == NULL)
		return NULL;

	(void)strlcpy(sp, base, totallen);
	cp = sp + baselen;
	for (i = 0; i < argc; i++) {
		if (i > 0)
			cp = kcn_uri_encputs(cp, " ");
		cp = kcn_uri_encputs(cp, argv[i]);
	}

	return sp;
}

void
kcn_uri_resize(char *uri, size_t totallen)
{

	assert(strlen(uri) >= totallen);
	uri[totallen] = '\0';
}

void
kcn_uri_free(char *uri)
{

	free(uri);
}
