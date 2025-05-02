/** @file  pmAstrometryObjects.c
*
*  @brief This file defines the basic types for matching objects
*  based on their astrometry.
*
*  @ingroup AstroImage
*
*  @author EAM, IfA
*
*  @version $Revision: 1.45 $ $Name: not supported by cvs2svn $
*  @date $Date: 2009-02-09 21:25:20 $
*
*  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


/******************************************************************************/
/*  INCLUDE FILES                                                             */
/******************************************************************************/
#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <unistd.h>   // for unlink
#include <pslib.h>

#include "pmHDU.h"
#include "pmFPA.h"
#include "pmFPAExtent.h"
#include "pmFPAfile.h"
#include "pmAstrometryObjects.h"
#include "pmKapaPlots.h"
#include "pmAstrometryVisual.h"

// XXX this is defined in pmPSFtry.h, which makes no sense
float psVectorSystematicError (psVector *residuals, psVector *errors, float clipFraction);
float pmAstrom2DSystematics (psVector *xPos, psVector *yPos, psVector *value);
float pmAstromSubsetSystematics (psVector *value);

#define PM_ASTROMETRYOBJECTS_DEBUG 1

/******************************************************************************
pmAstromObjSortByMag(**a, **b): sort by mag (descending)

Is this a private routine?
Should we do the early asserts?
 ******************************************************************************/
int pmAstromObjSortByMag(
    const void **a,
    const void **b)
{
    if (PM_ASTROMETRYOBJECTS_DEBUG) {
        PS_ASSERT_PTR_NON_NULL(a, 0);
        PS_ASSERT_PTR_NON_NULL(*a, 0);
        PS_ASSERT_PTR_NON_NULL(b, 0);
        PS_ASSERT_PTR_NON_NULL(*b, 0);
    }

    pmAstromObj *A = *(pmAstromObj **)a;
    pmAstromObj *B = *(pmAstromObj **)b;

    psF32 diff = A->Mag - B->Mag;
    if (diff > FLT_EPSILON) {
        return (-1);
    }

    if (diff < FLT_EPSILON) {
        return (+1);
    }

    return (0);
}

/************************************************************************************************************/
/*
 * Working routine to match two lists (where x[12] are sorted), given psVectors of their coordinates and the
 * permutation used to sort in x
 */
static psArray *match_lists(const psVector *x1, const psVector *y1, // x/y coordinates of first set of objects
                            const psVector *x2, const psVector *y2, // x/y   "    "    "  second "   "  "   "
                            const psVector *sorted1, const psVector *sorted2, // mapping to original order
                            const double RADIUS) // matching radius
{
    psArray *matches = psArrayAllocEmpty(x1->n);
    psVector *found1 = psVectorAlloc(x1->n, PS_TYPE_S8);
    psVector *found2 = psVectorAlloc(x2->n, PS_TYPE_S8);

    const double RADIUS_SQR = PS_SQR(RADIUS);
    double dX, dY, dR;

    psVectorInit (found1, 0);
    psVectorInit (found2, 0);

    int jStart;
    int i = 0, j = 0;
    while (i < x1->n && j < x2->n) {
        dX = x1->data.F64[i] - x2->data.F64[j];
        if (dX <= -RADIUS) {
            i++;
            continue;
        }
        if (dX >= +RADIUS) {
            j++;
            continue;
        }

        if (found1->data.S8[i]) {
            i++;
            continue;
        }
        if (found2->data.S8[j]) {
            j++;
            continue;
        }

        jStart = j;
        while ((fabs(dX) < RADIUS) && (j < x2->n)) {

            dX = x1->data.F64[i] - x2->data.F64[j];
            dY = y1->data.F64[i] - y2->data.F64[j];
            dR = dX*dX + dY*dY;

            if (dR > RADIUS_SQR) {
                j++;
                continue;
            }
            if (found2->data.S8[j]) {
                j++;
                continue;
            }

            // got a match; add to output list
            pmAstromMatch *match = pmAstromMatchAlloc (sorted1->data.S32[i], sorted2->data.S32[j]);
            psArrayAdd (matches, 100, match);
            psFree (match);

            found1->data.S8[i] = 1;
            found2->data.S8[j] = 1;

            j++;
        }
        j = jStart;
        i++;
    }
    psFree (found1);
    psFree (found2);

    return (matches);
}

/************************************************************************************************************/
// macro to generate code for radius match function based on desired member
// radius is in units of matching member (eg, pixels for chip, microns for FP, etc)
#define MAKE_ASTROM_RADIUS(FUNC, MEMBER) \
psArray *FUNC( \
               const psArray *st1, \
               const psArray *st2, \
               double RADIUS) \
{ \
    PS_ASSERT_PTR_NON_NULL(st1, NULL); \
    PS_ASSERT_PTR_NON_NULL(st2, NULL); \
    \
    assert(st1->n == 0 || pmAstromObjTest(st1->data[0])); \
    assert(st2->n == 0 || pmAstromObjTest(st2->data[0])); \
    \
    /* sort both lists by X coord; st1 first */ \
    psVector *x1 = psVectorAlloc(st1->n, PS_TYPE_F64); \
    for (int i = 0; i < st1->n; i++) { \
        x1->data.F64[i] = ((pmAstromObj *)st1->data[i])->MEMBER->x; \
    } \
    const psVector *sorted1 = psVectorSortIndex(NULL, x1); \
    assert (sorted1->type.type == PS_TYPE_S32); \
    \
    psVector *y1 = psVectorAlloc(st1->n, PS_TYPE_F64); \
    for (int i = 0; i < st1->n; i++) { \
        x1->data.F64[i] = ((pmAstromObj *)st1->data[sorted1->data.S32[i]])->MEMBER->x; \
        y1->data.F64[i] = ((pmAstromObj *)st1->data[sorted1->data.S32[i]])->MEMBER->y; \
    } \
    \
    /* now st2 */ \
    psVector *x2 = psVectorAlloc(st2->n, PS_TYPE_F64); \
    for (int i = 0; i < st2->n; i++) { \
        x2->data.F64[i] = ((pmAstromObj *)st2->data[i])->MEMBER->x; \
    } \
    const psVector *sorted2 = psVectorSortIndex(NULL, x2); \
    \
    psVector *y2 = psVectorAlloc(st2->n, PS_TYPE_F64); \
    for (int i = 0; i < st2->n; i++) { \
        x2->data.F64[i] = ((pmAstromObj *)st2->data[sorted2->data.S32[i]])->MEMBER->x; \
        y2->data.F64[i] = ((pmAstromObj *)st2->data[sorted2->data.S32[i]])->MEMBER->y; \
    } \
    /* Do the work */ \
    psArray *matches = match_lists(x1, y1, x2, y2, sorted1, sorted2, RADIUS); \
    \
    psFree(sorted1); \
    psFree(sorted2); \
    psFree(x1); \
    psFree(y1); \
    psFree(x2); \
    psFree(y2); \
    \
    psLogMsg (__func__, 3, "radius match: %ld pairs (radius: %f)\n", matches->n, RADIUS); \
    return (matches); \
}

/******************************************************************************/
/*
 * Match two lists of pmAstromObjs, based on the FP, TP, or chip coordinates
 */
MAKE_ASTROM_RADIUS(pmAstromRadiusMatch, FP)
MAKE_ASTROM_RADIUS(pmAstromRadiusMatchFP, FP)
MAKE_ASTROM_RADIUS(pmAstromRadiusMatchTP, TP)
MAKE_ASTROM_RADIUS(pmAstromRadiusMatchChip, chip)

