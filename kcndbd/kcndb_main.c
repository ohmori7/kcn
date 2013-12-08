#include <err.h>
#include <errno.h>
#include <limits.h>	/* LONG_{MIN,MAX} */
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "kcn.h"
#include "kcn_log.h"

#include "kcndb_db.h"
#include "kcndb_net.h"

static void usage(const char *, ...);

static const char *pname;

int
main(int argc, char * const argv[])
{
	const char *p;
	char *ep;
	bool fflag;
	int ch;
	long lval;

	pname = (p = strrchr(argv[0], '/')) != NULL ? p + 1 : argv[0];

	fflag = false;
	while ((ch = getopt(argc, argv, "d:p:v?")) != -1) {
		switch (ch) {
		case 'd':
			if (! kcndb_db_path_set(optarg))
				usage("cannot set DB path, \"%s\"", optarg);
				/*NOTREACHED*/
			break;
		case 'f':
			fflag = true;
			break;
		case 'p':	
			errno = 0;
			lval = strtol(optarg, &ep, 10);
			if (*ep != '\0')
				usage("invalid TCP port number");
				/*NOTREACHED*/
			if ((errno == ERANGE &&
			     (lval == LONG_MAX || lval == LONG_MIN)) ||
			    lval < KCNDB_NET_PORT_MIN ||
			    lval > KCNDB_NET_PORT_MAX)
				usage("invalid TCP port number");
				/*NOTERACHED*/
			if (! kcndb_net_port_set(lval))
				usage("cannot set TCP port, \"%s\"", optarg);
				/*NOTREACHED*/
			break;
		case 'v':
			kcn_log_priority_increment();
			break;
		case 'h':
		case '?':
		default:
			usage(NULL);
		}
	}
	argc -= optind;
	argv += optind;

	if (! fflag && daemon(1, 1) == -1)
		usage("cannot daemonize");
		/*NOTREACHED*/

	return 0;
}

static void
usage(const char *errfmt, ...)
{
	va_list ap;

	if (errfmt != NULL) {
		va_start(ap, errfmt);
		fprintf(stderr, "ERROR: ");
		vfprintf(stderr, errfmt, ap);
		if (errno != 0)
			fprintf(stderr, ": %s", strerror(errno));
		fprintf(stderr, "\n\n");
		va_end(ap);
	}
	fprintf(stderr, "\
Usage: %s [-d directory] [-h] [-p port] [-v] ...\n\
\n\
Options:\n\
	-d: databse directory (default %s)\n\
	-f: do not daemonize\n\
	-h: print this messsage\n\
	-p: TCP listen port number (default %d)\n\
	-v: increment verbosity (can be specified 7 times at maximum)\n\
\n",
	    pname, KCNDB_DB_PATH_DEFAULT, KCNDB_NET_PORT_DEFAULT);
	exit(EXIT_FAILURE);
}
