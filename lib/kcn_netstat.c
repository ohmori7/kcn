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

size_t
kcn_netstat_match(const char *s)
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
	return score;
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
kcn_netstat_search_storage(struct kcn_info *ki,
    const struct kcn_netstat_formula *knf)
{
	size_t i, n;
	struct {
		unsigned long long total;
		unsigned long long free;
		const char *uri;
	} storages[] = {
#define BBASE	(1024ULL)
#define KB	BBASE
#define MB	(KB * BBASE)
#define GB	(MB * BBASE)
#define TB	(GB * BBASE)
		{ 10ULL * GB,	100ULL * MB,	"kcn1.mobile-ip.org" },
		{ 20ULL * GB,	10ULL * MB,	"kcn2.mobile-ip.org" },
		{ 100ULL * GB,	50ULL * GB,	"kcn3.mobile-ip.org" },
		{ 1ULL * TB,	500ULL * GB,	"kcn4.mobile-ip.org" },
	};

	n = 0;
	for (i = 0; i < sizeof(storages) / sizeof(storages[0]); i++) {
		switch (knf->knf_op) {
		case KCN_NETSTAT_OP_LT:
			if (storages[i].free >= knf->knf_val)
				continue;
			break;
		case KCN_NETSTAT_OP_LE:
			if (storages[i].free > knf->knf_val)
				continue;
			break;
		case KCN_NETSTAT_OP_EQ:
			if (storages[i].free != knf->knf_val)
				continue;
			break;
		case KCN_NETSTAT_OP_GT:
			if (storages[i].free <= knf->knf_val)
				continue;
			break;
		case KCN_NETSTAT_OP_GE:
			if (storages[i].free < knf->knf_val)
				continue;
			break;
		default:
			assert(0);
			continue;
		}
		if (! kcn_info_loc_add(ki, storages[i].uri))
			return false;
		if (++n >= kcn_info_maxnlocs(ki))
			break;
	}

	return true;
}

bool
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
	switch (knf.knf_type) {
	case KCN_NETSTAT_TYPE_STORAGE:
		rc = kcn_netstat_search_storage(ki, &knf);
		break;
	default:
		errno = EOPNOTSUPP;
		rc = false;
	}
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
