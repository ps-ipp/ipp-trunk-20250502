AC_PREREQ(2.61)

AC_INIT([ppImage], [1.1.0], [ipp-support@ifa.hawaii.edu])
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

dnl ------------------------------------------------------------

AC_PATH_PROG([ERRORCODES], [psParseErrorCodes], [missing])
if test "$ERRORCODES" = "missing" ; then
  AC_MSG_ERROR([psParseErrorCodes is required])
fi

PKG_CHECK_MODULES([PSLIB],    [pslib >= 1.0.0])
PKG_CHECK_MODULES([PSMODULE], [psmodules >= 1.0.0])
PKG_CHECK_MODULES([PSPHOT],   [psphot >= 0.8.0]) 
PKG_CHECK_MODULES([PSASTRO],  [psastro >= 0.8.0]) 
PKG_CHECK_MODULES([PPSTATS],  [ppStats >= 1.0.0]) 

IPP_STDOPTS
IPP_STDCFLAGS

IPP_VERSION

AC_SUBST([PPIMAGE_CFLAGS])
AC_SUBST([PPIMAGE_LIBS])

AC_CONFIG_FILES([
  Makefile
  src/Makefile
  Doxyfile
])
AC_OUTPUT
