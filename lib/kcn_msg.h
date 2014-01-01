#define KCN_MSG_VERSION		1U
#define KCN_MSG_HDRSIZ		(1 /* version */ + 1 /* type */ + 2 /* length*/)
#define KCN_MSG_MAXSIZ		4096
#define KCN_MSG_MAXBODYSIZ	(KCN_MSG_MAXSIZ - KCN_MSG_HDRSIZ)
#define KCN_MSG_QUERY_SIZ	(1 + 1 + 8 + 1 + 1 + 8)
#define KCN_MSG_RESPONSE_MINSIZ	(1 + 1 + 1)
#define KCN_MSG_ADD_MINSIZ	(1 + 8 + 8)
#define KCN_MSG_MAXLOCSIZ						\
	(KCN_MSG_MAXBODYSIZ - max(KCN_MSG_RESPONSE_MINSIZ, KCN_MSG_ADD_MINSIZ))

enum kcn_msg_type {
	KCN_MSG_TYPE_RESERVED,
	KCN_MSG_TYPE_QUERY,
	KCN_MSG_TYPE_RESPONSE,
	KCN_MSG_TYPE_ADD,
	KCN_MSG_TYPE_DEL
};

struct kcn_msg_header {
	uint8_t kmh_version;
	enum kcn_msg_type kmh_type;
	size_t kmh_len;
};

struct kcn_msg_query {
	enum kcn_loc_type kmq_loctype;
	uint8_t kmq_maxcount;
	uint64_t kmq_time;
	struct kcn_formula kmq_formula;
};

struct kcn_msg_response {
	uint8_t kmr_error;
	uint8_t kmr_leftcount;
	uint8_t kmr_score;
	struct kcn_info *kmr_ki;
};

struct kcn_msg_add {
	enum kcn_formula_type kma_type;
	uint64_t kma_time;
	uint64_t kma_val;
	const char *kma_loc;
	size_t kma_loclen;
};

bool kcn_msg_header_decode(struct kcn_pkt *, struct kcn_msg_header *);
void kcn_msg_query_encode(struct kcn_pkt *, const struct kcn_msg_query *);
bool kcn_msg_query_decode(struct kcn_pkt *, const struct kcn_msg_header *,
    struct kcn_msg_query *);
void kcn_msg_response_init(struct kcn_msg_response *, struct kcn_info *);
void kcn_msg_response_encode(struct kcn_pkt *, const struct kcn_msg_response *);
bool kcn_msg_response_decode(struct kcn_pkt *, const struct kcn_msg_header *,
    struct kcn_msg_response *);
void kcn_msg_add_encode(struct kcn_pkt *, const struct kcn_msg_add *);
bool kcn_msg_add_decode(struct kcn_pkt *, const struct kcn_msg_header *,
    struct kcn_msg_add *);
