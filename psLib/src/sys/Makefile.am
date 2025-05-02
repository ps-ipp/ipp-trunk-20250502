#Makefile for sys functions of psLib
#
noinst_LTLIBRARIES = libpslibsys.la

# Force recompilation of psConfigure.c, since it gets the version information
psConfigure.c: psVersionDefinitions.h
psVersionDefinitions.h: psVersionDefinitions.h.in FORCE
	../../etc/pslib-setsvnversion.pl PSLIB psVersionDefinitions.h.in psVersionDefinitions.h
FORCE: ;

libpslibsys_la_CPPFLAGS = $(SRCINC) $(PSLIB_CFLAGS) $(CFITSIO_CFLAGS)
libpslibsys_la_SOURCES = \
	psAbort.c \
	psConfigure.c  \
	psError.c      \
	psErrorCodes.c \
	psLine.c       \
	psLogMsg.c     \
	psMemory.c     \
	psSlurp.c      \
	psString.c     \
	psThread.c     \
	psTrace.c      \
	psType.c       \
	strcasestr.c

EXTRA_DIST = sys.i psErrorCodes.c.in psErrorCodes.h.in psVersionDefinitions.h

BUILT_SOURCES = psErrorCodes.c psVersionDefinitions.h

psErrorCodes.c: ../psErrorCodes_$(PS_LANG).dat psErrorCodes.c.in psErrorCodes.h
	$(top_srcdir)/utils/psParseErrorCodes --data=../psErrorCodes_$(PS_LANG).dat $@

psError.h: psErrorCodes.h

psErrorCodes.h: ../psErrorCodes_$(PS_LANG).dat psErrorCodes.h.in
	$(top_srcdir)/utils/psParseErrorCodes --data=../psErrorCodes_$(PS_LANG).dat $@

pkginclude_HEADERS = \
	psAbort.h \
	psAssert.h \
	psConfigure.h  \
	psError.h      \
	psErrorCodes.h \
	psExit.h       \
	psLine.h       \
	psLogMsg.h     \
	psMemory.h     \
	psMutex.h      \
	psSlurp.h      \
	psString.h     \
	psThread.h     \
	psTrace.h      \
	psType.h

CLEANFILES = *~ *.bb *.bbg *.da
