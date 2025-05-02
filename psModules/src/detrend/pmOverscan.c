#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <pslib.h>

#include "pmHDU.h"
#include "pmFPA.h"
#include "pmHDUUtils.h"
#include "pmFPALevel.h"
#include "pmFPAview.h"
#include "pmFPACalibration.h"

#include "pmOverscan.h"

#define SMOOTH_NSIGMA 4.0               // Number of Gaussian sigma the smoothing kernel extends

static void pmOverscanOptionsFree(pmOverscanOptions *options)
{
    psFree(options->stat);
    psFree(options->poly);
    psFree(options->spline);
}

pmOverscanOptions *pmOverscanOptionsAlloc(bool single, pmFit fitType, unsigned int order, psStats *stat,
                                          int boxcar, float gauss)
{
    pmOverscanOptions *opts = psAlloc(sizeof(pmOverscanOptions));
    psMemSetDeallocator(opts, (psFreeFunc)pmOverscanOptionsFree);

    // Inputs
    opts->single = single;
    opts->constant = false;
    opts->fitType = fitType;
    opts->order = order;
    opts->stat = psMemIncrRefCounter(stat);

    opts->minValid = 0.0; // default value if not defined
    opts->maxValid = (float) 0x10000; // default value if not defined
    opts->maskVal = 0x0001; // default value if not defined

    // Smoothing
    opts->boxcar = boxcar;
    opts->gauss = gauss;

    // Outputs
    opts->poly = NULL;
    opts->spline = NULL;

    return opts;
}

