#include <sys/time.h>
#include <err.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "kcn.h"
#include "kcn_eq.h"
#include "kcn_str.h"
#include "kcn_log.h"
#include "kcn_netstat.h"

#include "kcndb_db.h"
#include "kcndb_server.h"

static void usage(const char *, ...);

static const char *pname;

int
main(int argc, char * const argv[])
{
	const char *p;
	bool dflag, fflag;
	int ch;
	unsigned long long llval;

	pname = (p = strrchr(argv[0], '/')) != NULL ? p + 1 : argv[0];

	dflag = fflag = false;
	while ((ch = getopt(argc, argv, "d:fhp:v?")) != -1) {
		switch (ch) {
		case 'd':
			if (dflag)
				usage("-d can be specified only once");
				/*NOTREACHED*/
			kcndb_db_path_set(optarg);
			break;
		case 'f':
			fflag = true;
			break;
		case 'p':
			if (! kcn_strtoull(optarg, KCNDB_NET_PORT_MIN,
			    KCNDB_NET_PORT_MAX, &llval))
				usage("invalid TCP port number");
				/*NOTERACHED*/
			kcndb_server_port_set(llval);
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

	if (! kcndb_db_init())
		usage("cannot open database path, \"%s\"", kcndb_db_path_get());
		/*NOTREACHED*/

	if (! fflag && daemon(1, 1) == -1)
		usage("cannot daemonize");
		/*NOTREACHED*/

	if (! kcndb_server_start())
		usage("cannot launch server");
		/*NOTREACHED*/

	kcndb_server_loop();

	kcndb_db_finish();

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
	    pname, KCNDB_DB_PATH_DEFAULT, KCN_NETSTAT_PORT_DEFAULT);
	exit(EXIT_FAILURE);
}
