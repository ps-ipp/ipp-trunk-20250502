#include <stdio.h>
#include <string.h>
#include <pslib.h>

#include "pmFPA.h"

#include "pmPattern.h"

#define PATTERN_ROW_BKG_FIX 1

/* some in-line notes:

   patternMaskRow sets the data value to NAN, inconsistent with new plan
   
   here is the outline of pmPatternRow

   * at this point, we have already done overscan subtraction, right?

   * measure stats on the full cell (MEDIAN, STDEV)
   ** subsample?
   ** if it fails, it masks the entire cell and set the value to NAN

   * calculate an upper and lower threshold (median +/- T * sigma)
   * define a normalized x-coordinate ('index') : 
   ** see note below on chebys

   * each row is treated independently
   * pixels are masked for the fit if they are out-of-range 
     or if they are already masked

   ** note that the clipping threshold will be larger if there 
      are pixels which have astronomical structures
      
      a possible better option would be to set the threshold based on the median
      and a sigma calculated from Poisson stats (do we know the gain?)
      
   ** fit is allowed to proceed if even N+1 pixels exist, which is clearly too low
   
   ** Remaining pixels are fitted with clip-fit 

   ** solution is subtracted from the data
   (this is implemented with psPolynomial1DEvalVector)
   perhaps faster if we fixed the order to 2 and hardwired the result
   
   * after each row is fitted, the intercept (A value) is fitted
   as a function of the y-coordinate and the result is subtracted

   * the slope value is also fitted as a function of the column 
     and added back in -- I'm not sure I understand this step.

   *****************

   ** what we calculate are related to chebychevs (domain is -1 : +1)
   *** T0(x) = 1
   *** T1(x) = x
   *** T2(x) = 2x^2 - 1

   *** we calculate y = A + Bx + Cx^2

   a_0 + a_1 x + a_2 (2x^2 - 1) = A + B + Cx^2

   a_1       = B
   a_0 - a_2 = A
   2 a_2     = C

   a_0       = A + C/2
   a_1       = B
   a_2       = C/2

   *****************
   
   I have 3 goals in re-working the code:
   
   1) improve overall speed
   2) improve reliability of the fit
   3) skip fit if we can

   Let's assume the signal in the cell is light + bias drift

   The bias drift has an amplitude of ~5 - 10 DN

   That makes a detectable source with ~N * a few counts (multiple pixels in a row)
   
   So, the effective flux is ~10 * 5 = 50 DN
   for which sky level is this value - 3 sigma?
   
   50 / sqrt(sky sigma^2 * effective area)

   area ~ 5pixels, sky sigma^2 = sky

   10 * Npix / sqrt(sky * Npix) = 3

   Npix = sky * (S/N)^2 / (peak^2)
   
   sky = Npix * peak^2 / SN^2

   if (sky < Npix * peak^2 / SN^2), we should skip:
   
   Npix ~ 5
   peak ~ 10
   SN ~ 3 (or even less)

   sky < 5 * 100 / 9 = 55 or so


   To address these in order:
   
   1) speed: 
   * the analysis threaded is not threaded: thread across cells
   
   2) 

 */

// Mask a row as bad
static void patternMaskRow(pmReadout *ro, // Readout to mask
                           int y,       // Row to mask
                           psImageMaskType bad // Mask value to give
                           )
{
    psImage *image = ro->image;         // Image to mask
    psAssert(image, "Require image");
    psAssert(y < image->numRows, "Row not in image");

    int numCols = image->numCols;       // Number of columns
    for (int x = 0; x < numCols; x++) {
        image->data.F32[y][x] = NAN;
    }
    if (ro->mask) {
        psImage *mask = ro->mask;       // Mask image to mask
        for (int x = 0; x < numCols; x++) {
            mask->data.PS_TYPE_IMAGE_MASK_DATA[y][x] |= bad;
        }
    }
    return;
}

// Comparison and swap functions for sorting values directly
#define SORT_COMPARE(A,B) (sampleArray[A] < sampleArray[B])
#define SORT_SWAP(TYPE,A,B) {			\
    if (A != B) { \
        TYPE temp = sampleArray[A];			\
        sampleArray[A] = sampleArray[B]; \
        sampleArray[B] = temp; \
    } \
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Measurement and application
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool pmPatternRowUnbinned(pmReadout *ro, int order, int iter, float rej, float thresh,
			  psStatsOptions clipMean, psStatsOptions clipStdev,
			  psImageMaskType maskVal, psImageMaskType maskBad);


bool pmPatternRowBinned(pmReadout *ro, int order, int iter, float rej, float thresh,
			psStatsOptions clipMean, psStatsOptions clipStdev,
			psImageMaskType maskVal, psImageMaskType maskBad);


// XXX allow user choice of binned vs unbinned analysis?
bool pmPatternRow(pmReadout *ro, int order, int iter, float rej, float thresh,
                  psStatsOptions clipMean, psStatsOptions clipStdev,
                  psImageMaskType maskVal, psImageMaskType maskBad) {

  bool status = false;
  if (true) {
    status = pmPatternRowBinned(ro, order, iter, rej, thresh, clipMean, clipStdev, maskVal, maskBad);
  } else {
    status = pmPatternRowUnbinned(ro, order, iter, rej, thresh, clipMean, clipStdev, maskVal, maskBad);
  }
  return status;
}

// USE_BACKGROUND_STDEV: if TRUE, the analysis will use the measured robust stdev to clip the out-of-range pixles
// if FALSE, the stdev will be estimated based on the Poisson statistics of the median (sky level).
# define USE_BACKGROUND_STDEV 0

bool pmPatternRowUnbinned(pmReadout *ro, int order, int iter, float rej, float thresh,
                  psStatsOptions clipMean, psStatsOptions clipStdev,
                  psImageMaskType maskVal, psImageMaskType maskBad)
{
    PM_ASSERT_READOUT_NON_NULL(ro, false);
    PM_ASSERT_READOUT_IMAGE(ro, false);
    PS_ASSERT_INT_NONNEGATIVE(order, false);
    PS_ASSERT_INT_NONNEGATIVE(iter, false);
    PS_ASSERT_FLOAT_LARGER_THAN(rej, 0.0, false);
    PS_ASSERT_FLOAT_LARGER_THAN(thresh, 0.0, false);

    bool mdok;                          // Status of MD lookup

    pmCell *cell = ro->parent;
    pmChip *chip = cell->parent;
    const char *chipName = psMetadataLookupStr(&mdok, chip->concepts, "CHIP.NAME"); // Name of chip
    const char *cellName = psMetadataLookupStr(&mdok, cell->concepts, "CELL.NAME"); // Name of cell

    psImage *image = ro->image;         // Image to correct
    psImage *mask = ro->mask;           // Mask for image
    int numCols = image->numCols, numRows = image->numRows; // Size of image

    psStats *stats = psStatsAlloc(PS_STAT_ROBUST_MEDIAN | PS_STAT_ROBUST_STDEV);
    psRandom *rng = psRandomAlloc(PS_RANDOM_TAUS); // Random number generator
    if (!psImageBackground(stats, NULL, ro->image, ro->mask, maskVal, rng)) {
	psWarning("Unable to calculate statistics on readout; skipping pattern correction for %s, %s.", chipName, cellName);
        psErrorClear();
        psFree(stats);
        psFree(rng);
        return true;
    }

# if (USE_BACKGROUND_STDEV) 
    float lower = stats->robustMedian - thresh * stats->robustStdev; // Lower bound for data
    float upper = stats->robustMedian + thresh * stats->robustStdev; // Upper bound for data
    float background = stats->robustMedian;
# else
    // the signal we are looking for is a small variation on top of the background.  if
    // the background is uniform with only read noise + sky noise, then the pixel-to-pixel
    // stdev should only be due to known noise sources and predictable.  If the
    // pixel-to-pixel variations are from other features, then those variations will
    // probably dominate the row-by-row bias variations.

    // instead of using the image pixel statistics to measure the stdev, lets assume only
    // dark noise plus poisson sky noise.  we are not carrying in the read noise, but it is
    // fairly modest for GPC1 (~10 DN)

    // if we assume a gain of 1 and the read noise of 10 DN, then a sky of 200 would have
    // a noise of N = sqrt (1 * 200 + 10^2) = sqrt (300) ~ 17

    // if the gain were as much as 2, then the noise in DN would be N = sqrt(2 * (200 + 100)) / 2 = sqrt(300) / sqrt(2)
    // so smaller by a factor of 1.4 than what we predict, which is not very large

    // find the nominal signal amplitude (check the ghost and/or crosstalk recipe file)
    float nominalAmplitude = psMetadataLookupF32 (&mdok, cell->analysis, "PTN.ROW.AMP");
    if (!mdok) nominalAmplitude = 20; // XXX EAM : somewhat arbitrary number
    // If we cannot determine the nominal amplitude, we fall-back on the worst case

    // XXX retrieve noise and gain from 'concepts' if possible
# define READNOISE 10 /* arbitrary number */
    float sigma = sqrt(stats->robustMedian + PS_SQR(READNOISE));
    float delta = PS_MIN (thresh * sigma, 2*nominalAmplitude); 
    float lower = stats->robustMedian - delta; // Lower bound for data
    float upper = stats->robustMedian + delta; // Upper bound for data
    float background = stats->robustMedian;

    // if the noise from the background is too large 
    float significance = nominalAmplitude / sigma;
   
    // XXX EAM : arbitrary number
    if (isfinite(nominalAmplitude) && (significance < 1.0/6.0)) {
      psLogMsg("ppImage", PS_LOG_INFO, "Skipping row pattern correction for %s, %s, stats: %f - %f - %f : %f %f %f\n", chipName, cellName, lower, background, upper, sigma, nominalAmplitude, significance);
      return true; 
    }
    fprintf (stderr, "correcting pattern row background %s: %f - %f - %f : %f %f %f\n", cellName, lower, background, upper, sigma, nominalAmplitude, significance);
    psLogMsg("ppImage", PS_LOG_INFO, "Performing row pattern correction for %s, %s, stats: %f - %f - %f : %f %f %f\n", chipName, cellName, lower, background, upper, sigma, nominalAmplitude, significance);
# endif    
    
    psFree(stats);
    psFree(rng);

    // Indices are distributed [-1:1)
    psVector *indices = psVectorAlloc(numCols, PS_TYPE_F32); // Indices for fitting
    float norm = 2.0 / (float)numCols;  // Normalisation for indices
    for (int x = 0; x < numCols; x++) {
        indices->data.F32[x] = x * norm - 1.0;
    }

    psStats *clip = psStatsAlloc(clipMean | clipStdev); // Clipping statistics
    clip->clipIter = iter;
    clip->clipSigma = rej;
    psVector *clipMask = psVectorAlloc(numCols, PS_TYPE_VECTOR_MASK); // Mask for clipping
    psPolynomial1D *poly = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, order); // Polynomial to fit
    psVector *data = psVectorAlloc(numCols, PS_TYPE_F32); // Data to fit

    psImage *corr = psImageAlloc(order + 1, numRows, PS_TYPE_F64); // Corrections applied
    psImageInit(corr, NAN);

