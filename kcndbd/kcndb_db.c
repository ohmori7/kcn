#include <sys/param.h>	/* MAXPATHLEN */

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "kcn.h"
#include "kcn_str.h"
#include "kcn_eq.h"
#include "kcn_log.h"
#include "kcn_buf.h"

#include "kcndb_file.h"
#include "kcndb_db.h"

struct kcndb_db_table {
	struct kcndb_file *kdt_loc;
	struct kcndb_file *kdt_table;
};

static const char *kcndb_db_path = KCNDB_DB_PATH_DEFAULT;
static struct kcndb_db_table *kcndb_db[KCN_EQ_TYPE_MAX - 1];

static void kcndb_db_table_close(struct kcndb_db_table *);
static bool kcndb_db_loc_init(struct kcndb_db_table *);

static struct kcndb_db_table *
kcndb_db_table_new(void)
{
	struct kcndb_db_table *kdt;

	kdt = malloc(sizeof(*kdt));
	if (kdt == NULL)
		return NULL;
	return kdt;
}

static void
kcndb_db_table_destroy(struct kcndb_db_table *kdt)
{

	if (kdt == NULL)
		return;
	free(kdt);
}

static int
kcndb_db_table_type2index(enum kcn_eq_type type)
{

	return type - 1;
}

void
kcndb_db_path_set(const char *path)
{

	kcndb_db_path = path;
}

const char *
kcndb_db_path_get(void)
{

	return kcndb_db_path;
}

static struct kcndb_db_table *
kcndb_db_table_open(enum kcn_eq_type type)
{
	struct kcndb_db_table *kdt;
	const char *name;
	char path[MAXPATHLEN];

	name = kcn_eq_type_ntoa(type);
	if (name == NULL) {
		errno = ENOENT;
		KCN_LOG(DEBUG, "unknown database type: %hu", type);
		return NULL;
	}

	kdt = kcndb_db_table_new();
	if (kdt == NULL) {
		KCN_LOG(DEBUG, "cannot allocate table structure");
		return NULL;
	}

	(void)snprintf(path, sizeof(path), "%s/%s%s",
	    kcndb_db_path, name, KCNDB_DB_PATH_LOC_SUFFIX);
	kdt->kdt_loc = kcndb_file_open(path);

	(void)snprintf(path, sizeof(path), "%s/%s", kcndb_db_path, name);
	kdt->kdt_table = kcndb_file_open(path);

	if (kdt->kdt_loc == NULL || kdt->kdt_table == NULL)
		goto bad;
	if (! kcndb_db_loc_init(kdt))
		goto bad;
	return kdt;
  bad:
	kcndb_db_table_close(kdt);
	return NULL;
}

static void
kcndb_db_table_close(struct kcndb_db_table *kdt)
{

	if (kdt == NULL)
		return;
	kcndb_file_close(kdt->kdt_loc);
	kcndb_file_close(kdt->kdt_table);
	kcndb_db_table_destroy(kdt);
}

static struct kcndb_db_table *
kcndb_db_table_lookup(enum kcn_eq_type type)
{
	int idx;

	idx = kcndb_db_table_type2index(type);
	assert(idx >= 0);
	return kcndb_db[idx];
}

static bool
kcndb_db_loc_init(struct kcndb_db_table *kdt)
{
	struct kcndb_file *kf;
	struct kcn_buf *kb;

	kf = kdt->kdt_loc;
	if (kcndb_file_size(kf) > 0)
		return true;

#define KCNDB_DB_LOC_HASHSIZ	256
#define KCNDB_DB_LOC_INDEXSIZ	sizeof(uint64_t)
#define KCNDB_DB_LOC_INDEXTABLESIZ					\
	(KCNDB_DB_LOC_INDEXSIZ * KCNDB_DB_LOC_HASHSIZ)
	kb = kcndb_file_buf(kf);
	kcn_buf_putnull(kb, KCNDB_DB_LOC_INDEXTABLESIZ);
	return kcndb_file_append(kf);
}