/******************************************************************************
pmAstromMatchFit(map, raw, ref, match, stats): take two matched star lists
and fit a psPlaneTransform between them
 ******************************************************************************/
pmAstromFitResults *pmAstromMatchFit(
    psPlaneTransform *map,
    psArray *raw,
    psArray *ref,
    psArray *match,
    psStats *stats,
    const psMetadata *config)
{
    PS_ASSERT_PTR_NON_NULL(map, NULL);
    PS_ASSERT_PTR_NON_NULL(raw, NULL);
    PS_ASSERT_PTR_NON_NULL(ref, NULL);
    PS_ASSERT_PTR_NON_NULL(match, NULL);
    PS_ASSERT_PTR_NON_NULL(stats, NULL);
    PS_ASSERT_PTR_NON_NULL(config, NULL);

    // sigma of gaussian window to down-weight photometric outliers (ignored if NAN or 0.0)
    bool status;
    double photomWindowSigma  = psMetadataLookupF32 (&status, config, "PSASTRO.PHOTOM.WINDOW.SIGMA");
    bool   photomWindowApply  = !(isnan(photomWindowSigma) || (fabs(photomWindowSigma) < 0.01));
    double photomWindowFactor = photomWindowApply ? -0.5/PS_SQR(photomWindowSigma) : 0.0;

    // reassign values for clip fit
    // XXX set wt based on mag error?
    psVector *X = psVectorAlloc (match->n, PS_TYPE_F32);
    psVector *Y = psVectorAlloc (match->n, PS_TYPE_F32);
    psVector *x = psVectorAlloc (match->n, PS_TYPE_F32);
    psVector *y = psVectorAlloc (match->n, PS_TYPE_F32);
    psVector *wt = psVectorAlloc (match->n, PS_TYPE_F32);
    // take the matched stars, first fit
    for (int i = 0; i < match->n; i++) {
        pmAstromMatch *pair = match->data[i];
        pmAstromObj *rawStar = raw->data[pair->raw];
        pmAstromObj *refStar = ref->data[pair->ref];

        X->data.F32[i] = rawStar->chip->x;
        Y->data.F32[i] = rawStar->chip->y;

        x->data.F32[i] = refStar->FP->x;
        y->data.F32[i] = refStar->FP->y;

	// wt is used as an error (sqrt(variance)) in the fit. the 1.01 prevents the weight from going to 0.0 for perfect matches
        wt->data.F32[i] = 1.01 - exp(photomWindowFactor*PS_SQR(refStar->magCal - rawStar->magCal)); 
    }

    // constant errors
    psVector *mask = psVectorAlloc (match->n, PS_TYPE_VECTOR_MASK);
    psVectorInit (mask, 0);

    // the stats options supplied are used to perform the clip fitting
    pmAstromFitResults *results = pmAstromFitResultsAlloc();
    results->xStats = psStatsAlloc (PS_STAT_NONE);
    results->yStats = psStatsAlloc (PS_STAT_NONE);
    *results->xStats = *stats;
    *results->yStats = *stats;

    int nIter = stats->clipIter;

    results->xStats->clipIter = 1;
    results->yStats->clipIter = 1;

    // fit chip-to-FPA transformation
    // we run 'clipIter' cycles clipping in each of x and y, with only one iteration each
    // need to use the stats lookups functions to get the width and center
    for (int i = 0; i < nIter; i++) {
        if (!psVectorClipFitPolynomial2D (map->x, results->xStats, mask, 0xff, x, wt, X, Y)) {
            // psError(PS_ERR_UNKNOWN, false, "failure in clip-fitting for x\n");
            psLogMsg("psModule.astrom", 4, "failure in clip-fitting for x\n");
            psFree (x);
            psFree (y);
            psFree (X);
            psFree (Y);
            psFree (wt);
            psFree (mask);

            return results;
        }
        // psTrace ("psModules.astrom", 3, "x resid: %f +/- %f (%ld of %ld)\n", results->xStats->clippedMean, results->xStats->clippedStdev, results->xStats->clippedNvalues, x->n);
        psTrace ("psModules.astrom", 3, "x resid: %f +/- %f (%ld of %ld)\n", results->xStats->robustMedian, results->xStats->robustStdev, results->xStats->clippedNvalues, x->n);

        if (!psVectorClipFitPolynomial2D (map->y, results->yStats, mask, 0xff, y, wt, X, Y)) {
            // psError(PS_ERR_UNKNOWN, false, "failure in clip-fitting for y\n");
            psLogMsg("psModule.astrom", 4, "failure in clip-fitting for y\n");
            psFree (x);
            psFree (y);
            psFree (X);
            psFree (Y);
            psFree (wt);
            psFree (mask);

            return results;
        }
        // psTrace ("psModules.astrom", 3, "y resid: %f +/- %f (%ld of %ld)\n", results->yStats->clippedMean, results->yStats->clippedStdev, results->yStats->clippedNvalues, y->n);
        psTrace ("psModules.astrom", 3, "y resid: %f +/- %f (%ld of %ld)\n", results->yStats->robustMedian, results->yStats->robustStdev, results->yStats->clippedNvalues, y->n);
    }
    results->xStats->clipIter = stats->clipIter;
    results->yStats->clipIter = stats->clipIter;

    // *** calculate the 90%-ile and the systematic scatter for each direction.

    // generate the X residual vector
    psVector *xFit = psPolynomial2DEvalVector (map->x, X, Y);
    if (!xFit) abort();
    psVector *xRes = (psVector *) psBinaryOp (NULL, x, "-", xFit);
    if (!xRes) abort();
    psFree (xFit);

    psVector *yFit = psPolynomial2DEvalVector (map->y, X, Y);
    if (!yFit) abort();
    psVector *yRes = (psVector *) psBinaryOp (NULL, y, "-", yFit);
    if (!yRes) abort();
    psFree (yFit);

    // extract a high-quality subset (unmasked, S/N > XXX) and position errors
    // XXX for now, generate a position error based on the magnitude error
    psVector *xErr     = psVectorAllocEmpty (match->n, PS_TYPE_F32);
    psVector *yErr     = psVectorAllocEmpty (match->n, PS_TYPE_F32);
    psVector *xResGood = psVectorAllocEmpty (match->n, PS_TYPE_F32);
    psVector *yResGood = psVectorAllocEmpty (match->n, PS_TYPE_F32);

    // we measure the stdev of the median residual in NxN bins.
    // use only valid (not NAN) measurements
    psVector *xPosValid = psVectorAllocEmpty (match->n, PS_TYPE_F32);
    psVector *yPosValid = psVectorAllocEmpty (match->n, PS_TYPE_F32);
    psVector *xResValid = psVectorAllocEmpty (match->n, PS_TYPE_F32);
    psVector *yResValid = psVectorAllocEmpty (match->n, PS_TYPE_F32);

    for (int i = 0; i < match->n; i++) {
        if (mask->data.PS_TYPE_VECTOR_MASK_DATA[i]) continue;
        pmAstromMatch *pair = match->data[i];
        pmAstromObj *rawStar = raw->data[pair->raw];
        if (!isfinite(rawStar->dMag)) continue;

	bool isValid = true;
	isValid = isValid & isfinite (x->data.F32[i]);
	isValid = isValid & isfinite (y->data.F32[i]);
	isValid = isValid & isfinite (xRes->data.F32[i]);
	isValid = isValid & isfinite (yRes->data.F32[i]);
	if (isValid) {
	  psVectorAppend (xPosValid, x->data.F32[i]);
	  psVectorAppend (yPosValid, y->data.F32[i]);
	  psVectorAppend (xResValid, xRes->data.F32[i]);
	  psVectorAppend (yResValid, yRes->data.F32[i]);
	}

	// for the systematic error, use only high S/N stars
        if (rawStar->dMag > 0.02) continue;

        // two likely failure values: NAN or 0.0 --> use dMag in this case
        float xErrValue, yErrValue;
        if (isfinite(rawStar->chip->xErr) && (rawStar->chip->xErr > 0.0)) {
            xErrValue = rawStar->chip->xErr;
        } else {
            xErrValue = PS_MAX(0.005, rawStar->dMag);
        }
        if (isfinite(rawStar->chip->yErr) && (rawStar->chip->yErr > 0.0)) {
            yErrValue = rawStar->chip->yErr;
        } else {
            yErrValue = PS_MAX(0.005, rawStar->dMag);
        }

        psVectorAppend (xErr,     xErrValue);
        psVectorAppend (yErr,     yErrValue);
        psVectorAppend (xResGood, xRes->data.F32[i]);
        psVectorAppend (yResGood, yRes->data.F32[i]);
    }

    // if xResGood or yResGood have no valid data, set these to NAN 
    if ((xResGood->n == 0) || (yResGood->n == 0)) {
      results->dXsys   = results->dYsys   = NAN;
      results->dXrange = results->dYrange = NAN;
      results->dXstdev = results->dYstdev = NAN;
    } else {
      results->dXsys = psVectorSystematicError (xResGood, xErr, 0.05);
      results->dYsys = psVectorSystematicError (yResGood, yErr, 0.05);

      results->dXrange = pmAstromVectorRange (xResGood, 0.1, 0.9, results->xStats->clippedStdev);
      results->dYrange = pmAstromVectorRange (yResGood, 0.1, 0.9, results->yStats->clippedStdev);

      results->dXstdev = pmAstrom2DSystematics (xPosValid, yPosValid, xResValid);
      results->dYstdev = pmAstrom2DSystematics (xPosValid, yPosValid, yResValid);
    }

    psTrace ("psModules.astrom", 3, "dXsys: %f, dXrange: %f, dXstdev: %f\n", results->dXsys, results->dXrange, results->dXstdev);
    psTrace ("psModules.astrom", 3, "dYsys: %f, dYrange: %f, dYstdev: %f\n", results->dYsys, results->dYrange, results->dYstdev);

    psFree (xErr);
    psFree (yErr);
    psFree (xRes);
    psFree (yRes);
    psFree (xResGood);
    psFree (yResGood);
    psFree (xPosValid);
    psFree (yPosValid);
    psFree (xResValid);
    psFree (yResValid);

    psFree (x);
    psFree (y);
    psFree (X);
    psFree (Y);
    psFree (wt);
    psFree (mask);

    return (results);
}

