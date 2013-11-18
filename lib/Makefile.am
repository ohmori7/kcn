lib_LIBRARIES = libkcn.a
libkcn_a_SOURCES =							\
	kcn.c kcn_buf.c kcn_token.c kcn_uri.c kcn_info.c kcn_http.c	\
	kcn_google.c kcn_netstat.c kcn_db.c kcn_search.c
noinst_HEADERS =							\
	kcn_buf.h kcn_token.h kcn_uri.h kcn_http.h			\
	kcn_google.h kcn_netstat.h kcn_db.h
include_HEADERS =							\
	kcn.h kcn_info.h kcn_search.h
libkcn_a_CFLAGS = $(CURL_CFLAGS) $(JANSSON_CFLAGS)
