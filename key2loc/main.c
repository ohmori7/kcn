#include <err.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "kcn.h"
#include "kcn_search.h"

#define KCN_LOC_COUNT_MAX_DEFAULT	1
#define KCN_LOC_TYPE_DEFAULT		KCN_LOC_TYPE_DOMAINNAME

static void usage(const char *, const char *);
static void doit(enum kcn_loc_type, size_t,
    const char *, const char *, int, char * const []);

int
main(int argc, char * const argv[])
{
	enum kcn_loc_type type;
	const char *p, *pname, *country, *userip;
	int n, ch;

	pname = (p = strrchr(argv[0], '/')) != NULL ? p + 1 : argv[0];

	type = KCN_LOC_TYPE_DEFAULT;
	country = NULL;
	userip = NULL;
	n = KCN_LOC_COUNT_MAX_DEFAULT;
	while ((ch = getopt(argc, argv, "c:i:n:t:")) != -1) {
		switch (ch) {
		case 'c':
			country = optarg;
			break;
		case 'i':
			userip = optarg;
			break;
		case 'n':
			n = atoi(optarg);
			break;
		case 't':
			if (strcasecmp(optarg, "domain") == 0)
				type = KCN_LOC_TYPE_DOMAINNAME;
			else if (strcasecmp(optarg, "uri") == 0)
				type = KCN_LOC_TYPE_URI;
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

	doit(type, n, country, userip, argc, argv);
	return 0;
}

static void
usage(const char *pname, const char *errmsg)
{

	if (errmsg != NULL)
		fprintf(stderr, "ERROR: %s\n\n", errmsg);
	fprintf(stderr, "\
Usage: %s [-c country] [-i user IP] [-n number] [-t type] <keyword1> [<keyword2>] [<keyword3>] ...\n\
\n\
Options:\n\
	country: the country of locators returned\n\
	user IP: the IP address of this host (not supported yet)\n\
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
doit(enum kcn_loc_type type, size_t nmaxlocs,
    const char *country, const char *userip, int argc, char * const argv[])
{
	struct kcn_search_res *ksr;
	int error;
	size_t i;

	ksr = kcn_search_res_new(type, nmaxlocs, country, userip);
	if (ksr == NULL)
		errx(EXIT_FAILURE, "cannot allocate search results");
	error = kcn_search(argc, argv, ksr);
	if (error != 0)
		errx(EXIT_FAILURE, "search failure: %s", strerror(error));
	for (i = 0; i < kcn_search_res_nlocs(ksr); i++)
		printf("%s\n", kcn_search_res_loc(ksr, i));
	kcn_search_res_destroy(ksr);
}