# define VAL_COUNT 9.0
# define MIN_COUNT 5.0

float pmAstrom2DSystematics (psVector *xPos, psVector *yPos, psVector *value) {

    // pre-filter the values to ensure no NANs, other invalid

    // if we do not have enough measurements (< 25), use pmAstromSubsetSystematics instead (OK to 9 values)
    if (xPos->n < MIN_COUNT*2*2) {
      float result = pmAstromSubsetSystematics (value);
      return result;
    }

    // generate a grid covering the full range of x,y and measure the median value in each
    // grid cell.  calculate the stdev of those median values.

    // VAL_COUNT (9) is the (min) goal density, but a single cell may have only MIN_COUNT (7)
    // we have N points.  require a min of 9 pts per cell (configurable?).  grid is square.
    // Ncell*Ncell*9 = Npts, Ncell = MIN(sqrt(Npts/9), 5)

    int Ncell = PS_MIN(sqrt((float)(xPos->n / VAL_COUNT)), 2);  // have to at least have a 2x2 grid

    // find the range of x,y values
    float xMin = 1e9, xMax = -1e9, yMin = 1e9, yMax = 1e9;
    for (int i = 0; i < xPos->n; i++) {
	xMin = PS_MIN(xPos->data.F32[i],xMin);
	xMax = PS_MAX(xPos->data.F32[i],xMax);
	yMin = PS_MIN(yPos->data.F32[i],yMin);
	yMax = PS_MAX(yPos->data.F32[i],yMax);
    }

    float xStep = Ncell / (xMax - xMin);
    float yStep = Ncell / (yMax - yMin);
  
    if (isnan(xStep)) {
      return NAN;
    }
    if (isnan(yStep)) {
      return NAN;
    }

    psArray *xBin = psArrayAlloc (Ncell);
    for (int ix = 0; ix < Ncell; ix++) {
	psArray *yBin = psArrayAlloc (Ncell); 
	xBin->data[ix] = yBin;
	for (int iy = 0; iy < Ncell; iy++) {
	    yBin->data[iy] = psVectorAllocEmpty(128, PS_TYPE_F32);
	}
    }    

    // xValue = ix/xStep + xMin -> ix = (xValue - xMin) * xStep;

    for (int i = 0; i < xPos->n; i++) {
	int ix = PS_MIN(PS_MAX(0, (xPos->data.F32[i] - xMin) * xStep), Ncell - 1);
	int iy = PS_MIN(PS_MAX(0, (yPos->data.F32[i] - yMin) * yStep), Ncell - 1);
    
	psArray *yBin = xBin->data[ix];
	psVector *Bin = yBin->data[iy];

	psVectorAppend(Bin, value->data.F32[i]);
    }

    psVector *sample = psVectorAllocEmpty (Ncell*Ncell, PS_TYPE_F32);
    psStats *stats = psStatsAlloc (PS_STAT_SAMPLE_MEDIAN);

    // calculate the median for each vector and save on a vector
    for (int ix = 0; ix < Ncell; ix++) {
	psArray *yBin = xBin->data[ix];
	for (int iy = 0; iy < Ncell; iy++) {
	    psVector *Bin = yBin->data[iy];
	    if (Bin->n < MIN_COUNT) { continue; }

	    // psVectorStats resets stats so we can call this repeatedly
	    psVectorStats (stats, Bin, NULL, NULL, 0); 
	    if (isfinite(stats->sampleMedian)) {
		psVectorAppend (sample, stats->sampleMedian);
	    }
	}
    }    
  
    stats->options = PS_STAT_SAMPLE_STDEV;
    psVectorStats (stats, sample, NULL, NULL, 0); 
    float result = stats->sampleStdev;

    if (!isfinite(stats->sampleStdev)) {
      fprintf (stderr, "*** bad solution ***\n");
    }

    // NOTE: the elements of xBin are freed automatically, which extends down to the vectors
    psFree (xBin); 
    psFree (stats);
    psFree (sample);
  
    return result;
}

# define SUBSET_NSAMPLE 3
// last-ditch attempt to assess the systematic error in the data.  Just split into 3 bins,
// calculate median in each, and calculate stdev of the subset.  The minimum number of
// values needed to make this measurement in any sensible way : 3*3 = 6.
float pmAstromSubsetSystematics (psVector *value) {

    // give up if too few stars:
    if (value->n < 3*SUBSET_NSAMPLE) return NAN;

    psVector *sample = psVectorAlloc (SUBSET_NSAMPLE, PS_TYPE_F32);
    psStats *stats = psStatsAlloc (PS_STAT_SAMPLE_MEDIAN);

    // note that this drops the last 1 or 2 values if value->n % 3 = 1 or 2
    int nSubset = value->n / SUBSET_NSAMPLE;
    psVector *subset = psVectorAlloc (nSubset, PS_TYPE_F32);
    for (int iter = 0; iter < SUBSET_NSAMPLE; iter++) {
	for (int i = 0; i < nSubset; i++) {
	    subset->data.F32[i] = value->data.F32[i + nSubset*iter];
	}  

	// psVectorStats resets stats so we can call this repeatedly
	psVectorStats (stats, subset, NULL, NULL, 0); 
	if (isfinite(stats->sampleMedian)) {
	    sample->data.F32[iter] = stats->sampleMedian;
	}
    }

    stats->options = PS_STAT_SAMPLE_STDEV;
    psVectorStats (stats, sample, NULL, NULL, 0); 
    float result = stats->sampleStdev;

    if (!isfinite(stats->sampleStdev)) {
      fprintf (stderr, "*** bad solution (2) ***\n");
    }

    psFree (stats);
    psFree (sample);
    psFree (subset); 

    return result;
}

