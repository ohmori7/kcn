enum kcn_formula_type {
	KCN_FORMULA_TYPE_NONE,
	KCN_FORMULA_TYPE_STORAGE,
	KCN_FORMULA_TYPE_CPULOAD,
	KCN_FORMULA_TYPE_TRAFFIC,
	KCN_FORMULA_TYPE_LATENCY,
	KCN_FORMULA_TYPE_HOPCOUNT,
	KCN_FORMULA_TYPE_ASPATHLEN,
	KCN_FORMULA_TYPE_WLANASSOC
#define KCN_FORMULA_TYPE_MAX	(KCN_FORMULA_TYPE_WLANASSOC + 1)
};

enum kcn_formula_operator {
	KCN_FORMULA_OP_NONE,
	KCN_FORMULA_OP_LT,
	KCN_FORMULA_OP_LE,
	KCN_FORMULA_OP_EQ,
	KCN_FORMULA_OP_GT,
	KCN_FORMULA_OP_GE
};

struct kcn_formula {
	enum kcn_formula_type kf_type;
	enum kcn_formula_operator kf_op;
	unsigned long long kf_val;
};

const char *kcn_formula_type_ntoa(enum kcn_formula_type);
bool kcn_formula_type_aton(const char *, enum kcn_formula_type *);
bool kcn_formula_operator_aton(const char *, enum kcn_formula_operator *);
bool kcn_formula_val_aton(const char *, unsigned long long *);
