#ifdef __linux__ /* XXX hack for strcasestr(). */
#define _GNU_SOURCE
#endif /* __linux__ */
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "kcn.h"
#include "kcn_info.h"
#include "kcn_db.h"
#include "kcn_netstat.h"

enum kcn_netstat_type {
	KCN_NETSTAT_TYPE_NONE,
	KCN_NETSTAT_TYPE_STORAGE,	/* XXX: acutually, not network */
	KCN_NETSTAT_TYPE_CPULOAD,	/* XXX: acutually, not network */
	KCN_NETSTAT_TYPE_TRAFFIC,
	KCN_NETSTAT_TYPE_LATENCY,
	KCN_NETSTAT_TYPE_HOPCOUNT,
	KCN_NETSTAT_TYPE_ASPATHLEN,
	KCN_NETSTAT_TYPE_WLANASSOC
};

enum kcn_netstat_operator {
	KCN_NETSTAT_OP_NONE,
	KCN_NETSTAT_OP_LT,
	KCN_NETSTAT_OP_LE,
	KCN_NETSTAT_OP_EQ,
	KCN_NETSTAT_OP_GT,
	KCN_NETSTAT_OP_GE
};

struct kcn_netstat_formula {
	enum kcn_netstat_type knf_type;
	enum kcn_netstat_operator knf_op;
	unsigned long long knf_val;
};

struct kcn_netstat_record {
	unsigned long long knr_val;
	const char *knr_uri;
};

static struct kcn_netstat_record kcn_netstat_storage_db[] = {
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

static struct kcn_netstat_record kcn_netstat_latency_db[] = {
	{ 200ULL,	"mobile-ip.org" },
	{ 3ULL,		"wlan-exhibits-rtr.sc13.org" },
	{ 175ULL,	"www.sinet.ad.jp" },
	{ 30ULL,	"64.125.12.86" },
	{ 67ULL,	"129.250.3.190" }
};

static bool kcn_netstat_match(const char *, size_t *);
static bool kcn_netstat_search(struct kcn_info *, const char *);

static struct kcn_db kcn_netstat = {
	.kd_type = KCN_TYPE_NETSTAT,
	.kd_prio = 255,
	.kd_name = "net",
	.kd_desc = "Network statistics",
	.kd_match = kcn_netstat_match,
	.kd_search = kcn_netstat_search
};

void
kcn_netstat_init(void)
{

	kcn_db_register(&kcn_netstat);
}

void
kcn_netstat_finish(void)
{

	kcn_db_deregister(&kcn_netstat);
}

static bool
kcn_netstat_match(const char *s, size_t *scorep)
{
	size_t score;

	score = 0;
#define MATCH(a)							\
do {									\
	if (strcasestr(s, (a)) != NULL)					\
		++score;						\
} while (0/*CONSTCOND*/)
	MATCH("server");
	MATCH("network");
	MATCH("terminal");
	MATCH("host");
	MATCH("router");
	MATCH("switch");
	MATCH("equipment");
	MATCH("HDD");
	MATCH("storage");
	MATCH("CPU");
	MATCH("load");
	MATCH("traffic");
	MATCH("latency");
	MATCH("hop");
	MATCH("ttl");
	MATCH("as");
	MATCH("assoc");
	MATCH("less");
	MATCH("greater");
	MATCH("equal");
	MATCH("than");
	MATCH("ge");
	MATCH("gt");
	MATCH("eq");
	MATCH("le");
	MATCH("lt");
	MATCH("max");
	MATCH("min");
#undef MATCH
	*scorep = score;
	errno = 0; /* XXX: should cause errors when format is invalid. */
	if (score == 0)
		return false;
	else
		return true;
}

static enum kcn_netstat_type
kcn_netstat_type_parse(const char *s)
{
#define ISMATCH(a)							\
	(strncasecmp((a), s, sizeof(a)) == 0 ? true : false)
	if (ISMATCH("HDD") || ISMATCH("storage"))
		return KCN_NETSTAT_TYPE_STORAGE;
	else if (ISMATCH("CPU") || ISMATCH("load"))
		return KCN_NETSTAT_TYPE_CPULOAD;
	else if (ISMATCH("traffic"))
		return KCN_NETSTAT_TYPE_TRAFFIC;
	else if (ISMATCH("latency"))
		return KCN_NETSTAT_TYPE_LATENCY;
	else if (ISMATCH("hop") || ISMATCH("ttl"))
		return KCN_NETSTAT_TYPE_HOPCOUNT;
	else if (ISMATCH("as"))
		return KCN_NETSTAT_TYPE_ASPATHLEN;
	else if (ISMATCH("assoc"))
		return KCN_NETSTAT_TYPE_WLANASSOC;
	else
		return KCN_NETSTAT_TYPE_NONE;
#undef ISMATCH
}

static enum kcn_netstat_operator
kcn_netstat_operator_parse(const char *w)
{

	if (strcasecmp("<", w) == 0 ||
	    strcasecmp("lt", w) == 0)
		return KCN_NETSTAT_OP_LT;
	if (strcasecmp("<=", w) == 0 ||
	    strcasecmp("le", w) == 0)
		return KCN_NETSTAT_OP_LE;
	if (strcasecmp("=", w) == 0 ||
	    strcasecmp("==", w) == 0 ||
	    strcasecmp("eq", w) == 0)
		return KCN_NETSTAT_OP_EQ;
	if (strcasecmp(">", w) == 0 ||
	    strcasecmp("gt", w) == 0)
		return KCN_NETSTAT_OP_GT;
	if (strcasecmp(">=", w) == 0 ||
	    strcasecmp("ge", w) == 0)
		return KCN_NETSTAT_OP_GE;
	return KCN_NETSTAT_OP_NONE;
}

static bool
kcn_netstat_value_parse(const char *w, unsigned long long *valp)
{
	int oerrno;
	char *ep;

	oerrno = errno;
	errno = 0;
	*valp = strtoull(w, &ep, 10);
	/* XXX: other error handling. */
	if (errno != 0)
		return false;
	errno = oerrno;
	return true;
}

static bool
kcn_netstat_compile(size_t keyc, char * const keyv[],
    struct kcn_netstat_formula *knf)
{

