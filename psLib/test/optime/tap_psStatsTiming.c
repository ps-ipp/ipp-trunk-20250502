#include <stdio.h>
#include <string.h>
#include <pslib.h>

#include "tap.h"
#include "pstap.h"

// example tap lines:
// ok(condition, "condition succeeded");
// skip_start(condition, Nskip, "Skipping tests because of failure");

# define DTIME(A,B) ((A.tv_sec - B.tv_sec) + 1e-6*(A.tv_usec - B.tv_usec))
struct timeval start, mark;

int main (void)
{
    plan_tests(68);

//    diag("psStats timing tests");

    // build a gauss-deviate vector (mean = 0.0, sigma = 1.0) for tests
    psRandom *seed = psRandomAllocSpecific (PS_RANDOM_TAUS, 0);
    psVector *rnd = psVectorAlloc (1000, PS_TYPE_F32);
    for (int i = 0; i < rnd->n; i++) {
        rnd->data.F32[i] = psRandomGaussian (seed);
    }

//    diag ("timing for sample mean");
    /********** SAMPLE MEAN ***********/
    // test stat sample mean (no mask, no range)
    {
        psMemId id = psMemGetId();

        psStats *stats = psStatsAlloc (PS_STAT_SAMPLE_MEAN);

        gettimeofday (&start, NULL);
        for (int i = 0; i < 10000; i++)
        {
            psVectorStats (stats, rnd, NULL, NULL, 0);
        }
        gettimeofday (&mark, NULL);
        psF64 delta = DTIME(mark, start);
        ok (delta < 0.1, "sample mean %f (mask: 0, range: 0): %.3f sec", stats->sampleMean, delta);
        psFree (stats);

        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // test stat sample mean (mask, no range)
    {
        psMemId id = psMemGetId();

        psStats *stats = psStatsAlloc (PS_STAT_SAMPLE_MEAN);
        psVector *mask = psVectorAlloc (1000, PS_TYPE_U8);
        psVectorInit (mask, 0);
        mask->data.U8[100] = 1;
        mask->data.U8[200] = 1;
        mask->data.U8[300] = 1;

        gettimeofday (&start, NULL);
        for (int i = 0; i < 10000; i++)
        {
            psVectorStats (stats, rnd, NULL, mask, 1);
        }
        gettimeofday (&mark, NULL);
        psF64 delta = DTIME(mark, start);
        ok (delta < 0.12, "sample mean %f (mask: 1, range: 0): %.3f sec", stats->sampleMean, delta);
        psFree (stats);
        psFree (mask);

        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // test stat sample mean (no mask, range)
    {
        psMemId id = psMemGetId();

        psStats *stats = psStatsAlloc (PS_STAT_SAMPLE_MEAN | PS_STAT_USE_RANGE);
        stats->min = -10;
        stats->max = +10;

        gettimeofday (&start, NULL);
        for (int i = 0; i < 10000; i++)
        {
            psVectorStats (stats, rnd, NULL, NULL, 0);
        }
        gettimeofday (&mark, NULL);
        psF64 delta = DTIME(mark, start);
        ok (delta < 0.18, "sample mean %f (mask: 0, range: 1): %.3f sec", stats->sampleMean, delta);
        psFree (stats);

        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // test stat sample mean (mask, range)
    {
        psMemId id = psMemGetId();

        psStats *stats = psStatsAlloc (PS_STAT_SAMPLE_MEAN | PS_STAT_USE_RANGE);
        stats->min = -10;
        stats->max = +10;
        psVector *mask = psVectorAlloc (1000, PS_TYPE_U8);
        psVectorInit (mask, 0);
        mask->data.U8[100] = 1;
        mask->data.U8[200] = 1;
        mask->data.U8[300] = 1;

        gettimeofday (&start, NULL);
        for (int i = 0; i < 10000; i++)
        {
            psVectorStats (stats, rnd, NULL, mask, 1);
        }
        gettimeofday (&mark, NULL);
        psF64 delta = DTIME(mark, start);
        ok (delta < 0.2, "sample mean %f (mask: 1, range: 1): %.3f sec", stats->sampleMean, delta);
        psFree (stats);
        psFree (mask);

        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // test stat sample mean (mask, range : small sample)
    {
        psMemId id = psMemGetId();

        psStats *stats = psStatsAlloc (PS_STAT_SAMPLE_MEAN | PS_STAT_USE_RANGE);
        stats->min = -10;
        stats->max = +10;
        psVector *mask = psVectorAlloc (10, PS_TYPE_U8);
        psVectorInit (mask, 0);
        mask->data.U8[3] = 1;
        int nOld = rnd->n;

        rnd->n = 10;
        gettimeofday (&start, NULL);
        for (int i = 0; i < 1000000; i++)
        {
            psVectorStats (stats, rnd, NULL, mask, 1);
        }
        gettimeofday (&mark, NULL);
        rnd->n = nOld;

        psF64 delta = DTIME(mark, start);
        ok (delta < 0.2, "sample mean %f (mask: 1, range: 1): %.3f sec", stats->sampleMean, delta);
        psFree (stats);
        psFree (mask);

        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

//    diag ("timing for sample median");
    /********** SAMPLE MEDIAN ***********/
    // test stat sample median (no mask, no range)
    {
        psMemId id = psMemGetId();

        psStats *stats = psStatsAlloc (PS_STAT_SAMPLE_MEDIAN);

        gettimeofday (&start, NULL);
        for (int i = 0; i < 10000; i++)
        {
            psVectorStats (stats, rnd, NULL, NULL, 0);
        }
        gettimeofday (&mark, NULL);
        psF64 delta = DTIME(mark, start);
        ok (delta < 2.8, "sample median %f (mask: 0, range: 0): %.3f sec", stats->sampleMedian, delta);
        psFree (stats);

        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // test stat sample median (mask, no range)
    {
        psMemId id = psMemGetId();

        psStats *stats = psStatsAlloc (PS_STAT_SAMPLE_MEDIAN);
        psVector *mask = psVectorAlloc (1000, PS_TYPE_U8);
        psVectorInit (mask, 0);
        mask->data.U8[100] = 1;
        mask->data.U8[200] = 1;
        mask->data.U8[300] = 1;

        gettimeofday (&start, NULL);
        for (int i = 0; i < 10000; i++)
        {
            psVectorStats (stats, rnd, NULL, mask, 1);
        }
        gettimeofday (&mark, NULL);
        psF64 delta = DTIME(mark, start);
        ok (delta < 2.8, "sample median %f (mask: 1, range: 0): %.3f sec", stats->sampleMedian, delta);
        psFree (stats);
        psFree (mask);

        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // test stat sample median (no mask, range)
    {
        psMemId id = psMemGetId();

        psStats *stats = psStatsAlloc (PS_STAT_SAMPLE_MEDIAN | PS_STAT_USE_RANGE);
        stats->min = -10;
        stats->max = +10;

        gettimeofday (&start, NULL);
        for (int i = 0; i < 10000; i++)
        {
            psVectorStats (stats, rnd, NULL, NULL, 0);
        }
        gettimeofday (&mark, NULL);
        psF64 delta = DTIME(mark, start);
        ok (delta < 2.8, "sample median %f (mask: 0, range: 1): %.3f sec", stats->sampleMedian, delta);
        psFree (stats);

        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // test stat sample median (mask, range)
    {
        psMemId id = psMemGetId();

        psStats *stats = psStatsAlloc (PS_STAT_SAMPLE_MEDIAN | PS_STAT_USE_RANGE);
        stats->min = -10;
        stats->max = +10;
        psVector *mask = psVectorAlloc (1000, PS_TYPE_U8);
        psVectorInit (mask, 0);
        mask->data.U8[100] = 1;
        mask->data.U8[200] = 1;
        mask->data.U8[300] = 1;

        gettimeofday (&start, NULL);
        for (int i = 0; i < 10000; i++)
        {
            psVectorStats (stats, rnd, NULL, mask, 1);
        }
        gettimeofday (&mark, NULL);
        psF64 delta = DTIME(mark, start);
        ok (delta < 2.8, "sample median %f (mask: 1, range: 1): %.3f sec", stats->sampleMedian, delta);
        psFree (stats);
        psFree (mask);

        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

//    diag ("timing for sample stdev");
    /********** SAMPLE STDEV ***********/
    // test stat sample stdev (no mask, no range)
    {
        psMemId id = psMemGetId();

        psStats *stats = psStatsAlloc (PS_STAT_SAMPLE_STDEV);

        gettimeofday (&start, NULL);
        for (int i = 0; i < 10000; i++)
        {
            psVectorStats (stats, rnd, NULL, NULL, 0);
        }
        gettimeofday (&mark, NULL);
        psF64 delta = DTIME(mark, start);
        ok (delta < 0.2, "sample stdev %f (mask: 0, range: 0): %.3f sec", stats->sampleStdev, delta);
        psFree (stats);

        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // test stat sample stdev (mask, no range)
    {
        psMemId id = psMemGetId();

        psStats *stats = psStatsAlloc (PS_STAT_SAMPLE_STDEV);
        psVector *mask = psVectorAlloc (1000, PS_TYPE_U8);
        psVectorInit (mask, 0);
        mask->data.U8[100] = 1;
        mask->data.U8[200] = 1;
        mask->data.U8[300] = 1;

        gettimeofday (&start, NULL);
        for (int i = 0; i < 10000; i++)
        {
            psVectorStats (stats, rnd, NULL, mask, 1);
        }
        gettimeofday (&mark, NULL);
        psF64 delta = DTIME(mark, start);
        ok (delta < 0.27, "sample stdev %f (mask: 1, range: 0): %.3f sec", stats->sampleStdev, delta);
        psFree (stats);
        psFree (mask);

        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // test stat sample stdev (no mask, range)
    {
        psMemId id = psMemGetId();

        psStats *stats = psStatsAlloc (PS_STAT_SAMPLE_STDEV | PS_STAT_USE_RANGE);
        stats->min = -10;
        stats->max = +10;

        gettimeofday (&start, NULL);
        for (int i = 0; i < 10000; i++)
        {
            psVectorStats (stats, rnd, NULL, NULL, 0);
        }
        gettimeofday (&mark, NULL);
        psF64 delta = DTIME(mark, start);
        ok (delta < 0.36, "sample stdev %f (mask: 0, range: 1): %.3f sec", stats->sampleStdev, delta);
        psFree (stats);

        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // test stat sample stdev (mask, range)
    {
        psMemId id = psMemGetId();

        psStats *stats = psStatsAlloc (PS_STAT_SAMPLE_STDEV | PS_STAT_USE_RANGE);
        stats->min = -10;
        stats->max = +10;
        psVector *mask = psVectorAlloc (1000, PS_TYPE_U8);
        psVectorInit (mask, 0);
        mask->data.U8[100] = 1;
        mask->data.U8[200] = 1;
        mask->data.U8[300] = 1;

        gettimeofday (&start, NULL);
        for (int i = 0; i < 10000; i++)
        {
            psVectorStats (stats, rnd, NULL, mask, 1);
        }
        gettimeofday (&mark, NULL);
        psF64 delta = DTIME(mark, start);
        ok (delta < 0.42, "sample stdev %f (mask: 1, range: 1): %.3f sec", stats->sampleStdev, delta);
        psFree (stats);
        psFree (mask);

        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // test stat sample stdev (mask, range)
    {
        psMemId id = psMemGetId();

        psStats *stats = psStatsAlloc (PS_STAT_SAMPLE_STDEV | PS_STAT_USE_RANGE);
        stats->min = -10;
        stats->max = +10;
        psVector *mask = psVectorAlloc (10, PS_TYPE_U8);
        psVectorInit (mask, 0);
        mask->data.U8[1] = 1;
        int nOld = rnd->n;

        rnd->n = 10;
        gettimeofday (&start, NULL);
        for (int i = 0; i < 1000000; i++)
        {
            psVectorStats (stats, rnd, NULL, mask, 1);
        }
        gettimeofday (&mark, NULL);
        rnd->n = nOld;

        psF64 delta = DTIME(mark, start);
        ok (delta < 0.42, "sample stdev %f (mask: 1, range: 1): %.3f sec", stats->sampleStdev, delta);
        psFree (stats);
        psFree (mask);

        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

//    diag ("timing for sample min,max");
    /*************** MIN,MAX ******************/
    // test stat min,max (no mask, no range)
    {
        psMemId id = psMemGetId();

        psStats *stats = psStatsAlloc (PS_STAT_MIN | PS_STAT_MAX);
        gettimeofday (&start, NULL);
        for (int i = 0; i < 10000; i++)
        {
            psVectorStats (stats, rnd, NULL, NULL, 1);
        }
        gettimeofday (&mark, NULL);
        psF64 delta = DTIME(mark, start);
        ok (delta < 0.17, "sample min,max %f,%f (mask: 0, range: 0): %.3f sec", stats->min, stats->max, delta);
        psFree (stats);

        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
    // test stat min,max (no mask, no range)
    {
        psMemId id = psMemGetId();

        psStats *stats = psStatsAlloc (PS_STAT_MIN | PS_STAT_MAX);
        psVector *mask = psVectorAlloc (1000, PS_TYPE_U8);
        psVectorInit (mask, 0);
        mask->data.U8[100] = 1;
        mask->data.U8[200] = 1;
        mask->data.U8[300] = 1;

        gettimeofday (&start, NULL);
        for (int i = 0; i < 10000; i++)
        {
            psVectorStats (stats, rnd, NULL, mask, 1);
        }
        gettimeofday (&mark, NULL);
        psF64 delta = DTIME(mark, start);
        ok (delta < 0.18, "sample min,max %f,%f (mask: 1, range: 0): %.3f sec", stats->min, stats->max, delta);
        psFree (stats);
        psFree (mask);

        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
    // test stat min,max (no mask, no range)
    {
        psMemId id = psMemGetId();

        psStats *stats = psStatsAlloc (PS_STAT_MIN | PS_STAT_MAX | PS_STAT_USE_RANGE);
        stats->min = -10;
        stats->max = +10;

        gettimeofday (&start, NULL);
        for (int i = 0; i < 10000; i++)
        {
            psVectorStats (stats, rnd, NULL, NULL, 1);
        }
        gettimeofday (&mark, NULL);
        psF64 delta = DTIME(mark, start);
        ok (delta < 0.22, "sample min,max %f,%f (mask: 0, range: 1): %.3f sec", stats->min, stats->max, delta);
        psFree (stats);

        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
    // test stat min,max (no mask, no range)
    {
        psMemId id = psMemGetId();

        psStats *stats = psStatsAlloc (PS_STAT_MIN | PS_STAT_MAX | PS_STAT_USE_RANGE);
        stats->min = -10;
        stats->max = +10;
        psVector *mask = psVectorAlloc (1000, PS_TYPE_U8);
        psVectorInit (mask, 0);
        mask->data.U8[100] = 1;
        mask->data.U8[200] = 1;
        mask->data.U8[300] = 1;

        gettimeofday (&start, NULL);
        for (int i = 0; i < 10000; i++)
        {
            psVectorStats (stats, rnd, NULL, mask, 1);
        }
        gettimeofday (&mark, NULL);
        psF64 delta = DTIME(mark, start);
        ok (delta < 0.26, "sample min,max %f,%f (mask: 1, range: 1): %.3f sec", stats->min, stats->max, delta);
        psFree (stats);
        psFree (mask);

        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

//    diag ("timing for clipped stats");
    /********** CLIPPED STATS ***********/
    {
        psMemId id = psMemGetId();

        psStats *stats = psStatsAlloc (PS_STAT_CLIPPED_MEAN | PS_STAT_CLIPPED_STDEV);
        psVector *rnd2 = psVectorAlloc (1000, PS_TYPE_F32);
        for (int i = 0; i < rnd2->n; i++)
        {
            rnd2->data.F32[i] = psRandomGaussian (seed);
        }

        gettimeofday (&start, NULL);
        for (int i = 0; i < 1000; i++)
        {
            psVectorStats (stats, rnd2, NULL, NULL, 1);
        }
        gettimeofday (&mark, NULL);
        psF64 delta = DTIME(mark, start);
        ok (delta < 0.3, "clipped mean %f, stdev %f (mask: 0, range: 0): %.3f sec (1000 pts / 1000 loops)", stats->clippedMean, stats->clippedStdev, delta);
        psFree (stats);
        psFree (rnd2);

        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
    {
        psMemId id = psMemGetId();

        psStats *stats = psStatsAlloc (PS_STAT_CLIPPED_MEAN | PS_STAT_CLIPPED_STDEV);
        psVector *rnd2 = psVectorAlloc (3000, PS_TYPE_F32);
        for (int i = 0; i < rnd2->n; i++)
        {
            rnd2->data.F32[i] = psRandomGaussian (seed);
        }

        gettimeofday (&start, NULL);
        for (int i = 0; i < 1000; i++)
        {
            psVectorStats (stats, rnd2, NULL, NULL, 1);
        }
        gettimeofday (&mark, NULL);
        psF64 delta = DTIME(mark, start);
        ok (delta < 0.5, "clipped mean %f, stdev %f (mask: 0, range: 0): %.3f sec (3000 pts / 1000 loops)", stats->clippedMean, stats->clippedStdev, delta);
        psFree (stats);
        psFree (rnd2);

        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
    {
        psMemId id = psMemGetId();

        psStats *stats = psStatsAlloc (PS_STAT_CLIPPED_MEAN | PS_STAT_CLIPPED_STDEV);
        psVector *rnd2 = psVectorAlloc (10000, PS_TYPE_F32);
        for (int i = 0; i < rnd2->n; i++)
        {
            rnd2->data.F32[i] = psRandomGaussian (seed);
        }

        gettimeofday (&start, NULL);
        for (int i = 0; i < 1000; i++)
        {
            psVectorStats (stats, rnd2, NULL, NULL, 1);
        }
        gettimeofday (&mark, NULL);
        psF64 delta = DTIME(mark, start);
        ok (delta < 1.2, "clipped mean %f, stdev %f (mask: 0, range: 0): %.3f sec (10000 pts / 1000 loops)", stats->clippedMean, stats->clippedStdev, delta);
        psFree (stats);
        psFree (rnd2);

        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

//    diag ("timing for robust stats");
    /********** ROBUST STATS ***********/
    {
        psMemId id = psMemGetId();

        psStats *stats = psStatsAlloc (PS_STAT_ROBUST_MEDIAN | PS_STAT_ROBUST_STDEV | PS_STAT_ROBUST_QUARTILE);
        psVector *rnd2 = psVectorAlloc (1000, PS_TYPE_F32);
        for (int i = 0; i < rnd2->n; i++)
        {
            rnd2->data.F32[i] = psRandomGaussian (seed);
        }

        gettimeofday (&start, NULL);
        for (int i = 0; i < 1000; i++)
        {
            psVectorStats (stats, rnd2, NULL, NULL, 1);
        }
        gettimeofday (&mark, NULL);
        psF64 delta = DTIME(mark, start);
        ok (delta < 0.3, "robust mean %f, stdev %f (mask: 0, range: 0): %.3f sec (1000 pts / 1000 loops)", stats->robustMedian, stats->robustStdev, delta);
        psFree (stats);
        psFree (rnd2);

        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
    {
        psMemId id = psMemGetId();

        psStats *stats = psStatsAlloc (PS_STAT_ROBUST_MEDIAN | PS_STAT_ROBUST_STDEV | PS_STAT_ROBUST_QUARTILE);
        psVector *rnd2 = psVectorAlloc (3000, PS_TYPE_F32);
        for (int i = 0; i < rnd2->n; i++)
        {
            rnd2->data.F32[i] = psRandomGaussian (seed);
        }

        gettimeofday (&start, NULL);
        for (int i = 0; i < 1000; i++)
        {
            psVectorStats (stats, rnd2, NULL, NULL, 1);
        }
        gettimeofday (&mark, NULL);
        psF64 delta = DTIME(mark, start);
        ok (delta < 0.5, "robust mean %f, stdev %f (mask: 0, range: 0): %.3f sec (3000 pts / 1000 loops)", stats->robustMedian, stats->robustStdev, delta);
        psFree (stats);
        psFree (rnd2);

        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
    {
        psMemId id = psMemGetId();

        psStats *stats = psStatsAlloc (PS_STAT_ROBUST_MEDIAN | PS_STAT_ROBUST_STDEV | PS_STAT_ROBUST_QUARTILE);
        psVector *rnd2 = psVectorAlloc (10000, PS_TYPE_F32);
        for (int i = 0; i < rnd2->n; i++)
        {
            rnd2->data.F32[i] = psRandomGaussian (seed);
        }

        gettimeofday (&start, NULL);
        for (int i = 0; i < 1000; i++)
        {
            psVectorStats (stats, rnd2, NULL, NULL, 1);
        }
        gettimeofday (&mark, NULL);
        psF64 delta = DTIME(mark, start);
        ok (delta < 1.2, "robust mean %f, stdev %f (mask: 0, range: 0): %.3f sec (10000 pts / 1000 loops)", stats->robustMedian, stats->robustStdev, delta);
        psFree (stats);
        psFree (rnd2);

        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

//    diag ("timing for fitted stats");
    /********** FITTED TIMING ***********/
    {
        psMemId id = psMemGetId();

        psStats *stats = psStatsAlloc (PS_STAT_FITTED_MEAN | PS_STAT_FITTED_STDEV);
        psVector *rnd2 = psVectorAlloc (1000, PS_TYPE_F32);
        for (int i = 0; i < rnd2->n; i++)
        {
            rnd2->data.F32[i] = psRandomGaussian (seed);
        }

        gettimeofday (&start, NULL);
        for (int i = 0; i < 1000; i++)
        {
            psVectorStats (stats, rnd2, NULL, NULL, 1);

        }
        gettimeofday (&mark, NULL);
        psF64 delta = DTIME(mark, start);
        ok (delta < 0.7, "fitted mean %f, stdev %f (mask: 0, range: 0): %.3f sec (1000 pts / 1000 loops)", stats->fittedMean, stats->fittedStdev, delta);
        psFree (stats);
        psFree (rnd2);

        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
    {
        psMemId id = psMemGetId();

        psStats *stats = psStatsAlloc (PS_STAT_FITTED_MEAN | PS_STAT_FITTED_STDEV);
        psVector *rnd2 = psVectorAlloc (3000, PS_TYPE_F32);
        for (int i = 0; i < rnd2->n; i++)
        {
            rnd2->data.F32[i] = psRandomGaussian (seed);
        }

        gettimeofday (&start, NULL);
        for (int i = 0; i < 1000; i++)
        {
            psVectorStats (stats, rnd2, NULL, NULL, 1);
        }
        gettimeofday (&mark, NULL);
        psF64 delta = DTIME(mark, start);
        ok (delta < 0.8, "fitted mean %f, stdev %f (mask: 0, range: 0): %.3f sec (3000 pts / 1000 loops)", stats->fittedMean, stats->fittedStdev, delta);
        psFree (stats);
        psFree (rnd2);

        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
    {
        psMemId id = psMemGetId();

        psStats *stats = psStatsAlloc (PS_STAT_FITTED_MEAN | PS_STAT_FITTED_STDEV);
        psVector *rnd2 = psVectorAlloc (10000, PS_TYPE_F32);
        for (int i = 0; i < rnd2->n; i++)
        {
            rnd2->data.F32[i] = psRandomGaussian (seed);
        }

        gettimeofday (&start, NULL);
        for (int i = 0; i < 1000; i++)
        {
            psVectorStats (stats, rnd2, NULL, NULL, 1);
        }
        gettimeofday (&mark, NULL);
        psF64 delta = DTIME(mark, start);
        ok (delta < 2.2, "fitted mean %f, stdev %f (mask: 0, range: 0): %.3f sec (10000 pts / 1000 loops)", stats->fittedMean, stats->fittedStdev, delta);
        psFree (stats);
        psFree (rnd2);

        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

//    diag ("timing for fitted (v2) stats");
    /********** FITTED (v2) TIMING ***********/
    {
        psMemId id = psMemGetId();

        psStats *stats = psStatsAlloc (PS_STAT_FITTED_MEAN | PS_STAT_FITTED_STDEV);
        psVector *rnd2 = psVectorAlloc (1000, PS_TYPE_F32);
        for (int i = 0; i < rnd2->n; i++)
        {
            rnd2->data.F32[i] = psRandomGaussian (seed);
        }

        gettimeofday (&start, NULL);
        for (int i = 0; i < 1000; i++)
        {
            psVectorStats (stats, rnd2, NULL, NULL, 1);

        }
        gettimeofday (&mark, NULL);
        psF64 delta = DTIME(mark, start);
        ok (delta < 0.7, "fitted mean %f, stdev %f (mask: 0, range: 0): %.3f sec (1000 pts / 1000 loops)", stats->fittedMean, stats->fittedStdev, delta);
        psFree (stats);
        psFree (rnd2);

        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
    {
        psMemId id = psMemGetId();

        psStats *stats = psStatsAlloc (PS_STAT_FITTED_MEAN | PS_STAT_FITTED_STDEV);
        psVector *rnd2 = psVectorAlloc (3000, PS_TYPE_F32);
        for (int i = 0; i < rnd2->n; i++)
        {
            rnd2->data.F32[i] = psRandomGaussian (seed);
        }

        gettimeofday (&start, NULL);
        for (int i = 0; i < 1000; i++)
        {
            psVectorStats (stats, rnd2, NULL, NULL, 1);
        }
        gettimeofday (&mark, NULL);
        psF64 delta = DTIME(mark, start);
        ok (delta < 0.8, "fitted mean %f, stdev %f (mask: 0, range: 0): %.3f sec (3000 pts / 1000 loops)", stats->fittedMean, stats->fittedStdev, delta);
        psFree (stats);
        psFree (rnd2);

        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
    {
        psMemId id = psMemGetId();

        psStats *stats = psStatsAlloc (PS_STAT_FITTED_MEAN | PS_STAT_FITTED_STDEV);
        psVector *rnd2 = psVectorAlloc (10000, PS_TYPE_F32);
        for (int i = 0; i < rnd2->n; i++)
        {
            rnd2->data.F32[i] = psRandomGaussian (seed);
        }

        gettimeofday (&start, NULL);
        for (int i = 0; i < 1000; i++)
        {
            psVectorStats (stats, rnd2, NULL, NULL, 1);
        }
        gettimeofday (&mark, NULL);
        psF64 delta = DTIME(mark, start);
        ok (delta < 2.2, "fitted mean %f, stdev %f (mask: 0, range: 0): %.3f sec (10000 pts / 1000 loops)", stats->fittedMean, stats->fittedStdev, delta);
        psFree (stats);
        psFree (rnd2);

        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

//    diag ("compare sample, robust, and fitted mean and stdev to theoretical");
    // compare SAMPLE, FITTED, ROBUST mean to theoretical
    {
        psMemId id = psMemGetId();

        psStats *stats = psStatsAlloc (PS_STAT_SAMPLE_MEAN | PS_STAT_SAMPLE_STDEV | PS_STAT_ROBUST_MEDIAN | PS_STAT_ROBUST_STDEV | PS_STAT_FITTED_MEAN | PS_STAT_FITTED_STDEV);
        psVector *sample = psVectorAlloc (1000, PS_TYPE_F32);
        psVector *robust = psVectorAlloc (1000, PS_TYPE_F32);
        psVector *fitted = psVectorAlloc (1000, PS_TYPE_F32);

        for (int i = 0; i < 1000; i++)
        {
            // generate a new sample
            for (int j = 0; j < rnd->n; j++) {
                rnd->data.F32[j] = psRandomGaussian (seed);
            }
            // measure the stats
            psVectorStats (stats, rnd, NULL, NULL, 1);
            sample->data.F32[i] = stats->sampleMean;
            robust->data.F32[i] = stats->robustMedian;
            fitted->data.F32[i] = stats->fittedMean;
        }
        psFree (stats);

        stats = psStatsAlloc (PS_STAT_SAMPLE_MEAN | PS_STAT_SAMPLE_STDEV);
        psVectorStats (stats, sample, NULL, NULL, 1);
        ok (stats->sampleStdev < 2/sqrt(1000), "sample mean %f, stdev %f (1000 tries)", stats->sampleMean, stats->sampleStdev);
        psVectorStats (stats, robust, NULL, NULL, 1);
        ok (stats->sampleStdev < 2/sqrt(1000), "robust mean %f, stdev %f (1000 tries)", stats->sampleMean, stats->sampleStdev);
        psVectorStats (stats, fitted, NULL, NULL, 1);
        ok (stats->sampleStdev < 2/sqrt(1000), "fitted mean %f, stdev %f (1000 tries)", stats->sampleMean, stats->sampleStdev);
        psFree (stats);
        psFree (sample);
        psFree (robust);
        psFree (fitted);

        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

//    diag ("compare sample, robust, and fitted mean and stdev to theoretical");
    // compare SAMPLE, FITTED_V2, ROBUST mean to theoretical
    {
        psMemId id = psMemGetId();

        psStats *stats = psStatsAlloc (PS_STAT_SAMPLE_MEAN | PS_STAT_SAMPLE_STDEV | PS_STAT_ROBUST_MEDIAN | PS_STAT_ROBUST_STDEV | PS_STAT_FITTED_MEAN | PS_STAT_FITTED_STDEV);
        psVector *sample = psVectorAlloc (1000, PS_TYPE_F32);
        psVector *robust = psVectorAlloc (1000, PS_TYPE_F32);
        psVector *fitted = psVectorAlloc (1000, PS_TYPE_F32);

        for (int i = 0; i < 1000; i++)
        {
            // generate a new sample
            for (int j = 0; j < rnd->n; j++) {
                rnd->data.F32[j] = psRandomGaussian (seed);
            }
            // measure the stats
            psVectorStats (stats, rnd, NULL, NULL, 1);
            sample->data.F32[i] = stats->sampleMean;
            robust->data.F32[i] = stats->robustMedian;
            fitted->data.F32[i] = stats->fittedMean;
        }
        psFree (stats);

        stats = psStatsAlloc (PS_STAT_SAMPLE_MEAN | PS_STAT_SAMPLE_STDEV);
        psVectorStats (stats, sample, NULL, NULL, 1);
        ok (stats->sampleStdev < 2/sqrt(1000), "sample mean %f, stdev %f (1000 tries)", stats->sampleMean, stats->sampleStdev);
        psVectorStats (stats, robust, NULL, NULL, 1);
        ok (stats->sampleStdev < 2/sqrt(1000), "robust mean %f, stdev %f (1000 tries)", stats->sampleMean, stats->sampleStdev);
        psVectorStats (stats, fitted, NULL, NULL, 1);
        ok (stats->sampleStdev < 2/sqrt(1000), "fitted mean %f, stdev %f (1000 tries)", stats->sampleMean, stats->sampleStdev);
        psFree (stats);
        psFree (sample);
        psFree (robust);
        psFree (fitted);

        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    return exit_status();
}

