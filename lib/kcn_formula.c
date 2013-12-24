#ifdef __linux__ /* XXX hack for strcasestr(). */
#define _GNU_SOURCE
#endif /* __linux__ */

#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <string.h>

#include "kcn_str.h"
#include "kcn_formula.h"

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
	else if (ISMATCH("latency"))
		*typep = KCN_FORMULA_TYPE_LATENCY;
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
