#Makefile for fits functions of psLib
#
noinst_LTLIBRARIES = libpslibfits.la

libpslibfits_la_CFLAGS = $(SRCINC) $(PSLIB_CFLAGS) $(CFITSIO_CFLAGS) $(AM_CFLAGS)
libpslibfits_la_SOURCES = \
	psFits.c \
	psFitsHeader.c \
	psFitsImage.c \
	psFitsTable.c \
	psFitsTableNew.c \
	psFitsFloat.c \
	psFitsFloatFile.c \
	psFitsScale.c

EXTRA_DIST = fits.i

pkginclude_HEADERS = \
	psFits.h \
	psFitsHeader.h \
	psFitsImage.h \
	psFitsTable.h \
	psFitsTableNew.h \
	psFitsFloat.h \
	psFitsFloatFile.h \
	psFitsScale.h

CLEANFILES = *~ *.bb *.bbg *.da
