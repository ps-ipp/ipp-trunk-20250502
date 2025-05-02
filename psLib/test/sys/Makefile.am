AM_CPPFLAGS = \
	$(SRCINC) \
	-I$(top_srcdir)/test/tap/src \
	-I$(top_srcdir)/test/pstap/src \
	$(PSLIB_CFLAGS)
AM_LDFLAGS = \
	$(top_builddir)/src/libpslib.la  \
	$(top_builddir)/test/tap/src/libtap.la \
	$(top_builddir)/test/pstap/src/libpstap.la \
	$(PSLIB_LIBS)

TEST_PROGS = \
	tap_psString \
	tap_psTrace \
	tap_psError \
	tap_psMemory \
	tap_psLine \
	tap_psConfigure \
	tap_psStringSubstitute

if BUILD_TESTS
bin_PROGRAMS = $(TEST_PROGS)
TESTS = $(TEST_PROGS)
else
check_PROGRAMS = $(TEST_PROGS)
endif

CLEANFILES = $(check_DATA) temp/* log.txt log2.txt test.fits tst_psTrace02_OUT \
	core core.* *~ *.bb *.bbg *.da gmon.out

test: check
