noinst_LTLIBRARIES = libpsmodulesextras.la

libpsmodulesextras_la_CPPFLAGS = $(SRCINC) $(PSMODULES_CFLAGS) -I../pslib/
libpsmodulesextras_la_LDFLAGS  = -release $(PACKAGE_VERSION)
libpsmodulesextras_la_SOURCES  = \
	psVectorBracket.c \
	psPipe.c \
	psIOBuffer.c \
	pmKapaPlots.c \
	pmVisual.c \
	pmVisualUtils.c \
	pmThreadTools.c \
	ippStages.c \
        pmCensor.c

pkginclude_HEADERS = \
	psVectorBracket.h \
	psPipe.h \
	psIOBuffer.h \
	pmKapaPlots.h \
	pmVisual.h \
	pmVisualUtils.h \
	pmThreadTools.h \
	ippDiffMode.h \
	ippStages.h \
        pmCensor.h

CLEANFILES = *~
