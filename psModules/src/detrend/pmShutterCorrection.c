#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <math.h>
#include <strings.h>
#include <pslib.h>

#include "pmHDU.h"
#include "pmFPA.h"
#include "pmHDUUtils.h"
#include "psVectorBracket.h"
#include "pmConceptsAverage.h"
#include "pmReadoutStack.h"
#include "pmDetrendThreads.h"

#include "pmShutterCorrection.h"

/// Measure shutter correction:
///
/// input  : collection of shutter correction exposures (pre-processed)
/// output : a shutter correction image
///
/// The measurement could be performed on any focal-plane unit at a time. for GPC, the obvious scale is to
/// measure the effect on the entire focal plane at once, with a single reference point in the field.  this is
/// a little more complex than just measuring the effect for a single 2D image array.  the reference point and
/// the detailed analysis points need to be defined for the entire hierarchy rather than just as coordinate
/// pairs or regions.  a pmFPAview would be a natural element with which to define these points, but at the
/// moment, the pmFPAview structure defines a band in the CCD, not a coordinate.  An option is to instead
/// specify the reference locations as a pmFPAview coupled with a psRegion, though we need to be careful not
/// to over-specify the pixels (ie, conflict between pmFPAview and psRegion).
///
/// At each point in an image with exposure time T, we measure f(k;T) = F(k;T) / F(0;T) where k is the
/// coordinate of the point of interest, 0 is the reference coordinate, and F(k;T) is the measured number of
/// counts at the point of interest in this image.  given a collection of f(k;T) values, we need to determine
/// the model f(k;T) = A(k) (T + dTk) / (T + dTo) where dTk is the shutter error at the given position, dTo is
/// the shutter error at the reference position, and A(k) is the scaling factor for the given position.
///
/// The process for generating a shutter correction is as follows:
/// - for each image
/// -- measure the reference point counts
/// - for each analysis region:
/// -- measure shutter parameters (dTo, dTk, A):
/// --- for each image:
/// ---- measure counts at the region
/// ---- divide by the reference counts
/// --- linear extrapolation to find f(inf) = A(k)
/// --- linear extrapolation to find f(0) = A(k) dTk / dTo
/// --- linear interpolation to find coordinate where f(dTo) = A (1 + dTk/dTo) / 2
/// --- non-linear fit of T, f(T) to f(k;T) = A(k) (T + dTk) / (T + dTo)
/// - use the collection of dTo values to choose a best value for dTo (median)
/// - for each image pixel
/// -- divide by the reference counts
/// -- generate the vectors T, f(T)
/// -- linear fit of T, f(T) to f(k;T) = A(k) (T + dTk) / (T + dTo) using dTo above
/// -- save dTk, A(k) in output image pixels
/// -- apply dTk, A(k) to measure residual images
/// -- generate residual FITS/JPEG images


#define MEASURE_SAMPLES 4               // Number of samples to make over the image.  This should only be
                                        // changed with great caution, since assumptions on its value are in
                                        // the code (see pmShutterCorrectionDataAlloc).


static void pmShutterCorrectionFree(pmShutterCorrection *pars)
{
    // Nothing to free
    return;
}

pmShutterCorrection *pmShutterCorrectionAlloc(void)
{
    pmShutterCorrection *corr = (pmShutterCorrection*)psAlloc(sizeof(pmShutterCorrection));
    psMemSetDeallocator(corr, (psFreeFunc)pmShutterCorrectionFree);

    corr->scale  = 0.0;
    corr->offset = 0.0;
    corr->offref = 0.0;
    corr->num = 0;
    corr->stdev = NAN;
    corr->valid = true;

    return corr;
}

pmShutterCorrection *pmShutterCorrectionGuess(const psVector *exptime, const psVector *counts)
{
    // NOTE: vectors must be sorted on input.  It is expensive to sort or check this here, but
    // it is easy to arrange by sorting the images before generating these vectors.

    PS_ASSERT_VECTOR_NON_NULL(exptime, NULL);
    PS_ASSERT_VECTOR_NON_NULL(counts, NULL);
    PS_ASSERT_VECTOR_TYPE(exptime, PS_TYPE_F32, NULL);
    PS_ASSERT_VECTOR_TYPE(counts, PS_TYPE_F32, NULL);
    PS_ASSERT_VECTORS_SIZE_EQUAL(exptime, counts, NULL);
    if (exptime->n <= 2) {
        psError(PS_ERR_BAD_PARAMETER_SIZE, true,
                "Require more than 2 exposures to guess shutter correction.\n");
        return NULL;
    }

    long N = exptime->n;                // Number of exposures

    // use interpolation to guess shutter correction parameters given a set of exposures times and normalized
    // counts (divided by the reference counts for each image)

    pmShutterCorrection *corr = pmShutterCorrectionAlloc(); // Shutter correction, to be returned
    psPolynomial1D *line = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, 1); // Straight line, for extrapolation

    // choose the highest exptime point as the guess for the scale:
    // XXX we could examine the top 2 or 3 values and decide if we
    // extended exptime enough or median clip.
    corr->scale = counts->data.F32[N-1];

    // fit a line to the lowest three points and extrapolate to 0.0
    psVector *tmpX = psVectorAlloc(2, PS_TYPE_F32);
    psVector *tmpY = psVectorAlloc(2, PS_TYPE_F32);

    long index;

    // Iterate only
    for (index = 0; !isfinite(exptime->data.F32[index]) && index < N - 1; index++);

    if (index == N - 1) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                "Not enough good values to guess shutter correction.\n");
        goto GUESS_ERROR;
    }
    tmpX->data.F32[0] = exptime->data.F32[index];
    tmpY->data.F32[0] = counts->data.F32[index];

    for (index++;
            (!isfinite(exptime->data.F32[index]) || exptime->data.F32[index] == exptime->data.F32[0]) &&
            index < N; index++)
        ; // Iterate only
    if (index == N) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                "Exposure times are all identical --- cannot guess shutter correction.\n");
        goto GUESS_ERROR;
    }
    tmpY->data.F32[1] = counts->data.F32[index];
    tmpX->data.F32[1] = exptime->data.F32[index];

    // fit a line and extrapolate the fit to 0.0
    if (!psVectorFitPolynomial1D(line, NULL, 0, tmpY, NULL, tmpX)) {
        psError(PS_ERR_UNEXPECTED_NULL, false, "Unable to fit for the time offset.\n");
        goto GUESS_ERROR;
    }
    float ratio = psPolynomial1DEval(line, 0.0) / corr->scale;

    // XXX we need a sanity check:
    // if the mean value of the three points is higher than corr->scale,
    // then the slope should be negative.
    // if the mean value of the three points is lower than corr->scale,
    // then the slope should be positive.

    // find two points bracketing the value counts = A (1 + dTk/dTo) / 2 = corr->scale (1 + ratio) / 2
    float value = corr->scale * (1 + ratio) / 2.0;

    int Np;                             // Index of the value above (positive side)
    if (ratio < 1.0) {
        Np = psVectorBracket(counts, value, true);
    } else {
        Np = psVectorBracketDescend(counts, value, true);
    }
    int Nm = (Np == 0) ? 1 : Np - 1;    // Index of the value below (negative side)

    tmpX->data.F32[0] = counts->data.F32[Nm];
    tmpX->data.F32[1] = counts->data.F32[Np];
    tmpY->data.F32[0] = exptime->data.F32[Nm];
    tmpY->data.F32[1] = exptime->data.F32[Np];

    // fit a line and extrapolate the fit to counts = A (1 + dTk/dTo) : exptime = dTo
    if (!psVectorFitPolynomial1D (line, NULL, 0, tmpY, NULL, tmpX)) {
        psError(PS_ERR_UNEXPECTED_NULL, false, "Unable to fit for the reference offset.\n");
        goto GUESS_ERROR;
    }
    corr->offref = psPolynomial1DEval(line, value);
    corr->offset = ratio * corr->offref;

    psFree(line);
    psFree(tmpX);
    psFree(tmpY);

    return corr;

