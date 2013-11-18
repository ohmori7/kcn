#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#include "kcn.h"
#include "kcn_info.h"
#include "kcn_google.h"
#include "kcn_netstat.h"
#include "kcn_search.h"

/* XXX: there might be the case where keywords are ambiguous or inconsistent. */
static enum kcn_type
kcn_search_type_guess(const char *keys)
{
	size_t score;

	score = kcn_netstat_match(keys);
#define KCN_SEARCH_MATCH_THRESHOLD	1 /* XXX */
	if (score > KCN_SEARCH_MATCH_THRESHOLD)
		return KCN_TYPE_NETSTAT;
	return KCN_TYPE_GOOGLE;
}

bool
kcn_search(struct kcn_info *ki, const char *keys)
{
	enum kcn_type type;

	type = kcn_info_type(ki);
	if (type == KCN_TYPE_NONE)
		type = kcn_search_type_guess(keys);

	switch (type) {
	case KCN_TYPE_GOOGLE:
		return kcn_google_search(ki, keys);
	case KCN_TYPE_NETSTAT:
		return kcn_netstat_search(ki, keys);
	default:
		errno = EPROTONOSUPPORT;
		return false;
	}
}

bool
kcn_searchv(struct kcn_info *ki, int keyc, char * const keyv[])
{
	bool rc;
	char *keys;

	keys = kcn_key_concat(keyc, keyv);
	if (keys == NULL)
		return false;
	rc = kcn_search(ki, keys);
	free(keys);
	return rc;
}
