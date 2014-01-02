#include <sys/param.h>	/* MAXPATHLEN */
#include <sys/time.h>

#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "kcn.h"
#include "kcn_info.h"
#include "kcn_formula.h"
#include "kcn_log.h"
#include "kcn_pkt.h"

#include "kcndb_file.h"
#include "kcndb_db.h"

static const char *kcndb_db_path = KCNDB_DB_PATH_DEFAULT;

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

static struct kcndb_file *
kcndb_db_table_open(enum kcn_formula_type type, enum kcndb_file_op op)
{
	const char *name;
	char path[MAXPATHLEN];

	name = kcn_formula_type_ntoa(type);
	if (name == NULL) {
		errno = ENOENT;
		KCN_LOG(DEBUG, "unknown database type: %hu", type);
		return NULL;
	}

	(void)snprintf(path, sizeof(path), "%s/%s", kcndb_db_path, name);
	return kcndb_file_open(path, op);
}

struct kcndb_file *
kcndb_db_table_create(enum kcn_formula_type type)
{

	return kcndb_db_table_open(type, KCNDB_FILE_OP_WRITE);
}

void
kcndb_db_table_close(struct kcndb_file *kf)
{

	kcndb_file_close(kf);
}

static bool
kcndb_db_record_read(struct kcndb_file *kf, struct kcndb_db_record *kdr)
{
	struct kcn_pkt *kp = kcndb_file_packet(kf);

#define KCNDB_HDRSIZ	(8 + 4 + 8 + 2)
	if (! kcndb_file_ensure(kf, KCNDB_HDRSIZ))
		return false;

	kdr->kdr_time = kcn_pkt_get64(kp);
	kdr->kdr_val = kcn_pkt_get64(kp);
	kdr->kdr_loclen = kcn_pkt_get16(kp);
	if (! kcndb_file_ensure(kf, KCNDB_HDRSIZ + kdr->kdr_loclen))
		return false;
	kdr->kdr_loc = kcn_pkt_current(kp);
	kcn_pkt_forward(kp, kdr->kdr_loclen);
	kcn_pkt_trim_head(kp, kcn_pkt_headingdata(kp));
	return true;
}

bool
kcndb_db_record_add(struct kcndb_file *kf, struct kcndb_db_record *kdr)
{
	struct kcn_pkt *kp = kcndb_file_packet(kf);

	kcn_pkt_put64(kp, kdr->kdr_time);
	kcn_pkt_put64(kp, kdr->kdr_val);
	kcn_pkt_put16(kp, kdr->kdr_loclen);
	kcn_pkt_put(kp, kdr->kdr_loc, kdr->kdr_loclen);
	return kcndb_file_write(kf);
}

bool
kcndb_db_search(struct kcn_info *ki, const struct kcn_formula *kf)
{
	struct kcndb_file *kfile;
	struct kcndb_db_record kdr;
	size_t i, score;

	kfile = kcndb_db_table_open(kf->kf_type, KCNDB_FILE_OP_READ);
	if (kfile == NULL)
		goto bad;

	score = 0; /* XXX: should compute score. */
	for (i = 0; kcn_info_nlocs(ki) < kcn_info_maxnlocs(ki); i++) {
		if (! kcndb_db_record_read(kfile, &kdr)) {
			if (errno == ESHUTDOWN)
				break;
			/* XXX: should we accept this error??? */
			KCN_LOG(ERR, "cannot read record: %s", strerror(errno));
			goto bad;
		}

		KCN_LOG(DEBUG, "record[%zu]: %llu %llu %.*s (%zu)", i,
		    (unsigned long long)kdr.kdr_time,
		    (unsigned long long)kdr.kdr_val,
		    (int)kdr.kdr_loclen, kdr.kdr_loc, kdr.kdr_loclen);

		switch (kf->kf_op) {
		case KCN_FORMULA_OP_LT:
			if (kdr.kdr_val >= kf->kf_val)
				continue;
			break;
		case KCN_FORMULA_OP_LE:
			if (kdr.kdr_val > kf->kf_val)
				continue;
			break;
		case KCN_FORMULA_OP_EQ:
			if (kdr.kdr_val != kf->kf_val)
				continue;
			break;
		case KCN_FORMULA_OP_GT:
			if (kdr.kdr_val <= kf->kf_val)
				continue;
			break;
		case KCN_FORMULA_OP_GE:
			if (kdr.kdr_val < kf->kf_val)
				continue;
			break;
		default:
			assert(0);
			continue;
		}
		KCN_LOG(DEBUG, "record: match");
		if (! kcn_info_loc_add(ki, kdr.kdr_loc, kdr.kdr_loclen, score))
			goto bad;
	}

	KCN_LOG(INFO, "%zu record(s) read", i);

	if (kcn_info_nlocs(ki) == 0) {
		KCN_LOG(DEBUG, "no matching record found");
		errno = ESRCH;
		goto bad;
	}

	kcndb_db_table_close(kfile);
	return true;
  bad:
	kcndb_db_table_close(kfile);
	return false;
}
