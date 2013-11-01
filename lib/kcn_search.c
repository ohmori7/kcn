#include <sys/param.h>	/* just for NBBY. */

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>

#include <curl/curl.h>
#include <jansson.h>

#include "kcn.h"
#include "kcn_buf.h"
#include "kcn_uri.h"
#include "kcn_info.h"
#include "kcn_search.h"

/* XXX: is there any appropriate library that defines HTTP response code? */
#define KCN_SEARCH_HTTP_OK		200
#define KCN_SEARCH_HTTP_FORBIDDEN	403

/*
 * Google specific constants.
 *
 * You can obtain more information at:
 *	https://developers.google.com/web-search/docs/reference
 */
#define	KCN_SEARCH_URI_BASE_GOOGLE					\
	"http://ajax.googleapis.com/ajax/services/search/web?v=1.0&q="
#define KCN_SEARCH_GOOGLE_API_RCOUNTOPT		"rsz"
#define KCN_SEARCH_GOOGLE_API_RCOUNTMAX		(8 + 1)
#define KCN_SEARCH_GOOGLE_API_COUNTRYOPT	"gl"
#define KCN_SEARCH_GOOGLE_API_USERIPOPT		"userip"
#define KCN_SEARCH_GOOGLE_API_STARTOPT		"start"

static size_t
kcn_search_curl_callback(const char *p, size_t size, size_t n, void *kb)
{
	size_t totalsize;

	totalsize = size * n;
	if (! kcn_buf_append(kb, p, totalsize))
		return 0;
	return totalsize;
}

static char *
kcn_search_response_get(const char *uri)
{
	struct kcn_buf *kb;
	CURL *curl;
	CURLcode curlrc;
	char *p;

	assert(uri != NULL);
	p = NULL;
	kb = kcn_buf_new();
	if (kb == NULL)
		goto bad;

	curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_URL, uri);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, kcn_search_curl_callback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, kb);
	curlrc = curl_easy_perform(curl);
	curl_easy_cleanup(curl);
	if (curlrc == CURLE_OK && kcn_buf_append(kb, "\0", 1))
		p = kcn_buf_get(kb);

	kcn_buf_destroy(kb);
  bad:

	return p;
}

static int
kcn_search_opt_puts(char **urip, const char *param, const char *val)
{
	char *opt;
	size_t optlen;
	bool rc;

	optlen = sizeof('&') + strlen(param) + sizeof('=') + strlen(val) +
	    sizeof('\0');
	opt = malloc(optlen);
	if (opt == NULL)
		return ENOMEM;

	(void)snprintf(opt, optlen, "&%s=%s", param, val);
	rc = kcn_uri_puts(urip, opt);
	free(opt);
	if (rc)
		return 0;
	else
		return ENOMEM;
}

static int
kcn_search_one(const char *uri, struct kcn_info *ki)
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
	int error;

	assert(kcn_info_nlocs(ki) < kcn_info_maxnlocs(ki));

	error = 0;
	res = kcn_search_response_get(uri);
	if (res == NULL)
		return ENETDOWN;
	jroot = json_loads(res, 0, &jerr);
	free(res);
	if (jroot == NULL) {
		fprintf(stderr, "cannot load root object: %s\n", jerr.text);
		return EINVAL;
	}

	jval = json_object_get(jroot, "responseStatus");
	if (jval == NULL || ! json_is_integer(jval)) {
		error = EINVAL;
		goto out;
	}
	if (json_integer_value(jval) != KCN_SEARCH_HTTP_OK) {
		if (json_integer_value(jval) == KCN_SEARCH_HTTP_FORBIDDEN)
			error = EAGAIN; /* XXX */
		else
			error = ENETDOWN; /* XXX */
		goto out;
	}

	jresdata = json_object_get(jroot, "responseData");
	if (jresdata == NULL) {
		error = EINVAL;
		goto out;
	}

	jres = json_object_get(jresdata, "results");
	if (jres == NULL || ! json_is_array(jres)) {
		fprintf(stderr, "Non-array object (%u) returned",
		    json_typeof(jresdata));
		error = EINVAL;
		goto out;
	}
	json_array_foreach(jres, i, jval) {
		jloc = json_object_get(jval, jlockey[kcn_info_type(ki)]);
		if (jloc == NULL ||
		    (jlocstr = json_string_value(jloc)) == NULL) {
			error = EINVAL;
			break;
		}
		error = kcn_info_loc_add(ki, jlocstr);
		if (error != 0)
			break;
		if (kcn_info_nlocs(ki) == kcn_info_maxnlocs(ki))
			break;
	}
	if (json_array_size(jres) == 0)
		error = ESRCH;
  out:
	json_decref(jroot);
	return error;
}

/* XXX: this supports google for now. */
int
kcn_search(int keyc, char * const keyv[], struct kcn_info *ki)
{
	char *uri;
	char startopt[KCN_INFO_NLOCSTRLEN];
	char rcountmaxstr[2];
	const char *country;
	const char *userip;
	size_t n, uriolen, rcountmax;
	int error;

	assert(kcn_info_type(ki) == KCN_LOC_TYPE_DOMAINNAME ||
	    kcn_info_type(ki) == KCN_LOC_TYPE_URI);
	assert(kcn_info_maxnlocs(ki) > 0);

	error = 0;
	uri = kcn_uri_build(KCN_SEARCH_URI_BASE_GOOGLE, keyc, keyv);
	if (uri == NULL)
		return ENOMEM;

	if (kcn_info_maxnlocs(ki) < KCN_SEARCH_GOOGLE_API_RCOUNTMAX)
		rcountmax = kcn_info_maxnlocs(ki);
	else
		rcountmax = KCN_SEARCH_GOOGLE_API_RCOUNTMAX - 1;
	rcountmaxstr[0] = '0' + rcountmax;
	rcountmaxstr[1] = '\0';
	error = kcn_search_opt_puts(&uri, KCN_SEARCH_GOOGLE_API_RCOUNTOPT,
	    rcountmaxstr);
	if (error != 0)
		goto out;

	country = kcn_info_country(ki);
	if (country != NULL) {
		error = kcn_search_opt_puts(&uri,
		    KCN_SEARCH_GOOGLE_API_COUNTRYOPT, country);
		if (error != 0)
			goto out;
	}

	userip = kcn_info_userip(ki);
	if (userip != NULL) {
		error = kcn_search_opt_puts(&uri,
		    KCN_SEARCH_GOOGLE_API_USERIPOPT, userip);
		if (error != 0)
			goto out;
	}

	uriolen = strlen(uri);
	for (n = 0; n < kcn_info_maxnlocs(ki); n += rcountmax) {
		if (n > 0) {
			snprintf(startopt, sizeof(startopt), "%zu", n);
			error = kcn_search_opt_puts(&uri,
			    KCN_SEARCH_GOOGLE_API_STARTOPT, startopt);
			if (error != 0)
				goto out;
		}
		error = kcn_search_one(uri, ki);
		if (error != 0) {
			if (error == ESRCH && kcn_info_nlocs(ki) != 0)
				error = 0;
			break;
		}
		kcn_uri_resize(uri, uriolen);
		if (kcn_info_nlocs(ki) % rcountmax != 0)
			break;
	}
  out:
	kcn_uri_free(uri);
	return error;
}
