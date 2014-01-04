#include <sys/param.h>	/* MAXPATHLEN */

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pthread.h>

#include "kcn.h"
#include "kcn_str.h"
#include "kcn_eq.h"
#include "kcn_log.h"
#include "kcn_buf.h"

#include "kcndb_file.h"
#include "kcndb_db.h"

#define	TYPE2INDEX(t)	((t) - 1)

struct kcndb_db_table {
	enum kcn_eq_type kdt_type;
	struct kcndb_file *kdt_loc;
	struct kcndb_file *kdt_table;
};

struct kcndb_db {
	struct kcndb_db_table *kd_tables[KCN_EQ_TYPE_MAX - 1];
};

struct kcndb_db_base {
	size_t kdb_locsize;
	size_t kdb_tablesize;
	pthread_rwlock_t kdb_lock;
};

#define KCNDB_DB_BASE_INITIALIZER	{ 0, 0, PTHREAD_RWLOCK_INITIALIZER }

static const char *kcndb_db_path = KCN_DB_PATH;

static struct kcndb_db_base kcndb_db_base[KCN_EQ_TYPE_MAX - 1] = {
	[TYPE2INDEX(KCN_EQ_TYPE_STORAGE)]	= KCNDB_DB_BASE_INITIALIZER,
	[TYPE2INDEX(KCN_EQ_TYPE_CPULOAD)]	= KCNDB_DB_BASE_INITIALIZER,
	[TYPE2INDEX(KCN_EQ_TYPE_TRAFFIC)]	= KCNDB_DB_BASE_INITIALIZER,
	[TYPE2INDEX(KCN_EQ_TYPE_RTT)]		= KCNDB_DB_BASE_INITIALIZER,
	[TYPE2INDEX(KCN_EQ_TYPE_HOPCOUNT)]	= KCNDB_DB_BASE_INITIALIZER,
	[TYPE2INDEX(KCN_EQ_TYPE_ASPATHLEN)]	= KCNDB_DB_BASE_INITIALIZER,
	[TYPE2INDEX(KCN_EQ_TYPE_WLANASSOC)]	= KCNDB_DB_BASE_INITIALIZER
};

static void kcndb_db_table_close(struct kcndb_db_table *);
static bool kcndb_db_loc_init(struct kcndb_db_table *);

static struct kcndb_db_table *
kcndb_db_table_new(enum kcn_eq_type type)
{
	struct kcndb_db_table *kdt;

	kdt = malloc(sizeof(*kdt));
	if (kdt == NULL)
		return NULL;
	kdt->kdt_type = type;
	kdt->kdt_loc = NULL;
	kdt->kdt_table = NULL;
	return kdt;
}

