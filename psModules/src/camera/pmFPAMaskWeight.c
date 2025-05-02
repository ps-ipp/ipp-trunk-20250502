#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <assert.h>

#include <pslib.h>
#include "pmHDU.h"
#include "pmFPA.h"
#include "pmHDUUtils.h"
#include "pmHDUGenerate.h"
#include "pmFPAMaskWeight.h"

#define PIXELS_BUFFER 100               // Size of buffer for allocating pixel lists
#define RENORM_NUM_SIGMA 3.0            // Number of standard deviations for Gaussian kernel
#define RENORM_PEAK 2.0                 // Number of standard deviations of noise for fake source peak flux


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// File-static (private) functions
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Create the parent mask images that reside in the HDU
static void createParentMasks(pmHDU *hdu // The HDU for which to create
                             )
{
    assert(hdu);
    assert(hdu->images);

    // Generate the parent mask images
    psArray *images = hdu->images;      // Array of images
    psArray *masks = hdu->masks;        // Array of masks
    if (!masks) {
        masks = psArrayAlloc(images->n);
        hdu->masks = masks;
    }

    for (long i = 0; i < images->n; i++) {
        psImage *image = images->data[i]; // The image for this readout
        if (!image || masks->data[i]) {
            continue;
        }
        masks->data[i] = psImageAlloc(image->numCols, image->numRows, PS_TYPE_IMAGE_MASK);
        psImageInit(masks->data[i], 0);
    }

    return;
}

// Create the parent variance images that reside in the HDU
static void createParentVariances(pmHDU *hdu // The HDU for which to create
                               )
{
    assert(hdu);
    assert(hdu->images);

    // Generate the parent mask images
    psArray *images = hdu->images;      // Array of images
    psArray *variances = hdu->variances;    // Array of variance images
    if (!variances) {
        variances = psArrayAlloc(images->n);
        hdu->variances = variances;
    }

    for (long i = 0; i < images->n; i++) {
        psImage *image = images->data[i]; // The image for this readout
        if (!image || variances->data[i]) {
            continue;
        }
        variances->data[i] = psImageAlloc(image->numCols, image->numRows, PS_TYPE_F32);
    }

    return;
}