GUESS_ERROR:
    psFree(tmpX);
    psFree(tmpY);
    psFree(line);
    psFree(corr);
    return NULL;
}

// linear fit to the counts and exptime, given a value for offref
pmShutterCorrection *pmShutterCorrectionLinFit(const psVector *exptime, const psVector *counts,
                                               const psVector *cntError, const psVector *mask, float offref,
                                               int nIter, float rej)
{
    PS_ASSERT_VECTOR_NON_NULL(exptime, NULL);
    PS_ASSERT_VECTOR_NON_NULL(counts, NULL);
    PS_ASSERT_VECTOR_TYPE(exptime, PS_TYPE_F32, NULL);
    PS_ASSERT_VECTOR_TYPE(counts, PS_TYPE_F32, NULL);
    PS_ASSERT_VECTORS_SIZE_EQUAL(exptime, counts, NULL);
    if (exptime->n <= 2) {
        psError(PS_ERR_BAD_PARAMETER_SIZE, true,
                "Require more than 2 exposures to guess shutter correction.\n");
        return NULL;
    }
    if (cntError) {
        PS_ASSERT_VECTOR_TYPE(cntError, PS_TYPE_F32, NULL);
        PS_ASSERT_VECTORS_SIZE_EQUAL(counts, cntError, NULL);
    }
    PS_ASSERT_FLOAT_LARGER_THAN(offref, 0.0, NULL);

    // this step is identical for all pixels: do it once and save?
    psVector *x = psVectorAlloc(exptime->n, PS_TYPE_F32);
    psVector *y = psVectorAlloc(exptime->n, PS_TYPE_F32);

    for (long i = 0; i < exptime->n; i++) {
        // Should be safe (if expensive) to stick NaNs in --- the fitter deals with them
        float value = 1.0 / (exptime->data.F32[i] + offref);
        x->data.F32[i] = exptime->data.F32[i] * value;
        y->data.F32[i] = value;
    }

    psPolynomial2D *line = psPolynomial2DAlloc (PS_POLYNOMIAL_ORD, 1, 1);

    // mask out the terms we will not fit
    line->coeffMask[0][0] = PS_POLY_MASK_SET;
    line->coeffMask[1][1] = PS_POLY_MASK_SET;
    line->coeff[0][0] = 0;
    line->coeff[1][1] = 0;

    // the stats structure determines how the clipping statistic is measured
    // too few points to use the robust analysis method
    psStats *stats = psStatsAlloc (PS_STAT_SAMPLE_MEDIAN | PS_STAT_SAMPLE_STDEV);
    stats->clipSigma = rej;
    stats->clipIter = nIter;

    if (!psVectorClipFitPolynomial2D(line, stats, mask, 0xff, counts, cntError, x, y)) {
        psError(PS_ERR_UNEXPECTED_NULL, false, "Unable to fit shutter correction.\n");
        psFree(stats);
        psFree(x);
        psFree(y);
        psFree(line);
        return NULL;
    }

    pmShutterCorrection *corr = pmShutterCorrectionAlloc();
    corr->offref = offref;
    corr->scale  = line->coeff[1][0];
    corr->offset = line->coeff[0][1] / line->coeff[1][0];
    corr->num = stats->clippedNvalues;
    corr->stdev = stats->clippedStdev;

    psFree(stats);
    psFree(x);
    psFree(y);
    psFree(line);

    return corr;
}

static psF32 pmShutterCorrectionModel(psVector *deriv, const psVector *params, const psVector *x)
{
    // This is in a tight loop, so we won't assert here.

    psF32 A = params->data.F32[0];
    psF32 p = x->data.F32[0] + params->data.F32[1];
    psF32 q = 1.0 / (x->data.F32[0] + params->data.F32[2]);
    psF32 f = A * p * q;

    if (deriv) {
        deriv->data.F32[0] = p * q;
        deriv->data.F32[1] = A * q;
        deriv->data.F32[2] = - f * q;
    }
    return f;
}

