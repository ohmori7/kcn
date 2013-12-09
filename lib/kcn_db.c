#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "kcn.h"
#include "kcn_log.h"
#include "kcn_info.h"
#include "kcn_db.h"

static TAILQ_HEAD(, kcn_db) kcn_db_list = TAILQ_HEAD_INITIALIZER(kcn_db_list);

void
kcn_db_register(struct kcn_db *kdn)
{
	struct kcn_db *kd;

	KCN_LOG(INFO, "register \"%s\" database", kdn->kd_desc);
	TAILQ_FOREACH(kd, &kcn_db_list, kd_chain) {
		assert(kd != kdn);
		if (kd->kd_prio < kdn->kd_prio) {
			TAILQ_INSERT_BEFORE(kd, kdn, kd_chain);
			return;
		}
	}
	TAILQ_INSERT_TAIL(&kcn_db_list, kdn, kd_chain);
}

void
kcn_db_deregister(struct kcn_db *kd)
{

	KCN_LOG(INFO, "deregister \"%s\" database", kd->kd_desc);
	TAILQ_REMOVE(&kcn_db_list, kd, kd_chain);
}

static const struct kcn_db *
kcn_db_match(const char *keys)
{
	struct kcn_db *kd, *kdmatch;
	size_t score, scorematch;

	scorematch = 0;
	kdmatch = NULL;
	TAILQ_FOREACH(kd, &kcn_db_list, kd_chain) {
		if (! (*kd->kd_match)(keys, &score)) {
			if (errno != 0) {
				KCN_LOG(DEBUG, "invalid syntax");
				errno = EINVAL;
				return NULL;
			}
			continue;
		}
		if (kdmatch == NULL || score > scorematch) {
			scorematch = score;
			kdmatch = kd;
		}
	}
	if (kdmatch == NULL)
		errno = EPROTONOSUPPORT;
	return kdmatch;
}

static const struct kcn_db *
kcn_db_lookup(const char *name)
{
	const struct kcn_db *kd;

	TAILQ_FOREACH(kd, &kcn_db_list, kd_chain)
		if (strcasecmp(kd->kd_name, name) == 0)
			return kd;
	errno = ESRCH;
	return NULL;
}

bool
kcn_db_exists(const char *name)
{

	return kcn_db_lookup(name) != NULL;
}

void
kcn_db_name_list_puts(const char *prefix)
{
	const struct kcn_db *kd;

	TAILQ_FOREACH(kd, &kcn_db_list, kd_chain)
		fprintf(stderr, "%s%s: %s\n", prefix, kd->kd_name, kd->kd_desc);
}

bool
kcn_db_search(struct kcn_info *ki, const char *keys)
{
	const char *name;
	const struct kcn_db *kd;

	name = kcn_info_db(ki);
	if (name == NULL)
		kd = kcn_db_match(keys);
	else
		kd = kcn_db_lookup(name);
	if (kd == NULL)
		return false;
	KCN_LOG(DEBUG, "select a database of \"%s\"", kd->kd_name);
	return (*kd->kd_search)(ki, keys);
}
