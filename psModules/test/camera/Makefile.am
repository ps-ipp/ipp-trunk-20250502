AM_CPPFLAGS = \
	$(SRCINC) \
	-I$(top_srcdir)/test/tap/src \
	-I$(top_srcdir)/test/pstap/src \
	$(PSMODULES_CFLAGS)

AM_LDFLAGS = \
	$(top_builddir)/src/libpsmodules.la  \
	$(top_builddir)/test/tap/src/libtap.la \
	$(top_builddir)/test/pstap/src/libpstap.la \
	$(PSMODULES_LIBS)

TEST_PROGS = \
	tap_pmFPA \
	tap_pmFPAReadWrite \
	tap_pmHDUUtils \
	tap_pmFPALevel \
	tap_pmFPAFlags \
	tap_pmFPAExtent \
	tap_pmFPAUtils \
	tap_pmFPAConstruct \
	tap_pmFPAView \
	tap_pmFPAHeader \
	tap_pmFPAMaskW \
	tap_pmFPACellSquish \
	tap_pmFPACopy

if BUILD_TESTS
bin_PROGRAMS = $(TEST_PROGS)
TESTS = $(TEST_PROGS)
else
check_PROGRAMS = $(TEST_PROGS)
endif

CLEANFILES = $(check_DATA) temp/* core core.* *~ *.bb *.bbg *.da gmon.out

test: check
	$(top_srcdir)/test/test.pl
