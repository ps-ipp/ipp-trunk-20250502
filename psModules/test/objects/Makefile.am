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
	tap_pmSourceFitModel

#	tap_pmPeaks \
#	tap_pmGrowthCurve \
#	tap_pmMoments \
#	tap_pmSource \
#	tap_pmModel \
#	tap_pmModelUtils \
#	tap_pmModelClass \
#	tap_pmModel_CentralPixel \
#	tap_pmModel_CentralPixel_v2 \
#	tap_pmModel_SET_FWHM \
#	tap_pmPSF \
#	tap_pmTrend2D \
#	tap_pmResiduals \
#	tap_pmSourceExtendedPars \
#	tap_pmSourceSky \
#	tap_pmSourceMoments \
#	tap_pmSourceIO_PS1_DEV_0 \
#	tap_pmSourceIO_PS1_DEV_1 \
#	tap_pmSourceIO_SMPDATA \
#	tap_pmSourceContour \
#	tap_pmSourceUtils \
#	tap_pmSourceFitSet \
#	tap_pmPSF_IO \
#	tap_pmSourceIO_SMPDATA

if BUILD_TESTS
bin_PROGRAMS = $(TEST_PROGS)
TESTS = $(TEST_PROGS)
else
check_PROGRAMS = $(TEST_PROGS)
endif

CLEANFILES = $(check_DATA) temp/* core core.* *~ *.bb *.bbg *.da gmon.out

test: check
	$(top_srcdir)/test/test.pl

# Removed
#	tap_pmSourcePhotometry
#	tap_pmGrowthCurve
#	tap_pmSourceFitModel
#	tap_pmSourceFitModel_Delta
