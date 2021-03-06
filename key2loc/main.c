#include <sys/time.h>
#include <err.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "kcn.h"
#include "kcn_log.h"
#include "kcn_info.h"
#include "kcn_db.h"
#include "kcn_netstat.h"
#include "kcn_google.h"
#include "kcn_buf.h"
#include "kcn_eq.h"
#include "kcn_msg.h"
#include "kcn_client.h"

#define KCN_DB_DEFAULT			NULL
#define KCN_LOC_COUNT_MAX_DEFAULT	1
#define KCN_LOC_TYPE_DEFAULT		KCN_LOC_TYPE_DOMAINNAME

static void usage(const char *, const char *);
static void doit(size_t, const char *, enum kcn_loc_type, size_t,
    const char *, const char *, int, char * const []);

int
main(int argc, char * const argv[])
{
	enum kcn_loc_type loctype;
	const char *p, *pname, *db, *country, *userip, *server;
	size_t r;
	int n, ch;

	pname = (p = strrchr(argv[0], '/')) != NULL ? p + 1 : argv[0];

	kcn_netstat_init();
	kcn_google_init();

	db = KCN_DB_DEFAULT;
	loctype = KCN_LOC_TYPE_DEFAULT;
	country = NULL;
	userip = NULL;
	server = NULL;
	r = 1;
	n = KCN_LOC_COUNT_MAX_DEFAULT;
	while ((ch = getopt(argc, argv, "c:hi:n:l:r:s:t:v?")) != -1) {
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
		case 'r':
			r = atoi(optarg);
			break;
		case 's':
			if (server != NULL)
				usage(pname, "multiple servers specified");
				/*NOTREACHED*/
			server = optarg;
			break;
		case 't':
			if (! kcn_db_exists(optarg))
				usage(pname, "unknown database type");
				/*NOTREACHED*/
			db = optarg;
			break;
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
		usage(pname, "no keywords specified");
		/*NOTREACHED*/

	if (server != NULL)
		kcn_client_server_name_set(server);

	doit(r, db, loctype, n, country, userip, argc, argv);
	return 0;
}

static void
usage(const char *pname, const char *errmsg)
{

	if (errmsg != NULL)
		fprintf(stderr, "ERROR: %s\n\n", errmsg);
	fprintf(stderr, "\
Usage: %s [-c country] [-i user IP] [-l loctype] [-n number] [-t type] [-v] <keyword1> [<keyword2>] [<keyword3>] ...\n\
\n\
Options:\n\
	-c country: a country code of locators in ISO 3166-1 (e.g., us, jp)\n\
	-i user IP: the IP address of this host\n\
	-l loctype: a type of locators returned\n\
		domain: domain name\n\
		URI: URI\n\
		IP: IP address (not supported yet)\n\
	-n number: the maximum number of locators returned\n\
	-r count: repeat to query for ``count'' times\n\
	-s server: KCN database server\n\
	-t type: a type of database listed below\n\
	-v: increment verbosity (can be specified 7 times at maximum)\n\
\n",
	    pname);
	fprintf(stderr, "\
Supported database types are:\n");
	kcn_db_name_list_puts("\t");
	exit(EXIT_FAILURE);
}

static void
doit(size_t r, const char *db, enum kcn_loc_type loctype, size_t nmaxlocs,
    const char *country, const char *userip, int keyc, char * const keyv[])
{
	struct kcn_info *ki;
	size_t i, oerrno;
	struct timeval tvs, tve, tvd;
	bool rc;

	ki = kcn_info_new(loctype, nmaxlocs);
	if (ki == NULL)
		err(EXIT_FAILURE, "cannot allocate KCN information structure");
	kcn_info_db_set(ki, db);
	kcn_info_country_set(ki, country);
	kcn_info_userip_set(ki, userip);

	for (i = 0; i < r; i++) {
		kcn_info_loc_free(ki);
		gettimeofday(&tvs, NULL);
		rc = kcn_searchv(ki, keyc, keyv);
		oerrno = errno;
		gettimeofday(&tve, NULL);
		timersub(&tve, &tvs, &tvd);
		fprintf(stderr, "Resolution finishes with %zu.%06zu sec\n",
		    (size_t)tvd.tv_sec, (size_t)tvd.tv_usec);
		errno = oerrno;
		if (! rc)
			err(EXIT_FAILURE, "search failure");
	}
	for (i = 0; i < kcn_info_nlocs(ki); i++)
		printf("%s\n", kcn_info_loc(ki, i));
	kcn_info_destroy(ki);
}
