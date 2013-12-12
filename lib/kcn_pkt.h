struct kcn_pkt;

STAILQ_HEAD(kcn_pkt_head, kcn_pkt);

struct kcn_pkt_handle {
	struct kcn_pkt_head *kph_head;
	struct kcn_pkt *kph_kp;
	size_t kph_cp;
};

bool kcn_pkt_new(struct kcn_pkt_handle *, size_t);
void kcn_pkt_destroy(struct kcn_pkt_handle *);
size_t kcn_pkt_len(const struct kcn_pkt_handle *);
void kcn_pkt_reset(struct kcn_pkt_handle *);
size_t kcn_pkt_trailingspace(const struct kcn_pkt_handle *);
size_t kcn_pkt_trailingdata(const struct kcn_pkt_handle *);
void kcn_pkt_trim_head(struct kcn_pkt_handle *, size_t);
void *kcn_pkt_sp(const struct kcn_pkt_handle *);
void *kcn_pkt_cp(const struct kcn_pkt_handle *);
uint8_t kcn_pkt_get8(struct kcn_pkt_handle *);
uint16_t kcn_pkt_get16(struct kcn_pkt_handle *);
uint32_t kcn_pkt_get32(struct kcn_pkt_handle *);
uint64_t kcn_pkt_get64(struct kcn_pkt_handle *);
void kcn_pkt_put8(struct kcn_pkt_handle *, uint8_t);
void kcn_pkt_put16(struct kcn_pkt_handle *, uint16_t);
void kcn_pkt_put32(struct kcn_pkt_handle *, uint32_t);
void kcn_pkt_put64(struct kcn_pkt_handle *, uint64_t);
void kcn_pkt_put(struct kcn_pkt_handle *, void *, size_t);
bool kcn_pkt_enqueue(struct kcn_pkt_handle *);
bool kcn_pkt_fetch(struct kcn_pkt_handle *);
void kcn_pkt_dequeue(struct kcn_pkt_handle *);
