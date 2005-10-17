dnl amber configure script
dnl $Id$


dnl
dnl autoconf setup
dnl

AC_PREREQ(2.59)

dnl startup
AC_INIT(amber, 0.01, rob@cataclysm.cx)
AM_INIT_AUTOMAKE

dnl copyright
AC_COPYRIGHT([Copyright (c) 2005 Robert Norris
This configure script may be copied, distributed and modified under the
terms of the amber license; see COPYING for more details.])

dnl identify the source tree
AC_CONFIG_SRCDIR([amber/amber.c])

dnl output config header
AC_CONFIG_HEADER([config.h])


dnl
dnl build environment
dnl

dnl developer flags
AC_ARG_ENABLE(developer, AC_HELP_STRING([--enable-developer],
                                        [Compile with warnings and debugging symbols]),
              CFLAGS="-Wall -g $CFLAGS")

dnl basic tools
AC_PROG_CC

AC_LIBTOOL_DLOPEN
AC_PROG_LIBTOOL


dnl
dnl header checks
dnl
AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h string.h unistd.h])


dnl
dnl function checks
dnl
AC_FUNC_REALLOC
AC_FUNC_STAT
AC_CHECK_FUNCS([strerror])


dnl
dnl externals
dnl

dnl spidermonkey has many homes
CPPFLAGS="-DXP_UNIX $CPPFLAGS"
AC_CHECK_HEADER(jsapi.h)
if test "x-$ac_cv_header_jsapi_h" != "x-yes" ; then
    for incpath in /usr/include/js /usr/local/include/js /usr/include/smjs ; do
        if test "x-$ac_cv_header_jsapi_h" != "x-yes" ; then
            AC_MSG_CHECKING([for jsapi.h in $incpath])
            save_cppflags="$CPPFLAGS"
            CPPFLAGS="-I$incpath $CPPFLAGS"
            AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <jsapi.h>]])],
                              [AC_MSG_RESULT(yes)
                               AC_DEFINE(HAVE_JSAPI_H,,[Define if you have jsapi.h])
                               ac_cv_header_jsapi_h=yes],
                              [AC_MSG_RESULT(no)
                               CPPFLAGS="$save_cppflags"])
        fi
    done
fi
if test "x-$ac_cv_header_jsapi_h" = "x-yes" ; then
    for lib in js smjs ; do
        if test "x-have_libjs" != "x-yes" ; then
            AC_MSG_CHECKING([for JS_InitStandardClasses in -l$lib])
            save_libs="$LIBS"
            LIBS="-l$lib $LIBS"
            AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <jsapi.h>]],
                                            [[JS_InitStandardClasses(0,0)]])],
                           [AC_MSG_RESULT(yes)
                            have_libjs=yes],
                           [AC_MSG_RESULT(no)
                            LIBS="$save_libs"])
        fi
    done
fi
if test "x-$have_libjs" != "x-yes" ; then
    AC_MSG_ERROR([SpiderMonkey engine not found])
fi


dnl
dnl finishing up
dnl
AC_OUTPUT(Makefile \
          amber/Makefile \
          modules/Makefile)
