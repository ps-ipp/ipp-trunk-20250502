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
	tap_psDB

if BUILD_TESTS
bin_PROGRAMS = $(TEST_PROGS)
TESTS = $(TEST_PROGS)
else
check_PROGRAMS = $(TEST_PROGS)
endif

CLEANFILES = $(check_DATA) temp/* core core.* *~ *.bb *.bbg *.da gmon.out

clean-local:
	mysql test -e "drop table1" 2> /dev/null || \
	mysql test -e "drop table2" 2> /dev/null || \
	mysql test -e "drop table3" 2> /dev/null || \
	mysql test -e "drop table4" 2> /dev/null || \
	mysql test -e "drop table5" 2> /dev/null || \
	mysql test -e "drop table6" 2> /dev/null || \
	mysql test -e "drop table7" 2> /dev/null || \
	mysql test -e "drop table8" 2> /dev/null || \
	mysql test -e "drop table9" 2> /dev/null || \
	mysql test -e "drop table10" 2> /dev/null || \
	mysql test -e "drop table11" 2> /dev/null || \
	true

test: check
