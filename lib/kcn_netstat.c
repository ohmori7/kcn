#ifdef __linux__ /* XXX hack for strcasestr(). */
#define _GNU_SOURCE
#endif /* __linux__ */
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
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
#undef MATCH
	return score;
}

static enum kcn_netstat_type
kcn_netstat_type_guess(const char *s)
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

bool
kcn_netstat_search(struct kcn_info *ki, const char *keys)
{
	enum kcn_netstat_type type;

	type = kcn_netstat_type_guess(keys);
	if (type == KCN_NETSTAT_TYPE_NONE) {
		errno = EINVAL;
		return false;
	}

	(void)ki;
	errno = EPROTONOSUPPORT;
	return false;
}
