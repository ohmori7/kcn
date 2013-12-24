#define KCNDB_DB_PATH_DEFAULT	"/var/db/kcndb"

void kcndb_db_path_set(const char *);
const char *kcndb_db_path_get(void);
bool kcndb_db_init(void);
void kcndb_db_finish(void);
bool kcndb_db_search_all(struct kcn_info *, const struct kcn_formula *);
