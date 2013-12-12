struct kcn_pkt_data;

STAILQ_HEAD(kcn_pkt_queue, kcn_pkt_data);

struct kcn_pkt {
	struct kcn_pkt_data *kp_kpd;
	size_t kp_cp;
};

void kcn_pkt_queue_init(struct kcn_pkt_queue *);
void kcn_pkt_init(struct kcn_pkt *, struct kcn_pkt_data *);
struct kcn_pkt_data *kcn_pkt_data_new(size_t);
void kcn_pkt_data_destroy(struct kcn_pkt_data *);
size_t kcn_pkt_len(const struct kcn_pkt *);
void kcn_pkt_reset(struct kcn_pkt *);
size_t kcn_pkt_trailingspace(const struct kcn_pkt *);
size_t kcn_pkt_trailingdata(const struct kcn_pkt *);
void kcn_pkt_start(struct kcn_pkt *);
void kcn_pkt_end(struct kcn_pkt *);
void kcn_pkt_trim_head(struct kcn_pkt *, size_t);
void *kcn_pkt_head(const struct kcn_pkt *);
void *kcn_pkt_current(const struct kcn_pkt *);
void *kcn_pkt_tail(const struct kcn_pkt *);
uint8_t kcn_pkt_get8(struct kcn_pkt *);
uint16_t kcn_pkt_get16(struct kcn_pkt *);
uint32_t kcn_pkt_get32(struct kcn_pkt *);
uint64_t kcn_pkt_get64(struct kcn_pkt *);
void kcn_pkt_put8(struct kcn_pkt *, uint8_t);
void kcn_pkt_put16(struct kcn_pkt *, uint16_t);
void kcn_pkt_put32(struct kcn_pkt *, uint32_t);
void kcn_pkt_put64(struct kcn_pkt *, uint64_t);
void kcn_pkt_put(struct kcn_pkt *, void *, size_t);
bool kcn_pkt_enqueue(struct kcn_pkt *, struct kcn_pkt_queue *);
bool kcn_pkt_fetch(struct kcn_pkt *, struct kcn_pkt_queue *);
void kcn_pkt_drop(struct kcn_pkt *, struct kcn_pkt_queue *);
void kcn_pkt_purge(struct kcn_pkt_queue *);
