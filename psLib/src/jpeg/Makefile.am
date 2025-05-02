#Makefile for image jpeg functions of psLib
#
noinst_LTLIBRARIES = libpslibjpeg.la

libpslibjpeg_la_CPPFLAGS = $(SRCINC) $(PSLIB_CFLAGS) $(JPEG_CFLAGS)
libpslibjpeg_la_SOURCES = \
	psImageJpeg.c

pkginclude_HEADERS = \
	psImageJpeg.h

CLEANFILES = *~ *.bb *.bbg *.da
