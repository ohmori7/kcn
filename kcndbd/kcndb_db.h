#define KCNDB_DB_PATH_DEFAULT		"/var/db/kcndb"
#define KCNDB_DB_PATH_LOC_SUFFIX	"-loc"

struct kcndb_db_record {
	time_t kdr_time;
	unsigned long long kdr_val;
	uint64_t kdr_locidx;
	const char *kdr_loc;
	size_t kdr_loclen;
};

struct kcndb_db;

void kcndb_db_path_set(const char *);
const char *kcndb_db_path_get(void);
bool kcndb_db_record_add(struct kcndb_db *, enum kcn_eq_type,
    struct kcndb_db_record *);
bool kcndb_db_search(struct kcndb_db *, const struct kcn_eq *, size_t,
    bool (*)(const struct kcndb_db_record *, size_t, void *), void *);
struct kcndb_db *kcndb_db_new(void);
void kcndb_db_destroy(struct kcndb_db *);
