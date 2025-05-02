#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>

#include "psMemory.h"
#include "psTrace.h"
#include "psImage.h"
#include "psVector.h"
#include "psStats.h"
#include "psType.h"
#include "psAssert.h"
#include "psAbort.h"
#include "psRandom.h"
#include "psError.h"

static int nFailures = 0;

void psImageBackgroundInit() {
    nFailures = 0;
    return;
}

bool psImageBackground(psStats *stats, psVector **sample, const psImage *image, const psImage *mask, psImageMaskType maskValue, psRandom *rng)
{
    PS_ASSERT_IMAGE_NON_NULL(image, NULL);
    if (mask) {
        PS_ASSERT_IMAGE_NON_NULL(mask, NULL);
        PS_ASSERT_IMAGE_TYPE(mask, PS_TYPE_IMAGE_MASK, NULL);
        PS_ASSERT_IMAGES_SIZE_EQUAL(mask, image, NULL);
    }
    if (stats->options & PS_STAT_ROBUST_QUARTILE) {
        PS_ASSERT_FLOAT_LARGER_THAN_OR_EQUAL(stats->min, 0.0, NULL);
        PS_ASSERT_FLOAT_LARGER_THAN_OR_EQUAL(stats->max, 0.0, NULL);
        PS_ASSERT_FLOAT_LESS_THAN_OR_EQUAL(stats->min, 1.0, NULL);
        PS_ASSERT_FLOAT_LESS_THAN_OR_EQUAL(stats->max, 1.0, NULL);
    }
    PS_ASSERT_INT_NONNEGATIVE(stats->nSubsample, NULL);
    PS_ASSERT_PTR_NON_NULL(rng, NULL);

    // Size of image
    long nx = image->numCols;
    long ny = image->numRows;
    long nPixels = nx * ny;

    long nSubset = (stats->nSubsample == 0) ? (nx * ny) : PS_MIN(stats->nSubsample, (nx * ny));

    /*** discussion of the sampling and algorithm choices:

	 We want stats->nSubsample pixels.  How many good pixels do we actually have?  

	 There are a few domains of interest:

	 If nGoodPixels < f1 * nSubset, we should fail [1]
	 If nSubset >= nGoodPixels, we should just loop over all pixels [2]
	 If (nSubset <  f2 * nGoodPixels) and (nGoodPixels >= f3 * nPixels), we should just use the random sample technique [3]
	 If (nSubset >= f2 * nGoodPixels) or  (nGoodPixel  <  f3 * nPixels), we should use the Fisher-Yates technique.

	 to use Fisher-Yates, we need to have a copy of the pixels so we can re-shuffle.  We
	 should not do that with the whole list unless we need to.

	 [1] we just have too few samples to be useful; f1 ~ 0.01?

	 [2] we need to select from all pixels to reach the desired sample size

	 [3] this avoids a full-image-sized alloc, but only makes sense if nGoodPixels is actually
	 large.  f2 ~ 0.01, f3 ~ 0.1 (4 tries per success on average)

    ***/

    // count the number of good pixels
    long nGoodPixels = 0;
    for (long iy = 0; iy < ny; iy++) {
	for (long ix = 0; ix < nx; ix++) {
	    if (!isfinite(image->data.F32[iy][ix])) continue;
	    if (mask && mask->data.PS_TYPE_IMAGE_MASK_DATA[iy][ix] & maskValue) continue;
	    nGoodPixels ++;
	}
    }

    // XXX This value of 1% is somewhat arbitrary
    if (nGoodPixels < 0.01*nSubset) {
        if ((nFailures < 3) || (nFailures % 100 == 0)) {
            psLogMsg("psLib.psImageBackground", PS_LOG_DETAIL, "Unable to measure image background: too few data points (%ld) (%d failures)", nGoodPixels, nFailures);
        }
        nFailures ++;
        psTrace ("psLib.imageops", 4, "case 1: nGoodPixels < 0.01*nSubset (%d x %d : %d : %d : %d)\n", (int) nx, (int) ny, (int) nSubset, (int) nGoodPixels, (int) nPixels);
        return false;
    }

    // alloc or recycle the vector containing subsample pixels
    psVector *values;
    if (sample) {
        *sample = psVectorRecycle(*sample, nSubset, PS_TYPE_F32);
        values = psMemIncrRefCounter(*sample);
        values->n = 0;
    } else {
        values = psVectorAllocEmpty(nSubset, PS_TYPE_F32);
    }

    // Minimum and maximum values
    float min = +PS_MAX_F32;
    float max = -PS_MAX_F32;
    
    if ((nSubset < 0.01*nGoodPixels) && (nGoodPixels >= 0.1*nPixels)) {
	psTrace ("psLib.imageops", 4, "case 3: nSubset < 0.01*nGoodPixels && nGoodPixels > 0.1*nPixels (%d x %d : %d : %d : %d)\n", (int) nx, (int) ny, (int) nSubset, (int) nGoodPixels, (int) nPixels);
	// for small subsets, just use simple random sampling (saves a full-image-sized alloc),
	// unless we have lots of masked pixels
        for (long i = 0; i < nSubset; i++) {
            double frnd = psRandomUniform(rng);
            long pixel = nPixels * frnd;
            long ix = pixel % nx;
            long iy = pixel / nx;

	    if (!isfinite(image->data.F32[iy][ix])) continue;
	    if (mask && mask->data.PS_TYPE_IMAGE_MASK_DATA[iy][ix] & maskValue) continue;

            float value = image->data.F32[iy][ix];
            min = PS_MIN(value, min);
            max = PS_MAX(value, max);
            psVectorAppend (values, value);
        }
    } else if (nSubset >= nGoodPixels) {
	psTrace ("psLib.imageops", 4, "case 2: nSubset >= nGoodPixels (%d x %d : %d : %d : %d)\n", (int) nx, (int) ny, (int) nSubset, (int) nGoodPixels, (int) nPixels);
	// in this case, we have to select from all masked pixels just to get the desired
	// sample size
	for (long iy = 0; iy < ny; iy++) {
	    for (long ix = 0; ix < nx; ix++) {
		if (!isfinite(image->data.F32[iy][ix])) continue;
		if (mask && mask->data.PS_TYPE_IMAGE_MASK_DATA[iy][ix] & maskValue) continue;
		float value = image->data.F32[iy][ix];
		min = PS_MIN(value, min);
		max = PS_MAX(value, max);
		psVectorAppend(values, value);
	    }
	}
    } else {
	psTrace ("psLib.imageops", 4, "case 4: Fisher-Yates (%d x %d : %d : %d : %d)\n", (int) nx, (int) ny, (int) nSubset, (int) nGoodPixels, (int) nPixels);
	// use Knuth's version of Fisher-Yates to select a random subset of the pixels, but
	// only drawing each value once
	    
	// generate a vector of all pixels which may in theory be selected
	psVector *pixelVector = psVectorAllocEmpty(nGoodPixels, PS_TYPE_F32);
	int ipix = 0;
	for (long iy = 0; iy < ny; iy++) {
	    for (long ix = 0; ix < nx; ix++) {
		if (!isfinite(image->data.F32[iy][ix])) continue;
		if (mask && mask->data.PS_TYPE_IMAGE_MASK_DATA[iy][ix] & maskValue) continue;
		psAssert (ipix < pixelVector->nalloc, "we must have mis-counted above");
		pixelVector->data.F32[ipix] = image->data.F32[iy][ix];
		ipix ++;
	    }
	}
	pixelVector->n = ipix;
	psAssert (ipix <= pixelVector->nalloc, "we must have mis-counted above");
	psAssert (nGoodPixels == pixelVector->n, "we must have mis-counted above");

	// generate the unique random subset
	for (int i = 0; i < nSubset; i++) {
	    double frnd = psRandomUniform(rng);
	    int pixel = pixelVector->n * frnd;
	    // psAssert (pixel < pixelVector->nalloc, "oops, we went too far (1)");
	    // psAssert (pixel >= 0, "oops, we went too far (2)");
	    // psAssert (pixelVector->n - 1 < pixelVector->nalloc, "oops, we went too far (3)");
	    // psAssert (pixelVector->n - 1 >= 0, "oops, we went too far (4)");

	    psVectorAppend(values, pixelVector->data.F32[pixel]);
	    pixelVector->data.F32[pixel] = pixelVector->data.F32[pixelVector->n - 1];
	    pixelVector->n --;
	}
	psFree (pixelVector);
    }
    psAssert (values->n >= 0.01*nSubset, "didn't we test this above?");

    if (stats->options & PS_STAT_ROBUST_QUARTILE) {
        // use simple stats code (old verions)
        // XXX this hack is just for testing, drop when I am happy with the psStats version of the values

        int imin = stats->min * values->n;
        int imax = stats->max * values->n;
        int npts = imax - imin + 1;

        if (!psVectorSort(values, values)) {
            if ((nFailures < 3) || (nFailures % 100 == 0)) {
                psError(PS_ERR_UNKNOWN, false, "Unable to sort values.(%d failures)\n", nFailures);
            }
            nFailures ++;
            psFree(values);
            return false;
        }

        // Subtract the median when we add the numbers, so we don't get numerical problems
        float median = npts % 2 ? 0.5 * (values->data.F32[npts/2 - 1] + values->data.F32[npts/2]) :
	    values->data.F32[npts/2];
        double value = 0;
        for (long i = imin; (i <= imax) && (i < values->n); i++) {
            value += values->data.F32[i] - median;
        }
        value = value / npts + median;
        stats->robustMedian = value;
        stats->robustUQ = values->data.F32[imax];
        stats->robustLQ = values->data.F32[imin];
    } else {
        if (!psVectorStats (stats, values, NULL, NULL, 0)) {
            if (psTraceGetLevel("psLib.imageops") >= 5) {
                FILE *f = fopen ("vector.dat", "w");
                int fd = fileno(f);
                p_psVectorPrint (fd, values, "values");
                fclose (f);
            }
            if ((nFailures < 3) || (nFailures % 100 == 0)) {
                psError(PS_ERR_UNKNOWN, false, "Unable to measure statistics for image background "
                        "(%dx%d, (row0,col0) = (%d,%d) (%d failures)\n",
                        image->numRows, image->numCols, image->row0, image->col0, nFailures);
            }
            nFailures ++;
            psFree(values);
            return false;
        }
        if (psTraceGetLevel("psLib.imageops") >= 6) {
            FILE *f = fopen ("vector.dat", "w");
            int fd = fileno(f);
            p_psVectorPrint (fd, values, "values");
            fclose (f);
        }
    }

    psFree(values);

    return true;
}


