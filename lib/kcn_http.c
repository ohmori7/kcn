#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>

#include <curl/curl.h>

#include "kcn.h"
#include "kcn_buf.h"
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
kcn_http_curl_callback(const char *p, size_t size, size_t n, void *kb)
{
	size_t totalsize;

	totalsize = size * n;
	if (! kcn_buf_append(kb, p, totalsize))
		return 0;
	return totalsize;
}

char *
kcn_http_response_get(const char *uri)
{
	struct kcn_buf *kb;
	CURL *curl;
	CURLcode cc;
	char *p;

	assert(uri != NULL);
	p = NULL;
	kb = kcn_buf_new();
	if (kb == NULL)
		goto bad;

	curl = curl_easy_init();
	if (curl == NULL)
		goto bad;
	curl_easy_setopt(curl, CURLOPT_URL, uri);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, kcn_http_curl_callback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, kb);
	cc = curl_easy_perform(curl);
	curl_easy_cleanup(curl);

	switch (cc) {
	case CURLE_OK:
		if (kcn_buf_append(kb, "\0", 1))
			p = kcn_buf_get(kb);
		break;
	default:
		errno = kcn_http_curl_code2errno(cc);
		break;
	}

  bad:
	kcn_buf_destroy(kb);
	return p;
}

void
kcn_http_response_free(char *p)
{

	if (p == NULL)
		return;
	free(p);
}
