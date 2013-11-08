#include <errno.h>
#include <stdbool.h>
#include <stddef.h>

#include "kcn_db.h"

struct kcb_db *
kcn_db_open(void)
{

	return NULL;
}

void
kcn_db_close(struct kcn_db *kdb)
{

	(void)kdb;
}

bool
kcn_db_add(struct kcn_db *kdb, const char *key, const char *val)
{

	(void)kdb;
	(void)key;
	(void)val;
	errno = ENOSYS;
	return false;
}

bool
kcn_db_del(struct kcn_db *kdb, const char *key)
{

	(void)kdb;
	(void)key;
	errno = ENOSYS;
	return false;
}