/***  old comments

 // Subsample all pixels
 // This is not optimal, since there may be a large masked fraction that leaves us with few good pixels.
 // In that case, you should have set Nsubset.......

 // 2011/03/22 - MWV:  On prompting from Gene, changed the sampling so that it doesn't build and then sort 
 //     a huge array when Nsubset << Npixel.  
 //   O(Npixel) log(Npixel) is unnecessarily painful when Npixel=38 million (one skycell) 
 //     but we only needed Nsubset pixels and Nsubset << Npixel.

 //   It also perhaps partially defeats any gain of taking only subsamples if we're sorting the array.
 //   I should do some performance tests on this anyway.

 // If our chance of collision is low and the gain from not sorting the array is likely to be high, 
 // then just pick randomnly
 // Note that we're guaranteeing Nsubset pixels returned here (up to the limit of Npixels)

 // 2011/03/16 - MWV:  Fixed double-sampling of sky pixels.
 //   The pick-a-random-pixel-at-a-time approach  doesn't work well when Npixels is close to Nsubset
 //   If Nsubset is 0.8*Npixels, then we will get lots of pixels double-counted
 //   This is correct on average, but isn't the optimal way to estimate the sky level
 // Instead we take the following approach:
 //   1) Calculate a random ordering of the pixels
 //   2) Go through this ordering up to Nsubset to select pixels
 // This is O(n log(n))

 ***/
