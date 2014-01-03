#define KCNDB_FILE_BUFSIZ	4096	/* XXX */

struct kcndb_file *kcndb_file_open(const char *);
void kcndb_file_close(struct kcndb_file *);
int kcndb_file_fd(const struct kcndb_file *);
size_t kcndb_file_size(const struct kcndb_file *);
struct kcn_buf *kcndb_file_buf(struct kcndb_file *);
bool kcndb_file_ensure(struct kcndb_file *, size_t);
bool kcndb_file_seek_head(struct kcndb_file *, off_t);
bool kcndb_file_write(struct kcndb_file *);
bool kcndb_file_append(struct kcndb_file *);