#ifdef PATTERN_ROW_BKG_FIX
    // CZW: 2011-11-30
    // Define the vectors to hold the "x" and "y" slope trends.
    // Briefly, the slope trend in the y-axis is a due to variations in the 0-th order term
    // of the PATTERN.ROW fit between individual rows across the cell.  Similarly, the 1-st
    // order term of the PATTERN.ROW fit defines the trend in the x-axis (as that's what we
    // are fitting with PATTERN.ROW in the first place).  However, the thing we're trying to
    // fix with PATTERN.ROW is the detector level bias wiggles.  These should be overlaid on
    // the true sky level.  Therefore, simply applying the PATTERN.ROW correction will
    // introduce cell-to-cell sky variations as these two trends are removed.  To avoid this,
    // We store the 0th and 1st order values used for each row, and then fit a polynomial to
    // these results.  By re-adding these systematic trends back, we can remove the row-to-row
    // variations without improperly removing the real sky trend.
    psVector *yaxisData = psVectorAlloc(numRows, PS_TYPE_F32); // Data to fit to the constant term
    psVector *yaxisMask = psVectorAlloc(numRows, PS_TYPE_VECTOR_MASK); // Mask for rows with no fit
    psVector *xaxisData = psVectorAlloc(numRows, PS_TYPE_F32); // Data to fit to the linear term
    psVectorInit(yaxisMask, 0);
#endif

    // we really need more than order + 1 points (= 4).
    // this should be tunable, but let's try 5 - 10%
    int validNmin = numCols * 0.1;

    for (int y = 0; y < numRows; y++) {
        psVectorInit(clipMask, 0);
        data = psImageRow(data, image, y);
        int num = 0;                    // Number of good pixels

	// if the unmasked pixels only span a small range in x then we cannot fit the
	// 2nd order polynomial variations very well.  Require a minimum fractional range
	float validXmin = +1;
	float validXmax = -1;

	// XXX can we do just as well fitting 1/3 of the pixels? (NOT REALLY)
	// (x % 3) ||
        for (int x = 0; x < numCols; x++) {
	    if ((mask && mask->data.PS_TYPE_IMAGE_MASK_DATA[y][x] & maskVal) ||
		data->data.F32[x] < lower || data->data.F32[x] > upper) {
		clipMask->data.PS_TYPE_VECTOR_MASK_DATA[x] = 0xFF;
            } else {
                clipMask->data.PS_TYPE_VECTOR_MASK_DATA[x] = 0;
                num++;
		validXmin = PS_MIN(indices->data.F32[x], validXmin);
		validXmax = PS_MAX(indices->data.F32[x], validXmax);
            }
        }

	// XXX how much time is spent in the fitting
        if (num < validNmin) {
            // Not enough points to fit
            patternMaskRow(ro, y, maskBad);
#ifdef PATTERN_ROW_BKG_FIX
	    // Ignore this row in our subsequent fits, because the fit failed.
	    yaxisMask->data.PS_TYPE_VECTOR_MASK_DATA[y] = 0xFF;
#endif
            continue;
        }
	// XXX does this need to be a clipped fit if we are clipping based on the median poisson noise?
        if (!psVectorClipFitPolynomial1D(poly, clip, clipMask, 0xFF, data, NULL, indices)) {
            psWarning("Unable to fit polynomial to row %d", y);
            psErrorClear();
            patternMaskRow(ro, y, maskBad);
#ifdef PATTERN_ROW_BKG_FIX
	    // Ignore this row in our subsequent fits, because the fit failed.
	    yaxisMask->data.PS_TYPE_VECTOR_MASK_DATA[y] = 0xFF;
#endif
            continue;
        }
#ifndef PATTERN_ROW_BKG_FIX
 	poly->coeff[0] -= background;
#else
	// Store the results we found for this row.
	yaxisData->data.F32[y] = poly->coeff[0];
	xaxisData->data.F32[y] = poly->coeff[1];
	psTrace("pattern",1,"%d %g %g\n",y,poly->coeff[0],poly->coeff[1]);
	
	//	yaxisData->data.F32[y] = 0.0;
/* 	xaxisData->data.F32[y] = 0.0; */
	
#endif
        memcpy(corr->data.F64[y], poly->coeff, (order + 1) * PSELEMTYPE_SIZEOF(PS_TYPE_F64));
        psVector *solution = psPolynomial1DEvalVector(poly, indices); // Solution vector
        if (!solution) {
            psWarning("Unable to evaluate polynomial for row %d", y);
            psErrorClear();
            patternMaskRow(ro, y, maskBad);
#ifdef PATTERN_ROW_BKG_FIX
	    yaxisMask->data.PS_TYPE_VECTOR_MASK_DATA[y] = 0xFF;
#endif
            continue;
        }

        for (int x = 0; x < numCols; x++) {
            image->data.F32[y][x] -= solution->data.F32[x];
	    psTrace("pattern",5,"A: %d %d %g\n",x,y,solution->data.F32[x]);
        }
        psFree(solution);
    }

#ifdef PATTERN_ROW_BKG_FIX
    // Put the global trends back that were removed by the PATTERN.ROW correction.
    // Set up the indices for the polynomial
    psVector *yaxisIndices = psVectorAlloc(numRows, PS_TYPE_F32);
    norm = 2.0 / (float)numRows;
    for (int y = 0; y < numRows; y++) {
      yaxisIndices->data.F32[y] = y * norm - 1.0;
      psTrace("psModules.detrend.pattern",10,"%d %f %f\n",y,yaxisIndices->data.F32[y],yaxisData->data.F32[y]);
    }

    // Fit the trend of the constant term, producing the y-axis global trend
    psStatsInit(clip);
    psPolynomial1D *yaxisPoly = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, 1); // Polynomial to fit.
    if (!psVectorClipFitPolynomial1D(yaxisPoly,clip,yaxisMask,0xFF,yaxisData, NULL, yaxisIndices)) {
      psWarning("Unable to fit polynomial to y-axis trend");
      psErrorClear();
      // If we've failed, we need to do something, so add back in the background level, and
      // expect that the final image will have background mismatches.
      for (int y = 0; y < numRows; y++) {
	for (int x = 0; x < numCols; x++) {
	  image->data.F32[y][x] += background;
	}
	corr->data.F64[y][0]  -= background;
      }
    }
    else {
      psVector *solution = psPolynomial1DEvalVector(yaxisPoly,yaxisIndices);
      if (!solution) {
	psWarning("Unable to evaluate polynomial");
	psErrorClear();
	// If we've failed, we need to do something, so add back in the background level, and
	// expect that the final image will have background mismatches.
	for (int y = 0; y < numRows; y++) {
	  for (int x = 0; x < numCols; x++) {
	    image->data.F32[y][x] += background;
	  }
	  corr->data.F64[y][0]  -= background;
	}
      }
      else {
	for (int y = 0; y < numRows; y++) {
	  for (int x = 0; x < numCols; x++) {
	    image->data.F32[y][x] += solution->data.F32[y];
	    psTrace("pattern",5,"B: %d %d %g\n",x,y,solution->data.F32[y]);
	  }
	  corr->data.F64[y][0]  -= solution->data.F32[y];
	}
      }
      psFree(solution);
    }      

    // Fit the trend of the linear term, producing the x-axis global trend
    // We can use the same mask vector, as the same rows failed the row-fit earlier.
    psStatsInit(clip);
    psPolynomial1D *xaxisPoly = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, 1); // Polynomial to fit.
    if (!psVectorClipFitPolynomial1D(xaxisPoly,clip,yaxisMask,0xFF,xaxisData, NULL, yaxisIndices)) {
      psWarning("Unable to fit polynomial to x-axis trend");
      psErrorClear();
    }
    else {
      psVector *solution = psPolynomial1DEvalVector(xaxisPoly,yaxisIndices);
      if (!solution) {
	psWarning("Unable to evaluate polynomial");
	psErrorClear();
      }
      else {
	for (int y = 0; y < numRows; y++) {
	  for (int x = 0; x < numCols; x++) {
	    image->data.F32[y][x] += solution->data.F32[y] * indices->data.F32[x];
	    // XXX EAM : this was [x] which is wrong
	    psTrace("pattern",5,"C: %d %d %g %g\n",x,y,solution->data.F32[y],indices->data.F32[x]);
	  }
	  corr->data.F64[y][1]  -= solution->data.F32[y] ;
	}
      }
      psFree(solution);
    }
    psFree(yaxisPoly);
    psFree(xaxisPoly);
    psFree(yaxisIndices);
    psFree(yaxisMask);
    psFree(yaxisData);
    psFree(xaxisData);
    // End PATTERN_ROW_BKG_FIX global trend replacement
#endif 
    
    psMetadataAddImage(ro->analysis, PS_LIST_TAIL, PM_PATTERN_ROW_CORRECTION, PS_META_REPLACE,
                       "Pattern row correction", corr);
    psFree(corr);

    psFree(indices);
    psFree(clip);
    psFree(clipMask);
    psFree(poly);
    psFree(data);

    return true;
}

# define NPIX 15