static bool
kcndb_db_loc_add(struct kcndb_db_table *kdt, const char *loc, size_t loclen,
    uint64_t *idxp)
{
	struct kcndb_file *kf;
	unsigned int h;
	struct kcn_buf *kb;
	uint64_t idx, oidx;
	size_t len;

	kf = kdt->kdt_loc;
	h = kcn_str_hash(loc, loclen, KCNDB_DB_LOC_HASHSIZ);
	kb = kcndb_file_buf(kf);
	kcn_buf_reset(kb, 0);
	if (! kcndb_file_seek_head(kf, 0))
		return false;
	if (! kcndb_file_ensure(kf, KCNDB_DB_LOC_INDEXSIZ))
		return false;
	kcn_buf_forward(kb, KCNDB_DB_LOC_INDEXSIZ * h);
	while ((idx = kcn_buf_get64(kb)) != 0) {
		if (! kcndb_file_seek_head(kf, idx))
			return false;
		kcn_buf_reset(kb, 0);
		if (! kcndb_file_ensure(kf, sizeof(uint16_t)))
			return false;
		len = kcn_buf_get16(kb);
		if (! kcndb_file_ensure(kf, len + sizeof(uint64_t)))
			return false;
		if (loclen == len &&
		    strncmp(kcn_buf_current(kb), loc, loclen) == 0)
			goto out;
		kcn_buf_forward(kb, len);
	}

	oidx = idx + kcn_buf_headingdata(kb) - sizeof(idx);
	idx = kcndb_file_size(kf);
	kcn_buf_reset(kb, 0);
	kcn_buf_put16(kb, loclen);
	kcn_buf_put(kb, loc, loclen);
	kcn_buf_put64(kb, 0);
	if (! kcndb_file_append(kf))
		return false;

	kcn_buf_put64(kb, idx);
	if (! kcndb_file_seek_head(kf, oidx) || ! kcndb_file_write(kf))
		return false;
  out:
	*idxp = idx;

	return true;
}

static bool
kcndb_db_loc_lookup(struct kcndb_db_table *kdt, uint64_t idx,
    const char **locp, size_t *loclenp)
{
	struct kcndb_file *kf = kdt->kdt_loc;
	struct kcn_buf *kb = kcndb_file_buf(kf);

	if (kcndb_file_size(kf) < idx) {
		errno = EINVAL;
		return false;
	}
	kcn_buf_reset(kb, 0);
	if (! kcndb_file_seek_head(kf, idx))
		return false;
	if (! kcndb_file_ensure(kf, sizeof(uint16_t)))
		return false;
	*loclenp = kcn_buf_get16(kb);
	if (! kcndb_file_ensure(kf, *loclenp))
		return false;
	*locp = kcn_buf_current(kb);
	return true;
}

static bool
kcndb_db_record_read(struct kcndb_db_table *kdt, struct kcndb_db_record *kdr)
{
	struct kcn_buf *kb = kcndb_file_buf(kdt->kdt_table);

#define KCNDB_HDRSIZ	(8 + 8 + 8)
	if (! kcndb_file_ensure(kdt->kdt_table, KCNDB_HDRSIZ))
		return false;

	kdr->kdr_time = kcn_buf_get64(kb);
	kdr->kdr_val = kcn_buf_get64(kb);
	kdr->kdr_locidx = kcn_buf_get64(kb);
	kcn_buf_trim_head(kb, kcn_buf_headingdata(kb));
	return true;
}

bool
kcndb_db_record_add(enum kcn_eq_type type, struct kcndb_db_record *kdr)
{
	struct kcndb_db_table *kdt;
	struct kcn_buf *kb;

	kdt = kcndb_db_table_lookup(type);
	kb = kcndb_file_buf(kdt->kdt_table);
	if (! kcndb_db_loc_add(kdt, kdr->kdr_loc, kdr->kdr_loclen,
	    &kdr->kdr_locidx))
		return false;
	kcn_buf_put64(kb, kdr->kdr_time);
	kcn_buf_put64(kb, kdr->kdr_val);
	kcn_buf_put64(kb, kdr->kdr_locidx);
	return kcndb_file_append(kdt->kdt_table);
}

