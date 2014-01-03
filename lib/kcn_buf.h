#include <sys/queue.h>

struct kcn_buf_data;

STAILQ_HEAD(kcn_buf_queue, kcn_buf_data);

struct kcn_buf {
	struct kcn_buf_data *kb_kbd;
	size_t kb_cp;
};

void kcn_buf_queue_init(struct kcn_buf_queue *);
void kcn_buf_init(struct kcn_buf *, struct kcn_buf_data *);
struct kcn_buf_data *kcn_buf_data_new(size_t);
void kcn_buf_data_destroy(struct kcn_buf_data *);
size_t kcn_buf_len(const struct kcn_buf *);
void kcn_buf_reset(struct kcn_buf *, size_t);
size_t kcn_buf_headingdata(const struct kcn_buf *);
size_t kcn_buf_trailingdata(const struct kcn_buf *);
void kcn_buf_forward(struct kcn_buf *, size_t);
void kcn_buf_backward(struct kcn_buf *, size_t);
void kcn_buf_start(struct kcn_buf *);
void kcn_buf_end(struct kcn_buf *);
void *kcn_buf_head(const struct kcn_buf *);
void *kcn_buf_current(const struct kcn_buf *);
void *kcn_buf_tail(const struct kcn_buf *);
void kcn_buf_prepend(struct kcn_buf *, size_t);
void kcn_buf_trim_head(struct kcn_buf *, size_t);
void kcn_buf_realign(struct kcn_buf *);
uint8_t kcn_buf_get8(struct kcn_buf *);
uint16_t kcn_buf_get16(struct kcn_buf *);
uint32_t kcn_buf_get32(struct kcn_buf *);
uint64_t kcn_buf_get64(struct kcn_buf *);
void kcn_buf_put8(struct kcn_buf *, uint8_t);
void kcn_buf_put16(struct kcn_buf *, uint16_t);
void kcn_buf_put32(struct kcn_buf *, uint32_t);
void kcn_buf_put64(struct kcn_buf *, uint64_t);
void kcn_buf_put(struct kcn_buf *, const void *, size_t);
void kcn_buf_putnull(struct kcn_buf *, size_t);
bool kcn_buf_enqueue(struct kcn_buf *, struct kcn_buf_queue *);
bool kcn_buf_fetch(struct kcn_buf *, struct kcn_buf_queue *);
void kcn_buf_drop(struct kcn_buf *, struct kcn_buf_queue *);
void kcn_buf_purge(struct kcn_buf_queue *);
int kcn_buf_read(int, struct kcn_buf *);
int kcn_buf_write(int, struct kcn_buf *);
void kcn_buf_dump(const struct kcn_buf *, size_t);
