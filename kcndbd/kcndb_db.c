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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "kcn.h"
#include "kcn_info.h"
#include "kcn_formula.h"
#include "kcn_log.h"
#include "kcn_pkt.h"

#include "kcndb_db.h"

enum kcndb_db_op {
	KCNDB_DB_OP_READ,
	KCNDB_DB_OP_WRITE
};

struct kcndb_db_table {
	int kdt_fd;
	struct kcn_pkt kdt_kp;
	struct kcn_pkt_data *kdt_kpd;
};

static const char *kcndb_db_path = KCNDB_DB_PATH_DEFAULT;

static void kcndb_db_table_destroy(struct kcndb_db_table *);

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
kcndb_db_table_new(void)
{
	struct kcndb_db_table *kdt;

	kdt = malloc(sizeof(*kdt));
	if (kdt == NULL)
		goto bad;
	kdt->kdt_fd = -1;
#define KCNDB_BUFSIZ	4096	/* XXX */
	kdt->kdt_kpd = kcn_pkt_data_new(KCNDB_BUFSIZ);
	if (kdt->kdt_kpd == NULL)
		goto bad;
	kcn_pkt_init(&kdt->kdt_kp, kdt->kdt_kpd);
	return kdt;
  bad:
	kcndb_db_table_destroy(kdt);
	return NULL;
}

static void
kcndb_db_table_destroy(struct kcndb_db_table *kdt)
{
	int oerrno;

	if (kdt == NULL)
		return;
	if (kdt->kdt_fd >= 0) {
		oerrno = errno;
		(void)close(kdt->kdt_fd);
		errno = oerrno;
	}
	kcn_pkt_data_destroy(kdt->kdt_kpd);
	free(kdt);
}

static struct kcndb_db_table *
kcndb_db_table_open(enum kcn_formula_type type, enum kcndb_db_op op)
{
	const char *name;
	char path[MAXPATHLEN];
	struct kcndb_db_table *kdt;
	int flags;

	name = kcn_formula_type_ntoa(type);
	if (name == NULL) {
		errno = ENOENT;
		KCN_LOG(DEBUG, "unknown database type: %hu", type);
		return NULL;
	}

	kdt = kcndb_db_table_new();
	if (kdt == NULL) {
		KCN_LOG(DEBUG, "cannot allocate database table: %s",
		    strerror(errno));
		return NULL;
	}

	(void)snprintf(path, sizeof(path), "%s/%s", kcndb_db_path, name);
	if (op == KCNDB_DB_OP_READ)
		flags = O_RDONLY;
	else {
		assert(op == KCNDB_DB_OP_WRITE);
		flags = O_WRONLY | O_CREAT | O_APPEND;
	}
	kdt->kdt_fd = open(path, flags, S_IRUSR | S_IWUSR);
	if (kdt->kdt_fd == -1) {
		KCN_LOG(DEBUG, "cannot open database file: %s",
		    strerror(errno));
		goto bad;
	}
	return kdt;
  bad:
	kcndb_db_table_destroy(kdt);
	return NULL;
}

struct kcndb_db_table *
kcndb_db_table_create(enum kcn_formula_type type)
{

	return kcndb_db_table_open(type, KCNDB_DB_OP_WRITE);
}

void
kcndb_db_table_close(struct kcndb_db_table *kdt)
{

	kcndb_db_table_destroy(kdt);
}

static bool
kcndb_db_record_ensure(struct kcndb_db_table *kdt, size_t len)
{
	struct kcn_pkt *kp = &kdt->kdt_kp;
	int error;

	while (kcn_pkt_len(kp) < len) {
		error = kcn_pkt_read(kdt->kdt_fd, kp);
		if (error != 0) {
			errno = error;
			if (error != ESHUTDOWN)
				KCN_LOG(ERR, "cannot read database: %s",
				    strerror(error));
			return false;
		}
	}
	return true;
}

static bool
kcndb_db_record_read(struct kcndb_db_table *kdt, struct kcndb_db_record *kdr)
{
	struct kcn_pkt *kp = &kdt->kdt_kp;

#define KCNDB_HDRSIZ	(8 + 4 + 8 + 2)
	if (! kcndb_db_record_ensure(kdt, KCNDB_HDRSIZ))
		return false;

	kdr->kdr_time.tv_sec = kcn_pkt_get64(kp);
	kdr->kdr_time.tv_usec = kcn_pkt_get32(kp);
	kdr->kdr_val = kcn_pkt_get64(kp);
	kdr->kdr_loclen = kcn_pkt_get16(kp);
	if (! kcndb_db_record_ensure(kdt, KCNDB_HDRSIZ + kdr->kdr_loclen))
		return false;
	kdr->kdr_loc = kcn_pkt_current(kp);
	kcn_pkt_forward(kp, kdr->kdr_loclen);
	kcn_pkt_trim_head(kp, kcn_pkt_headingdata(kp));
	return true;
}

bool
kcndb_db_record_add(struct kcndb_db_table *kdt, struct kcndb_db_record *kdr)
{
	struct kcn_pkt *kp = &kdt->kdt_kp;
	int error;

	kcn_pkt_put64(kp, kdr->kdr_time.tv_sec);
	kcn_pkt_put32(kp, kdr->kdr_time.tv_usec);
	kcn_pkt_put64(kp, kdr->kdr_val);
	kcn_pkt_put16(kp, kdr->kdr_loclen);
	kcn_pkt_put(kp, kdr->kdr_loc, kdr->kdr_loclen);
	while (kcn_pkt_len(kp) > 0)
		if ((error = kcn_pkt_write(kdt->kdt_fd, kp)) != 0) {
			KCN_LOG(ERR, "cannot write database: %s",
			    strerror(error));
			return false;
		}
	kcn_pkt_reset(kp, 0);
	return true;
}

bool
kcndb_db_search(struct kcn_info *ki, const struct kcn_formula *kf)
{
	struct kcndb_db_table *kdt;
	struct kcndb_db_record kdr;
	size_t i, score;

	kdt = kcndb_db_table_open(kf->kf_type, KCNDB_DB_OP_READ);
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

		KCN_LOG(DEBUG, "record: %llu %llu %.*s (%zu)",
		    (unsigned long long)kdr.kdr_time.tv_sec,
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
