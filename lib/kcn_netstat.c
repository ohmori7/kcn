#ifdef __linux__ /* XXX hack for strcasestr(). */
#define _GNU_SOURCE
#endif /* __linux__ */

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "kcn.h"
#include "kcn_info.h"
#include "kcn_eq.h"
#include "kcn_db.h"
#include "kcn_pkt.h"
#include "kcn_time.h"
#include "kcn_msg.h"
#include "kcn_client.h"
#include "kcn_netstat.h"

static bool kcn_netstat_match(const char *, size_t *);
static bool kcn_netstat_search(struct kcn_info *, const char *);

static struct kcn_db kcn_netstat = {
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

static bool
kcn_netstat_compile(size_t keyc, char * const keyv[], struct kcn_eq *ke)
{

	errno = 0;
	if (keyc < 3)
		return false;
	if (! kcn_eq_type_aton(keyv[0], &ke->ke_type))
		return false;
	if (! kcn_eq_operator_aton(keyv[1], &ke->ke_op))
		return false;
	if (! kcn_eq_val_aton(keyv[2], &ke->ke_val))
		return false;
	if (keyc == 3) {
		ke->ke_start = KCN_TIME_NOW;
		if (time(&ke->ke_end) == -1)
			return false;
		ke->ke_end -= KCN_TIME_JITTER;
	} else {
		/* XXX: should compile time expressions. */
	}
	return true;
}

static bool
kcn_netstat_search(struct kcn_info *ki, const char *keys)
{
	struct kcn_msg_query kmq;
	size_t keyc;
	char **keyv;

	keyv = kcn_key_split(keys, &keyc);
	if (keyv == NULL) {
		errno = EINVAL;
		goto bad;
	}
	kmq.kmq_loctype = kcn_info_loc_type(ki);
	kmq.kmq_maxcount = kcn_info_maxnlocs(ki);
	if (! kcn_netstat_compile(keyc, keyv, &kmq.kmq_eq))
		goto bad;
	if (! kcn_client_search(ki, &kmq))
		goto bad;
	kcn_key_free(keyc, keyv);
	return true;
  bad:
	kcn_key_free(keyc, keyv);
	return false;
}
