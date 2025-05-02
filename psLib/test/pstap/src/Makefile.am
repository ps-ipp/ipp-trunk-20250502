AM_CPPFLAGS = $(SRCINC) -I$(top_srcdir)/test/tap/src $(PSLIB_CFLAGS)
AM_LDFLAGS = \
	$(top_builddir)/src/libpslib.la  \
	$(top_builddir)/test/tap/src/libtap.la \
	$(PSLIB_LIBS)

TEST_LTLIBS = libpstap.la
libpstap_la_SOURCES = pstap.c
noinst_HEADERS = pstap.h

noinst_LTLIBRARIES = $(TEST_LTLIBS)