// Produce an overscan vector from an array of pixels
psVector *pmOverscanVector(float *chi2, // chi^2 from fit
			   pmOverscanOptions *overscanOpts, // Overscan options
			   const psArray *pixels, // Array of vectors containing the pixel values
			   psStats *myStats // Statistic to use in reducing the overscan
    )
{
    assert(overscanOpts);
    assert(pixels);
    assert(myStats);

    psStatsOptions statistic = psStatsSingleOption(myStats->options); // Statistic to use
    assert(statistic != 0);

    // Reduce the overscans
    psVector *reduced = psVectorAlloc(pixels->n, PS_TYPE_F32); // Overscan for each row
    psVector *ordinate = psVectorAlloc(pixels->n, PS_TYPE_F32); // Ordinate
    psVector *mask = psVectorAlloc(pixels->n, PS_TYPE_VECTOR_MASK); // Mask for fitting

    for (int i = 0; i < pixels->n; i++) {
        psVector *values = pixels->data[i]; // Vector with overscan values
        if (values->n > 0) {
            mask->data.PS_TYPE_VECTOR_MASK_DATA[i] = 0;
            ordinate->data.F32[i] = 2.0*(float)i/(float)pixels->n - 1.0; // Scale to [-1,1]
            if (!psVectorStats(myStats, values, NULL, NULL, 0)) {
		psError(PS_ERR_UNKNOWN, false, "failure to measure stats");
		return false;
	    }
            reduced->data.F32[i] = psStatsGetValue(myStats, statistic);
        } else if (overscanOpts->fitType == PM_FIT_NONE) {
            psError(PS_ERR_UNKNOWN, true, "The overscan is not supplied for all points on the "
                    "image, and no fit is requested.\n");
            return NULL;
        } else {
            // We'll fit this one out
            mask->data.PS_TYPE_VECTOR_MASK_DATA[i] = 1;
        }
    }

    // Smooth the reduced vector
    if (overscanOpts->boxcar > 0) {
        psVector *smoothed = psVectorBoxcar(NULL, reduced, overscanOpts->boxcar); // Smoothed vector
        psFree(reduced);
        reduced = smoothed;
    }
    if (isfinite(overscanOpts->gauss) && overscanOpts->gauss > 0) {
        if (overscanOpts->boxcar > 0) {
            psWarning("Gaussian smoothing the boxcar smoothed overscan --- you asked for it.");
        }
        psVector *smoothed = psVectorSmooth(NULL, reduced, overscanOpts->gauss, SMOOTH_NSIGMA);
        psFree(reduced);
        reduced = smoothed;
    }

    // Fit the overscan, if required
    psVector *fitted = NULL;                   // Fitted overscan values
    switch (overscanOpts->fitType) {
      case PM_FIT_NONE:
        // No fitting --- that's easy.
        fitted = psMemIncrRefCounter(reduced);
        break;
      case PM_FIT_POLY_ORD:
        if (overscanOpts->poly && (overscanOpts->poly->nX != overscanOpts->order ||
                                   overscanOpts->poly->type != PS_POLYNOMIAL_ORD)) {
            psFree(overscanOpts->poly);
            overscanOpts->poly = NULL;
        }
        if (! overscanOpts->poly) {
            overscanOpts->poly = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, overscanOpts->order);
        }
        psVectorFitPolynomial1D(overscanOpts->poly, mask, 1, reduced, NULL, ordinate);
        fitted = psPolynomial1DEvalVector(overscanOpts->poly, ordinate);
        break;
      case PM_FIT_POLY_CHEBY:
        if (overscanOpts->poly && (overscanOpts->poly->nX != overscanOpts->order ||
                                   overscanOpts->poly->type != PS_POLYNOMIAL_CHEB)) {
            psFree(overscanOpts->poly);
            overscanOpts->poly = NULL;
        }
        if (! overscanOpts->poly) {
            overscanOpts->poly = psPolynomial1DAlloc(PS_POLYNOMIAL_CHEB, overscanOpts->order);
        }
        psVectorFitPolynomial1D(overscanOpts->poly, mask, 1, reduced, NULL, ordinate);
        fitted = psPolynomial1DEvalVector(overscanOpts->poly, ordinate);
        break;
      case PM_FIT_SPLINE:

        // XXX I don't think psSpline1D is up to scratch yet --- it has no mask, and it assumes
	// a knot for every input point.  it needs an argument like 'number of knots' for the
	// output spline.  EAM: still true 2023.01.22

        // overscanOpts->spline = psVectorFitSpline1D(reduced, ordinate);
        // fitted = psSpline1DEvalVector(overscanOpts->spline, ordinate);
        psError(PS_ERR_UNKNOWN, true, "Spline overscan fitting is broken\n");
        break;
      default:
        psError(PS_ERR_UNKNOWN, true, "Unknown value for the fitting type: %d\n", overscanOpts->fitType);
        return NULL;
        break;
    }

    if (chi2) {
        *chi2 = 0.0;                    // chi^2 (sort of)
        for (int i = 0; i < reduced->n; i++) {
            *chi2 += PS_SQR(fitted->data.F32[i] - reduced->data.F32[i]);
        }
    }

    psFree(reduced);
    psFree(ordinate);
    psFree(mask);

    return fitted;
}

bool pmOverscanUpdateHeader (pmHDU *hdu, pmOverscanOptions *overscanOpts, float chi2) {

    psString comment = NULL;    // Comment to add

    switch (overscanOpts->fitType) {
      case PM_FIT_POLY_ORD:
      case PM_FIT_POLY_CHEBY: {
	  psStringAppend(&comment, "Overscan fit (chi2: %.2f): ", chi2);
	  psPolynomial1D *poly = overscanOpts->poly; // The polynomial
	  for (int i = 0; i < poly->nX; i++) {
	      psStringAppend(&comment, "%.1f ", poly->coeff[i]);
	  }
	  psMetadataAddStr(hdu->header, PS_LIST_TAIL, "HISTORY", PS_META_DUPLICATE_OK, comment, "");
	  psFree(comment);
	  comment = NULL;

	  // write metadata header value
	  psMetadataAddF32(hdu->header, PS_LIST_TAIL, "OVER_VAL", PS_META_REPLACE,
			   "Overscan value", poly->coeff[0]);
	  psMetadataAddF32(hdu->header, PS_LIST_TAIL, "OVER_SIG", PS_META_REPLACE,
			   "Overscan stdev", poly->coeffErr[0]);
	  break;
      }
      case PM_FIT_SPLINE: {
	/*
	  psSpline1D *spline = overscanOpts->spline; // The spline
	  for (int i = 0; i < spline->n; i++) {
	      psStringAppend(&comment, "Overscan fit (chi2: %.2f) %d:", chi2, i);
	      psPolynomial1D *poly = spline->spline[i]; // i-th polynomial
	      for (int j = 0; j < poly->nX; j++) {
		  psStringAppend(&comment, "%.1f ", poly->coeff[i]);
	      }
	      psMetadataAddStr(hdu->header, PS_LIST_TAIL, "HISTORY", PS_META_DUPLICATE_OK,
			       comment, "");
	      psFree(comment);
	      comment = NULL;
	  }
	*/
	  // write metadata header value
	  psMetadataAddF32(hdu->header, PS_LIST_TAIL, "OVER_VAL", PS_META_REPLACE,
			   "Overscan value", NAN);
	  psMetadataAddF32(hdu->header, PS_LIST_TAIL, "OVER_SIG", PS_META_REPLACE,
			   "Overscan stdev", NAN);
	  break;
      }
      case PM_FIT_NONE:
	break;
      default:
	psAbort("Should never get here!!!\n");
    }
    return true;
}

