#include <sys/param.h>	/* MAXPATHLEN */
#include <sys/time.h>

#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "kcn.h"
#include "kcn_info.h"
#include "kcn_formula.h"
#include "kcn_log.h"
#include "kcn_pkt.h"

#include "kcndb_db.h"

struct kcndb_db_record {
	struct timeval kdr_time;
	unsigned long long kdr_val;
	const char *kdr_uri;
	size_t kdr_urilen;
};

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

static int
kcndb_db_table_open(enum kcn_formula_type type)
{
	const char *name;
	char path[MAXPATHLEN];
	int fd;

	name = kcn_formula_type_ntoa(type);
	if (name == NULL) {
		errno = ENOENT;
		KCN_LOG(DEBUG, "unknown database type: %hu", type);
		return -1;
	}
	(void)snprintf(path, sizeof(path), "%s/%s", kcndb_db_path, name);
	fd = open(path, O_RDONLY);
	if (fd == -1)
		KCN_LOG(DEBUG, "cannot open database file: %s",
		    strerror(errno));
	return fd;
}

static void
kcndb_db_table_close(int fd)
{
	int oerrno;

	if (fd >= 0)
		return;
	oerrno = errno;
	(void)close(fd);
	errno = oerrno;
}

static bool
kcndb_db_record_read(int fd, struct kcn_pkt *kp, struct kcndb_db_record *kdr)
{

	kcn_pkt_realign(kp);

	if (kcn_pkt_read(fd, kp) != 0)
		return false;

#define KCNDB_HDRSIZ	(8 + 4 + 8 + 2)
	if (kcn_pkt_len(kp) < KCNDB_HDRSIZ)
		return false;
	kdr->kdr_time.tv_sec = kcn_pkt_get64(kp);
	kdr->kdr_time.tv_usec = kcn_pkt_get32(kp);
	kdr->kdr_val = kcn_pkt_get64(kp);
	kdr->kdr_urilen = kcn_pkt_get16(kp);
	if (kcn_pkt_trailingdata(kp) < kdr->kdr_urilen)
		return false;

	kdr->kdr_uri = kcn_pkt_current(kp);
	kcn_pkt_trim_head(kp, kcn_pkt_headingdata(kp));
	return true;
}

bool
kcndb_db_search(struct kcn_info *ki, const struct kcn_formula *kf)
{
	struct kcn_pkt_data *kpd;
	struct kcn_pkt kp;
	struct kcndb_db_record kdr;
	size_t i, score;
	int fd;

	fd = kcndb_db_table_open(kf->kf_type);
	if (fd == -1) {
		KCN_LOG(ERR, "cannot open table: %s", strerror(errno));
		goto bad1;
	}

#define KCNDB_BUFSIZ	4096	/* XXX */
	kpd = kcn_pkt_data_new(KCNDB_BUFSIZ);
	if (kpd == NULL) {
		KCN_LOG(ERR, "cannot prepare for record buffer: %s",
		    strerror(errno));
		goto bad1;
	}
	kcn_pkt_init(&kp, kpd);

	score = 0; /* XXX: should compute score. */
	for (i = 0; kcn_info_nlocs(ki) < kcn_info_maxnlocs(ki); i++) {
		if (! kcndb_db_record_read(fd, &kp, &kdr)) {
			KCN_LOG(ERR, "cannot read record: %s", strerror(errno));
			goto bad2;
		}

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
		if (! kcn_info_loc_add(ki, kdr.kdr_uri, kdr.kdr_urilen, score))
			goto bad2;
	}

	kcn_pkt_data_destroy(kpd);
	kcndb_db_table_close(fd);
	return true;
  bad2:
	kcn_pkt_data_destroy(kpd);
  bad1:
	kcndb_db_table_close(fd);
	return false;
}
