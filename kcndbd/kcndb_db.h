#define KCNDB_DB_PATH_DEFAULT	"/var/db/kcndb"

struct kcndb_db_record {
	time_t kdr_time;
	unsigned long long kdr_val;
	const char *kdr_loc;
	size_t kdr_loclen;
};

struct kcndb_db_table;

void kcndb_db_path_set(const char *);
const char *kcndb_db_path_get(void);
bool kcndb_db_init(void);
void kcndb_db_finish(void);
struct kcndb_db_table *kcndb_db_table_create(enum kcn_formula_type);
void kcndb_db_table_close(struct kcndb_db_table *);
bool kcndb_db_record_add(struct kcndb_db_table *, struct kcndb_db_record *);
bool kcndb_db_search(struct kcn_info *, const struct kcn_formula *);