bool pmOverscanSubtract (pmReadout *input, pmOverscanOptions *overscanOpts) {

    assert (input);

    if (overscanOpts == NULL) return true; // no overscan subtraction requested

    pmHDU *hdu = pmHDUFromReadout(input);  // HDU of interest
    psImage *image = input->image;

    // check for 'soft bias' (simple, fixed offset to be subtracted)
    if (overscanOpts->constant) {
	// write metadata header value
	psMetadataAddF32(hdu->header, PS_LIST_TAIL, "OVER_VAL", PS_META_REPLACE, "Overscan value",
			 overscanOpts->value);
	psMetadataAddF32(hdu->header, PS_LIST_TAIL, "OVER_SIG", PS_META_REPLACE, "Overscan stdev", NAN);

	// NOTE psBinaryOp frees arg2 if it is a scalar
	(void)psBinaryOp(input->image, input->image, "-", psScalarAlloc((float)overscanOpts->value, PS_TYPE_F32));

	return true;
    }

    // we are performing a statitical analysis of the overscan region

    // Check for an unallowable pmFit.
    if (overscanOpts->fitType != PM_FIT_NONE && overscanOpts->fitType != PM_FIT_POLY_ORD &&
	overscanOpts->fitType != PM_FIT_POLY_CHEBY && overscanOpts->fitType != PM_FIT_SPLINE) {
	psError(PS_ERR_UNKNOWN, true, "Invalid fit type (%d).  Returning original image.\n",
		overscanOpts->fitType);
	return false;
    }

    psList *overscans = input->bias; // List of the overscan images

    psStatsOptions statistic = psStatsSingleOption(overscanOpts->stat->options); // Statistic to use
    if (statistic == 0) {
	psError(PS_ERR_BAD_PARAMETER_VALUE, false, "Multiple or no statistics options set: %p\n",
		overscanOpts->stat);
	return false;
    }
    psStats *stats = psStatsAlloc(statistic); // A new psStats, to avoid clobbering original

    psString comment = NULL;    // Comment to add
    psStringAppend(&comment, "Subtracting overscan (stat %x; type %x; order %d)",
		   statistic, overscanOpts->fitType, overscanOpts->order);
    psMetadataAddStr(hdu->header, PS_LIST_TAIL, "HISTORY", PS_META_DUPLICATE_OK,
		     comment, "");
    psTrace ("psModules.detrend", 4, "%s\n", comment);
    psFree(comment);

    // Reduce all overscan pixels to a single value
    if (overscanOpts->single) {
	psVector *pixels = psVectorAlloc(0, PS_TYPE_F32);
	psListIterator *iter = psListIteratorAlloc(overscans, PS_LIST_HEAD, false); // Iterator
	psImage *overscan = NULL;   // Overscan image from iterator
	while ((overscan = psListGetAndIncrement(iter))) {
	    int index = pixels->n;  // Index
	    pixels = psVectorRealloc(pixels, pixels->n + overscan->numRows * overscan->numCols);
	    pixels->n += overscan->numRows * overscan->numCols;
	    for (int i = 0; i < overscan->numRows; i++) {
		memcpy(&pixels->data.F32[index], overscan->data.F32[i],
		       overscan->numCols * sizeof(psF32));
		index += overscan->numCols;
	    }
	}
	psFree(iter);

	if (!psVectorStats(stats, pixels, NULL, NULL, 0)) {
	    psError(PS_ERR_UNKNOWN, false, "failure to measure stats");
	    return false;
	}
	psFree(pixels);
	double reduced = psStatsGetValue(stats, statistic); // Result of statistics

	psString comment = NULL;    // Comment to add
	psStringAppend(&comment, "Overscan value: %f", reduced);
	psMetadataAddStr(hdu->header, PS_LIST_TAIL, "HISTORY", PS_META_DUPLICATE_OK, comment, "");
	psFree(comment);

	// write metadata header value
	psMetadataAddF32(hdu->header, PS_LIST_TAIL, "OVER_VAL", PS_META_REPLACE, "Overscan value",
			 reduced);
	psMetadataAddF32(hdu->header, PS_LIST_TAIL, "OVER_SIG", PS_META_REPLACE, "Overscan stdev", NAN);

	psScalar *reducedScalar = psScalarAlloc(reduced, PS_TYPE_F32);
	psBinaryOp (image, image, "-", psMemIncrRefCounter(reducedScalar)); // NOTE: psBinaryOp frees arg2 if it a scalar, so we need to bump to re-use

	// subtract the measured value from each overscan region as well
	iter = psListIteratorAlloc(overscans, PS_LIST_HEAD, false); // Iterator
	overscan = NULL;   // Overscan image from iterator
	while ((overscan = psListGetAndIncrement(iter))) {
	  psBinaryOp(overscan, overscan, "-", psMemIncrRefCounter(reducedScalar)); // NOTE: psBinaryOp frees arg2 if it a scalar, so we need to bump to re-use
	}
	psFree(iter);
	psFree(reducedScalar);

	// EAM 2022.03.29 : if the calculated overscan value is below the threshold,
	// declare the readout dead and mask

	if ((reduced < overscanOpts->minValid) || (reduced > overscanOpts->maxValid)) {
	    fprintf (stderr, "bad overscan (1) %f, masking readout\n", reduced);
	    psImage *mask = input->mask;
	    for (int y = 0; y < mask->numRows; y++) {
		for (int x = 0; x < mask->numCols; x++) {
		    mask->data.PS_TYPE_IMAGE_MASK_DATA[y][x] |= overscanOpts->maskVal;
		}
	    }
	}

	psFree(stats);
	return true;
    } 

    bool mdok = false;

    // We are performing a row-by-row overscan subtraction
    int cellreaddir = psMetadataLookupS32(&mdok, input->parent->concepts, "CELL.READDIR"); // Read direction
    if ((cellreaddir != 1) && (cellreaddir != 2)) {
	psError(PS_ERR_UNKNOWN, true, "CELL.READDIR must be 1 (rows) or 2 (cols)\n");
	return false;
    }

    float chi2 = NAN;           // chi^2 from fit

    // adjust operation depending on the read direction : need to re-org pixels for columns
    if (cellreaddir == 1) {
	// The read direction is rows
	psArray *pixels = psArrayAlloc(image->numRows); // Array of vectors containing pixels
	for (int i = 0; i < pixels->n; i++) {
	    pixels->data[i] = psVectorAlloc(0, PS_TYPE_F32);
	}

	// Pull the pixels out into the vectors
	psListIterator *iter = psListIteratorAlloc(overscans, PS_LIST_HEAD, false); // Iterator
	psImage *overscan = NULL; // Overscan image from iterator
	while ((overscan = psListGetAndIncrement(iter))) {
	    // the overscan and image might not be aligned.  pixels->data represents
	    // the image row pixels.
	    int diff = overscan->row0 - image->row0; // Offset between the two regions
	    for (int i = PS_MAX(0,diff); i < PS_MIN(image->numRows, overscan->numRows + diff); i++) {
		int j = i - diff;
		// i is row on image
		// j is row on overscan
		psVector *values = pixels->data[i];
		int index = values->n; // Index in the vector
		values = psVectorRealloc(values, values->n + overscan->numCols);
		values->n += overscan->numCols;
		memcpy(&values->data.F32[index], overscan->data.F32[j],
		       overscan->numCols * PSELEMTYPE_SIZEOF(PS_TYPE_F32));
		index += overscan->numCols;
		pixels->data[i] = values; // Update the pointer in case it's moved
	    }
	}
	psFree(iter);

	// Reduce the overscans
	psVector *reduced = pmOverscanVector(&chi2, overscanOpts, pixels, stats);
	psFree(pixels);
	if (! reduced) {
	    psError(PS_ERR_UNEXPECTED_NULL, false, "Unable to generate overscan vector.\n");
	    psFree(stats);
	    return false;
	}

	// generate stats of overscan vector for header
	{ 
	    psString comment = NULL;    // Comment to add
	    psStats *vectorStats = psStatsAlloc (PS_STAT_SAMPLE_MEAN | PS_STAT_SAMPLE_STDEV);
	    if (!psVectorStats (vectorStats, reduced, NULL, NULL, 0)) {
		psError(PS_ERR_UNKNOWN, false, "failure to measure stats");
		return false;
	    }
	    psStringAppend(&comment, "Mean Overscan value: %f", vectorStats->sampleMean);
	    psMetadataAddStr(hdu->header, PS_LIST_TAIL, "HISTORY", PS_META_DUPLICATE_OK, comment, "");
	    psFree(comment);

	    // write metadata header value
	    psMetadataAddF32(hdu->header, PS_LIST_TAIL, "OVER_VAL", PS_META_REPLACE, "Overscan mean", vectorStats->sampleMean);
	    psMetadataAddF32(hdu->header, PS_LIST_TAIL, "OVER_SIG", PS_META_REPLACE, "Overscan stdev", vectorStats->sampleStdev);

	    // EAM 2022.03.29 : if the calculated overscan value is below the threshold,
	    // declare the readout dead and mask
	  
	    if ((vectorStats->sampleMean < overscanOpts->minValid) || (vectorStats->sampleMean > overscanOpts->maxValid)) {
		fprintf (stderr, "bad overscan (2) %f, masking readout\n", vectorStats->sampleMean);
		psImage *mask = input->mask;
		for (int y = 0; y < mask->numRows; y++) {
		    for (int x = 0; x < mask->numCols; x++) {
			mask->data.PS_TYPE_IMAGE_MASK_DATA[y][x] |= overscanOpts->maskVal;
		    }
		}
	    }

	    psFree (vectorStats);
	}

	// Subtract row by row
	for (int i = 0; i < image->numRows; i++) {
	    for (int j = 0; j < image->numCols; j++) {
		image->data.F32[i][j] -= reduced->data.F32[i];
	    }
	}

	// subtract from the overscan regions
	{
	  psListIterator *iter = psListIteratorAlloc(overscans, PS_LIST_HEAD, false); // Iterator
	  psImage *overscan = NULL; // Overscan image from iterator
	  while ((overscan = psListGetAndIncrement(iter))) {
	    // the overscan and image might not be aligned.
	    int diff = overscan->row0 - image->row0; // Offset between the two regions
	    for (int i = PS_MAX(0,diff); i < PS_MIN(image->numRows, overscan->numRows + diff); i++) {
	      int j = i - diff;
	      // i is row on image
	      // j is row on overscan
	      for (int k = 0; k < overscan->numCols; k++) {
		overscan->data.F32[j][k] -= reduced->data.F32[j];
	      }
	    }
	  }
	  psFree(iter);
	}
	psFree(reduced);
    } 
    if (cellreaddir == 2) {
	// The read direction is columns
	psArray *pixels = psArrayAlloc(image->numCols); // Array of vectors containing pixels
	for (int i = 0; i < pixels->n; i++) {
	    psVector *values = psVectorAlloc(0, PS_TYPE_F32);
	    pixels->data[i] = values;
	}

	// Pull the pixels out into the vectors
	psListIterator *iter = psListIteratorAlloc(overscans, PS_LIST_HEAD, false); // Iterator
	psImage *overscan = NULL; // Overscan image from iterator
	while ((overscan = psListGetAndIncrement(iter))) {
	    // the overscan and image might not be aligned.  pixels->data represents
	    // the image row pixels.
	    int diff = overscan->col0 - image->col0; // Offset between the two regions
	    for (int i = PS_MAX(0,diff); i < PS_MIN(image->numCols, overscan->numCols + diff); i++) {
		int iFixed = i - diff;
		// i is column on image
		// iFixed is column on overscan
		psVector *values = pixels->data[i];
		int index = values->n; // Index in the vector
		values = psVectorRealloc(values, values->n + overscan->numRows);
		for (int j = 0; j < overscan->numRows; j++) {
		    values->data.F32[index++] = overscan->data.F32[j][iFixed];
		}
		values->n += overscan->numRows;
		pixels->data[i] = values; // Update the pointer in case it's moved
	    }
	}
	psFree(iter);

	// Reduce the overscans
	psVector *reduced = pmOverscanVector(&chi2, overscanOpts, pixels, stats);
	psFree(pixels);
	if (! reduced) {
	    psError(PS_ERR_UNEXPECTED_NULL, false, "Unable to generate overscan vector.\n");
	    psFree(stats);
	    return false;
	}

	// generate stats of overscan vector for header
	{ 
	  psString comment = NULL;    // Comment to add
	  psStats *vectorStats = psStatsAlloc (PS_STAT_SAMPLE_MEAN | PS_STAT_SAMPLE_STDEV);
	  if (!psVectorStats (vectorStats, reduced, NULL, NULL, 0)) {
	      psError(PS_ERR_UNKNOWN, false, "failure to measure stats");
	      return false;
	  }
	  psStringAppend(&comment, "Mean Overscan value: %f", vectorStats->sampleMean);
	  psMetadataAddStr(hdu->header, PS_LIST_TAIL, "HISTORY", PS_META_DUPLICATE_OK, comment, "");
	  psFree(comment);

	  // write metadata header value
	  psMetadataAddF32(hdu->header, PS_LIST_TAIL, "OVER_VAL", PS_META_REPLACE, "Overscan mean", vectorStats->sampleMean);
	  psMetadataAddF32(hdu->header, PS_LIST_TAIL, "OVER_SIG", PS_META_REPLACE, "Overscan stdev", vectorStats->sampleStdev);

	  // EAM 2022.03.29 : if the calculated overscan value is below the threshold,
	  // declare the readout dead and mask
	  
	  if ((vectorStats->sampleMean < overscanOpts->minValid) || (vectorStats->sampleMean > overscanOpts->maxValid)) {
	      fprintf (stderr, "bad overscan (3) %f, masking readout\n", vectorStats->sampleMean);
	      psImage *mask = input->mask;
	      for (int y = 0; y < mask->numRows; y++) {
		  for (int x = 0; x < mask->numCols; x++) {
		      mask->data.PS_TYPE_IMAGE_MASK_DATA[y][x] |= overscanOpts->maskVal;
		  }
	      }
	  }
	  psFree (vectorStats);
	}

	// Subtract column by column 
	for (int j = 0; j < image->numRows; j++) {
	  for (int i = 0; i < image->numCols; i++) {
		image->data.F32[j][i] -= reduced->data.F32[i];
	    }
	}

	// subtract from the overscan regions
	{
	  psListIterator *iter = psListIteratorAlloc(overscans, PS_LIST_HEAD, false); // Iterator
	  psImage *overscan = NULL; // Overscan image from iterator
	  while ((overscan = psListGetAndIncrement(iter))) {
	    // the overscan and image might not be aligned.
	    int diff = overscan->col0 - image->col0; // Offset between the two regions
	    for (int i = PS_MAX(0,diff); i < PS_MIN(image->numCols, overscan->numCols + diff); i++) {
	      int j = i - diff;
	      // i is col on image
	      // j is col on overscan
	      for (int k = 0; i < overscan->numRows; k++) {
		overscan->data.F32[k][j] -= reduced->data.F32[j];
	      }
	    }
	  }
	  psFree(iter);
	}

	psFree(reduced);
    }

    pmOverscanUpdateHeader (hdu, overscanOpts, chi2);
    psFree(stats);

    return true;

} // End of overscan subtraction
    
