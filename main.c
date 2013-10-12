#include <err.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "kcn_search.h"

#define KCN_LOC_COUNT_MAX_DEFAULT	1
#define KCN_LOC_TYPE_DEFAULT		SEARCH_TYPE_DOMAINNAME

static void usage(const char *, const char *);
static void doit(enum search_type, size_t, int, char * const []);

int
main(int argc, char * const argv[])
{
	enum search_type type;
	const char *p, *pname;
	int n, ch;

	pname = (p = strrchr(argv[0], '/')) != NULL ? p + 1 : argv[0];

	type = KCN_LOC_TYPE_DEFAULT;
	n = KCN_LOC_COUNT_MAX_DEFAULT;
	while ((ch = getopt(argc, argv, "n:t:")) != -1) {
		switch (ch) {
		case 'n':
			n = atoi(optarg);
			break;
		case 't':
			if (strcasecmp(optarg, "domain") == 0)
				type = SEARCH_TYPE_DOMAINNAME;
			else if (strcasecmp(optarg, "uri") == 0)
				type = SEARCH_TYPE_URI;
			else if (strcasecmp(optarg, "ip") == 0)
				usage(pname, "not yet supported locator type");
				/*NOTREACHED*/
			else
				usage(pname, "unknown locator type");
				/*NOTREACHED*/
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
		usage(pname, "no keywords specified");
		/*NOTREACHED*/

	doit(type, n, argc, argv);
	return 0;
}

static void
usage(const char *pname, const char *errmsg)
{

	if (errmsg != NULL)
		fprintf(stderr, "ERROR: %s\n\n", errmsg);
	fprintf(stderr, "\
Usage: %s [-n number] [-t type] <keyword1> [<keyword2>] [<keyword3>] ...\n\
\n\
Options:\n\
	number: the maximum number of locators returned\n\
	type: a type of locators returned\n\
		domain: domain name\n\
		URI: URI\n\
		IP: IP address (not supported yet)\n\
\n",
	    pname);
	exit(EXIT_FAILURE);
}

static void
doit(enum search_type type, size_t nmaxlocs, int argc, char * const argv[])
{
	struct search_res *sr;
	int error;
	size_t i;

	sr = search_res_new(type, nmaxlocs);
	if (sr == NULL)
		errx(EXIT_FAILURE, "cannot allocate search results");
	error = search(argc, argv, sr);
	if (error != 0)
		errx(EXIT_FAILURE, "search failure");
	for (i = 0; i < search_res_nlocs(sr); i++)
		fprintf(stderr, "%s\n", search_res_loc(sr, i));
	search_res_destroy(sr);
}
