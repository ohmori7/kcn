#define KCN_INFO_DBNAMESTRLEN	max(sizeof("net"), sizeof("google"))
#define KCN_INFO_COUNTRYSTRLEN	3
#define KCN_INFO_NLOCSTRLEN						\
	(sizeof(size_t /* ki->ki_maxnlocs */ ) * NBBY / 3 /*approximate */)

struct kcn_info;

struct kcn_info *kcn_info_new(enum kcn_loc_type, size_t);
void kcn_info_destroy(struct kcn_info *);
enum kcn_loc_type kcn_info_loc_type(const struct kcn_info *);
size_t kcn_info_maxnlocs(const struct kcn_info *);
void kcn_info_db_set(struct kcn_info *, const char *);
const char *kcn_info_db(const struct kcn_info *);
void kcn_info_country_set(struct kcn_info *, const char *);
const char *kcn_info_country(const struct kcn_info *);
void kcn_info_userip_set(struct kcn_info *, const char *);
const char *kcn_info_userip(const struct kcn_info *);
size_t kcn_info_nlocs(const struct kcn_info *);
const char *kcn_info_loc(const struct kcn_info *, size_t);
size_t kcn_info_loc_score(const struct kcn_info *, size_t);
bool kcn_info_loc_add(struct kcn_info *, const char *, size_t, size_t);
void kcn_info_loc_free(struct kcn_info *);
