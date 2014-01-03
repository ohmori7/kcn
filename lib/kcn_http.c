#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>

#include <curl/curl.h>

#include "kcn.h"
#include "kcn_httpbuf.h"
#include "kcn_http.h"

static int
kcn_http_curl_code2errno(CURLcode cc)
{

	switch (cc) {
	case CURLE_OUT_OF_MEMORY:	return ENOMEM;
	case CURLE_OPERATION_TIMEOUTED:	return ETIMEDOUT;
	default:			return ENETDOWN;
	}
}

static size_t
kcn_http_curl_callback(const char *p, size_t size, size_t n, void *khb)
{
	size_t totalsize;

	totalsize = size * n;
	if (totalsize < size)
		return 0;
	if (! kcn_httpbuf_append(khb, p, totalsize))
		return 0;
	return totalsize;
}

char *
kcn_http_response_get(const char *uri)
{
	struct kcn_httpbuf *khb;
	CURL *curl;
	CURLcode cc;
	char *p;

	assert(uri != NULL);
	p = NULL;
	khb = kcn_httpbuf_new();
	if (khb == NULL)
		goto bad;

	curl = curl_easy_init();
	if (curl == NULL)
		goto bad;
	curl_easy_setopt(curl, CURLOPT_URL, uri);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, kcn_http_curl_callback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, khb);
	cc = curl_easy_perform(curl);
	curl_easy_cleanup(curl);

	switch (cc) {
	case CURLE_OK:
		if (kcn_httpbuf_append(khb, "\0", 1))
			p = kcn_httpbuf_get(khb);
		break;
	default:
		errno = kcn_http_curl_code2errno(cc);
		break;
	}

  bad:
	kcn_httpbuf_destroy(khb);
	return p;
}

void
kcn_http_response_free(char *p)
{

	if (p == NULL)
		return;
	free(p);
}
