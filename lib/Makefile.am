lib_LIBRARIES = libkcn.a
libkcn_a_SOURCES =							\
	kcn.c kcn_buf.c kcn_uri.c kcn_info.c kcn_http.c kcn_search.c
noinst_HEADERS =							\
	kcn.h kcn_buf.h kcn_uri.h kcn_http.h
include_HEADERS =							\
	kcn_info.h kcn_search.h
libkcn_a_CFLAGS = $(CURL_CFLAGS) $(JANSSON_CFLAGS)