// set the bin closest to the corresponding value.  if USE_END is +/- 1,
// out-of-range saturates on lower/upper bin REGARDLESS of actual value
#define PS_BIN_FOR_VALUE(RESULT, VECTOR, VALUE, USE_END) { \
        psVectorBinaryDisectResult result; \
        psScalar tmpScalar; \
        tmpScalar.type.type = PS_TYPE_F32; \
        tmpScalar.data.F32 = (VALUE); \
        RESULT = psVectorBinaryDisect (&result, VECTOR, &tmpScalar); \
        switch (result) { \
          case PS_BINARY_DISECT_PASS: \
            break; \
          case PS_BINARY_DISECT_OUTSIDE_RANGE: \
            psTrace("psModules.astrom", 6, "selected bin outside range"); \
            if (USE_END == -1) { RESULT = 0; } \
            if (USE_END == +1) { RESULT = VECTOR->n - 1; } \
            break; \
          case PS_BINARY_DISECT_INVALID_INPUT: \
          case PS_BINARY_DISECT_INVALID_TYPE: \
            psAbort ("programming error"); \
            break; \
        } }

# define PS_BIN_INTERPOLATE(RESULT, VECTOR, BOUNDS, BIN, VALUE) { \
        float dX, dY, Xo, Yo, Xt; \
        if (BIN == BOUNDS->n - 1) { \
            dX = 0.5*(BOUNDS->data.F32[BIN+1] - BOUNDS->data.F32[BIN-1]); \
            dY = VECTOR->data.F32[BIN] - VECTOR->data.F32[BIN-1]; \
            Xo = 0.5*(BOUNDS->data.F32[BIN+1] + BOUNDS->data.F32[BIN]); \
            Yo = VECTOR->data.F32[BIN]; \
        } else { \
            dX = 0.5*(BOUNDS->data.F32[BIN+2] - BOUNDS->data.F32[BIN]); \
            dY = VECTOR->data.F32[BIN+1] - VECTOR->data.F32[BIN]; \
            Xo = 0.5*(BOUNDS->data.F32[BIN+1] + BOUNDS->data.F32[BIN]); \
            Yo = VECTOR->data.F32[BIN]; \
        } \
        if (dY != 0) { \
            Xt = (VALUE - Yo)*dX/dY + Xo; \
        } else { \
            Xt = Xo; \
        } \
        Xt = PS_MIN (BOUNDS->data.F32[BIN+1], PS_MAX(BOUNDS->data.F32[BIN], Xt)); \
        psTrace("psModules.astrom", 6, "(Xo, Yo, dX, dY, Xt, Yt) is (%.2f %.2f %.2f %.2f %.2f %.2f)\n", \
                Xo, Yo, dX, dY, Xt, VALUE); \
        RESULT = Xt; }

float pmAstromVectorRange (psVector *myVector, float minFrac, float maxFrac, float stdevGuess) {

    psStats *stats = psStatsAlloc(PS_STAT_MIN | PS_STAT_MAX); // Statistics for min and max
    psHistogram *histogram = NULL;      // Histogram of the data
    psHistogram *cumulative = NULL;     // Cumulative histogram of the data
    float min = NAN, max = NAN;         // Mimimum and maximum values

    // Get the minimum and maximum values
    // XXX either clear the associated error, or disallow in psVectorStats
    if (!psVectorStats(stats, myVector, NULL, NULL, 0)) {
        psFree(stats);
        return NAN;
    }
    min = stats->min;
    max = stats->max;
    if (isnan(min) || isnan(max)) {
        psFree(stats);
        return NAN;
    }

    psTrace("psModules.astrom", 5, "Data min/max is (%.2f, %.2f)\n", min, max);

    // If all data points have the same value, then we set the appropriate members of stats and return.
    if (fabs(max - min) <= FLT_EPSILON) {
        psFree (stats);
        return 0.0;
    }

    // Define the histogram bin size.
    float binSize = 0.001;
    long numBins = PS_MAX(PS_MIN(100000, (max - min) / binSize), 2); // Number of bins
    psTrace("psModules.astrom", 5, "Numbins is %ld\n", numBins);
    psTrace("psModules.astrom", 5, "Creating a robust histogram from data range (%.2f - %.2f)\n", min, max);

    // allocate the histogram containers
    histogram = psHistogramAlloc(min, max, numBins);
    cumulative = psHistogramAlloc(min, max, numBins);

    if (!psVectorHistogram(histogram, myVector, NULL, NULL, 0)) {
        // if psVectorHistogram returns false, we have a programming error
        psAbort ("Unable to generate histogram");
    }
    if (psTraceGetLevel("psModules.astrom") >= 8) {
        PS_VECTOR_PRINT_F32(histogram->bounds);
        PS_VECTOR_PRINT_F32(histogram->nums);
    }

    // Convert the specific histogram to a cumulative histogram
    // The cumulative histogram data points correspond to the UPPER bound value (N < Bin[i+1])
    cumulative->nums->data.F32[0] = histogram->nums->data.F32[0];
    for (long i = 1; i < histogram->nums->n; i++) {
        cumulative->nums->data.F32[i] = cumulative->nums->data.F32[i-1] + histogram->nums->data.F32[i];
        cumulative->bounds->data.F32[i-1] = histogram->bounds->data.F32[i];
    }
    if (psTraceGetLevel("psModules.astrom") >= 8) {
        PS_VECTOR_PRINT_F32(cumulative->bounds);
        PS_VECTOR_PRINT_F32(cumulative->nums);
    }

    // Find the bin which contains the first data point above the limit
    long totalDataPoints = cumulative->nums->data.F32[numBins - 1];
    psTrace("psModules.astrom", 6, "Total data points is %ld\n", totalDataPoints);

    // find bin which is the lower bound of the limit value (value[bin] < f < value[bin+1]
    long binMin;
    PS_BIN_FOR_VALUE(binMin, cumulative->nums, minFrac * totalDataPoints, 0);
    psTrace("psModules.astrom", 6, "The bin is %ld (%.4f to %.4f)\n", binMin, cumulative->bounds->data.F32[binMin], cumulative->bounds->data.F32[binMin+1]);

    // Linear interpolation to the limit value in bin units
    float valueMin;
    PS_BIN_INTERPOLATE (valueMin, cumulative->nums, cumulative->bounds, binMin, totalDataPoints * minFrac);
    psTrace("psModules.astrom", 6, "limit value is %f\n", valueMin);

    // find bin which is the lower bound of the limit value (value[bin] < f < value[bin+1]
    long binMax;
    PS_BIN_FOR_VALUE(binMax, cumulative->nums, maxFrac * totalDataPoints, 0);
    psTrace("psModules.astrom", 6, "The bin is %ld (%.4f to %.4f)\n", binMax, cumulative->bounds->data.F32[binMax], cumulative->bounds->data.F32[binMax+1]);

    // Linear interpolation to the limit value in bin units
    float valueMax;
    PS_BIN_INTERPOLATE (valueMax, cumulative->nums, cumulative->bounds, binMax, totalDataPoints * maxFrac);
    psTrace("psModules.astrom", 6, "limit value is %f\n", valueMax);

    // Clean up
    psFree(histogram);
    psFree(cumulative);
    psFree(stats);

    return (valueMax - valueMin);
}