bool
kcndb_db_search(const struct kcn_eq *ke, size_t maxnlocs,
    bool (*cb)(const struct kcndb_db_record *, size_t, void *), void *arg)
{
	struct kcndb_db_table *kdt;
	struct kcn_buf *kb;
	struct kcndb_db_record kdr;
	size_t i, n, score;

	kdt = kcndb_db_table_lookup(ke->ke_type);
	if (! kcndb_file_seek_head(kdt->kdt_table, 0)) /* XXX: should improve */
		goto bad;
	kb = kcndb_file_buf(kdt->kdt_table);
	kcn_buf_reset(kb, 0);

	score = 0; /* XXX: should compute score. */
	for (i = 0, n = 0; n < maxnlocs; i++) {
		if (! kcndb_db_record_read(kdt, &kdr)) {
			if (errno == ESHUTDOWN)
				break;
			/* XXX: should we accept this error??? */
			KCN_LOG(ERR, "cannot read record: %s", strerror(errno));
			goto bad;
		}

		KCN_LOG(DEBUG, "record[%zu]: %llu %llu %zu", i,
		    (unsigned long long)kdr.kdr_time,
		    (unsigned long long)kdr.kdr_val,
		    (size_t)kdr.kdr_locidx);

		if (! kcn_eq_time_match(kdr.kdr_time, ke))
			continue;

		switch (ke->ke_op) {
		case KCN_EQ_OP_LT:
			if (kdr.kdr_val >= ke->ke_val)
				continue;
			break;
		case KCN_EQ_OP_LE:
			if (kdr.kdr_val > ke->ke_val)
				continue;
			break;
		case KCN_EQ_OP_EQ:
			if (kdr.kdr_val != ke->ke_val)
				continue;
			break;
		case KCN_EQ_OP_GT:
			if (kdr.kdr_val <= ke->ke_val)
				continue;
			break;
		case KCN_EQ_OP_GE:
			if (kdr.kdr_val < ke->ke_val)
				continue;
			break;
		default:
			assert(0);
			continue;
		}
		if (! kcndb_db_loc_lookup(kdt, kdr.kdr_locidx,
		    &kdr.kdr_loc, &kdr.kdr_loclen))
			goto bad;
		KCN_LOG(DEBUG, "record[%zu]: match loc=%.*s",
		    i, (int)kdr.kdr_loclen, kdr.kdr_loc);
		if (! (*cb)(&kdr, score, arg))
			goto bad;
		++n;
	}

	KCN_LOG(INFO, "%zu record(s) read", i);

	if (n == 0) {
		KCN_LOG(DEBUG, "no matching record found");
		errno = ESRCH;
		goto bad;
	}

	return true;
  bad:
	return false;
}

bool
kcndb_db_init(void)
{
	enum kcn_eq_type type;
	int idx;

	for (type = KCN_EQ_TYPE_MIN + 1; type < KCN_EQ_TYPE_MAX; type++) {
		idx = kcndb_db_table_type2index(type);
		kcndb_db[idx] = kcndb_db_table_open(type);
		if (kcndb_db[idx] == NULL)
			return false;
	}
	/* XXX: may need to pre-load onto memory... */

	return true;
}

void
kcndb_db_finish(void)
{
	enum kcn_eq_type type;
	int idx;

	for (type = KCN_EQ_TYPE_MIN + 1; type < KCN_EQ_TYPE_MAX; type++) {
		idx = kcndb_db_table_type2index(type);
		kcndb_db_table_close(kcndb_db[idx]);
		kcndb_db[idx] = NULL;
	}
	/* XXX: may need to write back on-memory cache in the future. */
}