static void
kcndb_db_table_destroy(struct kcndb_db_table *kdt)
{

	if (kdt == NULL)
		return;
	free(kdt);
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

static bool
kcndb_db_init(struct kcndb_db_table *kdt)
{
	struct kcndb_db_base *kdb;

	kdb = &kcndb_db_base[TYPE2INDEX(kdt->kdt_type)];
	if (kdb->kdb_locsize == 0 &&
	    ! kcndb_file_size_get(kdt->kdt_loc, &kdb->kdb_locsize))
		return false;
	if (kdb->kdb_tablesize == 0 &&
	    ! kcndb_file_size_get(kdt->kdt_table, &kdb->kdb_tablesize))
		return false;
	if (! kcndb_db_loc_init(kdt))
		return false;
	return true;
}

static size_t
kcndb_db_loc_size(const struct kcndb_db_table *kdt)
{

	return kcndb_db_base[TYPE2INDEX(kdt->kdt_type)].kdb_locsize;
}

static void
kcndb_db_loc_size_increment(const struct kcndb_db_table *kdt, size_t size)
{

	kcndb_db_base[TYPE2INDEX(kdt->kdt_type)].kdb_locsize += size;
}

static size_t
kcndb_db_table_size(const struct kcndb_db_table *kdt)
{

	return kcndb_db_base[TYPE2INDEX(kdt->kdt_type)].kdb_tablesize;
}

static void
kcndb_db_table_size_increment(const struct kcndb_db_table *kdt, size_t size)
{

	kcndb_db_base[TYPE2INDEX(kdt->kdt_type)].kdb_tablesize += size;
}

static bool
kcndb_db_rdlock(const struct kcndb_db_table *kdt)
{
	struct kcndb_db_base *kdb = &kcndb_db_base[TYPE2INDEX(kdt->kdt_type)];
	int error;

	error = pthread_rwlock_rdlock(&kdb->kdb_lock);
	if (error != 0) {
		errno = error;
		KCN_LOG(ERR, "database read lock error: %s", strerror(errno));
	}
	return error == 0 ? true : false;
}

static bool
kcndb_db_wrlock(const struct kcndb_db_table *kdt)
{
	struct kcndb_db_base *kdb = &kcndb_db_base[TYPE2INDEX(kdt->kdt_type)];
	int error;

	error = pthread_rwlock_wrlock(&kdb->kdb_lock);
	if (error != 0) {
		errno = error;
		KCN_LOG(ERR, "database write lock error: %s", strerror(errno));
	}
	return error == 0 ? true : false;
}

static void
kcndb_db_unlock(const struct kcndb_db_table *kdt)
{
	struct kcndb_db_base *kdb = &kcndb_db_base[TYPE2INDEX(kdt->kdt_type)];
	int error;

	error = pthread_rwlock_unlock(&kdb->kdb_lock);
	if (error != 0) {
		errno = error;
		KCN_LOG(ERR, "database unlock failed: %s", strerror(errno));
	}
}

static struct kcndb_db_table *
kcndb_db_table_open(enum kcn_eq_type type)
{
	struct kcndb_db_table *kdt;
	const char *name;
	char path[MAXPATHLEN];
	bool rc;

	name = kcn_eq_type_ntoa(type);
	if (name == NULL) {
		errno = ENOENT;
		KCN_LOG(DEBUG, "unknown database type: %hu", type);
		return NULL;
	}

	kdt = kcndb_db_table_new(type);
	if (kdt == NULL) {
		KCN_LOG(DEBUG, "cannot allocate table structure");
		return NULL;
	}

	(void)snprintf(path, sizeof(path), "%s/%s%s",
	    kcndb_db_path, name, KCNDB_DB_PATH_LOC_SUFFIX);
	kdt->kdt_loc = kcndb_file_open(path);
	if (kdt->kdt_loc == NULL)
		goto bad;

	(void)snprintf(path, sizeof(path), "%s/%s", kcndb_db_path, name);
	kdt->kdt_table = kcndb_file_open(path);
	if (kdt->kdt_table == NULL)
		goto bad;

	if (! kcndb_db_wrlock(kdt))
		goto bad;
	rc = kcndb_db_init(kdt);
	kcndb_db_unlock(kdt);
	if (! rc)
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
kcndb_db_table_lookup(struct kcndb_db *kd, enum kcn_eq_type type)
{

	return kd->kd_tables[TYPE2INDEX(type)];
}

static bool
kcndb_db_loc_init(struct kcndb_db_table *kdt)
{
	struct kcndb_file *kf;
	struct kcn_buf *kb;
	bool rc;

	if (kcndb_db_loc_size(kdt) > 0)
		return true;

#define KCNDB_DB_LOC_HASHSIZ	256
#define KCNDB_DB_LOC_INDEXSIZ	sizeof(uint64_t)
#define KCNDB_DB_LOC_INDEXTABLESIZ					\
	(KCNDB_DB_LOC_INDEXSIZ * KCNDB_DB_LOC_HASHSIZ)
	kf = kdt->kdt_loc;
	kb = kcndb_file_buf(kf);
	kcn_buf_putnull(kb, KCNDB_DB_LOC_INDEXTABLESIZ);
	rc = kcndb_file_append(kf);
	if (rc)
		kcndb_db_loc_size_increment(kdt, KCNDB_DB_LOC_INDEXTABLESIZ);
	return rc;
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
	idx = kcndb_db_loc_size(kdt);
	kcn_buf_reset(kb, 0);
	kcn_buf_put16(kb, loclen);
	kcn_buf_put(kb, loc, loclen);
	kcn_buf_put64(kb, 0);
	len = kcn_buf_len(kb);
	if (! kcndb_file_append(kf))
		return false;
	kcndb_db_loc_size_increment(kdt, len);

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

	if (kcndb_db_table_size(kdt) < idx) {
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

#define KCNDB_DB_RECORDSIZ	(8 + 8 + 8)
	if (! kcndb_file_ensure(kdt->kdt_table, KCNDB_DB_RECORDSIZ))
		return false;

	kdr->kdr_time = kcn_buf_get64(kb);
	kdr->kdr_val = kcn_buf_get64(kb);
	kdr->kdr_locidx = kcn_buf_get64(kb);
	kcn_buf_trim_head(kb, kcn_buf_headingdata(kb));
	return true;
}

static bool
kcndb_db_record_read_last(struct kcndb_db_table *kdt,
    struct kcndb_db_record *kdr)
{
	struct kcndb_db_base *kdb;
	size_t off;

	kdb = &kcndb_db_base[TYPE2INDEX(kdt->kdt_type)];
	if (kdb->kdb_tablesize < KCNDB_DB_RECORDSIZ) {
		kdr->kdr_time = 0;
		kdr->kdr_val = 0;
		kdr->kdr_locidx = 0;
		return true;
	}
	off = kdb->kdb_tablesize - KCNDB_DB_RECORDSIZ;
	if (! kcndb_file_seek_head(kdt->kdt_table, off))
		return false;
	return kcndb_db_record_read(kdt, kdr);
}

bool
kcndb_db_record_add(struct kcndb_db *kd, enum kcn_eq_type type,
    struct kcndb_db_record *kdr)
{
	struct kcndb_db_record kdr0;
	struct kcndb_db_table *kdt;
	struct kcn_buf *kb;
	bool rc;

	kdt = kcndb_db_table_lookup(kd, type);
	kb = kcndb_file_buf(kdt->kdt_table);
	if (! kcndb_db_wrlock(kdt))
		return false;
	rc = kcndb_db_record_read_last(kdt, &kdr0);
	if (! rc)
		goto out;
	if (kdr0.kdr_time > kdr->kdr_time) {
		errno = ERANGE; /* XXX */
		rc = false;
		goto out;
	}
	rc = kcndb_db_loc_add(kdt, kdr->kdr_loc, kdr->kdr_loclen,
	    &kdr->kdr_locidx);
	if (! rc)
		goto out;
	kcn_buf_put64(kb, kdr->kdr_time);
	kcn_buf_put64(kb, kdr->kdr_val);
	kcn_buf_put64(kb, kdr->kdr_locidx);
	rc = kcndb_file_append(kdt->kdt_table);
	if (rc)
		kcndb_db_table_size_increment(kdt, KCNDB_DB_RECORDSIZ);
  out:
	kcndb_db_unlock(kdt);
	return rc;
}

bool
kcndb_db_search(struct kcndb_db *kd, const struct kcn_eq *ke, size_t maxnlocs,
    bool (*cb)(const struct kcndb_db_record *, size_t, void *), void *arg)
{
	struct kcndb_db_table *kdt;
	struct kcn_buf *kb;
	struct kcndb_db_record kdr;
	size_t i, n, score;

	kdt = kcndb_db_table_lookup(kd, ke->ke_type);
	if (! kcndb_db_rdlock(kdt))
		return false;
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
	kcndb_db_unlock(kdt);
	return false;
}

struct kcndb_db *
kcndb_db_new(void)
{
	struct kcndb_db *kd;
	enum kcn_eq_type type;
	int idx;

	kd = malloc(sizeof(*kd));
	if (kd == NULL)
		goto bad;
	for (type = KCN_EQ_TYPE_MIN + 1; type < KCN_EQ_TYPE_MAX; type++) {
		idx = TYPE2INDEX(type);
		kd->kd_tables[idx] = kcndb_db_table_open(type);
		if (kd->kd_tables[idx] == NULL)
			goto bad;
	}
	/* XXX: may need to pre-load onto memory... */

	return kd;
  bad:
	kcndb_db_destroy(kd);
	return NULL;
}

void
kcndb_db_destroy(struct kcndb_db *kd)
{
	enum kcn_eq_type type;
	int idx;

	if (kd == NULL)
		return;
	for (type = KCN_EQ_TYPE_MIN + 1; type < KCN_EQ_TYPE_MAX; type++) {
		idx = TYPE2INDEX(type);
		if (kd->kd_tables[idx] == NULL)
			break;
		kcndb_db_table_close(kd->kd_tables[idx]);
		kd->kd_tables[idx] = NULL;
	}
	/* XXX: may need to write back on-memory cache in the future. */
}