// non-linear fit to the counts and exptime, given a guess for the three parameters
pmShutterCorrection *pmShutterCorrectionFullFit(const psVector *exptime, const psVector *counts,
                                                const psVector *cntError, const pmShutterCorrection *guess)
{
    PS_ASSERT_VECTOR_NON_NULL(exptime, NULL);
    PS_ASSERT_VECTOR_NON_NULL(counts, NULL);
    PS_ASSERT_VECTOR_TYPE(exptime, PS_TYPE_F32, NULL);
    PS_ASSERT_VECTOR_TYPE(counts, PS_TYPE_F32, NULL);
    PS_ASSERT_VECTORS_SIZE_EQUAL(exptime, counts, NULL);
    if (exptime->n <= 2) {
        psError(PS_ERR_BAD_PARAMETER_SIZE, true,
                "Require more than 2 exposures to guess shutter correction.\n");
        return NULL;
    }
    if (cntError) {
        PS_ASSERT_VECTOR_TYPE(cntError, PS_TYPE_F32, NULL);
        PS_ASSERT_VECTORS_SIZE_EQUAL(counts, cntError, NULL);
    }
    PS_ASSERT_PTR_NON_NULL(guess, NULL);

    psMinimization *minInfo = psMinimizationAlloc(15, 0.1, 1.0); // Minimization information

    psVector *params = psVectorAlloc (3, PS_TYPE_F32); // Fitting parameters
    params->data.F32[0] = guess->scale;
    params->data.F32[1] = guess->offset;
    params->data.F32[2] = guess->offref;

    // XXX for the moment, don't set any constraints
    // psMinConstraint *constraint = psMinConstraintAlloc();
    // constrain->checkLimits = pmShutterParamLimits;
    // constrain->paramMask   = NULL;
    psMinConstraint *constraint = NULL;   // Constraints on the minimization

    // XXX ignore covariance matrix for the moment
    // psImage *covar = psImageAlloc (params->n, params->n, PS_TYPE_F64);
    psImage *covar = NULL;              // Covariance matrix

    // construct the coordinate and value entries (y is counts)
    psArray *x = psArrayAlloc(exptime->n); // Coordinates

    for (long i = 0; i < exptime->n; i++) {
        psVector *coord = psVectorAlloc(1, PS_TYPE_F32);
        coord->data.F32[0] = exptime->data.F32[i];
        x->data[i] = coord;
    }

    if (!psMinimizeLMChi2(minInfo, covar, params, constraint, x, counts, cntError, pmShutterCorrectionModel)) {
        psError(PS_ERR_UNEXPECTED_NULL, false, "Unable to fit for shutter correction.\n");
        psFree(x);
        psFree(minInfo);
        psFree(params);
        return NULL;
    }

    pmShutterCorrection *corr = pmShutterCorrectionAlloc(); // Shutter correction
    corr->scale  = params->data.F32[0];
    corr->offset = params->data.F32[1];
    corr->offref = params->data.F32[2];

    // apply the correction and measure the residual scatter
    psVector *resid = psVectorAlloc (exptime->n, PS_TYPE_F32);
    for (int i = 0; i < exptime->n; i++) {
        float fitCounts = corr->scale * (exptime->data.F32[i] + corr->offset) / (exptime->data.F32[i] + corr->offref);
        resid->data.F32[i] = counts->data.F32[i] - fitCounts;
    }

    psStats *rawStats = psStatsAlloc (PS_STAT_SAMPLE_MEDIAN | PS_STAT_SAMPLE_STDEV);
    psStats *resStats = psStatsAlloc (PS_STAT_SAMPLE_MEDIAN | PS_STAT_SAMPLE_STDEV);
    if (!psVectorStats (rawStats, counts, NULL, NULL, 0)) {
        psError(PS_ERR_UNKNOWN, false, "failure to measure stats");
        return NULL;
    }
    if (!psVectorStats (resStats, resid, NULL, NULL, 0)) {
        psError(PS_ERR_UNKNOWN, false, "failure to measure stats");
        return NULL;
    }

    // XXX temporary hard-wired minimum stdev improvement factor
    psTrace("psModules.detrend", 3, "raw scatter %f vs res scatter %f\n", rawStats->sampleStdev, resStats->sampleStdev);
    if (rawStats->sampleStdev / resStats->sampleStdev < 1.5) corr->valid = false;
    if (isnan(rawStats->sampleStdev) || isnan(resStats->sampleStdev)) corr->valid = false;

    psFree (rawStats);
    psFree (resStats);
    psFree (resid);

    psFree(minInfo);
    psFree(params);
    psFree(x);

    return corr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool pmShutterCorrectionMeasure(pmReadout *output, const psArray *readouts, int size, psStatsOptions meanStat,
                                psStatsOptions stdevStat, int nIter, float rej, psImageMaskType maskVal)
{
    PS_ASSERT_ARRAY_NON_NULL(readouts, NULL);
    PS_ASSERT_ARRAY_NON_EMPTY(readouts, NULL);
    PS_ASSERT_INT_POSITIVE(size, NULL);

    long num = readouts->n;             // Number of readouts
    PS_ASSERT_INT_POSITIVE(nIter, NULL);
    PS_ASSERT_FLOAT_LARGER_THAN(rej, 0.0, NULL);

    psArray *images = psArrayAlloc(num);// Array of images
    psArray *masks = NULL; // Array of masks
    psArray *variances = NULL; // Array of variances
    psVector *exptimes = psVectorAlloc(num, PS_TYPE_F32); // Vector of exposure times

    {
        pmReadout *readout = readouts->data[0]; // Representative readout
        if (readout->mask)
        {
            masks = psArrayAlloc(num);
        }
        if (readout->variance)
        {
            variances = psArrayAlloc(num);
        }
    }

    // Check input sizes, generate first-pass statistics
    psVector *refs = psVectorAlloc(num, PS_TYPE_F32); // Reference measurements
    psVectorInit(refs, 0);
    psArray *regions = psArrayAlloc(MEASURE_SAMPLES); // Array of sample regions, made on each image
    psImage *samplesMean = psImageAlloc(num, MEASURE_SAMPLES, PS_TYPE_F32); // Measurements for each file
    psImage *samplesStdev = psImageAlloc(num, MEASURE_SAMPLES, PS_TYPE_F32); // Errors for each file
    psStats *stats = psStatsAlloc(meanStat | stdevStat);
    int numRows = 0, numCols = 0; // Size of images
    for (long i = 0; i < images->n; i++) {
        pmReadout *readout = readouts->data[i]; // Readout of interest
        if (!readout) {
            continue;
        }

        bool mdok;                      // Status of MD lookup
        float exptime = psMetadataLookupF32(&mdok, readout->parent->concepts, "CELL.EXPOSURE"); // Exp. time
        if (!mdok || !isfinite(exptime)) {
            psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Exposure time for readout %ld is not set.\n", i);
            goto MEASURE_ERROR;
        }
        exptimes->data.F32[i] = exptime;

        psImage *image = readout->image; // Image of interest
        if (!image) {
            continue;
        }
        images->data[i] = psMemIncrRefCounter(image);
        if (image->type.type != PS_TYPE_F32) {
            psError(PS_ERR_BAD_PARAMETER_TYPE, true, "Bad type for image: %x\n", image->type.type);
            goto MEASURE_ERROR;
        }
        if (numRows == 0 || numCols == 0) {
            numRows = image->numRows;
            numCols = image->numCols;
            // define the reference region : a box of size 'size' at the center
            // XXX unused psRegion refRegion = psRegionForSquare(0.5 * numCols, 0.5 * numRows, size);
            // Set up the sample regions : boxes of size 'size' at the 4 image corners
            for (int j = 0; j < MEASURE_SAMPLES; j++) {
                int x = (j % 2) ? size : image->numCols - size;
                int y = (j > 1) ? size : image->numRows - size;
                psRegion region = psRegionForSquare(x, y, size);
                region = psRegionForImage(image, region);
                regions->data[j] = psRegionAlloc(region.x0, region.x1, region.y0, region.y1);
            }
        } else if (numRows != image->numRows || numCols != image->numCols) {
            psError(PS_ERR_BAD_PARAMETER_SIZE, true,
                    "Image sizes don't match: %dx%d vs %dx%d\n", image->numCols, image->numRows,
                    numCols, numRows);
            goto MEASURE_ERROR;
        }
        psImage *mask = readout->mask; // Mask of interest
        if (mask) {
            if (!masks) {
                psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Not all readouts have masks.\n");
                goto MEASURE_ERROR;
            }
            masks->data[i] = psMemIncrRefCounter(mask);

            if (mask->type.type != PS_TYPE_IMAGE_MASK) {
                psError(PS_ERR_BAD_PARAMETER_TYPE, true, "Bad type for mask: %x\n", mask->type.type);
                goto MEASURE_ERROR;
            }
            if (mask->numRows != numRows || mask->numCols != numCols) {
                psError(PS_ERR_BAD_PARAMETER_SIZE, true,
                        "Mask sizes don't match: %dx%d vs %dx%d\n", mask->numCols, mask->numRows,
                        numCols, numRows);
                goto MEASURE_ERROR;
            }
        } else if (masks) {
            psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Not all readouts have masks.\n");
            goto MEASURE_ERROR;
        }

        psImage *variance = readout->variance; // Variance map of interest
        if (variance) {
            if (!variances) {
                psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Not all readouts have variances.\n");
                goto MEASURE_ERROR;
            }
            variances->data[i] = psMemIncrRefCounter(variance);

            if (variance->type.type != PS_TYPE_F32) {
                psError(PS_ERR_BAD_PARAMETER_TYPE, true, "Bad type for variances: %x\n", variance->type.type);
                goto MEASURE_ERROR;
            }
            if (variance->numRows != numRows || variance->numCols != numCols) {
                psError(PS_ERR_BAD_PARAMETER_SIZE, true,
                        "Variance sizes don't match: %dx%d vs %dx%d\n", variance->numCols, variance->numRows,
                        numCols, numRows);
                goto MEASURE_ERROR;
            }
        } else if (variances) {
            psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Not all readouts have variances.\n");
            goto MEASURE_ERROR;
        }


        // Measure statistics
        if (!psImageStats(stats, image, mask, maskVal)) {
            psWarning("Unable to measure reference statistics.\n");
        }
        refs->data.F32[i] = psStatsGetValue(stats, meanStat);
        psTrace("psModules.detrend", 3, "Reference value for image %ld = %f\n", i, refs->data.F32[i]);
        if (refs->data.F32[i] <= 0.0) {
            psError(PS_ERR_UNKNOWN, true, "Measured non-positive reference value.\n");
            goto MEASURE_ERROR;
        }
        refs->data.F32[i] = 1.0 / refs->data.F32[i];
        for (int j = 0; j < MEASURE_SAMPLES; j++) {
            psRegion *region = regions->data[j]; // Region of interest
            psImage *subImage = psImageSubset(image, *region); // Sub-image
            psImage *subMask = NULL;
            if (mask) {
                subMask = psImageSubset(mask, *region);
            }
            if (!psImageStats(stats, subImage, subMask, maskVal)) {
                psString regionString = psRegionToString(*region);
                psWarning("Unable to measure sample statistics at %s in image %ld.\n",
                          regionString, i);
                psFree(regionString);
            }
            psFree(subImage);
            samplesMean->data.F32[j][i] = psStatsGetValue(stats, meanStat) * refs->data.F32[i];
            samplesStdev->data.F32[j][i] = psStatsGetValue(stats, stdevStat) * refs->data.F32[i];
            psTrace("psModules.detrend", 5, "Image %ld, sample %d: %f +/- %f\n", i, j,
                    samplesMean->data.F32[j][i], samplesStdev->data.F32[j][i]);
        }
    }
    psFree(regions);
    psFree(stats);

    float meanRef = 0.0;                // Mean reference offset
    int numGood = 0;                    // Number of good measurements
    psVector *counts = psVectorAlloc(num, PS_TYPE_F32); // Mean for each image
    psVector *errors = psVectorAlloc(num, PS_TYPE_F32); // Stdev for each image
    for (int i = 0; i < MEASURE_SAMPLES; i++) {
        counts = psImageRow(counts, samplesMean, i);
        errors = psImageRow(errors, samplesStdev, i);
        pmShutterCorrection *guess = pmShutterCorrectionGuess(exptimes, counts); // Guess at correction
        pmShutterCorrection *corr = pmShutterCorrectionFullFit(exptimes, counts, errors, guess); // Correct'n
        if (!corr) {
            psError(PS_ERR_UNEXPECTED_NULL, false, "Unable to measure shutter reference correction.\n");
            psFree(guess);
            psFree(counts);
            psFree(errors);
            goto MEASURE_ERROR;
        }
        psTrace("psModules.detrend", 5, "Sample reference value: %f\n", corr->offref);
        if (isfinite(corr->offref)) {
            meanRef += corr->offref;
            numGood++;
        }
        psFree(corr);
        psFree(guess);
    }
    psFree(samplesMean);
    psFree(samplesStdev);

    if (numGood == 0) {
        psError(PS_ERR_UNKNOWN, true, "Unable to measure mean reference offset.\n");
        psFree(counts);
        psFree(errors);
        goto MEASURE_ERROR;
    }
    meanRef /= (float)numGood;
    psTrace("psModules.detrend", 3, "Mean reference value: %f\n", meanRef);

    // Check the variances
    if (variances && nIter > 1) {
        for (int i = 0; i < variances->n && nIter > 1; i++) {
            psImage *variance = variances->data[i]; // Variance image
            if (!variance) {
                // We don't have variances, so no realistic errors: turn off iteration
                if (nIter > 0) {
                    psWarning("Not all images have variances --- turning iteration off.\n");
                }
                nIter = 1;
            }
        }
    }

    psImage *shutter = psImageAlloc(numCols, numRows, PS_TYPE_F32); // Shutter correction image
    psImage *pattern = psImageAlloc(numCols, numRows, PS_TYPE_F32); // Illumination pattern
    psVector *mask = psVectorAlloc(num, PS_TYPE_VECTOR_MASK); // Mask for each image
    psVectorInit(mask, 0);
    psTrace("psModules.detrend", 2, "Performing linear fit on individual pixels...\n");
    for (int y = 0; y < numRows; y++) {
        for (int x = 0; x < numCols; x++) {
            for (int i = 0; i < num; i++) {
                psImage *image = images->data[i]; // Image of interest
                counts->data.F32[i] = image->data.F32[y][x] * refs->data.F32[i];
                psImage *maskImage;     // Mask image
                if (masks && (maskImage = masks->data[i])) {
                    mask->data.PS_TYPE_VECTOR_MASK_DATA[i] = (maskImage->data.PS_TYPE_IMAGE_MASK_DATA[y][x] & maskVal);
                }
                psImage *variance;        // Variance image
                if (variances && (variance = variances->data[i])) {
                    errors->data.F32[i] = sqrtf(variance->data.F32[y][x]) * refs->data.F32[i];
                } else {
                    errors->data.F32[i] = sqrtf(image->data.F32[y][x]) * refs->data.F32[i];
                }
            }

            pmShutterCorrection *corr = pmShutterCorrectionLinFit(exptimes, counts, errors, mask, meanRef, nIter, rej);
            shutter->data.F32[y][x] = corr->offset;
            pattern->data.F32[y][x] = corr->scale;
            psFree(corr);
        }
    }
    psFree(mask);
    psFree(counts);
    psFree(errors);
    psFree(refs);

    if (psTraceGetLevel("psModules.detrend") > 5) {
        psFits *fits = psFitsOpen("pattern.fits", "w");
        psFitsWriteImage(fits, NULL, pattern, 0, NULL);
        psFitsClose(fits);
    }
    psFree(pattern);

    output->image = shutter;

    // Update the "concepts"
    psList *inputCells = psListAlloc(NULL); // List of cells
    for (long i = 0; i < readouts->n; i++) {
        pmReadout *readout = readouts->data[i]; // Readout of interest
        psListAdd(inputCells, PS_LIST_TAIL, readout->parent);
    }
    bool success = pmConceptsAverageCells(output->parent, inputCells, NULL, NULL, true);
    psFree(inputCells);

    // Correct the exposure times --- they don't make sense any more.
    psMetadataItem *item = psMetadataLookup(output->parent->concepts, "CELL.EXPOSURE");
    item->data.F32 = NAN;
    item = psMetadataLookup(output->parent->concepts, "CELL.DARKTIME");
    item->data.F32 = NAN;

    return success;


MEASURE_ERROR:
    // Clean up after error
    psFree(exptimes);
    psFree(images);
    psFree(masks);
    psFree(variances);
    psFree(refs);
    psFree(regions);
    psFree(stats);
    psFree(samplesMean);
    psFree(samplesStdev);
    return false;
}


bool pmShutterCorrectionApplyScan_Threaded(psThreadJob *job)
{
    PS_ASSERT_THREAD_JOB_NON_NULL(job, false);

    psImage *image = job->args->data[0];
    psImage *mask  = job->args->data[1];
    psImage *var   = job->args->data[2];
    const psImage *shutterImage = job->args->data[3];

    float exptime    = PS_SCALAR_VALUE(job->args->data[4], F32);
    psImageMaskType blank = PS_SCALAR_VALUE(job->args->data[5], PS_TYPE_IMAGE_MASK_DATA);
    int rowStart     = PS_SCALAR_VALUE(job->args->data[6], S32);
    int rowStop      = PS_SCALAR_VALUE(job->args->data[7], S32);
    return pmShutterCorrectionApplyScan(image, mask, var, shutterImage, exptime, blank, rowStart, rowStop);
}

bool pmShutterCorrectionApplyScan(psImage *image, psImage *mask, psImage *var,
                                  const psImage *shutterImage, float exptime,
                                  psImageMaskType blank, int rowStart, int rowStop)
{
    // Neglecting asserts because inputs should have been checked already

    int numCols = image->numCols;       // Number of columns

    for (int y = rowStart; y < rowStop; y++) {
        for (int x = 0; x < numCols; x++) {
            if (mask && !isfinite(shutterImage->data.F32[y][x])) {
                mask->data.PS_TYPE_IMAGE_MASK_DATA[y][x] |= blank;
                image->data.F32[y][x] = NAN;
                if (var) {
                    var->data.F32[y][x] = NAN;
                }
                continue;
            }
            float correction = exptime / (exptime + shutterImage->data.F32[y][x]); // Correction factor
            image->data.F32[y][x] *= correction;
            if (var) {
                var->data.F32[y][x] *= PS_SQR(correction);
            }
        }
    }
    return true;
}

bool pmShutterCorrectionApply(pmReadout *readout, const pmReadout *shutter, psImageMaskType blank)
{
    PM_ASSERT_READOUT_NON_NULL(readout, false);
    PM_ASSERT_READOUT_NON_NULL(shutter, false);
    PM_ASSERT_READOUT_IMAGE(readout, false);
    PM_ASSERT_READOUT_IMAGE(shutter, false);

    psRegion region = psRegionSet(readout->col0, readout->col0 + readout->image->numCols,
                                  readout->row0, readout->row0 + readout->image->numRows); // Detector region

    pmCell *cell = readout->parent;     // Parent cell
    if (!cell) {
        psError(PS_ERR_BAD_PARAMETER_NULL, true,
                "Parent cell is NULL --- unable to determine exposure time.\n");
        return false;
    }
    float exptime = psMetadataLookupF32(NULL, cell->concepts, "CELL.EXPOSURE"); // Exposure time
    if (!isfinite(exptime)) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Bad exposure time: %f.\n", exptime);
        return false;
    }

    pmHDU *hdu = pmHDUFromCell(cell);// HDU  of interest

    psVector *md5 = psImageMD5(shutter->image); // md5 hash
    psString md5string = psMD5toString(md5); // String
    psFree(md5);
    psStringPrepend(&md5string, "Shutter image MD5: ");
    psMetadataAddStr(hdu->header, PS_LIST_TAIL, "HISTORY", PS_META_DUPLICATE_OK, md5string, "");
    psFree(md5string);


    psImage *shutterImage = psImageSubset(shutter->image, region); // Subimage with shutter
    if (!shutterImage) {
        psString regionString = psRegionToString(region);
        psError(PS_ERR_BAD_PARAMETER_VALUE, false, "Size mismatch: %s vs %dx%d\n",
                regionString, shutter->image->numCols, shutter->image->numRows);
        psFree(regionString);
        psFree(shutterImage);
        return false;
    }
    psImage *image = readout->image;    // Image to correct
    psImage *mask = readout->mask;      // Corresponding mask
    psImage *var = readout->variance;   // Corresponding variance map

    bool threaded = true;
    int scanRows = pmDetrendGetScanRows();
    if (scanRows == 0) {
        threaded = false;
        scanRows = image->numRows;
    }

    if (exptime <= 0.0) {
        // In the extreme case that we have exptime <= 0.0, we correct the image to
        // counts-per-second, rather than counts in the nominal exposure time
        for (int y = 0; y < image->numRows; y++) {
            for (int x = 0; x < image->numCols; x++) {
                if (mask && !isfinite(shutterImage->data.F32[y][x])) {
                    mask->data.PS_TYPE_IMAGE_MASK_DATA[y][x] |= blank;
                    image->data.F32[y][x] = NAN;
                    continue;
                }
                image->data.F32[y][x] *= 1.0 / (exptime + shutterImage->data.F32[y][x]);
            }
        }
        psMetadataAddF32(cell->concepts, PS_LIST_TAIL, "CELL.EXPOSURE", PS_META_REPLACE,
                         "exposure time re-normalized to 1.0", 1.0); // Exposure time
        psString line = NULL;
        psStringAppend(&line, "extreme exposure time %f, re-normalized to 1.0", exptime);
        psMetadataAddStr(hdu->header, PS_LIST_TAIL, "HISTORY", PS_META_DUPLICATE_OK, line, "");
        psFree(line);
    } else {
        for (int rowStart = 0; rowStart < image->numRows; rowStart += scanRows) {
            int rowStop = PS_MIN (rowStart + scanRows, image->numRows);

            if (threaded) {
                // allocate a job, construct the arguments for this job
                psThreadJob *job = psThreadJobAlloc("PSMODULES_DETREND_SHUTTER");
                psArrayAdd(job->args, 1, image);
                psArrayAdd(job->args, 1, mask);
                psArrayAdd(job->args, 1, var);
                psArrayAdd(job->args, 1, shutterImage);
                PS_ARRAY_ADD_SCALAR(job->args, exptime, PS_TYPE_F32);
                PS_ARRAY_ADD_SCALAR(job->args, blank, PS_TYPE_IMAGE_MASK);
                PS_ARRAY_ADD_SCALAR(job->args, rowStart, PS_TYPE_S32);
                PS_ARRAY_ADD_SCALAR(job->args, rowStop, PS_TYPE_S32);

                if (!psThreadJobAddPending(job)) {
                    return false;
                }
            } else if (!pmShutterCorrectionApplyScan(image, mask, var, shutterImage, exptime, blank,
                                                     rowStart, rowStop)) {
                psError(PS_ERR_UNKNOWN, false, "Unable to apply shutter correction.");
                psFree(shutterImage);
                return false;
            }
        }
        if (threaded) {
            // wait here for the threaded jobs to finish
            if (!psThreadPoolWait(true, true)) {
                psError(PS_ERR_UNKNOWN, false, "Unable to apply shutter correction.");
                psFree(shutterImage);
                return false;
            }
        }
    }
    psFree(shutterImage);

    psTime *time = psTimeGetNow(PS_TIME_TAI); // The time now, used for reporting
    psString timeString = psTimeToISO(time); // String with time
    psFree(time);
    psStringPrepend(&timeString, "Shutter correction completed at ");
    psMetadataAddStr(hdu->header, PS_LIST_TAIL, "HISTORY", PS_META_DUPLICATE_OK,
                     timeString, "");
    psFree(timeString);


    return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////


#define IMAGES_BUFFER 10                // Allocate space for this many images at a time

static void shutterCorrectionDataFree(pmShutterCorrectionData *data)
{
    psFree(data->regions);
    psFree(data->mean);
    psFree(data->stdev);

    psFree(data->exptimes);
    psFree(data->refs);

    return;
}

pmShutterCorrectionData *pmShutterCorrectionDataAlloc(int numCols, int numRows, int size)
{
    pmShutterCorrectionData *data = psAlloc(sizeof(pmShutterCorrectionData));
    psMemSetDeallocator(data, (psFreeFunc)shutterCorrectionDataFree);

    data->num = 0;
    data->numCols = 0;
    data->numRows = 0;

    data->regions = psArrayAlloc(MEASURE_SAMPLES);
    for (int j = 0; j < MEASURE_SAMPLES; j++) {
        int x = (j % 2) ? size : numCols - size - 1;
        int y = (j > 1) ? size : numRows - size - 1;
        psRegion region = psRegionForSquare(x, y, size);
        data->regions->data[j] = psRegionAlloc(region.x0, region.x1, region.y0, region.y1);
    }

    data->mean = psArrayAlloc(MEASURE_SAMPLES);
    data->stdev = psArrayAlloc(MEASURE_SAMPLES);
    for (int i = 0; i < MEASURE_SAMPLES; i++) {
        data->mean->data[i] = psVectorAllocEmpty(IMAGES_BUFFER, PS_TYPE_F32);
        data->stdev->data[i] = psVectorAllocEmpty(IMAGES_BUFFER, PS_TYPE_F32);
    }

    data->exptimes = psVectorAllocEmpty(IMAGES_BUFFER, PS_TYPE_F32);
    data->refs = psVectorAllocEmpty(IMAGES_BUFFER, PS_TYPE_F32);

    return data;
}

bool pmShutterCorrectionAddReadout(pmShutterCorrectionData *data,
                                   const pmReadout *readout, ///< Readout to add
                                   psStatsOptions meanStat, ///< Statistic to use for mean
                                   psStatsOptions stdevStat, ///< Statistic to use for stdev
                                   psImageMaskType maskVal, ///< Mask value
                                   psRandom *rng ///< Random number generator
    )
{
    PS_ASSERT_PTR_NON_NULL(data, NULL);
    PS_ASSERT_PTR_NON_NULL(readout, NULL);
    PS_ASSERT_IMAGE_NON_NULL(readout->image, NULL);
    PS_ASSERT_IMAGE_TYPE(readout->image, PS_TYPE_F32, NULL);
    if (data->num == 0) {
        data->numCols = readout->image->numCols;
        data->numRows = readout->image->numRows;
    } else {
        PS_ASSERT_IMAGE_SIZE(readout->image, data->numCols, data->numRows, NULL);
    }
    if (readout->mask) {
        PS_ASSERT_IMAGE_NON_NULL(readout->mask, NULL);
        PS_ASSERT_IMAGE_TYPE(readout->mask, PS_TYPE_IMAGE_MASK, NULL);
        PS_ASSERT_IMAGE_SIZE(readout->mask, data->numCols, data->numRows, NULL);
    }
    if (readout->variance) {
        PS_ASSERT_IMAGE_NON_NULL(readout->variance, NULL);
        PS_ASSERT_IMAGE_TYPE(readout->variance, PS_TYPE_F32, NULL);
        PS_ASSERT_IMAGE_SIZE(readout->variance, data->numCols, data->numRows, NULL);
    }

    // Add the exposure time
    float exptime = psMetadataLookupF32(NULL, readout->parent->concepts, "CELL.EXPOSURE"); // Exp. time
    if (!isfinite(exptime)) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Exposure time is not set.");
        return false;
    }
    data->exptimes->data.F32[data->exptimes->n] = exptime;
    data->exptimes = psVectorExtend(data->exptimes, IMAGES_BUFFER, 1);

    // Add the statistics

    // Add the reference value
    psStats *stats = psStatsAlloc(meanStat | stdevStat); // Statistics to apply
    if (!rng) {
        rng = psRandomAlloc(PS_RANDOM_TAUS);
    } else {
        psMemIncrRefCounter(rng);
    }
    if (!psImageBackground(stats, NULL, readout->image, readout->mask, maskVal, rng)) {
        psError(PS_ERR_UNKNOWN, false, "Unable to measure reference statistics.\n");
        psFree(stats);
        psFree(rng);
        return false;
    }
    psFree(rng);
    float refValue = psStatsGetValue(stats, meanStat); // Reference value
    psTrace("psModules.detrend", 3, "Reference value & exptime for shutter image : %f cnts %f sec\n", refValue, exptime);
    if (refValue <= 0.0) {
        psError(PS_ERR_UNKNOWN, true, "Measured non-positive reference value.\n");
        psFree(stats);
        return false;
    }
    refValue = 1.0 / refValue;
    data->refs->data.F32[data->refs->n] = refValue;
    data->refs = psVectorExtend(data->refs, IMAGES_BUFFER, 1);

    // Add the region statistics
    for (int j = 0; j < MEASURE_SAMPLES; j++) {
        psRegion *region = data->regions->data[j]; // Region of interest
        psRegion adjusted = *region;    // Adjusted region, compensating for offsets
        adjusted.x0 += readout->image->col0;
        adjusted.x1 += readout->image->col0;
        adjusted.y0 += readout->image->row0;
        adjusted.y1 += readout->image->row0;
        psImage *subImage = psImageSubset(readout->image, adjusted); // Sub-image
        psImage *subMask = NULL;        // Sub-image of mask
        if (readout->mask) {
            subMask = psImageSubset(readout->mask, adjusted);
        }
        if (!psImageStats(stats, subImage, subMask, maskVal)) {
            psString regionString = psRegionToString(adjusted);
            psWarning("Unable to measure sample statistics at %s in image.\n",
                      regionString);
            psFree(regionString);
        }
        psFree(subImage);
        psFree(subMask);

        psVector *mean = data->mean->data[j]; // Vector of means for this region
        psVector *stdev = data->stdev->data[j]; // Vector of standard deviations for this region

        mean->data.F32[mean->n] = psStatsGetValue(stats, meanStat) * refValue;
        stdev->data.F32[stdev->n] = psStatsGetValue(stats, stdevStat) * refValue;

        psTrace("psModules.detrend", 5, "input shutter image sample value %d: %f +/- %f  ->  %f +/- %f\n", j,
                psStatsGetValue(stats, meanStat), psStatsGetValue(stats, stdevStat),
                mean->data.F32[mean->n], stdev->data.F32[stdev->n]);

        data->mean->data[j] = psVectorExtend(mean, IMAGES_BUFFER, 1);
        data->stdev->data[j] = psVectorExtend(stdev, IMAGES_BUFFER, 1);
    }

    data->num++;

    return true;
}

