dnl aclocal.m4 generated automatically by aclocal 1.4-p4

dnl Copyright (C) 1994, 1995-8, 1999 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl This program is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY, to the extent permitted by law; without
dnl even the implied warranty of MERCHANTABILITY or FITNESS FOR A
dnl PARTICULAR PURPOSE.

dnl Macro: unet_CHECK_TYPE_SIZES
dnl
dnl Check the size of several types and define a valid int16_t and int32_t.
dnl
AC_DEFUN(CHECK_TYPE_SIZES,
[dnl Check type sizes
AC_CHECK_SIZEOF(short)
AC_CHECK_SIZEOF(int)
AC_CHECK_SIZEOF(long)
if test "$ac_cv_sizeof_int" = 2 ; then
  AC_CHECK_TYPE(int16_t, int)
  AC_CHECK_TYPE(u_int16_t, unsigned int)
elif test "$ac_cv_sizeof_short" = 2 ; then
  AC_CHECK_TYPE(int16_t, short)
  AC_CHECK_TYPE(u_int16_t, unsigned short)
else
  AC_MSG_ERROR([No puede encontrar un tipo de 16 bits])
fi
if test "$ac_cv_sizeof_int" = 4 ; then
  AC_CHECK_TYPE(int32_t, int)
  AC_CHECK_TYPE(u_int32_t, unsigned int)
elif test "$ac_cv_sizeof_short" = 4 ; then
  AC_CHECK_TYPE(int32_t, short)
  AC_CHECK_TYPE(u_int32_t, unsigned short)
elif test "$ac_cv_sizeof_long" = 4 ; then
  AC_CHECK_TYPE(int32_t, long)
  AC_CHECK_TYPE(u_int32_t, unsigned long)
else
  AC_MSG_ERROR([No puede encontrar un tipo de 32 bits])
fi
AC_CHECK_SIZEOF(rlim_t)
if test "$ac_cv_sizeof_rlim_t" = 8 ; then
AC_DEFINE(LONG_LONG_RLIM_T)
fi
])

dnl the following 2 macros are based on CHECK_SSL by Mark Ethan Trostler <trostler@juniper.net> 

AC_DEFUN([CHECK_SSL],
[
AC_ARG_ENABLE(ssl,
[AC_HELP_STRING([--enable-ssl=],[enable ssl will check /usr/local/ssl /usr/lib/ssl /usr/ssl /usr/pkg /usr/local /usr])],
[ 
AC_MSG_CHECKING(para openssl)
    for dir in $enableval /usr/local/ssl /usr/lib/ssl /usr/ssl /usr/pkg /usr/local /usr; do
        ssldir="$dir"
        if test -f "$dir/include/openssl/ssl.h"; then
	    AC_MSG_RESULT(encontrado en $ssldir/include/openssl)
            found_ssl="yes";
	    if test ! "$ssldir" = "/usr" ; then
                CFLAGS="$CFLAGS -I$ssldir/include";
  	    fi
            break;
        fi
        if test -f "$dir/include/ssl.h"; then
	    AC_MSG_RESULT(encontrado en $ssldir/include)
            found_ssl="yes";
	    if test ! "$ssldir" = "/usr" ; then
	        CFLAGS="$CFLAGS -I$ssldir/include";
	    fi
            break
        fi
    done
    if test x_$found_ssl != x_yes; then
	AC_MSG_RESULT(no encontrado)
	AC_WARN(deshabilitando soporte SSL)
    else
        CRYPTOLIB="-lssl -lcrypto";
	if test ! "$ssldir" = "/usr" ; then
           LDFLAGS="$LDFLAGS -L$ssldir/lib";
        fi
	AC_DEFINE(USA_SSL)
    fi
],
)
])

