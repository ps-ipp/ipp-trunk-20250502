#Makefile for astronomy functions of psLib
#
AM_CFLAGS=$(CFLAGS) -DPS_CONFIG_FILE_DEFAULT=\"$(sysconfdir)/pslib/pslib.config\"

noinst_LTLIBRARIES = libpslibastro.la

libpslibastro_la_CPPFLAGS = $(SRCINC) $(PSLIB_CFLAGS) 
libpslibastro_la_SOURCES = \
	psTime.c \
	psCoord.c \
	psSphereOps.c \
        psEarthOrientation.c

EXTRA_DIST = astro.i

pkginclude_HEADERS = \
	psTime.h \
	psCoord.h \
	psSphereOps.h \
	psEarthOrientation.h

CLEANFILES = *~ *.bb *.bbg *.da