float pmShutterCorrectionReference(pmShutterCorrectionData *data)
{
    PS_ASSERT_PTR_NON_NULL(data, NAN);
    PS_ASSERT_INT_POSITIVE(data->num, NAN);

    // supply counts sorted by exptime

    // generate the index for the exptimes vector
    psVector *index = psVectorSortIndex (NULL, data->exptimes);
    psVector *newtimes = psVectorAlloc (data->exptimes->n, PS_TYPE_F32);

    // reshuffle exptimes to new sequence (this is only a local value)
    for (int j = 0; j < newtimes->n; j++) {
        newtimes->data.F32[j] = data->exptimes->data.F32[index->data.S32[j]];
    }

    float meanRef = 0.0;                // Mean reference offset
    int numGood = 0;                    // Number of good measurements
    for (int i = 0; i < MEASURE_SAMPLES; i++) {
        psVector *newcounts = psVectorAlloc (data->exptimes->n, PS_TYPE_F32);
        psVector *newerrors = psVectorAlloc (data->exptimes->n, PS_TYPE_F32);
        psVector *counts = data->mean->data[i];
        psVector *errors = data->stdev->data[i];

        for (int j = 0; j < newcounts->n; j++) {
            newcounts->data.F32[j] = counts->data.F32[index->data.S32[j]];
            newerrors->data.F32[j] = errors->data.F32[index->data.S32[j]];
        }

        // use the sorted exptime, counts, and errors for the measurements
        pmShutterCorrection *guess = pmShutterCorrectionGuess(newtimes, newcounts); // Guess at correction
        psTrace("psModules.detrend", 5, "Shutter correction guess: scale: %f, offset: %f, offref: %f\n", guess->scale, guess->offset, guess->offref);

        pmShutterCorrection *corr = pmShutterCorrectionFullFit(newtimes, newcounts, newerrors, guess); // The actual correction

        if (corr) {
            psTrace("psModules.detrend", 5, "Shutter correction fit: scale: %f, offset: %f, offref: %f\n", corr->scale, corr->offset, corr->offref);
            if (isfinite(corr->offref) && corr->valid) {
                psTrace("psModules.detrend", 5, "Sample reference value: %f\n", corr->offref);
                meanRef += corr->offref;
                numGood++;
            }
        } else {
            psTrace("psModules.detrend", 5, "failed Shutter correction fit\n");
        }

        psFree(corr);
        psFree(guess);
        psFree (newcounts);
        psFree (newerrors);
    }
    psFree (newtimes);
    psFree (index);

    if (numGood == 0) {
        psError(PS_ERR_UNKNOWN, true, "Unable to measure mean reference offset.\n");
        return false;
    }
    meanRef /= (float)numGood;
    psTrace("psModules.detrend", 3, "Mean reference value: %f\n", meanRef);
    return meanRef;
}