// bin by NPIX in the x-direction to reduce the number of calculations needed to measure
// the pattern
bool pmPatternRowBinned(pmReadout *ro, int order, int iter, float rej, float thresh,
                  psStatsOptions clipMean, psStatsOptions clipStdev,
                  psImageMaskType maskVal, psImageMaskType maskBad)
{
    PM_ASSERT_READOUT_NON_NULL(ro, false);
    PM_ASSERT_READOUT_IMAGE(ro, false);
    PS_ASSERT_INT_NONNEGATIVE(order, false);
    PS_ASSERT_INT_NONNEGATIVE(iter, false);
    PS_ASSERT_FLOAT_LARGER_THAN(rej, 0.0, false);
    PS_ASSERT_FLOAT_LARGER_THAN(thresh, 0.0, false);

    bool mdok;                          // Status of MD lookup

    pmCell *cell = ro->parent;
    pmChip *chip = cell->parent;
    const char *chipName = psMetadataLookupStr(&mdok, chip->concepts, "CHIP.NAME"); // Name of chip
    const char *cellName = psMetadataLookupStr(&mdok, cell->concepts, "CELL.NAME"); // Name of cell

    psImage *image = ro->image;         // Image to correct
    psImage *mask = ro->mask;           // Mask for image
    int numCols = image->numCols, numRows = image->numRows; // Size of image

    psStats *stats = psStatsAlloc(PS_STAT_ROBUST_MEDIAN | PS_STAT_ROBUST_STDEV);
    psRandom *rng = psRandomAlloc(PS_RANDOM_TAUS); // Random number generator
    if (!psImageBackground(stats, NULL, ro->image, ro->mask, maskVal, rng)) {
	psWarning("Unable to calculate statistics on readout; skipping pattern correction for %s, %s.", chipName, cellName);
        psErrorClear();
        psFree(stats);
        psFree(rng);
	
	// EAM 20211011 : we used to mask cells which fail the above, but this seems excessive
        // psImageInit(image, NAN);
        // if (mask) {
        //     psBinaryOp(mask, mask, "|", psScalarAlloc(maskBad, PS_TYPE_IMAGE_MASK));
        // }
        // if (ro->variance) {
        //     psImageInit(image, NAN);
        // }
        return true;
    }

    // if USE_BACKGROUND_STDEV is TRUE, the observed standard deviation is used to set the
    // thresholds.  this is going to be an overestimate if there is any structure in the
    // image.  If FALSE, the thresholds are set based on poisson stats for the background
    // level.  We assume the gain is 1, so this is an overestimate if the gain is > 1

# if (USE_BACKGROUND_STDEV) 
    float lower = stats->robustMedian - thresh * stats->robustStdev; // Lower bound for data
    float upper = stats->robustMedian + thresh * stats->robustStdev; // Upper bound for data
    float background = stats->robustMedian;
# else
    // the signal we are looking for is a small variation on top of the background.  if
    // the background is uniform with only read noise + sky noise, then the pixel-to-pixel
    // stdev should only be due to known noise sources and predictable.  If the
    // pixel-to-pixel variations are from other features, then those variations will
    // probably dominate the row-by-row bias variations.

    // instead of using the image pixel statistics to measure the stdev, lets assume only
    // dark noise plus poisson sky noise.  we are not carrying in the read noise, but it is
    // fairly modest for GPC1 (~10 DN)

    // if we assume a gain of 1 and the read noise of 10 DN, then a sky of 200 would have
    // a noise of N = sqrt (1 * 200 + 10^2) = sqrt (300) ~ 17

    // if the gain were as much as 2, then the noise in DN would be N = sqrt(2 * (200 + 100)) / 2 = sqrt(300) / sqrt(2)
    // so smaller by a factor of 1.4 than what we predict, which is not very large

    // find the nominal signal amplitude (check the ghost and/or crosstalk recipe file)
    float nominalAmplitude = psMetadataLookupF32 (&mdok, cell->analysis, "PTN.ROW.AMP");
    if (!mdok) nominalAmplitude = 20; // XXX EAM : somewhat arbitrary number
    if (!isfinite(nominalAmplitude)) nominalAmplitude = 20; // XXX EAM : somewhat arbitrary number
    // If we cannot determine the nominal amplitude, we fall-back on the worst case

    // retrieve noise and gain from 'concepts' if possible
# define READNOISE 10 /* arbitrary number */
    float sigma = sqrt(stats->robustMedian + PS_SQR(READNOISE));
    float delta = PS_MIN (thresh * sigma, 5*nominalAmplitude); 
    float lower = stats->robustMedian - delta; // Lower bound for data
    float upper = stats->robustMedian + delta; // Upper bound for data
    float background = stats->robustMedian;

    // if the noise from the background is too large 
    float significance = nominalAmplitude / sigma;
   
    // XXX EAM : arbitrary number
    if (isfinite(nominalAmplitude) && (significance < 1.0/6.0)) {
      psLogMsg("ppImage", PS_LOG_INFO, "Skipping row pattern correction for %s, %s, stats: %f - %f - %f : %f %f %f\n", chipName, cellName, lower, background, upper, sigma, nominalAmplitude, significance);
      psFree(stats);
      psFree(rng);
      return true; 
    }
    psLogMsg("ppImage", PS_LOG_INFO, "Performing row pattern correction for %s, %s, stats: %f - %f - %f : %f %f %f\n", chipName, cellName, lower, background, upper, sigma, nominalAmplitude, significance);
# endif    

    psFree(stats);
    psFree(rng);

    // the vector 'indices' maps the x-coordinate to a range [-1:1].  the element number (i) of indices
    // related to the x-coordinate (column number) by x = (i + 0.5) * NPIX

    int nSamples = numCols / NPIX;

    psVector *indices = psVectorAlloc(numCols, PS_TYPE_F32); // Indices for fit solutions
    psVector *xFit = psVectorAlloc(nSamples, PS_TYPE_F32); // x-coordinate for fitting
    psVector *yFit = psVectorAlloc(nSamples, PS_TYPE_F32); // flux values for fitting

    // xFit elements run from 0 - nSamples, element 'sample' corresponds to the middle of the bin sample*NPIX + 0.5*NPIX

    float norm = 2.0 / (float)numCols;  // Normalisation for indices
    for (int sample = 0; sample < nSamples; sample ++) {
	int x = (sample + 0.5)*NPIX;
        xFit->data.F32[sample] = x * norm - 1.0;
    }
    for (int x = 0; x < numCols; x ++) {
        indices->data.F32[x] = x * norm - 1.0;
    }

    psStats *clip = psStatsAlloc(clipMean | clipStdev); // Clipping statistics
    clip->clipIter = iter;
    clip->clipSigma = rej;
    psVector *clipMask = psVectorAlloc(nSamples, PS_TYPE_VECTOR_MASK); // Mask for clipping
    psPolynomial1D *poly = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, order); // Polynomial to fit
    psVector *data = psVectorAlloc(numCols, PS_TYPE_F32); // Data to fit

    psImage *corr = psImageAlloc(order + 1, numRows, PS_TYPE_F64); // Corrections applied
    psImageInit(corr, NAN);

    // CZW: 2011-11-30
    // Define the vectors to hold the "x" and "y" slope trends.
    // Briefly, the slope trend in the y-axis is a due to variations in the 0-th order term
    // of the PATTERN.ROW fit between individual rows across the cell.  Similarly, the 1-st
    // order term of the PATTERN.ROW fit defines the trend in the x-axis (as that's what we
    // are fitting with PATTERN.ROW in the first place).  However, the thing we're trying to
    // fix with PATTERN.ROW is the detector level bias wiggles.  These should be overlaid on
    // the true sky level.  Therefore, simply applying the PATTERN.ROW correction will
    // introduce cell-to-cell sky variations as these two trends are removed.  To avoid this,
    // We store the 0th and 1st order values used for each row, and then fit a polynomial to
    // these results.  By re-adding these systematic trends back, we can remove the row-to-row
    // variations without improperly removing the real sky trend.
    psVector *yaxisData = psVectorAlloc(numRows, PS_TYPE_F32); // Data to fit to the constant term
    psVector *yaxisMask = psVectorAlloc(numRows, PS_TYPE_VECTOR_MASK); // Mask for rows with no fit
    psVector *xaxisData = psVectorAlloc(numRows, PS_TYPE_F32); // Data to fit to the linear term
    psVectorInit(yaxisMask, 0);

    // validNmin is the minimum number of samples needed to measure the trend.
    // this should be tunable, but let's try 5 - 10%
    int validNmin = PS_MAX (nSamples * 0.1, order + 2);

    for (int y = 0; y < numRows; y++) {
        psVectorInit(clipMask, 0);
        data = psImageRow(data, image, y);
        int num = 0;                    // Number of good pixels

	// if the unmasked pixels only span a small range in x then we cannot fit the
	// 2nd order polynomial variations very well.  Require a minimum fractional range
	float validXmin = +1;
	float validXmax = -1;

        for (int sample = 0; sample < nSamples; sample ++) {

	    // store valid samples in the array to be sorted
	    float sampleArray[NPIX];
	    int seq = 0;
	    for (int j = 0; j < NPIX; j++) {
		int pix = sample  * NPIX + j; // real pixel elements in x-dir
		psAssert (pix >= 0, "invalid pix value");
		psAssert (pix < numCols, "invalid pix value");
		if ((mask && mask->data.PS_TYPE_IMAGE_MASK_DATA[y][pix] & maskVal)) continue;
		if (data->data.F32[pix] < lower || data->data.F32[pix] > upper) continue;
		sampleArray[seq] = data->data.F32[pix]; // store the value to be sorted
		seq ++;
            } 
	    if (seq < 1) {
		clipMask->data.PS_TYPE_VECTOR_MASK_DATA[sample] = 0xFF;
		yFit->data.F32[sample] = NAN;
		continue;
	    }
	    // note that we are treating the x-coordinate as the center
	    // of the binned pixel group, even if some or most pixels have
	    // been masked.  compared to the amplitude of the slope, this
	    // error is small
	    clipMask->data.PS_TYPE_VECTOR_MASK_DATA[sample] = 0;
	    validXmin = PS_MIN(xFit->data.F32[sample], validXmin);
	    validXmax = PS_MAX(xFit->data.F32[sample], validXmax);
	    num++;

	    // PSSORT operates on sampleArray (see define of macro SORT_SWAP above)
	    PSSORT (seq, SORT_COMPARE, SORT_SWAP, float);

	    int midPt = 0.5 * seq;
	    if (seq % 2 == 1) { psAssert (midPt >= 0, "invalid midPt"); }
	    if (seq % 2 == 0) { psAssert (midPt >= 1, "invalid midPt"); }
	    psAssert (midPt < NPIX, "invalid midPt");

	    float medValue = (seq % 2) ? sampleArray[midPt] : 0.5*(sampleArray[midPt] + sampleArray[midPt-1]);
	    yFit->data.F32[sample] = medValue;
        }

	// If not enough points are valid, skip
        if (num < validNmin) {
	    // Ignore this row in our subsequent fits, because the fit failed.
	    yaxisMask->data.PS_TYPE_VECTOR_MASK_DATA[y] = 0xFF;
            continue;
        }
	// XXX does this need to be a clipped fit if we are clipping based on the median poisson noise?
        if (!psVectorClipFitPolynomial1D(poly, clip, clipMask, 0xFF, yFit, NULL, xFit)) {
            psWarning("Unable to fit polynomial to row %d", y);
            psErrorClear();
	    // Ignore this row in our subsequent fits, because the fit failed.
	    yaxisMask->data.PS_TYPE_VECTOR_MASK_DATA[y] = 0xFF;
            continue;
        }
	// Store the results we found for this row.
	yaxisData->data.F32[y] = poly->coeff[0];
	xaxisData->data.F32[y] = poly->coeff[1];
	psTrace("pattern",1,"%d %g %g\n",y,poly->coeff[0],poly->coeff[1]);

        memcpy(corr->data.F64[y], poly->coeff, (order + 1) * PSELEMTYPE_SIZEOF(PS_TYPE_F64));
        psVector *solution = psPolynomial1DEvalVector(poly, indices); // Solution vector
        if (!solution) {
            psWarning("Unable to evaluate polynomial for row %d", y);
            psErrorClear();
	    yaxisMask->data.PS_TYPE_VECTOR_MASK_DATA[y] = 0xFF;
            continue;
        }

	psAssert (solution->n == numCols, "oops");
        for (int x = 0; x < numCols; x++) {
            image->data.F32[y][x] -= solution->data.F32[x];
	    psTrace("pattern",5,"A: %d %d %g\n",x,y,solution->data.F32[x]);
        }
        psFree(solution);
    }

    // Put the global trends back that were removed by the PATTERN.ROW correction.
    // Set up the indices for the polynomial
    psVector *yaxisIndices = psVectorAlloc(numRows, PS_TYPE_F32);
    norm = 2.0 / (float)numRows;
    for (int y = 0; y < numRows; y++) {
      yaxisIndices->data.F32[y] = y * norm - 1.0;
      psTrace("psModules.detrend.pattern",10,"%d %f %f\n",y,yaxisIndices->data.F32[y],yaxisData->data.F32[y]);
    }

    // Fit the trend of the constant term, producing the y-axis global trend
    psStatsInit(clip);
    psPolynomial1D *yaxisPoly = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, 1); // Polynomial to fit.
    if (!psVectorClipFitPolynomial1D(yaxisPoly,clip,yaxisMask,0xFF,yaxisData, NULL, yaxisIndices)) {
      psWarning("Unable to fit polynomial to y-axis trend");
      psErrorClear();
      // If we've failed, we need to do something, so add back in the background level, and
      // expect that the final image will have background mismatches.
      for (int y = 0; y < numRows; y++) {
	for (int x = 0; x < numCols; x++) {
	  image->data.F32[y][x] += background;
	}
	corr->data.F64[y][0]  -= background;
      }
    } else {
      psVector *solution = psPolynomial1DEvalVector(yaxisPoly,yaxisIndices);
      if (!solution) {
	psWarning("Unable to evaluate polynomial");
	psErrorClear();
	// If we've failed, we need to do something, so add back in the background level, and
	// expect that the final image will have background mismatches.
	for (int y = 0; y < numRows; y++) {
	  for (int x = 0; x < numCols; x++) {
	    image->data.F32[y][x] += background;
	  }
	  corr->data.F64[y][0]  -= background;
	}
      } else {
	psAssert (solution->n == numRows, "oops");
	for (int y = 0; y < numRows; y++) {
	  for (int x = 0; x < numCols; x++) {
	    image->data.F32[y][x] += solution->data.F32[y];
	    // XXX EAM : this was [x], which is wrong
	    psTrace("pattern",5,"B: %d %d %g\n",x,y,solution->data.F32[y]);
	  }
	  corr->data.F64[y][0]  -= solution->data.F32[y];
	}
      }
      psFree(solution);
    }      

    // Fit the trend of the linear term, producing the x-axis global trend
    // We can use the same mask vector, as the same rows failed the row-fit earlier.
    psStatsInit(clip);
    psPolynomial1D *xaxisPoly = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, 1); // Polynomial to fit.
    if (!psVectorClipFitPolynomial1D(xaxisPoly,clip,yaxisMask,0xFF,xaxisData, NULL, yaxisIndices)) {
      psWarning("Unable to fit polynomial to x-axis trend");
      psErrorClear();
    } else {
      psVector *solution = psPolynomial1DEvalVector(xaxisPoly,yaxisIndices);
      if (!solution) {
	psWarning("Unable to evaluate polynomial");
	psErrorClear();
      } else {
	psAssert (solution->n == numRows, "oops");
	for (int y = 0; y < numRows; y++) {
	  for (int x = 0; x < numCols; x++) {
	    image->data.F32[y][x] += solution->data.F32[y] * indices->data.F32[x];
	    // XXX EAM : this was set to [x] which is wrong (numCols > numRows)
	    psTrace("pattern",5,"C: %d %d %g %g\n",x,y,solution->data.F32[y],indices->data.F32[x]);
	  }
	  corr->data.F64[y][1]  -= solution->data.F32[y] ;
	}
      }
      psFree(solution);
    }
    psFree(yaxisPoly);
    psFree(xaxisPoly);
    psFree(yaxisIndices);
    psFree(yaxisMask);
    psFree(yaxisData);
    psFree(xaxisData);
    
    psMetadataAddImage(ro->analysis, PS_LIST_TAIL, PM_PATTERN_ROW_CORRECTION, PS_META_REPLACE,
                       "Pattern row correction", corr);
    psFree(corr);

    psFree(indices);
    psFree(xFit);
    psFree(yFit);
    psFree(clip);
    psFree(clipMask);
    psFree(poly);
    psFree(data);

    return true;
}

