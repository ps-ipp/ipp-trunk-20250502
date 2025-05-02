#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include "pmHDU.h"
#include "pmFPA.h"

#include "pmTrend2D.h"
#include "pmResiduals.h"
#include "pmGrowthCurve.h"
#include "pmSpan.h"
#include "pmFootprintSpans.h"
#include "pmFootprint.h"
#include "pmPeaks.h"
#include "pmMoments.h"
#include "pmModelFuncs.h"
#include "pmModelClass.h"
#include "pmModel.h"
#include "pmModelUtils.h"
#include "pmSourceMasks.h"
#include "pmSourceExtendedPars.h"
#include "pmSourceDiffStats.h"
#include "pmSourceSatstar.h"
#include "pmSourceLensing.h"
#include "pmSource.h"
#include "pmSourceFitModel.h"
#include "pmPSF.h"
#include "pmPSFtry.h"
#include "pmDetections.h"

#include "pmErrorCodes.h"

#include "pmSourceMatch.h"

#define SOURCE_MASK (PM_SOURCE_MODE_FAIL | PM_SOURCE_MODE_SATSTAR | PM_SOURCE_MODE_BLEND | \
                     PM_SOURCE_MODE_BADPSF | PM_SOURCE_MODE_DEFECT | PM_SOURCE_MODE_SATURATED | \
                     PM_SOURCE_MODE_CR_LIMIT | PM_SOURCE_MODE_EXT_LIMIT) // Mask to apply to input sources
#define SOURCE_FAINTEST 50.0            // Faintest magnitude to consider
#define SOURCE_BRIGHTEST -30.0          // Brightest magnitude to consider
#define SOURCES_MAX_LEAF 2              // Maximum number of points on a tree leaf
#define ARRAY_BUFFER 16                 // Buffer for array

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Private (file-static) functions
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Extract coordinates from a source
static inline void coordsFromSource(float *x, float *y, // Coordinates to return
                                    pmSource *source // Source of interest
    )
{
    if (!source) {
        *x = NAN;
        *y = NAN;
    } else if (source->modelPSF) {
        *x = source->modelPSF->params->data.F32[PM_PAR_XPOS];
        *y = source->modelPSF->params->data.F32[PM_PAR_YPOS];
    } else {
        *x = source->peak->xf;
        *y = source->peak->yf;
    }
    return;
}

