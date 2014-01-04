#include <sys/param.h>	/* MAXPATHLEN */
#include <sys/stat.h>
#include <sys/file.h>	/* XXX: flock on linux. */
#include <assert.h>
#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <event.h>

#include "kcn.h"
#include "kcn_log.h"
#include "kcn_str.h"
#include "kcn_buf.h"
#include "kcn_net.h"
#include "kcn_eq.h"
#include "kcn_msg.h"
#include "kcn_client.h"
#include "kcndbctl_file.h"

static bool
getfilesize(int fd, size_t *sizep)
{
	struct stat st;

	if (fstat(fd, &st) == -1)
		return false;
	*sizep = st.st_size;
	return true;
}

static bool
bufalloc(struct kcn_buf *kb, size_t filesize)
{
	size_t bufsize;
	struct kcn_buf_data *kbd;

#define MAXBUFSIZ	(1ULL << 20)	/* 1MB */
	if (filesize < MAXBUFSIZ)
		bufsize = filesize;
	else
		bufsize = MAXBUFSIZ;
	kbd = kcn_buf_data_new(bufsize);
	if (kbd == NULL)
		return false;
	kcn_buf_init(kb, kbd);
	return true;
}

static bool
forward(struct kcn_buf *kb)
{
	int c;

	for (;;) {
		if (kcn_buf_trailingdata(kb) == 0) {
			errno = EAGAIN;
			return false;
		}
		c = kcn_buf_get8(kb);
		if (isspace(c))
			return true;
		if (! isprint(c)) {
			errno = EINVAL;
			return false;
		}
	}
}

static bool
getstr(struct kcn_buf *kb, const char **s, size_t *slenp)
{
	size_t slen;

	*s = kcn_buf_current(kb);
	if (! forward(kb))
		return false;
	slen = *slenp = kcn_buf_current(kb) - (void *)*s - 1;
	if (slen > KCN_MSG_MAXLOCSIZ) {
		errno = E2BIG;
		return false;
	}
	return true;
}

static bool
get64(struct kcn_buf *kb, uint64_t *vp)
{
	unsigned long long ullval;
	const char *p;

	p = kcn_buf_current(kb);
	if (! forward(kb))
		return false;
	kcn_buf_backward(kb, 1);
	kcn_buf_put8(kb, '\0');
	if (! kcn_strtoull(p, 0, ULLONG_MAX, &ullval))
		return false;
	*vp = ullval;
	return true;
}

struct kcn_file {
	enum kcn_eq_type kf_type;
	size_t kf_size;
	size_t kf_line;
	struct kcn_buf kf_kb;
	struct event kf_ev;
	struct kcn_net *kf_kn;
};

static void
readfile(int fd, short event, void *arg)
{
	struct kcn_file *kf = arg;
	struct kcn_buf *kb = &kf->kf_kb;
	struct kcn_msg_add kma;
	int error;

	if ((event & EV_READ) == 0)
		goto again;

	error = kcn_buf_read(fd, kb);
	if (error != 0)
		goto out;
	kma.kma_type = kf->kf_type;
	kf->kf_size -= kcn_buf_len(kb);
	while (kf->kf_size == 0 || kcn_buf_len(kb) >= KCN_MSG_MAXSIZ) {
		if (! get64(kb, &kma.kma_time) ||
		    ! get64(kb, &kma.kma_val) ||
		    ! getstr(kb, &kma.kma_loc, &kma.kma_loclen)) {
			if (errno != EAGAIN)
				err(EXIT_FAILURE, "invalid field");
			KCN_LOG(DEBUG, "partial read");
			kcn_buf_start(kb);
			break;
		}
		if (! kcn_client_add_send(kf->kf_kn, &kma))
			err(EXIT_FAILURE, "cannot send add message");
		++kf->kf_line;
		kcn_buf_trim_head(kb, kcn_buf_headingdata(kb));
		KCN_LOG(DEBUG, "%zu %llu %.*s", kma.kma_time,
		    kma.kma_val, (int)kma.kma_loclen, kma.kma_loc);
	}
	kf->kf_size += kcn_buf_len(kb);

  out:
	KCN_LOG(INFO, "file read with %s", strerror(error));
	if (error == ESHUTDOWN) {
		KCN_LOG(INFO, "finish reading file, %zu lines read",
		    kf->kf_line);
		return;
	}
  again:
	if (event_add(&kf->kf_ev, NULL) == -1)
		err(EXIT_FAILURE, "cannot add event");
}

int
kcndbctl_file_process(enum kcn_eq_type type, struct kcn_net *kn,
    const char *path)
{
	struct kcn_file kf;
	int fd, rc;

	kf.kf_type = type;
	kf.kf_line = 0;
	kf.kf_kn = kn;

	fd = open(path, O_RDONLY);
	if (fd == -1)
		err(EXIT_FAILURE, "cannot open file: %s", strerror(errno));
	if (flock(fd, LOCK_EX) == -1)
		err(EXIT_FAILURE, "cannot lock file: %s", strerror(errno));
	if (! getfilesize(fd, &kf.kf_size))
		err(EXIT_FAILURE, "cannot get file size: %s", strerror(errno));
	if (! bufalloc(&kf.kf_kb, kf.kf_size))
		err(EXIT_FAILURE, "cannot alloc. buffer: %s", strerror(errno));
	event_set(&kf.kf_ev, fd, EV_READ, readfile, &kf);
	if (event_base_set(kcn_net_event_base(kf.kf_kn), &kf.kf_ev) == -1)
		err(EXIT_FAILURE, "cannot set event base: %s", strerror(errno));
	if (event_add(&kf.kf_ev, NULL) == -1)
		err(EXIT_FAILURE, "cannot enable event: %s", strerror(errno));

	if (kcn_net_loop(kf.kf_kn))
		rc = EXIT_SUCCESS;
	else
		rc = EXIT_FAILURE;

	(void)flock(fd, LOCK_UN);
	(void)close(fd);

	return rc;
}