bool pmPatternRowApply(pmReadout *ro, psImageMaskType maskBad)
{
    PM_ASSERT_READOUT_NON_NULL(ro, false);
    PM_ASSERT_READOUT_IMAGE(ro, false);

    bool mdok;                          // Status of MD lookup
    psImage *corr = psMetadataLookupPtr(&mdok, ro->analysis, PM_PATTERN_ROW_CORRECTION); // Correction
    if (!mdok) {
        // No correction to apply
        return true;
    }

    psImage *image = ro->image; // Image of interest
    int numCols = image->numCols, numRows = image->numRows; // Size of image

    if (corr->numRows != numRows) {
        psError(PS_ERR_BAD_PARAMETER_SIZE, true,
                "Number of rows of image (%d) does not match number of rows of pattern correction (%d)\n",
                numRows, corr->numRows);
        return false;
    }

    int order = corr->numCols - 1;                                        // Polynomial order
    psPolynomial1D *poly = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, order); // Polynomial to apply
    psVector *indices = psVectorAlloc(numCols, PS_TYPE_F32); // Indices for polynomial
    float norm = 2.0 / (float)numCols;  // Normalisation for indices
    for (int x = 0; x < numCols; x++) {
        indices->data.F32[x] = x * norm - 1.0;
    }

    for (int y = 0; y < numRows; y++) {
        memcpy(poly->coeff, corr->data.F64[y], (order + 1) * PSELEMTYPE_SIZEOF(PS_TYPE_F64));
        psVector *solution = psPolynomial1DEvalVector(poly, indices); // Solution vector
        if (!solution) {
            psWarning("Unable to evaluate polynomial for row %d", y);
            psErrorClear();
            patternMaskRow(ro, y, maskBad);
            continue;
        }

        for (int x = 0; x < numCols; x++) {
            image->data.F32[y][x] -= solution->data.F32[x];
        }
        psFree(solution);
    }

    psFree(poly);
    psFree(indices);

    return true;
}


bool pmPatternCell(pmChip *chip, const psVector *tweak, psStatsOptions bgStat, psStatsOptions cellStat,
                   psImageMaskType maskVal, psImageMaskType maskBad)
{
    PS_ASSERT_PTR_NON_NULL(chip, false);
    PS_ASSERT_VECTOR_NON_NULL(tweak, false);
    PS_ASSERT_VECTOR_SIZE(tweak, chip->cells->n, false);
    PS_ASSERT_VECTOR_TYPE(tweak, PS_TYPE_U8, false);

    int numCells = tweak->n;            // Number of cells

    psVector *mean = psVectorAlloc(numCells, PS_TYPE_F32); // Mean for each cell
    psVector *meanMask = psVectorAlloc(numCells, PS_TYPE_VECTOR_MASK); // Mask for means
    psVectorInit(mean, NAN);
    psVectorInit(meanMask, 0);

    // Mask bits
    enum {
        PM_PATTERN_IGNORE = 0x01,       // Ignore this cell
        PM_PATTERN_TWEAK  = 0x02,       // Tweak this cell
        PM_PATTERN_ERROR  = 0x04,       // Error in calculating background
        PM_PATTERN_ALL    = 0xFF,       // All causes
    };

    // Count number of cells to tweak
    int numTweak = 0;                   // Number of cells to tweak
    int numIgnore = 0;                  // Number of cells to ignore
    for (int i = 0; i < numCells; i++) {
        pmCell *cell = chip->cells->data[i]; // Cell of interest
        if (!cell || !cell->data_exists || !cell->process ||
            cell->readouts->n == 0 || cell->readouts->n > 1 || !cell->readouts->data[0]) {
            numIgnore++;
            meanMask->data.PS_TYPE_VECTOR_MASK_DATA[i] = PM_PATTERN_IGNORE;
            continue;
        }
        if (tweak->data.U8[i]) {
            meanMask->data.PS_TYPE_VECTOR_MASK_DATA[i] = PM_PATTERN_TWEAK;
            numTweak++;
        }
    }
    if (numTweak == 0) {
        // Nothing to do
        psFree(mean);
        psFree(meanMask);
        return true;
    }
    if (numTweak >= numCells - numIgnore) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Cannot pattern-correct all cells within a chip.");
        psFree(mean);
        psFree(meanMask);
        return false;
    }

    // Measure mean of each cell
    // This is not really the perfect thing to do, which would be to take a common mean for the set of cells
    // which aren't being tweaked (because some cells will be heavily masked, so shouldn't be weighted the
    // same as pure cells), but it's simple and fast.
    psStats *bgStats = psStatsAlloc(bgStat); // Statistics on background
    psRandom *rng = psRandomAlloc(PS_RANDOM_TAUS); // Random number generator
    for (int i = 0; i < numCells; i++) {
        if (meanMask->data.PS_TYPE_VECTOR_MASK_DATA[i] & PM_PATTERN_IGNORE) {
            continue;
        }
        pmCell *cell = chip->cells->data[i]; // Cell of interest
        pmReadout *ro = cell->readouts->data[0]; // Readout of interest

        psStatsInit(bgStats);
#if 1
        if (!psImageBackground(bgStats, NULL, ro->image, ro->mask, maskVal, rng)) {
            psWarning("Unable to measure background for cell %d\n", i);
            psErrorClear();
            meanMask->data.PS_TYPE_VECTOR_MASK_DATA[i] |= PM_PATTERN_ERROR;
            continue;
        }
#else
        if (!psImageStats(bgStats, ro->image, ro->mask, maskVal)) {
            psWarning("Unable to measure background for cell %d\n", i);
            psErrorClear();
            meanMask->data.PS_TYPE_VECTOR_MASK_DATA[i] |= PM_PATTERN_ERROR;
            continue;
        }
#endif
        mean->data.F32[i] = psStatsGetValue(bgStats, bgStat);
        if (!isfinite(mean->data.F32[i])) {
            psWarning("Non-finite background for cell %d\n", i);
            psErrorClear();
            meanMask->data.PS_TYPE_VECTOR_MASK_DATA[i] |= PM_PATTERN_ERROR;
            continue;
        }
    }
    psFree(bgStats);
    psFree(rng);

    psStats *cellStats = psStatsAlloc(cellStat); // Statistics on cells
    if (!psVectorStats(cellStats, mean, NULL, meanMask, PM_PATTERN_ALL)) {
        // an error in psVectorStats implies a programming error
        psError(PS_ERR_UNKNOWN, false, "Unable to calculate mean cell background.");
        psFree(mean);
        psFree(meanMask);
        psFree(cellStats);
        return false;
    }

    float background = psStatsGetValue(cellStats, cellStat); // Background value for chip
    psFree(cellStats);
    if (!isfinite(background)) {
        // this can happen if all data in the image is bad -- in this case, do not correct, but
        // do not treat this as an error (other functions are responsible for check data validity)
        psLogMsg("psModules.detrend", PS_LOG_DETAIL, "Non-finite mean cell background -- skipping correction (data probabaly bad).");
        psFree(mean);
        psFree(meanMask);
        return true;
    }

    psLogMsg("psModules.detrend", PS_LOG_DETAIL, "Mean chip background is %f", background);

    for (int i = 0; i < numCells; i++) {
        if (meanMask->data.PS_TYPE_VECTOR_MASK_DATA[i] & PM_PATTERN_IGNORE) {
            continue;
        }
        if (!(meanMask->data.PS_TYPE_VECTOR_MASK_DATA[i] & PM_PATTERN_TWEAK)) {
            continue;
        }
        pmCell *cell = chip->cells->data[i]; // Cell of interest
        pmReadout *ro = cell->readouts->data[0]; // Readout of interest
        if (meanMask->data.PS_TYPE_VECTOR_MASK_DATA[i] & PM_PATTERN_ERROR) {
            psImageInit(ro->image, NAN);
            psBinaryOp(ro->mask, ro->mask, "|", psScalarAlloc(maskBad, PS_TYPE_IMAGE_MASK));
            psMetadataAddF32(ro->analysis, PS_LIST_TAIL, PM_PATTERN_CELL_CORRECTION, PS_META_REPLACE,
                             "Pattern cell correction solution", NAN);
            continue;
        }
        float correction = background - mean->data.F32[i]; // Correction to apply
        const char *cellName = psMetadataLookupStr(NULL, cell->concepts, "CELL.NAME"); // Name of cell
        psLogMsg("psModules.detrend", PS_LOG_DETAIL, "Correcting background of cell %s by %f",
                 cellName, correction);
        psBinaryOp(ro->image, ro->image, "+", psScalarAlloc(correction, PS_TYPE_F32));
        psMetadataAddF32(ro->analysis, PS_LIST_TAIL, PM_PATTERN_CELL_CORRECTION, PS_META_REPLACE,
                         "Pattern cell correction solution", correction);
    }

    psFree(mean);
    psFree(meanMask);

    return true;
}

