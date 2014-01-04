#ifdef __linux__ /* XXX hack for strcasestr(). */
#define _GNU_SOURCE
#endif /* __linux__ */

#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

#include "kcn_str.h"
#include "kcn_time.h"
#include "kcn_eq.h"

const char *
kcn_eq_type_ntoa(enum kcn_eq_type type)
{
	static const char *name[KCN_EQ_TYPE_MAX] = {
		[KCN_EQ_TYPE_NONE]	= "none",
		[KCN_EQ_TYPE_STORAGE]	= "storage",
		[KCN_EQ_TYPE_CPULOAD]	= "cpu",
		[KCN_EQ_TYPE_TRAFFIC]	= "traffic",
		[KCN_EQ_TYPE_RTT]	= "rtt",
		[KCN_EQ_TYPE_HOPCOUNT]	= "hopcount",
		[KCN_EQ_TYPE_ASPATHLEN]	= "aspathlen",
		[KCN_EQ_TYPE_WLANASSOC]	= "wlanassoc"
	};
	if (type <= KCN_EQ_TYPE_NONE || type >= KCN_EQ_TYPE_MAX) {
		errno = ENOENT;
		return NULL;
	}
	return name[type];
}

bool
kcn_eq_type_aton(const char *s, enum kcn_eq_type *typep)
{
#define ISMATCH(a)							\
	(strncasecmp((a), s, sizeof(a)) == 0 ? true : false)
	if (ISMATCH("hdd") || ISMATCH("storage"))
		*typep = KCN_EQ_TYPE_STORAGE;
	else if (ISMATCH("cpu") || ISMATCH("load"))
		*typep = KCN_EQ_TYPE_CPULOAD;
	else if (ISMATCH("traffic"))
		*typep = KCN_EQ_TYPE_TRAFFIC;
	else if (ISMATCH("rtt"))
		*typep = KCN_EQ_TYPE_RTT;
	else if (ISMATCH("hopcount") || ISMATCH("ttl"))
		*typep = KCN_EQ_TYPE_HOPCOUNT;
	else if (ISMATCH("aspathlen"))
		*typep = KCN_EQ_TYPE_ASPATHLEN;
	else if (ISMATCH("wlanassoc"))
		*typep = KCN_EQ_TYPE_WLANASSOC;
	else {
		errno = ENOENT;
		return false;
	}
#undef ISMATCH
	return true;
}

bool
kcn_eq_operator_aton(const char *w, enum kcn_eq_operator *opp)
{

	if (strcasecmp("<", w) == 0 ||
	    strcasecmp("lt", w) == 0)
		*opp = KCN_EQ_OP_LT;
	else if (strcasecmp("<=", w) == 0 ||
	    strcasecmp("le", w) == 0)
		*opp = KCN_EQ_OP_LE;
	else if (strcasecmp("=", w) == 0 ||
	    strcasecmp("==", w) == 0 ||
	    strcasecmp("eq", w) == 0)
		*opp = KCN_EQ_OP_EQ;
	else if (strcasecmp(">", w) == 0 ||
	    strcasecmp("gt", w) == 0)
		*opp = KCN_EQ_OP_GT;
	else if (strcasecmp(">=", w) == 0 ||
	    strcasecmp("ge", w) == 0)
		*opp = KCN_EQ_OP_GE;
	else {
		errno = ENOENT;
		return false;
	}
	return true;
}

bool
kcn_eq_val_aton(const char *w, unsigned long long *valp)
{

	return kcn_strtoull(w, 0, ULLONG_MAX, valp);
}

bool
kcn_eq_time_match(time_t t, const struct kcn_eq *ke)
{

	if (ke->ke_end != KCN_TIME_NOW && t > ke->ke_end)
		return false;
	if (t < ke->ke_start)
		return false;
	return true;
}
