PROG= 	kcn
SRCS=	main.c
NOMAN=
#
CPPFLAGS+=-I../jansson-2.5/src
LDADD+=../jansson-2.5/src/.libs/libjansson.a
#
.include <bsd.prog.mk>
