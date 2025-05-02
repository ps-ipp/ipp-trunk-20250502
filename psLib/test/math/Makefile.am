
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
	tap_psHistogram \
	tap_psMD5 \
	tap_psMatrix01 \
	tap_psMatrix02 \
	tap_psMatrix03 \
	tap_psMatrix04 \
	tap_psMatrix05 \
	tap_psMatrix06 \
	tap_psMatrix07 \
	tap_psMatrix08 \
	tap_psPolyFit1D \
	tap_psPolyFit2D \
	tap_psPolyFit3D \
	tap_psPolyFit4D \
	tap_psPolynomial \
	tap_psPolynomialEval1D \
	tap_psPolynomialEval2D \
	tap_psPolynomialEval3D \
	tap_psPolynomialEval4D \
	tap_psPolynomialUtils_Derivatives \
	tap_psSparse \
	tap_psStats00 \
	tap_psStats01 \
	tap_psStats02 \
	tap_psStats03 \
	tap_psStats05 \
	tap_psStats06 \
	tap_psStats07 \
	tap_psStats08 \
	tap_psStats09 \
	tap_psFunc01 \
	tap_psStats_Sample_01 \
	tap_psMatrixVectorArithmetic01 \
	tap_psMatrixVectorArithmetic04 \
	tap_psMinimizePowell \
	tap_psMinimizeLMM \
	tap_psMinimizeLMM_Alt \
	tap_psMinimizeLMM_Trail \
	tap_psSpline1D \
	tap_psPolynomialMD \
	tap_psPolynomialMD_sampleDark

#	tap_psRandom

if BUILD_TESTS
bin_PROGRAMS = $(TEST_PROGS)
TESTS = $(TEST_PROGS)
else
check_PROGRAMS = $(TEST_PROGS)
endif

CLEANFILES = $(check_DATA) temp/* core core.* *~ *.bb *.bbg *.da gmon.out

test: check
	$(top_srcdir)/test/test.pl
