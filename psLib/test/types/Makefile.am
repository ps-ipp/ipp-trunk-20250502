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
AM_CFLAGS = -DXML_CONFIG_FILE="\"$(top_srcdir)/etc/pslib/psTime.xml\""

TEST_PROGS = \
	tap_psMetadataIterator \
	tap_psListIterator \
	tap_psMetadataConfigRead \
	tap_psMetadataConfigParse_time \
	tap_psMetadataItemCompare \
	tap_psMetadataItemParse \
	tap_psMetadataUpdate \
	tap_psMetadataOverlay \
	tap_psMetadata_printing \
	tap_psMetadata_copying \
	tap_psMetadata_creating \
	tap_psMetadata_manip   \
	tap_psMetadata_polynomials \
	tap_psArray_all \
	tap_psArguments_all \
	tap_psPixels_all \
	tap_psHash_all \
	tap_psBits_all \
	tap_psList_all \
	tap_psLookupTable_all \
	tap_psMetadataConfigWrite \
	tap_psMetadataConfigPrint \
	tap_psMetadataConfigParse \
	tap_psMetadataConfigFormat \
	tap_psMetadataConfig_input \
	tap_psHash_845	\
	tap_psTree

if BUILD_TESTS
bin_PROGRAMS = $(TEST_PROGS)
TESTS = $(TEST_PROGS)
else
check_PROGRAMS = $(TEST_PROGS)
endif

EXTRA_DIST = \
	data/header_1.fits \
	data/header_2.fits \
	data/mdcfgwrt.verified \
	data/test.config \
	data/test1.config \
	data/test2.config \
	data/test3.config \
	data/test4.config \
	data/test5.config

tmp_files = \
	header_1.fits \
	header_2.fits \
	mdcfgwrt.verified \
	test.config \
	test1.config \
	test2.config \
	test3.config \
	test4.config \
	test5.config

CLEANFILES = $(tmp_files) multi.fits table.fits temp/* MDCopy.in MDCopy.out mdcfgwrt.out \
	core core.* *~ *.bb *.bbg *.da gmon.out

tests: $(check_PROGRAMS)
	$(top_srcdir)/test/test.pl

# XXX what is the point of verbatim copying these files?

%.dat: verified/%.dat
	cp $? $@

%.config: $(srcdir)/data/%.config
	cp $? $@

#test.config: $(srcdir)/data/test.config
#	cp $? $@

#test1.config: $(srcdir)/data/test1.config
#	cp $? $@

#test2.config: $(srcdir)/data/test2.config
#	cp $? $@

#test3.config: $(srcdir)/data/test3.config
#	cp $? $@

#test4.config: $(srcdir)/data/test4.config
#	cp $? $@

#test5.config: $(srcdir)/data/test5.config
#	cp $? $@

#header_1.fits: $(srcdir)/data/header_1.fits
#	cp $? $@

#header_2.fits: $(srcdir)/data/header_2.fits
#	cp $? $@

mdcfgwrt.verified: $(srcdir)/data/mdcfgwrt.verified
	cp $? $@

test: check
