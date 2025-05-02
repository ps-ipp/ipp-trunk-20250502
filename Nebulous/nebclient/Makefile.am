# Copyright (C) 2005  Joshua Hoblitt
#
# $Id: Makefile.am,v 1.13 2007-05-04 02:14:04 jhoblitt Exp $

SUBDIRS = src tests

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA= @PACKAGE@.pc

EXTRA_DIST = nebulous.wsdl

if HAVE_DOXYGEN
man_MANS = \
    $(top_builddir)/docs/man/man3/@PACKAGE@.3

docs/man/man3/@PACKAGE@.3:
	$(DOXYGEN)
endif

clean-local:
	-rm -rf docs
