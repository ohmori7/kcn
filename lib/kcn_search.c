#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include <netinet/in.h>

#include <curl/curl.h>
#include <jansson.h>

#include "kcn.h"
#include "kcn_buf.h"
#include "kcn_uri.h"
#include "kcn_search.h"

/* XXX: is there any appropriate library defins HTTP response code? */
#define KCN_SEARCH_HTTP_OK	200

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
#define KCN_SEARCH_GOOGLE_API_COUNTRYLEN	2		/* ISO 3166-1 */
#define KCN_SEARCH_GOOGLE_API_USERIPOPT		"userip"

struct kcn_search_res {
	enum kcn_loc_type ksr_type;
	size_t ksr_maxnlocs;
	char ksr_country[KCN_SEARCH_GOOGLE_API_COUNTRYLEN + 1];
	char ksr_userip[KCN_INET_ADDRSTRLEN];
	size_t ksr_nlocs;
	char *ksr_locs[1];
};

struct kcn_search_res *
kcn_search_res_new(enum kcn_loc_type type, size_t maxnlocs, const char *country,
    const char *userip)
{
	struct kcn_search_res *ksr;

	ksr = malloc(offsetof(struct kcn_search_res, ksr_locs[maxnlocs]));
	if (ksr == NULL)
		return NULL;
	ksr->ksr_type = type;
	ksr->ksr_maxnlocs = maxnlocs;
	if (country == NULL)
		ksr->ksr_country[0] = '\0';
	else {
		assert(country[0] != '\0');
		ksr->ksr_country[0] = country[0];
		ksr->ksr_country[1] = country[1];
		ksr->ksr_country[2] = '\0';
	}
	if (userip == NULL)
		ksr->ksr_userip[0] = '\0';
	else
		strlcpy(ksr->ksr_userip, userip, sizeof(ksr->ksr_userip));
	ksr->ksr_nlocs = 0;
	if ((country != NULL && ksr->ksr_country == NULL) ||
	    (userip != NULL && ksr->ksr_userip == NULL))
		goto bad;
	return ksr;
  bad:
	kcn_search_res_destroy(ksr);
	return NULL;
}

void
kcn_search_res_destroy(struct kcn_search_res *ksr)
{
	size_t i;

	for (i = 0; i < ksr->ksr_nlocs; i++)
		free(ksr->ksr_locs[i]);
	free(ksr);
}

size_t
kcn_search_res_nlocs(const struct kcn_search_res *ksr)
{

	return ksr->ksr_nlocs;
}

const char *
kcn_search_res_loc(const struct kcn_search_res *ksr, size_t idx)
{

	if (idx >= kcn_search_res_nlocs(ksr))
		return NULL;
	return ksr->ksr_locs[idx];
}

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
	if (curlrc == CURLE_OK)
		p = kcn_buf_get(kb);
	kcn_buf_destroy(kb);
  bad:

	return p;
}

static int
kcn_search_opt_puts(char **urip, const char *key, const char *val)
{
	char *opt;
	size_t optlen;
	bool rc;

	optlen = sizeof('&') + strlen(key) + sizeof('=') + strlen(val) +
	    sizeof('\0');
	opt = malloc(optlen);
	if (opt == NULL)
		return ENOMEM;

	(void)snprintf(opt, optlen, "&%s=%s", key, val);
	rc = kcn_uri_puts(urip, opt);
	free(opt);
	if (rc)
		return 0;
	else
		return ENOMEM;
}

/* XXX: this supports google for now. */
int
kcn_search(int keyc, char * const keyv[], struct kcn_search_res *ksr)
{
	char *uri, *res;
	json_error_t jerr;
	json_t *jroot, *jval, *jresdata, *jres, *jloc;
	const char *jlocstr;
	const char *jlockey[] = {
		[KCN_LOC_TYPE_DOMAINNAME] = "visibleUrl",
		[KCN_LOC_TYPE_URI] = "url"
	};
	char rcountmaxstr[2];
	size_t i, rcountmax;
	int error;

	assert(ksr->ksr_type == KCN_LOC_TYPE_DOMAINNAME ||
	    ksr->ksr_type == KCN_LOC_TYPE_URI);
	assert(ksr->ksr_maxnlocs > 0);

	error = 0;
	uri = kcn_uri_build(KCN_SEARCH_URI_BASE_GOOGLE, keyc, keyv);
	if (uri == NULL)
		return ENOMEM;

	if (ksr->ksr_maxnlocs < KCN_SEARCH_GOOGLE_API_RCOUNTMAX)
		rcountmax = ksr->ksr_maxnlocs;
	else
		rcountmax = KCN_SEARCH_GOOGLE_API_RCOUNTMAX - 1;
	rcountmaxstr[0] = '0' + rcountmax;
	rcountmaxstr[1] = '\0';
	error = kcn_search_opt_puts(&uri, KCN_SEARCH_GOOGLE_API_RCOUNTOPT,
	    rcountmaxstr);
	if (error != 0)
		goto bad;

	if (ksr->ksr_country[0] != '\0') {
		error = kcn_search_opt_puts(&uri,
		    KCN_SEARCH_GOOGLE_API_COUNTRYOPT, ksr->ksr_country);
		if (error != 0)
			goto bad;
	}

	if (ksr->ksr_userip[0] != '\0') {
		error = kcn_search_opt_puts(&uri,
		    KCN_SEARCH_GOOGLE_API_USERIPOPT, ksr->ksr_userip);
		if (error != 0)
			goto bad;
	}

	res = kcn_search_response_get(uri);
	if (res == NULL) {
		error = ENETDOWN;
		goto bad;
	}

	jroot = json_loads(res, 0, &jerr);
	free(res);
	if (jroot == NULL)
		return EINVAL;

	jval = json_object_get(jroot, "responseStatus");
	if (jval == NULL || ! json_is_integer(jval)) {
		error = EINVAL;
		goto out;
	}
	if (json_integer_value(jval) != KCN_SEARCH_HTTP_OK) {
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
		if (i == ksr->ksr_maxnlocs)
			break;
		jloc = json_object_get(jval, jlockey[ksr->ksr_type]);
		if (jloc == NULL ||
		    (jlocstr = json_string_value(jloc)) == NULL) {
			error = EINVAL;
			goto out;
		}
		ksr->ksr_locs[ksr->ksr_nlocs] = strdup(jlocstr);
		if (ksr->ksr_locs[ksr->ksr_nlocs] == NULL) {
			error = ENOMEM;
			goto out;
		}
		++ksr->ksr_nlocs;
	}
	if (ksr->ksr_nlocs == 0)
		error = ESRCH;
  out:
	json_decref(jroot);
  bad:
	kcn_uri_free(uri);
	return error;
}