	if (keyc < 3)
		goto bad;
	knf->knf_type = kcn_netstat_type_parse(keyv[0]);
	if (knf->knf_type == KCN_NETSTAT_TYPE_NONE)
		goto bad;
	knf->knf_op = kcn_netstat_operator_parse(keyv[1]);
	if (knf->knf_op == KCN_NETSTAT_OP_NONE)
		goto bad;
	if (! kcn_netstat_value_parse(keyv[2], &knf->knf_val))
		goto bad;
	return true;
  bad:
	errno = EINVAL; /* XXX */
	return false;
}

static bool
kcn_netstat_search_db(struct kcn_info *ki,
    const struct kcn_netstat_formula *knf,
    const struct kcn_netstat_record *db, size_t dbcount)
{
	size_t i, n;
	n = 0;
	for (i = 0; i < dbcount; i++) {
		switch (knf->knf_op) {
		case KCN_NETSTAT_OP_LT:
			if (db[i].knr_val >= knf->knf_val)
				continue;
			break;
		case KCN_NETSTAT_OP_LE:
			if (db[i].knr_val > knf->knf_val)
				continue;
			break;
		case KCN_NETSTAT_OP_EQ:
			if (db[i].knr_val != knf->knf_val)
				continue;
			break;
		case KCN_NETSTAT_OP_GT:
			if (db[i].knr_val <= knf->knf_val)
				continue;
			break;
		case KCN_NETSTAT_OP_GE:
			if (db[i].knr_val < knf->knf_val)
				continue;
			break;
		default:
			assert(0);
			continue;
		}
		if (! kcn_info_loc_add(ki, db[i].knr_uri))
			return false;
		if (++n >= kcn_info_maxnlocs(ki))
			break;
	}

	return true;
}

static bool
kcn_netstat_search(struct kcn_info *ki, const char *keys)
{
	struct kcn_netstat_formula knf;
	size_t keyc;
	char **keyv;
	bool rc;

	keyv = kcn_key_split(keys, &keyc);
	if (keyv == NULL) {
		errno = EINVAL;
		goto bad;
	}
	if (! kcn_netstat_compile(keyc, keyv, &knf)) {
		errno = EINVAL;
		goto bad;
	}
#define SEARCH(db)							\
	kcn_netstat_search_db(ki, &knf, (db), sizeof(db) / sizeof((db)[0]))
	switch (knf.knf_type) {
	case KCN_NETSTAT_TYPE_LATENCY:
		rc = SEARCH(kcn_netstat_latency_db);
		break;
	case KCN_NETSTAT_TYPE_STORAGE:
		rc = SEARCH(kcn_netstat_storage_db);
		break;
	default:
		errno = EOPNOTSUPP;
		goto bad;
	}
#undef SEARCH
	if (rc == false)
		goto bad;
	if (kcn_info_nlocs(ki) == 0) {
		errno = ESRCH;
		goto bad;
	}
	kcn_key_free(keyc, keyv);
	return true;
  bad:
	if (keyv != NULL)
		kcn_key_free(keyc, keyv);
	return false;
}
