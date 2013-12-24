# automatic configuration script (a.k.a. autoconf) for KCN programs.
#
# please process configure.ac to produce configure with following commands:
# % aclocal
# % autoheader
# % automake --add-missing --copy
# % autoconf

#
dnl file="${srcdir}/VERSION"
dnl VERSION=`sed -ne 's/.*Version \([0-9]\.[0-9]\.[0-9]\)/\1/p' $file`

#
AC_PREREQ([2.59])
AC_INIT([kcn], [0.0.0], [ohmori@tottori-u.ac.jp])
AC_CANONICAL_TARGET([])
AC_CONFIG_SRCDIR([README])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE

# Default values of options
AC_ARG_ENABLE(ipv6,
	[  --disable-ipv6	Disable IPv6 support])

#
AC_PROG_MAKE_SET
AC_PROG_CPP
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_GCC_TRADITIONAL
dnl
dnl expand AC_PROG_OBJC for old autoconf tools
dnl
AC_CHECK_TOOL([OBJC], [gcc])
AC_SUBST(OBJC)
AC_SUBST(OBJCFLAGS)
dnl AC_PROG_OBJC
AC_PROG_RANLIB
AC_PROG_INSTALL
AC_CHECK_TOOL(AR, ar)
AC_CHECK_TOOL(RANLIB, ranlib, :)
AC_KCN_CHECK_C99_VA_ARGS
AC_KCN_CHECK_GCC_VA_ARGS

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([							\
	sys/types.h sys/socket.h sys/un.h 				\
	net/if_var.h							\
	netinet/in.h netinet6/in6.h					\
	stdbool.h							\
	])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_CHECK_TYPES([
	sa_family_t, socklen_t, in_port_t],
	[],
	[],
	[
	#ifdef HAVE_SYS_SOCKET_H
	#include <sys/socket.h>
	#endif /* HAVE_SYS_SOCKET_H */
	#ifdef HAVE_NETINET_IN_H
	#include <netinet/in.h>
	#endif /* HAVE_NETINET_IN_H */
	])
AC_CHECK_TYPES([
	struct sockaddr_in,
	struct sockaddr_in6,
	struct sockaddr_storage,
	struct sockaddr_un],
	[],
	[],
	[
	#ifdef HAVE_SYS_TYPES_H
	#include <sys/types.h>
	#endif /* HAVE_SYS_TYPES_H */
	#ifdef HAVE_SYS_SOCKET_H
	#include <sys/socket.h>
	#endif /* HAVE_SYS_SOCKET_H */
	#ifdef HAVE_SYS_UN_H
	#include <sys/un.h>
	#endif /* HAVE_SYS_UN_H */
	#ifdef HAVE_NETINET_IN_H
	#include <netinet/in.h>
	#endif /* HAVE_NETINET_IN_H */
	#ifdef HAVE_NETINET6_IN6_H
	#include <netinet6/in6.h>
	#endif /* HAVE_NETINET6_IN6_H */
	])
AC_CHECK_MEMBERS([
	struct sockaddr.sa_len,
	struct sockaddr_in.sin_len,
	struct sockaddr_in.sin_zero,
	struct sockaddr_in6.sin6_len,
	struct sockaddr_in6.sin6_scope_id,
	struct sockaddr_un.sun_len,
	struct sockaddr_storage.ss_len],
	[],
	[],
	[
	#ifdef HAVE_SYS_TYPES_H
	#include <sys/types.h>
	#endif /* HAVE_SYS_TYPES_H */
	#ifdef HAVE_SYS_SOCKET_H
	#include <sys/socket.h>
	#endif /* HAVE_SYS_SOCKET_H */
	#ifdef HAVE_SYS_UN_H
	#include <sys/un.h>
	#endif /* HAVE_SYS_UN_H */
	#include <net/if.h>
	#ifdef HAVE_NET_IF_VAR_H
	#include <net/if_var.h>
	#endif /* HAVE_NET_IF_VAR_H */
	#include <netinet/in.h>
	#ifdef HAVE_NETINET6_IN6_H
	#include <netinet6/in6.h>
	#endif /* HAVE_NETINET6_IN6_H */
	])

# Checks for library functions.
AC_CHECK_FUNCS([strlcpy strlcat])

#
PKG_CHECK_MODULES(CURL, libcurl)
AC_SUBST(CURL_CFLAGS)
AC_SUBST(CURL_LIBS)
AC_KCN_CHECK_LIB_PATH(CRUL_LIBS)
#
PKG_CHECK_MODULES(JANSSON, jansson)
AC_SUBST(JANSSON_CFLAGS)
AC_SUBST(JANSSON_LIBS)
AC_KCN_CHECK_LIB_PATH(JANSSON_LIBS)

#
AC_KCN_CHECK_LIB(event, event_init, event.h)
AC_SUBST(EVENT_CFLAGS)
AC_SUBST(EVENT_LIBS)
AC_KCN_CHECK_LIB_PATH(EVENT_LIBS)

#
# Check for IPv6 support
#
AC_MSG_CHECKING(IPv6 support)
if test "${enable_ipv6}" = "no"; then
	AC_MSG_RESULT(disabled)
else
	AC_TRY_COMPILE([
	#ifdef HAVE_SYS_TYPES_H
	#include <sys/types.h>
	#endif /* HAVE_SYS_TYPES_H */
	#ifdef HAVE_SYS_SOCKET_H
	#include <sys/socket.h>
	#endif /* HAVE_SYS_SOCKET_H */
	#ifdef HAVE_NETINET_IN_H
	#include <netinet/in.h>
	#endif /* HAVE_NETINET_IN_H */
	#ifdef HAVE_NETINET6_IN6_H
	#include <netinet6/in6.h>
	#endif /* HAVE_NETINET6_IN6_H */
	],
	[return (int)&((struct sockaddr_in6 *)0)->sin6_family;],
	[AC_MSG_RESULT(yes)
	 AC_DEFINE([HAVE_IPV6],1,[Define to 1 if you have an IPv6 support.])],
	 AC_MSG_RESULT(no))
fi

#
CPPFLAGS="$CPPFLAGS -I../lib"
CFLAGS="$CFLAGS -W -Wall -Werror -Wmissing-declarations"
KCN_LIBS="-L../lib -lkcn"
KCNSE_LIBS="-lkcnse"

AC_SUBST(KCN_LIBS)
AC_SUBST(KCNSE_LIBS)
AC_CONFIG_FILES([Makefile lib/Makefile key2loc/Makefile kcndbd/Makefile])
AC_OUTPUT
