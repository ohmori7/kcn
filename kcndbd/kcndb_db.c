#include <sys/param.h>	/* MAXPATHLEN */

#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "kcn.h"
#include "kcn_str.h"
#include "kcn_info.h"
#include "kcn_eq.h"
#include "kcn_log.h"
#include "kcn_pkt.h"

#include "kcndb_file.h"
#include "kcndb_db.h"

struct kcndb_db_table {
	struct kcndb_file *kdt_loc;
	struct kcndb_file *kdt_table;
};

static const char *kcndb_db_path = KCNDB_DB_PATH_DEFAULT;

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

bool
kcndb_db_init(void)
{
	DIR *ndir;
	int oerrno;

	ndir = opendir(kcndb_db_path);
	if (ndir == NULL)
		return false;

	/* XXX: may need to pre-load onto memory... */

	oerrno = errno;
	(void)closedir(ndir);
	errno = oerrno;

	return true;
}

void
kcndb_db_finish(void)
{

	/* XXX: may need to write back on-memory cache in the future. */
}

static struct kcndb_db_table *
kcndb_db_table_open(enum kcn_eq_type type, int flags)
{
	struct kcndb_db_table *kdt;
	const char *name;
	char path[MAXPATHLEN];
	int tflags;

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
	kdt->kdt_loc = kcndb_file_open(path, flags);

	tflags = flags;
	if (flags & O_RDWR)
		tflags |= O_APPEND;
	(void)snprintf(path, sizeof(path), "%s/%s", kcndb_db_path, name);
	kdt->kdt_table = kcndb_file_open(path, tflags);

	if (kdt->kdt_loc == NULL || kdt->kdt_table == NULL)
		goto bad;
	if (! kcndb_db_loc_init(kdt))
		goto bad;
	return kdt;
  bad:
	kcndb_db_table_close(kdt);
	return NULL;
}

struct kcndb_db_table *
kcndb_db_table_create(enum kcn_eq_type type)
{

	return kcndb_db_table_open(type, O_RDWR | O_CREAT);
}

void
kcndb_db_table_close(struct kcndb_db_table *kdt)
{

	if (kdt == NULL)
		return;
	kcndb_file_close(kdt->kdt_loc);
	kcndb_file_close(kdt->kdt_table);
	kcndb_db_table_destroy(kdt);
}

static bool
kcndb_db_loc_init(struct kcndb_db_table *kdt)
{
	struct kcndb_file *kf;
	struct kcn_pkt *kp;

	kf = kdt->kdt_loc;
	if (kcndb_file_size(kf) > 0)
		return true;

#define KCNDB_DB_LOC_HASHSIZ	256
#define KCNDB_DB_LOC_INDEXSIZ	sizeof(uint64_t)
#define KCNDB_DB_LOC_INDEXTABLESIZ					\
	(KCNDB_DB_LOC_INDEXSIZ * KCNDB_DB_LOC_HASHSIZ)
	kp = kcndb_file_packet(kf);
	kcn_pkt_putnull(kp, KCNDB_DB_LOC_INDEXTABLESIZ);
	return kcndb_file_append(kf);
}

