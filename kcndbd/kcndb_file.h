enum kcndb_file_op {
	KCNDB_FILE_OP_READ,
	KCNDB_FILE_OP_WRITE
};

struct kcndb_file *kcndb_file_open(const char *, enum kcndb_file_op);
void kcndb_file_close(struct kcndb_file *);
struct kcn_pkt *kcndb_file_packet(struct kcndb_file *);
bool kcndb_file_ensure(struct kcndb_file *, size_t);
bool kcndb_file_write(struct kcndb_file *);
