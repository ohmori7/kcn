#include <ctype.h>
#include <stdlib.h>

#include "kcn.h"
#include "kcn_token.h"

struct kcn_token {
	const char *kt_sp;
	const char *kt_ep;
};

struct kcn_token *
kcn_token_new(const char *s)
{
	struct kcn_token *kt;

	kt = malloc(sizeof(*kt));
	if (kt == NULL)
		return NULL;

	kt->kt_sp = kt->kt_ep = s;

	return kt;
}

void
kcn_token_destroy(struct kcn_token *kt)
{

	free(kt);
}

const char *
kcn_token_get(struct kcn_token *kt, size_t *tlenp)
{
	const char *cp, *t;

	for (cp = kt->kt_ep; isblank((unsigned char)*cp); cp++)
		;
	kt->kt_sp = cp;

	for (; *cp != '\0' && ! isblank((unsigned char)*cp); cp++)
		;
	kt->kt_ep = cp;

	*tlenp = kt->kt_ep - kt->kt_sp;

	if (*tlenp != 0)
		t = kt->kt_sp;
	else
		t = NULL;
	return t;
}
