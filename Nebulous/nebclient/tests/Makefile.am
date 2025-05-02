SUBDIRS = tap

TESTS = tests

AM_CPPFLAGS = \
    -I$(top_srcdir)/tests/tap/src \
    -I$(top_srcdir)/src

AM_LDFLAGS = \
    $(top_builddir)/tests/tap/src/libtap.la \
    $(top_builddir)/src/libnebclient.la

check_PROGRAMS = tests
