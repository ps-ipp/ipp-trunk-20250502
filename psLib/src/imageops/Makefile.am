#Makefile for image operation functions of psLib
#
noinst_LTLIBRARIES = libpslibimageops.la

libpslibimageops_la_CPPFLAGS = $(SRCINC) $(PSLIB_CFLAGS) $(GSL_CFLAGS)
libpslibimageops_la_SOURCES = \
	psImageBackground.c \
	psImageConvolve.c \
	psImageConvolve2dCache.c \
	psImageCovariance.c \
	psImageGeomManip.c \
	psImageInterpolate.c \
	psImagePixelExtract.c \
	psImagePixelManip.c \
	psImageStats.c \
	psImageStructManip.c \
	psImageMaskOps.c \
	psImageBinning.c \
	psImageMap.c \
	psImageMapFit.c \
	psImagePixelInterpolate.c \
	psImageUnbin.c

EXTRA_DIST = imageops.i

pkginclude_HEADERS = \
	psImageBackground.h \
	psImageConvolve.h \
	psImageCovariance.h \
	psImageGeomManip.h \
	psImageInterpolate.h \
	psImagePixelExtract.h \
	psImagePixelManip.h \
	psImageStats.h \
	psImageStructManip.h \
	psImageMaskOps.h \
	psImageBinning.h \
	psImageMap.h \
	psImageMapFit.h \
	psImagePixelInterpolate.h \
	psImageUnbin.h

CLEANFILES = *~ *.bb *.bbg *.da
