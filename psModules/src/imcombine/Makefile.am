noinst_LTLIBRARIES = libpsmodulesimcombine.la

libpsmodulesimcombine_la_CPPFLAGS = $(SRCINC) $(PSMODULES_CFLAGS)
libpsmodulesimcombine_la_LDFLAGS  = -release $(PACKAGE_VERSION)

libpsmodulesimcombine_la_SOURCES = \
	pmReadoutCombine.c	\
	pmStack.c		\
	pmStackReject.c		\
	pmSubtraction.c		\
	pmSubtractionAnalysis.c	\
	pmSubtractionEquation.c	\
	pmSubtractionIO.c	\
	pmSubtractionKernels.c	\
	pmSubtractionHermitian.c	\
	pmSubtractionDeconvolve.c	\
	pmSubtractionMask.c	\
	pmSubtractionMatch.c	\
	pmSubtractionParams.c	\
	pmSubtractionStamps.c	\
	pmSubtractionThreads.c	\
	pmSubtractionSimple.c   \
	pmPSFEnvelope.c         \
	pmSubtractionVisual.c   \
	pmStackVisual.c

pkginclude_HEADERS = \
	pmImageCombine.h \
	pmReadoutCombine.h	\
	pmStack.h		\
	pmStackReject.h		\
	pmSubtraction.h		\
	pmSubtractionTypes.h		\
	pmSubtractionAnalysis.h	\
	pmSubtractionEquation.h	\
	pmSubtractionIO.h	\
	pmSubtractionKernels.h	\
	pmSubtractionHermitian.h	\
	pmSubtractionDeconvolve.h	\
	pmSubtractionMask.h	\
	pmSubtractionMatch.h	\
	pmSubtractionParams.h	\
	pmSubtractionStamps.h	\
	pmSubtractionThreads.h	\
	pmSubtractionSimple.h   \
	pmPSFEnvelope.h         \
	pmSubtractionVisual.h   \
	pmStackVisual.h

CLEANFILES = *~
