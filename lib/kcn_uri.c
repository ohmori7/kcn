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
	size_t ku_urilen;
	char *ku_uri;
};

static bool
kcn_uri_str_isnormal(char c)
{

	if (isalnum(c) || c == '.' || c == '-' || c == '_')
		return true;
	else
		return false;
}

static size_t
kcn_uri_str_len(const char *s, bool isencoding)
{
	const char *cp;
	size_t len;

	len = 0;
	for (cp = s; *cp != '\0'; cp++)
		if (isencoding && ! kcn_uri_str_isnormal(*cp) && *cp != ' ')
			len += KCN_URI_ENCODING_FACTOR;
		else
			++len;
	return len;
}

struct kcn_uri *
kcn_uri_new(const char *base)
{
	struct kcn_uri *ku;

	ku = malloc(sizeof(*ku));
	if (ku == NULL)
		goto bad;

	ku->ku_urilen = kcn_uri_str_len(base, false);
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

	assert(len <= ku->ku_urilen);
	ku->ku_urilen = len;
	ku->ku_uri[len] = '\0';
}

static bool
kcn_uri_encputs(struct kcn_uri *ku, const char *s0)
{
	static const char *hexstr = "0123456789ABCDEF";
	const unsigned char *s;
	char *d;

	if (! kcn_uri_ensure_trailing_space(ku, kcn_uri_str_len(s0, true)))
		return false;

	d = ku->ku_uri + ku->ku_urilen;
	for (s = (const unsigned char *)s0; *s != '\0'; s++) {
		if (kcn_uri_str_isnormal(*s))
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
	ku->ku_urilen = d - ku->ku_uri;
	return true;
}

static bool
kcn_uri_puts(struct kcn_uri *ku, const char *s)
{
	size_t slen;

	slen = kcn_uri_str_len(s, false);
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
	    kcn_uri_encputs(ku, val))
		return true;
	else
		return false;
}

bool
kcn_uri_param_putv(struct kcn_uri *ku, const char *param,
    int argc, char * const argv[])
{
	int i;

	assert(argc > 0);

	if (! kcn_uri_puts(ku, "&") || /* XXX */
	    ! kcn_uri_puts(ku, param) ||
	    ! kcn_uri_puts(ku, "="))
		return false;

	/*
	 * XXX: should convert to utf-8 when character encodings
	 *	of input string is different.
	 */
	for (i = 0; i < argc; i++) {
		if (i > 0 && ! kcn_uri_encputs(ku, " "))
			return false;
		if (! kcn_uri_encputs(ku, argv[i]))
			return false;
	}

	return true;
}