bool pmShutterCorrectionGeneratePrepare(pmReadout *shutter, pmReadout *pattern, const psArray *inputs,
                                        psImageMaskType maskVal)
{
    PS_ASSERT_PTR_NON_NULL(shutter, false);
    PS_ASSERT_PTR_NON_NULL(pattern, false);
    PS_ASSERT_ARRAY_NON_NULL(inputs, false);

    // determine the output image size based on the input images
    int row0, col0, numCols, numRows;
    if (!pmReadoutStackSetOutputSize(&col0, &row0, &numCols, &numRows, inputs)) {
        psError(PS_ERR_UNKNOWN, false, "problem setting output readout size.");
        return false;
    }

    // generate the required output image based on the specified sizes
    pmReadoutStackDefineOutput(shutter, col0, row0, numCols, numRows, false, false, maskVal);
    if (pattern) {
        pmReadoutStackDefineOutput(pattern, col0, row0, numCols, numRows, false, false, maskVal);
    }

    psImage *nums = pmReadoutSetAnalysisImage(shutter, PM_READOUT_STACK_ANALYSIS_COUNT, numCols, numRows,
                                              PS_TYPE_U16, 0); // Image with number fitted per pixel
    if (!nums) {
        return false;
    }
    psImage *sigma = pmReadoutSetAnalysisImage(shutter, PM_READOUT_STACK_ANALYSIS_SIGMA, numCols, numRows,
                                               PS_TYPE_F32, NAN); // Image with stdev per pixel
    if (!sigma) {
        return false;
    }

    // Update the "concepts"
    psList *inputCells = psListAlloc(NULL); // List of cells
    for (long i = 0; i < inputs->n; i++) {
        pmReadout *readout = inputs->data[i]; // Readout of interest
        psListAdd(inputCells, PS_LIST_TAIL, readout->parent);
    }
    bool success = pmConceptsAverageCells(shutter->parent, inputCells, NULL, NULL, true);
    psFree(inputCells);

    // Correct the exposure times --- they don't make sense any more.
    psMetadataItem *item = psMetadataLookup(shutter->parent->concepts, "CELL.EXPOSURE");
    item->data.F32 = NAN;
    item = psMetadataLookup(shutter->parent->concepts, "CELL.DARKTIME");
    item->data.F32 = NAN;

    shutter->data_exists = true;
    shutter->parent->data_exists = true;
    shutter->parent->parent->data_exists = true;

    pattern->data_exists = true;
    if (pattern->parent) {
        pattern->parent->data_exists = true;
        if (pattern->parent->parent) {
            pattern->parent->parent->data_exists = true;
        }
    }

    return success;
}

