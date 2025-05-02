/** @file  psImageMap.c
 *
 *  @brief Functions define a 2d coarse representation of a finer 2D field
 *
 *  @ingroup Image
 *
 *  @author Eugene Magnier, IfA
 *
 *  @version $Revision: 1.13 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-01-27 06:39:37 $
 *
 *  Copyright 2007 Institute for Astronomy, University of Hawaii
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>

#include "psError.h"
#include "psAbort.h"

#include "psFits.h"
#include "psAssert.h"
#include "psRegion.h"
#include "psFitsImage.h"

#include "psMemory.h"
#include "psVector.h"
#include "psImage.h"
#include "psStats.h"
#include "psImageBinning.h"
#include "psImagePixelInterpolate.h"
#include "psImageUnbin.h"

#include "psImageMap.h"

static void imageMapFree(psImageMap *map)
{
    psFree(map->map);
    psFree(map->error);
    psFree(map->stats);
    psFree(map->binning);

    return;
}

psImageMap *psImageMapAlloc(const psImage *field, psImageBinning *binning, psStats *stats)
{
    psAssert(field, "Require field image");
    psAssert(binning, "Require binning");
    // stats may be NULL

    psImageMap *map = psAlloc(sizeof(psImageMap));
    psMemSetDeallocator(map, (psFreeFunc)imageMapFree);

    map->binning = psMemIncrRefCounter(binning);
    map->stats   = psMemIncrRefCounter(stats);

    map->col0 = field->col0;
    map->row0 = field->row0;
    map->numCols = field->numCols;
    map->numRows = field->numRows;

    map->map = psImageAlloc(binning->nXruff, binning->nYruff, PS_TYPE_F32);
    psImageInit(map->map, 0.0);

    map->error = psImageAlloc(binning->nXruff, binning->nYruff, PS_TYPE_F32);
    psImageInit(map->error, 0.0);

    psImageBinningSetScale(map->binning, PS_IMAGE_BINNING_CENTER);
    psImageBinningSetSkipByOffset(map->binning, map->col0, map->row0);

    return map;
}

bool psMemCheckImageMap(psPtr ptr)
{
    PS_ASSERT_PTR(ptr, false);
    return (psMemGetDeallocator(ptr) == (psFreeFunc)imageMapFree);
}

// allocate the image map using the psImageBinning supplied
psImageMap *psImageMapNoImageAlloc(psImageBinning *binning, psStats *stats)
{
    psAssert(binning, "Require binning");
    // stats may be NULL

    psImageMap *map = psAlloc(sizeof(psImageMap));
    psMemSetDeallocator(map, (psFreeFunc)imageMapFree);

    map->binning = psMemIncrRefCounter(binning);
    map->stats   = psMemIncrRefCounter(stats);

    map->col0 = 0;
    map->row0 = 0;
    map->numCols = 0;
    map->numRows = 0;

    map->map = psImageAlloc(binning->nXruff, binning->nYruff, PS_TYPE_F32);
    psImageInit(map->map, 0.0);

    map->error = psImageAlloc(binning->nXruff, binning->nYruff, PS_TYPE_F32);
    psImageInit(map->error, 0.0);

    return map;
}

bool psImageMapModifyScale(psImageMap *map, int nXruff, int nYruff)
{
    PS_ASSERT_IMAGE_MAP_NON_NULL(map, false);

    map->binning->nXruff = nXruff;
    map->binning->nYruff = nYruff;

    map->map = psImageRecycle(map->map, nXruff, nYruff, PS_TYPE_F32);
    map->error = psImageRecycle(map->error, nXruff, nYruff, PS_TYPE_F32);

    psImageBinningSetScale(map->binning, PS_IMAGE_BINNING_CENTER);
    psImageBinningSetSkipByOffset(map->binning, map->col0, map->row0);

    return true;
}

// generate a psImageMap (or NULL) with the given number of superpixels in X and Y
// this function returns an error if the output map has impossible holes
bool psImageMapGenerate(psImageMap *map, const psVector *x, const psVector *y,
                        const psVector *f, const psVector *df, float badFrac)
{
    PS_ASSERT_IMAGE_MAP_NON_NULL(map, false);
    PS_ASSERT_IMAGE_MAP_STATS(map, false);
    PS_ASSERT_VECTOR_NON_NULL(x, false);
    PS_ASSERT_VECTOR_TYPE(x, PS_TYPE_F32, false);
    PS_ASSERT_VECTOR_NON_NULL(y, false);
    PS_ASSERT_VECTOR_TYPE(y, PS_TYPE_F32, false);
    PS_ASSERT_VECTORS_SIZE_EQUAL(x, y, false);
    PS_ASSERT_VECTOR_NON_NULL(f, false);
    PS_ASSERT_VECTOR_TYPE(f, PS_TYPE_F32, false);
    PS_ASSERT_VECTORS_SIZE_EQUAL(x, f, false);
    if (df) {
        PS_ASSERT_VECTOR_NON_NULL(df, false);
        PS_ASSERT_VECTOR_TYPE(df, PS_TYPE_F32, false);
        PS_ASSERT_VECTORS_SIZE_EQUAL(x, df, false);
    }

    psImage *mask = psImageAlloc (map->map->numCols, map->map->numRows, PS_TYPE_IMAGE_MASK);
    psImage *xCoord = psImageAlloc (map->map->numCols, map->map->numRows, PS_TYPE_F32);
    psImage *yCoord = psImageAlloc (map->map->numCols, map->map->numRows, PS_TYPE_F32);

    // accumulate the values for each map pixel

    // we can do this by accumulating a vector of pixel indexes for each cell
    psArray *pixelSets = psArrayAlloc (map->map->numCols*map->map->numRows);
    for (int i = 0; i < pixelSets->n; i++) {
        pixelSets->data[i] = psVectorAllocEmpty (4, PS_TYPE_S32);
    }
    // associate each value with a cell
    for (int i = 0; i < x->n; i++) {
        int xRuff = psImageBinningGetRuffX (map->binning, x->data.F32[i]);
        int yRuff = psImageBinningGetRuffY (map->binning, y->data.F32[i]);

        int bin = xRuff + yRuff*map->map->numCols;
        assert (bin >= 0);
        assert (bin < pixelSets->n);

        psVector *pixels = pixelSets->data[bin];
        pixels->data.S32[pixels->n] = i;
        psVectorExtend (pixels, 4, 1);
    }

    // stats structure for getting the position centers
    psStats *meanStat = psStatsAlloc (PS_STAT_SAMPLE_MEAN);

    // accumulate the x,y coords for each point to calculate the mean position.
    int Nx = map->map->numCols;
    int Ny = map->map->numRows;
    for (int iy = 0; iy < Ny; iy++) {
        for (int ix = 0; ix < Nx; ix++) {

            // pixel index for this cell
            psVector *pixels = pixelSets->data[ix + iy*Nx];

            // storage vectors
            psVector *xCell = psVectorAlloc (pixels->n, PS_TYPE_F32);
            psVector *yCell = psVectorAlloc (pixels->n, PS_TYPE_F32);
            psVector *fCell = psVectorAlloc (pixels->n, PS_TYPE_F32);

            // error vector, if needed
            psVector *dfCell = NULL;
            if (df) {
                dfCell = psVectorAlloc (pixels->n, PS_TYPE_F32);
            }

            // collect data for this cell
            for (int i = 0; i < pixels->n; i++) {
                int bin = pixels->data.S32[i];
                // convert x,y in the fine image to the ruff image
                xCell->data.F32[i]  = psImageBinningGetRuffX (map->binning, x->data.F32[bin]);
                yCell->data.F32[i]  = psImageBinningGetRuffY (map->binning, y->data.F32[bin]);
                fCell->data.F32[i]  = f->data.F32[bin];
                if (df) {
                    dfCell->data.F32[i] = df->data.F32[bin];
                }
            }

            // reset the stats to avoid contamination from the previous loop
            psStatsInit (map->stats);

            // get the value
            // XXX need to supply a mask and skip the masked pixels when calculating the centroid
            // this will not in general be properly weighted...
            if (!psVectorStats (map->stats, fCell, dfCell, NULL, 0)) {
		psError(PS_ERR_UNKNOWN, false, "failure to measure stats");
		return false;
	    }

	    // XXX ensure only one option is selected, or save both position and width
	    map->map->data.F32[iy][ix] = psStatsGetValue (map->stats, map->stats->options);

	    if (isnan(map->map->data.F32[iy][ix])) {
                mask->data.PS_TYPE_IMAGE_MASK_DATA[iy][ix] = 1;
	    } else {
                mask->data.PS_TYPE_IMAGE_MASK_DATA[iy][ix] = 0;

                // calculate the mean position and save:
                psStatsInit (meanStat);
                if (!psVectorStats (meanStat, xCell, NULL, NULL, 0)) {
		    psError(PS_ERR_UNKNOWN, false, "failure to measure stats");
		    return false;
		}
                xCoord->data.F32[iy][ix] = psStatsGetValue (meanStat, meanStat->options);
                psStatsInit (meanStat);
                if (!psVectorStats (meanStat, yCell, NULL, NULL, 0)) {
		    psError(PS_ERR_UNKNOWN, false, "failure to measure stats");
		    return false;
		}
                yCoord->data.F32[iy][ix] = psStatsGetValue (meanStat, meanStat->options);
            }

            psFree (xCell);
            psFree (yCell);
            psFree (fCell);
            psFree (dfCell);
        }
    }
    psFree (pixelSets);
    psFree (meanStat);
   // at this point, for each map pixel, we have (f,x,y), or the pixel is masked.


#if 0
    {
        psFits *fits = psFitsOpen ("imageMap.raw.fits", "w");
        psFitsWriteImage (fits, NULL, map->map, 0, NULL);
        psFitsClose (fits);
    }
#endif

    // did this analysis succeed?  (enough good or OK pixels?)
    psImage *state = psImagePixelInterpolateState (&map->nBad, &map->nPoor, mask, 0xff);
    map->nGood = mask->numCols * mask->numRows - map->nBad - map->nPoor;
    if (map->nBad > badFrac * mask->numCols * mask->numRows) {
        psFree (xCoord);
        psFree (yCoord);
        psFree (mask);
        return false;
    }

    // fit the valid pixels to (0,1,2) order polynomials, interpolate values to the pixel center
    // XXX I need to be careful about the pixel coordinates: center is 0,0 or 0.5, 0.5?
    psImagePixelInterpolateCenter (map->map, xCoord, yCoord, state, mask, 0xff);
    psFree (xCoord);
    psFree (yCoord);

#if 0
    {
        psFits *fits = psFitsOpen ("imageMap.ref.fits", "w");
        psFitsWriteImage (fits, NULL, map->map, 0, NULL);
        psFitsClose (fits);
    }
#endif

    psImagePixelInterpolatePoor (map->map, state, mask, 0xff);

#if 0
    {
        psFits *fits = psFitsOpen ("imageMap.fix.fits", "w");
        psFitsWriteImage (fits, NULL, map->map, 0, NULL);
        psFitsClose (fits);
    }
#endif

    psFree (state);
    psFree (mask);
    return true;
}

// using the points given, generate a map with maximum resolution that yields only good and ok pixels
bool psImageMapGenerateScale(psImageMap *map, const psVector *x, const psVector *y,
                             const psVector *f, const psVector *df, float badFrac)
{
    PS_ASSERT_IMAGE_MAP_NON_NULL(map, false);
    PS_ASSERT_IMAGE_MAP_STATS(map, false);
    PS_ASSERT_VECTOR_NON_NULL(x, false);
    PS_ASSERT_VECTOR_TYPE(x, PS_TYPE_F32, false);
    PS_ASSERT_VECTOR_NON_NULL(y, false);
    PS_ASSERT_VECTOR_TYPE(y, PS_TYPE_F32, false);
    PS_ASSERT_VECTORS_SIZE_EQUAL(x, y, false);
    PS_ASSERT_VECTOR_NON_NULL(f, false);
    PS_ASSERT_VECTOR_TYPE(f, PS_TYPE_F32, false);
    PS_ASSERT_VECTORS_SIZE_EQUAL(x, f, false);
    if (df) {
        PS_ASSERT_VECTOR_NON_NULL(df, false);
        PS_ASSERT_VECTOR_TYPE(df, PS_TYPE_F32, false);
        PS_ASSERT_VECTORS_SIZE_EQUAL(x, df, false);
    }

    int nXruff, nYruff;

    while (!psImageMapGenerate (map, x, y, f, df, badFrac)) {
        // if we failed to build an acceptable map, decrease nXruff, nYruff as appropriate, and
        // try again...  try to keep the aspect ratio.

        // if both axes are at 1, give up
        if ((map->binning->nXruff == 1) && (map->binning->nYruff == 1)) {
            return false;
        }

        // if one axis is at 1, decrement the other
        if (map->binning->nXruff == 1) {
            nXruff = map->binning->nXruff;
            nYruff = map->binning->nYruff - 1;
            psImageMapModifyScale (map, nXruff, nYruff);
            continue;
        }
        if (map->binning->nYruff == 1) {
            nYruff = map->binning->nYruff;
            nXruff = map->binning->nXruff - 1;
            psImageMapModifyScale (map, nXruff, nYruff);
            continue;
        }

        // otherwise, decrement the larger axis, and set the smaller based
        // on the aspect ratio
        float aRatio = map->binning->nXruff / map->binning->nYruff;
        if (map->binning->nXruff > map->binning->nYruff) {
            nXruff = map->binning->nXruff - 1;
            nYruff = (int)(0.5 + (nXruff / aRatio));
        } else {
            nYruff = map->binning->nYruff - 1;
            nXruff = (int)(0.5 + (nYruff * aRatio));
        }

        psImageMapModifyScale (map, nXruff, nYruff);
    }
    return true;
}

// x,y are in fractional pixel coords of the fine image (pixel center: 0.5)
double psImageMapEval(const psImageMap *map, float x, float y)
{
    // This may be called in a tight loop, so no assertions
    return psImageUnbinPixel(x, y, map->map, map->binning);
}

psVector *psImageMapEvalVector(const psImageMap *map, const psVector *mask, psVectorMaskType maskValue, const psVector *x, const psVector *y)
{
    PS_ASSERT_IMAGE_MAP_NON_NULL(map, NULL);
    PS_ASSERT_VECTOR_NON_NULL(x, NULL);
    PS_ASSERT_VECTOR_TYPE(x, PS_TYPE_F32, NULL);
    PS_ASSERT_VECTOR_NON_NULL(y, NULL);
    PS_ASSERT_VECTOR_TYPE(y, PS_TYPE_F32, NULL);
    PS_ASSERT_VECTORS_SIZE_EQUAL(x, y, NULL);

    psVector *result = psVectorAlloc(x->n, PS_TYPE_F32); // Results to return

    for (int i = 0; i < x->n; i++) {
      if (mask && (mask->data.PS_TYPE_VECTOR_MASK_DATA[i] & maskValue)) {
	result->data.F32[i] = 0.0;
      } else {	
	result->data.F32[i] = psImageUnbinPixel(x->data.F32[i], y->data.F32[i], map->map, map->binning);
      }
    }

    return result;
}

bool psImageMapCleanup (psImageMap *map) {

    if ((map->map->numRows == 1) && (map->map->numCols == 1)) return true;

    // find the weighted average of all pixels
    float Sum = 0.0;
    float Wt = 0.0;
    for (int j = 0; j < map->map->numRows; j++) {
        for (int i = 0; i < map->map->numCols; i++) {
            if (!isfinite(map->map->data.F32[j][i])) continue;
            Sum += map->map->data.F32[j][i] * map->error->data.F32[j][i];
            Wt += map->error->data.F32[j][i];
        }
    }

    float Mean = Sum / Wt;

    // do any of the pixels in the map need to be repaired?
    // XXX for now, we are just replacing bad pixels with the Mean
    for (int j = 0; j < map->map->numRows; j++) {
        for (int i = 0; i < map->map->numCols; i++) {
            if (isfinite(map->map->data.F32[j][i]) &&
                (map->error->data.F32[j][i] > 0.0)) continue;
            map->map->data.F32[j][i] = Mean;
        }
    }
    return true;
}