AC_DEFUN([CHECK_ZLIB],
[
AC_ARG_ENABLE(ziplinks,
[AC_HELP_STRING([--enable-ziplinks],[enable ziplinks will check /usr/local /usr /usr/pkg])],
[ 
AC_MSG_CHECKING(para zlib)
    for dir in $enableval /usr/local /usr /usr/pkg; do
        zlibdir="$dir"
        if test -f "$dir/include/zlib.h"; then
	    AC_MSG_RESULT(encontrado en $zlibdir)
            found_zlib="yes";
	    if test "$zlibdir" = "/usr" ; then
		CFLAGS="$CFLAGS -DZIP_LINKS";
	    else
	        CFLAGS="$CFLAGS -I$zlibdir/include -DZIP_LINKS";
	    fi
            break;
        fi
    done
    if test x_$found_zlib != x_yes; then
	AC_MSG_RESULT(no encontrado)
	AC_WARN(deshabilitando soporte zlib)
    else
        IRCDLIBS="$IRCDLIBS -lz";
	if test "$zlibdir" != "/usr" ; then
             LDFLAGS="$LDFLAGS -L$zlibdir/lib";
	fi 
        HAVE_ZLIB=yes
    fi
    AC_SUBST(HAVE_ZLIB)
],
)
])

dnl module checking based on Hyb7's module checking code
AC_DEFUN(AC_ENABLE_DYN,
[
AC_CHECK_FUNC(dlopen,, AC_CHECK_LIB(dl, dlopen,IRCDLIBS="$IRCDLIBS -ldl",
[
AC_MSG_WARN(Dynamic linking is not enabled because dlopen was not found) 
AC_DEFINE(STATIC_LINKING)
]))

hold_cflags=$CFLAGS
CFLAGS="$CFLAGS -export-dynamic"
AC_CACHE_CHECK(if we need the -export-dynamic flag, ac_cv_export_dynamic, [
AC_TRY_LINK(, [int i];, ac_cv_export_dynamic=yes, ac_cv_export_dynamic=no)])
if test "$ac_cv_export_dynamic" = "no"; then
CFLAGS=$hold_cflags
fi

AC_CACHE_CHECK(for compiler option to produce PIC,ac_cv_pic,[
if test "$ac_cv_prog_gcc" = "yes"; then
	ac_cv_pic="-fPIC -DPIC -shared"
	case `uname -s` in
		Darwin*[)]
		ac_cv_pic="-bundle -flat_namespace -undefined suppress"
		;;
		HP-UX*[)]
		ac_cv_pic="-fPIC"
		;;
	esac
else
case `uname -s` in
	SunOS*[)]
	ac_cv_pic="-KPIC -DPIC -G"
	;;
esac
fi
])
AC_CACHE_CHECK(if your system prepends an underscore on symbols,ac_cv_underscore,[
cat >uscore.c << __EOF__
int main() {
	return 0;
}
__EOF__
$CC -o uscore $CFLAGS uscore.c 1>&5
if test -z "`strings -a uscore |grep '^_main$'`"; then
ac_cv_underscore=no
else
ac_cv_underscore=yes
fi
rm -f uscore uscore.c
])
if test "$ac_cv_underscore" = "yes"; then
AC_DEFINE(UNDERSCORE)
fi

MODULEFLAGS=$ac_cv_pic
AC_DEFINE(DYNAMIC_LINKING)
])
AC_DEFUN(AC_ENABLE_INET6,[
AC_CACHE_CHECK(if your system has IPv6 support, ac_cv_ip6, [
save_libs="$LIBS"
LIBS="$LIBS $SOCKLIB"
AC_TRY_RUN([
#include <sys/types.h>
#include <sys/socket.h>
int main() {
if (socket(AF_INET6, SOCK_STREAM, 0) < 0)
exit(1);
exit(0);
}
], ac_cv_ip6=yes, ac_cv_ip6=no)
])
if test "$ac_cv_ip6" = "no"; then
AC_MSG_WARN(Your system does not support IPv6 so it will not be enabled)
else
AC_DEFINE(INET6)
fi
LIBS="$save_libs"
])

