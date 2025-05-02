AC_PREREQ(2.59)

AC_INIT([ippConfig], [1.1.0], [ipp-support@ifa.hawaii.edu])
AC_CONFIG_SRCDIR([ipprc.config.in])

AM_INIT_AUTOMAKE([1.6 foreign dist-bzip2])
AM_MAINTAINER_MODE

AM_CONDITIONAL([HAVE_GENTOO], [test -f /etc/gentoo-release])
AM_CONDITIONAL([HAVE_LINUX], [test -f /etc/os-release])

AC_PROG_INSTALL

AC_CONFIG_FILES([
  Makefile
  recipes/Makefile
  isp/Makefile
  gpc1/Makefile
  gpc1-tdi/Makefile
  gpc2/Makefile
  cfh12k/Makefile
  megacam/Makefile
  mosaic2/Makefile
  simple/Makefile
  simmosaic/Makefile
  simtest/Makefile
  vysos5/Makefile
  vysos20/Makefile
  tc3/Makefile
  skycell/Makefile
  sdssmosaic/Makefile
  sdss/Makefile
  esowfi/Makefile
  lbc_red/Makefile
  lulin/Makefile
  ssp/Makefile
  uh8k/Makefile
  suprime/Makefile
  hsc/Makefile
])
AC_OUTPUT
