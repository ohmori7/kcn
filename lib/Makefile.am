AM_CFLAGS = -I../libkcn # XXX
lib_LIBRARIES = libkcn.a
libkcn_a_SOURCES =							\
	kcn_buf.c kcn_uri.c kcn_search.c
noinst_HEADERS =							\
	kcn.h kcn_buf.h kcn_uri.h kcn_search.h
libkcn_a_CFLAGS = $(CURL_CFLAGS) $(JANSSON_CFLAGS)
