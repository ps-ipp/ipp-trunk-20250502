noinst_LTLIBRARIES = libpsmodulesastrom.la

libpsmodulesastrom_la_CPPFLAGS = $(SRCINC) $(PSMODULES_CFLAGS) -I../pslib/
libpsmodulesastrom_la_LDFLAGS  = -release $(PACKAGE_VERSION)
libpsmodulesastrom_la_SOURCES  = \
	pmAstrometryObjects.c \
	pmAstrometryRegions.c \
	pmAstrometryDistortion.c \
	pmAstrometryUtils.c \
	pmAstrometryModel.c \
	pmAstrometryRefstars.c \
	pmAstrometryWCS.c \
	pmAstrometryVisual.c \
	pmKHcorrect.c

pkginclude_HEADERS = \
	pmAstrometryObjects.h \
	pmAstrometryRegions.h \
	pmAstrometryDistortion.h \
	pmAstrometryUtils.h \
	pmAstrometryModel.h \
	pmAstrometryRefstars.h \
	pmAstrometryWCS.h \
	pmAstrometryVisual.h \
	pmKHcorrect.h

CLEANFILES = *~
