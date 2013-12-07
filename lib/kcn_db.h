#include <sys/queue.h>

struct kcn_db {
	TAILQ_ENTRY(kcn_db) kd_chain;
	enum kcn_type kd_type;
	size_t kd_prio;
	bool (*kd_match)(const char *, size_t *);
	bool (*kd_search)(struct kcn_info *, const char *);
};

void kcn_db_register(struct kcn_db *);
void kcn_db_deregister(struct kcn_db *);
bool kcn_db_search(struct kcn_info *, const char *);
