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
	tap_psFits \
	tap_psFitsBlank_00 \
	tap_psFitsImage

if BUILD_TESTS
bin_PROGRAMS = $(TEST_PROGS)
TESTS = $(TEST_PROGS)
else
check_PROGRAMS = $(TEST_PROGS)
endif

CLEANFILES = $(check_DATA) temp/* multi.fits table.fits tmpImages/* core core.* *~ \
	*.bb *.bbg *.da gmon.out

test: check

tests: $(check_PROGRAMS)
	$(top_srcdir)/test/test.pl
