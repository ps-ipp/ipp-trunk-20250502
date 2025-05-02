AM_CFLAGS = @ippdb_CFLAGS@

include_HEADERS = ippdb.h
lib_LTLIBRARIES = libippdb.la
libippdb_la_CFLAGS  = $(PSLIB_CFLAGS) $(AM_CFLAGS)
libippdb_la_LIBS    = $(PSLIB_LIBS) $(AM_LIBS)
libippdb_la_LDFLAGS = -release $(PACKAGE_VERSION)
libippdb_la_SOURCES = \
    ippdb.h \
    ippdb.c
