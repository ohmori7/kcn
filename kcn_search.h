enum search_type {
	SEARCH_TYPE_DOMAINNAME,
	SEARCH_TYPE_URI
};

struct search_res;

struct search_res *search_res_new(enum search_type, size_t);
void search_res_destroy(struct search_res *);
size_t search_res_nlocs(const struct search_res *);
const char *search_res_loc(const struct search_res *, size_t);

int search(int, char * const [], struct search_res *);
