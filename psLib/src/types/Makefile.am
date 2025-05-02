#Makefile for types functions of psLib
#
noinst_LTLIBRARIES = libpslibtypes.la

libpslibtypes_la_CPPFLAGS = $(SRCINC) $(PSLIB_CFLAGS) $(CFITSIO_CFLAGS)
libpslibtypes_la_SOURCES = \
	psArray.c \
	psBits.c \
	psHash.c \
	psList.c \
	psLookupTable.c \
	psMetadata.c \
	psMetadataConfig.c \
	psMetadataItemParse.c \
	psMetadataItemCompare.c \
	psMetadataHeader.c \
	psPixels.c \
	psArguments.c	\
	psTree.c

EXTRA_DIST = types.i

pkginclude_HEADERS = \
	psArray.h \
	psBits.h \
	psHash.h \
	psList.h \
	psLookupTable.h \
	psMetadata.h \
	psMetadataConfig.h \
	psMetadataItemParse.h \
	psMetadataItemCompare.h \
	psMetadataHeader.h \
	psPixels.h \
	psArguments.h	\
	psTree.h

CLEANFILES = *~ *.bb *.bbg *.da
