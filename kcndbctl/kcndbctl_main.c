#include <err.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <event.h>

#include "kcn.h"
#include "kcn_log.h"
#include "kcn_buf.h"
#include "kcn_net.h"
#include "kcn_eq.h"
#include "kcn_msg.h"
#include "kcn_client.h"
#include "kcndbctl_msg.h"
#include "kcndbctl_file.h"

static const char *usage(const char *, const char *, ...);

int
main(int argc, char * const argv[])
{
	const char *pname;
	const char *path;
	struct event_base *evb;
	struct kcn_net *kn;
	enum kcn_eq_type type;
	int ch, rc;

	pname = (pname = strrchr(argv[0], '/')) != NULL ? pname + 1 : argv[0];
	path = NULL;

	while ((ch = getopt(argc, argv, "f:hv?")) != -1) {
		switch (ch) {
		case 'f':
			path = optarg;
			break;
		case 'v':
			kcn_log_priority_increment();
			break;
		case 'h':
		case '?':
		default:
			usage(pname, NULL);
			/*NOTREACHED*/
		}
	}
	argc -= optind;
	argv += optind;

	if (argc < 1)
		usage(pname, "missing database type");
		/*NOTREACHED*/
	if (! kcn_eq_type_aton(argv[0], &type))
		usage(pname, "unknown database type");
		/*NOTREACHED*/
	KCN_LOG(NOTICE, "choose a table type of %s", kcn_eq_type_ntoa(type));
	--argc, ++argv;

	evb = event_init();
	if (evb == NULL)
		usage(pname, "cannot allocate event base");
		/*NOTREACHED*/

	kn = kcn_client_init(evb, NULL);
	if (kn == NULL)
		usage(pname, "cannot connect to kcndbd");
		/*NOTREACHED*/

	if ((path != NULL && argc > 0) || (path == NULL && argc != 2))
		usage(pname, "wrong number of arguments");
		/*NOTREACHED*/

	if (path != NULL)
		rc = kcndbctl_file_process(type, kn, path);
	else
		rc = kcndbctl_msg_add_send(type, kn, argv[0], argv[1]);

	kcn_client_finish(kn);
	event_base_free(evb);

	return rc;
}

static const char *
usage(const char *pname, const char *errfmt, ...)
{
	va_list ap;
	enum kcn_eq_type type;

	if (errfmt != NULL)  {
		fprintf(stderr, "ERROR: ");
		va_start(ap, errfmt);
		vfprintf(stderr, errfmt, ap);
		va_end(ap);
		fprintf(stderr, "\n");
	}
	fprintf(stderr, "\
Usage: %s [-v] type value locator\n\
       %s [-v] -f filename type\n\
Options:\n\
	type: Database type.\n\
	value, locator: Send current data to a KCN database server.\n\
			Note that timestamp is computed at the server.\n\
			\"value\" and \"locator\" are same as below.\n\
	-f filename: Specify a file name, parse data, and send messages to\n\
		     the server.  A file should be formatted as follows:\n\
\n\
		          timestamp value locator\n\
\n\
			timestamp:	UTC time in second.\n\
			value:		value of unsigned 64-bit integer.\n\
			locator:	FQDN or URI consisting of printable\n\
					characters.\n\
 	-v: Increment verbosity (can be specified 7 times at maximum).\n\
\n\
Supported database types are:\n\
",
	    pname, pname);
	for (type = KCN_EQ_TYPE_MIN + 1; type < KCN_EQ_TYPE_MAX; type++)
		fprintf(stderr, "\t%s\n", kcn_eq_type_ntoa(type));
	exit(EXIT_FAILURE);
}
