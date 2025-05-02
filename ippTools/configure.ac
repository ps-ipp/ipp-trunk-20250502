AC_PREREQ(2.61)

AC_INIT([ipptools], [1.1.77], [ipp-support@ifa.hawaii.edu])
AC_CONFIG_SRCDIR([autogen.sh])

AM_INIT_AUTOMAKE([1.6 foreign dist-bzip2])
AM_CONFIG_HEADER([src/config.h])
AM_MAINTAINER_MODE

IPP_STDLDFLAGS

AC_LANG(C)
AC_GNU_SOURCE
AC_PROG_CC_C99
AC_PROG_INSTALL
AC_PROG_LIBTOOL

PKG_CHECK_MODULES([PSLIB], [pslib >= 1.1.0])
PKG_CHECK_MODULES([PSMODULES], [psmodules >= 1.1.0])
PKG_CHECK_MODULES([IPPDB], [ippdb >= 1.1.77]) 
PKG_CHECK_MODULES([PPSTAMP], [ppstamp >= 0.1.1]) 

PXTOOLS_CFLAGS="${PSLIB_CFLAGS=} ${PSMODULES_CFLAGS=} ${PPSTAMP_CFLAGS=} ${IPPDB_CFLAGS=}"
PXTOOLS_LIBS="${PSLIB_LIBS=} ${PSMODULES_LIBS=} ${IPPDB_LIBS=}"
AC_SUBST(PXTOOLS_CFLAGS,[$PXTOOLS_CFLAGS])
AC_SUBST(PXTOOLS_LIBS,[$PXTOOLS_LIBS])

dnl check for PSDB support
TMP_CFLAGS=${CFLAGS}
CFLAGS="${CFLAGS=} ${PSLIB_CFLAGS}"

AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
    [[#if !defined HAVE_PSDB
      error: PSDB support is not avaiable
      #endif
    ]])],
  [],
  [AC_MSG_ERROR([psLib was built without PSDB support, bailing out.])]
)

CFLAGS=${TMP_CFLAGS}

dnl --  check for psParseErrorCodes --------------------------------------

AC_PATH_PROG([ERRORCODES], [psParseErrorCodes], [missing])
if test "$ERRORCODES" = "missing" ; then
  AC_MSG_ERROR([psParseErrorCodes is required])
fi

dnl check for perl
AC_PATH_PROG([PERL], [perl], [missing])
if test "$PERL" = "missing" ; then
  AC_MSG_ERROR([perl is required])
fi

AC_PROG_SED

IPP_STDOPTS
IPP_STDCFLAGS

IPPDB_VERSION=`$PKG_CONFIG --modversion ippdb`
AC_DEFINE_UNQUOTED(IPPDB_VERSION, ["$IPPDB_VERSION"], [Version of ippdb])

AC_CONFIG_FILES([
  Makefile
  src/Makefile
  share/Makefile
  scripts/Makefile
  pxtools.pc
])
AC_OUTPUT
