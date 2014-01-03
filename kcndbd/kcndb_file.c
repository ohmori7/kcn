#include <sys/stat.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "kcn.h"
#include "kcn_log.h"
#include "kcn_pkt.h"
#include "kcndb_file.h"

struct kcndb_file {
	int kf_fd;
	size_t kf_size;
	struct kcn_pkt kf_kp;
	struct kcn_pkt_data *kf_kpd;
};

static void kcndb_file_destroy(struct kcndb_file *);

static struct kcndb_file *
kcndb_file_new(void)
{
	struct kcndb_file *kf;

	kf = malloc(sizeof(*kf));
	if (kf == NULL)
		goto bad;
	kf->kf_fd = -1;
	kf->kf_size = 0;
	kf->kf_kpd = kcn_pkt_data_new(KCNDB_FILE_BUFSIZ);
	if (kf->kf_kpd == NULL)
		goto bad;
	kcn_pkt_init(&kf->kf_kp, kf->kf_kpd);
	return kf;
  bad:
	kcndb_file_destroy(kf);
	return NULL;
}

static void
kcndb_file_destroy(struct kcndb_file *kf)
{
	int oerrno;

	if (kf == NULL)
		return;
	if (kf->kf_fd >= 0) {
		oerrno = errno;
		(void)close(kf->kf_fd);
		errno = oerrno;
	}
	kcn_pkt_data_destroy(kf->kf_kpd);
	free(kf);
}

static bool
kcndb_file_size_get(struct kcndb_file *kf)
{
	struct stat st;

	if (fstat(kf->kf_fd, &st) == -1)
		return false;
	kf->kf_size = st.st_size; /* convert off_t to size_t */
	return true;
}

struct kcndb_file *
kcndb_file_open(const char *path, int flags)
{
	struct kcndb_file *kf;

	kf = kcndb_file_new();
	if (kf == NULL) {
		KCN_LOG(DEBUG, "cannot allocate file structure: %s",
		    strerror(errno));
		return NULL;
	}

	kf->kf_fd = open(path, flags, S_IRUSR | S_IWUSR);
	if (kf->kf_fd == -1) {
		KCN_LOG(DEBUG, "cannot open file: %s", strerror(errno));
		goto bad;
	}
	if (! kcndb_file_size_get(kf)) {
		KCN_LOG(DEBUG, "cannot get file size: %s", strerror(errno));
		goto bad;
	}
	return kf;
  bad:
	kcndb_file_destroy(kf);
	return NULL;
}

void
kcndb_file_close(struct kcndb_file *kf)
{

	kcndb_file_destroy(kf);
}

int
kcndb_file_fd(const struct kcndb_file *kf)
{

	return kf->kf_fd;
}

size_t
kcndb_file_size(const struct kcndb_file *kf)
{

	return kf->kf_size;
}

struct kcn_pkt *
kcndb_file_buf(struct kcndb_file *kf)
{

	return &kf->kf_kp;
}

bool
kcndb_file_ensure(struct kcndb_file *kf, size_t len)
{
	struct kcn_pkt *kp = kcndb_file_buf(kf);
	int error;

	while (kcn_pkt_len(kp) < len) {
		error = kcn_pkt_read(kf->kf_fd, kp);
		if (error != 0) {
			errno = error;
			if (error != ESHUTDOWN)
				KCN_LOG(ERR, "cannot read file: %s",
				    strerror(error));
			return false;
		}
	}
	return true;
}

static bool
kcndb_file_seek(struct kcndb_file *kf, off_t off, int whence)
{

	if ((off = lseek(kf->kf_fd, off, whence)) == -1) {
		KCN_LOG(DEBUG, "seek() failure: %s", strerror(errno));
		return false;
	}
	KCN_LOG(DEBUG, "seek at %zd from %s", (ssize_t)off,
	    whence == SEEK_SET ? "head" : whence == SEEK_END ? "end" :
	    whence == SEEK_CUR ? "current" : "unknown");
	return true;
}

bool
kcndb_file_seek_head(struct kcndb_file *kf, off_t off)
{

	return kcndb_file_seek(kf, off, SEEK_SET);
}

bool
kcndb_file_write(struct kcndb_file *kf)
{
	struct kcn_pkt *kp = kcndb_file_buf(kf);
	int error;

	while (kcn_pkt_len(kp) > 0)
		if ((error = kcn_pkt_write(kf->kf_fd, kp)) != 0) {
			KCN_LOG(ERR, "cannot write file: %s", strerror(error));
			return false;
		}
	kcn_pkt_reset(kp, 0);
	return true;
}

bool
kcndb_file_append(struct kcndb_file *kf)
{
	size_t len;
	bool rc;

	if (! kcndb_file_seek(kf, 0, SEEK_END))
		return false;
	len = kcn_pkt_len(&kf->kf_kp);
	rc = kcndb_file_write(kf);
	if (rc)
		kf->kf_size += len;
	return rc;
}
