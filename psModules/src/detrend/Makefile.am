noinst_LTLIBRARIES = libpsmodulesdetrend.la

libpsmodulesdetrend_la_CPPFLAGS = $(SRCINC) $(PSMODULES_CFLAGS)
libpsmodulesdetrend_la_LDFLAGS  = -release $(PACKAGE_VERSION)
libpsmodulesdetrend_la_SOURCES  = \
	pmFlatField.c \
	pmFlatNormalize.c \
	pmFringeStats.c \
	pmMaskBadPixels.c \
	pmMaskStats.c \
	pmNonLinear.c \
	pmNewNonLinear.c \
	pmBias.c \
	pmOverscan.c \
	pmDetrendDB.c \
	pmShutterCorrection.c \
	pmDetrendThreads.c \
	pmShifts.c \
	pmDark.c \
	pmRemnance.c \
	pmPattern.c \
	pmPatternIO.c

#	pmSkySubtract.c

pkginclude_HEADERS = \
	pmFlatField.h \
	pmFlatNormalize.h \
	pmFringeStats.h \
	pmMaskBadPixels.h \
	pmMaskStats.h \
	pmNonLinear.h \
	pmNewNonLinear.h \
	pmBias.h \
	pmOverscan.h \
	pmDetrendDB.h \
	pmShutterCorrection.h \
	pmDetrendThreads.h \
	pmShifts.h \
	pmDark.h \
	pmRemnance.h \
	pmPattern.h \
	pmPatternIO.h

#	pmSkySubtract.h

CLEANFILES = *~
