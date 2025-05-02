#Makefile for mathtypes functions of psLib
#
noinst_LTLIBRARIES = libpslibmathtypes.la

libpslibmathtypes_la_CPPFLAGS = $(SRCINC) $(PSLIB_CFLAGS) 
libpslibmathtypes_la_SOURCES = \
	psImage.c \
	psScalar.c \
	psVector.c

EXTRA_DIST = mathtypes.i

pkginclude_HEADERS = \
	psImage.h \
	psScalar.h \
	psVector.h

CLEANFILES = *~ *.bb *.bbg *.da