/******************************************************************************
pmAstromRotateObj(old, center, angle, angle): rotate & scale the focal-plane coordinates
about the center coordinate angle specified in radians
 ******************************************************************************/
psArray *pmAstromRotateObj(
    const psArray *old,
    psPlane center,
    double angle,
    double scale)
{
    PS_ASSERT_PTR_NON_NULL(old, NULL);

    double X, Y;
    pmAstromObj *newObj;
    const pmAstromObj *oldObj;

    psArray *new = psArrayAlloc (old->n);
    double cs = scale*cos(angle);
    double sn = scale*sin(angle);
    double xCenter = center.x;
    double yCenter = center.y;

    for (int i = 0; i < old->n; i++) {

        oldObj = (pmAstromObj *)old->data[i];
        newObj = pmAstromObjCopy (oldObj);

        X = oldObj->FP->x - xCenter;
        Y = oldObj->FP->y - yCenter;

        newObj->FP->x = X*cs + Y*sn + xCenter;
        newObj->FP->y = Y*cs - X*sn + yCenter;

        new->data[i] = newObj;
    }
    return (new);
}

/******************************************************************************
pmAstromStatsFree(stats)
 ******************************************************************************/
static void pmAstromStatsFree(pmAstromStats *stats)
{
    if (stats == NULL)
        return;
    return;
}

/******************************************************************************
pmAstromStatsAlloc()
 ******************************************************************************/
pmAstromStats *pmAstromStatsAlloc(void)
{
    pmAstromStats *stats = psAlloc (sizeof(pmAstromStats));
    psMemSetDeallocator (stats, (psFreeFunc)pmAstromStatsFree);

    stats->center.x    = 0;
    stats->center.y    = 0;
    stats->center.xErr = 0;
    stats->center.yErr = 0;

    stats->offset.x    = 0;
    stats->offset.y    = 0;
    stats->offset.xErr = 0;
    stats->offset.yErr = 0;

    stats->angle       = 0.0;
    stats->scale       = 1.0;
    stats->minMetric   = 0.0;
    stats->minVar      = 0.0;
    stats->nMatch      = 0;
    stats->nTest       = 0;
    stats->nSigma      = 0;

    return (stats);
}

/******************************************************************************
pmAstromFitResultsFree(stats)
 ******************************************************************************/
static void pmAstromFitResultsFree(pmAstromFitResults *results)
{
    if (results == NULL)
        return;
    psFree (results->xStats);
    psFree (results->yStats);
    return;
}

/******************************************************************************
pmAstromFitResultsAlloc()
 ******************************************************************************/
pmAstromFitResults *pmAstromFitResultsAlloc(void)
{
    pmAstromFitResults *results = psAlloc (sizeof(pmAstromFitResults));
    psMemSetDeallocator (results, (psFreeFunc)pmAstromFitResultsFree);

    results->xStats    = NULL;
    results->yStats    = NULL;
    results->nMatch    = 0;
    results->nSigma    = 0;

    return (results);
}

/******************************************************************************
astromObjFree(obj)
 ******************************************************************************/
static void astromObjFree(pmAstromObj *obj)
{
    if (obj == NULL) {
        return;
    }

    psFree(obj->pix);
    psFree(obj->cell);
    psFree(obj->chip);
    psFree(obj->FP);
    psFree(obj->TP);
    psFree(obj->sky);

    return;
}


/******************************************************************************
pmAstromObjAlloc()
 ******************************************************************************/
pmAstromObj *pmAstromObjAlloc(void)
{
    pmAstromObj *obj = psAlloc (sizeof(pmAstromObj));
    psMemSetDeallocator (obj, (psFreeFunc)astromObjFree);

    obj->pix  = psPlaneAlloc();
    obj->cell = psPlaneAlloc();
    obj->chip = psPlaneAlloc();
    obj->FP   = psPlaneAlloc();
    obj->TP   = psPlaneAlloc();
    obj->sky  = psSphereAlloc();
    obj->Mag  = 0;
    obj->Color= 0;
    obj->dMag = 0;
    obj->magCal = 0;

    return (obj);
}

bool pmAstromObjTest(const psPtr ptr)
{
    return (psMemGetDeallocator(ptr) == (psFreeFunc)astromObjFree);
}



/******************************************************************************
pmAstromObjCopy(old)
 ******************************************************************************/
pmAstromObj *pmAstromObjCopy(const pmAstromObj *old)
{
    PS_ASSERT_PTR_NON_NULL(old, NULL);
    pmAstromObj *obj = pmAstromObjAlloc();

    *obj->pix  = *old->pix;
    *obj->cell = *old->cell;
    *obj->chip = *old->chip;
    *obj->FP   = *old->FP;
    *obj->TP   = *old->TP;
    *obj->sky  = *old->sky;
    obj->Mag   =  old->Mag;
    obj->Color =  old->Color;
    obj->dMag  =  old->dMag;
    obj->magCal =  old->magCal;

    return(obj);
}


/******************************************************************************
 ******************************************************************************/
static void pmAstromMatchFree (pmAstromMatch *match)
{
    if (match == NULL)
        return;
    return;
}


/******************************************************************************
 ******************************************************************************/
pmAstromMatch *pmAstromMatchAlloc(
    int raw,
    int ref)
{
    pmAstromMatch *match = psAlloc (sizeof(pmAstromMatch));
    psMemSetDeallocator(match, (psFreeFunc) pmAstromMatchFree);

    match->raw = raw;
    match->ref = ref;

    return (match);
}


static double maxOffpix;                // maximum allowed offset between lists, in raw pixels
static double Scale;                    // grid pixel scale static
double Offset;                          // deltas to pixels
/******************************************************************************
AstromGridBin(*dx, *dy, dX, dY): local function to convert x,y coords to grid
bins it requires the globals defined above.

 ******************************************************************************/
static bool AstromGridBin(
    int *dx,
    int *dy,
    double dX,
    double dY)
{
    if (PM_ASTROMETRYOBJECTS_DEBUG) {
        PS_ASSERT_PTR_NON_NULL(dx, false);
        PS_ASSERT_PTR_NON_NULL(dy, false);
    }

    if (!isfinite(dX)) return false;
    if (!isfinite(dY)) return false;

    if (fabs(dX) > maxOffpix) return false;
    if (fabs(dY) > maxOffpix) return false;

    *dx = dX / Scale + Offset;
    *dy = dY / Scale + Offset;
    return true;
}


/******************************************************************************
pmAstromGridAngle(raw, ref, config): match the two lists using the binned
delta-delta max.
 ******************************************************************************/
