dnl Process this file with autoconf to produce a configure script.
AC_PREREQ(2.61)

AC_INIT([ppSub], [0.1.1], [ipp-support@ifa.hawaii.edu])
AC_CONFIG_SRCDIR([src])

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
PKG_CHECK_MODULES([PSMODULE], [psmodules >= 1.0.0])
PKG_CHECK_MODULES([PPSTATS], [ppStats >= 1.0.0]) 
PKG_CHECK_MODULES([PSPHOT], [psphot >= 0.9.0]) 

AC_PATH_PROG([ERRORCODES], [psParseErrorCodes], [missing])
if test "$ERRORCODES" = "missing" ; then
  AC_MSG_ERROR([psParseErrorCodes is required])
fi

dnl Set CFLAGS for build
IPP_STDOPTS
IPP_STDCFLAGS

IPP_VERSION

AC_SUBST([PPSUB_CFLAGS])
AC_SUBST([PPSUB_LIBS])

AC_CONFIG_FILES([
  Makefile
  src/Makefile
  Doxyfile
])

AC_OUTPUT
