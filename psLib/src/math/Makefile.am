#Makefile for math functions of psLib
#
noinst_LTLIBRARIES = libpslibmath.la

libpslibmath_la_CPPFLAGS = $(SRCINC) $(PSLIB_CFLAGS) $(GSL_CFLAGS)
libpslibmath_la_SOURCES = \
	psUnaryOp.c \
	psBinaryOp.c \
	psBinomialCoeff.c \
	psClip.c \
	psCompare.c \
	psEllipse.c \
	psMathUtils.c \
	psMatrix.c \
	psMD5.c \
	psMinimizeLMM.c \
	psMinimizePowell.c \
	psMinimizePolyFit.c \
	psMixtureModels.c \
	psPolynomial.c \
	psPolynomialMetadata.c \
	psPolynomialUtils.c \
	psPolynomialMD.c \
	psRandom.c \
	psRegion.c \
	psRegionForImage.c \
	psSparse.c \
	psSpline.c \
	psStats.c \
	psHistogram.c \
	psVectorSmooth.c \
	md5.c

EXTRA_DIST = math.i

pkginclude_HEADERS = \
	psUnaryOp.h \
	psBinaryOp.h \
	psBinomialCoeff.h \
	psClip.h \
	psCompare.h \
	psConstants.h \
	psEllipse.h \
	psMathUtils.h \
	psMatrix.h \
	psMD5.h \
	psMinimizeLMM.h \
	psMinimizePowell.h \
	psMinimizePolyFit.h \
	psMixtureModels.h \
	psPolynomial.h \
	psPolynomialMetadata.h \
	psPolynomialUtils.h \
	psPolynomialMD.h \
	psRandom.h \
	psRegion.h \
	psRegionForImage.h \
	psSort.h \
	psSparse.h \
	psSpline.h \
	psStats.h \
	psHistogram.h \
	psVectorSmooth.h

noinst_HEADERS = \
	md5.h

CLEANFILES = *~ *.bb *.bbg *.da
