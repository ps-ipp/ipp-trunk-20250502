#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include <psmodules.h>
#include "tap.h"
#include "pstap.h"
/* STATUS:
    All functions are tested.
        pmTrend2DFit(): Must test the PM_TREND_MAP case.
*/

#define NUM_ROWS 8
#define NUM_COLS 16
#define ERR_TRACE_LEVEL 0
#define TEST_FLOATS_EQUAL(X, Y) (abs(X - Y) < 0.01)

int main(int argc, char* argv[])
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    psTraceSetLevel("err", ERR_TRACE_LEVEL);
    plan_tests(91);

    // ------------------------------------------------------------------------
    // Test pmTrend2DAlloc()
    // Call pmTrend2DAlloc() with NULL psImage input parameter (PM_TREND_MAP)
    {
        psMemId id = psMemGetId();
        psImage *img = psImageAlloc(NUM_COLS, NUM_ROWS, PS_TYPE_F32);
        psStats *stats = psStatsAlloc(PS_STAT_SAMPLE_MEAN);
        pmTrend2D *trend = pmTrend2DAlloc(PM_TREND_MAP, NULL, 2, 2, stats);
        ok(trend == NULL, "pmTrend2DAlloc() returned NULL with NULL psImage input parameter");
        psFree(img);
        psFree(stats);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmTrend2DAlloc() with NULL psImage input parameter (PM_TREND_POLY_ORD)
    {
        psMemId id = psMemGetId();
        psImage *img = psImageAlloc(NUM_COLS, NUM_ROWS, PS_TYPE_F32);
        psStats *stats = psStatsAlloc(PS_STAT_SAMPLE_MEAN);
        pmTrend2D *trend = pmTrend2DAlloc(PM_TREND_POLY_ORD, NULL, 2, 2, stats);
        ok(trend != NULL, "pmTrend2DAlloc() returned NULL with NULL psImage input parameter");
        psFree(img);
        psFree(stats);
        psFree(trend);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmTrend2DAlloc() with NULL psStats input parameter
    {
        psMemId id = psMemGetId();
        psImage *img = psImageAlloc(NUM_COLS, NUM_ROWS, PS_TYPE_F32);
        psStats *stats = psStatsAlloc(PS_STAT_SAMPLE_MEAN);
        pmTrend2D *trend = pmTrend2DAlloc(PM_TREND_POLY_ORD, img, 2, 2, NULL);
        ok(trend == NULL, "pmTrend2DAlloc() returned NULL with NULL psStats input parameter");
        psFree(img);
        psFree(stats);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmTrend2DAlloc() with unallowed pmTrend2DMode
    // XXX: We skip this test because pmTrend2DAlloc() aborts.
    if (0) {
        psMemId id = psMemGetId();
        psImage *img = psImageAlloc(NUM_COLS, NUM_ROWS, PS_TYPE_F32);
        psStats *stats = psStatsAlloc(PS_STAT_SAMPLE_MEAN);
        pmTrend2D *trend = pmTrend2DAlloc(999, img, 2, 2, stats);
        ok(trend == NULL, "pmTrend2DAlloc() returned NULL with unallowed pmTrend2DMode");
        psFree(img);
        psFree(stats);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmTrend2DAlloc() with acceptable input parameters
    {
        psMemId id = psMemGetId();
        psImage *img = psImageAlloc(NUM_COLS, NUM_ROWS, PS_TYPE_F32);
        psStats *stats = psStatsAlloc(PS_STAT_SAMPLE_MEAN);

        // Call pmTrend2DAlloc() with PM_TREND_POLY_ORD
        pmTrend2D *trend = pmTrend2DAlloc(PM_TREND_POLY_ORD, img, 2, 4, stats);
        ok(trend != NULL && psMemCheckTrend2D(trend), 
          "pmTrend2DAlloc() returned non-NULL with acceptable input parameters");
        ok(trend->map == NULL, "pmTrend2DAlloc() set trend->map to NULL");
        ok(trend->mode == PM_TREND_POLY_ORD, "pmTrend2DAlloc() set trend->mode correctly");
        ok(trend->stats == stats, "pmTrend2DAlloc() set trend->stats correctly");
        ok(trend->poly != NULL && trend->poly->type == PS_POLYNOMIAL_ORD, "pmTrend2DAlloc() set trend->poly->type correctly");
        ok(trend->poly->type == PS_POLYNOMIAL_ORD, "pmTrend2DAlloc() set trend->poly->type correctly");
        ok(trend->poly->nX == 2, "pmTrend2DAlloc() set trend->poly->nX correctly");
        ok(trend->poly->nY == 4, "pmTrend2DAlloc() set trend->poly->nY correctly");
        psFree(trend);

        // Call pmTrend2DAlloc() with PM_TREND_POLY_CHEB
        trend = pmTrend2DAlloc(PM_TREND_POLY_CHEB, img, 2, 4, stats);
        ok(trend != NULL && psMemCheckTrend2D(trend), 
          "pmTrend2DAlloc() returned non-NULL with acceptable input parameters");
        ok(trend->map == NULL, "pmTrend2DAlloc() set trend->map to NULL");
        ok(trend->mode == PM_TREND_POLY_CHEB, "pmTrend2DAlloc() set trend->mode correctly");
        ok(trend->stats == stats, "pmTrend2DAlloc() set trend->stats correctly");
        ok(trend->poly != NULL && trend->poly->type == PS_POLYNOMIAL_CHEB, "pmTrend2DAlloc() set trend->poly->type correctly");
        ok(trend->poly->type == PS_POLYNOMIAL_CHEB, "pmTrend2DAlloc() set trend->poly->type correctly");
        ok(trend->poly->nX == 2, "pmTrend2DAlloc() set trend->poly->nX correctly");
        ok(trend->poly->nY == 4, "pmTrend2DAlloc() set trend->poly->nY correctly");
        psFree(trend);

        // Create a new pmTrend with PM_TREND_MAP
        trend = pmTrend2DAlloc(PM_TREND_MAP, img, 2, 4, stats);
        ok(trend->map != NULL && psMemCheckImageMap(trend->map), 
           "pmTrend2DAlloc() set trend->map correctly");
        psFree(trend);

        psFree(img);
        psFree(stats);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ------------------------------------------------------------------------
    // Test pmTrend2DNoImageAlloc()
    // Call pmTrend2DNoImageAlloc() with NULL psImageBinning input parameter
    {
        psMemId id = psMemGetId();
        psStats *stats = psStatsAlloc(PS_STAT_SAMPLE_MEAN);
        psImageBinning *binning = psImageBinningAlloc();
        pmTrend2D *trend = pmTrend2DNoImageAlloc(PM_TREND_MAP, NULL, stats);
        ok(trend == NULL, "pmTrend2DNoImageAlloc() returned NULL with NULL psImageBinning input parameter");
        psFree(stats);
        psFree(binning);
        psFree(trend);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmTrend2DNoImageAlloc() with NULL psStats input parameter
    {
        psMemId id = psMemGetId();
        psStats *stats = psStatsAlloc(PS_STAT_SAMPLE_MEAN);
        psImageBinning *binning = psImageBinningAlloc();
        pmTrend2D *trend = pmTrend2DNoImageAlloc(PM_TREND_MAP, binning, NULL);
        ok(trend == NULL, "pmTrend2DNoImageAlloc() returned NULL with NULL psStats input parameter");
        psFree(stats);
        psFree(binning);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmTrend2DNoImageAlloc() with acceptable input parameters
    {
        psMemId id = psMemGetId();
        psStats *stats = psStatsAlloc(PS_STAT_SAMPLE_MEAN);
        psImageBinning *binning = psImageBinningAlloc();
        binning->nXruff = 2;
        binning->nYruff = 4;

        // Call pmTrend2DNoImageAlloc() with PM_TREND_POLY_ORD
        pmTrend2D *trend = pmTrend2DNoImageAlloc(PM_TREND_POLY_ORD, binning, stats);
        ok(trend != NULL && psMemCheckTrend2D(trend), "pmTrend2DNoImageAlloc() returned non-NULL with acceptable input parameters");
        ok(trend->map == NULL, "pmTrend2DAlloc() set trend->map to NULL");
        ok(trend->mode == PM_TREND_POLY_ORD, "pmTrend2DAlloc() set trend->mode correctly");
        ok(trend->stats == stats, "pmTrend2DAlloc() set trend->stats correctly");
        ok(trend->poly != NULL && trend->poly->type == PS_POLYNOMIAL_ORD, "pmTrend2DAlloc() set trend->poly->type correctly");
        ok(trend->poly->type == PS_POLYNOMIAL_ORD, "pmTrend2DAlloc() set trend->poly->type correctly");
        ok(trend->poly->nX == 2, "pmTrend2DAlloc() set trend->poly->nX correctly");
        ok(trend->poly->nY == 4, "pmTrend2DAlloc() set trend->poly->nY correctly");
        psFree(trend);

        // Call pmTrend2DNoImageAlloc() with PM_TREND_POLY_CHEB
        trend = pmTrend2DNoImageAlloc(PM_TREND_POLY_CHEB, binning, stats);
        ok(trend != NULL && psMemCheckTrend2D(trend), "pmTrend2DNoImageAlloc() returned non-NULL with acceptable input parameters");
        ok(trend->map == NULL, "pmTrend2DAlloc() set trend->map to NULL");
        ok(trend->mode == PM_TREND_POLY_CHEB, "pmTrend2DAlloc() set trend->mode correctly");
        ok(trend->stats == stats, "pmTrend2DAlloc() set trend->stats correctly");
        ok(trend->poly != NULL && trend->poly->type == PS_POLYNOMIAL_CHEB, "pmTrend2DAlloc() set trend->poly->type correctly");
        ok(trend->poly->type == PS_POLYNOMIAL_CHEB, "pmTrend2DAlloc() set trend->poly->type correctly");
        ok(trend->poly->nX == 2, "pmTrend2DAlloc() set trend->poly->nX correctly");
        ok(trend->poly->nY == 4, "pmTrend2DAlloc() set trend->poly->nY correctly");
        psFree(trend);

        // Call pmTrend2DNoImageAlloc() with PM_TREND_MAP
        trend = pmTrend2DNoImageAlloc(PM_TREND_MAP, binning, stats);
        ok(trend != NULL && psMemCheckTrend2D(trend), "pmTrend2DNoImageAlloc() returned non-NULL with acceptable input parameters");
        ok(trend->map && psMemCheckImageMap(trend->map), "pmTrend2DNoImageAlloc() set the trend->map correctly");
        psFree(trend);

        psFree(stats);
        psFree(binning);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ------------------------------------------------------------------------
    // Test pmTrend2DFieldAlloc()
    // Call pmTrend2DFieldAlloc() with NULL psStats input parameter
    {
        psMemId id = psMemGetId();
        psImage *img = psImageAlloc(NUM_COLS, NUM_ROWS, PS_TYPE_F32);
        psStats *stats = psStatsAlloc(PS_STAT_SAMPLE_MEAN);
        pmTrend2D *trend = pmTrend2DFieldAlloc(PM_TREND_MAP, 1, 2, 3, 4, NULL);
        ok(trend == NULL, "pmTrend2DFieldAlloc() returned NULL with NULL psStats input parameter");
        psFree(img);
        psFree(stats);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmTrend2DFieldAlloc() with acceptable input parameters
    {
        psMemId id = psMemGetId();
        psStats *stats = psStatsAlloc(PS_STAT_SAMPLE_MEAN);
        pmTrend2D *trend = pmTrend2DFieldAlloc(PM_TREND_POLY_ORD, 1, 2, 3, 4, stats);
        ok(trend != NULL && psMemCheckTrend2D(trend), "pmTrend2DFieldAlloc() returned non-NULL with acceptable input parameters");
        ok(trend->poly != NULL && trend->poly->type == PS_POLYNOMIAL_ORD, "pmTrend2DFieldAlloc() set trend->poly->type correctly");
        ok(trend->poly->type == PS_POLYNOMIAL_ORD, "pmTrend2DFieldAlloc() set trend->poly->type correctly");
        ok(trend->poly->nX == 3, "pmTrend2DFieldAlloc() set trend->poly->nX correctly");
        ok(trend->poly->nY == 4, "pmTrend2DFieldAlloc() set trend->poly->nY correctly");
        psFree(trend);
        trend = NULL;

        // Create a new pmTrend with PM_TREND_MAP
        // XXX: This currently fails due to a big in pmTrend2DFieldAlloc():
        if (0) {
            trend = pmTrend2DFieldAlloc(PM_TREND_MAP, 1, 2, 3, 4, stats);
            ok(trend->map != NULL && psMemCheckImageMap(trend->map), 
               "pmTrend2DAlloc() set trend->map correctly");
	}

        psFree(stats);
        psFree(trend);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ------------------------------------------------------------------------
    // Test pmTrend2DModeToString()
    // psString pmTrend2DModeToString (pmTrend2DMode mode)
    // Call pmTrend2DModeToString() with unallowed pmTrend2DMode.
    // XX: We comment this out because pmTrend2DModeToString() aborts.
    if (0) {
        psMemId id = psMemGetId();
        psString str = pmTrend2DModeToString(99);
        ok(str == NULL, "pmTrend2DModeToString() returned NULL with unallowed pmTrend2DMode");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmTrend2DModeToString() with unallowed pmTrend2DMode.
    {
        psMemId id = psMemGetId();
        psString str = pmTrend2DModeToString(PM_TREND_NONE);
        ok(!strcmp(str, "NONE"), "pmTrend2DModeToString(PM_TREND_NONE)");
        psFree(str);

        str = pmTrend2DModeToString(PM_TREND_POLY_ORD);
        ok(!strcmp(str, "POLY_ORD"), "pmTrend2DModeToString(PM_TREND_POLY_ORD)");
        psFree(str);

        str = pmTrend2DModeToString(PM_TREND_POLY_CHEB);
        ok(!strcmp(str, "POLY_CHEB"), "pmTrend2DModeToString(PM_TREND_POLY_CHEB)");
        psFree(str);

        str = pmTrend2DModeToString(PM_TREND_MAP);
        ok(!strcmp(str, "MAP"), "pmTrend2DModeToString(PM_TREND_MAP)");
        psFree(str);

        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ------------------------------------------------------------------------
    // Test pmTrend2DModeFromString()
    // Call pmTrend2DModeFromString() with NULL input parameter
    {
        psMemId id = psMemGetId();
        pmTrend2DMode mode = pmTrend2DModeFromString(NULL);
        ok(PM_TREND_NONE == mode, "pmTrend2DModeFromString(NULL) returned PM_TREND_NONE");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmTrend2DModeFromString() with unallowed input string
    {
        psMemId id = psMemGetId();
        pmTrend2DMode mode = pmTrend2DModeFromString("BOGUS");
        ok(PM_TREND_NONE == mode, "pmTrend2DModeFromString(BOGUS) returned PM_TREND_NONE");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmTrend2DModeFromString() with acceptable input string
    {
        psMemId id = psMemGetId();
        pmTrend2DMode mode = pmTrend2DModeFromString("NONE");
        ok(PM_TREND_NONE == mode, "pmTrend2DModeFromString(NONE) returned PM_TREND_NONE");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmTrend2DModeFromString() with acceptable input string
    {
        psMemId id = psMemGetId();
        pmTrend2DMode mode = pmTrend2DModeFromString("POLY_ORD");
        ok(PM_TREND_POLY_ORD == mode, "pmTrend2DModeFromString(POLY_ORD) returned PM_TREND_NONE");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmTrend2DModeFromString() with acceptable input string
    {
        psMemId id = psMemGetId();
        pmTrend2DMode mode = pmTrend2DModeFromString("POLY_CHEB");
        ok(PM_TREND_POLY_CHEB == mode, "pmTrend2DModeFromString(POLY_CHEB) returned PM_TREND_NONE");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmTrend2DModeFromString() with acceptable input string
    {
        psMemId id = psMemGetId();
        pmTrend2DMode mode = pmTrend2DModeFromString("MAP");
        ok(PM_TREND_MAP == mode, "pmTrend2DModeFromString(MAP) returned PM_TREND_NONE");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ------------------------------------------------------------------------
    // Test pmTrend2DFit()
    // Call pmTrend2DFit() with bad input parameters
    {
        #define VEC_SIZE 9
        psMemId id = psMemGetId();
        psImage *img = psImageAlloc(NUM_COLS, NUM_ROWS, PS_TYPE_F32);
        psStats *stats = psStatsAlloc(PS_STAT_SAMPLE_MEAN | PS_STAT_SAMPLE_STDEV);
        pmTrend2D *trend = pmTrend2DAlloc(PM_TREND_POLY_ORD, img, 4, 4, stats);
        ok(trend != NULL && psMemCheckTrend2D(trend), 
          "pmTrend2DAlloc() returned non-NULL with acceptable input parameters");
        psVector *x = psVectorAlloc(VEC_SIZE, PS_TYPE_F32);
        psVector *y = psVectorAlloc(VEC_SIZE, PS_TYPE_F32);
        psVector *f = psVectorAlloc(VEC_SIZE, PS_TYPE_F32);
        psVector *mask = psVectorAlloc(VEC_SIZE, PS_TYPE_U8);
        psVector *df = psVectorAlloc(VEC_SIZE, PS_TYPE_F32);
        for (int i = 0 ; i < VEC_SIZE ; i++) {
            x->data.F32[i] = (float) (i);
            y->data.F32[i] = (float) (2 * i);
            f->data.F32[i] = x->data.F32[i] * y->data.F32[i];
            mask->data.U8[i] = 0;
            df->data.F32[i] = 0.0;
        }

        // NULL pmTrend2D input parameter
        bool rc = pmTrend2DFit(NULL, mask, 0, x, y, f, df);
        ok(rc == false, "pmTrend2DFit() returned FALSE with NULL pmTrend2D input parameter");

        // NULL mask input parameter
        rc = pmTrend2DFit(trend, NULL, 0, x, y, f, df);
        ok(rc == false, "pmTrend2DFit() returned FALSE with NULL mask input parameter");

        // NULL x psVector input parameter
        rc = pmTrend2DFit(trend, mask, 0, NULL, y, f, df);
        ok(rc == false, "pmTrend2DFit() returned FALSE with NULL x psVector input parameter");

        // NULL y psVector input parameter
        rc = pmTrend2DFit(trend, mask, 0, x, NULL, f, df);
        ok(rc == false, "pmTrend2DFit() returned FALSE with NULL y psVector input parameter");

        // NULL f psVector input parameter
        rc = pmTrend2DFit(trend, mask, 0, x, y, NULL, df);
        ok(rc == false, "pmTrend2DFit() returned FALSE with NULL f psVector input parameter");

        // NULL df psVector input parameter
        rc = pmTrend2DFit(trend, mask, 0, x, y, f, NULL);
        ok(rc == true, "pmTrend2DFit() returned TRUE with NULL df psVector input parameter");

        psFree(img);
        psFree(stats);
        psFree(trend);
        psFree(x);
        psFree(y);
        psFree(f);
        psFree(mask);
        psFree(df);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Test pmTrend2DEval()
    // Call pmTrend2DEval() with bad input parameters
    {
        psMemId id = psMemGetId();
        psF64 tmpD = pmTrend2DEval(NULL, 0.0, 0.0);
        ok(TEST_FLOATS_EQUAL(tmpD, 0.0), "pmTrend2DEval() returned 0.0 with NULL pmTrend2D input parameter");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

/**
 * Needs to be rewritten to correlate with revised function prototype:
 * psVector *pmTrend2DEvalVector(const pmTrend2D *trend, psVector *mask, psVectorMaskType maskValue, const psVector *x, const psVector *y)
 */

/*
    // ------------------------------------------------------------------------

    // ------------------------------------------------------------------------
    // Test pmTrend2DEvalVector()
    // psVector *pmTrend2DEvalVector (pmTrend2D *trend, psVector *x, psVector *y)
    // Call pmTrend2DEvalVector() with bad input parameters
    {
        psMemId id = psMemGetId();
        psImage *img = psImageAlloc(NUM_COLS, NUM_ROWS, PS_TYPE_F32);
        psStats *stats = psStatsAlloc(PS_STAT_SAMPLE_MEAN | PS_STAT_SAMPLE_STDEV);
        pmTrend2D *trend = pmTrend2DAlloc(PM_TREND_POLY_ORD, img, 4, 4, stats);
        psVector *x = psVectorAlloc(VEC_SIZE, PS_TYPE_F32);
        psVector *y = psVectorAlloc(VEC_SIZE, PS_TYPE_F32);

        // NULL pmTrend2D input parameter
        psVector *f = pmTrend2DEvalVector(NULL, x, y);
        ok(f == NULL, "pmTrend2DEvalVector() returned NULL with NULL pmTrend2D input parameter");

        // NULL x psVector input parameter
        f = pmTrend2DEvalVector(trend, NULL, y);
        ok(f == NULL, "pmTrend2DEvalVector() returned NULL with NULL x psVector input parameter");

        // NULL y psVector input parameter
        f = pmTrend2DEvalVector(trend, x, NULL);
        ok(f == NULL, "pmTrend2DEvalVector() returned NULL with NULL y psVector input parameter");

        psFree(img);
        psFree(stats);
        psFree(trend);
        psFree(x);
        psFree(y);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Test pmTrend2DFit(), pmTrend2DEval(), pmTrend2DEvalVector() with acceptable input parameters
    // NOTE: We only test with a very simple 2D polynomial fit.  This is appropriate since the
    // polynomial testing routines are tested extensively elsewhere.
    // XXX: Must test the PM_TREND_MAP case.
    {
        #define VEC_SIZE 9
        psMemId id = psMemGetId();
        psImage *img = psImageAlloc(NUM_COLS, NUM_ROWS, PS_TYPE_F32);
        psStats *stats = psStatsAlloc(PS_STAT_SAMPLE_MEAN | PS_STAT_SAMPLE_STDEV);
        pmTrend2D *trend = pmTrend2DAlloc(PM_TREND_POLY_ORD, img, 4, 4, stats);
        ok(trend != NULL && psMemCheckTrend2D(trend), 
          "pmTrend2DAlloc() returned non-NULL with acceptable input parameters");
        psVector *x = psVectorAlloc(VEC_SIZE, PS_TYPE_F32);
        psVector *y = psVectorAlloc(VEC_SIZE, PS_TYPE_F32);
        psVector *f = psVectorAlloc(VEC_SIZE, PS_TYPE_F32);
        psVector *mask = psVectorAlloc(VEC_SIZE, PS_TYPE_U8);
        psVector *df = psVectorAlloc(VEC_SIZE, PS_TYPE_F32);

        int cnt = 0;
        for (int i = 0 ; i < 3 ; i++) {
            for (int j = 0 ; j < 3 ; j++) {
                x->data.F32[cnt] = (float) i;
                y->data.F32[cnt] = (float) j;
                f->data.F32[cnt] = x->data.F32[cnt] * y->data.F32[cnt];
                mask->data.U8[cnt] = 0;
                df->data.F32[cnt] = 0.0;
                cnt++;
            }
        }

        bool rc = pmTrend2DFit(trend, mask, 0, x, y, f, NULL);
        ok(rc == true, "pmTrend2DFit() returned TRUE with acceptable input parameters");

        // Test pmTrend2DFit, pmTrend2DEval()
        bool errorFlag = false;
        for (int i = 0 ; i < VEC_SIZE ; i++) {
            if (!TEST_FLOATS_EQUAL(pmTrend2DEval(trend, x->data.F32[i], y->data.F32[i]), f->data.F32[i])) {
                diag("ERROR: at (%.2f %.2f), eval is %.2f, should be %.2f\n", x->data.F32[i], y->data.F32[i],
                      pmTrend2DEval(trend, x->data.F32[i], y->data.F32[i]), f->data.F32[i]);
                errorFlag = true;
            }
        }
        ok(!errorFlag, "pmTrend2DFit() and pmTrend2DEval() set and evaluated the 2DTrend polynomial correctly");

        // Test pmTrend2DEvalVector()
        psVector *fTest = pmTrend2DEvalVector(trend, x, y);
        errorFlag = false;
        for (int i = 0 ; i < VEC_SIZE ; i++) {
            if (!TEST_FLOATS_EQUAL(fTest->data.F32[i], f->data.F32[i])) {
                diag("ERROR: at (%.2f %.2f), eval is %.2f, should be %.2f\n", 
                      x->data.F32[i], y->data.F32[i], fTest->data.F32[i], f->data.F32[i]);
                errorFlag = true;
            }
        }
        ok(!errorFlag, "pmTrend2DFit() and pmTrend2DEval() set and evaluated the 2DTrend polynomial correctly");


        psFree(img);
        psFree(stats);
        psFree(trend);
        psFree(x);
        psFree(y);
        psFree(f);
        psFree(fTest);
        psFree(mask);
        psFree(df);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
*/
}
