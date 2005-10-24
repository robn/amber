dnl amber configure script
dnl $Id$


dnl
dnl autoconf setup
dnl

AC_PREREQ(2.59)

dnl startup
AC_INIT(amber, 0.00, rob@cataclysm.cx)
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
dnl host checks
dnl

dnl eventually in here we'll do some host type checking and set this accordingly
CFLAGS="-DXP_UNIX $CFLAGS"


dnl
dnl header checks
dnl
AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h string.h unistd.h])


dnl
dnl type checks
dnl
AC_C_CONST
AC_TYPE_PID_T


dnl
dnl function checks
dnl
AC_FUNC_REALLOC
AC_FUNC_STAT
AC_FUNC_FORK
AC_CHECK_FUNCS([strerror])


dnl
dnl externals
dnl


dnl
dnl finishing up
dnl
AC_OUTPUT(Makefile \
          js/Makefile \
          js/src/Makefile \
          amber/Makefile \
          modules/Makefile)
