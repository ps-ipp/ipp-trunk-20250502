noinst_LTLIBRARIES = libpsmodulesconcepts.la

libpsmodulesconcepts_la_CPPFLAGS = $(SRCINC) $(PSMODULES_CFLAGS)
libpsmodulesconcepts_la_LDFLAGS  = -release $(PACKAGE_VERSION)
libpsmodulesconcepts_la_SOURCES  = \
	pmConcepts.c \
	pmConceptsAverage.c \
	pmConceptsRead.c \
	pmConceptsWrite.c \
	pmConceptsCopy.c \
	pmConceptsStandard.c \
	pmConceptsPhotcode.c \
	pmConceptsUpdate.c

pkginclude_HEADERS = \
	pmConcepts.h \
	pmConceptsAverage.h \
	pmConceptsRead.h \
	pmConceptsWrite.h \
	pmConceptsCopy.h \
	pmConceptsStandard.h \
	pmConceptsPhotcode.h \
	pmConceptsUpdate.h

CLEANFILES = *~
