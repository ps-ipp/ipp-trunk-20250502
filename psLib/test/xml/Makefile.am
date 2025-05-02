AM_CPPFLAGS = $(SRCINC) -I$(top_srcdir)/test/tap/src $(PSLIB_CFLAGS)
AM_LDFLAGS = \
	$(top_builddir)/src/libpslib.la  \
	$(top_builddir)/test/tap/src/libtap.la \
	$(PSLIB_LIBS)

TEST_PROGS = 

if BUILD_TESTS
bin_PROGRAMS = $(TEST_PROGS)
TESTS = $(TEST_PROGS)
else
check_PROGRAMS = $(TEST_PROGS)
endif

EXTRA_DIST = psTime.xml psTime2.xml psTime3.xml psTime4.xml

CLEANFILES = $(check_DATA) temp/* psTest5.xml psTest2.xml psTest.xml psTest3.xml psTest4.xml \
	core core.* *~ *.bb *.bbg *.da gmon.out

test: check
