lib_LIBRARIES = libkcn.a libkcnse.a
libkcn_a_SOURCES =							\
	kcn.c kcn_str.c kcn_log.c kcn_token.c kcn_pkt.c	kcn_net.c	\
	kcn_uri.c kcn_socket.c kcn_sockaddr.c kcn_info.c kcn_netstat.c	\
	kcn_db.c kcn_msg.c kcn_eq.c kcn_client.c
libkcnse_a_SOURCES =							\
	kcn_uri.c kcn_httpbuf.c kcn_http.c kcn_google.c
noinst_HEADERS =							\
	kcn_str.h kcn_token.h kcn_uri.h kcn_pkt.h kcn_net.h		\
	kcn_http.h kcn_httpbuf.h kcn_socket.h kcn_sockaddr.h		\
	kcn_time.h kcn_db.h kcn_msg.h kcn_eq.h kcn_client.h
include_HEADERS =							\
	kcn.h kcn_log.h kcn_info.h kcn_netstat.h kcn_google.h
libkcnse_a_CFLAGS = $(CURL_CFLAGS) $(JANSSON_CFLAGS)