pmAstromStats *pmAstromGridAngle(
    const psArray *raw,
    const psArray *ref,
    const psMetadata *config)
{

    PS_ASSERT_PTR_NON_NULL(raw, NULL);
    PS_ASSERT_PTR_NON_NULL(ref, NULL);
    PS_ASSERT_PTR_NON_NULL(config, NULL);

    bool status;
    int nPix;       // size of matching grid
    int nPixHalf;   // half-size of matching grid
    double dX, dY;  // offset between a possible matched pair
    int iX, iY;     // corresponding grid bin

    const pmAstromObj *ob1, *ob2; // short-cut pointers to the objects

    pmAstromStats *stats = pmAstromStatsAlloc();    // output match statistics

    // max allowed offset in either X or Y directions
    double gridOffset = psMetadataLookupF32 (&status, config, "PSASTRO.GRID.OFFSET");

     // sampling scale of the grid
     double gridScale  = psMetadataLookupF32 (&status, config, "PSASTRO.GRID.SCALE");

    // sigma of gaussian window to down-weight photometric outliers (ignored if NAN or 0.0)
    double photomWindowSigma  = psMetadataLookupF32 (&status, config, "PSASTRO.PHOTOM.WINDOW.SIGMA");
    bool   photomWindowApply  = !(isnan(photomWindowSigma) || (fabs(photomWindowSigma) < 0.01));
    double photomWindowFactor = photomWindowApply ? -0.5/PS_SQR(photomWindowSigma) : 0.0;

    // set the static scaling factors
    nPixHalf = (int)(gridOffset / gridScale + 0.5);  // half-grid
    nPix = 2*nPixHalf + 1;                           // full grid width

    // these are globals used by p_pmAstromGridBin
    maxOffpix = gridScale * (nPixHalf + 0.5);            // max offset from true center
    Offset    = maxOffpix / gridScale;
    Scale     = gridScale;

    // images used as accumulators for the loop below
    psImage *gridNP = psImageAlloc (nPix, nPix, PS_TYPE_U32);
    psImage *gridDX = psImageAlloc (nPix, nPix, PS_TYPE_F32);
    psImage *gridDY = psImageAlloc (nPix, nPix, PS_TYPE_F32);
    psImage *gridD2 = psImageAlloc (nPix, nPix, PS_TYPE_F32);
    psImageInit (gridNP, 0);
    psImageInit (gridDX, 0);
    psImageInit (gridDY, 0);
    psImageInit (gridD2, 0);

    // short-cut names for grid images
    psU32 **NP = gridNP->data.U32;
    psF32 **DX = gridDX->data.F32;
    psF32 **DY = gridDY->data.F32;
    psF32 **D2 = gridD2->data.F32;


    // accumulate grids for focal plane (L,M) matches
    for (int i = 0; i < raw->n; i++) {
        ob1 = (pmAstromObj *)raw->data[i];
        for (int j = 0; j < ref->n; j++) {
            ob2 = (pmAstromObj *)ref->data[j];
            dX = ob1->FP->x - ob2->FP->x;
            dY = ob1->FP->y - ob2->FP->y;

            // find bin coordinates for this delta-delta
            if (!AstromGridBin (&iX, &iY, dX, dY)) {
                continue; // matched pair is too far offset
            }

	    // XXX should I make the scale factor in front a recipe value?
	    int Npts = 10 * exp(photomWindowFactor*PS_SQR(ob1->magCal - ob2->magCal)); 

            // accumulate bin stats
            NP[iY][iX] += Npts;
            DX[iY][iX] += dX*Npts;
            DY[iY][iX] += dY*Npts;
            D2[iY][iX] += PS_SQR(dX*Npts) + PS_SQR(dY*Npts);
        }
    }

    // now assess the grid images
    {
        double minMetric = 1e10;
        double minVar = 1e10;
        int minX = -1;
        int minY = -1;
        double metric, var;

        // find the max pixel
        psStats *imStats = psStatsAlloc (PS_STAT_MAX | PS_STAT_MAX | PS_STAT_SAMPLE_MEDIAN | PS_STAT_SAMPLE_STDEV);
        if (!psImageStats(imStats, gridNP, NULL, 0)) {
            psError(PS_ERR_UNKNOWN, false, "Unable to get image statistics.\n");
            psFree(imStats);
            psFree(gridNP);
            psFree(gridDX);
            psFree(gridDY);
            psFree(gridD2);
            psFree(stats);
            return NULL;
        }

        if (psTraceGetLevel("psModules.astrom") >= 5) {
            char line[16];
            psFits *fits = psFitsOpen ("grid.image.fits", "w");
            psFitsWriteImage (fits, NULL, gridNP, 0, NULL);
            psFitsClose (fits);
            fprintf (stderr, "wrote grid image, press return to continue\n");
            if (!fgets (line, 15, stdin)) {
                fprintf(stderr, "Error waiting for RETURN.");
            }
        }

        // only check bins with at least 1/2 of max bin
        // XXX requiring at least 3 matches in bin
        int minNpts = PS_MAX (0.5*imStats->max, 5);
        psTrace("psModule.astrom", 4, "minNpts: %d, min: %d, max: %d, median: %f, stdev: %f", minNpts, (int)(imStats->min), (int)(imStats->max), imStats->sampleMedian, imStats->sampleStdev);

        // find the 'best' bin
        for (int j = 0; j < gridNP->numRows; j++) {
            for (int i = 0; i < gridNP->numCols; i++) {
                if (NP[j][i] < minNpts) continue;

                // this metric emphasizes a narrow peak with lots of sources over one with few.
                var = fabs((D2[j][i]/NP[j][i]) - PS_SQR(DX[j][i]/NP[j][i]) - PS_SQR(DY[j][i]/NP[j][i]));
                metric = var / PS_SQR(NP[j][i]) / PS_SQR(NP[j][i]);

                // fprintf (stderr, "try : %f %f (%d pts, %f var, %f met)\n", DX[j][i]/NP[j][i], DY[j][i]/NP[j][i], NP[j][i], var, metric);

                if (metric < minMetric) {
                    minMetric = metric;
                    minVar    = var;
                    minX      = i;
                    minY      = j;
                }
            }
        }

        // convert the bin to delta-delta
        if ((minX < 0) || (minY < 0))
        {
            // no valid matches found
            stats->offset.x   = 0;
            stats->offset.y   = 0;
            stats->minMetric  = minMetric;
            stats->minVar     = minVar;
            stats->nMatch     = 0;
        } else
        {
            stats->offset.x  = DX[minY][minX] / NP[minY][minX];
            stats->offset.y  = DY[minY][minX] / NP[minY][minX];
            stats->minMetric = minMetric;
            stats->minVar    = minVar;
            stats->nMatch    = NP[minY][minX];
        }

        // XXX this function is crashing
        pmAstromVisualPlotGridMatch(raw, ref, gridNP, stats->offset.x, stats->offset.y, maxOffpix, Scale, Offset);
        pmAstromVisualPlotGridMatchOverlay(raw, ref, stats->offset);

        psFree (imStats);
        // XXX EAM : This routine, and pmAstromGridMatch, need to handle failure cases better
    }

    // sort the NP values and choose
    psVector *listNP = psVectorAlloc (nPix*nPix, PS_TYPE_U32);
    int n = 0;
    for (int i = 0; i < nPix; i++) {
        for (int j = 0; j < nPix; j++) {
            listNP->data.U32[n] = gridNP->data.U32[j][i];
            n++;
        }
    }
    psVector *sort = psVectorSort (NULL, listNP);
    stats->nTest = sort->data.U32[sort->n - 5];
    stats->nSigma = (stats->nMatch - stats->nTest) / sqrt(stats->nTest);
    // XXX this needs a better analysis of the image histogram..
    // fprintf (stderr, "sigma: nMatch: %d, nTest: %d, nTen: %d\n", stats->nMatch, stats->nTest, sort->data.U32[sort->n - 10]);

    psFree (sort);
    psFree (listNP);
    psFree (gridNP);
    psFree (gridDX);
    psFree (gridDY);
    psFree (gridD2);
    return (stats);
}



/******************************************************************************
pmAstromGridMatch(*raw, *ref, *config): match two star lists.
 ******************************************************************************/

