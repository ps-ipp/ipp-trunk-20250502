dnl Process this file with autoconf to produce a configure script.
AC_PREREQ(2.61)

AC_INIT([ppMops], [0.1.1], [ipp-support@ifa.hawaii.edu])
AC_CONFIG_SRCDIR([src])
AC_CONFIG_MACRO_DIR([m4])

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

PKG_CHECK_MODULES([PSLIB], [pslib >= 1.0.0])
PKG_CHECK_MODULES([PSMODULES], [psmodules >= 1.0.0])
CHECK_ZLIB

AC_PATH_PROG([ERRORCODES], [psParseErrorCodes], [missing])
if test "$ERRORCODES" = "missing" ; then
  AC_MSG_ERROR([psParseErrorCodes is required])
fi

dnl Set CFLAGS for build
IPP_STDOPTS
IPP_STDCFLAGS

IPP_VERSION

AC_SUBST([PPMOPS_CFLAGS])
AC_SUBST([PPMOPS_LIBS])

AC_CONFIG_FILES([
  Makefile
  src/Makefile
])

AC_OUTPUT
