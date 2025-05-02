dnl Copyright (C) 2005  Joshua Hoblitt
dnl
dnl $Id: configure.ac,v 1.21 2008-10-16 23:30:47 jhoblitt Exp $

AC_PREREQ(2.61)

AC_INIT([nebclient], [0.17], [jhoblitt@cpan.org])
AC_CONFIG_SRCDIR([nebulous.wsdl])

dnl this enables the building of libtap
AC_CONFIG_SUBDIRS([tests/tap])

AM_INIT_AUTOMAKE([1.6 foreign dist-bzip2])
AM_CONFIG_HEADER([config.h])
AM_MAINTAINER_MODE

AC_LANG(C)
AC_PROG_CC_C99
AC_PROG_INSTALL
AC_PROG_LIBTOOL

AC_ARG_ENABLE(doxygen,
  [AS_HELP_STRING(--enable-doxygen ,enable manpage generation)],
  [AC_MSG_RESULT(doxygen enabled)
    AC_PATH_PROG([DOXYGEN], [doxygen], [])
  ],
  [AC_MSG_RESULT([doxygen disabled])
    doxygen=off
  ]
)
AM_CONDITIONAL([HAVE_DOXYGEN], test -n "$DOXYGEN" -a "x$doxygen" != "xoff")

dnl needed by doxygen
AC_PATH_PROG([PERL], [perl], [missing])
if test "$PERL" = "missing" ; then
  AC_MSG_ERROR([perl is required])
fi

AM_CFLAGS="-Wall -fno-strict-aliasing"
AC_SUBST([AM_CFLAGS])

dnl default install prefix
AC_PREFIX_DEFAULT([/usr])

AC_CONFIG_FILES([
  Makefile
  nebclient.pc
  Doxyfile
  src/Makefile
  tests/Makefile
])
AC_OUTPUT
