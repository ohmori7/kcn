#define KCNDB_DB_PATH_DEFAULT	"/var/db/kcndb"

bool kcndb_db_path_set(const char *);
bool kcndb_db_search_all(struct kcn_info *, const struct kcn_formula *);
