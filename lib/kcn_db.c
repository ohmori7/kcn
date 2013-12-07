#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>

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
		assert(kd->kd_type != kdn->kd_type);
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

/* XXX: there might be the case where keywords are ambiguous or inconsistent. */
static const struct kcn_db *
kcn_db_lookup(const char *keys)
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
kcn_db_lookup_by_type(enum kcn_type type)
{
	struct kcn_db *kd;

	TAILQ_FOREACH(kd, &kcn_db_list, kd_chain)
		if (kd->kd_type == type)
			return kd;
	KCN_LOG(DEBUG, "unknown database type: %u", type);
	errno = EINVAL;
	return NULL;
}

bool
kcn_db_search(struct kcn_info *ki, const char *keys)
{
	enum kcn_type type;
	const struct kcn_db *kd;

	type = kcn_info_type(ki);
	if (type == KCN_TYPE_NONE)
		kd = kcn_db_lookup(keys);
	else
		kd = kcn_db_lookup_by_type(type);
	if (kd == NULL)
		return false;
	return (*kd->kd_search)(ki, keys);
}
