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
#include "kcn_info.h"
#include "kcn_str.h"
#include "kcn_pkt.h"
#include "kcn_net.h"
#include "kcn_formula.h"
#include "kcn_msg.h"
#include "kcn_client.h"

static const char *usage(const char *, const char *, ...);
static int doit(const char *);

int
main(int argc, char * const argv[])
{
	const char *pname;
	int ch;

	pname = (pname = strrchr(argv[0], '/')) != NULL ? pname + 1 : argv[0];

	while ((ch = getopt(argc, argv, "hv?")) != -1) {
		switch (ch) {
		case 'v':
			kcn_log_priority_increment();
			break;
		case 'h':
		case '?':
		default:
			usage(pname, NULL);
		}
	}
	argc -= optind;
	argv += optind;

	if (argc < 1)
		usage(pname, "no argument specified");
		/*NOTREACHED*/
	if (argc > 1)
		usage(pname, "extra argument specified");
		/*NOTREACHED*/

	return doit(argv[0]);
}

static const char *
usage(const char *pname, const char *errfmt, ...)
{
	va_list ap;

	if (errfmt != NULL)  {
		va_start(ap, errfmt);
		vfprintf(stderr, errfmt, ap);
		va_end(ap);
	}
	fprintf(stderr, "\
Usage: %s [-v] <filename>\n\
\n",
	    pname);
	exit(EXIT_FAILURE);
}

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
bufalloc(struct kcn_pkt *kp, size_t filesize)
{
	size_t bufsize;
	struct kcn_pkt_data *kpd;

#define MAXBUFSIZ	(1ULL << 20)	/* 1MB */
	if (filesize < MAXBUFSIZ)
		bufsize = filesize;
	else
		bufsize = MAXBUFSIZ;
	kpd = kcn_pkt_data_new(bufsize);
	if (kpd == NULL)
		return false;
	kcn_pkt_init(kp, kpd);
	return true;
}

static bool
forward(struct kcn_pkt *kp)
{
	int c;

	for (;;) {
		if (kcn_pkt_trailingdata(kp) == 0) {
			errno = EAGAIN;
			return false;
		}
		c = kcn_pkt_get8(kp);
		if (isspace(c))
			return true;
		if (! isprint(c)) {
			errno = EINVAL;
			return false;
		}
	}
}

static bool
getstr(struct kcn_pkt *kp, const char **s, size_t *slenp)
{
	size_t slen;

	*s = kcn_pkt_current(kp);
	if (! forward(kp))
		return false;
	slen = *slenp = kcn_pkt_current(kp) - (void *)*s - 1;
	if (slen > KCN_MSG_MAXLOCSIZ) {
		errno = E2BIG;
		return false;
	}
	return true;
}

static bool
get64(struct kcn_pkt *kp, uint64_t *vp)
{
	unsigned long long ullval;
	const char *p;

	p = kcn_pkt_current(kp);
	if (! forward(kp))
		return false;
	kcn_pkt_backward(kp, 1);
	kcn_pkt_put8(kp, '\0');
	if (! kcn_strtoull(p, 0, ULLONG_MAX, &ullval))
		return false;
	*vp = ullval;
	return true;
}

struct kcn_file {
	size_t kf_size;
	size_t kf_line;
	struct kcn_pkt kf_kp;
	struct event kf_ev;
	struct kcn_net *kf_kn;
};

static void
readfile(int fd, short event, void *arg)
{
	struct kcn_file *kf = arg;
	struct kcn_pkt *kp = &kf->kf_kp;
	struct kcn_msg_add kma;
	int error;

	if ((event & EV_READ) == 0)
		goto again;

	error = kcn_pkt_read(fd, kp);
	if (error != 0)
		goto out;
	kf->kf_size -= kcn_pkt_len(kp);
	while (kf->kf_size == 0 || kcn_pkt_len(kp) >= KCN_MSG_MAXSIZ) {
		if (! getstr(kp, &kma.kma_loc, &kma.kma_loclen) ||
		    ! get64(kp, &kma.kma_val) ||
		    ! get64(kp, &kma.kma_time)) {
			if (errno != EAGAIN)
				err(EXIT_FAILURE, "invalid field");
			KCN_LOG(DEBUG, "partial read");
			kcn_pkt_start(kp);
			break;
		}
		if (! kcn_client_add_send(kf->kf_kn, &kma))
			err(EXIT_FAILURE, "cannot send add message");
		++kf->kf_line;
		kcn_pkt_trim_head(kp, kcn_pkt_headingdata(kp));
		KCN_LOG(DEBUG, "%zu %llu %.*s", kma.kma_time,
		    kma.kma_val, (int)kma.kma_loclen, kma.kma_loc);
	}
	kf->kf_size += kcn_pkt_len(kp);

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

static int
doit(const char *path)
{
	struct event_base *evb;
	struct kcn_file kf;
	int fd;

	kf.kf_line = 0;

	evb = event_init();
	if (evb == NULL)
		err(EXIT_FAILURE, "cannot allocate event base");

	kf.kf_kn = kcn_client_init(evb, NULL);
	if (kf.kf_kn == NULL)
		err(EXIT_FAILURE, "cannot connect to kcndbd: %s",
		    strerror(errno));

	fd = open(path, O_RDONLY);
	if (fd == -1)
		err(EXIT_FAILURE, "cannot open file: %s", strerror(errno));
	if (flock(fd, LOCK_EX) == -1)
		err(EXIT_FAILURE, "cannot lock file: %s", strerror(errno));
	if (! getfilesize(fd, &kf.kf_size))
		err(EXIT_FAILURE, "cannot get file size: %s", strerror(errno));
	if (! bufalloc(&kf.kf_kp, kf.kf_size))
		err(EXIT_FAILURE, "cannot alloc. buffer: %s", strerror(errno));
	event_set(&kf.kf_ev, fd, EV_READ, readfile, &kf);
	if (event_base_set(evb, &kf.kf_ev) == -1)
		err(EXIT_FAILURE, "cannot set event base: %s", strerror(errno));
	if (event_add(&kf.kf_ev, NULL) == -1)
		err(EXIT_FAILURE, "cannot enable event: %s", strerror(errno));

	kcn_net_loop(kf.kf_kn);

	(void)flock(fd, LOCK_UN);
	(void)close(fd);

	kcn_client_finish(kf.kf_kn);
	event_base_free(evb);

	return 0;
}