// Compare the backs (cellBackgrounds) vector to each of the patterns loaded for this chip.
// Choose the one that matches best & mask as appropriate
// can we pass the backgrounds in using an image (8 x 8)
bool pmPatternDeadCells(pmChip *chip, const psVector *backs, psImageMaskType maskVal)
{
    PS_ASSERT_PTR_NON_NULL(chip, false);
    PS_ASSERT_VECTOR_NON_NULL(backs, false);
    PS_ASSERT_VECTOR_SIZE(backs, chip->cells->n, false);
    PS_ASSERT_VECTOR_TYPE(backs, PS_TYPE_F32, false);

    // Look for the dead cell pattern cube in the analysis metadata.
    // NOTE: not all chips have the pattern, skip if not found.
    bool mdok;
    psImage *deadCellPattern = (psImage *) psMetadataLookupPtr (&mdok, chip->analysis, "PTN.DEAD.CELL");
    if (!mdok) {
        psLogMsg("psModules.detrend", PS_LOG_DETAIL, "No DEAD CELL pattern for chip, skipping\n");
	return true;
    }

    int numCells = backs->n;            // Number of cells
    int numColsD = deadCellPattern->numCols;
    int numRowsD = deadCellPattern->numRows;

    assert (backs->n == numRowsD);

    // The background values need to be normalized (divide by the median).
    // First extract the valid data values
    psVector *valid = psVectorAllocEmpty(backs->n, PS_TYPE_F32); // Mean for each cell
    for (int i = 0; i < backs->n; i++) {
	float value = backs->data.F32[i];
	if (!isfinite(value)) { backs->data.F32[i] = NAN; continue; }
	if (value > 50000.0)  { backs->data.F32[i] = NAN; continue; } // XXX warning: hard-wired value
	psVectorAppend (valid, value);
    }
    if (valid->n == 0) {
	psLogMsg("psModules.detrend", PS_LOG_DETAIL, "No valid backgrounds, skipping\n");
	psFree (valid);
	return true;
    }

    // Second, calculate the median
    psVectorSortInPlace (valid);
    int midPt = valid->n / 2.0;
    float median = valid->n % 2 ? valid->data.F32[midPt] : 0.5*(valid->data.F32[midPt] + valid->data.F32[midPt-1]);
    psFree (valid);

    // Finally, renormalize:
    for (int i = 0; i < backs->n; i++) {
	if (!isfinite(backs->data.F32[i])) { continue; }
	backs->data.F32[i] /= median;
    }

    // there are 2 columns (value & mask) for each mode, plus the constant mode
    int nModes = numColsD / 2 + 1;
    
    psVector *stdev = psVectorAlloc(nModes, PS_TYPE_F32); // Mean for each cell

    // measure stdev for each comparison
    for (int i = 0; i < nModes; i++) {
	float Sum1 = 0.0;
	float Sum2 = 0.0;
	int   Npts = 0;

	// choose the column with the background values for this mode
	int nModeColumn = 2*(i - 1);

	for (int j = 0; j < backs->n; j++) {

	    float valObs = backs->data.F32[j];
	    float valRef = (i == 0) ? 0.0 : deadCellPattern->data.F32[j][nModeColumn];

	    if (!isfinite(valObs)) continue;
	    if (!isfinite(valRef)) continue;

	    float dS = valObs - valRef;
	    Sum1 += dS;
	    Sum2 += dS*dS;
	    Npts ++;
	}
	if (Npts == 0) {
	    // is this is an error?
	    // no valid data, ignore this test
	    psLogMsg("psModules.detrend", PS_LOG_DETAIL, "No valid backgrounds, skipping\n");
	    psFree (stdev);
	    return true;
	}

	float mean = Sum1 / Npts;
	float sigma = sqrt(Sum2 / Npts - mean*mean);
	stdev->data.F32[i] = sigma;
	fprintf (stderr, "mode: %d, stdev: %f\n", i, sigma);
    }

    // loop over stdev and choose the lowest one
    int   minI = 0;
    float minV = stdev->data.F32[minI];

    for (int i = 1; i < nModes; i++) {
	if (stdev->data.F32[i] < minV) {
	    minI = i;
	    minV = stdev->data.F32[i];
	}
    }
    psFree (stdev);

    if (minI == 0) return true;

    int nModeColumnMask = 2*(minI - 1) + 1;

    // now mask bad cells
    for (int i = 0; i < numCells; i++) {
        if (deadCellPattern->data.F32[i][nModeColumnMask] > 0) continue;

        pmCell *cell = chip->cells->data[i]; // Cell of interest
        pmReadout *ro = cell->readouts->data[0]; // Readout of interest

	psImage *mask = ro->mask; // mask of interest
	int numCols = mask->numCols, numRows = mask->numRows; // Size of image
	
	for (int y = 0; y < numRows; y++) {
	    for (int x = 0; x < numCols; x++) {
		mask->data.PS_TYPE_IMAGE_MASK_DATA[y][x] |= maskVal;
	    }
	}
    }
    return true;
}

bool pmPatternCellApply(pmReadout *ro, psImageMaskType maskBad)
{
    PM_ASSERT_READOUT_NON_NULL(ro, false);
    PM_ASSERT_READOUT_IMAGE(ro, false);

    bool mdok;                          // Status of MD lookup
    float corr = psMetadataLookupF32(&mdok, ro->analysis, PM_PATTERN_CELL_CORRECTION); // Correction to apply
    if (!mdok) {
        // No correction to apply
        return true;
    }

    psImage *image = ro->image, *mask = ro->mask; // Image and mask of interest
    int numCols = image->numCols, numRows = image->numRows; // Size of image

    if (!isfinite(corr)) {
        for (int y = 0; y < numRows; y++) {
            for (int x = 0; x < numCols; x++) {
                image->data.F32[y][x] = NAN;
            }
        }
        if (mask) {
            for (int y = 0; y < numRows; y++) {
                for (int x = 0; x < numCols; x++) {
                    mask->data.PS_TYPE_IMAGE_MASK_DATA[y][x] |= maskBad;
                }
            }
        }
    } else {
        for (int y = 0; y < numRows; y++) {
            for (int x = 0; x < numCols; x++) {
                image->data.F32[y][x] += corr;
            }
        }
    }

    return true;
}