pmAstromStats *pmAstromGridMatch(const psArray *raw,
                                 const psArray *ref,
                                 const psMetadata *config)
{
    PS_ASSERT_PTR_NON_NULL(raw, NULL);
    PS_ASSERT_PTR_NON_NULL(ref, NULL);
    PS_ASSERT_PTR_NON_NULL(config, NULL);
    bool status;
    double xMin, xMax, yMin, yMax;
    const pmAstromObj *obj;
    psArray *rot;

    pmAstromStats *minStat = pmAstromStatsAlloc ();
    pmAstromStats *newStat = NULL;

    psPlane center;

    // find center of the raw field (focal-plane coords)
    xMin = yMin = +1e10;
    xMax = yMax = -1e10;
    for (int i = 0; i < raw->n; i++) {
        obj = (pmAstromObj *)raw->data[i];
        xMin = PS_MIN (obj->FP->x, xMin);
        xMax = PS_MAX (obj->FP->x, xMax);
        yMin = PS_MIN (obj->FP->y, yMin);
        yMax = PS_MAX (obj->FP->y, yMax);
    }
    center.x = 0.5*(xMin + xMax);
    center.y = 0.5*(yMin + yMax);

    double minScale = psMetadataLookupF32 (&status, config, "PSASTRO.GRID.MIN.SCALE");
    double maxScale = psMetadataLookupF32 (&status, config, "PSASTRO.GRID.MAX.SCALE");
    double delScale = psMetadataLookupF32 (&status, config, "PSASTRO.GRID.DEL.SCALE");

    double minAngle = PS_RAD_DEG*psMetadataLookupF32 (&status, config, "PSASTRO.GRID.MIN.ANGLE");
    double maxAngle = PS_RAD_DEG*psMetadataLookupF32 (&status, config, "PSASTRO.GRID.MAX.ANGLE");
    double delAngle = PS_RAD_DEG*psMetadataLookupF32 (&status, config, "PSASTRO.GRID.DEL.ANGLE");
    double minSigma = psMetadataLookupF32 (&status, config, "PSASTRO.GRID.MIN.SIGMA");

    minStat->minMetric = 1e10;
    for (double scale = minScale; scale <= maxScale; scale += delScale) {
        for (double angle = minAngle; angle <= maxAngle; angle += delAngle) {
            rot = pmAstromRotateObj (raw, center, angle, scale);

# if 0
            FILE *f1 = fopen ("raw.dat", "w");
            for (int i = 0; i < rot->n; i++) {
                pmAstromObj *obj = rot->data[i];
                fprintf (f1, "%8.2f %8.2f   %6.2f\n", obj->FP->x, obj->FP->y, obj->Mag);
            }
            fclose (f1);
            FILE *f2 = fopen ("ref.dat", "w");
            for (int i = 0; i < ref->n; i++) {
                pmAstromObj *obj = ref->data[i];
                fprintf (f2, "%8.2f %8.2f   %6.2f\n", obj->FP->x, obj->FP->y, obj->Mag);
            }
            fclose (f2);
            fprintf (stderr, "type return");
            char c;
            fscanf (stdin, "%c", &c);
# endif
            newStat = pmAstromGridAngle (rot, ref, config);
            newStat->angle  = angle;
            newStat->scale  = scale;
            newStat->center = center;

            if (isfinite(newStat->minMetric) && (newStat->minMetric > 0.0) && (newStat->nSigma >= minSigma) && (newStat->minMetric < minStat->minMetric)) {
                *minStat = *newStat;
                psLogMsg ("psModule.astrom", 4, "grid test - offset: %7.2f,%7.2f @ %6.1f deg x %7.3f (%4d pts, %5.1f sig, %5.1f var, %6.3f log metric) *",
                          minStat->offset.x, minStat->offset.y, PS_DEG_RAD*minStat->angle, minStat->scale, minStat->nMatch, minStat->nSigma, minStat->minVar, log10(minStat->minMetric));
            } else {
                psLogMsg ("psModule.astrom", 4, "grid test - offset: %7.2f,%7.2f @ %6.1f deg x %7.3f (%4d pts, %5.1f sig, %5.1f var, %6.3f log metric)",
                          newStat->offset.x, newStat->offset.y, PS_DEG_RAD*newStat->angle, newStat->scale, newStat->nMatch, newStat->nSigma, newStat->minVar, log10(newStat->minMetric));

            }
            psFree (newStat);
            psFree (rot);
        }
    }
    psLogMsg ("psModule.astrom.grid.match", 4, "grid best - offset: %7.2f,%7.2f @ %6.1f deg x %7.3f (%4d pts, %5.1f sig, %5.1f var, %6.3f log metric)",
              minStat->offset.x, minStat->offset.y, PS_DEG_RAD*minStat->angle, minStat->scale, minStat->nMatch, minStat->nSigma, minStat->minVar, log10(minStat->minMetric));

    // I need to decide if a solution is likely to be a good solution or just a mis-match
    // one posibility: how significant is the peak relative to the 4th or 5th most significant pixel?

    if (minStat->nSigma < minSigma) {
	psLogMsg ("psModule.astrom.grid.match", 3, "Failed to find a valid match (%f sigma for best)", minStat->nSigma);
        psFree (minStat);
        return NULL;
    }
    return (minStat);
}

/******************************************************************************
pmAstromGridTweak(*raw, *ref, *recipe, stats): improve match for two star lists.
 ******************************************************************************/
pmAstromStats *pmAstromGridTweak(
    psArray *raw,
    psArray *ref,
    psMetadata *recipe,
    pmAstromStats *stats)
{
    bool status;
    pmAstromObj *ob1, *ob2;  // short-cut pointers to the objects
    double dX, dY;   // offset between a possible matched pair
    psArray *rot;
    int xBin = 0, yBin = 0;

    rot = pmAstromRotateObj (raw, stats->center, stats->angle, stats->scale);

    // sampling scale of the grid
    double tweakScale  = psMetadataLookupF32 (&status, recipe, "PSASTRO.TWEAK.SCALE");
    double tweakRange  = psMetadataLookupF32 (&status, recipe, "PSASTRO.TWEAK.RANGE");
    double tweakSmooth = psMetadataLookupF32 (&status, recipe, "PSASTRO.TWEAK.SMOOTH");
    double tweakNsigma = psMetadataLookupF32 (&status, recipe, "PSASTRO.TWEAK.NSIGMA");

    // sigma of gaussian window to down-weight photometric outliers (ignored if NAN or 0.0)
    double photomWindowSigma  = psMetadataLookupF32 (&status, recipe, "PSASTRO.PHOTOM.WINDOW.SIGMA");
    bool   photomWindowApply  = !(isnan(photomWindowSigma) || (fabs(photomWindowSigma) < 0.01));
    double photomWindowFactor = photomWindowApply ? -0.5/PS_SQR(photomWindowSigma) : 0.0;

    int nBin = 2*tweakRange / tweakScale;
    psVector *xHist = psVectorAlloc (nBin, PS_TYPE_F32);
    psVector *yHist = psVectorAlloc (nBin, PS_TYPE_F32);
    psVectorInit (xHist, 0);
    psVectorInit (yHist, 0);

    // accumulate grids for focal plane (L,M) matches
    for (int i = 0; i < rot->n; i++) {
        ob1 = (pmAstromObj *)rot->data[i];
        for (int j = 0; j < ref->n; j++) {
            ob2 = (pmAstromObj *)ref->data[j];
            dX = ob1->FP->x - ob2->FP->x - stats->offset.x;
            dY = ob1->FP->y - ob2->FP->y - stats->offset.y;

            xBin = (dX + tweakRange) / tweakScale;
            yBin = (dY + tweakRange) / tweakScale;

            if (xBin < 0)
                continue;
            if (yBin < 0)
                continue;
            if (xBin >= nBin)
                continue;
            if (yBin >= nBin)
                continue;

	    // XXX should I make the scale factor in front a recipe value?
	    int Npts = 10 * exp(photomWindowFactor*PS_SQR(ob1->magCal - ob2->magCal));  // sigma = 0.22 mag

            xHist->data.F32[xBin] += Npts;
            yHist->data.F32[yBin] += Npts;
        }
    }

    pmAstromVisualPlotTweak (xHist, yHist, xBin, yBin);

    // smooth histgram vector with gaussian of 1sigma = radius
    psVector *xHistNew = psVectorSmooth(NULL, xHist, tweakSmooth, tweakNsigma);
    psVector *yHistNew = psVectorSmooth(NULL, yHist, tweakSmooth, tweakNsigma);

    // if we failed to smooth, just use the original vector (probably too narrow a range)
    if (!xHistNew) xHistNew = psMemIncrRefCounter (xHist);
    if (!yHistNew) yHistNew = psMemIncrRefCounter (yHist);

    // select peak in x and in y
    xBin = yBin = 0;
    double xMax = 0;
    double yMax = 0;
    for (int i = 0; i < nBin; i++) {
        if (xHistNew->data.F32[i] > xMax) {
            xBin = i;
            xMax = xHistNew->data.F32[i];
        }
        if (yHistNew->data.F32[i] > yMax) {
            yBin = i;
            yMax = yHistNew->data.F32[i];
        }
    }
    double xPeak = xBin*tweakScale - tweakRange;
    double yPeak = yBin*tweakScale - tweakRange;
    psLogMsg (__func__, 3, "tweak peak by %f,%f\n", xPeak, yPeak);

    // adjust offset by peak center
    pmAstromStats *tweak = pmAstromStatsAlloc();
    *tweak = *stats;
    tweak->offset.x += xPeak;
    tweak->offset.y += yPeak;

    pmAstromVisualPlotTweak (xHistNew, yHistNew, xBin, yBin);

    psFree (rot);
    psFree (xHist);
    psFree (yHist);
    psFree (xHistNew);
    psFree (yHistNew);

    return tweak;
}

