struct kcn_db;

struct kcb_db *kcn_db_open(void);
void kcn_db_close(struct kcn_db *);
bool kcn_db_add(struct kcn_db *, const char *, const char *);
bool kcn_db_del(struct kcn_db *, const char *);

