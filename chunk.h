struct chunk {
	void *c_ptr;
	size_t c_size;
};

void chunk_init(struct chunk *);
void chunk_finish(struct chunk *);
bool chunk_append(struct chunk *, const void *, size_t);
