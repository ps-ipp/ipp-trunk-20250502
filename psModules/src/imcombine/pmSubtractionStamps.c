#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <pslib.h>

#include "pmErrorCodes.h"
#include "pmHDU.h"
#include "pmFPA.h"

// All these includes required to get stamps out of an array of pmSources
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
#include "pmSourceMasks.h"
#include "pmSourceExtendedPars.h"
#include "pmSourceDiffStats.h"
#include "pmSourceSatstar.h"
#include "pmSourceLensing.h"
#include "pmSource.h"

#include "pmSubtractionTypes.h"
#include "pmSubtraction.h"
#include "pmSubtractionStamps.h"
#include "pmSubtractionVisual.h"

#define STAMP_LIST_BUFFER 20            // Number of stamps to add to list at a time

#define SOURCE_MASK (PM_SOURCE_MODE_FAIL | PM_SOURCE_MODE_SATSTAR | PM_SOURCE_MODE_BLEND | \
                     PM_SOURCE_MODE_BADPSF | PM_SOURCE_MODE_DEFECT | PM_SOURCE_MODE_SATURATED | \
                     PM_SOURCE_MODE_CR_LIMIT) // Mask for bad sources
#define SOURCE_FAINTEST 50.0            // Faintest magnitude to consider


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Private (file-static) functions
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Free function for pmSubtractionStampList
static void subtractionStampListFree(pmSubtractionStampList *list // Stamp list to free
                                     )
{
    psFree(list->stamps);
    psFree(list->regions);
    psFree(list->x);
    psFree(list->y);
    psFree(list->flux);
    psFree(list->window);
    psFree(list->window1);
    psFree(list->window2);
}

// Free function for pmSubtractionStamp
static void subtractionStampFree(pmSubtractionStamp *stamp // Stamp to free
                                 )
{
    psFree(stamp->image1);
    psFree(stamp->image2);
    psFree(stamp->weight);
    psFree(stamp->convolutions1);
    psFree(stamp->convolutions2);
    psFree(stamp->matrix);
    psFree(stamp->vector);
}

// Is this region OK?
static bool checkStampRegion(int x, int y, // Coordinates of stamp
                             const psRegion *region // Region of interest
                             )
{
    if (!region) {
        return true;
    }
    return (x < region->x0 || x > region->x1 || y < region->y0 || y > region->y1) ?
        false : true;
}