// Identify a readout within the HDU, on the basis of the image pointer.
// This is a little dirty, but hopefully should work....
static long identifyReadout(pmHDU *hdu, // The HDU containing the readouts
                            pmReadout *readout // The readout to be identified
                           )
{
    assert(hdu);
    assert(readout);

    long index = -1;                    // Index of the readout
    for (long i = 0; i < hdu->images->n && index == -1; i++) {
        if (hdu->images->data[i] == readout->image->parent) {
            index = i;
        }
    }

    return index;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Public functions
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool pmReadoutSetMask(pmReadout *readout, psImageMaskType satMask, psImageMaskType badMask)
{
    PS_ASSERT_PTR_NON_NULL(readout, false);
    PS_ASSERT_IMAGE_NON_NULL(readout->image, false);

    pmCell *cell = readout->parent;     // The parent cell
    bool mdok = true;                   // Status of MD lookup

    // Get the "concepts" of interest
    float saturation = psMetadataLookupF32(&mdok, cell->concepts, "CELL.SATURATION"); // Saturation level
    if (!mdok || isnan(saturation)) {
        // psError(PS_ERR_IO, true, "CELL.SATURATION is not set --- unable to set mask.\n");
        // return false;
        psWarning("CELL.SATURATION is not set --- completely masking cell.\n");
        saturation = NAN;
    }
    float bad = psMetadataLookupF32(&mdok, cell->concepts, "CELL.BAD"); // Bad level
    if (!mdok || isnan(bad)) {
        // psError(PS_ERR_IO, true, "CELL.BAD is not set --- unable to set mask.\n");
        // return false;
        psWarning("CELL.BAD is not set --- completely masking cell.\n");
        bad = NAN;
    }
    psTrace("psModules.camera", 5, "Saturation: %f, bad: %f\n", saturation, bad);

    // if CELL.GAIN or CELL.READNOISE are not set, then the variance will be set to NAN;
    // in this case, we have to set the mask as well
    float gain = psMetadataLookupF32(&mdok, cell->concepts, "CELL.GAIN"); // Cell gain
    if (!mdok) { gain = NAN; }
    float readnoise = psMetadataLookupF32(&mdok, cell->concepts, "CELL.READNOISE"); // Cell read noise
    if (!mdok) { readnoise = NAN; }

    // Set up the mask
    psImage *image = readout->image;    // The image pixels
    if (!readout->mask) {
        // Generate a (throwaway) mask image, if required
        readout->mask = psImageAlloc(image->numCols, image->numRows, PS_TYPE_IMAGE_MASK);
    }
    psImage *mask = readout->mask;      // The mask pixels

    // completely mask if SATURATION or BAD are invalid
    if (isnan(saturation) || isnan(bad) || isnan(gain) || isnan(readnoise)) {
        psImageInit(mask, badMask);
        return true;
    }

    psImageInit(mask, 0);

    // Dereference pointers for speed
    psF32 **imageData = image->data.F32;// The image
    psImageMaskType **maskData = mask->data.PS_TYPE_IMAGE_MASK_DATA;  // The mask

    for (int i = 0; i < image->numRows; i++) {
        for (int j = 0; j < image->numCols; j++) {
            if (imageData[i][j] >= saturation) {
                maskData[i][j] |= satMask;
            }
            if (imageData[i][j] <= bad) {
                maskData[i][j] |= badMask;
            }
            if (!isfinite(imageData[i][j])) {
                maskData[i][j] |= badMask;
            }
        }
    }

    return true;
}

// XXX this function creates the mask pixels, or uses the existing mask
// pixels.  currently, it will set mask bits if (value <= BAD) or (value >= SATURATION)
// should we optionally ignore these tests?
bool pmReadoutGenerateMask(pmReadout *readout, psImageMaskType satMask, psImageMaskType badMask)
{
    PS_ASSERT_PTR_NON_NULL(readout, false);

    pmCell *cell = readout->parent;     // The parent cell
    bool mdok = true;                   // Status of MD lookup

    // Create the mask image if required
    if (!readout->mask) {
        psRegion *trimsec = psMetadataLookupPtr(&mdok, cell->concepts, "CELL.TRIMSEC"); // Trim section
        if (!mdok || psRegionIsNaN(*trimsec)) {
            psError(PS_ERR_IO, true, "CELL.TRIMSEC is not set --- unable to set mask.\n");
            return false;
        }

        pmHDU *hdu = pmHDUFromCell(cell);   // The HDU containing the cell's pixels
        PS_ASSERT_PTR_NON_NULL(hdu, false);
        if (!hdu->images && !pmHDUGenerateForCell(cell)) {
            psError(PS_ERR_UNKNOWN, false, "Unable to generate HDU for cell.\n");
            return false;
        }

        createParentMasks(hdu);

        // Need to identify which readout we're working with....
        long index = identifyReadout(hdu, readout); // Index of the readout
        if (index == -1) {
            psError(PS_ERR_UNKNOWN, true, "Unable to identify readout image in HDU.\n");
            return false;
        }

        psImage *mask = psImageSubset(hdu->masks->data[index], *trimsec); // The mask pixels
        if (!mask) {
            psString trimsecString = psRegionToString(*trimsec);
            psError(PS_ERR_UNKNOWN, false, "Unable to set mask from HDU with trimsec: %s.\n", trimsecString);
            psFree(trimsecString);
            return false;
        }
        psImageInit(mask, 0);
        assert (readout->mask == NULL); // or else this is a memory leak.
        readout->mask = mask;
    }

    return pmReadoutSetMask(readout, satMask, badMask);
}

bool pmReadoutSetVariance(pmReadout *readout, const psImage *noiseMap, bool poisson)
{
    PS_ASSERT_PTR_NON_NULL(readout, false);
    // check that the noiseMap (if it exists) matches the readout variance size)

    pmCell *cell = readout->parent;     // The parent cell
    bool mdok = true;                   // Status of MD lookup

    // Get the "concepts" of interest
    float gain = psMetadataLookupF32(&mdok, cell->concepts, "CELL.GAIN"); // Cell gain
    if (!mdok || isnan(gain)) {
        // psError(PS_ERR_IO, true, "CELL.GAIN is not set --- unable to set variance.\n");
        // return false;
        psWarning("CELL.GAIN is not set --- setting variance to NAN\n");
        gain = NAN;
    }
    float readnoise = psMetadataLookupF32(&mdok, cell->concepts, "CELL.READNOISE"); // Cell read noise
    if (!mdok || isnan(readnoise)) {
        // psError(PS_ERR_IO, true, "CELL.READNOISE is not set --- unable to set variance.\n");
        // return false;
        psWarning("CELL.READNOISE is not set --- setting variance to NAN\n");
        readnoise = NAN;
    }
    // if we have a non-NAN readnoise, then we need to ensure it has been updated (not necessary if NAN)
    if (!isnan(gain) && psMetadataLookup(cell->concepts, "CELL.READNOISE.UPDATE")) {
        psError(PS_ERR_IO, true, "CELL.READNOISE has not yet been updated for the gain");
        return false;
    }

    // for invalid input data, set the readout variance to NAN
    if (isnan(gain) || isnan(readnoise)) {
        if (!readout->variance) {
            // generate the image if needed
            readout->variance = psImageAlloc(readout->image->numCols, readout->image->numRows, PS_TYPE_F32);
        }
        // XXX need to set the mask, if defined
        psImageInit(readout->variance, NAN);
        return true;
    }

    if (poisson) {
        // Set variance image to the variance in ADU = f/g + rn^2
        psImage *image = readout->image;    // The image pixels
        readout->variance = (psImage*)psBinaryOp(readout->variance, image, "/", psScalarAlloc(gain, PS_TYPE_F32));

        // a negative variance is non-sensical. if the image value drops below 1, the variance must be 1.
        // XXX this calculation is wrong: limit is 1 e-, but this is in DN
        readout->variance = (psImage*)psUnaryOp(readout->variance, readout->variance, "abs");
        readout->variance = (psImage*)psBinaryOp(readout->variance, readout->variance, "max",
                                               psScalarAlloc(1, PS_TYPE_F32));
    } else {
        // Just use the read noise
        if (!readout->variance) {
            readout->variance = psImageAlloc(readout->image->numCols, readout->image->numRows, PS_TYPE_F32);
        }
        psImageInit(readout->variance, 0.0);
    }

    // apply a supplied readnoise map (NOTE: in DN, not electrons):
    if (noiseMap) {
        psImage *rdVar = (psImage*)psBinaryOp(NULL, (const psPtr) noiseMap, "*", (const psPtr) noiseMap);
        readout->variance = (psImage*)psBinaryOp(readout->variance, readout->variance, "+", rdVar);
        psFree (rdVar);
    } else {
        readout->variance = (psImage*)psBinaryOp(readout->variance, readout->variance, "+", psScalarAlloc(readnoise*readnoise/gain/gain, PS_TYPE_F32));
    }

    return true;
}

// this function creates the variance pixels, or uses the existing variance pixels.  it will set
// the noise pixel values only if the variance image is not supplied
bool pmReadoutGenerateVariance(pmReadout *readout, const psImage *noiseMap, bool poisson)
{
    PS_ASSERT_PTR_NON_NULL(readout, false);

    pmCell *cell = readout->parent;     // The parent cell
    bool mdok = true;                   // Status of MD lookup

    // Create the variance image if required
    if (readout->variance)
        return true;

    psRegion *trimsec = psMetadataLookupPtr(&mdok, cell->concepts, "CELL.TRIMSEC"); // Trim section
    if (!mdok || psRegionIsNaN(*trimsec)) {
      // if trimsec is not defined, use the full image
      trimsec = psRegionAlloc(0,0,0,0);
      // psError(PS_ERR_IO, true, "CELL.TRIMSEC is not set --- unable to set variance.\n");
      // return false;
    }

    pmHDU *hdu = pmHDUFromCell(cell);   // The HDU containing the cell's pixels
    PS_ASSERT_PTR_NON_NULL(hdu, false);
    if (!hdu->images && !pmHDUGenerateForCell(cell)) {
        psError(PS_ERR_UNKNOWN, false, "Unable to generate HDU for cell.\n");
        return false;
    }

    createParentVariances(hdu);

    // Need to identify which readout we're working with....
    long index = identifyReadout(hdu, readout); // Index of the readout
    if (index == -1) {
        psError(PS_ERR_UNKNOWN, true, "Unable to identify readout image in HDU.\n");
        return false;
    }

    psImage *variance = psImageSubset(hdu->variances->data[index], *trimsec); // The variance pixels
    if (!variance) {
        psString trimsecString = psRegionToString(*trimsec);
        psError(PS_ERR_UNKNOWN, false, "Unable to set variance from HDU with trimsec: %s.\n",
                trimsecString);
        psFree(trimsecString);
        return false;
    }
    psImageInit(variance, 0);
    readout->variance = variance;

    return pmReadoutSetVariance(readout, noiseMap, poisson);
}

bool pmReadoutGenerateMaskVariance(pmReadout *readout, psImageMaskType satMask, psImageMaskType badMask, const psImage *noiseMap, bool poisson)
{
    PS_ASSERT_PTR_NON_NULL(readout, false);

    bool success = true;                // Was everything successful?

    success &= pmReadoutGenerateMask(readout, satMask, badMask);
    success &= pmReadoutGenerateVariance(readout, noiseMap, poisson);

    return success;
}

bool pmCellGenerateMaskVariance(pmCell *cell, psImageMaskType satMask, psImageMaskType badMask, const psImage *noiseMap, bool poisson)
{
    PS_ASSERT_PTR_NON_NULL(cell, false);

    bool success = true;                // Was everything successful?
    psArray *readouts = cell->readouts; // Array of readouts
    for (int i = 0; i < readouts->n; i++) {
        pmReadout *readout = readouts->data[i]; // The readout
        success &= pmReadoutGenerateMaskVariance(readout, satMask, badMask, noiseMap, poisson);
    }

    return success;
}


bool pmReadoutVarianceRenormalise(const pmReadout *readout, psImageMaskType maskVal,
                                  int sample, float minValid, float maxValid)
{
    PM_ASSERT_READOUT_NON_NULL(readout, false);
    PM_ASSERT_READOUT_IMAGE(readout, false);
    PM_ASSERT_READOUT_VARIANCE(readout, false);

    psImage *image = readout->image, *mask = readout->mask, *variance = readout->variance; // Readout parts
    int numCols = image->numCols, numRows = image->numRows; // Size of image

    int xMin, xMax, yMin, yMax;         // Bounds of image
    if (mask) {
        xMin = numCols;
        xMax = 0;
        yMin = numRows;
        yMax = 0;
        for (int y = 0; y < numRows; y++) {
            for (int x = 0; x < numCols; x++) {
                if (mask->data.PS_TYPE_IMAGE_MASK_DATA[y][x] & maskVal) {
                    continue;
                }
                xMin = PS_MIN(xMin, x);
                xMax = PS_MAX(xMax, x);
                yMin = PS_MIN(yMin, y);
                yMax = PS_MAX(yMax, y);
            }
        }
    } else {
        xMin = 0;
        xMax = numCols;
        yMin = 0;
        yMax = numRows;
    }

    int xNum = xMax - xMin, yNum = yMax - yMin; // Number of pixels

    int numPix = xNum * yNum;                                  // Number of pixels
    int num = PS_MIN(sample, numPix);                          // Number we care about
    psVector *signoise = psVectorAllocEmpty(num, PS_TYPE_F32);   // Signal-to-noise values

    if (num >= numPix) {
        // We have an image smaller than Nsubset, so just loop over the image pixels
        int index = 0;                  // Index for vector
        for (int y = yMin; y < yMax; y++) {
            for (int x = xMin; x < xMax; x++) {
                if ((mask && mask->data.PS_TYPE_IMAGE_MASK_DATA[y][x] & maskVal) ||
                    !isfinite(image->data.F32[y][x]) || !isfinite(variance->data.F32[y][x])) {
                    continue;
                }

                signoise->data.F32[index] = image->data.F32[y][x] / sqrtf(variance->data.F32[y][x]);
                index++;
            }
        }
        signoise->n = index;
    } else {
        psRandom *rng = psRandomAlloc(PS_RANDOM_TAUS); // Random number generator
        int index = 0;                  // Index for vector
        for (long i = 0; i < num; i++) {
            // Pixel coordinates
            int pixel = numPix * psRandomUniform(rng);
            int x = xMin + pixel % xNum;
            int y = yMin + pixel / xNum;

            if ((mask && mask->data.PS_TYPE_IMAGE_MASK_DATA[y][x] & maskVal) ||
                !isfinite(image->data.F32[y][x]) || !isfinite(variance->data.F32[y][x])) {
                continue;
            }

            signoise->data.F32[index] = image->data.F32[y][x] / sqrtf(variance->data.F32[y][x]);
            index++;
        }
        signoise->n = index;
        psFree(rng);
    }

    psStats *stats = psStatsAlloc(PS_STAT_ROBUST_STDEV); // Statistics

    if (!psVectorStats(stats, signoise, NULL, NULL, 0)) {
        psError(PS_ERR_UNKNOWN, false, "Unable to measure statistics on S/N image");
        psFree(signoise);
        return false;
    }
    psFree(signoise);

    float covar = sqrtf(psImageCovarianceFactor(readout->covariance)); // Covariance factor
    float correction = stats->robustStdev / covar; // Correction factor
    psFree(stats);
    psLogMsg("psModules.camera", PS_LOG_DETAIL, "Variance renormalisation factor is %f", correction);

    // Check valid range of correction factor
    if ((isfinite(minValid) && correction < minValid) || (isfinite(maxValid) && correction > maxValid)) {
	psError(PS_ERR_UNKNOWN, true, "Variance renormalisation is outside valid range: %f vs %f:%f --- no correction made", correction, minValid, maxValid);
	psMetadataAddF32(readout->analysis, PS_LIST_TAIL, PM_READOUT_ANALYSIS_RENORM, 0, "Renormalisation of variance", PS_SQR(correction));
        return false;
    }

    psImage *subImage = psImageSubset(variance, psRegionSet(xMin, xMax, yMin, yMax)); // Smaller image
    psBinaryOp(subImage, subImage, "*", psScalarAlloc(PS_SQR(correction), PS_TYPE_F32));
    psFree(subImage);

    pmHDU *hdu = pmHDUFromReadout(readout); // HDU for readout
    if (hdu)  {
        psString history = NULL;
        psStringAppend(&history, "Rescaled variance by %6.4f (stdev by %6.4f)",
                       PS_SQR(correction), correction);
        psMetadataAddStr(hdu->header, PS_LIST_TAIL, "HISTORY", PS_META_DUPLICATE_OK, NULL, history);
        psFree(history);
    }

    return psMetadataAddF32(readout->analysis, PS_LIST_TAIL, PM_READOUT_ANALYSIS_RENORM, 0,
                            "Renormalisation of variance", PS_SQR(correction));
}

// find any pixels which are not already masked (with maskTest) which are not valid and raise maskSet bits
bool pmReadoutMaskInvalid (const pmReadout *readout, psImageMaskType maskTest, psImageMaskType maskSet) {

    if (!readout) return true;

    psImage *image = readout->image;
    psImage *mask  = readout->mask;
    psImage *variance = readout->variance;
    for (int y = 0; y < image->numRows; y++) {
        for (int x = 0; x < image->numCols; x++) {
            if (mask->data.PS_TYPE_IMAGE_MASK_DATA[y][x] & maskTest) continue;
            bool valid = false;
            valid = isfinite(image->data.F32[y][x]);
            if (valid && variance) {
                valid &= isfinite(variance->data.F32[y][x]);
            }
            if (valid) continue;
            mask->data.PS_TYPE_IMAGE_MASK_DATA[y][x] |= maskSet;
        }
    }

    return true;
}

// raise maskVal for any invalid pixels
bool pmReadoutMaskNonfinite(pmReadout *readout, psImageMaskType maskVal)
{
    PM_ASSERT_READOUT_NON_NULL(readout, false);
    PM_ASSERT_READOUT_IMAGE(readout, false);

    psImage *image = readout->image;    // Readout's image
    psImage *variance = readout->variance;  // Readout's variance
    int numCols = image->numCols, numRows = image->numRows; // Size of image

    if (!readout->mask) {
        readout->mask = psImageAlloc(numCols, numRows, PS_TYPE_IMAGE_MASK);
    }
    psImage *mask = readout->mask;      // Readout's mask

    for (int y = 0; y < numRows; y++) {
        for (int x = 0; x < numCols; x++) {
            if (!isfinite(image->data.F32[y][x]) || (variance && !isfinite(variance->data.F32[y][x]))) {
                mask->data.PS_TYPE_IMAGE_MASK_DATA[y][x] |= maskVal;
            }
        }
    }

    return true;
}



bool pmReadoutMaskApply(pmReadout *readout, psImageMaskType maskVal)
{
    PM_ASSERT_READOUT_NON_NULL(readout, false);
    PM_ASSERT_READOUT_IMAGE(readout, false);
    PM_ASSERT_READOUT_MASK(readout, false);

    int numCols = readout->image->numCols, numRows = readout->image->numRows; // Size of image
    psImageMaskType **maskData = readout->mask->data.PS_TYPE_IMAGE_MASK_DATA; // Dereference mask
    psF32 **imageData = readout->image->data.F32;// Dereference image
    psF32 **varianceData = readout->variance ? readout->variance->data.F32 : NULL; // Dereference variance map
    float maskFrac = 0.0;
    for (int y = 0; y < numRows; y++) {
        for (int x = 0; x < numCols; x++) {
            if (maskData[y][x] & maskVal) {
  	        maskFrac += 1;
                imageData[y][x] = NAN;
                if (varianceData) {
                    varianceData[y][x] = NAN;
                }
            }
        }
    }
    maskFrac = maskFrac / (1.0 * numRows * numCols);
    psMetadataAddF32(readout->analysis, PS_LIST_TAIL, "READOUT.MASK.FRAC", PS_META_REPLACE,
		     "fraction of pixels masked by pmReadoutMaskApply",maskFrac);
    return true;
}


bool pmReadoutInterpolateBadPixels(pmReadout *readout, psImageMaskType maskVal, psImageInterpolateMode mode,
                                   float poorFrac, psImageMaskType maskPoor, psImageMaskType maskBad)
{
    PM_ASSERT_READOUT_NON_NULL(readout, false);
    PM_ASSERT_READOUT_IMAGE(readout, false);
    PM_ASSERT_READOUT_MASK(readout, false);
    if (!maskVal) {
        return true;
    }

    psImage *image = readout->image;    // Image
    psImage *mask = readout->mask;      // Mask
    psImage *variance = readout->variance;  // Variance map

    psImageInterpolation *interp = psImageInterpolationAlloc(mode, image, variance, mask, maskVal,
                                                             NAN, NAN, maskBad, maskPoor, poorFrac, 0);
    interp->shifting = false;           // Turn off "exact shifts" so we get proper interpolation

    int numCols = mask->numCols, numRows = mask->numRows; // Size of image

    psPixels *pixels = psPixelsAllocEmpty(PIXELS_BUFFER); // Pixels that have been interpolated
    psVector *imagePix = psVectorAllocEmpty(PIXELS_BUFFER, PS_TYPE_F32); // Corresponding values for image
    psVector *variancePix = psVectorAllocEmpty(PIXELS_BUFFER, PS_TYPE_F32); // Corresponding values for variance
    psVector *maskPix = psVectorAllocEmpty(PIXELS_BUFFER, PS_TYPE_IMAGE_MASK); // Corresponding values for mask
    // NOTE: maskPix carries the actual image mask values -- do NOT use
    // PS_TYPE_VECTOR_MASK here; it is storage, and is not treated as a vector mask

    long numBad = 0;                    // Number of bad pixels interpolated
    for (int y = 0; y < numRows; y++) {
        for (int x = 0; x < numCols; x++) {
            if (mask->data.PS_TYPE_IMAGE_MASK_DATA[y][x] & maskVal) {
                double imageValue, varianceValue; // Image and variance value from interpolation
                psImageMaskType maskValue = 0; // Mask value from interpolation

		// interpolate to pixel center (index + 0.5)
                psImageInterpolateStatus status = psImageInterpolate(&imageValue, &varianceValue, &maskValue, x + 0.5, y + 0.5, interp);
                if (status == PS_INTERPOLATE_STATUS_ERROR || status == PS_INTERPOLATE_STATUS_OFF) {
                    psError(PS_ERR_UNKNOWN, false, "Unable to interpolate readout at %d,%d", x, y);
                    psFree(interp);
                    psFree(pixels);
                    psFree(imagePix);
                    psFree(variancePix);
                    psFree(maskPix);
                    return false;
                }
                if (status == PS_INTERPOLATE_STATUS_BAD) {
                    // It's still bad: couldn't interpolate enough
                    continue;
                }

                pixels = psPixelsAdd(pixels, PIXELS_BUFFER, x, y);
                imagePix = psVectorExtend(imagePix, PIXELS_BUFFER, 1);
                variancePix = psVectorExtend(variancePix, PIXELS_BUFFER, 1);
                maskPix = psVectorExtend(maskPix, PIXELS_BUFFER, 1);
                imagePix->data.F32[numBad] = imageValue;
                variancePix->data.F32[numBad] = varianceValue;
                maskPix->data.PS_TYPE_IMAGE_MASK_DATA[numBad] = maskValue;
                numBad++;
            }
        }
    }

    psFree(interp);

    for (long i = 0; i < numBad; i++) {
        int x = pixels->data[i].x, y = pixels->data[i].y; // Coordinates of pixel
        image->data.F32[y][x] = imagePix->data.F32[i];
        variance->data.F32[y][x] = variancePix->data.F32[i];
        mask->data.PS_TYPE_IMAGE_MASK_DATA[y][x] = maskPix->data.PS_TYPE_IMAGE_MASK_DATA[i];
    }

    psFree(pixels);
    psFree(imagePix);
    psFree(variancePix);
    psFree(maskPix);

    psLogMsg("psModules.camera", PS_LOG_INFO, "Interpolated over %ld pixels", numBad);

    return true;
}
