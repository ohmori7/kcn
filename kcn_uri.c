#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "kcn_uri.h"

static char *
kcn_uri_puts(char *d, const char *s0)
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

char *
kcn_uri_build(const char *base, int argc, char * const argv[])
{
	int i;
	size_t baselen, totallen;
	char *sp, *cp;

	assert(base != NULL);
	assert(argc > 0);

	/*
	 * XXX: should convert to utf-8 when character encodings
	 *	of input string is different.
	 */
	baselen = strlen(base);
	totallen = baselen + (argc - 1) /* separator */ + 1 /* terminator */;
	for (i = 0; i < argc; i++) {
		assert(argv[i] != NULL);
		totallen += strlen(argv[i]);
	}

	/*
	 * note that one character is encoded with 3
	 * characters at maximum. see kcn_uri_puts().
	 */
	sp = malloc(totallen * 3);
	if (sp == NULL)
		return NULL;

	(void)strlcpy(sp, base, totallen);
	cp = sp + baselen;
	for (i = 0; i < argc; i++) {
		if (i > 0)
			cp = kcn_uri_puts(cp, " ");
		cp = kcn_uri_puts(cp, argv[i]);
	}

	return sp;
}

void
kcn_uri_free(char *uri)
{

	free(uri);
}
