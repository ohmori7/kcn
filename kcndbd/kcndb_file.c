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
#define KCNDB_FILE_BUFSIZ	4096	/* XXX */
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

struct kcndb_file *
kcndb_file_open(const char *path, enum kcndb_file_op op)
{
	struct kcndb_file *kf;
	int flags;

	kf = kcndb_file_new();
	if (kf == NULL) {
		KCN_LOG(DEBUG, "cannot allocate file structure: %s",
		    strerror(errno));
		return NULL;
	}

	if (op == KCNDB_FILE_OP_READ)
		flags = O_RDONLY;
	else {
		assert(op == KCNDB_FILE_OP_WRITE);
		flags = O_WRONLY | O_CREAT | O_APPEND;
	}
	kf->kf_fd = open(path, flags, S_IRUSR | S_IWUSR);
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

struct kcn_pkt *
kcndb_file_packet(struct kcndb_file *kf)
{

	return &kf->kf_kp;
}

bool
kcndb_file_ensure(struct kcndb_file *kf, size_t len)
{
	struct kcn_pkt *kp = kcndb_file_packet(kf);
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

bool
kcndb_file_write(struct kcndb_file *kf)
{
	struct kcn_pkt *kp = kcndb_file_packet(kf);
	int error;

	while (kcn_pkt_len(kp) > 0)
		if ((error = kcn_pkt_write(kf->kf_fd, kp)) != 0) {
			KCN_LOG(ERR, "cannot write database: %s",
			    strerror(error));
			return false;
		}
	kcn_pkt_reset(kp, 0);
	return true;
}
