dnl Process this file with autoconf to produce a configure script.
AC_PREREQ(2.61)

AC_INIT([ppstamp], [0.1.1], [ipp-support@ifa.hawaii.edu])
AC_CONFIG_SRCDIR([src])
AC_CONFIG_SRCDIR([scripts])

AM_INIT_AUTOMAKE([1.6 foreign dist-bzip2])
AM_CONFIG_HEADER([src/config.h])
AM_MAINTAINER_MODE

IPP_STDLDFLAGS

AC_LANG(C)
AC_GNU_SOURCE
AC_PROG_CC_C99
AC_PROG_INSTALL
AC_PROG_LIBTOOL
AC_SYS_LARGEFILE

dnl ------------------------------------------------------------

AC_PATH_PROG([ERRORCODES], [psParseErrorCodes], [missing])
if test "$ERRORCODES" = "missing" ; then
  AC_MSG_ERROR([psParseErrorCodes is required])
fi


PKG_CHECK_MODULES([PSLIB], [pslib >= 1.0.0])
PKG_CHECK_MODULES([PSMODULE], [psmodules >= 1.0.0])

OHANA_CONFIG=`which ohana-config`

dnl get CFLAGS for ohana
AC_MSG_NOTICE([libohana cflags info supplied by ohana-config])
AC_MSG_CHECKING([libohana cflags])
OHANA_CFLAGS="`${OHANA_CONFIG} --cflags`"
AC_MSG_RESULT([${OHANA_CFLAGS}])

dnl get LDFLAGS for ohana
AC_MSG_NOTICE([libohana ldflags info supplied by ohana-config])
AC_MSG_CHECKING([libohana ldflags])
OHANA_LIBS="`${OHANA_CONFIG} --libs`"
AC_MSG_RESULT([${OHANA_LIBS}])

dnl Set CFLAGS for build
IPP_STDOPTS
IPP_STDCFLAGS

PPSTAMP_CFLAGS="${PSLIB_CFLAGS=} ${PSMODULE_CFLAGS=} ${OHANA_CFLAGS}"
PPSTAMP_LIBS="${PSLIB_LIBS=} ${PSMODULE_LIBS=} ${OHANA_LIBS}"
echo "PPSTAMP_CFLAGS: $PPSTAMP_CFLAGS"
echo "PPSTAMP_LIBS: $PPSTAMP_LIBS"

AC_SUBST([PPSTAMP_CFLAGS])
AC_SUBST([PPSTAMP_LIBS])

AC_CONFIG_FILES([
  Makefile
  src/Makefile
  scripts/Makefile
  ppstamp.pc
])

AC_OUTPUT
