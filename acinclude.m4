dnl KCN local macro for autoconf.

dnl
AC_DEFUN([AC_KCN_DEFINE], [
	$1=yes
	ABOUT_$1="$3"
	AC_DEFINE($1, 1, [Define to $2 if you have $3.])
	AC_MSG_RESULT($1)
])

dnl
AC_DEFUN([AC_KCN_CHECK_C99_VA_ARGS], [
	AC_MSG_CHECKING(if cpp has __VA_ARGS__)
	AC_TRY_COMPILE([#include <stdio.h>],[
#define X(...)	printf(__VA_ARGS__)
	X("hello");
	],
	[AC_KCN_DEFINE(HAVE_C99_VA_ARGS, 1, [standard C99 of __VA_ARGS__])],
	[AC_MSG_RESULT(no)])
])

dnl
AC_DEFUN([AC_KCN_CHECK_GCC_VA_ARGS], [
	AC_MSG_CHECKING(if cpp has GCC extension of __VA_ARGS__)
	AC_TRY_COMPILE([#include <stdio.h>],[
#define X(fmt, ...)	printf(fmt, ## __VA_ARGS__)
	X("hello");
	],
	[AC_KCN_DEFINE(HAVE_GCC_VA_ARGS, 1, [gcc extension of __VA_ARGS__])],
	[AC_MSG_RESULT(no)])
])
