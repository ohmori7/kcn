dnl KCN local macro for autoconf.

dnl
AC_DEFUN([AC_KCN_DEFINE], [
	$1=yes
	ABOUT_$1="$3"
	AC_DEFINE($1, 1, [Define to $2 if you have $3.])
	AC_MSG_RESULT(yes)
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

dnl AC_KCN_CHECK_LIB(library, function, header)
AC_DEFUN([AC_KCN_CHECK_LIB], [
	AC_ARG_WITH($1, [  --with-$1=PREFIX     $1 library],,[with_$1=yes])
	AC_CHECK_LIB($1, $2, [needldflag=no], [
		AC_MSG_RESULT(no)
		needflag=yes
		eval with=\$with_$1
		if test x"$with" = xyes; then
			with='/usr /usr/local /usr/pkg'
		fi
		ldflags="$LDFLAGS"
		founddir=''
		for dir in $with; do
			unset ac_cv_lib_$1_$2
			LDFLAGS="$LDFLAGS -L$dir/lib"
			AC_CHECK_LIB($1, $2, [founddir="$dir" break],
			    [AC_MSG_RESULT(no)])
		done
		LDFLAGS="$ldflags"
		if test x"$founddir" = x; then
			AC_MSG_ERROR([$1 library not found.])
		fi
	])
	dnl
	name=`echo $1 | tr 'a-z' 'A-Z'`
	cflagsname="${name}_CFLAGS"
	libsname="${name}_LIBS"
	if test "$needldflag" = "yes"; then
		eval $cflagsname=\"-I${founddir}/include\"
		eval $libsname=\"-Wl,-R${founddir}/lib -L${founddir}/lib \"
	fi
	eval $libsname=\"\$$libsname-l$1\"
	dnl
	cflags=$CFLAGS
	AC_CHECK_HEADER([$3],,
	    [AC_MSG_ERROR(lib$1 developing environment not found.)])
	CFLAGS=$cflags
])
