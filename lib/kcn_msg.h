#define KCN_MSG_VERSION		1U

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

#define KCN_MSG_MAXSIZ	4096

bool kcn_msg_header_decode(struct kcn_pkt *, struct kcn_msg_header *);
void kcn_msg_query_encode(struct kcn_pkt *, const struct kcn_msg_query *);
bool kcn_msg_query_decode(struct kcn_pkt *, struct kcn_msg_query *);
void kcn_msg_response_init(struct kcn_msg_response *, struct kcn_info *);
void kcn_msg_response_encode(struct kcn_pkt *, const struct kcn_msg_response *);
bool kcn_msg_response_decode(struct kcn_pkt *, struct kcn_msg_response *);
