/** @file  pmTrend2D.c
 *
 *  @author EAM, IfA
 *
 *  @version $Revision: 1.12 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-01-27 06:39:38 $
 *  Copyright 2004 Institute for Astronomy, University of Hawaii
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <strings.h>
#include <pslib.h>
#include "pmTrend2D.h"

static void pmTrend2DFree(pmTrend2D *trend)
{
    psFree(trend->stats);
    psFree(trend->poly);
    psFree(trend->map);
    return;
}

pmTrend2D *pmTrend2DAlloc(pmTrend2DMode mode, psImage *image, int nXtrend, int nYtrend, psStats *stats)
{
    if (mode == PM_TREND_MAP) {
        psAssert(image, "Need an image for MAP trend mode");
    }

    pmTrend2D *trend = psAlloc(sizeof(pmTrend2D));
    psMemSetDeallocator(trend, (psFreeFunc)pmTrend2DFree);

    trend->map = NULL;
    trend->poly = NULL;
    trend->stats = psMemIncrRefCounter(stats);
    trend->mode = mode;

    switch (mode) {
      case PM_TREND_POLY_ORD:
        trend->poly = psPolynomial2DAlloc(PS_POLYNOMIAL_ORD, nXtrend, nYtrend);
        // set masking somehow
        for (int nx = 0; nx < trend->poly->nX + 1; nx++) {
            for (int ny = 0; ny < trend->poly->nY + 1; ny++) {
                if (nx + ny >= PS_MAX (trend->poly->nX, trend->poly->nY) + 1) {
                    trend->poly->coeffMask[nx][ny] = PS_POLY_MASK_SET;
                } else {
                    trend->poly->coeffMask[nx][ny] = PS_POLY_MASK_NONE;
                }
            }
        }
        break;

      case PM_TREND_POLY_CHEB:
        trend->poly = psPolynomial2DAlloc(PS_POLYNOMIAL_CHEB, nXtrend, nYtrend);
        break;

      case PM_TREND_MAP: {
          // binning defines the map scale relationship
          psImageBinning *binning = psImageBinningAlloc();
          binning->nXruff = nXtrend;
          binning->nYruff = nYtrend;
          binning->nXfine = image->numCols;
          binning->nYfine = image->numRows;

          trend->map = psImageMapAlloc(image, binning, stats);
          psFree(binning);
          break;
      }
      // XXX: Put a more graceful error here.
      default:
        psAbort("error");
    }
    return trend;
}

bool psMemCheckTrend2D(psPtr ptr)
{
    PS_ASSERT_PTR(ptr, false);
    return ( psMemGetDeallocator(ptr) == (psFreeFunc) pmTrend2DFree);
}

pmTrend2D *pmTrend2DNoImageAlloc(pmTrend2DMode mode, psImageBinning *binning, psStats *stats)
{
    if (mode == PM_TREND_MAP) {
        psAssert(binning, "Need binning for MAP mode");
    }
    pmTrend2D *trend = psAlloc(sizeof(pmTrend2D));
    psMemSetDeallocator(trend, (psFreeFunc)pmTrend2DFree);

    trend->map = NULL;
    trend->poly = NULL;
    trend->stats = psMemIncrRefCounter(stats);
    trend->mode = mode;

    switch (mode) {
      case PM_TREND_POLY_ORD:
        trend->poly = psPolynomial2DAlloc(PS_POLYNOMIAL_ORD, binning->nXruff, binning->nYruff);
        // set masking somehow
        for (int nx = 0; nx < trend->poly->nX + 1; nx++) {
            for (int ny = 0; ny < trend->poly->nY + 1; ny++) {
                if (nx + ny >= PS_MAX (trend->poly->nX, trend->poly->nY) + 1) {
                    trend->poly->coeffMask[nx][ny] = PS_POLY_MASK_SET;
                } else {
                    trend->poly->coeffMask[nx][ny] = PS_POLY_MASK_NONE;
                }
            }
        }
        break;

      case PM_TREND_POLY_CHEB:
        trend->poly = psPolynomial2DAlloc(PS_POLYNOMIAL_CHEB, binning->nXruff, binning->nYruff);
        break;

      case PM_TREND_MAP: {
          // binning defines the map scale relationship
          trend->map = psImageMapNoImageAlloc(binning, stats);
          break;
      }

      default:
        psAbort("error");
    }
    return trend;
}

pmTrend2D *pmTrend2DFieldAlloc(pmTrend2DMode mode, int nXfield, int nYfield,
                               int nXtrend, int nYtrend, psStats *stats)
{
    psAssert(stats, "Require statistics");

    pmTrend2D *trend = psAlloc(sizeof(pmTrend2D));
    psMemSetDeallocator(trend, (psFreeFunc)pmTrend2DFree);

    trend->map = NULL;
    trend->poly = NULL;
    trend->stats = psMemIncrRefCounter(stats);
    trend->mode = mode;

    switch (mode) {
      case PM_TREND_POLY_ORD:
        trend->poly = psPolynomial2DAlloc(PS_POLYNOMIAL_ORD, nXtrend, nYtrend);
        // set masking somehow
        for (int nx = 0; nx < trend->poly->nX + 1; nx++) {
            for (int ny = 0; ny < trend->poly->nY + 1; ny++) {
                if (nx + ny >= PS_MAX (trend->poly->nX, trend->poly->nY) + 1) {
                    trend->poly->coeffMask[nx][ny] = PS_POLY_MASK_SET;
                } else {
                    trend->poly->coeffMask[nx][ny] = PS_POLY_MASK_NONE;
                }
            }
        }
        break;

      case PM_TREND_POLY_CHEB:
        trend->poly = psPolynomial2DAlloc(PS_POLYNOMIAL_CHEB, nXtrend, nYtrend);
        break;

      case PM_TREND_MAP: {
          // binning defines the map scale relationship
          psImageBinning *binning = psImageBinningAlloc();
          binning->nXfine = nXfield;
          binning->nYfine = nYfield;
          binning->nXruff = nXtrend;
          binning->nYruff = nYtrend;

          trend->map = psImageMapAlloc(NULL, binning, stats);
          psFree (binning);
          break;
      }

      default:
        // XXX: Put a more graceful error here.
        psAbort("error");
    }
    return trend;
}

bool pmTrend2DFit(bool *pGoodFit, pmTrend2D *trend, psVector *mask, psVectorMaskType maskVal, const psVector *x,
                  const psVector *y, const psVector *f, const psVector *df)
{
    PM_ASSERT_TREND2D_NON_NULL(trend, false);
    PM_ASSERT_TREND2D_STATS(trend, false);
    PS_ASSERT_VECTOR_NON_NULL(x, false);
    PS_ASSERT_VECTOR_NON_NULL(y, false);
    PS_ASSERT_VECTOR_NON_NULL(f, false);

    bool status = false;
    *pGoodFit = false;
    // for the psImageMap fit, it is possible to have valid data but no valid solution for
    // example, an isolated cell may not be reached from other cells, making the solution
    // degenerate.  psImageMapFit should probably handle this case, but until it does, we allow
    // it to fail on the result, but not yield an error (pGoodFit = false).
    // psVectorClipFitPolynomial2D can not fail in this way (really?), so pGoodFit is always
    // true

    switch (trend->mode) {
      case PM_TREND_POLY_ORD:
      case PM_TREND_POLY_CHEB:
        status = psVectorClipFitPolynomial2D(trend->poly, trend->stats, mask, maskVal, f, df, x, y);
        // we can use the API here which adjusts the polynomial order based on the number
        // of points in the image, and potentially based on the fractional range of the
        // data?
	*pGoodFit = true;
        break;

      case PM_TREND_MAP:
        // XXX supply fraction from trend elements
        // XXX need to add the API which adjusts the scale
        status = psImageMapClipFit(pGoodFit, trend->map, trend->stats, mask, maskVal, x, y, f, df);
	if (!status) {
	  psError(PS_ERR_UNKNOWN, true, "failed to build PSF model map");
	  return false;
	}

	// the psf model map can have nan pixels: repair this by extrapolation / interpolation
	// XXX TEST: p_psImagePrint(0, trend->map->map, "before");
	status = psImageMapRepair (trend->map->map);
	// XXX TEST: p_psImagePrint(0, trend->map->map, "after");
	if (!status) {
	  psError(PS_ERR_UNKNOWN, true, "failed to repair PSF model map");
	  return false;
	}
        break;

      default:
        psAbort ("error");
    }
    return status;
}

bool psImageMapRepair (psImage *map) {


  // XXX why is my repair not working??

    // patch over bad regions (use average of 8 possible neighbor pixels)
    // XXX consider testing all pixels against the 8 neighbors and replacing outliers...
    double Count = 0;                   // number of good pixels
    double Value = 0;                   // sum of good pixel's value
    for (int iy = 0; iy < map->numRows; iy++) {
        for (int ix = 0; ix < map->numCols; ix++) {
            if (isfinite(map->data.F32[iy][ix])) {
                Value += map->data.F32[iy][ix];
                Count++;
                continue;
            }

            double value = 0;
            double count = 0;
            for (int jy = iy - 1; jy <= iy + 1; jy++) {
                if (jy <   0) continue;
                if (jy >= map->numRows) continue;
                for (int jx = ix - 1; jx <= ix + 1; jx++) {
                    if (!jx && !jy) continue;
                    if (jx   <   0) continue;
                    if (jx   >= map->numCols) continue;
		    if (!isfinite(map->data.F32[jy][jx])) continue;
                    value += map->data.F32[jy][jx];
                    count += 1.0;
                }
            }
            if (count > 0) {
	      // psLogMsg ("psphot", PS_LOG_DETAIL, "patching image map %d, %d: %f (%d pts)\n", ix, iy, (value / count), (int) count);
		map->data.F32[iy][ix] = value / count;
	    }
        }
    }
    if (Count == 0) {
        psError(PS_ERR_UNKNOWN, true, "failed to repair PSF model map");
        return false;
    }
    Value /= Count;

    // patch over remaining bad regions (use global average)
    for (int iy = 0; iy < map->numRows; iy++) {
        for (int ix = 0; ix < map->numCols; ix++) {
            if (!isnan(map->data.F32[iy][ix])) continue;
            map->data.F32[iy][ix] = Value;
        }
    }
    return true;
}

double pmTrend2DEval(const pmTrend2D *trend, float x, float y)
{
    // This might be in a tight loop, so no complicated assertions
    if (!trend) {
        return 0.0;
    }

    double result;
    switch (trend->mode) {
      case PM_TREND_POLY_ORD:
      case PM_TREND_POLY_CHEB:
        result = psPolynomial2DEval(trend->poly, x, y);
        break;

      case PM_TREND_MAP:
        result = psImageMapEval(trend->map, x, y);
        break;

      default:
        psAbort ("error");
    }
    return result;
}

psVector *pmTrend2DEvalVector(const pmTrend2D *trend, psVector *mask, psVectorMaskType maskValue, const psVector *x, const psVector *y)
{
    PM_ASSERT_TREND2D_NON_NULL(trend, NULL);
    PS_ASSERT_VECTOR_NON_NULL(x, false);
    PS_ASSERT_VECTOR_NON_NULL(y, false);
    psVector *result;

    switch (trend->mode) {
      case PM_TREND_POLY_ORD:
      case PM_TREND_POLY_CHEB:
	// XXX supply a mask here as well.
        result = psPolynomial2DEvalVector (trend->poly, x, y);
        break;

      case PM_TREND_MAP:
        result = psImageMapEvalVector (trend->map, mask, maskValue, x, y);
        break;

      default:
        psAbort ("error");
    }
    return result;
}

psString pmTrend2DModeToString(pmTrend2DMode mode)
{
    switch (mode) {
      case PM_TREND_NONE:
        return psStringCopy("NONE");
      case PM_TREND_POLY_ORD:
        return psStringCopy("POLY_ORD");
        break;
      case PM_TREND_POLY_CHEB:
        return psStringCopy("POLY_CHEB");
      case PM_TREND_MAP:
        return psStringCopy("MAP");
        break;
      default:
        psError(PS_ERR_UNKNOWN, true, "Unknown pmTrend2D mode");
    }
    psAbort("invalid mode %d", mode);
}

pmTrend2DMode pmTrend2DModeFromString(psString name)
 {
    if (!name) {
        return PM_TREND_NONE;
    }

    if (!strcasecmp(name, "NONE")) {
        return PM_TREND_NONE;
    }
    if (!strcasecmp(name, "POLY_ORD")) {
        return PM_TREND_POLY_ORD;
    }
    if (!strcasecmp(name, "POLY_CHEB")) {
        return PM_TREND_POLY_CHEB;
    }
    if (!strcasecmp(name, "MAP")) {
        return PM_TREND_MAP;
    }
    psError(PS_ERR_UNKNOWN, true, "Unknown pmTrend2D mode %s", name);
    return PM_TREND_NONE;
}

bool pmTrend2DPrintMap (pmTrend2D *trend) {

    if (!trend->map) return false;
    if (!trend->map->map) return false;

    for (int j = 0; j < trend->map->map->numRows; j++) {
        for (int i = 0; i < trend->map->map->numCols; i++) {
            fprintf (stderr, "%5.2f  ", trend->map->map->data.F32[j][i]);
        }
        fprintf (stderr, "\t\t\t");
        for (int i = 0; i < trend->map->map->numCols; i++) {
            fprintf (stderr, "%5.2f  ", trend->map->error->data.F32[j][i]);
        }
        fprintf (stderr, "\n");
    }
    return true;
}