bool pmPatternContinuity(pmChip *chip, const psVector *tweak, psStatsOptions bgStat, psStatsOptions cellStat,
			 psImageMaskType maskVal, psImageMaskType maskBad, int edgeWidth)
{
    PS_ASSERT_PTR_NON_NULL(chip, false);
    PS_ASSERT_VECTOR_NON_NULL(tweak, false);
    PS_ASSERT_VECTOR_SIZE(tweak, chip->cells->n, false);
    PS_ASSERT_VECTOR_TYPE(tweak, PS_TYPE_U8, false);

    int numCells = tweak->n;            // Number of cells

    psVector *meanMask = psVectorAlloc(numCells, PS_TYPE_VECTOR_MASK); // Mask for means
    psVectorInit(meanMask, 0);

    // Mask bits
    enum {
        PM_PATTERN_IGNORE = 0x01,       // Ignore this cell
        PM_PATTERN_TWEAK  = 0x02,       // Tweak this cell
        PM_PATTERN_ERROR  = 0x04,       // Error in calculating background
        PM_PATTERN_ALL    = 0xFF,       // All causes
    };

    // Count number of cells to tweak
    int numTweak = 0;                   // Number of cells to tweak
    int numIgnore = 0;                  // Number of cells to ignore
    for (int i = 0; i < numCells; i++) {
        pmCell *cell = chip->cells->data[i]; // Cell of interest
        if (!cell || !cell->data_exists || !cell->process ||
            cell->readouts->n == 0 || cell->readouts->n > 1 || !cell->readouts->data[0]) {
            numIgnore++;
            meanMask->data.PS_TYPE_VECTOR_MASK_DATA[i] = PM_PATTERN_IGNORE;
            continue;
        }
        if (tweak->data.U8[i]) {
            meanMask->data.PS_TYPE_VECTOR_MASK_DATA[i] = PM_PATTERN_TWEAK;
            numTweak++;
        }
    }
    if (numTweak == 0) {
        // Nothing to do
        psFree(meanMask);
        return true;
    }

    // Measure mean of each cell edge, and use that to determine the cell offsets.

    psStats *bgStats = psStatsAlloc(bgStat); // Statistics on background
    psRandom *rng = psRandomAlloc(PS_RANDOM_TAUS); // Random number generator

    psRegion region = {0,0,0,0};

    /* These images hold the edge data for the OTA structure.  */
    psImage *A = psImageAlloc(8,8,PS_TYPE_F64); // Top edge
    psImage *B = psImageAlloc(8,8,PS_TYPE_F64); // Bottom edge
    psImage *C = psImageAlloc(8,8,PS_TYPE_F64); // Right edge
    psImage *D = psImageAlloc(8,8,PS_TYPE_F64); // Left edge
    psImageInit(A,0.0);
    psImageInit(B,0.0);
    psImageInit(C,0.0);
    psImageInit(D,0.0);
    
    for (int i = 0; i < numCells; i++) {
        if (meanMask->data.PS_TYPE_VECTOR_MASK_DATA[i] & PM_PATTERN_IGNORE) {
            continue;
        }
        pmCell *cell = chip->cells->data[i]; // Cell of interest
        pmReadout *ro = cell->readouts->data[0]; // Readout of interest

        psStatsInit(bgStats);

	// Convert cell iterator i into an xy coordinate on the grid of cells
	int y = (i % 8);
	int x = (i - y) / 8;
	
	for (int j = 0; j < 4; j++) {
	  if (j == 0) {  // Region B
	    region = psRegionSet(0,ro->image->numCols,
				 0,edgeWidth);
	  }
	  else if (j == 1) { // Region A
	    region = psRegionSet(0,ro->image->numCols,
				 ro->image->numRows - edgeWidth,ro->image->numRows);
	  }
	  else if (j == 2) { // Region D
	    region = psRegionSet(0,edgeWidth,
				 0,ro->image->numRows);
	  }
	  else if (j == 3) { // Region C
	    region = psRegionSet(ro->image->numCols - edgeWidth,ro->image->numCols,
				 0,ro->image->numRows);
	  }
	  psImage *subset  = psImageSubset(ro->image,region);
	  psImage *submask = psImageSubset(ro->mask,region);

	  if (!psImageBackground(bgStats, NULL, subset, submask, maskVal, rng)) {
            psWarning("Unable to measure background for cell %d on edge %d\n", i, j);
            psErrorClear();
            meanMask->data.PS_TYPE_VECTOR_MASK_DATA[i] |= PM_PATTERN_ERROR;
	    if (j == 0)      { B->data.F64[y][x] = NAN; }
	    else if (j == 1) { A->data.F64[y][x] = NAN; }
	    else if (j == 2) { C->data.F64[y][x] = NAN; }
	    else if (j == 3) { D->data.F64[y][x] = NAN; }
	    psFree(subset);
	    psFree(submask);
            continue; // Move on to next edge, as only part of this cell may be a problem
	  }
 
	  // If the returned value is zero, assume something is wrong.  Do I still need this?
	  if (psStatsGetValue(bgStats,bgStat) < 1e-6) {
	    if (j == 0)      { B->data.F64[y][x] = NAN; }
	    else if (j == 1) { A->data.F64[y][x] = NAN; }
	    else if (j == 2) { C->data.F64[y][x] = NAN; }
	    else if (j == 3) { D->data.F64[y][x] = NAN; }
	  }
	  // If we have an error for this cell/edge, make sure we mask the value
	  if (meanMask->data.PS_TYPE_VECTOR_MASK_DATA[i] & PM_PATTERN_ERROR) {
	    if (j == 0)      { B->data.F64[y][x] = NAN; }
	    else if (j == 1) { A->data.F64[y][x] = NAN; }
	    else if (j == 2) { C->data.F64[y][x] = NAN; }
	    else if (j == 3) { D->data.F64[y][x] = NAN; }
	  }
	  else { // Set the value to match what we got from the edge box.
	    if (j == 0)      { B->data.F64[y][x] = psStatsGetValue(bgStats,bgStat); }
	    else if (j == 1) { A->data.F64[y][x] = psStatsGetValue(bgStats,bgStat); }
	    else if (j == 2) { C->data.F64[y][x] = psStatsGetValue(bgStats,bgStat); }
	    else if (j == 3) { D->data.F64[y][x] = psStatsGetValue(bgStats,bgStat); }
	  }

	  for (int u = 0; u < subset->numCols; u++) {
	    for (int v = 0; v < subset->numRows; v++) {
	      psTrace("psModules.detrend.cont",10,"BOX: %d %d (%d %d) (%d %d) %f %d",
		      i,j,x,y,u,v,subset->data.F32[v][u],submask->data.PS_TYPE_IMAGE_MASK_DATA[v][u]);
	    }
	  }	  
	  
	  psFree(subset);
	  psFree(submask);

	}
	psTrace("psModules.detrend.cont",5, "CELL: %d (%d %d) A: %f B: %f C: %f D: %f",
		i,x,y,
		A->data.F64[y][x],B->data.F64[y][x],C->data.F64[y][x],D->data.F64[y][x]);		
    }
    psFree(bgStats);
    psFree(rng);

    // We've now allocated all the edge values, so we can now minimize the offsets.
    // This involves solving the equation A x = b, where
    // A is the (64x64 for GPC1) matrix containing the edges that match for each cell
    // x is the solution vector
    // b is the combination of offsets across each cell boundary for each cell.
    // Below "XX" is used as the matrix A, and "solution" is used as both b and x
    //   (due to the way psMatrixLUSolve operates).
    psVector *solution = psVectorAlloc(64,PS_TYPE_F64);
    psImage  *XX       = psImageAlloc(64,64,PS_TYPE_F64);
    psVectorInit(solution,0.0);
    psImageInit(XX,0.0);
    
    for (int i = 0; i < numCells; i++) {
      // Accumulate all the possible edge differences we can for this cell.
      // As we do so, make a note of the correlations by incrementing the element of the matrix.
      int y = (i % 8);
      int x = (i - y) / 8;
      int j;
      double critical_value = 0.0;
      if (meanMask->data.PS_TYPE_VECTOR_MASK_DATA[i] & PM_PATTERN_IGNORE) {
	continue;
      }
      if (x + 1 < 8) {  // We have a neighbor adjacent in the +x direction
	j = 8 * (x + 1) + y; // Determine that neighbor's index
	if (fabs(C->data.F64[y][x]) > fabs(D->data.F64[y][x+1])) {
	  critical_value = 2.0 * fabs(D->data.F64[y][x+1]);
	}
	else {
	  critical_value = 2.0 * fabs(C->data.F64[y][x]);
	}
	if (critical_value < 25) { critical_value = 25; }
	psTrace("psModules.detrend.cont",5,"CmD %d %d %d %d %g %g %g", // diagnostic
		i,x,y,j,
		C->data.F64[y][x],
		D->data.F64[y][x+1],
		critical_value
		);
	if (!(meanMask->data.PS_TYPE_VECTOR_MASK_DATA[j] & PM_PATTERN_IGNORE)&&  // If there are no errors with the neighbor,
	    (isfinite(C->data.F64[y][x]))&&(isfinite(D->data.F64[y][x+1]))&&     // and all edges have valid values,
	    (fabs(C->data.F64[y][x] - D->data.F64[y][x+1]) < critical_value)     // and there are no large discontinuities,
	    ) {    
	  solution->data.F64[i] += C->data.F64[y][x] - D->data.F64[y][x+1];     // Take the difference
	  XX->data.F64[i][i] += 1;                                              // increment our relation with ourself
	  XX->data.F64[i][j] += -1;                                             // decrement our relation with the neighbor
	}
      }
      if (x - 1 > -1) { // etc.
	j = 8 * (x - 1) + y;
	if (fabs(C->data.F64[y][x-1]) > fabs(D->data.F64[y][x])) {
	  critical_value = 2.0 * fabs(D->data.F64[y][x]);
	}
	else {
	  critical_value = 2.0 * fabs(C->data.F64[y][x-1]);
	}
	if (critical_value < 25) { critical_value = 25; }
	psTrace("psModules.detrend.cont",5,"DmC %d %d %d %d %g %g %g",
		i,x,y,j,
		D->data.F64[y][x],
		C->data.F64[y][x-1],
		critical_value
		);

	if (!(meanMask->data.PS_TYPE_VECTOR_MASK_DATA[j] & PM_PATTERN_IGNORE)&&
	    (isfinite(D->data.F64[y][x]))&&(isfinite(C->data.F64[y][x-1]))&&
	    (fabs(D->data.F64[y][x] - C->data.F64[y][x-1]) < critical_value)
	    ) {
	  solution->data.F64[i] += D->data.F64[y][x] - C->data.F64[y][x-1];
	  XX->data.F64[i][i] += 1;
	  XX->data.F64[i][j] += -1;
	}
      }
      if (y + 1 < 8) {
	j = 8 * x + (y + 1);
	psTrace("psModules.detrend.cont",5,"AmB %d %d %d %d %g %g",
		i,x,y,j,
		A->data.F64[y][x],
		B->data.F64[y+1][x]
		);
	if (fabs(A->data.F64[y][x]) > fabs(B->data.F64[y+1][x])) {
	  critical_value = 2.0 * fabs(B->data.F64[y+1][x]);
	}
	else {
	  critical_value = 2.0 * fabs(A->data.F64[y][x]);
	}
	if (critical_value < 25) { critical_value = 25; }
	if (!(meanMask->data.PS_TYPE_VECTOR_MASK_DATA[j] & PM_PATTERN_IGNORE)&&
	    (isfinite(A->data.F64[y][x]))&&(isfinite(B->data.F64[y+1][x]))&&
	    (fabs(A->data.F64[y][x] - B->data.F64[y+1][x]) < critical_value)
	    ) {
	  solution->data.F64[i] += A->data.F64[y][x] - B->data.F64[y+1][x];
	  XX->data.F64[i][i] += 1;
	  XX->data.F64[i][j] += -1;
	}
      }
      if (y - 1 > -1) {
	j = 8 * x +  (y - 1);
	psTrace("psModules.detrend.cont",5,"BmA %d %d %d %d %g %g",
		i,x,y,j,
		B->data.F64[y][x],
		A->data.F64[y-1][x]
		);
	if (fabs(A->data.F64[y-1][x]) > fabs(B->data.F64[y][x])) {
	  critical_value = 2.0 * fabs(B->data.F64[y][x]);
	}
	else {
	  critical_value = 2.0 * fabs(A->data.F64[y-1][x]);
	}
	if (critical_value < 25) { critical_value = 25; }
	if (!(meanMask->data.PS_TYPE_VECTOR_MASK_DATA[j] & PM_PATTERN_IGNORE)&&
	    (isfinite(B->data.F64[y][x]))&&(isfinite(A->data.F64[y-1][x]))&&
	    (fabs(B->data.F64[y][x] - A->data.F64[y-1][x]) < critical_value)
	    ) {
	  solution->data.F64[i] += B->data.F64[y][x] - A->data.F64[y-1][x];
	  XX->data.F64[i][i] += 1;
	  XX->data.F64[i][j] += -1;
	}
      }
    }
    double max_XX = 0;
    double solution_V = 0;
    int i_peak = -1;
    for (int i = 0; i < numCells; i++) { // If any cells have no value of themself, set the matrix to 1.0.
      if (XX->data.F64[i][i] == 0.0) {
	XX->data.F64[i][i] = 1.0;
      }
      if (XX->data.F64[i][i] > max_XX) {
	max_XX = XX->data.F64[i][i];
	solution_V = solution->data.F64[i];
	i_peak = i;
      }
    }
    psTrace("psModules.detrend.cont",5,"fixed point: %d %g\n",
	    i_peak,solution_V);

    for (int i = 0; i < numCells; i++) {
/*        if (!((XX->data.F64[i][i] == 1.0)&& */
/*  	    (solution->data.F64[i] == 0.0))) { */
	solution->data.F64[i] -= solution_V;
	if (i != i_peak) {
	  for (int j = 0; j < numCells; j++) {
	    XX->data.F64[i][j] -= XX->data.F64[i_peak][j];
	  }
	}
/*        } */
    }
    for (int i = 0; i < numCells; i++) {
      XX->data.F64[i_peak][i] = 0.0;
    }
    XX->data.F64[i_peak][i_peak] = 1.0;
    
    
#if (1)
    for (int i = 0; i < numCells; i++) { // print matrix A
      psTrace("psModules.detrend.cont",5,"A: %3d % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f",
	      i,
	      XX->data.F64[i][0],	      XX->data.F64[i][1],	      XX->data.F64[i][2],	      XX->data.F64[i][3],
	      XX->data.F64[i][4],	      XX->data.F64[i][5],	      XX->data.F64[i][6],	      XX->data.F64[i][7],
	      XX->data.F64[i][8],	      XX->data.F64[i][9],	      XX->data.F64[i][10],	      XX->data.F64[i][11],
	      XX->data.F64[i][12],	      XX->data.F64[i][13],	      XX->data.F64[i][14],	      XX->data.F64[i][15],
	      XX->data.F64[i][16],	      XX->data.F64[i][17],	      XX->data.F64[i][18],	      XX->data.F64[i][19],
	      XX->data.F64[i][20],	      XX->data.F64[i][21],	      XX->data.F64[i][22],	      XX->data.F64[i][23],
	      XX->data.F64[i][24],	      XX->data.F64[i][25],	      XX->data.F64[i][26],	      XX->data.F64[i][27],
	      XX->data.F64[i][28],	      XX->data.F64[i][29],	      XX->data.F64[i][30],	      XX->data.F64[i][31],
	      XX->data.F64[i][32],	      XX->data.F64[i][33],	      XX->data.F64[i][34],	      XX->data.F64[i][35],
	      XX->data.F64[i][36],	      XX->data.F64[i][37],	      XX->data.F64[i][38],	      XX->data.F64[i][39],
	      XX->data.F64[i][40],	      XX->data.F64[i][41],	      XX->data.F64[i][42],	      XX->data.F64[i][43],
	      XX->data.F64[i][44],	      XX->data.F64[i][45],	      XX->data.F64[i][46],	      XX->data.F64[i][47],
	      XX->data.F64[i][48],	      XX->data.F64[i][49],	      XX->data.F64[i][50],	      XX->data.F64[i][51],
	      XX->data.F64[i][52],	      XX->data.F64[i][53],	      XX->data.F64[i][54],	      XX->data.F64[i][55],
	      XX->data.F64[i][56],	      XX->data.F64[i][57],	      XX->data.F64[i][58],	      XX->data.F64[i][59],
	      XX->data.F64[i][60],	      XX->data.F64[i][61],	      XX->data.F64[i][62],	      XX->data.F64[i][63]
	      );
    }

    for (int i = 0; i < numCells; i++) { // print vector b
      psTrace("psModules.detrend.cont",5,"b: %d %f",
	      i,
	      solution->data.F64[i]
	      );
    }
#endif    
    
    // Solve the Ax=b equation
    //    psMatrixLUSolve(XX,solution);
    psMatrixGJSolve(XX,solution);
#if (1)
    for (int i = 0; i < numCells; i++) { // print vector b
      psTrace("psModules.detrend.cont",5,"x: %d %f",
	      i,
	      solution->data.F64[i]
	      );
    }
#endif
    
    /* old code to remove the minimum solution value from the set, to give a "minimal set of offsets." Mathematically unnecessary. */
/*     double min = 99e99; */
/*     for (int i = 0; i < numCells; i++) { */
/*       if (solution->data.F64[i] < min) { */
/* 	min = solution->data.F64[i]; */
/*       } */
/*       psTrace("psModules.detrend.cont",5,"x: %d %f %f ", */
/* 	      i, */
/* 	      solution->data.F64[i],min */
/* 	      ); */
/*     } */
/*     for (int i = 0; i < numCells; i++) { */
/* 	if (solution->data.F64[i] != 0.0) { */
/* 	  solution->data.F64[i] -= min; */
/* 	} */
/*     } */

    // Cleanup
    psFree(XX);
    psFree(A);
    psFree(B);
    psFree(C);
    psFree(D);

    // Correct cells based on the offsets calculated, and store the result in the analysis metadata.
    for (int i = 0; i < numCells; i++) {
        if (meanMask->data.PS_TYPE_VECTOR_MASK_DATA[i] & PM_PATTERN_IGNORE) {
            continue;
        }
        if (!(meanMask->data.PS_TYPE_VECTOR_MASK_DATA[i] & PM_PATTERN_TWEAK)) {
            continue;
        }
        pmCell *cell = chip->cells->data[i]; // Cell of interest
        pmReadout *ro = cell->readouts->data[0]; // Readout of interest

        float correction = solution->data.F64[i];
        const char *cellName = psMetadataLookupStr(NULL, cell->concepts, "CELL.NAME"); // Name of cell
        psLogMsg("psModules.detrend", PS_LOG_DETAIL, "Correcting background of cell %s by %f",
                 cellName, correction);
        psBinaryOp(ro->image, ro->image, "-", psScalarAlloc(correction, PS_TYPE_F32));
        psMetadataAddF32(ro->analysis, PS_LIST_TAIL, PM_PATTERN_CELL_CORRECTION, PS_META_REPLACE,
                         "Pattern cell correction solution", correction);
    }

    psFree(solution);
    psFree(meanMask);

    return true;
}