static bool
kcndb_db_loc_add(struct kcndb_db_table *kdt, const char *loc, size_t loclen,
    uint64_t *idxp)
{
	struct kcndb_file *kf;
	unsigned int h;
	struct kcn_pkt *kp;
	uint64_t idx, oidx;
	size_t len;

	kf = kdt->kdt_loc;
	h = kcn_str_hash(loc, loclen, KCNDB_DB_LOC_HASHSIZ);
	kp = kcndb_file_packet(kf);
	if (! kcndb_file_seek_head(kf, 0))
		return false;
	if (! kcndb_file_ensure(kf, KCNDB_DB_LOC_INDEXSIZ))
		return false;
	kcn_pkt_forward(kp, KCNDB_DB_LOC_INDEXSIZ * h);
	while ((idx = kcn_pkt_get64(kp)) != 0) {
		if (! kcndb_file_seek_head(kf, idx))
			return false;
		kcn_pkt_reset(kp, 0);
		if (! kcndb_file_ensure(kf, sizeof(uint16_t)))
			return false;
		len = kcn_pkt_get16(kp);
		if (! kcndb_file_ensure(kf, len + sizeof(uint64_t)))
			return false;
		if (loclen == len &&
		    strncmp(kcn_pkt_current(kp), loc, loclen) == 0)
			goto out;
		kcn_pkt_forward(kp, len);
	}

	oidx = idx + kcn_pkt_headingdata(kp) - sizeof(idx);
	idx = kcndb_file_size(kf);
	kcn_pkt_reset(kp, 0);
	kcn_pkt_put16(kp, loclen);
	kcn_pkt_put(kp, loc, loclen);
	kcn_pkt_put64(kp, 0);
	if (! kcndb_file_append(kf))
		return false;

	kcn_pkt_put64(kp, idx);
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
	struct kcn_pkt *kp = kcndb_file_packet(kf);

	if ((uint64_t /*XXX*/)kcndb_file_size(kf) < idx) {
		errno = EINVAL;
		return false;
	}
	kcn_pkt_reset(kp, 0);
	if (! kcndb_file_seek_head(kf, idx))
		return false;
	if (! kcndb_file_ensure(kf, sizeof(uint16_t)))
		return false;
	*loclenp = kcn_pkt_get16(kp);
	if (! kcndb_file_ensure(kf, *loclenp))
		return false;
	*locp = kcn_pkt_current(kp);
	return true;
}

static bool
kcndb_db_record_read(struct kcndb_db_table *kdt, struct kcndb_db_record *kdr)
{
	struct kcn_pkt *kp = kcndb_file_packet(kdt->kdt_table);

#define KCNDB_HDRSIZ	(8 + 8 + 8)
	if (! kcndb_file_ensure(kdt->kdt_table, KCNDB_HDRSIZ))
		return false;

	kdr->kdr_time = kcn_pkt_get64(kp);
	kdr->kdr_val = kcn_pkt_get64(kp);
	kdr->kdr_locidx = kcn_pkt_get64(kp);
	kcn_pkt_trim_head(kp, kcn_pkt_headingdata(kp));
	return true;
}

bool
kcndb_db_record_add(struct kcndb_db_table *kdt, struct kcndb_db_record *kdr)
{
	struct kcn_pkt *kp = kcndb_file_packet(kdt->kdt_table);

	if (! kcndb_db_loc_add(kdt, kdr->kdr_loc, kdr->kdr_loclen,
	    &kdr->kdr_locidx))
		return false;
	kcn_pkt_put64(kp, kdr->kdr_time);
	kcn_pkt_put64(kp, kdr->kdr_val);
	kcn_pkt_put64(kp, kdr->kdr_locidx);
	return kcndb_file_write(kdt->kdt_table);
}

bool
kcndb_db_search(struct kcn_info *ki, const struct kcn_eq *ke)
{
	struct kcndb_db_table *kdt;
	struct kcndb_db_record kdr;
	size_t i, score;

	kdt = kcndb_db_table_open(ke->ke_type, O_RDONLY);
	if (kdt == NULL)
		goto bad;

	score = 0; /* XXX: should compute score. */
	for (i = 0; kcn_info_nlocs(ki) < kcn_info_maxnlocs(ki); i++) {
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
		if (! kcn_info_loc_add(ki, kdr.kdr_loc, kdr.kdr_loclen, score))
			goto bad;
	}

	KCN_LOG(INFO, "%zu record(s) read", i);

	if (kcn_info_nlocs(ki) == 0) {
		KCN_LOG(DEBUG, "no matching record found");
		errno = ESRCH;
		goto bad;
	}

	kcndb_db_table_close(kdt);
	return true;
  bad:
	kcndb_db_table_close(kdt);
	return false;
}
