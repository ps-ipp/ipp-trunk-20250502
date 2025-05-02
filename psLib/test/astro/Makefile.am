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
	tap_psSphereOps_all \
	tap_psEarthOrientation_motion \
	tap_psEarthOrientation_corrections \
	tap_psCoord \
	tap_psCoord02 \
	tap_psTime_01 \
	tap_psTime_02 \
	tap_psTime_03 \
	tap_psTime_04

if BUILD_TESTS
bin_PROGRAMS = $(TEST_PROGS)
TESTS = $(TEST_PROGS)
else
check_PROGRAMS = $(TEST_PROGS)
endif

EXTRA_DIST = \
	data/psTime.config.template \
	data/test.psTime.config1.template \
	data/test.psTime.config2.template \
	data/test.psTime.config3.template \
	data/test.psTime.config4.template \
	data/test.ser7.dat

tmp_files = \
	pslib.config \
	test.psTime.config1 \
	test.psTime.config2 \
	test.psTime.config3 \
	test.psTime.config4 \
	test.ser7.dat

CLEANFILES = $(tmp_files) core core.* *~ *.bb *.bbg *.da gmon.out

tests: $(check_PROGRAMS)
	$(top_srcdir)/test/test.pl


pslib.config: $(top_srcdir)/etc/pslib.config.template
	$(PERL) -pe 's|DATADIR|$(top_srcdir)/share|' $? > $@

test.psTime.config1: $(srcdir)/data/test.psTime.config1.template
	cp $? $@

test.psTime.config2: $(srcdir)/data/test.psTime.config2.template
	$(PERL) -pe 's|PREFIX|$(top_srcdir)|' $? > $@

test.psTime.config3: $(srcdir)/data/test.psTime.config3.template
	$(PERL) -pe 's|PREFIX|$(top_srcdir)|' $? > $@

test.psTime.config4: $(srcdir)/data/test.psTime.config4.template
	cp $? $@

test.ser7.dat: $(srcdir)/data/test.ser7.dat
	cp $? $@

test: check
