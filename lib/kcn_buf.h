struct kcn_buf {
	void *kb_ptr;
	size_t kb_size;
};

void kcn_buf_init(struct kcn_buf *);
void kcn_buf_finish(struct kcn_buf *);
bool kcn_buf_append(struct kcn_buf *, const void *, size_t);
