#ifdef __linux__ /* XXX hack for strcasestr(). */
#define _GNU_SOURCE
#endif /* __linux__ */

#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <string.h>

#include "kcn_str.h"
#include "kcn_formula.h"

const char *
kcn_formula_type_ntoa(enum kcn_formula_type type)
{
	static const char *name[KCN_FORMULA_TYPE_MAX] = {
		[KCN_FORMULA_TYPE_NONE]		= "none",
		[KCN_FORMULA_TYPE_STORAGE]	= "storage",
		[KCN_FORMULA_TYPE_CPULOAD]	= "cpu",
		[KCN_FORMULA_TYPE_TRAFFIC]	= "traffic",
		[KCN_FORMULA_TYPE_RTT]		= "rtt",
		[KCN_FORMULA_TYPE_HOPCOUNT]	= "hopcount",
		[KCN_FORMULA_TYPE_ASPATHLEN]	= "aspathlen",
		[KCN_FORMULA_TYPE_WLANASSOC]	= "wlanassoc"
	};
	if (type <= KCN_FORMULA_TYPE_NONE || type >= KCN_FORMULA_TYPE_MAX) {
		errno = ENOENT;
		return NULL;
	}
	return name[type];
}

bool
kcn_formula_type_aton(const char *s, enum kcn_formula_type *typep)
{
#define ISMATCH(a)							\
	(strncasecmp((a), s, sizeof(a)) == 0 ? true : false)
	if (ISMATCH("HDD") || ISMATCH("storage"))
		*typep = KCN_FORMULA_TYPE_STORAGE;
	else if (ISMATCH("CPU") || ISMATCH("load"))
		*typep = KCN_FORMULA_TYPE_CPULOAD;
	else if (ISMATCH("traffic"))
		*typep = KCN_FORMULA_TYPE_TRAFFIC;
	else if (ISMATCH("rtt"))
		*typep = KCN_FORMULA_TYPE_RTT;
	else if (ISMATCH("hop") || ISMATCH("ttl"))
		*typep = KCN_FORMULA_TYPE_HOPCOUNT;
	else if (ISMATCH("as"))
		*typep = KCN_FORMULA_TYPE_ASPATHLEN;
	else if (ISMATCH("assoc"))
		*typep = KCN_FORMULA_TYPE_WLANASSOC;
	else {
		errno = ENOENT;
		return false;
	}
#undef ISMATCH
	return true;
}

bool
kcn_formula_operator_aton(const char *w, enum kcn_formula_operator *opp)
{

	if (strcasecmp("<", w) == 0 ||
	    strcasecmp("lt", w) == 0)
		*opp = KCN_FORMULA_OP_LT;
	else if (strcasecmp("<=", w) == 0 ||
	    strcasecmp("le", w) == 0)
		*opp = KCN_FORMULA_OP_LE;
	else if (strcasecmp("=", w) == 0 ||
	    strcasecmp("==", w) == 0 ||
	    strcasecmp("eq", w) == 0)
		*opp = KCN_FORMULA_OP_EQ;
	else if (strcasecmp(">", w) == 0 ||
	    strcasecmp("gt", w) == 0)
		*opp = KCN_FORMULA_OP_GT;
	else if (strcasecmp(">=", w) == 0 ||
	    strcasecmp("ge", w) == 0)
		*opp = KCN_FORMULA_OP_GE;
	else {
		errno = ENOENT;
		return false;
	}
	return true;
}

bool
kcn_formula_val_aton(const char *w, unsigned long long *valp)
{

	return kcn_strtoull(w, 0, ULLONG_MAX, valp);
}
