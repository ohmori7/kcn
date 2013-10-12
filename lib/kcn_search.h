struct kcn_search_res;

struct kcn_search_res *kcn_search_res_new(enum kcn_loc_type, size_t);
void kcn_search_res_destroy(struct kcn_search_res *);
size_t kcn_search_res_nlocs(const struct kcn_search_res *);
const char *kcn_search_res_loc(const struct kcn_search_res *, size_t);

int kcn_search(int, char * const [], struct kcn_search_res *);
