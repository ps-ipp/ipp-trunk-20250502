AC_PREREQ(2.59)

AC_INIT([glueforge], [1.04], [ipp-support@ifa.hawaii.edu])
AC_CONFIG_SRCDIR([glueforge.in])

AM_INIT_AUTOMAKE([1.6 foreign dist-bzip2])
AM_MAINTAINER_MODE

AC_PROG_INSTALL

AC_PROG_PERL_MODULES(
  [PS::IPP::Metadata::Config], ,
  [AC_MSG_ERROR(perl module PS::IPP::Metadata::Config is required)]
)

AC_PROG_PERL_MODULES(
  [Template], ,
  [AC_MSG_ERROR(perl module Template is required)]
)

AC_PATH_PROG([POD2MAN], [pod2man], [missing])
if test "$POD2MAN" = "missing" ; then
  AC_MSG_ERROR([pod2man is required])
fi

dnl --- swallow --no-as-needed and --optmize
IPP_STDOPTS

AC_CONFIG_FILES([
  Makefile
  templates/latex/Makefile
  templates/psdb/Makefile
])
AC_OUTPUT
