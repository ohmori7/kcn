#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "kcn.h"
#include "kcn_log.h"
#include "kcndbctl_file.h"

static const char *usage(const char *, const char *, ...);

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

	return kcndbctl_file_process(argv[0]);
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
