#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include <curl/curl.h>
#include <jansson.h>

#include "chunk.h"
#include "uri.h"
#include "search.h"

/* XXX: is there any appropriate library defins HTTP response code? */
#define SEARCH_HTTP_OK	200

#define	SEARCH_URI_BASE_GOOGLE						\
	"http://ajax.googleapis.com/ajax/services/search/web?v=1.0&q="

struct search_res {
	enum search_type sr_type;
	size_t sr_maxnlocs;
	size_t sr_nlocs;
	char *sr_locs[1];
};

struct search_res *
search_res_new(enum search_type type, size_t maxnlocs)
{
	struct search_res *sr;

	sr = malloc(offsetof(struct search_res, sr_locs[maxnlocs]));
	if (sr == NULL)
		return NULL;
	sr->sr_type = type;
	sr->sr_maxnlocs = maxnlocs;
	sr->sr_nlocs = 0;
	return sr;
}

void
search_res_destroy(struct search_res *sr)
{
	size_t i;

	for (i = 0; i < sr->sr_nlocs; i++)
		free(sr->sr_locs[i]);
	free(sr);
}

size_t
search_res_nlocs(const struct search_res *sr)
{

	return sr->sr_nlocs;
}

const char *
search_res_loc(const struct search_res *sr, size_t idx)
{

	if (idx >= search_res_nlocs(sr))
		return NULL;
	return sr->sr_locs[idx];
}

static size_t
search_curl_callback(const char *p, size_t size, size_t n, void *c)
{
	size_t totalsize;

	totalsize = size * n;
	if (! chunk_append(c, p, totalsize))
		return 0;
	return totalsize;
}

static char *
search_response_get(const char *uri)
{
	struct chunk c;
	CURL *curl;
	CURLcode curlrc;
	char *p;

	if (uri == NULL)
		return NULL;

	chunk_init(&c);
	curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_URL, uri);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, search_curl_callback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &c);
	curlrc = curl_easy_perform(curl);

	curl_easy_cleanup(curl);
	if (curlrc != CURLE_OK)
		p = NULL;
	else {
		/* XXX: ugly!!! */
		p = c.c_ptr;
		c.c_ptr = NULL;
	}
	chunk_finish(&c);

	return p;
}

/* XXX: this supports google for now. */
int
search(int keyc, char * const keyv[], struct search_res *sr)
{
	char *uri, *res;
	json_error_t jerr;
	json_t *jroot, *jval, *jresdata, *jres, *jloc;
	const char *jlocstr;
	const char *jlockey[] = {
		[SEARCH_TYPE_DOMAINNAME] = "visibleUrl",
		[SEARCH_TYPE_URI] = "url"
	};
	size_t i;
	int error;

	assert(sr->sr_type == SEARCH_TYPE_DOMAINNAME ||
	    sr->sr_type == SEARCH_TYPE_URI);
	assert(sr->sr_maxnlocs > 0);

	error = 0;
	uri = uri_build(SEARCH_URI_BASE_GOOGLE, keyc, keyv);
	if (uri == NULL)
		return ENOMEM;
	res = search_response_get(uri);
	uri_free(uri);
	if (res == NULL)
		return ENETDOWN;

	jroot = json_loads(res, 0, &jerr);
	free(res);
	if (jroot == NULL)
		return EINVAL;

	jval = json_object_get(jroot, "responseStatus");
	if (jval == NULL || ! json_is_integer(jval)) {
		error = EINVAL;
		goto out;
	}
	if (json_integer_value(jval) != SEARCH_HTTP_OK) {
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
		if (i == sr->sr_maxnlocs)
			break;
		jloc = json_object_get(jval, jlockey[sr->sr_type]);
		if (jloc == NULL ||
		    (jlocstr = json_string_value(jloc)) == NULL) {
			error = EINVAL;
			goto out;
		}
		sr->sr_locs[sr->sr_nlocs] = strdup(jlocstr);
		if (sr->sr_locs[sr->sr_nlocs] == NULL) {
			error = ENOMEM;
			goto out;
		}
		++sr->sr_nlocs;
	}
	if (sr->sr_nlocs == 0)
		error = ESRCH;
  out:
	json_decref(jroot);
	return error;
}