// Search a region for a suitable stamp
bool stampSearch(float *xStamp, float *yStamp, // Coordinates of stamp, to return
                 float *fluxStamp, // Flux of stamp, to return
                 const psImage *image1, const psImage *image2, // Images to search
                 float thresh1, float thresh2, // Thresholds for images
                 const psImage *subMask, // Subtraction mask
                 int xMin, int xMax, int yMin, int yMax, // Bounds of search
                 int numCols, int numRows, // Size of images
                 int border             // Border around image
    )
{
    bool found = false;                 // Found a suitable stamp?
    *fluxStamp = -INFINITY;             // Flux of best stamp

    // fprintf (stderr, "xMin, xMax: %d, %d -> ", xMin, xMax);
    // float xRaw = xMin;
    // float yRaw = yMin;

    // Ensure we're not going to go outside the bounds of the image
    xMin = PS_MAX(border, xMin);
    xMax = PS_MIN(numCols - border - 1, xMax);
    yMin = PS_MAX(border, yMin);
    yMax = PS_MIN(numRows - border - 1, yMax);

    if (xMax < xMin) {
	// fprintf (stderr, "%f,%f : x-border\n", xRaw, yRaw);
	return false;
    }
    if (yMax < yMin) {
	// fprintf (stderr, "%f,%f : y-border\n", xRaw, yRaw);
	return false;
    }

    psAssert (xMin <= xMax, "x mismatch?");
    psAssert (yMin <= yMax, "y mismatch?");

    for (int y = yMin; y <= yMax; y++) {
        for (int x = xMin; x <= xMax; x++) {
            if ((image1 && image1->data.F32[y][x] < thresh1) ||
                (image2 && image2->data.F32[y][x] < thresh2)) {
		// fprintf (stderr, "%f,%f : thresh\n", xRaw, yRaw);
                continue;
            }

            if (subMask && subMask->data.PS_TYPE_IMAGE_MASK_DATA[y][x] &
                (PM_SUBTRACTION_MASK_BORDER | PM_SUBTRACTION_MASK_REJ)) {
		// fprintf (stderr, "%f,%f : masked\n", xRaw, yRaw);
                return false;
            }

            // We take the MIN to attempt to avoid transients in both images
            float flux = (image1 && image2) ? PS_MIN(image1->data.F32[y][x], image2->data.F32[y][x]) :
                ((image1) ? image1->data.F32[y][x] : image2->data.F32[y][x]); // Flux at pixel
            if (flux > *fluxStamp) {
                *xStamp = x + 0.5;
                *yStamp = y + 0.5;
                *fluxStamp = flux;
                found = true;
            }
        }
    }

    // if (!found) {
    // 	fprintf (stderr, "%f,%f : fails flux test\n", xRaw, yRaw);
    // }

    return found;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Functions for generating ds9 region files
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

static bool ds9regions = false;         // Save ds9 region files?

void pmSubtractionRegions(bool state)
{
    ds9regions = state;
}

FILE *pmSubtractionStampsFile(const pmSubtractionStampList *stamps, const char *filename,
                              const char *description)
{
    if (!ds9regions || !stamps || !filename) {
        return NULL;
    }

    psLogMsg("psModules.imcombine", PS_LOG_INFO, "Writing %s to ds9 region file: %s",
             description, filename);

    FILE *file = fopen(filename, "w");

    // Outline the stamps
    for (int i = 0; i < stamps->num; i++) {
        psRegion *region = stamps->regions->data[i]; // Region of interest
        float xCentre = 0.5 * (region->x0 + region->x1), yCentre = 0.5 * (region->y0 + region->y1);
        fprintf(file, "image;box(%f,%f,%f,%f,0) # color=blue\nimage;text(%f,%f,{%d}) # color=blue\n",
                xCentre, yCentre, region->x1 - region->x0, region->y1 - region->y0,
                xCentre, yCentre, i);
    }

    return file;
}

void pmSubtractionStampPrint(FILE *ds9, float x, float y, float size, const char *color)
{
    if (!ds9regions || !ds9) {
        return;
    }
    fprintf(ds9, "image;circle(%f,%f,%f)", x, y, size);
    if (color && strlen(color) > 0) {
        fprintf(ds9, " # color=%s", color);
    }
    fprintf(ds9, "\n");
    return;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Public functions
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

pmSubtractionStampList *pmSubtractionStampListAlloc(int numCols, int numRows, const psRegion *region,
                                                    int footprint, float spacing, float normFrac,
                                                    float sysErr, float skyErr)
{
    pmSubtractionStampList *list = psAlloc(sizeof(pmSubtractionStampList)); // Stamp list to return
    psMemSetDeallocator(list, (psFreeFunc)subtractionStampListFree);

    // Get region in which to find stamps: [xMin:xMax,yMin:yMax]
    int xMin = 0, xMax = numCols, yMin = 0, yMax = numRows;
    if (region) {
        xMin = PS_MAX(region->x0, xMin);
        xMax = PS_MIN(region->x1, xMax);
        yMin = PS_MAX(region->y0, yMin);
        yMax = PS_MIN(region->y1, yMax);
    }
    int xSize = xMax - xMin, ySize = yMax - yMin; // Size of region of interest
    int xStamps = (float)xSize / spacing + 1, yStamps = (float)ySize / spacing + 1; // Number of stamps

    list->num = xStamps * yStamps;
    list->stamps = psArrayAlloc(list->num);
    list->regions = psArrayAlloc(list->num);

    for (int y = 0, index = 0; y < yStamps; y++) {
        int yStart = yMin + y * ((float)ySize / (float)(yStamps)); // Subregion starts here
        int yStop = yMin + (y + 1) * ((float)ySize / (float)(yStamps)) - 1; // Subregion stops here
        assert(yStart >= yMin && yStop < yMax);

        for (int x = 0; x < xStamps; x++, index++) {
            int xStart = xMin + x * ((float)xSize / (float)(xStamps)); // Subregion starts here
            int xStop = xMin + (x + 1) * ((float)xSize / (float)(xStamps)) - 1; // Subregion stops here
            assert(xStart >= xMin && xStop < xMax);

            list->stamps->data[index] = pmSubtractionStampAlloc();
            psTrace("psModules.imcombine", 6, "Stamp region %d: [%d:%d,%d:%d]\n",
                    index, xStart, xStop, yStart, yStop);
            list->regions->data[index] = psRegionAlloc(xStart, xStop, yStart, yStop);
        }
    }

    list->x = NULL;
    list->y = NULL;
    list->flux = NULL;
    list->normFrac = normFrac;
    list->normValue = NAN;
    list->window = NULL;
    list->window1 = NULL;
    list->window2 = NULL;
    list->normWindow1 = 0;
    list->normWindow2 = 0;
    list->footprint = footprint;
    list->sysErr = sysErr;
    list->skyErr = skyErr;

    return list;
}

pmSubtractionStampList *pmSubtractionStampListCopy(const pmSubtractionStampList *in)
{
    PM_ASSERT_SUBTRACTION_STAMP_LIST_NON_NULL(in, NULL);

    pmSubtractionStampList *out = psAlloc(sizeof(pmSubtractionStampList)); // Copied stamp list to return
    psMemSetDeallocator(out, (psFreeFunc)subtractionStampListFree);

    int num = out->num = in->num;       // Number of stamps
    out->stamps = psArrayAlloc(num);
    out->regions = psArrayAlloc(num);
    out->x = NULL;
    out->y = NULL;
    out->flux = NULL;
    out->window = psMemIncrRefCounter(in->window);
    out->window1 = psMemIncrRefCounter(in->window1);
    out->window2 = psMemIncrRefCounter(in->window2);
    out->footprint = in->footprint;
    out->normWindow1 = in->normWindow1;
    out->normWindow2 = in->normWindow2;

    for (int i = 0; i < num; i++) {
        psRegion *inRegion = in->regions->data[i]; // Input region
        out->regions->data[i] = psRegionAlloc(inRegion->x0, inRegion->x1, inRegion->y0, inRegion->y1);

        pmSubtractionStamp *inStamp = in->stamps->data[i]; // Input stamp
        pmSubtractionStamp *outStamp = psAlloc(sizeof(pmSubtractionStamp));
        psMemSetDeallocator(outStamp, (psFreeFunc)subtractionStampFree);
        outStamp->x = inStamp->x;
        outStamp->y = inStamp->y;
        outStamp->flux = inStamp->flux;
        outStamp->xNorm = inStamp->xNorm;
        outStamp->yNorm = inStamp->yNorm;
        outStamp->status = inStamp->status;

        outStamp->image1 = inStamp->image1 ? psKernelCopy(inStamp->image1) : NULL;
        outStamp->image2 = inStamp->image2 ? psKernelCopy(inStamp->image2) : NULL;
        outStamp->weight = inStamp->weight ? psKernelCopy(inStamp->weight) : NULL;

        if (inStamp->convolutions1) {
            int size = inStamp->convolutions1->n; // Size of array
            outStamp->convolutions1 = psArrayAlloc(size);
            for (int j = 0; j < size; j++) {
                psKernel *conv = inStamp->convolutions1->data[j]; // Convolution
                outStamp->convolutions1->data[j] = conv ? psKernelCopy(conv) : NULL;
            }
        } else {
            outStamp->convolutions1 = NULL;
        }
        if (inStamp->convolutions2) {
            int size = inStamp->convolutions2->n; // Size of array
            outStamp->convolutions2 = psArrayAlloc(size);
            for (int j = 0; j < size; j++) {
                psKernel *conv = inStamp->convolutions2->data[j]; // Convolution
                outStamp->convolutions2->data[j] = conv ? psKernelCopy(conv) : NULL;
            }
        } else {
            outStamp->convolutions2 = NULL;
        }

        outStamp->matrix = inStamp->matrix ? psImageCopy(NULL, inStamp->matrix, PS_TYPE_F64) : NULL;
        outStamp->vector = inStamp->vector ? psVectorCopy(NULL, inStamp->vector, PS_TYPE_F64) : NULL;

        out->stamps->data[i] = outStamp;
    }

    return out;
}

pmSubtractionStamp *pmSubtractionStampAlloc(void)
{
    pmSubtractionStamp *stamp = psAlloc(sizeof(pmSubtractionStamp)); // Stamp to return
    psMemSetDeallocator(stamp, (psFreeFunc)subtractionStampFree);

    stamp->x = NAN;
    stamp->y = NAN;
    stamp->flux = NAN;
    stamp->xNorm = NAN;
    stamp->yNorm = NAN;
    stamp->status = PM_SUBTRACTION_STAMP_INIT;

    stamp->image1 = NULL;
    stamp->image2 = NULL;
    stamp->weight = NULL;
    stamp->convolutions1 = NULL;
    stamp->convolutions2 = NULL;

    stamp->matrix = NULL;
    stamp->vector = NULL;
    stamp->norm = NAN;
    stamp->normI1 = NAN;
    stamp->normI2 = NAN;
    stamp->normSquare1 = NAN;
    stamp->normSquare2 = NAN;

    stamp->MxxI1 = NULL;
    stamp->MyyI1 = NULL;
    stamp->MxxI2 = NULL;
    stamp->MyyI2 = NULL;

    stamp->MxxI1raw = NAN;
    stamp->MyyI1raw = NAN;
    stamp->MxxI2raw = NAN;
    stamp->MyyI2raw = NAN;

    return stamp;
}

bool pmSubtractionStampsSelect(pmSubtractionStampList **stamps, // Stamps to read
			       const pmReadout *ro1, // Readout 1
			       const pmReadout *ro2, // Readout 2
			       const psImage *subMask, // Mask for subtraction, or NULL
			       psImage *variance,  // Variance map
			       const psRegion *region, // Region of interest
			       float thresh1,  // Threshold for stamp finding on readout 1
			       float thresh2,  // Threshold for stamp finding on readout 2
			       float stampSpacing, // Spacing between stamps
			       float normFrac,     // Fraction of flux in window for normalisation window
			       float sysError,     // Relative systematic error in images
			       float skyError,     // Relative systematic error in images
			       int size,         // Kernel half-size
			       int footprint,     // Convolution footprint for stamps
			       pmSubtractionMode mode // Mode for subtraction
    )
{
    PS_ASSERT_PTR_NON_NULL(stamps, false);
    PM_ASSERT_READOUT_NON_NULL(ro1, false);
    PM_ASSERT_READOUT_NON_NULL(ro2, false);
    PS_ASSERT_IMAGE_NON_NULL(subMask, false);
    PS_ASSERT_IMAGE_NON_NULL(variance, false);
    PS_ASSERT_PTR_NON_NULL(region, false);

    psTrace("psModules.imcombine", 3, "Finding stamps...\n");

    psImage *image1 = ro1 ? ro1->image : NULL, *image2 = ro2 ? ro2->image : NULL; // Images of interest

    *stamps = pmSubtractionStampsFind(*stamps, image1, image2, subMask, region, thresh1, thresh2,
                                      size, footprint, stampSpacing, normFrac, sysError, skyError, mode);
    if (!*stamps) {
        psError(psErrorCodeLast(), false, "Unable to find stamps.");
        return false;
    }

    psTrace("psModules.imcombine", 3, "Extracting stamps...\n");
    if (!pmSubtractionStampsExtract(*stamps, ro1->image, ro2->image, variance, size, *region)) {
        psError(psErrorCodeLast(), false, "Unable to extract stamps.");
        return false;
    }

    pmSubtractionVisualPlotStamps(*stamps, (pmReadout *) ro1);
    return true;
}

pmSubtractionStampList *pmSubtractionStampsFind(pmSubtractionStampList *stamps, 
						const psImage *image1,
                                                const psImage *image2, 
						const psImage *subMask,
                                                const psRegion *region, 
						float thresh1, 
						float thresh2,
                                                int size, 
						int footprint, 
						float spacing, 
						float normFrac,
                                                float sysErr, 
						float skyErr, 
						pmSubtractionMode mode)
{
    if (!image1 && !image2) {
        psError(PS_ERR_UNEXPECTED_NULL, true, "Must specify an image");
        return NULL;
    }
    int numCols = 0, numRows = 0;       // Size of images
    if (image1) {
        PS_ASSERT_IMAGE_NON_NULL(image1, NULL);
        PS_ASSERT_IMAGE_TYPE(image1, PS_TYPE_F32, NULL);
        if (subMask) {
            PS_ASSERT_IMAGES_SIZE_EQUAL(image1, subMask, NULL);
        }
        numCols = image1->numCols;
        numRows = image1->numRows;
    }
    if (image2) {
        PS_ASSERT_IMAGE_NON_NULL(image2, NULL);
        PS_ASSERT_IMAGE_TYPE(image2, PS_TYPE_F32, NULL);
        if (subMask) {
            PS_ASSERT_IMAGES_SIZE_EQUAL(image2, subMask, NULL);
        }
        numCols = image2->numCols;
        numRows = image2->numRows;
    }
    if (image1 && image2) {
        PS_ASSERT_IMAGES_SIZE_EQUAL(image1, image2, NULL);
    }
    if (subMask) {
        PS_ASSERT_IMAGE_NON_NULL(subMask, NULL);
        PS_ASSERT_IMAGE_TYPE(subMask, PS_TYPE_IMAGE_MASK, NULL);
        PS_ASSERT_IMAGE_SIZE(subMask, numCols, numRows, NULL);
    }
    PS_ASSERT_INT_POSITIVE(size, NULL);
    PS_ASSERT_INT_NONNEGATIVE(footprint, NULL);
    PS_ASSERT_FLOAT_LARGER_THAN(spacing, 0.0, NULL);
    if (region) {
        if (psRegionIsNaN(*region)) {
            psString string = psRegionToString(*region);
            psError(PM_ERR_PROG, true, "Input region (%s) contains NAN values", string);
            psFree(string);
            return false;
        }
        if (region->x0 < 0 || region->x1 > numCols ||
            region->y0 < 0 || region->y1 > numRows) {
            psString string = psRegionToString(*region);
            psError(PM_ERR_PROG, true, "Input region (%s) does not fit in image (%dx%d)",
                    string, numCols, numRows);
            psFree(string);
            return false;
        }
    }

    int border = size + footprint;      // Border size

    if (!stamps) {
        stamps = pmSubtractionStampListAlloc(numCols, numRows, region, footprint, spacing,
                                             normFrac, sysErr, skyErr);
    }

    // XXX TEST : dump all stars in the stamps here
    if (0) {
	FILE *f = fopen ("stamp.dat", "w");
	for (int i = 0; i < stamps->num; i++) {
	    psVector *xList = stamps->x->data[i];
	    psVector *yList = stamps->y->data[i]; // Coordinate lists
	    psVector *fluxList = stamps->flux->data[i]; // List of stamp fluxes

	    for (int j = 0; j < xList->n; j++) {
		fprintf (f, "%d %d  %f %f  %f\n", i, j, xList->data.F32[j], yList->data.F32[j], fluxList->data.F32[j]);
	    }
	}
	fclose (f);
    }

    int numStamps = stamps->num;        // Number of stamp regions
    int numFound = 0;                   // Number of stamps found
    int numSearch = 0;                  // Number of regions searched for new stamp
    int numGood = 0;                    // Number of good stamps in total
    for (int i = 0; i < numStamps; i++) {
        pmSubtractionStamp *stamp = stamps->stamps->data[i]; // Stamp of interest

        switch (stamp->status) {
          case PM_SUBTRACTION_STAMP_NONE:
            continue;
          case PM_SUBTRACTION_STAMP_FOUND:
          case PM_SUBTRACTION_STAMP_CALCULATE:
          case PM_SUBTRACTION_STAMP_USED:
            numGood++;
            continue;
          case PM_SUBTRACTION_STAMP_INIT:
          case PM_SUBTRACTION_STAMP_REJECTED:
            numSearch++;

            float xStamp = 0.0, yStamp = 0.0; // Coordinates of stamp
            float fluxStamp = -INFINITY; // Flux of stamp
            bool goodStamp = false;     // Found a good stamp?

            // A couple different ways of finding stamps:
            if (stamps->x && stamps->y) {
                // Get the next stamp from the list
                psVector *xList = stamps->x->data[i], *yList = stamps->y->data[i]; // Coordinate lists
                psVector *fluxList = stamps->flux->data[i]; // List of stamp fluxes

                // Take stamps off the top of the (sorted) list
                for (int j = xList->n - 1; j >= 0 && !goodStamp; j--) {
		    // fprintf (stderr, "%d : xList: %ld elements\n", i, xList->n);
                    // Chop off the top of the list
                    xList->n = j;
                    yList->n = j;
                    fluxList->n = j;

#if 0
                    // Fish around a bit to see if we can find a pixel that isn't masked
                    // This is not a good idea if we're using the window feature
                    psTrace("psModules.imcombine", 7, "Searching for stamp %d around %d,%d\n",
                            i, xCentre, yCentre);

                    // Search bounds
                    int xCentre = xList->data.F32[j] - 0.5, yCentre = yList->data.F32[j] - 0.5;// Stamp centre
                    int search = footprint - size; // Search radius
                    int xMin = PS_MAX(border, xCentre - search);
                    int xMax = PS_MIN(numCols - border -1, xCentre + search);
                    int yMin = PS_MAX(border, yCentre - search);
                    int yMax = PS_MIN(numRows - border - 1, yCentre + search);

                    goodStamp = stampSearch(&xStamp, &yStamp, &fluxStamp, image1, image2, thresh1, thresh2,
                                            subMask, xMin, xMax, yMin, yMax, numCols, numRows, border);
                    // fprintf (stderr, "find: %d %d ==> %5.1f %5.1f (\n", xCentre, yCentre, xStamp, yStamp);
#else
                    // Only search the exact centre pixel
                    goodStamp = stampSearch(&xStamp, &yStamp, &fluxStamp, image1, image2, thresh1, thresh2,
                                            subMask, xList->data.F32[j], xList->data.F32[j],
                                            yList->data.F32[j], yList->data.F32[j], numCols, numRows, border);
#endif
                }
            } else {
                // Use a simple method of automatically finding stamps --- take the highest pixel in the
                // subregion
                psRegion *subRegion = stamps->regions->data[i]; // Sub-region of interest

                goodStamp = stampSearch(&xStamp, &yStamp, &fluxStamp, image1, image2, thresh1, thresh2,
                                        subMask, subRegion->x0, subRegion->x1, subRegion->y0, subRegion->y1,
                                        numCols, numRows, border);
            }

            if (goodStamp) {
                stamp->x = xStamp;
                stamp->y = yStamp;
                stamp->flux = fluxStamp;

                // Reset the postage stamps since we're making a new stamp
                psFree(stamp->image1);
                psFree(stamp->image2);
                psFree(stamp->weight);
                psFree(stamp->convolutions1);
                psFree(stamp->convolutions2);
                stamp->image1 = stamp->image2 = stamp->weight = NULL;
                stamp->convolutions1 = stamp->convolutions2 = NULL;

                stamp->status = PM_SUBTRACTION_STAMP_FOUND;
                numFound++;
                psTrace("psModules.imcombine", 5, "Found stamp in subregion %d: %d,%d\n",
                        i, (int)stamp->x, (int)stamp->y);
            } else {
                stamp->status = PM_SUBTRACTION_STAMP_NONE;
            }
        }
    }

    if (numSearch > 0) {
        psLogMsg("psModules.imcombine", PS_LOG_INFO, "Found %d stamps", numFound);
    }

    if (numGood == 0 && numFound == 0) {
        // No good stamps
        psError(PM_ERR_STAMPS, true, "Unable to find suitable stamps");
        psFree(stamps);
        return NULL;
    }

    return stamps;
}



pmSubtractionStampList *pmSubtractionStampsSet(const psVector *x, const psVector *y,
                                               const psImage *image, const psImage *subMask,
                                               const psRegion *region, int size, int footprint,
                                               float spacing, float normFrac, float sysErr, float skyErr,
                                               pmSubtractionMode mode)

{
    PS_ASSERT_VECTOR_NON_NULL(x, NULL);
    PS_ASSERT_VECTOR_TYPE(x, PS_TYPE_F32, NULL);
    PS_ASSERT_VECTOR_NON_NULL(y, NULL);
    PS_ASSERT_VECTOR_TYPE(y, PS_TYPE_F32, NULL);
    PS_ASSERT_VECTORS_SIZE_EQUAL(y, x, NULL);
    PS_ASSERT_IMAGE_NON_NULL(image, NULL);
    if (subMask) {
        PS_ASSERT_IMAGE_NON_NULL(subMask, NULL);
        PS_ASSERT_IMAGE_TYPE(subMask, PS_TYPE_IMAGE_MASK, NULL);
        PS_ASSERT_IMAGES_SIZE_EQUAL(image, subMask, NULL);
    }
    PS_ASSERT_INT_POSITIVE(size, NULL);
    PS_ASSERT_INT_POSITIVE(footprint, NULL);
    PS_ASSERT_FLOAT_LARGER_THAN(spacing, 0.0, NULL);

    int numStars = x->n;                // Number of stars
    pmSubtractionStampList *stamps = pmSubtractionStampListAlloc(subMask->numCols, subMask->numRows,
                                                                 region, footprint, spacing,
                                                                 normFrac, sysErr, skyErr); // Stamp list
    int numStamps = stamps->num;        // Number of stamps

    psString ds9name = NULL;            // Filename for ds9 region file
    static int ds9num = 0;              // File number for ds9 region file
    psStringAppend(&ds9name, "stamps_all_%d.ds9", ds9num);
    FILE *ds9 = pmSubtractionStampsFile(stamps, ds9name, "all stamps"); // ds9 region file
    psFree(ds9name);
    ds9num++;

    // Initialise the lists
    stamps->x = psArrayAlloc(numStamps);
    stamps->y = psArrayAlloc(numStamps);
    stamps->flux = psArrayAlloc(numStamps);
    for (int i = 0; i < numStamps; i++) {
        stamps->x->data[i] = psVectorAllocEmpty(STAMP_LIST_BUFFER, PS_TYPE_F32);
        stamps->y->data[i] = psVectorAllocEmpty(STAMP_LIST_BUFFER, PS_TYPE_F32);
        stamps->flux->data[i] = psVectorAllocEmpty(STAMP_LIST_BUFFER, PS_TYPE_F32);
    }

    // Put the stars into their appropriate subregions
    for (int i = 0; i < numStars; i++) {
        float xStamp = x->data.F32[i], yStamp = y->data.F32[i]; // Coordinates of stamp
        int xPix = xStamp - 0.5, yPix = yStamp - 0.5; // Pixel coordinate of stamp
        if (!checkStampRegion(xPix, yPix, region)) {
            // It's not in the big region
            psTrace("psModules.imcombine", 9, "Rejecting input stamp (%d,%d) because outside region",
                    xPix, yPix);
            pmSubtractionStampPrint(ds9, xPix, yPix, footprint, "red");
            continue;
        }

        // fprintf (stderr, "stamp: %5.1f %5.1f == %d %d\n", xStamp, yStamp, xPix, yPix);

        bool found = false;
        for (int j = 0; j < numStamps && !found; j++) {
            psRegion *subRegion = stamps->regions->data[j]; // Subregion of interest
            if (checkStampRegion(xPix, yPix, subRegion)) {
                psVector *xList = stamps->x->data[j], *yList = stamps->y->data[j]; // Pixel lists
                psVector *fluxList = stamps->flux->data[j]; // Flux list

                int index = xList->n;   // Index of new stamp candidate

                psVectorExtend(xList, STAMP_LIST_BUFFER, 1);
                psVectorExtend(yList, STAMP_LIST_BUFFER, 1);
                psVectorExtend(fluxList, STAMP_LIST_BUFFER, 1);

                xList->data.F32[index] = xStamp;
                yList->data.F32[index] = yStamp;
                fluxList->data.F32[index] = image->data.F32[yPix][xPix];

                found = true;
                psTrace("psModules.imcombine", 9, "Putting input stamp (%d,%d) into subregion %d",
                        xPix, yPix, j);
                pmSubtractionStampPrint(ds9, xPix, yPix, footprint, "green");
            }
        }

        if (!found) {
            psTrace("psModules.imcombine", 9, "Unable to find subregion for stamp (%d,%d)",
                    xPix, yPix);
            pmSubtractionStampPrint(ds9, xPix, yPix, footprint, "yellow");
        }
    }

    if (ds9) {
        fclose(ds9);
    }

    int nTotal = 0;

    // Sort the list by flux, with the brightest last
    for (int i = 0; i < numStamps; i++) {
        psVector *xList = stamps->x->data[i], *yList = stamps->y->data[i]; // Pixel lists
        psVector *fluxList = stamps->flux->data[i]; // Flux list

        psVector *indexes = psVectorSortIndex(NULL, fluxList); // Indices to sort flux
        int num = indexes->n;           // Number of candidate stamps in this subregion

        psVector *xSorted = psVectorAlloc(num, PS_TYPE_F32); // Sorted version of x list
        psVector *ySorted = psVectorAlloc(num, PS_TYPE_F32); // Sorted version of y list
        psVector *fluxSorted = psVectorAlloc(num, PS_TYPE_F32); // Sorted version of flux list
        for (int j = 0; j < num; j++) {
            int k = indexes->data.S32[j]; // Sorted index
            xSorted->data.F32[j] = xList->data.F32[k];
            ySorted->data.F32[j] = yList->data.F32[k];
            fluxSorted->data.F32[j] = fluxList->data.F32[k];
        }
        psFree(indexes);

        psFree(stamps->x->data[i]);
        psFree(stamps->y->data[i]);
        psFree(stamps->flux->data[i]);

        stamps->x->data[i] = xSorted;
        stamps->y->data[i] = ySorted;
        stamps->flux->data[i] = fluxSorted;
	nTotal += num;
    }
    // fprintf (stderr, "nTotal %d\n", nTotal);
    
    return stamps;
}

// we are essentially using aperture photometry to determine the photometric match between the
// images.  we need to choose an appropriate-sized aperture for this analysis.  If it is too
// large, the measurement will be noisy (and possibly biased) due to the sky noise.  If it is
// too small, or inconsistent, the measurement will be biased.  We use Kron-mag like aperture
// scaled by the first radial moment.
bool pmSubtractionStampsGetWindow(bool *tryAgain, pmSubtractionStampList *stamps, int kernelSize)
{
    PM_ASSERT_SUBTRACTION_STAMP_LIST_NON_NULL(stamps, false);
    PS_ASSERT_INT_NONNEGATIVE(kernelSize, false);

    // if we succeed, or fail with an unrecoverable error, do not try again
    if (tryAgain) {
	*tryAgain = false;
    }

    int size = stamps->footprint; // Size of postage stamps

    // window for moments calculations downstream
    psFree (stamps->window);
    stamps->window = psKernelAlloc(-size, size, -size, size);
    psImageInit(stamps->window->image, 0.0);

    // window1 and window2 are mean stamp images used here to measure the 
    // first radial moment, and thus the normalization window
    psFree (stamps->window1);
    stamps->window1 = psKernelAlloc(-size, size, -size, size);
    psImageInit(stamps->window1->image, 0.0);

    psFree (stamps->window2);
    stamps->window2 = psKernelAlloc(-size, size, -size, size);
    psImageInit(stamps->window2->image, 0.0);

    // Generate an initial weighting window based on the fwhms (50% larger than the largest)
    float fwhm1, fwhm2;

    // XXX this is annoyingly hack-ish
    pmSubtractionGetFWHMs(&fwhm1, &fwhm2);
    
    float sigma = 1.5 * PS_MAX(fwhm1, fwhm2) / 2.35;
    
    for (int y = -size; y <= size; y++) {
	for (int x = -size; x <= size; x++) {
	    stamps->window->kernel[y][x] = exp(-0.5*(x*x + y*y)/(sigma*sigma));
	}
    }

    // generate normalizations for each stamp
    psVector *norm1 = psVectorAlloc(stamps->num, PS_TYPE_F32);
    psVector *norm2 = psVectorAlloc(stamps->num, PS_TYPE_F32);
    for (int i = 0; i < stamps->num; i++) {
        pmSubtractionStamp *stamp = stamps->stamps->data[i]; // Stamp of interest
        if (!stamp) continue;
        if (!stamp->image1) continue;
        if (!stamp->image2) continue;

        float sum1 = 0.0;
        float sum2 = 0.0;
        for (int y = -size; y <= size; y++) {
            for (int x = -size; x <= size; x++) {
                sum1 += stamp->image1->kernel[y][x];
                sum2 += stamp->image2->kernel[y][x];
            }
        }
        norm1->data.F32[i] = sum1;
        norm2->data.F32[i] = sum2;
    }

    // storage vector for flux data
    psStats *stats = psStatsAlloc (PS_STAT_SAMPLE_MEDIAN);
    psVector *flux1 = psVectorAllocEmpty(2*stamps->num, PS_TYPE_F32);
    psVector *flux2 = psVectorAllocEmpty(2*stamps->num, PS_TYPE_F32);

    // generate the window pixels
    double sum1 = 0.0;                   // Sum inside the window
    double sum2 = 0.0;                   // Sum inside the window
    float maxValue1 = 0.0;               // Maximum value, for normalisation
    float maxValue2 = 0.0;               // Maximum value, for normalisation
    for (int y = -size; y <= size; y++) {
        for (int x = -size; x <= size; x++) {

            flux1->n = 0;
            flux2->n = 0;
            for (int i = 0; i < stamps->num; i++) {
                pmSubtractionStamp *stamp = stamps->stamps->data[i]; // Stamp of interest
                if (!stamp) continue;
                if (!stamp->image1) continue;
                if (!stamp->image2) continue;

                psVectorAppend(flux1, stamp->image1->kernel[y][x] / norm1->data.F32[i]);
                psVectorAppend(flux2, stamp->image2->kernel[y][x] / norm2->data.F32[i]);
            }

            float f1 = NAN;
            if (flux1->n > 0) {
                psStatsInit (stats);
                if (!psVectorStats (stats, flux1, NULL, NULL, 0)) {
                    psAbort ("failed to generate stats");
                }
                f1 = stats->sampleMedian;
            }

            float f2 = NAN;
            if (flux2->n > 0) {
                psStatsInit (stats);
                if (!psVectorStats (stats, flux2, NULL, NULL, 0)) {
                    psAbort ("failed to generate stats");
                }
                f2 = stats->sampleMedian;
            }

            stamps->window1->kernel[y][x] = f1;
            if (PS_SQR(x) + PS_SQR(y) <= PS_SQR(size)) {
                sum1 += stamps->window1->kernel[y][x];
            }
            maxValue1 = PS_MAX(maxValue1, stamps->window1->kernel[y][x]);

            stamps->window2->kernel[y][x] = f2;
            if (PS_SQR(x) + PS_SQR(y) <= PS_SQR(size)) {
                sum2 += stamps->window2->kernel[y][x];
            }
            maxValue2 = PS_MAX(maxValue2, stamps->window2->kernel[y][x]);
        }
    }

#if 0
    {
	psFits *fits = NULL;
	fits = psFitsOpen ("window1.raw.fits", "w");
        psFitsWriteImage (fits, NULL, stamps->window1->image, 0, NULL);
        psFitsClose (fits);
        fits = psFitsOpen ("window2.raw.fits", "w");
        psFitsWriteImage (fits, NULL, stamps->window2->image, 0, NULL);
        psFitsClose (fits);
    }
#endif

    psTrace("psModules.imcombine", 3, "Window total (1): %f, threshold: %f\n", sum1, (1.0 - stamps->normFrac) * sum1);
    psTrace("psModules.imcombine", 3, "Window total (2): %f, threshold: %f\n", sum2, (1.0 - stamps->normFrac) * sum2);

    // attempt to calculate the normalization window based on the first radial moment
    double Sr1 = 0.0;
    double Sr2 = 0.0;
    double Sf1 = 0.0;
    double Sf2 = 0.0;
    for (int y = -size; y <= size; y++) {
	for (int x = -size; x <= size; x++) {
	    float r = hypot(x, y);
	    Sr1 += r * stamps->window1->kernel[y][x];
	    Sr2 += r * stamps->window2->kernel[y][x];
	    Sf1 += stamps->window1->kernel[y][x];
	    Sf2 += stamps->window2->kernel[y][x];
        }
    }
    
    float R1 = Sr1 / Sf1;
    float R2 = Sr2 / Sf2;

    if (!isfinite(R1) || !isfinite(R2)) {
        psError(PM_ERR_STAMPS, true, "Kron Radii are not finite (failure to converge).");
	psFree (stats);
	psFree (flux1);
	psFree (flux2);
	psFree (norm1);
	psFree (norm2);
        return false;
    }

    // Compare the Kron Radii (R1 & R2) to above to the FWHMs : if they are too discrepant, we will need to rescale
    psLogMsg ("psModules.imcombine", PS_LOG_DETAIL, "Kron Radii vs FWHMs 1: fwhm: %f, kron %f\n", fwhm1, R1);
    psLogMsg ("psModules.imcombine", PS_LOG_DETAIL, "Kron Radii vs FWHMs 2: fwhm: %f, kron %f\n", fwhm2, R2);

    // XXX CAREFUL : in pmSubtractionMatch.c:703, we rely on this factor of 2.75..
    stamps->normWindow1 = 2.75*R1;
    stamps->normWindow2 = 2.75*R2;
    psLogMsg ("psModules.imcombine", PS_LOG_DETAIL, "Windows from Kron Radii: %f for 1, %f for 2\n", stamps->normWindow1, stamps->normWindow2);


    // if the calculated normWindows are too large, we will fall off the stamps.  In this case, we need to try again.
    if ((stamps->normWindow1 > size) || (stamps->normWindow2 > size)) { 
	if (tryAgain) {
	    *tryAgain = true;
	}
	psFree (stats);
	psFree (flux1);
	psFree (flux2);
	psFree (norm1);
	psFree (norm2);
	return false; 
    }

    // this is an unrecoverable error : something really bogus in the data
    if (stamps->normWindow1 <= 0) {
        psError(PM_ERR_STAMPS, true, "Unable to determine normalisation window size (1).");
	psFree (stats);
	psFree (flux1);
	psFree (flux2);
	psFree (norm1);
	psFree (norm2);
        return false;
    }
    if (stamps->normWindow2 <= 0) {
        psError(PM_ERR_STAMPS, true, "Unable to determine normalisation window size (2).");
	psFree (stats);
	psFree (flux1);
	psFree (flux2);
	psFree (norm1);
	psFree (norm2);
        return false;
    }

    // Generate a weighting window based on the kron radii
    float radius = 2.0 * PS_MAX(R1, R2);
    psImageInit(stamps->window->image, 0.0);

    // we use a top-hat window for the moments analysis
    for (int y = -size; y <= size; y++) {
	for (int x = -size; x <= size; x++) {
	    if (hypot(x,y) > radius) continue;
	    stamps->window->kernel[y][x] = 1.0;
	}
    }

    // re-normalize so chisquare values are sensible
    for (int y = -size; y <= size; y++) {
        for (int x = -size; x <= size; x++) {
            stamps->window1->kernel[y][x] /= maxValue1;
        }
    }
    // re-normalize so chisquare values are sensible
    for (int y = -size; y <= size; y++) {
        for (int x = -size; x <= size; x++) {
            stamps->window2->kernel[y][x] /= maxValue2;
        }
    }

    psFree (stats);
    psFree (flux1);
    psFree (flux2);
    psFree (norm1);
    psFree (norm2);
    return true;
}

static pthread_mutex_t getPenaltiesMutex = PTHREAD_MUTEX_INITIALIZER;

// kernels->penalty is an overall scaling factor (user-supplied)
bool pmSubtractionKernelPenaltiesStamp(pmSubtractionStamp *stamp, pmSubtractionKernels *kernels)
{
    // we only need the penalties if we are doing dual convolution
    if (kernels->mode != PM_SUBTRACTION_MODE_DUAL) return true;

    // we only calculate the penalties once.
    if (kernels->havePenalties) return true;

    // in a threaded context, only one thread can calculate the penalties.  attempt to grab a
    // mutex before continuing
    pthread_mutex_lock(&getPenaltiesMutex);

    // did someone else already get the mutex and do this?
    if (kernels->havePenalties) {
	pthread_mutex_unlock(&getPenaltiesMutex);
	return true;
    }

    for (int i = 0; i < kernels->num; i++) {
	pmSubtractionKernelPenalties(stamp, kernels, i);
    }

    kernels->havePenalties = true;
    pthread_mutex_unlock(&getPenaltiesMutex);
    return true;
}

# define EMPIRICAL 0

// kernels->penalty is an overall scaling factor (user-supplied)
bool pmSubtractionKernelPenalties(pmSubtractionStamp *stamp, pmSubtractionKernels *kernels, int index)
{
    float penalty1, penalty2;
    float fwhm1, fwhm2;

    // XXX this is annoyingly hack-ish
    pmSubtractionGetFWHMs(&fwhm1, &fwhm2);

    bool zeroNull = false;
    int uOrder = kernels->u->data.S32[index];
    int vOrder = kernels->v->data.S32[index];
    if (uOrder % 2 == 0 && vOrder % 2 == 0) zeroNull = true;

    if (EMPIRICAL) {
	psKernel *convolution1 = stamp->convolutions1->data[index];
	penalty1 = pmSubtractionKernelPenaltySingle(convolution1, zeroNull);

	psKernel *convolution2 = stamp->convolutions2->data[index];
	penalty2 = pmSubtractionKernelPenaltySingle(convolution2, zeroNull);
    } else {
	pmSubtractionKernelPreCalc *kernel = kernels->preCalc->data[index];
	float M2 = pmSubtractionKernelPenaltySingle(kernel->kernel, zeroNull);

	if (1) {
	    penalty1 = M2 * PS_SQR(fwhm1 / 2.35); // rescale the unconvolved second-moment by the image second moment 
	    penalty2 = M2 * PS_SQR(fwhm2 / 2.35); // rescale the unconvolved second-moment by the image second moment 
	    // penalty1 = M2 + PS_SQR(fwhm1 / 2.35); // rescale the unconvolved second-moment by the image second moment 
	    // penalty2 = M2 + PS_SQR(fwhm2 / 2.35); // rescale the unconvolved second-moment by the image second moment 
	} else {
	    penalty1 = PS_SQR(fwhm1 / 2.35); // rescale the unconvolved second-moment by the image second moment 
	    penalty2 = PS_SQR(fwhm2 / 2.35); // rescale the unconvolved second-moment by the image second moment 
	}
    }
    kernels->penalties1->data.F32[index] = kernels->penalty * penalty1;
    psAssert (isfinite(kernels->penalties1->data.F32[index]), "invalid penalty");

    kernels->penalties2->data.F32[index] = kernels->penalty * penalty2;
    psAssert (isfinite(kernels->penalties2->data.F32[index]), "invalid penalty");

    // fprintf(stderr, "penalty1: %f, penalty2: %f\n", penalty1, penalty2);

    return true;
}

float pmSubtractionKernelPenaltySingle(psKernel *kernel, bool zeroNull)
{
    // Calculate moments
    double penalty = 0.0;                   // Moment, for penalty
    double sum = 0.0, sum2 = 0.0;           // Sum of kernel component
    float min = INFINITY, max = -INFINITY;  // Minimum and maximum kernel value
    for (int v = kernel->yMin; v <= kernel->yMax; v++) {
	for (int u = kernel->xMin; u <= kernel->xMax; u++) {
            double value = kernel->kernel[v][u];
	    if (false && zeroNull && (u == 0) && (v == 0)) {
		value += 1.0;
	    }
            double value2 = PS_SQR(value);
            sum += value;
            sum2 += value2;
            penalty += value2 * PS_SQR((PS_SQR(u) + PS_SQR(v)));
            min = PS_MIN(value, min);
            max = PS_MAX(value, max);
        }
    }
    penalty *= 1.0 / sum2;

    if (0) {
	// fprintf(stderr, "min: %lf, max: %lf, moment: %lf, flux^2: %lf\n", min, max, penalty, sum2);
	// psTrace("psModules.imcombine", 7, "Kernel %d: %f %d %d %f\n", index, fwhm, uOrder, vOrder, penalty);
    }

    return penalty;
}

bool pmSubtractionStampsExtract(pmSubtractionStampList *stamps, psImage *image1, psImage *image2,
                                psImage *variance, int kernelSize, psRegion bounds)
{
    PM_ASSERT_SUBTRACTION_STAMP_LIST_NON_NULL(stamps, false);
    PS_ASSERT_IMAGE_NON_NULL(image1, false);
    PS_ASSERT_IMAGE_TYPE(image1, PS_TYPE_F32, false);
    if (image2) {
        PS_ASSERT_IMAGE_NON_NULL(image2, false);
        PS_ASSERT_IMAGES_SIZE_EQUAL(image2, image1, false);
        PS_ASSERT_IMAGE_TYPE(image2, PS_TYPE_F32, false);
    }
    if (variance) {
        PS_ASSERT_IMAGE_NON_NULL(variance, false);
        PS_ASSERT_IMAGES_SIZE_EQUAL(variance, image1, false);
        PS_ASSERT_IMAGE_TYPE(variance, PS_TYPE_F32, false);
        PS_ASSERT_INT_NONNEGATIVE(kernelSize, false);
    }

    int size = kernelSize + stamps->footprint; // Size of postage stamps

    for (int i = 0; i < stamps->num; i++) {
        pmSubtractionStamp *stamp = stamps->stamps->data[i]; // Stamp of interest
        if (!stamp || stamp->status != PM_SUBTRACTION_STAMP_FOUND) {
            continue;
        }

        p_pmSubtractionPolynomialNormCoords(&stamp->xNorm, &stamp->yNorm, stamp->x, stamp->y,
                                            bounds.x0, bounds.x1, bounds.y0, bounds.y1);

        int x = stamp->x - 0.5, y = stamp->y - 0.5; // Stamp coordinates
        // fprintf (stderr, "stamp: %5.1f %5.1f == %d %d (size: %d)\n", stamp->x, stamp->y, x, y, size);

        if (x < bounds.x0 + size || x > bounds.x1 - size || y < bounds.y0 + size || y > bounds.y1 - size) {
	  psLogMsg("psModules.imcombine", 3, "Stamp %d (%d,%d) is within the region border.\n", i, x, y);
          stamp->status = PM_SUBTRACTION_STAMP_NONE;
	  continue;
        }

        // Catch memory leaks --- these should have been freed and NULLed before
        assert(stamp->image1 == NULL);
        assert(stamp->image2 == NULL);
        assert(stamp->weight == NULL);

        psRegion region = psRegionSet(x - size, x + size + 1, y - size, y + size + 1); // Region of interest

        psImage *sub1 = psImageSubset(image1, region); // Subimage with stamp
        stamp->image1 = psKernelAllocFromImage(sub1, size, size);
        psFree(sub1);                   // Drop reference

        if (image2) {
            psImage *sub2 = psImageSubset(image2, region); // Subimage with stamp
            stamp->image2 = psKernelAllocFromImage(sub2, size, size);
            psFree(sub2);               // Drop reference
        }

        psKernel *weight = stamp->weight = psKernelAlloc(-size, size, -size, size); // Weight = 1/variance
        if (variance) {
            psImage *varSub = psImageSubset(variance, region); // Subimage with stamp
            psKernel *var = psKernelAllocFromImage(varSub, size, size); // Variance postage stamp

            if ((isfinite(stamps->skyErr) && (stamps->skyErr > 0)) ||
                (isfinite(stamps->sysErr) && (stamps->sysErr > 0))) {
                float sysErr = 0.25 * PS_SQR(stamps->sysErr); // Systematic error
                float skyErr = stamps->skyErr;
                psKernel *image1 = stamp->image1, *image2 = stamp->image2; // Input images
                for (int y = -size; y <= size; y++) {
                    for (int x = -size; x <= size; x++) {
                        float additional = image1->kernel[y][x] + image2->kernel[y][x];
                        weight->kernel[y][x] = 1.0 / (skyErr + var->kernel[y][x] + sysErr * PS_SQR(additional));
                    }
                }
            } else {
                for (int y = -size; y <= size; y++) {
                    for (int x = -size; x <= size; x++) {
                        weight->kernel[y][x] = 1.0 / var->kernel[y][x];
                    }
                }
            }
            psFree(var);
            psFree(varSub);
        } else {
            psImageInit(weight->image, 1.0);
        }

        stamp->status = PM_SUBTRACTION_STAMP_CALCULATE;
    }

    return true;
}

pmSubtractionStampList *pmSubtractionStampsSetFromSources(const psArray *sources, const psImage *image,
                                                          const psImage *subMask, const psRegion *region,
                                                          int size, int footprint, float spacing,
                                                          float normFrac, float sysErr, float skyErr,
                                                          pmSubtractionMode mode)
{
    PS_ASSERT_ARRAY_NON_NULL(sources, NULL);
    // Let pmSubtractionStampsSet take care of the rest of the assertions

    int numIn = sources->n;             // Number of sources in input list

    psVector *x = psVectorAllocEmpty(numIn, PS_TYPE_F32); // x coordinates
    psVector *y = psVectorAllocEmpty(numIn, PS_TYPE_F32); // y coordinates

    int numOut = 0;                     // Number of sources in output list
    for (int i = 0; i < numIn; i++) {
        pmSource *source = sources->data[i]; // Source of interest
        if (!source || (source->mode & SOURCE_MASK) ||(source->psfMag > SOURCE_FAINTEST)) {
            continue;
        }
       
	// fprintf (stderr, "%f,%f : %f %f : %f %f\n", source->peak->xf, source->peak->yf,
	// source->psfMag, source->apMag, source->psfMag - source->apMag, source->psfMagErr);

	// XXX this is somewhat arbitrary...
	if (source->psfMagErr > 0.05) continue;
        if (isfinite(source->apMag)) {
            if (fabs(source->psfMag - source->apMag) > 0.5) continue;
        } else if (isfinite(source->apMagRaw)) {
            if (fabs(source->psfMag - source->apMagRaw) > 0.5) continue;
        } else {
            // XXX: Should we carry on or drop this source?
            // drop it for now
            continue;
        }

        if (source->modelPSF) {
            x->data.F32[numOut] = source->modelPSF->params->data.F32[PM_PAR_XPOS];
            y->data.F32[numOut] = source->modelPSF->params->data.F32[PM_PAR_YPOS];
        } else {
            x->data.F32[numOut] = source->peak->xf;
            y->data.F32[numOut] = source->peak->yf;
        }
        numOut++;
    }
    x->n = numOut;
    y->n = numOut;

    pmSubtractionStampList *stamps = pmSubtractionStampsSet(x, y, image, subMask, region, size,
                                                            footprint, spacing, normFrac,
                                                            sysErr, skyErr, mode); // Stamps
    psFree(x);
    psFree(y);

    if (!stamps) {
        psError(psErrorCodeLast(), false, "Unable to set stamps from sources.");
    }

    return stamps;
}


pmSubtractionStampList *pmSubtractionStampsSetFromFile(const char *filename, const psImage *image,
                                                       const psImage *subMask, const psRegion *region,
                                                       int size, int footprint, float spacing, float normFrac,
                                                       float sysErr, float skyErr, pmSubtractionMode mode)
{
    PS_ASSERT_STRING_NON_EMPTY(filename, NULL);
    // Let pmSubtractionStampsSet take care of the rest of the assertions

    psArray *data = psVectorsReadFromFile(filename, "%f %f");
    if (!data) {
        psError(psErrorCodeLast(), false, "Unable to read stamps file %s", filename);
        return NULL;
    }
    psVector *x = data->data[0], *y = data->data[1]; // Stamp positions

    // Correct for IRAF/FITS (unit-offset) positions to C (zero-offset) positions
    psBinaryOp(x, x, "-", psScalarAlloc(1.0, PS_TYPE_F32));
    psBinaryOp(y, y, "-", psScalarAlloc(1.0, PS_TYPE_F32));

    pmSubtractionStampList *stamps = pmSubtractionStampsSet(x, y, image, subMask, region, size, footprint,
                                                            spacing, normFrac, sysErr, skyErr, mode);
    psFree(data);

    return stamps;

}


bool pmSubtractionStampsResetStatus (pmSubtractionStampList *stamps) {

    for (int i = 0; i < stamps->num; i++) {
        pmSubtractionStamp *stamp = stamps->stamps->data[i]; // Stamp of interest
        if (!stamp) continue;
        if (stamp->status != PM_SUBTRACTION_STAMP_USED) continue;
        stamp->status = PM_SUBTRACTION_STAMP_CALCULATE;
    }
    return true;
}

