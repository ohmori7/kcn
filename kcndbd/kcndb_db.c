#include <stdbool.h>
#include <stddef.h>

#include <dirent.h>

#include "kcn.h"
#include "kcn_log.h"

#include "kcndb_db.h"

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
