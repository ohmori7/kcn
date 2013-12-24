#include <sys/queue.h>

struct kcn_db {
	TAILQ_ENTRY(kcn_db) kd_chain;
	size_t kd_prio;
	const char *kd_name;
	const char *kd_desc;
	bool (*kd_match)(const char *, size_t *);
	bool (*kd_search)(struct kcn_info *, const char *);
};

void kcn_db_register(struct kcn_db *);
void kcn_db_deregister(struct kcn_db *);
bool kcn_db_exists(const char *);
void kcn_db_name_list_puts(const char *);
bool kcn_db_search(struct kcn_info *, const char *);