bool pmShutterCorrectionGenerate(pmReadout *shutter, pmReadout *pattern, const psArray *inputs,
                                 float reference, const pmShutterCorrectionData *data,
                                 int nIter, float rej, psImageMaskType maskVal)
{
    PS_ASSERT_PTR_NON_NULL(shutter, false);
    PS_ASSERT_PTR_NON_NULL(pattern, false);
    PS_ASSERT_ARRAY_NON_NULL(inputs, false);
    PS_ASSERT_INT_EQUAL(data->num, inputs->n, false);
    PS_ASSERT_INT_NONNEGATIVE(nIter, false);
    PS_ASSERT_FLOAT_LARGER_THAN(rej, 0.0, false);

    int minInputCols, maxInputCols, minInputRows, maxInputRows; // Smallest and largest values to combine
    int xSize, ySize;                   // Size of the output image
    if (!pmReadoutStackValidate(&minInputCols, &maxInputCols, &minInputRows, &maxInputRows, &xSize, &ySize,
                                inputs)) {
        psError(PS_ERR_UNKNOWN, false, "No valid input readouts.");
        return false;
    }

    psImage *nums = pmReadoutGetAnalysisImage(shutter, PM_READOUT_STACK_ANALYSIS_COUNT);
    if (!nums) {
        return false;
    }
    psImage *sigma = pmReadoutGetAnalysisImage(shutter, PM_READOUT_STACK_ANALYSIS_SIGMA);
    if (!sigma) {
        return false;
    }

    psImage *shutterImage = shutter->image; // Shutter correction image
    psImage *patternImage = pattern->image; // Illumination pattern

    int num = data->num;                // Number of images
    psVector *counts = psVectorAlloc(num, PS_TYPE_F32); // Counts in each image
    psVector *errors = psVectorAlloc(num, PS_TYPE_F32); // Counts in each image
    psVector *mask = psVectorAlloc(num, PS_TYPE_VECTOR_MASK); // Mask for each image
    psTrace("psModules.detrend", 2, "Performing linear fit on individual pixels...\n");
    for (int i = minInputRows; i < maxInputRows; i++) {
        int yOut = i - shutter->row0; // y position on output readout
        for (int j = minInputCols; j < maxInputCols; j++) {
            int xOut = j - shutter->col0; // x position on output readout

            psVectorInit(mask, 0);
            for (int r = 0; r < num; r++) {
                pmReadout *readout = inputs->data[r]; // Readout of interest
                int yIn = i - readout->row0; // y position on input readout
                int xIn = j - readout->col0; // x position on input readout
                psImage *image = readout->image; // Image of interest
                float ref = data->refs->data.F32[r]; // (Inverse) reference value
                counts->data.F32[r] = image->data.F32[yIn][xIn] * ref;
                if (readout->mask) {
                    mask->data.PS_TYPE_VECTOR_MASK_DATA[r] = (readout->mask->data.PS_TYPE_IMAGE_MASK_DATA[yIn][xIn] & maskVal);
                }
                if (readout->variance) {
                    errors->data.F32[r] = sqrtf(readout->variance->data.F32[yIn][xIn]) * ref;
                } else {
                    // XXX guess that the input data is Poisson distributed; if we go negative, force high
                    errors->data.F32[r] = sqrtf(fabs(image->data.F32[yIn][xIn])) * ref;
                }
            }

            pmShutterCorrection *corr = pmShutterCorrectionLinFit(data->exptimes, counts, errors, mask, reference, nIter, rej);
            if (!corr) {
                // Nothing we can do about it
                psErrorClear();
                shutterImage->data.F32[yOut][xOut] = NAN;
                patternImage->data.F32[yOut][xOut] = NAN;
                nums->data.U16[yOut][xOut] = 0;
                sigma->data.F32[yOut][xOut] = NAN;
                continue;
            }
            shutterImage->data.F32[yOut][xOut] = corr->offset;
            patternImage->data.F32[yOut][xOut] = corr->scale;
            nums->data.U16[yOut][xOut] = corr->num;
            sigma->data.F32[yOut][xOut] = corr->stdev;
            psFree(corr);
        }
    }
    psFree(mask);
    psFree(errors);
    psFree(counts);

    return true;
}
