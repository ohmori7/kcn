PROG= 	kcn
SRCS=	main.c buf.c uri.c search.c
NOMAN=
#
CPPFLAGS+=-W -Wall -Werror
CPPFLAGS+=-I../jansson-2.5/src
CPPFLAGS+=-I/usr/pkg/include
CFLAGS+=-g
LDADD+=../jansson-2.5/src/.libs/libjansson.a
# generated by `curl-config --libs'.
LDFLAGS+=-L/usr/pkg/lib -L/usr/pkg/lib -Wl,-R/usr/pkg/lib -L/usr/pkg/lib -lcurl -lidn -lssl -lcrypto -lgssapi -lheimntlm -lkrb5 -lhx509 -lcom_err -lcrypto -lasn1 -lwind -lroken -lcrypt -lpthread -lz
#
.include <bsd.prog.mk>
