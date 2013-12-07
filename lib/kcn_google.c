#include <sys/param.h>	/* just for NBBY. */

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>

#include <jansson.h>

#include "kcn.h"
#include "kcn_uri.h"
#include "kcn_info.h"
#include "kcn_db.h"
#include "kcn_http.h"
#include "kcn_google.h"

/*
 * Google specific constants.
 *
 * You can obtain more information at:
 *	https://developers.google.com/web-search/docs/reference
 */
#define	KCN_GOOGLE_URI_BASE						\
	"http://ajax.googleapis.com/ajax/services/search/web?v=1.0"
#define KCN_GOOGLE_API_QUERYOPT		"q"
#define KCN_GOOGLE_API_RCOUNTOPT	"rsz"
#define KCN_GOOGLE_API_RCOUNTMAX	(8 + 1)
#define KCN_GOOGLE_API_COUNTRYOPT	"gl"
#define KCN_GOOGLE_API_USERIPOPT	"userip"
#define KCN_GOOGLE_API_STARTOPT		"start"

static bool kcn_google_match(const char *, size_t *);
static bool kcn_google_search(struct kcn_info *, const char *);

static struct kcn_db kcn_google = {
	.kd_type = KCN_TYPE_GOOGLE,
	.kd_prio = 0,
	.kd_match = kcn_google_match,
	.kd_search = kcn_google_search
};

void
kcn_google_init(void)
{

	kcn_db_register(&kcn_google);
}

void
kcn_google_finish(void)
{

	kcn_db_deregister(&kcn_google);
}

static bool
kcn_google_match(const char *s, size_t *scorep)
{
	
	(void)*s;
	*scorep = 0;
	return true;
}

static bool
kcn_google_search_one(const struct kcn_uri *ku, struct kcn_info *ki)
{
	char *res;
	json_error_t jerr;
	json_t *jroot, *jval, *jresdata, *jres, *jloc;
	const char *jlocstr;
	const char *jlockey[] = {
		[KCN_LOC_TYPE_DOMAINNAME] = "visibleUrl",
		[KCN_LOC_TYPE_URI] = "url"
	};
	size_t i;

	assert(kcn_info_nlocs(ki) < kcn_info_maxnlocs(ki));

	res = kcn_http_response_get(kcn_uri(ku));
	if (res == NULL) {
		jroot = NULL;
		goto bad;
	}
	jroot = json_loads(res, 0, &jerr);
	kcn_http_response_free(res);
	if (jroot == NULL) {
		fprintf(stderr, "cannot load root object: %s\n", jerr.text);
		errno = EINVAL;
		goto bad;
	}

	jval = json_object_get(jroot, "responseStatus");
	if (jval == NULL || ! json_is_integer(jval)) {
		errno = EINVAL;
		goto bad;
	}
	if (json_integer_value(jval) != KCN_HTTP_OK) {
		if (json_integer_value(jval) == KCN_HTTP_FORBIDDEN)
			errno = EAGAIN; /* XXX */
		else
			errno = ENETDOWN; /* XXX */
		goto bad;
	}

	jresdata = json_object_get(jroot, "responseData");
	if (jresdata == NULL) {
		errno = EINVAL;
		goto bad;
	}

	jres = json_object_get(jresdata, "results");
	if (jres == NULL || ! json_is_array(jres)) {
		fprintf(stderr, "Non-array object (%u) returned",
		    json_typeof(jresdata));
		errno = EINVAL;
		goto bad;
	}
	json_array_foreach(jres, i, jval) {
		jloc = json_object_get(jval, jlockey[kcn_info_loc_type(ki)]);
		if (jloc == NULL ||
		    (jlocstr = json_string_value(jloc)) == NULL) {
			errno = EINVAL;
			goto bad;
		}
		if (! kcn_info_loc_add(ki, jlocstr))
			goto bad;
		if (kcn_info_nlocs(ki) == kcn_info_maxnlocs(ki))
			break;
	}
	if (kcn_info_nlocs(ki) == 0) {
		errno = ESRCH;
		goto bad;
	}
	json_decref(jroot);
	return true;
  bad:
	if (jroot != NULL)
		json_decref(jroot);
	return false;
}

static bool
kcn_google_search(struct kcn_info *ki, const char *keys)
{
	struct kcn_uri *ku;
	char startopt[KCN_INFO_NLOCSTRLEN];
	char rcountmaxstr[2];
	const char *country;
	const char *userip;
	size_t n, uriolen, rcountmax;
	int oerrno;

	assert(kcn_info_loc_type(ki) == KCN_LOC_TYPE_DOMAINNAME ||
	    kcn_info_loc_type(ki) == KCN_LOC_TYPE_URI);
	assert(kcn_info_maxnlocs(ki) > 0);

	oerrno = errno;
	ku = kcn_uri_new(KCN_GOOGLE_URI_BASE);
	if (ku == NULL)
		goto bad;

	if (kcn_info_maxnlocs(ki) < KCN_GOOGLE_API_RCOUNTMAX)
		rcountmax = kcn_info_maxnlocs(ki);
	else
		rcountmax = KCN_GOOGLE_API_RCOUNTMAX - 1;
	rcountmaxstr[0] = '0' + rcountmax;
	rcountmaxstr[1] = '\0';
	if (! kcn_uri_param_puts(ku, KCN_GOOGLE_API_RCOUNTOPT, rcountmaxstr))
		goto bad;

	country = kcn_info_country(ki);
	if (! kcn_uri_param_puts(ku, KCN_GOOGLE_API_COUNTRYOPT, country))
		goto bad;

	userip = kcn_info_userip(ki);
	if (! kcn_uri_param_puts(ku, KCN_GOOGLE_API_USERIPOPT, userip))
		goto bad;

	if (! kcn_uri_param_puts(ku, KCN_GOOGLE_API_QUERYOPT, keys))
		goto bad;

	uriolen = kcn_uri_len(ku);
	for (n = 0; n < kcn_info_maxnlocs(ki); n += rcountmax) {
		if (n > 0) {
			snprintf(startopt, sizeof(startopt), "%zu", n);
			if (! kcn_uri_param_puts(ku, KCN_GOOGLE_API_STARTOPT,
			    startopt))
				goto bad;
		}
		if (! kcn_google_search_one(ku, ki)) {
			if (errno != ESRCH || kcn_info_nlocs(ki) == 0)
				goto bad;
			errno = oerrno;
			break;
		}
		kcn_uri_resize(ku, uriolen);
		if (kcn_info_nlocs(ki) % rcountmax != 0)
			break;
	}
	kcn_uri_destroy(ku);
	return true;
  bad:
	kcn_uri_destroy(ku);
	return false;
}