bool pmPatternContinuityApply(pmReadout *ro, psImageMaskType maskBad)
{
    PM_ASSERT_READOUT_NON_NULL(ro, false);
    PM_ASSERT_READOUT_IMAGE(ro, false);

    bool mdok;                          // Status of MD lookup
    float corr = psMetadataLookupF32(&mdok, ro->analysis, PM_PATTERN_CELL_CORRECTION); // Correction to apply
    if (!mdok) {
        // No correction to apply
        return true;
    }

    psImage *image = ro->image, *mask = ro->mask; // Image and mask of interest
    int numCols = image->numCols, numRows = image->numRows; // Size of image

    if (!isfinite(corr)) {
        for (int y = 0; y < numRows; y++) {
            for (int x = 0; x < numCols; x++) {
                image->data.F32[y][x] = NAN;
            }
        }
        if (mask) {
            for (int y = 0; y < numRows; y++) {
                for (int x = 0; x < numCols; x++) {
                    mask->data.PS_TYPE_IMAGE_MASK_DATA[y][x] |= maskBad;
                }
            }
        }
    } else {
        for (int y = 0; y < numRows; y++) {
            for (int x = 0; x < numCols; x++) {
                image->data.F32[y][x] += corr;
            }
        }
    }

    return true;
}

// This should reuse code more efficiently
bool pmPatternContinuityBackground(pmFPAfile *in, pmFPAfile *out,  psStatsOptions bgStat, psStatsOptions cellStat,
				   psImageMaskType maskVal, psImageMaskType maskBad, int edgeWidth)
{
    PS_ASSERT_PTR_NON_NULL(in, false);
    PS_ASSERT_PTR_NON_NULL(out, false);

    int numChips = out->fpa->chips->n;            // Number of cells

    psVector *meanMask = psVectorAlloc(numChips, PS_TYPE_VECTOR_MASK); // Mask for means
    psVectorInit(meanMask, 0);

    // Mask bits
    enum {
        PM_PATTERN_IGNORE = 0x01,       // Ignore this cell
        PM_PATTERN_TWEAK  = 0x02,       // Tweak this cell
        PM_PATTERN_ERROR  = 0x04,       // Error in calculating background
        PM_PATTERN_ALL    = 0xFF,       // All causes
    };

    // Measure mean of each cell edge, and use that to determine the cell offsets.
    psStatsOptions stat = cellStat;     // Define which statistic to use.
    
    psStats *bgStats = psStatsAlloc(stat); // Statistics on background
    psRandom *rng = psRandomAlloc(PS_RANDOM_TAUS); // Random number generator

    psRegion region = {0,0,0,0};

    /* These images hold the edge data for the OTA structure.  */
    psImage *A = psImageAlloc(8,8,PS_TYPE_F64); // Top edge
    psImage *B = psImageAlloc(8,8,PS_TYPE_F64); // Bottom edge
    psImage *C = psImageAlloc(8,8,PS_TYPE_F64); // Right edge
    psImage *D = psImageAlloc(8,8,PS_TYPE_F64); // Left edge
    psImageInit(A,0.0);
    psImageInit(B,0.0);
    psImageInit(C,0.0);
    psImageInit(D,0.0);

    // Corners don't exist.
    A->data.F64[0][0] = NAN;
    B->data.F64[0][0] = NAN;
    C->data.F64[0][0] = NAN;
    D->data.F64[0][0] = NAN;
    A->data.F64[7][0] = NAN;
    B->data.F64[7][0] = NAN;
    C->data.F64[7][0] = NAN;
    D->data.F64[7][0] = NAN;
    A->data.F64[0][7] = NAN;
    B->data.F64[0][7] = NAN;
    C->data.F64[0][7] = NAN;
    D->data.F64[0][7] = NAN;
    A->data.F64[7][7] = NAN;
    B->data.F64[7][7] = NAN;
    C->data.F64[7][7] = NAN;
    D->data.F64[7][7] = NAN;
    
    for (int i = 0; i < numChips; i++) {
      pmChip *chip = out->fpa->chips->data[i];
      pmCell *cell = chip->cells->data[0]; // Cell of interest

      
      psStatsInit(bgStats);

      // Convert cell iterator i into an xy coordinate on the grid of cells
      // This is wrong for chips
      const char *chipName = psMetadataLookupStr(NULL,chip->concepts, "CHIP.NAME");
      int xParity          = psMetadataLookupS16(NULL,chip->concepts, "CHIP.XPARITY");
      int yParity          = psMetadataLookupS16(NULL,chip->concepts, "CHIP.YPARITY");
      int x = chipName[2] - '0';
      int y = chipName[3] - '0';

      if ((cell->readouts->n != 1)||(!chip->data_exists))  {
	A->data.F64[y][x] = NAN;
	B->data.F64[y][x] = NAN;
	C->data.F64[y][x] = NAN;
	D->data.F64[y][x] = NAN;
	continue;
      }
      pmReadout *ro = cell->readouts->data[0]; // Readout of interest      
      for (int j = 0; j < 4; j++) {
	if (j == 0) {  // Region B
	  region = psRegionSet(0,ro->image->numCols,
			       0,edgeWidth);
	}
	else if (j == 1) { // Region A
	  region = psRegionSet(0,ro->image->numCols,
			       ro->image->numRows - edgeWidth,ro->image->numRows);
	}
	else if (j == 2) { // Region D
	  region = psRegionSet(0,edgeWidth,
			       0,ro->image->numRows);
	}
	else if (j == 3) { // Region C
	  region = psRegionSet(ro->image->numCols - edgeWidth,ro->image->numCols,
			       0,ro->image->numRows);
	}
	psImage *subset  = psImageSubset(ro->image,region);

	if (!psImageBackground(bgStats, NULL, subset, NULL, maskVal, rng)) {
	  psWarning("Unable to measure background for cell %d on edge %d\n", i, j);
	  psErrorClear();
	  meanMask->data.PS_TYPE_VECTOR_MASK_DATA[i] |= PM_PATTERN_ERROR;
	  if (j == 0)      { B->data.F64[y][x] = NAN; }
	  else if (j == 1) { A->data.F64[y][x] = NAN; }
	  else if (j == 2) { C->data.F64[y][x] = NAN; }
	  else if (j == 3) { D->data.F64[y][x] = NAN; }
	  psFree(subset);
	  continue; // Move on to next edge, as only part of this cell may be a problem
	}
	
	// If the returned value is zero, assume something is wrong.  Do I still need this?
	if (psStatsGetValue(bgStats,stat) < 1e-6) {
	  if (j == 0)      { B->data.F64[y][x] = NAN; }
	  else if (j == 1) { A->data.F64[y][x] = NAN; }
	  else if (j == 2) { C->data.F64[y][x] = NAN; }
	  else if (j == 3) { D->data.F64[y][x] = NAN; }
	}
	// If we have an error for this cell/edge, make sure we mask the value
	if (meanMask->data.PS_TYPE_VECTOR_MASK_DATA[i] & PM_PATTERN_ERROR) {
	  if (j == 0)      { B->data.F64[y][x] = NAN; }
	  else if (j == 1) { A->data.F64[y][x] = NAN; }
	  else if (j == 2) { C->data.F64[y][x] = NAN; }
	  else if (j == 3) { D->data.F64[y][x] = NAN; }
	}
	else { // Set the value to match what we got from the edge box.
	  if (xParity == -1) {
	    if (j == 2)      { C->data.F64[y][x] = psStatsGetValue(bgStats,stat); }
	    else if (j == 3) { D->data.F64[y][x] = psStatsGetValue(bgStats,stat); }
	  }
	  if (xParity == 1) {
	    if (j == 3)      { C->data.F64[y][x] = psStatsGetValue(bgStats,stat); }
	    else if (j == 2) { D->data.F64[y][x] = psStatsGetValue(bgStats,stat); }
	  }
	  if (yParity == 1) {
	    if (j == 0)      { B->data.F64[y][x] = psStatsGetValue(bgStats,stat); }
	    else if (j == 1) { A->data.F64[y][x] = psStatsGetValue(bgStats,stat); }
	  }
	  if (yParity == -1) {
	    if (j == 1)      { B->data.F64[y][x] = psStatsGetValue(bgStats,stat); }
	    else if (j == 0) { A->data.F64[y][x] = psStatsGetValue(bgStats,stat); }
	  }
	  
	}
#if (0)	
	for (int u = 0; u < subset->numCols; u++) {
	  for (int v = 0; v < subset->numRows; v++) {
	    psTrace("psModules.detrend.cont",10,"BOX: %d %d (%d %d) (%d %d) %f %d",
		    i,j,x,y,u,v,subset->data.F32[v][u],0);
	  }
	}
#endif	
	psFree(subset);
	
      }
      psTrace("psModules.detrend.cont",5, "OTA: %d (%d %d) (%d %d) A: %f B: %f C: %f D: %f",
	      i,x,y,xParity,yParity,
	      A->data.F64[y][x],B->data.F64[y][x],C->data.F64[y][x],D->data.F64[y][x]);		
    }
    psFree(bgStats);
    psFree(rng);
    
    // We've now allocated all the edge values, so we can now minimize the offsets.
    // This involves solving the equation A x = b, where
    // A is the (64x64 for GPC1) matrix containing the edges that match for each cell
    // x is the solution vector
    // b is the combination of offsets across each cell boundary for each cell.
    // Below "XX" is used as the matrix A, and "solution" is used as both b and x
    //   (due to the way psMatrixLUSolve operates).
    psVector *solution = psVectorAlloc(64,PS_TYPE_F64);
    psImage  *XX       = psImageAlloc(64,64,PS_TYPE_F64);
    psVectorInit(solution,0.0);
    psImageInit(XX,0.0);


    for (int i = 0; i < numChips; i++) {
      // Accumulate all the possible edge differences we can for this cell.
      // As we do so, make a note of the correlations by incrementing the element of the matrix.
      pmChip *chip = out->fpa->chips->data[i];
      const char *chipName = psMetadataLookupStr(NULL,chip->concepts, "CHIP.NAME");
      int x = chipName[2] - '0';
      int y = chipName[3] - '0';

      int j;
      int k = 8 * x + y;
      double critical_value = 0.0;

      if (x + 1 < 8) {  // We have a neighbor adjacent in the +x direction
	j = 8 * (x + 1) + y; // Determine that neighbor's index
	
	if (fabs(C->data.F64[y][x]) > fabs(D->data.F64[y][x+1])) {
	  critical_value = 2.0 * fabs(D->data.F64[y][x+1]);
	}
	else {
	  critical_value = 2.0 * fabs(C->data.F64[y][x]);
	}
	if (critical_value < 25) { critical_value = 25; }
	psTrace("psModules.detrend.cont",5,"CmD %d %d %d %d %g %g %g", // diagnostic
		i,x,y,j,
		C->data.F64[y][x],
		D->data.F64[y][x+1],
		critical_value
		);
	if (// If there are no errors with the neighbor,
	    (isfinite(C->data.F64[y][x]))&&(isfinite(D->data.F64[y][x+1]))&&     // and all edges have valid values,
	    (fabs(C->data.F64[y][x] - D->data.F64[y][x+1]) < critical_value)     // and there are no large discontinuities,
	    ) {    
	  solution->data.F64[k] += C->data.F64[y][x] - D->data.F64[y][x+1];     // Take the difference
	  XX->data.F64[k][k] += 1;                                              // increment our relation with ourself
	  XX->data.F64[k][j] += -1;                                             // decrement our relation with the neighbor
	}
      }
      if (x - 1 > -1) { // etc.
	j = 8 * (x - 1) + y;
	if (fabs(C->data.F64[y][x-1]) > fabs(D->data.F64[y][x])) {
	  critical_value = 2.0 * fabs(D->data.F64[y][x]);
	}
	else {
	  critical_value = 2.0 * fabs(C->data.F64[y][x-1]);
	}
	if (critical_value < 25) { critical_value = 25; }
	psTrace("psModules.detrend.cont",5,"DmC %d %d %d %d %g %g %g",
		i,x,y,j,
		D->data.F64[y][x],
		C->data.F64[y][x-1],
		critical_value
		);

	if (
	    (isfinite(D->data.F64[y][x]))&&(isfinite(C->data.F64[y][x-1]))&&
	    (fabs(D->data.F64[y][x] - C->data.F64[y][x-1]) < critical_value)
	    ) {
	  solution->data.F64[k] += D->data.F64[y][x] - C->data.F64[y][x-1];
	  XX->data.F64[k][k] += 1;
	  XX->data.F64[k][j] += -1;
	}
      }
      if (y + 1 < 8) {
	j = 8 * x + (y + 1);
	psTrace("psModules.detrend.cont",5,"AmB %d %d %d %d %g %g",
		i,x,y,j,
		A->data.F64[y][x],
		B->data.F64[y+1][x]
		);
	if (fabs(A->data.F64[y][x]) > fabs(B->data.F64[y+1][x])) {
	  critical_value = 2.0 * fabs(B->data.F64[y+1][x]);
	}
	else {
	  critical_value = 2.0 * fabs(A->data.F64[y][x]);
	}
	if (critical_value < 25) { critical_value = 25; }
	if (
	    (isfinite(A->data.F64[y][x]))&&(isfinite(B->data.F64[y+1][x]))&&
	    (fabs(A->data.F64[y][x] - B->data.F64[y+1][x]) < critical_value)
	    ) {
	  solution->data.F64[k] += A->data.F64[y][x] - B->data.F64[y+1][x];
	  XX->data.F64[k][k] += 1;
	  XX->data.F64[k][j] += -1;
	}
      }
      if (y - 1 > -1) {
	j = 8 * x +  (y - 1);
	psTrace("psModules.detrend.cont",5,"BmA %d %d %d %d %g %g",
		i,x,y,j,
		B->data.F64[y][x],
		A->data.F64[y-1][x]
		);
	if (fabs(A->data.F64[y-1][x]) > fabs(B->data.F64[y][x])) {
	  critical_value = 2.0 * fabs(B->data.F64[y][x]);
	}
	else {
	  critical_value = 2.0 * fabs(A->data.F64[y-1][x]);
	}
	if (critical_value < 25) { critical_value = 25; }
	if (
	    (isfinite(B->data.F64[y][x]))&&(isfinite(A->data.F64[y-1][x]))&&
	    (fabs(B->data.F64[y][x] - A->data.F64[y-1][x]) < critical_value)
	    ) {
	  solution->data.F64[k] += B->data.F64[y][x] - A->data.F64[y-1][x];
	  XX->data.F64[k][k] += 1;
	  XX->data.F64[k][j] += -1;
	}
      }
    }
    double max_XX = 0;
#if (PS_TRACE_ON)
    double solution_V = 0;
    int i_peak = -1;
#endif
    for (int i = 0; i < numChips + 4; i++) { // If any cells have no value of themself, set the matrix to 1.0.
      if (XX->data.F64[i][i] == 0.0) {
	XX->data.F64[i][i] = 1.0;
      }
      if (XX->data.F64[i][i] > max_XX) {
	max_XX = XX->data.F64[i][i];
#if (PS_TRACE_ON)
	solution_V = solution->data.F64[i];
	i_peak = i;
#endif
      }
    }
    psTrace("psModules.detrend.cont",5,"fixed point: %d %g\n",
	    i_peak,solution_V);
    
#if (1)
    for (int i = 0; i < numChips + 4; i++) { // print matrix A
      psTrace("psModules.detrend.cont",5,"A: %3d % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f % 2.0f",
	      i,
	      XX->data.F64[i][0],	      XX->data.F64[i][1],	      XX->data.F64[i][2],	      XX->data.F64[i][3],
	      XX->data.F64[i][4],	      XX->data.F64[i][5],	      XX->data.F64[i][6],	      XX->data.F64[i][7],
	      XX->data.F64[i][8],	      XX->data.F64[i][9],	      XX->data.F64[i][10],	      XX->data.F64[i][11],
	      XX->data.F64[i][12],	      XX->data.F64[i][13],	      XX->data.F64[i][14],	      XX->data.F64[i][15],
	      XX->data.F64[i][16],	      XX->data.F64[i][17],	      XX->data.F64[i][18],	      XX->data.F64[i][19],
	      XX->data.F64[i][20],	      XX->data.F64[i][21],	      XX->data.F64[i][22],	      XX->data.F64[i][23],
	      XX->data.F64[i][24],	      XX->data.F64[i][25],	      XX->data.F64[i][26],	      XX->data.F64[i][27],
	      XX->data.F64[i][28],	      XX->data.F64[i][29],	      XX->data.F64[i][30],	      XX->data.F64[i][31],
	      XX->data.F64[i][32],	      XX->data.F64[i][33],	      XX->data.F64[i][34],	      XX->data.F64[i][35],
	      XX->data.F64[i][36],	      XX->data.F64[i][37],	      XX->data.F64[i][38],	      XX->data.F64[i][39],
	      XX->data.F64[i][40],	      XX->data.F64[i][41],	      XX->data.F64[i][42],	      XX->data.F64[i][43],
	      XX->data.F64[i][44],	      XX->data.F64[i][45],	      XX->data.F64[i][46],	      XX->data.F64[i][47],
	      XX->data.F64[i][48],	      XX->data.F64[i][49],	      XX->data.F64[i][50],	      XX->data.F64[i][51],
	      XX->data.F64[i][52],	      XX->data.F64[i][53],	      XX->data.F64[i][54],	      XX->data.F64[i][55],
	      XX->data.F64[i][56],	      XX->data.F64[i][57],	      XX->data.F64[i][58],	      XX->data.F64[i][59],
	      XX->data.F64[i][60],	      XX->data.F64[i][61],	      XX->data.F64[i][62],	      XX->data.F64[i][63]
	      );
    }

    for (int i = 0; i < numChips + 4; i++) { // print vector b
      psTrace("psModules.detrend.cont",5,"b: %d %f",
	      i,
	      solution->data.F64[i]
	      );
    }
#endif    
    
    // Solve the Ax=b equation
    //    psMatrixLUSolve(XX,solution);
    psMatrixGJSolve(XX,solution);
#if (1)
    for (int i = 0; i < numChips + 4; i++) { // print vector b
      psTrace("psModules.detrend.cont",5,"x: %d %f",
	      i,
	      solution->data.F64[i]
	      );
    }
#endif
    
    // Cleanup
    psFree(XX);
    psFree(A);
    psFree(B);
    psFree(C);
    psFree(D);

    // Correct cells based on the offsets calculated, and store the result in the analysis metadata.
    for (int i = 0; i < numChips; i++) {
	pmChip *chip = out->fpa->chips->data[i];
        pmCell *cell = chip->cells->data[0]; // Cell of interest
	if ((cell->readouts->n != 1)||(!chip->data_exists))  {
	  continue;
	}
        pmReadout *ro = cell->readouts->data[0]; // Readout of interest
	const char *chipName = psMetadataLookupStr(NULL,chip->concepts, "CHIP.NAME");
	int x = chipName[2] - '0';
	int y = chipName[3] - '0';
	int k = 8 * x + y;
        float correction = solution->data.F64[k];
        psLogMsg("psModules.detrend", PS_LOG_DETAIL, "Correcting background of chip %s by %f",
                 chipName, correction);
        psBinaryOp(ro->image, ro->image, "-", psScalarAlloc(correction, PS_TYPE_F32));
        psMetadataAddF32(ro->analysis, PS_LIST_TAIL, PM_PATTERN_CELL_CORRECTION, PS_META_REPLACE,
                         "Pattern chip correction solution", correction);
    }

    psFree(solution);
    psFree(meanMask);

    return true;
}