// Parse the sources into vectors for each coordinate, and a bounding box
// Returns number of valid sources
static long sourcesParse(psRegion **bounds, // Region to update with bounding box
                         psVector **x, psVector **y, // Coordinate vectors to return
                         psVector **mag, psVector **magErr, // Magnitude and error vectors to return
                         psVector **indices, // Indices for sources
                         const psArray *sources // Input sources
    )
{
    psAssert(bounds, "Must be given a region for bounding box");
    psAssert(x && y, "Must be given position vectors");
    psAssert(mag && magErr, "Must be given magnitude vectors");
    psAssert(sources, "Must be given sources");

    long numSources = sources->n;              // Number of sources
    *x = psVectorRecycle(*x, numSources, PS_TYPE_F32);
    *y = psVectorRecycle(*y, numSources, PS_TYPE_F32);
    *mag = psVectorRecycle(*mag, numSources, PS_TYPE_F32);
    *magErr = psVectorRecycle(*magErr, numSources, PS_TYPE_F32);
    *indices = psVectorRecycle(*indices, numSources, PS_TYPE_S32);
    float xMin = INFINITY, xMax = -INFINITY, yMin = INFINITY, yMax = -INFINITY; // Bounds of sources
    long num = 0;                       // Number of valid sources
    for (long i = 0; i < numSources; i++) {
        pmSource *source = sources->data[i]; // Source of interest
        if (!source || (source->mode & SOURCE_MASK) || !isfinite(source->psfMag) ||
            !isfinite(source->psfMagErr) || source->psfMag > SOURCE_FAINTEST || source->psfMag < SOURCE_BRIGHTEST) {
            continue;
        }

        float xSrc, ySrc;               // Coordinates of source
        coordsFromSource(&xSrc, &ySrc, source);
        if (xSrc < xMin) xMin = xSrc;
        if (xSrc > xMax) xMax = xSrc;
        if (ySrc < yMin) yMin = ySrc;
        if (ySrc > yMax) yMax = ySrc;

        (*x)->data.F32[num] = xSrc;
        (*y)->data.F32[num] = ySrc;
        (*mag)->data.F32[num] = source->psfMag;
        (*magErr)->data.F32[num] = source->psfMagErr;
        (*indices)->data.S32[num] = i;
        num++;
    }
    (*x)->n = num;
    (*y)->n = num;
    (*mag)->n = num;
    (*magErr)->n = num;
    (*indices)->n = num;

    if (*bounds) {
        (*bounds)->x0 = xMin;
        (*bounds)->x1 = xMax;
        (*bounds)->y0 = yMin;
        (*bounds)->y1 = yMax;
    } else {
        *bounds = psRegionAlloc(xMin, xMax, yMin, yMax);
    }

    psTrace("psModules.objects", 8, "%ld sources: bounds are [%.2f:%.2f,%.2f:%.2f]\n",
            num, xMin, xMax, yMin, yMax);

    return num;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// pmSourceMatch operations
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void sourceMatchFree(pmSourceMatch *match)
{
    psFree(match->mag);
    psFree(match->magErr);
    psFree(match->x);
    psFree(match->y);
    psFree(match->image);
    psFree(match->index);
    psFree(match->mask);
}

pmSourceMatch *pmSourceMatchAlloc(void)
{
    pmSourceMatch *match = psAlloc(sizeof(pmSourceMatch)); // Match data
    psMemSetDeallocator(match, (psFreeFunc)sourceMatchFree);

    match->num = 0;
    match->mag = psVectorAllocEmpty(ARRAY_BUFFER, PS_TYPE_F32);
    match->magErr = psVectorAllocEmpty(ARRAY_BUFFER, PS_TYPE_F32);
    match->x = psVectorAllocEmpty(ARRAY_BUFFER, PS_TYPE_F32);
    match->y = psVectorAllocEmpty(ARRAY_BUFFER, PS_TYPE_F32);
    match->image = psVectorAllocEmpty(ARRAY_BUFFER, PS_TYPE_U32);
    match->index = psVectorAllocEmpty(ARRAY_BUFFER, PS_TYPE_U32);
    match->mask = psVectorAllocEmpty(ARRAY_BUFFER, PS_TYPE_VECTOR_MASK);

    return match;
}

void pmSourceMatchAdd(pmSourceMatch *match, // Match data
                      float mag, float magErr, // Magnitude and error
                      float x, float y,        // Position
                      int image, // Image index
                      int index // Source index
    )
{
    int num = match->num;               // Number of matches

    match->mag = psVectorExtend(match->mag, match->mag->nalloc, 1);
    match->magErr = psVectorExtend(match->magErr, match->magErr->nalloc, 1);
    match->x = psVectorExtend(match->x, match->x->nalloc, 1);
    match->y = psVectorExtend(match->y, match->y->nalloc, 1);
    match->image = psVectorExtend(match->image, match->image->nalloc, 1);
    match->index = psVectorExtend(match->index, match->index->nalloc, 1);
    match->mask = psVectorExtend(match->mask, match->mask->nalloc, 1);

    match->mag->data.F32[num] = mag;
    match->magErr->data.F32[num] = magErr;
    match->x->data.F32[num] = x;
    match->y->data.F32[num] = y;
    match->image->data.S32[num] = image;
    match->index->data.S32[num] = index;
    match->mask->data.PS_TYPE_VECTOR_MASK_DATA[num] = 0;
    match->num++;

    return;
}


psArray *pmSourceMatchSources(const psArray *sourceArrays, float radius, bool cullSingles)
{
    PS_ASSERT_ARRAY_NON_NULL(sourceArrays, NULL);
    PS_ASSERT_FLOAT_LARGER_THAN(radius, 0.0, NULL);

    int numImages = sourceArrays->n;    // Number of images

    // First, merge the source lists, pulling out matches

    psRegion *boundsMaster = NULL;       // Bounds of source list
    psVector *xMaster = NULL, *yMaster = NULL; // Coordinates of sources
    long numMaster = 0;                 // Number in master list
    psArray *matches = NULL;            // Source matches (potential and actual matches)

    for (int i = 0; i < numImages; i++) {
        psArray *sources = sourceArrays->data[i]; // Sources in image
        if (!sources || sources->n == 0) {
            continue;
        }
        psRegion *boundsImage = NULL;   // Bounds of source list
        psVector *xImage = NULL, *yImage = NULL; // Coordinates of sources
        psVector *magImage = NULL, *magErrImage = NULL; // Magnitude and mag
        psVector *indices = NULL;     // Indices for sources

        int numSources = sourcesParse(&boundsImage, &xImage, &yImage, &magImage, &magErrImage, &indices,
                                      sources); // Number of sources

        // an input image with only poor detections can cause trouble, remove it
        if (!numSources) continue;

        if (!boundsMaster) {
            // First run through --- can just copy
            boundsMaster = psMemIncrRefCounter(boundsImage);
            xMaster = psMemIncrRefCounter(xImage);
            yMaster = psMemIncrRefCounter(yImage);
            matches = psArrayAlloc(numSources);
            for (int j = 0; j < numSources; j++) {
                pmSourceMatch *match = pmSourceMatchAlloc(); // Match data
                pmSourceMatchAdd(match, magImage->data.F32[j], magErrImage->data.F32[j],
                                 xImage->data.F32[j], yImage->data.F32[j], i, indices->data.S32[j]);
                matches->data[j] = match;
            }
            numMaster += numSources;
        } else if (boundsImage->x0 > boundsMaster->x1 || boundsImage->x1 < boundsMaster->x0 ||
                   boundsImage->y0 > boundsMaster->y1 || boundsImage->y1 < boundsMaster->y0) {
            // Bounds don't overlap --- can just add everything in to the master list
            psTrace("psModules.objects", 7, "Bounds don't overlap\n");
            long size = numMaster + numSources; // New size
            xMaster = psVectorRealloc(xMaster, size);
            yMaster = psVectorRealloc(yMaster, size);
            matches = psArrayRealloc(matches, size);

            memcpy(&xMaster->data.F32[numMaster], xImage->data.F32,
                   numSources * PSELEMTYPE_SIZEOF(PS_TYPE_F32));
            memcpy(&yMaster->data.F32[numMaster], yImage->data.F32,
                   numSources * PSELEMTYPE_SIZEOF(PS_TYPE_F32));
            for (int j = 0, k = numMaster; j < numSources; j++, k++) {
                pmSourceMatch *match = pmSourceMatchAlloc(); // Match data
                pmSourceMatchAdd(match, magImage->data.F32[j], magErrImage->data.F32[j],
                                 xImage->data.F32[j], yImage->data.F32[j], i, indices->data.S32[j]);
                matches->data[k] = match;
            }

            numMaster = size;
            xMaster->n = size;
            yMaster->n = size;
            matches->n = size;
        } else {
            // Match with the master list
            psTree *tree = psTreePlant(2, SOURCES_MAX_LEAF, PS_TREE_EUCLIDEAN, xMaster, yMaster); // kd Tree
            long numMatch = 0;          // Number of matches

            long size = numMaster + numSources; // New size
            xMaster = psVectorRealloc(xMaster, size);
            yMaster = psVectorRealloc(yMaster, size);
            matches = psArrayRealloc(matches, size);

            psVector *coords = psVectorAlloc(2, PS_TYPE_F32); // Coordinates for tree lookup
            for (int j = 0; j < numSources; j++) {
                coords->data.F32[0] = xImage->data.F32[j];
                coords->data.F32[1] = yImage->data.F32[j];
                long index = psTreeNearestWithin(tree, coords, radius); // Match index
                if (index >= 0) {
                    // Record the match
                    pmSourceMatch *match = matches->data[index]; // Match data
                    pmSourceMatchAdd(match, magImage->data.F32[j], magErrImage->data.F32[j],
                                     xImage->data.F32[j], yImage->data.F32[j], i, indices->data.S32[j]);
                    numMatch++;
                } else {
                    // Add to the master list
                    pmSourceMatch *match = pmSourceMatchAlloc(); // Match data
                    pmSourceMatchAdd(match, magImage->data.F32[j], magErrImage->data.F32[j],
                                     xImage->data.F32[j], yImage->data.F32[j], i, indices->data.S32[j]);
                    xMaster->data.F32[numMaster] = xImage->data.F32[j];
                    yMaster->data.F32[numMaster] = yImage->data.F32[j];
                    matches->data[numMaster] = match;
                    numMaster++;
                    xMaster->n = yMaster->n = matches->n = numMaster;
                }

            }
            psFree(coords);
            psFree(tree);
        }

        psFree(boundsImage);
        psFree(xImage);
        psFree(yImage);
        psFree(magImage);
        psFree(magErrImage);
        psFree(indices);
    }

    psFree(xMaster);
    psFree(yMaster);
    psFree(boundsMaster);

    if (!matches) {
        psError(PM_ERR_OBJECTS, true, "No matches made.");
        return NULL;
    }

    if (cullSingles) {
        // Now cull the matches that contain only a single star
        int numGood = 0;                    // Number of good matches
        for (int i = 0; i < matches->n; i++) {
            pmSourceMatch *match = matches->data[i]; // Match of interest
            if (match->num > 1) {
                if (i != numGood) {
                    psFree(matches->data[numGood]);
                    matches->data[numGood] = psMemIncrRefCounter(match);
                }
                numGood++;
            }
        }
        matches->n = numGood;
        for (int i = numGood; i < numMaster; i++) {
            psFree(matches->data[i]);
            matches->data[i] = NULL;
        }
    }

    if (matches->n == 0) {
        psError(PM_ERR_OBJECTS, true, "No matches made.");
        psFree(matches);
        return NULL;
    }

    return matches;
}


psArray *pmSourceMatchMerge(psArray *sourceArrays, float radius, bool cullSingles)
{
    PS_ASSERT_ARRAY_NON_NULL(sourceArrays, NULL);
    PS_ASSERT_FLOAT_LARGER_THAN(radius, 0.0, NULL);

    // XXX MEH - ppSub should only get matched list so cull true. merge refers to the picking of the first entry data for match
    //psArray *matches = pmSourceMatchSources(sourceArrays, radius, false); // Source matches
    psArray *matches = pmSourceMatchSources(sourceArrays, radius, cullSingles); // Source matches
    if (!matches) {
        psError(psErrorCodeLast(), false, "Unable to match source lists.");
        return NULL;
    }

    int index = 0;                      // Index to current position on list
    for (int i = 0; i < matches->n; i++) {
        pmSource *source = NULL;        // Source to put in merged list
        pmSourceMatch *match = matches->data[i]; // Match of interest
        for (int j = 0; j < match->num && !source; j++) {
            if (!isfinite(match->mag->data.F32[j]) || !isfinite(match->magErr->data.F32[j]) ||
                !isfinite(match->x->data.F32[j]) || !isfinite(match->y->data.F32[j])) {
                continue;
            }
            int imgIndex = match->image->data.S32[j]; // Index of image
            int srcIndex = match->index->data.S32[j]; // Index of source for image
            psArray *list = sourceArrays->data[imgIndex]; // List of interest
            source = list->data[srcIndex];
            break;
        }

        if (source) {
            psFree(matches->data[index]);
            matches->data[index] = psMemIncrRefCounter(source);
            index++;
        }
    }

    // Clear out the rest of the list
    int num = index;                    // Number of good sources
    for (; index < matches->n; index++) {
        psFree(matches->data[index]);
        matches->data[index] = NULL;
    }
    matches->n = num;

    return matches;
}

// Iterate on the star magnitudes and image transparencies
// Returns the solution chi^2
static float sourceMatchRelphotIterate(psVector *trans, // Transparencies
                                       psVector *stars, // Star magnitudes
                                       psVector *badImage, // Bad image mask
                                       const psArray *matches, // Array of matches
                                       const psVector *zp, // Zero points for each image (incl. airmass term)
                                       const psVector *photo, // Photometric image?
                                       float sysErr2 // Systematic error, squared
    )
{
    psAssert(zp && zp->type.type == PS_TYPE_F32, "Need zero points");
    psAssert(matches, "Need list of matches");

    int numImages = zp->n;              // Number of images
    int numStars = matches->n;          // Number of stars

    psAssert(trans && trans->type.type == PS_TYPE_F32, "Need transparencies");
    psAssert(trans->n == numImages, "Not enough transparencies: %ld\n", trans->n);
    psAssert(stars && stars->type.type == PS_TYPE_F32, "Need star magnitudes");
    psAssert(stars->n == numStars, "Not enough stars: %ld\n", stars->n);
    psAssert(zp->n == numImages, "Not enough ZPs: %ld", zp->n);
    psAssert(!photo || photo->type.type == PS_TYPE_U8, "Photometric determination is wrong type");
    psAssert(!photo || photo->n == numImages, "Not enough photometric determinations: %ld", photo->n);

    // Solve the star magnitudes
    psVectorInit(stars, 0.0);
    int numGoodStars = 0;               // Number of stars with good measurements
    for (int i = 0; i < numStars; i++) {
        pmSourceMatch *match = matches->data[i]; // Matched stars
        int numMeasurements = 0;        // Number of unmasked measurements for star
        double star = 0.0, starErr = 0.0; // Accumulators for star
        for (int j = 0; j < match->num; j++) {
            if (match->mask->data.PS_TYPE_VECTOR_MASK_DATA[j] & PM_SOURCE_MATCH_MASK_PHOT) {
                continue;
            }
            numMeasurements++;
            int index = match->image->data.U32[j]; // Image index
            float mag = match->mag->data.F32[j]; // Measured magnitude
            double magErr2 = PS_SQR(match->magErr->data.F32[j]) + sysErr2; // Error in measured magnitude
            double invErr2 = 1.0 / magErr2; // Inverse square error
            float cal = zp->data.F32[index]; // Calibration to apply to image
            if (!photo || !photo->data.U8[index]) {
                cal -= trans->data.F32[index];
            }
            star += (mag + cal) * invErr2;
            starErr += invErr2;
        }
        if (numMeasurements > 1) {
            // It's only a good star (contributing to the chi^2) if there's more than 1 measurement
            numGoodStars++;
        }
        stars->data.F32[i] = star / starErr;
    }

    // Solve for the transparencies
    // We solve even for the "photometric" images since they may jump out of that status upon iteration
    psVector *accum = psVectorAlloc(numImages, PS_TYPE_F64); // Transparency accumulator
    psVector *accumErr = psVectorAlloc(numImages, PS_TYPE_F64); // Transparency accumulator
    psVectorInit(accum, 0.0);
    psVectorInit(accumErr, 0.0);
    for (int i = 0; i < numStars; i++) {
        pmSourceMatch *match = matches->data[i]; // Matched stars
        for (int j = 0; j < match->num; j++) {
            if (match->mask->data.PS_TYPE_VECTOR_MASK_DATA[j] & PM_SOURCE_MATCH_MASK_PHOT) {
                continue;
            }
            int index = match->image->data.U32[j]; // Image index
            float mag = match->mag->data.F32[j]; // Measured magnitude
            double magErr2 = PS_SQR(match->magErr->data.F32[j]) + sysErr2; // Error in measured magnitude
            double invErr2 = 1.0 / magErr2; // Inverse square error
            float cal = zp->data.F32[index]; // Calibration to apply to image
            accum->data.F64[index] += (mag + cal - stars->data.F32[i]) * invErr2;
            accumErr->data.F64[index] += invErr2;
        }
    }
    for (int i = 0; i < numImages; i++) {
        trans->data.F32[i] = accum->data.F64[i] / accumErr->data.F64[i];
        if (!isfinite(trans->data.F32[i])) {
            badImage->data.U8[i] = 0xFF;
        }
        psTrace("psModules.objects", 3, "Transparency for image %d: %f\n", i, trans->data.F32[i]);
    }
    psFree(accum);
    psFree(accumErr);

    // Once more through to evaluate chi^2
    float chi2 = 0.0;                   // chi^2 for iteration
    int dof = 0;                        // Degrees of freedom
    for (int i = 0; i < numStars; i++) {
        pmSourceMatch *match = matches->data[i]; // Matched stars
        for (int j = 0; j < match->num; j++) {
            if (match->mask->data.PS_TYPE_VECTOR_MASK_DATA[j] & PM_SOURCE_MATCH_MASK_PHOT) {
                continue;
            }
            int index = match->image->data.U32[j]; // Image index
            if (badImage->data.U8[index]) {
                continue;
            }
            float mag = match->mag->data.F32[j]; // Measured magnitude
            float magErr2 = PS_SQR(match->magErr->data.F32[j]) + sysErr2; // Error in measured magnitude
            float cal = zp->data.F32[index]; // Calibration to apply to image
            if (!photo || !photo->data.U8[index]) {
                cal -= trans->data.F32[index];
            }
            float dev2 = mag + cal - stars->data.F32[index]; // Deviation from fit
            if (isfinite(dev2)) {
                chi2 += PS_SQR(dev2) / magErr2;
                dof++;
            }
        }
    }
    dof -= numGoodStars + numImages;
    chi2 /= dof;

    return chi2;
}

// Determine which images are photometric, based on estimated transparencies
// Returns number of photometric images, or -1 on error
static int sourceMatchRelphotPhotometric(psVector *photo, // Photometric determination
                                         const psVector *trans, // Estimated transparencies
                                         const psVector *badImage, // Bad image?
                                         int transIter, // Iterations for transparency
                                         float transClip, // Clipping level for transparency
                                         float photoLevel // Level below which we declare photometric
                                         )
{
    psAssert(photo && photo->type.type == PS_TYPE_U8, "Need photometric determination");
    psAssert(trans && trans->type.type == PS_TYPE_F32, "Need transparencies");
    psAssert(badImage && badImage->type.type == PS_TYPE_U8, "Need bad image determination");

    int numImages = photo->n;              // Number of images

    psAssert(trans->n == numImages, "Not enough transparencies: %ld", trans->n);
    psAssert(badImage->n == numImages, "Not enough bad image determinations: %ld", badImage->n);
    psAssert(transIter >= 0, "Iterations for transparency must be non-negative: %d", transIter);
    psAssert(transClip > 0, "Clipping level for transparency must be positive: %f", transClip);
    psAssert(photoLevel > 0, "Photometric level must be positive: %f", photoLevel);

    psStats *stats = psStatsAlloc(PS_STAT_CLIPPED_MEAN | PS_STAT_CLIPPED_STDEV); // Statistics
    stats->clipIter = transIter;
    stats->clipSigma = transClip;

    if (!psVectorStats(stats, trans, NULL, badImage, 0xFF)) {
        psError(PS_ERR_UNKNOWN, false, "Unable to perform statistics on transparencies.");
        psFree(stats);
        return -1;
    }
    // XXX handle this case better:
    if (isnan(stats->clippedMean))  {
        psError(PS_ERR_UNKNOWN, false, "Unable to perform statistics on transparencies.");
        psFree(stats);
        return -1;
    }

    float thresh = stats->clippedMean + photoLevel * stats->clippedStdev; // Threshold for clouds
    psFree(stats);

    int numPhoto = 0;                   // Number of photometric images
    for (int i = 0; i < numImages; i++) {
        if (badImage->data.U8[i]) {
            continue;
        }
        if (trans->data.F32[i] < thresh) {
            photo->data.U8[i] = 0xFF;
            numPhoto++;
        } else {
            photo->data.U8[i] = 0;
        }
    }

    return numPhoto;
}


// Reject star measurements
// Returns the fraction of measurements that were rejected
static float sourceMatchRelphotReject(psVector *trans, // Transparencies
                                      psVector *stars, // Star magnitudes
                                      const psArray *matches, // Array of matches
                                      const psVector *zp, // Zero points for each image
                                      const psVector *photo, // Photometric image?
                                      const psVector *badImage, // Bad image?
                                      float starClip, // Clipping for stars
                                      float sysErr2 // Systematic error squared
                                )
{
    psAssert(zp && zp->type.type == PS_TYPE_F32, "Need zero points");
    psAssert(matches, "Need list of matches");

    int numImages = zp->n;              // Number of images
    int numStars = matches->n;          // Number of stars

    psAssert(trans && trans->type.type == PS_TYPE_F32, "Need transparencies");
    psAssert(trans->n == numImages, "Not enough transparencies: %ld\n", trans->n);
    psAssert(stars && stars->type.type == PS_TYPE_F32, "Need star magnitudes");
    psAssert(stars->n == numStars, "Not enough stars: %ld\n", stars->n);
    psAssert(zp->n == numImages, "Not enough ZPs: %ld", zp->n);
    psAssert(!photo || photo->type.type == PS_TYPE_U8, "Photometric determination is wrong type");
    psAssert(!photo || photo->n == numImages, "Not enough photometric determinations: %ld", photo->n);
    psAssert(!badImage || badImage->type.type == PS_TYPE_U8, "Photometric determination is wrong type");
    psAssert(!badImage || badImage->n == numImages, "Not enough bad determinations: %ld", badImage->n);

    starClip = PS_SQR(starClip);

    int numRejected = 0;                // Number rejected
    int numMeasurements = 0;            // Number of measurements
    for (int i = 0; i < numStars; i++) {
        pmSourceMatch *match = matches->data[i]; // Matched stars
        for (int j = 0; j < match->num; j++) {
            if (match->mask->data.PS_TYPE_VECTOR_MASK_DATA[j] & PM_SOURCE_MATCH_MASK_PHOT) {
                continue;
            }
            numMeasurements++;
            int index = match->image->data.U32[j]; // Image index
            if (badImage->data.U8[index]) {
                continue;
            }
            float mag = match->mag->data.F32[j]; // Measured magnitude
            float magErr = match->magErr->data.F32[j]; // Error in measured magnitude
            float cal = zp->data.F32[index]; // Calibration to apply to image
            if (!photo || !photo->data.U8[index]) {
                cal -= trans->data.F32[index];
            }
            float dev = mag + cal - stars->data.F32[i]; // Deviation

            // only reject detections from photometric images (non-photometric images can
            // have large errors.  XXX Or: allow a much higher rejection threshold
            if (photo->data.U8[index]) {
                if (PS_SQR(dev) > starClip * (PS_SQR(magErr) + sysErr2)) {
                    numRejected++;
                    match->mask->data.PS_TYPE_VECTOR_MASK_DATA[j] |= PM_SOURCE_MATCH_MASK_PHOT;
                }
            }
        }
    }

    return (float)numRejected / (float)numMeasurements;
}


psVector *pmSourceMatchRelphot(const psArray *matches, // Array of matches
			       psArray *matchedSources, // Array of averaged sources
                               const psVector *zp, // Zero points for each image (including airmass term)
                               float tol, // Relative tolerance for convergence
                               int iter1, // Number of iterations for pass 1
                               float rej1, // Limit on rejection between iterations for pass 1
                               float sys1, // Systematic error in measurements for pass 1
                               int iter2, // Number of iterations for pass 2
                               float rej2, // Limit on rejection between iterations for pass 2
                               float sys2, // Systematic error in measurements for pass 2
                               float rejLimit, // Limit on rejection between iterations
                               int transIter, // Clipping iterations for transparency
                               float transClip, // Clipping level for transparency
                               float photoLevel // Level at which we declare image is photometric
                               )
{
    PS_ASSERT_ARRAY_NON_NULL(matches, NULL);
    PS_ASSERT_VECTOR_NON_NULL(zp, NULL);
    PS_ASSERT_VECTOR_TYPE(zp, PS_TYPE_F32, NULL);
    PS_ASSERT_FLOAT_LARGER_THAN(transClip, 0.0, NULL);
    PS_ASSERT_ARRAY_NON_NULL(matchedSources, NULL);
    
    sys1 *= sys1;
    sys2 *= sys2;

    int numImages = zp->n;              // Number of images
    int numStars = matches->n;          // Number of stars
    psVector *badImage = psVectorAlloc(numImages, PS_TYPE_U8); // Bad image?
    psVectorInit(badImage, 0);

    // Check for data integrity
    {
        psVector *num = psVectorAlloc(numImages, PS_TYPE_S32); // Number of stars per image
        psVectorInit(num, 0);
        for (int i = 0; i < numStars; i++) {
            pmSourceMatch *match = matches->data[i]; // Matched stars
            for (int j = 0; j < match->num; j++) {
                int index = match->image->data.U32[j]; // Image index
                psAssert(index >= 0 && index < numImages, "Bad index: %d", index);
                num->data.S32[index]++;
            }
        }
        int numGood = 0;                // Number of good images
        for (int i = 0; i < numImages; i++) {
            if (num->data.S32[i] == 0 || !isfinite(zp->data.F32[i])) {
                badImage->data.U8[i] = 0xFF;
                continue;
            }
            numGood++;
        }
        psFree(num);
        if (numGood == 0) {
            psError(PM_ERR_DATA, true, "No images with good stars.");
            psFree(badImage);
            return false;
        }
    }

    psVector *trans = psVectorAlloc(numImages, PS_TYPE_F32); // Transparencies for each image, magnitudes
    psVectorInit(trans, 0.0);
    psVector *photo = psVectorAlloc(numImages, PS_TYPE_U8); // Photometric determination for each image
    psVectorInit(photo, 0);
    psVector *stars = psVectorAlloc(numStars, PS_TYPE_F32); // Magnitudes for each star

    float chi2 = sourceMatchRelphotIterate(trans, stars, badImage, matches, zp,
                                           photo, sys1); // chi^2 for solution
    psTrace("psModules.objects", 1, "Initial: chi^2 = %f\n", chi2);
    float lastChi2 = INFINITY;          // chi^2 on last iteration
    float fracRej = INFINITY;        // Fraction of measurements rejected

    // In the first passes, the transparencies are not well deteremined: use high systematic error and
    // rejection thresholds
    for (int i = 0; i < iter1; i++) {

        // Identify photometric nights
        int numPhoto = sourceMatchRelphotPhotometric(photo, trans, badImage, transIter, transClip,
                                                     photoLevel); // Number of photometric images
        if (numPhoto < 0) {
            psError(PS_ERR_UNKNOWN, false, "Unable to perform photometric determination");
            psFree(trans);
            psFree(photo);
            psFree(stars);
            return NULL;
        }
        psTrace("psModules.objects", 3, "Pass 1: Determined %d/%d are photometric\n", numPhoto, numImages);

        fracRej = sourceMatchRelphotReject(trans, stars, matches, zp, photo, badImage, rej1, sys1);
        psTrace("psModules.objects", 3, "Pass 1: %f%% of measurements rejected\n", fracRej * 100);

        chi2 = sourceMatchRelphotIterate(trans, stars, badImage, matches, zp, photo, sys1);
        psTrace("psModules.objects", 1, "Pass 1: iter = %d: chi^2 = %f rejected = %f\n", i, chi2, fracRej);
    }

    for (int i = 0; i < iter2 && (fabsf(lastChi2 - chi2) > tol * chi2 || fracRej > rejLimit); i++) {
        lastChi2 = chi2;

        // Identify photometric nights
        int numPhoto = sourceMatchRelphotPhotometric(photo, trans, badImage, transIter, transClip,
                                                     photoLevel); // Number of photometric images
        if (numPhoto < 0) {
            psError(PS_ERR_UNKNOWN, false, "Unable to perform photometric determination");
            psFree(trans);
            psFree(photo);
            psFree(stars);
            return NULL;
        }
        psTrace("psModules.objects", 3, "Pass 2: Determined %d/%d are photometric\n", numPhoto, numImages);

        fracRej = sourceMatchRelphotReject(trans, stars, matches, zp, photo, badImage, rej2, sys2);
        psTrace("psModules.objects", 3, "Pass 2: %f%% of measurements rejected\n", fracRej * 100);

        chi2 = sourceMatchRelphotIterate(trans, stars, badImage, matches, zp, photo, sys2);
        psTrace("psModules.objects", 1, "Pass 2: iter = %d: chi^2 = %f rejected = %f\n", i, chi2, fracRej);
    }

    for (int i = 0; i < matches->n; i++) {
      pmSource *source = pmSourceAlloc();
      source->psfMag = stars->data.F32[i];
      psArrayAdd(matchedSources,matchedSources->n,source);
    }
    
    psFree(photo);
    psFree(badImage);
    psFree(stars);

    if (fabsf(lastChi2 - chi2) > tol * chi2 || fracRej > rejLimit) {
        psWarning("Unable to converge to relphot solution (%f,%f)", (lastChi2 - chi2) / chi2, fracRej);
    }

    return trans;
}


// Iterate on the star positions and image shifts
// Returns the solution chi^2
static float sourceMatchRelastroIterate(psVector *xShift, psVector *yShift, // Shift for image
                                        psVector *xStar, psVector *yStar,   // Position for star
                                        const psArray *matches // Array of matches
    )
{
    psAssert(matches, "Need list of matches");

    int numImages = xShift->n;          // Number of images
    int numStars = matches->n;          // Number of stars

    psAssert(xShift && xShift->type.type == PS_TYPE_F32 && yShift && yShift->type.type == PS_TYPE_F32,
             "Need shifts");
    psAssert(yShift->n == numImages, "Not enough shifts: %ld\n", yShift->n);
    psAssert(xStar && xStar->type.type == PS_TYPE_F32 && yStar && yStar->type.type == PS_TYPE_F32,
             "Need star positions");
    psAssert(xStar->n == numStars && yStar->n == numStars, "Not enough stars\n");

    // Solve the star positions
    psVectorInit(xStar, NAN);
    psVectorInit(yStar, NAN);
    psVector *starMask = psVectorAlloc(numStars, PS_TYPE_U8); // Mask for stars
    psVectorInit(starMask, 0xFF);
    int numGoodStars = 0;               // Number of stars with good measurements
    for (int i = 0; i < numStars; i++) {
        pmSourceMatch *match = matches->data[i]; // Matched stars
        int numMeasurements = 0;        // Number of unmasked measurements for star
        double xSum = 0.0, ySum = 0.0;  // Accumulators for star
        for (int j = 0; j < match->num; j++) {
            if (match->mask->data.PS_TYPE_VECTOR_MASK_DATA[j] & PM_SOURCE_MATCH_MASK_ASTRO) {
                continue;
            }
            numMeasurements++;
            int index = match->image->data.U32[j]; // Image index

            xSum += match->x->data.F32[j] - xShift->data.F32[index];
            ySum += match->y->data.F32[j] - yShift->data.F32[index];
        }
        if (numMeasurements > 1) {
            // It's only a good star (contributing to the chi^2) if there's more than 1 measurement
            numGoodStars++;
            xStar->data.F32[i] = xSum / numMeasurements;
            yStar->data.F32[i] = ySum / numMeasurements;
            starMask->data.U8[i] = 0;
        }
    }

    // Solve for the shifts
    psVectorInit(xShift, 0.0);
    psVectorInit(yShift, 0.0);
    psVector *num = psVectorAlloc(numImages, PS_TYPE_S32);    // Number of stars
    psVectorInit(num, 0);
    for (int i = 0; i < numStars; i++) {
        if (starMask->data.U8[i]) {
            continue;
        }
        pmSourceMatch *match = matches->data[i]; // Matched stars
        for (int j = 0; j < match->num; j++) {
            if (match->mask->data.PS_TYPE_VECTOR_MASK_DATA[j] & PM_SOURCE_MATCH_MASK_ASTRO) {
                continue;
            }
            int index = match->image->data.U32[j]; // Image index

            xShift->data.F32[index] += match->x->data.F32[j] - xStar->data.F32[i];
            yShift->data.F32[index] += match->y->data.F32[j] - yStar->data.F32[i];
            num->data.S32[index]++;
        }
    }
    for (int i = 0; i < numImages; i++) {
        xShift->data.F32[i] /= num->data.S32[i];
        yShift->data.F32[i] /= num->data.S32[i];
        psTrace("psModules.objects", 3, "Shift for image %d: %f,%f\n",
                i, xShift->data.F32[i], yShift->data.F32[i]);
    }
    psFree(num);

    // Once more through to evaluate chi^2
    float chi2 = 0.0;                   // chi^2 for iteration
    int dof = 0;                        // Degrees of freedom
    for (int i = 0; i < numStars; i++) {
        pmSourceMatch *match = matches->data[i]; // Matched stars
        if (starMask->data.U8[i]) {
            continue;
        }
        for (int j = 0; j < match->num; j++) {
            if (match->mask->data.PS_TYPE_VECTOR_MASK_DATA[j]) {
                continue;
            }

            int index = match->image->data.U32[j]; // Image index
            float dx = match->x->data.F32[j] - xShift->data.F32[index] - xStar->data.F32[i];
            float dy = match->y->data.F32[j] - yShift->data.F32[index] - yStar->data.F32[i];

            chi2 += PS_SQR(dx) + PS_SQR(dy);
            dof++;
        }
    }
    dof -= numGoodStars + numImages;
    chi2 /= dof;

    return chi2;
}

// Reject star measurements
// Returns the fraction of measurements that were rejected
static float sourceMatchRelastroReject(const psVector *xShift, const psVector *yShift, // Shifts for each image
                                       const psVector *xStar, const psVector *yStar, // Positions for each star
                                       const psArray *matches, // Array of matches
                                       float chi2,             // chi^2 from fit
                                       float rej               // Rejection threshold
                                )
{
    psAssert(matches, "Need list of matches");

    int numImages = xShift->n;          // Number of images
    int numStars = matches->n;          // Number of stars

    psAssert(xShift && xShift->type.type == PS_TYPE_F32 && yShift && yShift->type.type == PS_TYPE_F32,
             "Need shifts");
    psAssert(yShift->n == numImages, "Not enough shifts: %ld\n", yShift->n);
    psAssert(xStar && xStar->type.type == PS_TYPE_F32 && yStar && yStar->type.type == PS_TYPE_F32,
             "Need star positions");
    psAssert(xStar->n == numStars && yStar->n == numStars, "Not enough stars\n");

    int numRejected = 0;                // Number rejected
    int numMeasurements = 0;            // Number of measurements

    float thresh = PS_SQR(rej) * chi2;    // Threshold for rejection

    for (int i = 0; i < numStars; i++) {
        pmSourceMatch *match = matches->data[i]; // Matched stars
        for (int j = 0; j < match->num; j++) {
            if (match->mask->data.PS_TYPE_VECTOR_MASK_DATA[j] & PM_SOURCE_MATCH_MASK_ASTRO) {
                continue;
            }
            numMeasurements++;
            int index = match->image->data.U32[j]; // Image index

            float dx = match->x->data.F32[j] - xShift->data.F32[index] - xStar->data.F32[i];
            float dy = match->y->data.F32[j] - yShift->data.F32[index] - yStar->data.F32[i];

            if (PS_SQR(dx) + PS_SQR(dy) > thresh) {
                numRejected++;
                match->mask->data.PS_TYPE_VECTOR_MASK_DATA[j] |= PM_SOURCE_MATCH_MASK_ASTRO;
            }
        }
    }

    return (float)numRejected / (float)numMeasurements;
}

psArray *pmSourceMatchRelastro(const psArray *matches, // Array of matches
                               int numImages,          // Number of images
                               float tol, // Relative tolerance for convergence
                               int iter1, // Number of iterations for pass 1
                               float rej1, // Limit on rejection between iterations for pass 1
                               int iter2, // Number of iterations for pass 2
                               float rej2, // Limit on rejection between iterations for pass 2
                               float rejLimit // Limit on rejection between iterations
    )
{
    PS_ASSERT_ARRAY_NON_NULL(matches, NULL);

    int numStars = matches->n;          // Number of stars
    psVector *xShift = psVectorAlloc(numImages, PS_TYPE_F32); // x shift for each image
    psVector *yShift = psVectorAlloc(numImages, PS_TYPE_F32); // y shift for each image
    psVectorInit(xShift, 0.0);
    psVectorInit(yShift, 0.0);
    psVector *xStar = psVectorAlloc(numStars, PS_TYPE_F32); // x position for each star
    psVector *yStar = psVectorAlloc(numStars, PS_TYPE_F32); // y position for each star

    float chi2 = sourceMatchRelastroIterate(xShift, yShift, xStar, yStar, matches); // chi^2 for solution
    psTrace("psModules.objects", 1, "Initial: chi^2 = %f\n", chi2);
    float lastChi2 = INFINITY;          // chi^2 on last iteration
    float fracRej = INFINITY;           // Fraction of measurements rejected

    // In the first passes, the shifts are not well deteremined: use high systematic error and
    // rejection thresholds
    for (int i = 0; i < iter1; i++) {
        fracRej = sourceMatchRelastroReject(xShift, yShift, xStar, yStar, matches, chi2, rej1);
        psTrace("psModules.objects", 3, "Pass 1: %f%% of measurements rejected\n", fracRej * 100);

        chi2 = sourceMatchRelastroIterate(xShift, yShift, xStar, yStar, matches);
        psTrace("psModules.objects", 1, "Pass 1: iter = %d: chi^2 = %f rejected = %f\n", i, chi2, fracRej);
    }

    for (int i = 0; i < iter2 && (fabsf(lastChi2 - chi2) > tol * chi2 || fracRej > rejLimit); i++) {
        lastChi2 = chi2;

        fracRej = sourceMatchRelastroReject(xShift, yShift, xStar, yStar, matches, chi2, rej2);
        psTrace("psModules.objects", 3, "Pass 2: %f%% of measurements rejected\n", fracRej * 100);

        chi2 = sourceMatchRelastroIterate(xShift, yShift, xStar, yStar, matches);
        psTrace("psModules.objects", 1, "Pass 2: iter = %d: chi^2 = %f rejected = %f\n", i, chi2, fracRej);
    }

    psFree(xStar);
    psFree(yStar);

    if (fabsf(lastChi2 - chi2) > tol * chi2 || fracRej > rejLimit) {
        psWarning("Unable to converge to relphot solution (%f,%f)", (lastChi2 - chi2) / chi2, fracRej);
    }

    psArray *results = psArrayAlloc(numImages); // Array of results
    for (int i = 0; i < numImages; i++) {
        psVector *offset = results->data[i] = psVectorAlloc(2, PS_TYPE_F32); // Offset for image
        offset->data.F32[0] = xShift->data.F32[i];
        offset->data.F32[1] = yShift->data.F32[i];
    }
    psFree(xShift);
    psFree(yShift);

    return results;
}

