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

struct kcn_uri {
	bool ku_isparamseen;
	size_t ku_urilen;
	char *ku_uri;
};

struct kcn_uri *
kcn_uri_new(const char *base)
{
	struct kcn_uri *ku;
	const char *cp;

	ku = malloc(sizeof(*ku));
	if (ku == NULL)
		goto bad;

	ku->ku_isparamseen = false;
	for (cp = base; *cp != '\0'; cp++)
		if (*cp == '?')
			ku->ku_isparamseen = true;

	ku->ku_urilen = cp - base;
	ku->ku_uri = malloc(ku->ku_urilen + sizeof('\0'));
	if (ku->ku_uri == NULL)
		goto bad;
	(void)strlcpy(ku->ku_uri, base, ku->ku_urilen + sizeof('\0'));
	return ku;
  bad:
	kcn_uri_destroy(ku);
	return NULL;
}

void
kcn_uri_destroy(struct kcn_uri *ku)
{

	if (ku == NULL)
		return;
	if (ku->ku_uri != NULL)
		free(ku->ku_uri);
	free(ku);
}

size_t
kcn_uri_len(const struct kcn_uri *ku)
{

	return ku->ku_urilen;
}

const char *
kcn_uri(const struct kcn_uri *ku)
{

	return ku->ku_uri;
}

static bool
kcn_uri_ensure_trailing_space(struct kcn_uri *ku, size_t tlen)
{
	char *uri;
	size_t nlen;

	assert(ku->ku_uri != NULL);
	nlen = ku->ku_urilen + tlen;
	uri = realloc(ku->ku_uri, nlen + sizeof('\0'));
	if (uri == NULL)
		return false;
	uri[nlen] = '\0';
	ku->ku_uri = uri;
	return true;
}

void
kcn_uri_resize(struct kcn_uri *ku, size_t len)
{

	assert(strlen(ku->ku_uri) >= len);
	ku->ku_urilen = len;
	ku->ku_uri[len] = '\0';
}

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

static bool
kcn_uri_puts(struct kcn_uri *ku, const char *s)
{
	size_t slen;

	slen = strlen(s);
	if (! kcn_uri_ensure_trailing_space(ku, slen))
		return false;
	memmove(ku->ku_uri + ku->ku_urilen, s, slen);
	ku->ku_urilen += slen;
	return true;
}

bool
kcn_uri_param_puts(struct kcn_uri *ku, const char *param, const char *val)
{

	if (val == NULL)
		return true;
	if (kcn_uri_puts(ku, "&") && /* XXX */
	    kcn_uri_puts(ku, param) &&
	    kcn_uri_puts(ku, "=") &&
	    kcn_uri_puts(ku, val))
		return true;
	else
		return false;
}

bool
kcn_uri_param_putv(struct kcn_uri *ku, const char *param,
    int argc, char * const argv[])
{
	int i;
	size_t len;
	char *cp;

	assert(argc > 0);

	if (! kcn_uri_puts(ku, "&") || /* XXX */
	    ! kcn_uri_puts(ku, param) ||
	    ! kcn_uri_puts(ku, "="))
		return false;

	/*
	 * XXX: should convert to utf-8 when character encodings
	 *	of input string is different.
	 */
	len = argc - 1; /* separator, i.e., space character */
	for (i = 0; i < argc; i++) {
		assert(argv[i] != NULL);
		len += strlen(argv[i]) * KCN_URI_ENCODING_FACTOR;
	}
	if (! kcn_uri_ensure_trailing_space(ku, len))
		return false;

	cp = ku->ku_uri + ku->ku_urilen;
	for (i = 0; i < argc; i++) {
		if (i > 0)
			cp = kcn_uri_encputs(cp, " ");
		cp = kcn_uri_encputs(cp, argv[i]);
	}
	ku->ku_urilen = cp - ku->ku_uri - sizeof('\0');

	return true;
}
