#include <assert.h>
#include <err.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "kcn.h"
#include "kcn_info.h"
#include "kcn_search.h"

#define KCN_TYPE_DEFAULT		KCN_TYPE_GOOGLE
#define KCN_LOC_COUNT_MAX_DEFAULT	1
#define KCN_LOC_TYPE_DEFAULT		KCN_LOC_TYPE_DOMAINNAME

static void usage(const char *, const char *);
static char *concat(int, char * const []);
static void doit(enum kcn_type, enum kcn_loc_type, size_t,
    const char *, const char *, const char *);

int
main(int argc, char * const argv[])
{
	enum kcn_type type;
	enum kcn_loc_type loctype;
	const char *p, *pname, *country, *userip;
	char *keys;
	int n, ch;

	pname = (p = strrchr(argv[0], '/')) != NULL ? p + 1 : argv[0];

	type = KCN_TYPE_DEFAULT;
	loctype = KCN_LOC_TYPE_DEFAULT;
	country = NULL;
	userip = NULL;
	n = KCN_LOC_COUNT_MAX_DEFAULT;
	while ((ch = getopt(argc, argv, "c:i:n:l:t:")) != -1) {
		switch (ch) {
		case 'c':
			country = optarg;
			break;
		case 'i':
			userip = optarg;
			break;
		case 'l':
			if (strcasecmp(optarg, "domain") == 0)
				loctype = KCN_LOC_TYPE_DOMAINNAME;
			else if (strcasecmp(optarg, "uri") == 0)
				loctype = KCN_LOC_TYPE_URI;
			else if (strcasecmp(optarg, "ip") == 0)
				usage(pname, "not yet supported locator type");
				/*NOTREACHED*/
			else
				usage(pname, "unknown locator type");
				/*NOTREACHED*/
			break;
		case 'n':
			n = atoi(optarg);
			break;
		case 't':
			if (strcasecmp(optarg, "google") == 0)
				type = KCN_TYPE_GOOGLE;
			else if (strcasecmp(optarg, "local") == 0)
				type = KCN_TYPE_LOCAL;
			else
				usage(pname, "unknown search type");
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

	keys = concat(argc, argv);
	if (keys == NULL)
		err(EXIT_FAILURE, "cannot allocate memory for URI");
		/*NOTREACHED*/
	doit(type, loctype, n, country, userip, keys);
	free(keys);
	return 0;
}

static void
usage(const char *pname, const char *errmsg)
{

	if (errmsg != NULL)
		fprintf(stderr, "ERROR: %s\n\n", errmsg);
	fprintf(stderr, "\
Usage: %s [-c country] [-i user IP] [-l loctype] [-n number] [-t type] <keyword1> [<keyword2>] [<keyword3>] ...\n\
\n\
Options:\n\
	country: a country code of locators in ISO 3166-1 (e.g., us, jp)\n\
	user IP: the IP address of this host\n\
	loctype: a type of locators returned\n\
		domain: domain name\n\
		URI: URI\n\
		IP: IP address (not supported yet)\n\
	type: a type of search\n\
		google: Google search\n\
		local: KCN local database search\n\
	number: the maximum number of locators returned\n\
\n",
	    pname);
	exit(EXIT_FAILURE);
}

static char *
concat(int keyc, char * const keyv[])
{
	char *s;
	size_t size;
	int i;

	assert(keyc > 0);
	size = keyc; /* the number of separators and terminator */
	for (i = 0; i < keyc; i++)
		size += strlen(keyv[i]);
	s = malloc(size);
	if (s == NULL)
		return NULL;
	i = 0;
	strncpy(s, keyv[i++], size);
	for (; i < keyc; i++) {
		(void)strncat(s, " ", size);
		(void)strncat(s, keyv[i], size);
	}
	return s;
}

static void
doit(enum kcn_type type, enum kcn_loc_type loctype, size_t nmaxlocs,
    const char *country, const char *userip, const char *keys)
{
	struct kcn_info *ki;
	size_t i;

	ki = kcn_info_new(type, loctype, nmaxlocs, country, userip);
	if (ki == NULL)
		err(EXIT_FAILURE, "cannot allocate KCN information structure");
	if (! kcn_search(ki, keys))
		err(EXIT_FAILURE, "search failure");
	for (i = 0; i < kcn_info_nlocs(ki); i++)
		printf("%s\n", kcn_info_loc(ki, i));
	kcn_info_destroy(ki);
}
