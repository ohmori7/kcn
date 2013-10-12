struct buf {
	void *b_ptr;
	size_t b_size;
};

void buf_init(struct buf *);
void buf_finish(struct buf *);
bool buf_append(struct buf *, const void *, size_t);
