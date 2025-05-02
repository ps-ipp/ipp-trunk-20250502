#Makefile for db functions of psLib
#
noinst_LTLIBRARIES = libpslibdb.la

libpslibdb_la_CPPFLAGS = $(SRCINC) $(PSLIB_CFLAGS) $(MYSQL_CFLAGS)
libpslibdb_la_SOURCES = \
	psDB.c

EXTRA_DIST = db.i

pkginclude_HEADERS = \
	psDB.h

CLEANFILES = *~ *.bb *.bbg *.da
