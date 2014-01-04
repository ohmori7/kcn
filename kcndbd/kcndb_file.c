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
#include "kcn_buf.h"
#include "kcndb_file.h"

struct kcndb_file {
	int kf_fd;
	size_t kf_size;
	struct kcn_buf kf_kb;
	struct kcn_buf_data *kf_kbd;
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
	kf->kf_kbd = kcn_buf_data_new(KCNDB_FILE_BUFSIZ);
	if (kf->kf_kbd == NULL)
		goto bad;
	kcn_buf_init(&kf->kf_kb, kf->kf_kbd);
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
	kcn_buf_data_destroy(kf->kf_kbd);
	free(kf);
}

struct kcndb_file *
kcndb_file_open(const char *path)
{
	struct kcndb_file *kf;

	kf = kcndb_file_new();
	if (kf == NULL) {
		KCN_LOG(DEBUG, "cannot allocate file structure: %s",
		    strerror(errno));
		return NULL;
	}

	kf->kf_fd = open(path, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
	if (kf->kf_fd == -1) {
		KCN_LOG(DEBUG, "cannot open file: %s", strerror(errno));
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

bool
kcndb_file_size_get(const struct kcndb_file *kf, size_t *sizep)
{
	struct stat st;

	if (fstat(kf->kf_fd, &st) == -1)
		return false;
	*sizep = st.st_size; /* convert off_t to size_t */
	return true;
}

int
kcndb_file_fd(const struct kcndb_file *kf)
{

	return kf->kf_fd;
}

struct kcn_buf *
kcndb_file_buf(struct kcndb_file *kf)
{

	return &kf->kf_kb;
}

bool
kcndb_file_ensure(struct kcndb_file *kf, size_t len)
{
	struct kcn_buf *kb = kcndb_file_buf(kf);
	int error;

	while (kcn_buf_len(kb) < len) {
		error = kcn_buf_read(kf->kf_fd, kb);
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
	struct kcn_buf *kb = kcndb_file_buf(kf);
	int error;

	while (kcn_buf_len(kb) > 0)
		if ((error = kcn_buf_write(kf->kf_fd, kb)) != 0) {
			KCN_LOG(ERR, "cannot write file: %s", strerror(error));
			return false;
		}
	kcn_buf_reset(kb, 0);
	return true;
}

bool
kcndb_file_append(struct kcndb_file *kf)
{
	size_t len;
	bool rc;

	if (! kcndb_file_seek(kf, 0, SEEK_END))
		return false;
	len = kcn_buf_len(&kf->kf_kb);
	rc = kcndb_file_write(kf);
	if (rc)
		kf->kf_size += len;
	return rc;
}
