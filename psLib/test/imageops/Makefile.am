
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
	tap_psImageShift \
	tap_psImageGeomManip \
	tap_psImagePixelManip \
	tap_psImageSmooth \
	tap_psImageSmoothMask_Threaded \
	tap_psImageSmooth_PreAlloc \
	tap_psImageStructManip \
	tap_psImageConvolve \
	tap_psImageConvolve2 \
	tap_psImageConvolve2dCache \
	tap_psImagePixelExtract \
	tap_psImageInterpolate2 \
	tap_psImageInterpolate3 \
	tap_psImageMap \
	tap_psImageMapFit \
	tap_psImageMapFit2 \
	tap_psImageMaskOps \
	tap_psImageCovariance

#	tap_psImageShiftKernel
#	tap_psImageInterpolate

if BUILD_TESTS
bin_PROGRAMS = $(TEST_PROGS)
TESTS = $(TEST_PROGS)
else
check_PROGRAMS = $(TEST_PROGS)
endif

CLEANFILES = $(check_DATA) temp/* fOut.fits sOut.fits fBiOut.fits sBiOut.fits \
	core core.* *~ *.bb *.bbg *.da gmon.out

test: check

noinst_PROGRAMS = convolutionBench
convolutionBench_SOURCES = convolutionBench.c

