enum kcn_eq_type {
	KCN_EQ_TYPE_NONE,
	KCN_EQ_TYPE_STORAGE,
	KCN_EQ_TYPE_CPULOAD,
	KCN_EQ_TYPE_TRAFFIC,
	KCN_EQ_TYPE_RTT,
	KCN_EQ_TYPE_HOPCOUNT,
	KCN_EQ_TYPE_ASPATHLEN,
	KCN_EQ_TYPE_WLANASSOC
#define KCN_EQ_TYPE_MIN	KCN_EQ_TYPE_NONE
#define KCN_EQ_TYPE_MAX	(KCN_EQ_TYPE_WLANASSOC + 1)
};

enum kcn_eq_operator {
	KCN_EQ_OP_NONE,
	KCN_EQ_OP_LT,
	KCN_EQ_OP_LE,
	KCN_EQ_OP_EQ,
	KCN_EQ_OP_GT,
	KCN_EQ_OP_GE
};

struct kcn_eq {
	enum kcn_eq_type ke_type;
	enum kcn_eq_operator ke_op;
	unsigned long long ke_val;
};

const char *kcn_eq_type_ntoa(enum kcn_eq_type);
bool kcn_eq_type_aton(const char *, enum kcn_eq_type *);
bool kcn_eq_operator_aton(const char *, enum kcn_eq_operator *);
bool kcn_eq_val_aton(const char *, unsigned long long *);
