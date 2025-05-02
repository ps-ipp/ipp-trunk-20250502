#Makefile for xml functions of psLib
#
noinst_LTLIBRARIES = libpslibxml.la

libpslibxml_la_CPPFLAGS = $(SRCINC) $(PSLIB_CFLAGS) $(XML_CFLAGS)
libpslibxml_la_SOURCES = \
	psXML.c

EXTRA_DIST = xml.i

pkginclude_HEADERS = \
	psXML.h

CLEANFILES = *~ *.bb *.bbg *.da
