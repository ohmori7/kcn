#include <sys/queue.h>

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <dirent.h>

#include "kcn.h"
#include "kcn_info.h"
#include "kcn_formula.h"
#include "kcn_log.h"

#include "kcndb_db.h"

struct kcndb_db_record {
	unsigned long long kdr_val;
	const char *kdr_uri;
};

static struct kcndb_db_record kcndb_db_storage[] = {
#define BBASE	(1024ULL)
#define KB	BBASE
#define MB	(KB * BBASE)
#define GB	(MB * BBASE)
#define TB	(GB * BBASE)
	{ 100ULL * GB,	"mobile-ip.org" },
	{ 44ULL * GB,	"www.qgpop.net" },
	{ 10ULL * GB,	"kcn3.mobile-ip.org" },
	{ 5ULL * GB,	"kcn4.mobile-ip.org" },
};

static struct kcndb_db_record kcndb_db_latency[] = {
	{ 200ULL,	"mobile-ip.org" },
	{ 3ULL,		"wlan-exhibits-rtr.sc13.org" },
	{ 175ULL,	"www.sinet.ad.jp" },
	{ 30ULL,	"64.125.12.86" },
	{ 67ULL,	"129.250.3.190" }
};

static const char *kcndb_db_path = KCNDB_DB_PATH_DEFAULT;
static DIR *kcndb_db_dir = NULL;

static void kcndb_db_closedir(void);

static bool
kcndb_db_opendir(const char *path)
{
	DIR *ndir;

	ndir = opendir(path);
	if (ndir == NULL)
		return false;
	kcndb_db_closedir();
	kcndb_db_dir = ndir;
	return true;
}

static void
kcndb_db_closedir(void)
{

	if (kcndb_db_dir == NULL)
		return;
	closedir(kcndb_db_dir);
	kcndb_db_dir = NULL;
}

bool
kcndb_db_path_set(const char *path)
{
	static bool iscalled = false;

	if (iscalled)
		return false;
	if (! kcndb_db_opendir(path))
		return false;
	iscalled = true;
	kcndb_db_path = path;
	return true;
}

static bool
kcndb_db_search(struct kcn_info *ki, const struct kcn_formula *kf,
    const struct kcndb_db_record *kdr, size_t dbcount)
{
	size_t i, n;

	n = 0;
	for (i = 0; i < dbcount; i++) {
		switch (kf->kf_op) {
		case KCN_FORMULA_OP_LT:
			if (kdr[i].kdr_val >= kf->kf_val)
				continue;
			break;
		case KCN_FORMULA_OP_LE:
			if (kdr[i].kdr_val > kf->kf_val)
				continue;
			break;
		case KCN_FORMULA_OP_EQ:
			if (kdr[i].kdr_val != kf->kf_val)
				continue;
			break;
		case KCN_FORMULA_OP_GT:
			if (kdr[i].kdr_val <= kf->kf_val)
				continue;
			break;
		case KCN_FORMULA_OP_GE:
			if (kdr[i].kdr_val < kf->kf_val)
				continue;
			break;
		default:
			assert(0);
			continue;
		}
		if (! kcn_info_loc_add(ki, kdr[i].kdr_uri,
		    strlen(kdr[i].kdr_uri)))
			return false;
		if (++n >= kcn_info_maxnlocs(ki))
			break;
	}

	return true;
}

bool
kcndb_db_search_all(struct kcn_info *ki, const struct kcn_formula *kf)
{
	bool rc;

#define SEARCH(db)							\
	kcndb_db_search(ki, kf, (db), sizeof(db) / sizeof((db)[0]))
	switch (kf->kf_type) {
	case KCN_FORMULA_TYPE_LATENCY:
		rc = SEARCH(kcndb_db_latency);
		break;
	case KCN_FORMULA_TYPE_STORAGE:
		rc = SEARCH(kcndb_db_storage);
		break;
	default:
		rc = false;
		errno = EOPNOTSUPP;
	}
#undef SEARCH
	return rc;
}