/******************************************************************************
pmAstromGridApply(*map, stat): apply the measured FPA offset and rotation
(stat) to the fpa astrom structures.
 ******************************************************************************/
psPlaneTransform *pmAstromGridApply(
    psPlaneTransform *map,
    pmAstromStats *stat)
{
    PS_ASSERT_PTR_NON_NULL(map, NULL);
    PS_ASSERT_POLY_NON_NULL(map->x, NULL);
    PS_ASSERT_POLY_NON_NULL(map->y, NULL);

    double cs = stat->scale * cos (stat->angle);
    double sn = stat->scale * sin (stat->angle);

    double dx = (map->x->coeff[0][0] - stat->center.x);
    double dy = (map->y->coeff[0][0] - stat->center.y);

    // new offset
    map->x->coeff[0][0] =  cs*dx + sn*dy - stat->offset.x + stat->center.x;
    map->y->coeff[0][0] = -sn*dx + cs*dy - stat->offset.y + stat->center.y;

    // original rotation matrix
    double pc1_1 = map->x->coeff[1][0];
    double pc1_2 = map->x->coeff[0][1];
    double pc2_1 = map->y->coeff[1][0];
    double pc2_2 = map->y->coeff[0][1];

    // new rotation matrix
    map->x->coeff[1][0] = +cs*pc1_1 + sn*pc2_1;
    map->x->coeff[0][1] = +cs*pc1_2 + sn*pc2_2;
    map->y->coeff[1][0] = -sn*pc1_1 + cs*pc2_1;
    map->y->coeff[0][1] = -sn*pc1_2 + cs*pc2_2;

    return (map);
}

/* Illustration of the grid bins
   dX        bin
   -35:-25 -> 0     bin = dX / Scale + Offset
   -25:-15 -> 1     Scale = 10
   -15:-05 -> 2     Offset = 3.5
   -05:+05 -> 3     nPix = 3 (maxOffset = 35 = (nPix + 0.5)*dXix
   +05:+15 -> 4     dPix = 10
   +15:+25 -> 5
   +25:+35 -> 6

   maxOffsetRequest = 30
   nPix = (int) (maxOffset / dPix + 0.5);
   maxOffset = (nPix + 0.5)*Scale;
*/

/*****************************************************************************/
static void pmAstromMatchInfoFree (pmAstromMatchInfo *info)
{
    if (info == NULL) return;
    return;
}


/*****************************************************************************/
pmAstromMatchInfo *pmAstromMatchInfoAlloc()
{
    pmAstromMatchInfo *info = psAlloc (sizeof(pmAstromMatchInfo));
    psMemSetDeallocator(info, (psFreeFunc) pmAstromMatchInfoFree);

    info->match = NULL;
    info->radius = NAN;

    return (info);
}

// generate a unique set of matches (choose closest match)
psArray *pmAstromRadiusMatchUniq (psArray *rawstars, psArray *refstars, psArray *matches) {

    // I have the matches between the refstars and the rawstars.
    // For each refstar, find the single match which has the smallest radius and reject
    // all others.

    // create an array of refstars->n arrays, each containing all of the matches for the
    // given refstar.

    psArray *refstarMatches = psArrayAlloc (refstars->n);

    for (int i = 0; i < matches->n; i++) {

        pmAstromMatch *match = matches->data[i];

        // accumulate this refstar match on the array for this refstar (create if needed)
        psArray *refSet = refstarMatches->data[match->ref];
        if (!refSet) {
            refstarMatches->data[match->ref] = psArrayAllocEmpty (8);
            refSet = refstarMatches->data[match->ref];
        }

        pmAstromMatchInfo *matchInfo = pmAstromMatchInfoAlloc();

        pmAstromObj *refStar = refstars->data[match->ref];
        pmAstromObj *rawStar = rawstars->data[match->raw];

        matchInfo->match = match; // reference to the match of interest
        matchInfo->radius = hypot (refStar->FP->x - rawStar->FP->x, refStar->FP->y - rawStar->FP->y);

        psArrayAdd (refSet, 8, matchInfo); // matchInfo->match is just a reference
        psFree (matchInfo);
    }

    // we now have a set of matches for each refstar and their distances; create a new set
    // keeping only the closest entry for each match

    psArray *unique = psArrayAllocEmpty (PS_MAX(16, matches->n / 2));
    for (int i = 0; i < refstars->n; i++) {

        psArray *refSet = refstarMatches->data[i];
        if (!refSet) continue;
        if (refSet->n == 0) continue; // not certain how this can happen...

        if (refSet->n == 1) {
            pmAstromMatchInfo *matchInfo = refSet->data[0];
            psArrayAdd (unique, 32, matchInfo->match);
            continue;
        }

        pmAstromMatchInfo *matchInfo = refSet->data[0];
        float minRadius = matchInfo->radius;
        pmAstromMatch *minMatch = matchInfo->match;
        for (int j = 1; j < refSet->n; j++) {
            pmAstromMatchInfo *matchInfo = refSet->data[j];
            if (minRadius < matchInfo->radius) continue;
            minMatch = matchInfo->match;
            minRadius = matchInfo->radius;
        }

        psArrayAdd (unique, 32, minMatch); // minMatch is just a reference to a match on matches,
    }

    psLogMsg ("psModules.astrom", 3, "generate unique matches to reference stars: %ld matches -> %ld matches\n", matches->n, unique->n);
    psFree (refstarMatches);

    return unique;
}
