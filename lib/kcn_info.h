#define KCN_INFO_COUNTRYSTRLEN	3
#define KCN_INFO_NLOCSTRLEN						\
	(sizeof(size_t /* ki->ki_maxnlocs */ ) * NBBY / 3 /*approximate */)

enum kcn_type {
	KCN_TYPE_NONE,
	KCN_TYPE_GOOGLE,
	KCN_TYPE_NETSTAT
};

enum kcn_loc_type {
	KCN_LOC_TYPE_DOMAINNAME,
	KCN_LOC_TYPE_URI
};

struct kcn_info;

struct kcn_info *kcn_info_new(enum kcn_loc_type, size_t);
void kcn_info_destroy(struct kcn_info *);
void kcn_info_type_set(struct kcn_info *, enum kcn_type);
enum kcn_type kcn_info_type(const struct kcn_info *);
enum kcn_loc_type kcn_info_loc_type(const struct kcn_info *);
size_t kcn_info_maxnlocs(const struct kcn_info *);
void kcn_info_country_set(struct kcn_info *, const char *);
const char *kcn_info_country(const struct kcn_info *);
void kcn_info_userip_set(struct kcn_info *, const char *);
const char *kcn_info_userip(const struct kcn_info *);
size_t kcn_info_nlocs(const struct kcn_info *);
const char *kcn_info_loc(const struct kcn_info *, size_t);
bool kcn_info_loc_add(struct kcn_info *, const char *);
