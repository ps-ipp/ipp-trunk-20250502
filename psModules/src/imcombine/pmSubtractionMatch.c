#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pslib.h>

#include "pmHDU.h"
#include "pmFPA.h"
#include "pmHDUUtils.h"
#include "pmSubtractionTypes.h"
#include "pmSubtraction.h"
#include "pmSubtractionParams.h"
#include "pmSubtractionKernels.h"
#include "pmSubtractionStamps.h"
#include "pmSubtractionEquation.h"
#include "pmSubtractionAnalysis.h"
#include "pmSubtractionMask.h"
#include "pmSubtractionThreads.h"
#include "pmSubtractionVisual.h"
#include "pmSubtractionSimple.h"
#include "pmErrorCodes.h"

#include "pmSubtractionMatch.h"

#define BG_STAT PS_STAT_ROBUST_MEDIAN   // Statistic to use for background

static bool useFFT = true;              // Do convolutions using FFT

//#define TESTING
//#define TESTING_MEMORY

// Output memory usage information
static void memCheck(const char *where)
{
#ifdef TESTING_MEMORY
    psMemBlock **leaks = NULL;
    int numLeaks = psMemCheckLeaks(0, &leaks, NULL, true);
    size_t largestSize = 0;
    psMemId largest = 0;
    size_t totalSize = 0;
    for (int i = 0; i < numLeaks; i++) {
        psMemBlock *mb = leaks[i];
        totalSize += mb->userMemorySize;
        if (mb->userMemorySize > largestSize) {
            largestSize = mb->userMemorySize;
            largest = mb->id;
        }
    }
    psFree(leaks);
    fprintf(stderr, "%s:\n", where);
    fprintf(stderr, "    Memory in use: %zd\n", totalSize);
    fprintf(stderr, "    Largest block: %ld\n", largest);
    //MEH -- osx may not like sbrk
    fprintf(stderr, "    sbrk(): %zd\n", (size_t)sbrk(0));
#endif
    return;
}

// Check input arguments
static bool subtractionMatchCheck(pmReadout *conv1, pmReadout *conv2, // Convolved images
                                  const pmReadout *ro1, const pmReadout *ro2, // Input images
                                  int stride, // Size for convolution patches
                                  float normFrac,           // Fraction of window for normalisation window
                                  float sysError,           // Systematic error in images
                                  float skyError,           // Systematic error in images
                                  float kernelError, // Systematic error in kernel
                                  float covarFrac,   // Fraction for kernel truncation before covariance
                                  psImageMaskType maskVal, // Value to mask for input
                                  psImageMaskType maskBad, // Mask for output bad pixels
                                  psImageMaskType maskPoor, // Mask for output poor pixels
                                  float poorFrac, // Fraction for "poor"
                                  float badFrac,   // Maximum fraction of bad input pixels to accept
                                  pmSubtractionMode subMode // Mode of subtraction
    )
{
    if (subMode != PM_SUBTRACTION_MODE_2) {
        PM_ASSERT_READOUT_NON_NULL(conv1, false);
        PM_ASSERT_READOUT_NON_NULL(ro1, false);
        PM_ASSERT_READOUT_IMAGE(ro1, false);
        if (conv1->image) {
            psFree(conv1->image);
            conv1->image = NULL;
        }
        if (conv1->mask) {
            psFree(conv1->mask);
            conv1->mask = NULL;
        }
        if (conv1->variance) {
            psFree(conv1->variance);
            conv1->variance = NULL;
        }
    }
    if (subMode != PM_SUBTRACTION_MODE_1) {
        PM_ASSERT_READOUT_NON_NULL(conv2, false);
        PM_ASSERT_READOUT_NON_NULL(ro2, false);
        PM_ASSERT_READOUT_IMAGE(ro2, false);
        if (conv2->image) {
            psFree(conv2->image);
            conv2->image = NULL;
        }
        if (conv2->mask) {
            psFree(conv2->mask);
            conv2->mask = NULL;
        }
        if (conv2->variance) {
            psFree(conv2->variance);
            conv2->variance = NULL;
        }
    }

    if (ro1 && ro2) {
        PS_ASSERT_IMAGES_SIZE_EQUAL(ro1->image, ro2->image, false);
    }
    PS_ASSERT_INT_NONNEGATIVE(stride, false);
    if (isfinite(normFrac)) {
        PS_ASSERT_FLOAT_LARGER_THAN(normFrac, 0.0, false);
        PS_ASSERT_FLOAT_LESS_THAN(normFrac, 1.0, false);
    }
    if (isfinite(sysError)) {
        PS_ASSERT_FLOAT_LARGER_THAN_OR_EQUAL(sysError, 0.0, false);
        PS_ASSERT_FLOAT_LESS_THAN(sysError, 1.0, false);
    }
    if (isfinite(skyError)) {
        PS_ASSERT_FLOAT_LARGER_THAN_OR_EQUAL(skyError, 0.0, false);
    }
    if (isfinite(kernelError)) {
        PS_ASSERT_FLOAT_LARGER_THAN_OR_EQUAL(kernelError, 0.0, false);
        PS_ASSERT_FLOAT_LESS_THAN(kernelError, 1.0, false);
    }
    PS_ASSERT_FLOAT_LARGER_THAN_OR_EQUAL(covarFrac, 0.0, false);
    PS_ASSERT_FLOAT_LESS_THAN(covarFrac, 1.0, false);
    // Don't care about maskVal
    // Don't care about maskBad
    // Don't care about maskPoor
    PS_ASSERT_FLOAT_LARGER_THAN(poorFrac, 0.0, false);
    PS_ASSERT_FLOAT_LESS_THAN_OR_EQUAL(poorFrac, 1.0, false);
    if (isfinite(badFrac)) {
        PS_ASSERT_FLOAT_LARGER_THAN(badFrac, 0.0, false);
        PS_ASSERT_FLOAT_LESS_THAN_OR_EQUAL(badFrac, 1.0, false);
    }

    return true;
}


/// Allocate images, as required
static void subtractionMatchAlloc(pmReadout *conv1, pmReadout *conv2, // Output readouts
                                  const pmReadout *ro1, const pmReadout *ro2, // Input readouts
                                  const psImage *subMask,                     // Subtraction mask
                                  psImageMaskType maskBad,                    // Mask value for bad pixels
                                  pmSubtractionMode subMode,          // Subtraction mode
                                  int numCols, int numRows            // Size of image
    )
{
    if (subMode == PM_SUBTRACTION_MODE_1 || 
	subMode == PM_SUBTRACTION_MODE_SINGLE_AUTO ||
        subMode == PM_SUBTRACTION_MODE_DUAL) {
        if (!conv1->image) {
            conv1->image = psImageAlloc(numCols, numRows, PS_TYPE_F32);
        }
        psImageInit(conv1->image, NAN);
        if (ro1->variance) {
            if (!conv1->variance) {
                conv1->variance = psImageAlloc(numCols, numRows, PS_TYPE_F32);
            }
            psImageInit(conv1->variance, NAN);
        }
        if (subMask) {
            if (!conv1->mask) {
                conv1->mask = psImageAlloc(numCols, numRows, PS_TYPE_IMAGE_MASK);
            }
            psImageInit(conv1->mask, maskBad);
        }
    }
    if (subMode == PM_SUBTRACTION_MODE_2 || 
	subMode == PM_SUBTRACTION_MODE_SINGLE_AUTO ||
        subMode == PM_SUBTRACTION_MODE_DUAL) {
        if (!conv2->image) {
            conv2->image = psImageAlloc(numCols, numRows, PS_TYPE_F32);
        }
        psImageInit(conv2->image, NAN);
        if (ro2->variance) {
            if (!conv2->variance) {
                conv2->variance = psImageAlloc(numCols, numRows, PS_TYPE_F32);
            }
            psImageInit(conv2->variance, NAN);
        }
        if (subMask) {
            if (!conv2->mask) {
                conv2->mask = psImageAlloc(numCols, numRows, PS_TYPE_IMAGE_MASK);
            }
            psImageInit(conv2->mask, maskBad);
        }
    }

    return;
}


static void subtractionAnalysisUpdate(pmReadout *conv1, pmReadout *conv2, // Convolved images
                                      const psMetadata *analysis, // Analysis metadata
                                      const psMetadata *header // Header metadata
    )
{
    if (conv1) {
        conv1->analysis = psMetadataCopy(conv1->analysis, analysis);
    }
    if (conv2) {
        conv2->analysis = psMetadataCopy(conv2->analysis, analysis);
    }

    if (conv1 && conv1->parent) {
        pmHDU *hdu = pmHDUFromCell(conv1->parent);
        if (hdu) {
            hdu->header = psMetadataCopy(hdu->header, header);
        }
    }
    if (conv2 && conv2->parent) {
        pmHDU *hdu = pmHDUFromCell(conv2->parent);
        if (hdu) {
            hdu->header = psMetadataCopy(hdu->header, header);
        }
    }

    return;
}

// bool pmSubtractionMaskInvalid (const pmReadout *readout, psImageMaskType maskVal) {
// 
//     if (!readout) return true;
// 
//     psImage *image = readout->image;
//     psImage *mask  = readout->mask;
//     psImage *variance = readout->variance;
//     for (int y = 0; y < image->numRows; y++) {
//         for (int x = 0; x < image->numCols; x++) {
//             if (mask->data.PS_TYPE_IMAGE_MASK_DATA[y][x] & maskVal) continue;
//             bool valid = false;
//             valid = isfinite(image->data.F32[y][x]);
//             if (variance) {
//                 valid &= isfinite(variance->data.F32[y][x]);
//             }
//             if (valid) continue;
//             mask->data.PS_TYPE_IMAGE_MASK_DATA[y][x] = maskVal;
//         }
//     }
// 
//     return true;
// }

bool pmSubtractionMatchPrecalc(pmReadout *conv1, pmReadout *conv2, const pmReadout *ro1, const pmReadout *ro2,
                               psMetadata *analysis, int stride, float kernelError, float covarFrac,
                               psImageMaskType maskVal, psImageMaskType maskBad, psImageMaskType maskPoor,
                               float poorFrac, float badFrac)
{
    PS_ASSERT_METADATA_NON_NULL(analysis, false);

    // Extract the kernels
    pmSubtractionMode mode = PM_SUBTRACTION_MODE_UNSURE; // Subtraction mode: which image to convolve
    int size = 0;                       // Size of kernel
    psList *kernelList = psListAlloc(NULL); // List of kernels
    {
        psMetadataIterator *iter = psMetadataIteratorAlloc(analysis, PS_LIST_HEAD,
                                                           "^" PM_SUBTRACTION_ANALYSIS_KERNEL "$");
        psMetadataItem *item;               // Item from iteration
        while ((item = psMetadataGetAndIncrement(iter))) {
            if (item->type != PS_DATA_UNKNOWN) {
                psError(PM_ERR_PROG, true, "Unexpected type for kernel.");
                psFree(iter);
                psFree(kernelList);
                return false;
            }
            pmSubtractionKernels *kernel = item->data.V; // Kernel
            PM_ASSERT_SUBTRACTION_KERNELS_NON_NULL(kernel, false);
            size = PS_MAX(size, kernel->size);
            if (mode == PM_SUBTRACTION_MODE_UNSURE) {
                mode = kernel->mode;
            } else if (kernel->mode != mode) {
                // There's some confusion, so let's set the mode to dual convolution.
                // This is only used for the subtraction mask, so it's not a big deal.
                mode = PM_SUBTRACTION_MODE_DUAL;
            }
            psListAdd(kernelList, PS_LIST_TAIL, kernel);
        }
        psFree(iter);
    }
    if (psListLength(kernelList) == 0) {
        psError(PM_ERR_PROG, true, "Unable to find kernels");
        psFree(kernelList);
        return false;
    }
    psArray *kernels = psListToArray(kernelList); // Array of kernels
    psFree(kernelList);

    // Extract the regions
    psArray *regions = psArrayAllocEmpty(kernels->n); // Array of regions
    {
        psMetadataIterator *iter = psMetadataIteratorAlloc(analysis, PS_LIST_HEAD,
                                                           "^" PM_SUBTRACTION_ANALYSIS_REGION "$");
        psMetadataItem *item;               // Item from iteration
        while ((item = psMetadataGetAndIncrement(iter))) {
            if (item->type != PS_DATA_REGION) {
                psError(PM_ERR_PROG, true, "Unexpected type for region.");
                psFree(iter);
                psFree(kernels);
                psFree(regions);
                return false;
            }
            psRegion *region = item->data.V; // Region
            psArrayAdd(regions, regions->n, region);
        }
        psFree(iter);
    }
    if (regions->n != kernels->n) {
        psError(PM_ERR_PROG, true, "Differing number of kernels (%ld) and regions (%ld)",
                kernels->n, regions->n);
        psFree(regions);
        psFree(kernels);
        return false;
    }

    if (!subtractionMatchCheck(conv1, conv2, ro1, ro2, stride, NAN, NAN, NAN, kernelError, covarFrac,
                               maskVal, maskBad, maskPoor, poorFrac, badFrac, mode)) {
        psFree(kernels);
        psFree(regions);
        return false;
    }

    int numCols, numRows;       // Size of image
    if (ro1) {
        numCols = ro1->image->numCols;
        numRows = ro1->image->numRows;
    } else if (ro2) {
        numCols = ro2->image->numCols;
        numRows = ro2->image->numRows;
    } else {
        psAbort("No input image provided.");
    }

    // XXX this is done before calling this function
    // pmSubtractionMaskInvalid(ro1, maskVal);
    // pmSubtractionMaskInvalid(ro2, maskVal);

    // General background subtraction, since this is done in pmSubtractionMatch
    {
        psRandom *rng = psRandomAlloc(PS_RANDOM_TAUS); // Random number generator
        psStats *bg = psStatsAlloc(PS_STAT_ROBUST_MEDIAN); // Statistics for background
        if (ro1) {
            psStatsInit(bg);
            if (!psImageBackground(bg, NULL, ro1->image, ro1->mask, maskVal, rng)) {
                psError(PM_ERR_DATA, false, "Unable to measure background statistics.");
                psFree(bg);
                psFree(rng);
                return false;
            }
            psBinaryOp(ro1->image, ro1->image, "-", psScalarAlloc((float)bg->robustMedian, PS_TYPE_F32));
        }
        if (ro2) {
            psStatsInit(bg);
            if (!psImageBackground(bg, NULL, ro2->image, ro2->mask, maskVal, rng)) {
                psError(PM_ERR_DATA, false, "Unable to measure background statistics.");
                psFree(bg);
                psFree(rng);
                return false;
            }
            psBinaryOp(ro2->image, ro2->image, "-", psScalarAlloc((float)bg->robustMedian, PS_TYPE_F32));
        }
        psFree(bg);
        psFree(rng);
    }

    psRegion bounds = psRegionSet(NAN, NAN, NAN, NAN); // Bounds of valid pixels

    psImage *subMask = pmSubtractionMask(&bounds, ro1, ro2, maskVal, size, 0,
                                         badFrac, mode); // Subtraction mask
    if (!subMask) {
        psError(psErrorCodeLast(), false, "Unable to generate subtraction mask.");
        psFree(kernels);
        psFree(regions);
        return false;
    }

    psMetadata *outAnalysis = psMetadataAlloc(); // Output analysis values
    psMetadata *outHeader = psMetadataAlloc(); // Output header values

    subtractionMatchAlloc(conv1, conv2, ro1, ro2, subMask, maskBad, mode, numCols, numRows);

    psTrace("psModules.imcombine", 2, "Convolving...\n");
    for (int i = 0; i < kernels->n; i++) {
        pmSubtractionKernels *kernel = kernels->data[i]; // Kernel of interest
        psRegion *region = regions->data[i]; // Region of interest

        if (!pmSubtractionAnalysis(outAnalysis, outHeader, kernel, region, numCols, numRows)) {
            psError(psErrorCodeLast(), false, "Unable to generate QA data");
            psFree(outAnalysis);
            psFree(outHeader);
            psFree(subMask);
            psFree(kernels);
            psFree(regions);
            return false;
        }

        if (!pmSubtractionConvolve(conv1, conv2, ro1, ro2, subMask, stride, maskBad, maskPoor, poorFrac,
                                   kernelError, covarFrac, region, kernel, true, useFFT)) {
            psError(psErrorCodeLast(), false, "Unable to convolve image.");
            psFree(outAnalysis);
            psFree(outHeader);
            psFree(subMask);
            psFree(kernels);
            psFree(regions);
            return false;
        }
    }

    psFree(subMask);
    psFree(kernels);
    psFree(regions);

    subtractionAnalysisUpdate(conv1, conv2, outAnalysis, outHeader);
    psFree(outAnalysis);
    psFree(outHeader);

    return true;
}

bool pmSubtractionMatchAttempt(pmSubtractionQuality **bestMatch, pmSubtractionKernels *kernels, pmSubtractionStampList *stamps, pmSubtractionMode mode, int spatialOrder, bool final) {

    pmSubtractionMode nativeMode = kernels->mode;
    pmSubtractionMode nativeOrder = kernels->spatialOrder;

    kernels->mode = mode;
    kernels->spatialOrder = spatialOrder;

    // we always need to recalculate the matrix equation elements...
    pmSubtractionStampsResetStatus(stamps);

    psTrace("psModules.imcombine", 3, "Convolving stamps as needed...\n");
    if (!pmSubtractionConvolveStamps(stamps, kernels)) {
	psError(psErrorCodeLast(), false, "Unable to convolve stamps.");
	return false;
    }

    // step 1: generate the elements of the matrix equation Ax = B
    psTrace("psModules.imcombine", 3, "Calculating kernel equations...\n");
    if (!pmSubtractionCalculateEquation(stamps, kernels)) {
	psError(psErrorCodeLast(), false, "Unable to calculate least-squares equation.");
	return false;
    }
		    
    // step 2: solve the matrix equation Ax = B
    psTrace("psModules.imcombine", 3, "Solving kernel equations...\n");
    if (!pmSubtractionSolveEquation(kernels, stamps)) {
	psError(psErrorCodeLast(), false, "Unable to calculate least-squares equation.");
	return false;
    }
    memCheck("  solve equation");

    // calculate the score for this model fit attempt
    // XXX store the chisq, flux and moments for stamp rejection
    pmSubtractionCalculateChisqAndMoments(bestMatch, stamps, kernels); // Stamp deviations

    // display the input and model stamps
    pmSubtractionVisualShowFit(stamps, kernels);
    pmSubtractionVisualPlotFit(kernels);
    pmSubtractionVisualPlotConvKernels(kernels);

    // reset the kernel if desired (on final pass, do not reset)
    if (!final) {
	kernels->mode = nativeMode;
	kernels->spatialOrder = nativeOrder;
    } else {
      pmSubtractionKernelsMakeDescription(kernels);
      psLogMsg("psModules.imcombine", PS_LOG_INFO, "final kernel: %s", kernels->description);
    }
    return true;
}

bool pmSubtractionMatch(pmReadout *conv1, pmReadout *conv2, const pmReadout *ro1, const pmReadout *ro2,
                        int footprint, int stride, float regionSize, float stampSpacing, float threshold,
                        const psArray *sources, const char *stampsName,
                        pmSubtractionKernelsType type, int size, int spatialOrder,
                        psVector *isisWidths, const psVector *isisOrders,
                        int inner, int ringsOrder, int binning, float penalty,
                        bool optimum, const psVector *optFWHMs, int optOrder, float optThreshold,
                        int iter, float rej, float normFrac, float sysError, float skyError,
                        float kernelError, float covarFrac, psImageMaskType maskVal, psImageMaskType maskBad,
                        psImageMaskType maskPoor, float poorFrac, float badFrac, pmSubtractionMode subMode)
{
    if (!subtractionMatchCheck(conv1, conv2, ro1, ro2, stride, normFrac, sysError, skyError, kernelError,
                               covarFrac, maskVal, maskBad, maskPoor, poorFrac, badFrac, subMode)) {
        return false;
    }

    // We need both inputs
    PM_ASSERT_READOUT_NON_NULL(ro1, false);
    PM_ASSERT_READOUT_NON_NULL(ro2, false);

    PS_ASSERT_INT_NONNEGATIVE(footprint, false);
    // regionSize can be just about anything (except maybe negative, but it can be NAN)
    PS_ASSERT_FLOAT_LARGER_THAN(stampSpacing, 0.0, false);
    // Don't care what threshold is
    if (sources) {
        PS_ASSERT_ARRAY_NON_NULL(sources, false);
    }
    // stampsName may be anything
    // We'll check kernel type when we allocate the kernels
    PS_ASSERT_INT_POSITIVE(size, false);
    PS_ASSERT_INT_NONNEGATIVE(spatialOrder, false);
    if (isisWidths || isisOrders) {
        PS_ASSERT_VECTOR_NON_NULL(isisWidths, false);
        PS_ASSERT_VECTOR_TYPE(isisWidths, PS_TYPE_F32, false);
        PS_ASSERT_VECTOR_NON_NULL(isisOrders, false);
        PS_ASSERT_VECTOR_TYPE(isisOrders, PS_TYPE_S32, false);
        PS_ASSERT_VECTORS_SIZE_EQUAL(isisWidths, isisOrders, false);
    }
    PS_ASSERT_INT_NONNEGATIVE(inner, false);
    PS_ASSERT_INT_NONNEGATIVE(ringsOrder, false);
    PS_ASSERT_INT_POSITIVE(binning, false);
    if (optimum) {
        PS_ASSERT_VECTOR_NON_NULL(optFWHMs, false);
        PS_ASSERT_INT_NONNEGATIVE(optOrder, false);
        PS_ASSERT_FLOAT_LARGER_THAN(optThreshold, 0.0, false);
        PS_ASSERT_FLOAT_LESS_THAN_OR_EQUAL(optThreshold, 1.0, false);
    }
    PS_ASSERT_INT_NONNEGATIVE(iter, false);
    PS_ASSERT_FLOAT_LARGER_THAN(rej, 0.0, false);

    // If the stamp footprint is smaller than the kernel size, then we won't get much signal in the outer
    // parts of the kernel, which can result in bad matching artifacts.
    if (footprint < size) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                "Stamp footprint (%d) should be larger than or equal to the kernel size (%d)",
                footprint, size);
        return false;
    }

    // Bail here if we're doing the simple matching
    if (type == PM_SUBTRACTION_KERNEL_SIMPLE) {
      if (!pmSubtractionSimpleMatch(conv1,conv2,ro1,ro2,sources,size,maskVal,maskBad,maskPoor,optThreshold)) {
	return false;
      }
      return(true);
    }

    // Where does our variance map come from?
    // Getting the variance exactly right is not necessary --- it's just used for weighting.
    psImage *variance = NULL;             // Variance image to use
    if (ro1->variance && ro2->variance) {
        variance = (psImage*)psBinaryOp(NULL, ro1->variance, "+", ro2->variance);
    } else if (ro1->variance) {
        variance = psMemIncrRefCounter(ro1->variance);
    } else if (ro2->variance) {
        variance = psMemIncrRefCounter(ro2->variance);
    } else {
        variance = (psImage*)psBinaryOp(NULL, ro1->image, "+", ro2->image);
    }

    // Putting important variable declarations here, since they are freed after a "goto" if there is an error.
    psImage *subMask = NULL;            // Mask for subtraction
    psRegion *region = psRegionAlloc(NAN, NAN, NAN, NAN); // Iso-kernel region
    psString regionString = NULL;       // String for region
    pmSubtractionStampList *stamps = NULL; // Stamps for matching PSF
    pmSubtractionKernels *kernels = NULL; // Kernel basis functions
    psMetadata *analysis = psMetadataAlloc(); // QA data
    psMetadata *header = psMetadataAlloc(); // QA data for header

    int numCols = ro1->image->numCols, numRows = ro1->image->numRows; // Image dimensions

    psRandom *rng = psRandomAlloc(PS_RANDOM_TAUS); // Random number generator

    pmSubtractionQuality *bestMatch = NULL;

    int N_TEST_MODES;
    int N_TEST_ORDER = spatialOrder;

    pmSubtractionMode TestModes[3];
    switch (subMode) {
      case PM_SUBTRACTION_MODE_1:
	N_TEST_MODES = 1;
	TestModes[0] = PM_SUBTRACTION_MODE_1;
	break;
      case PM_SUBTRACTION_MODE_2:
	N_TEST_MODES = 1;
	TestModes[0] = PM_SUBTRACTION_MODE_2;
	break;
      case PM_SUBTRACTION_MODE_SINGLE_AUTO:
	N_TEST_MODES = 2;
	TestModes[0] = PM_SUBTRACTION_MODE_1;
	TestModes[1] = PM_SUBTRACTION_MODE_2;
	break;
      case PM_SUBTRACTION_MODE_DUAL:
	N_TEST_MODES = 3;
	TestModes[0] = PM_SUBTRACTION_MODE_1;
	TestModes[1] = PM_SUBTRACTION_MODE_2;
	TestModes[2] = PM_SUBTRACTION_MODE_DUAL;
	break;
      default:
	psError(psErrorCodeLast(), false, "For now, only modes 1, 2, and DUAL are supported.");
	goto MATCH_ERROR;
    }

    
    memCheck("start");

    // pmSubtractionMaskInvalid(ro1, maskVal);
    // pmSubtractionMaskInvalid(ro2, maskVal);

    psRegion bounds = psRegionSet(NAN, NAN, NAN, NAN); // Bounds of valid pixels

    subMask = pmSubtractionMask(&bounds, ro1, ro2, maskVal, size, footprint, badFrac, subMode);
    if (!subMask) {
        psError(psErrorCodeLast(), false, "Unable to generate subtraction mask.");
        goto MATCH_ERROR;
    }

    memCheck("mask");

    // Get region of interest
    int xRegions = 1, yRegions = 1;     // Number of iso-kernel regions
    float xRegionSize = NAN, yRegionSize = NAN; // Size of iso-kernel regions
    if (isfinite(regionSize) && regionSize != 0.0) {
        xRegions = (bounds.x1 - bounds.x0) / regionSize + 1;
        yRegions = (bounds.y1 - bounds.y0) / regionSize + 1;
        xRegionSize = (float)(bounds.x1 - bounds.x0) / (float)xRegions;
        yRegionSize = (float)(bounds.y1 - bounds.y0) / (float)yRegions;
    } else {
        xRegionSize = bounds.x1 - bounds.x0;
        yRegionSize = bounds.y1 - bounds.y0;
    }

    // General background subtraction and measurement of stamp threshold
    float stampThresh1 = NAN, stampThresh2 = NAN; // Stamp thresholds for images
    {
        psStats *bg = psStatsAlloc(PS_STAT_ROBUST_MEDIAN | PS_STAT_ROBUST_STDEV); // Statistics for background
        if (ro1) {
            psStatsInit(bg);
            if (!psImageBackground(bg, NULL, ro1->image, ro1->mask, maskVal, rng)) {
                psError(PM_ERR_DATA, false, "Unable to measure background statistics.");
                psFree(bg);
                goto MATCH_ERROR;
            }
            stampThresh1 = threshold * bg->robustStdev;
            psBinaryOp(ro1->image, ro1->image, "-", psScalarAlloc((float)bg->robustMedian, PS_TYPE_F32));
        }
        if (ro2) {
            psStatsInit(bg);
            if (!psImageBackground(bg, NULL, ro2->image, ro2->mask, maskVal, rng)) {
                psError(PM_ERR_DATA, false, "Unable to measure background statistics.");
                psFree(bg);
                goto MATCH_ERROR;
            }
            stampThresh2 = threshold * bg->robustStdev;
            psBinaryOp(ro2->image, ro2->image, "-", psScalarAlloc((float)bg->robustMedian, PS_TYPE_F32));
        }
        psFree(bg);
    }
    
    subtractionMatchAlloc(conv1, conv2, ro1, ro2, subMask, maskBad, subMode, numCols, numRows);
    
    // Iterate over iso-kernel regions
    for (int j = 0; j < yRegions; j++) {
        for (int i = 0; i < xRegions; i++) {
            psTrace("psModules.imcombine", 1, "Subtracting region %d of %d...\n",
                    j * xRegions + i + 1, xRegions * yRegions);
            *region = psRegionSet(bounds.x0 + (int)(i * xRegionSize),
                                  bounds.x0 + (int)((i + 1) * xRegionSize),
                                  bounds.y0 + (int)(j * yRegionSize),
                                  bounds.y0 + (int)((j + 1) * yRegionSize));
            psFree(regionString);
            regionString = psRegionToString(*region);
            psLogMsg("psModules.imcombine", PS_LOG_DETAIL, "Iso-kernel region: %s out of %d,%d\n",
		     regionString, numCols, numRows);

            if (stampsName && strlen(stampsName) > 0) {
                stamps = pmSubtractionStampsSetFromFile(stampsName, ro1->image, subMask, region, size,
                                                        footprint, stampSpacing, normFrac,
                                                        sysError, skyError, subMode);
            } else if (sources) {
                stamps = pmSubtractionStampsSetFromSources(sources, ro1->image, subMask, region, size,
                                                           footprint, stampSpacing, normFrac,
                                                           sysError, skyError, subMode);
            }

	    int nTries = 0;
	    bool tryAgain = true;
	    while (tryAgain) {
		// We get the stamps here; we will also attempt to get stamps at the first iteration, but it
		// doesn't matter.
		if (!pmSubtractionStampsSelect(&stamps, ro1, ro2, subMask, variance, region, stampThresh1, stampThresh2,
					       stampSpacing, normFrac, sysError, skyError, size, footprint, subMode)) {
		    goto MATCH_ERROR;
		}

		// generate the window function from the set of stamps
		// we attempt to set the window based on the measured kron radius, but the
		// initial guess may be too small. allow the window to grow if the kron radius
		// implies the need for a larger windon.  But only allow 2 additional tries
		if (!pmSubtractionStampsGetWindow(&tryAgain, stamps, size)) {
		    // if we failed, it might be due to the desired normWindow being larger than the current footprint.
		    // in this case, just adjust the footprint and try again.
		    if (tryAgain && (nTries >= 5)) {
			// unrecoverable error
			psError(PM_ERR_STAMPS, true, "Unable to get stamp window (failure to converge).");
			goto MATCH_ERROR;
		    }
		    if (tryAgain) {
			// keep the border constant
			int border = footprint - size;
			size = PS_MAX(stamps->normWindow1, stamps->normWindow2) + 2;
			footprint = size + border;

			// we need to reconstruct everything, so just free the stamps here and retry
			psFree(stamps);
			nTries ++;
		    } else {
			// unrecoverable error generated in pmSubtractionStampsGetWindow
			psError(psErrorCodeLast(), false, "Unable to get stamp window.");
			goto MATCH_ERROR;
		    }
		}
	    }

	    // check on the kernel scaling -- if the kron-based radial moments are very different, adjust to match them
	    { 
		// float fwhm1;
		// float fwhm2;
		// pmSubtractionGetFWHMs(&fwhm1, &fwhm2);
		// psAssert(isfinite(fwhm1), "fwhm 1 not set");
		// psAssert(isfinite(fwhm2), "fwhm 2 not set");

		// XXX this is BAD: depends on the relationship below:
		// stamps->normWindow1 = 2.75*R1;
		// stamps->normWindow2 = 2.75*R2;
		float radMoment1 = stamps->normWindow1 / 2.75;
		float radMoment2 = stamps->normWindow2 / 2.75;
		pmSubtractionParamsScale(NULL, NULL, isisWidths, radMoment1, radMoment2);

		// float maxFWHM = PS_MAX(fwhm1, fwhm2);
		// float maxRadial = PS_MAX(radMoment1, radMoment2);
		
		// if (fabs(2.0*(maxFWHM - maxRadial)/(maxFWHM + maxRadial)) > 0.25) {
		// if (1) {
		// 
		//     float scale = maxRadial / maxFWHM;
		//     psLogMsg ("psModules.imcombine", PS_LOG_INFO, "Kron and FWHM scales are quite different, re-scale by %f to use Kron", scale);
		//     
		//     for (int i = 0; i < isisWidths->n; i++) {
		// 	isisWidths->data.F32[i] *= scale;
		//     }
		// }
	    }

	    // Define kernel basis functions
	    if (optimum && (type == PM_SUBTRACTION_KERNEL_ISIS || type == PM_SUBTRACTION_KERNEL_GUNK)) {
		kernels = pmSubtractionKernelsOptimumISIS(type, size, inner, spatialOrder,
							  optFWHMs, optOrder, stamps, footprint,
							  optThreshold, penalty, bounds, subMode);
		if (!kernels) {
		    psErrorClear();
		    psWarning("Unable to derive optimum ISIS kernel --- switching to default.");
		}
	    }
	    if (kernels == NULL) {
		// Not an ISIS/GUNK kernel, or the optimum kernel search failed
		kernels = pmSubtractionKernelsGenerate(type, size, spatialOrder, isisWidths, isisOrders,
						       inner, binning, ringsOrder, penalty, bounds, subMode);
	    }

	    memCheck("kernels");

// this section was an old version of auto-choosing the direction.  the test was not as reliable as
// we would like; this is replaced by pmSubtractionMatchAttempt
# if (0)
	    if (subMode == PM_SUBTRACTION_MODE_UNSURE) {
		pmSubtractionMode newMode = pmSubtractionBestMode(&stamps, &kernels, subMask, rej);
		switch (newMode) {
		  case PM_SUBTRACTION_MODE_1:
		    psLogMsg("psModules.imcombine", PS_LOG_INFO, "Convolving image 1 to match image 2.");
		    break;
		  case PM_SUBTRACTION_MODE_2:
		    psLogMsg("psModules.imcombine", PS_LOG_INFO, "Convolving image 2 to match image 1.");
		    break;
		  default:
		    psError(psErrorCodeLast(), false, "Unable to determine subtraction order.");
		    goto MATCH_ERROR;
		}
		subMode = newMode;
	    }
# endif

	    int numRejected = -1;       // Number of rejected stamps in each iteration
	    for (int k = 0; (k < iter) && (numRejected != 0); k++) {
		psLogMsg("psModules.imcombine", PS_LOG_INFO, "Iteration %d.", k);

		bool tryAgain = true;
		while (tryAgain) {
		    // We get the stamps here; we will also attempt to get stamps at the first iteration, but it
		    // doesn't matter.
		    if (!pmSubtractionStampsSelect(&stamps, ro1, ro2, subMask, variance, region, stampThresh1, stampThresh2,
						   stampSpacing, normFrac, sysError, skyError, size, footprint, subMode)) {
			goto MATCH_ERROR;
		    }

		    // generate the window function from the set of stamps
		    if (!pmSubtractionStampsGetWindow(&tryAgain, stamps, size)) {
			// if we failed, it might be due to the desired normWindow being larger than the current footprint.
			// in this case, just adjust the footprint and try again.
			if (tryAgain) {
			    footprint = PS_MAX(stamps->normWindow1, stamps->normWindow2) + 2;

			    // we need to reconstruct everything, so just free the stamps here and retry
			    psFree(stamps);
			} else {
			    // unrecoverable error
			    psError(psErrorCodeLast(), false, "Unable to get stamp window.");
			    goto MATCH_ERROR;
			}
		    }
		}

		// step 0 : calculate the normalizations, pass along to the next steps via stamps->normValue
		psTrace("psModules.imcombine", 3, "Calculating normalization...\n");
		if (!pmSubtractionCalculateNormalization(stamps, kernels->mode)) {
		    psError(psErrorCodeLast(), false, "Unable to calculate least-squares equation.");
		    goto MATCH_ERROR;
		}

		// on each iteration, we start from scratch
		psFree(bestMatch);

		// choose the spatial order and subtraction direction (1, 2, dual)
		// XXX need to make these respect recipe somewhat
		for (int order = 0; order <= N_TEST_ORDER; order++) {
		    for (int j = 0; j < N_TEST_MODES; j++) {
			if (!pmSubtractionMatchAttempt(&bestMatch, kernels, stamps, TestModes[j], order, false)) {
			    goto MATCH_ERROR;
			}
		    }
		}
		
		// reject the deviant stamps based on the stats of the best match
		psTrace("psModules.imcombine", 3, "Rejecting stamps...\n");
		numRejected = pmSubtractionRejectStamps(kernels, stamps, bestMatch, subMask, rej);
		if (numRejected < 0) {
		    psError(psErrorCodeLast(), false, "Unable to reject stamps.");
		    goto MATCH_ERROR;
		}
		memCheck("  reject stamps");
	    }

	    // apply the best fit so we are ready to roll
	    psLogMsg("psModules.imcombine", PS_LOG_INFO, "applying order: %d, mode: %d\n", bestMatch->spatialOrder, bestMatch->mode);
	    if (!pmSubtractionMatchAttempt(NULL, kernels, stamps, bestMatch->mode, bestMatch->spatialOrder, true)) {
		goto MATCH_ERROR;
	    }
	    psFree(stamps);
	    psFree(bestMatch);
	    memCheck("solution");

	    if (!pmSubtractionAnalysis(analysis, header, kernels, region, numCols, numRows)) {
		psError(psErrorCodeLast(), false, "Unable to generate QA data");
		goto MATCH_ERROR;
	    }
	    memCheck("diag outputs");

	    psTrace("psModules.imcombine", 2, "Convolving...\n");
	    if (!pmSubtractionConvolve(conv1, conv2, ro1, ro2, subMask, stride, maskBad, maskPoor, poorFrac,
				       kernelError, covarFrac, region, kernels, true, useFFT)) {
		psError(psErrorCodeLast(), false, "Unable to convolve image.");
		goto MATCH_ERROR;
	    }

	    psFree(kernels);
	    kernels = NULL;
	}
    }
    psFree(rng);
    rng = NULL;
    psFree(region);
    region = NULL;
    psFree(regionString);
    regionString = NULL;
    psFree(subMask);
    subMask = NULL;
    psFree(variance);
    variance = NULL;

    if (conv1 && !pmSubtractionBorder(conv1->image, conv1->variance, conv1->mask, size, maskBad)) {
	psError(psErrorCodeLast(), false, "Unable to set border of convolved image.");
	goto MATCH_ERROR;
    }
    if (conv2 && !pmSubtractionBorder(conv2->image, conv2->variance, conv2->mask, size, maskBad)) {
	psError(psErrorCodeLast(), false, "Unable to set border of convolved image.");
	goto MATCH_ERROR;
    }

    memCheck("convolution");

    subtractionAnalysisUpdate(conv1, conv2, analysis, header);
    psFree(analysis);
    psFree(header);

#ifdef TESTING
    {
	if (subMode == PM_SUBTRACTION_MODE_1 || subMode == PM_SUBTRACTION_MODE_DUAL) {
	    psFits *fits = psFitsOpen("convolved1.fits", "w");
	    psFitsWriteImage(fits, NULL, conv1->image, 0, NULL);
	    psFitsClose(fits);
	}

	if (subMode == PM_SUBTRACTION_MODE_2 || subMode == PM_SUBTRACTION_MODE_DUAL) {
	    psFits *fits = psFitsOpen("convolved2.fits", "w");
	    psFitsWriteImage(fits, NULL, conv2->image, 0, NULL);
	    psFitsClose(fits);
	}
    }
#endif

    return true;

MATCH_ERROR:
    psFree(analysis);
    psFree(header);
    psFree(region);
    psFree(regionString);
    psFree(subMask);
    psFree(kernels);
    psFree(stamps);
    psFree(variance);
    psFree(rng);
    psFree(bestMatch);
    return false;
}


// Determine a rough width (integer value) of the star in the image
// XXX Could improve this by using a user-provided list of floating-point widths (or an end point and
// increment).
static int subtractionOrderWidth(const psKernel *kernel, // Image
				 float bg, // Background in image
				 int size, // Maximum size
				 const psArray *models, // Buffer of models
				 const psVector *modelSums // Buffer of model sums
    )
{
    assert(kernel);
    assert(models);
    assert(modelSums);

    int xMin = -size, xMax = size; // Bounds in x
    int yMin = -size, yMax = size; // Bounds in y

    // Fit gaussians of varying widths to the image, record the chi^2
    psVector *chi2 = psVectorAlloc(size, PS_TYPE_F32); // chi^2 as a function of radius
    for (int sigma = 0; sigma < size; sigma++) {
	double sumFG = 0.0; // Sum for calculating the normalisation of the Gaussian
	psKernel *model = models->data[sigma]; // Model of interest
	for (int y = yMin; y <= yMax; y++) {
	    for (int x = xMin; x <= xMax; x++) {
		sumFG += model->kernel[y][x] * (kernel->kernel[y][x] - bg);
	    }
	}
	float norm = sumFG * modelSums->data.F64[sigma]; // Normalisation for Gaussian
	double sumDev2 = 0.0;           // Sum of square deviations
	for (int y = yMin; y <= yMax; y++) {
	    for (int x = xMin; x <= xMax; x++) {
		float dev = kernel->kernel[y][x] - bg - norm * model->kernel[y][x]; // Deviation
		sumDev2 += PS_SQR(dev);
	    }
	}
	chi2->data.F32[sigma] = sumDev2;
    }

    // Find the minimum chi^2
    int bestIndex = -1;                 // Index of best chi^2
    float bestChi2 = INFINITY;          // Best chi^2
    for (int i = 0; i < size; i++) {
	if (chi2->data.F32[i] < bestChi2) {
	    bestChi2 = chi2->data.F32[i];
	    bestIndex = i;
	}
    }
    psFree(chi2);

    return bestIndex + 1;
}


bool pmSubtractionOrderStamp(psVector *ratios, psVector *mask, const pmSubtractionStampList *stamps,
			     const psArray *models, const psVector *modelSums,
			     int index, float bg1, float bg2)
{
    PS_ASSERT_VECTOR_NON_NULL(ratios, false);
    PS_ASSERT_VECTOR_NON_NULL(mask, false);
    PS_ASSERT_VECTORS_SIZE_EQUAL(ratios, mask, false);
    PM_ASSERT_SUBTRACTION_STAMP_LIST_NON_NULL(stamps, false);
    PS_ASSERT_INT_NONNEGATIVE(index, false);
    PS_ASSERT_INT_LESS_THAN(index, stamps->num, false);
    PS_ASSERT_ARRAY_NON_NULL(models, false);
    PS_ASSERT_VECTOR_NON_NULL(modelSums, false);
    PS_ASSERT_VECTOR_SIZE(modelSums, models->n, false);

    pmSubtractionStamp *stamp = stamps->stamps->data[index]; // Stamp of interest
    psAssert(stamp->status == PM_SUBTRACTION_STAMP_CALCULATE || stamp->status == PM_SUBTRACTION_STAMP_USED,
	     "We checked this earlier.");

    // Widths of stars
    int width1 = subtractionOrderWidth(stamp->image1, bg1, stamps->footprint, models, modelSums);
    int width2 = subtractionOrderWidth(stamp->image2, bg2, stamps->footprint, models, modelSums);

    if (width1 == 0 || width2 == 0) {
	ratios->data.F32[index] = NAN;
	mask->data.PS_TYPE_VECTOR_MASK_DATA[index] = 0xff;
    } else {
	ratios->data.F32[index] = (float)width1 / (float)width2;
	mask->data.PS_TYPE_VECTOR_MASK_DATA[index] = 0;
	psTrace("psModules.imcombine", 3, "Stamp %d (%.1f,%.1f) widths: %d, %d --> %f\n",
		index, stamp->x, stamp->y, width1, width2, ratios->data.F32[index]);
    }

    return true;
}

bool pmSubtractionOrderThread(psThreadJob *job)
{
    PS_ASSERT_THREAD_JOB_NON_NULL(job, false);

    psVector *ratios = job->args->data[0]; // Ratios of widths
    psVector *mask = job->args->data[1]; // Mask for ratios
    const pmSubtractionStampList *stamps = job->args->data[2]; // List of stamps
    const psArray *models = job->args->data[3]; // Gaussian models
    const psVector *modelSums = job->args->data[4]; // Gaussian model sums
    int index = PS_SCALAR_VALUE(job->args->data[5], S32); // Stamp index
    float bg1 = PS_SCALAR_VALUE(job->args->data[6], F32); // Background of image 1
    float bg2 = PS_SCALAR_VALUE(job->args->data[7], F32); // Background of image 2

    return pmSubtractionOrderStamp(ratios, mask, stamps, models, modelSums, index, bg1, bg2);
}

pmSubtractionMode pmSubtractionOrder(pmSubtractionStampList *stamps, float bg1, float bg2)
{
    PM_ASSERT_SUBTRACTION_STAMP_LIST_NON_NULL(stamps, PM_SUBTRACTION_MODE_ERR);

    psVector *mask = psVectorAlloc(stamps->num, PS_TYPE_VECTOR_MASK); // Mask for stamps
    psVector *ratios = psVectorAlloc(stamps->num, PS_TYPE_F32); // Ratios of widths

    // Generate models
    int size = stamps->footprint;       // Maximum size
    psArray *models = psArrayAlloc(size); // Gaussian models
    psVector *modelSums = psVectorAlloc(size, PS_TYPE_F64); // Gaussian model sums
    for (int sigma = 0; sigma < size; sigma++) {
	psKernel *model = psKernelAlloc(-size, size, -size, size); // Gaussian model
	float invSigma2 = 1.0 / (float)PS_SQR(1 + sigma); // Inverse sigma squared
	double sumGG = 0.0;         // Sum of square of Gaussian
	for (int y = -size; y <= size; y++) {
	    int y2 = PS_SQR(y);     // y squared
	    for (int x = -size; x <= size; x++) {
		float rad2 = PS_SQR(x) + y2; // Radius squared
		float value = expf(-rad2 * invSigma2); // Model value
		model->kernel[y][x] = value;
		sumGG += PS_SQR(value);
	    }
	}
	models->data[sigma] = model;
	modelSums->data.F64[sigma] = 1.0 / sumGG;
    }

    // Fit models to stamps
    for (int i = 0; i < stamps->num; i++) {
	pmSubtractionStamp *stamp = stamps->stamps->data[i]; // Stamp of interest
	if (stamp->status != PM_SUBTRACTION_STAMP_CALCULATE && stamp->status != PM_SUBTRACTION_STAMP_USED) {
	    mask->data.PS_TYPE_VECTOR_MASK_DATA[i] = 0xff;
	    continue;
	}

	if (pmSubtractionThreaded()) {
	    psThreadJob *job = psThreadJobAlloc("PSMODULES_SUBTRACTION_ORDER");
	    psArrayAdd(job->args, 1, ratios);
	    psArrayAdd(job->args, 1, mask);
	    psArrayAdd(job->args, 1, stamps);
	    psArrayAdd(job->args, 1, models);
	    psArrayAdd(job->args, 1, modelSums);
	    PS_ARRAY_ADD_SCALAR(job->args, i, PS_TYPE_S32);
	    PS_ARRAY_ADD_SCALAR(job->args, bg1, PS_TYPE_F32);
	    PS_ARRAY_ADD_SCALAR(job->args, bg2, PS_TYPE_F32);
	    if (!psThreadJobAddPending(job)) {
		return false;
	    }
	} else {
	    if (!pmSubtractionOrderStamp(ratios, mask, stamps, models, modelSums, i, bg1, bg2)) {
		psError(psErrorCodeLast(), false, "Unable to measure PSF width for stamp %d", i);
		psFree(models);
		psFree(modelSums);
		psFree(ratios);
		psFree(mask);
		return false;
	    }
	}
    }

    if (!psThreadPoolWait(true, true)) {
	psError(psErrorCodeLast(), false, "Error waiting for threads.");
	psFree(models);
	psFree(modelSums);
	psFree(ratios);
	psFree(mask);
	return false;
    }

    psFree(models);
    psFree(modelSums);

    psStats *stats = psStatsAlloc(PS_STAT_ROBUST_MEDIAN);
    if (!psVectorStats(stats, ratios, NULL, mask, 0xff)) {
	psError(psErrorCodeLast(), false, "Unable to calculate statistics for moments ratio.");
	psFree(mask);
	psFree(ratios);
	psFree(stats);
	return PM_SUBTRACTION_MODE_ERR;
    }
    psFree(ratios);
    psFree(mask);

    // XXX raise an error here or not?
    if (isnan(stats->robustMedian)) {
	psFree(stats);
	return PM_SUBTRACTION_MODE_ERR;
    }

    psLogMsg("psModules.imcombine", PS_LOG_INFO, "Median width ratio: %lf", stats->robustMedian);
    pmSubtractionMode mode = (stats->robustMedian <= 1.0 ? PM_SUBTRACTION_MODE_1 : PM_SUBTRACTION_MODE_2);
    psFree(stats);

    return mode;
}


// Test a subtraction mode by performing a single iteration
static bool subtractionModeTest(pmSubtractionStampList *stamps, // Stamps to use to find best mode
				pmSubtractionKernels *kernels, // Kernel description
				const char *description, // Description for trace
				psImage *subMask,  // Subtraction mask
				float rej               // Rejection threshold
    )
{
    assert(stamps);
    assert(kernels);

    psAbort("this function is not working");
# if (0)
    psTrace("psModules.imcombine", 3, "Convolving stamps as needed...\n");
    if (!pmSubtractionConvolveStamps(stamps, kernels)) {
	psError(psErrorCodeLast(), false, "Unable to convolve stamps.");
	return false;
    }

    psTrace("psModules.imcombine", 3, "Calculating %s normalization equation...\n", description);
    if (!pmSubtractionCalculateEquation(stamps, kernels)) {
	psError(psErrorCodeLast(), false, "Unable to calculate least-squares equation.");
	return false;
    }

    psTrace("psModules.imcombine", 3, "Solving %s normalization equation...\n", description);
    if (!pmSubtractionSolveEquation(kernels, stamps)) {
	psError(psErrorCodeLast(), false, "Unable to calculate least-squares equation.");
	return false;
    }

    psTrace("psModules.imcombine", 3, "Calculate %s deviations...\n", description);
    psVector *deviations = pmSubtractionCalculateDeviations(stamps, kernels); // Stamp deviations
    if (!deviations) {
	psError(psErrorCodeLast(), false, "Unable to calculate deviations.");
	return false;
    }

    // XXX this needs to be made consistent with the modified 'reject stamps' function
    psTrace("psModules.imcombine", 3, "Rejecting %s stamps...\n", description);
    long numRejected = pmSubtractionRejectStamps(kernels, stamps, deviations, subMask, rej);
    if (numRejected < 0) {
	psError(psErrorCodeLast(), false, "Unable to reject stamps.");
	psFree(deviations);
	return false;
    }
    psFree(deviations);

    if (numRejected > 0) {
	// Allow re-fit with reduced stamps set
	psTrace("psModules.imcombine", 3, "Calculating %s normalization equation...\n", description);
	if (!pmSubtractionCalculateEquation(stamps, kernels)) {
	    psError(psErrorCodeLast(), false, "Unable to calculate least-squares equation.");
	    return false;
	}

	psTrace("psModules.imcombine", 3, "Resolving %s equation...\n", description);
	if (!pmSubtractionSolveEquation(kernels, stamps)) {
	    psError(psErrorCodeLast(), false, "Unable to calculate least-squares equation.");
	    return false;
	}
	psTrace("psModules.imcombine", 3, "Recalculate %s deviations...\n", description);

	psVector *deviations = pmSubtractionCalculateDeviations(stamps, kernels); // Stamp deviations
	if (!deviations) {
	    psError(psErrorCodeLast(), false, "Unable to calculate deviations.");
	    return false;
	}
	psTrace("psModules.imcombine", 3, "Measuring %s quality...\n", description);
	long numRejected = pmSubtractionRejectStamps(kernels, stamps, deviations, subMask, NAN);
	if (numRejected < 0) {
	    psError(psErrorCodeLast(), false, "Unable to reject stamps.");
	    psFree(deviations);
	    return false;
	}
	psFree(deviations);
    }
# endif
    return true;
}


pmSubtractionMode pmSubtractionBestMode(pmSubtractionStampList **stamps, pmSubtractionKernels **kernels,
					const psImage *subMask, float rej)
{
    PM_ASSERT_SUBTRACTION_STAMP_LIST_NON_NULL(*stamps, PM_SUBTRACTION_MODE_ERR);
    PM_ASSERT_SUBTRACTION_KERNELS_NON_NULL(*kernels, PM_SUBTRACTION_MODE_ERR);

    // Copies of the inputs
    pmSubtractionStampList *stamps1 = pmSubtractionStampListCopy(*stamps);
    pmSubtractionKernels *kernels1 = pmSubtractionKernelsCopy(*kernels);
    psImage *subMask1 = psImageCopy(NULL, subMask, subMask->type.type);
    kernels1->mode = PM_SUBTRACTION_MODE_1;

    if (!subtractionModeTest(stamps1, kernels1, "convolve 1", subMask1, rej)) {
	psError(psErrorCodeLast(), false, "Unable to test subtraction with convolution of image 1");
	psFree(stamps1);
	psFree(kernels1);
	psFree(subMask1);
	return PM_SUBTRACTION_MODE_ERR;
    }
    psFree(subMask1);

    // Copies of the inputs
    pmSubtractionStampList *stamps2 = pmSubtractionStampListCopy(*stamps);
    pmSubtractionKernels *kernels2 = pmSubtractionKernelsCopy(*kernels);
    psImage *subMask2 = psImageCopy(NULL, subMask, subMask->type.type);
    kernels2->mode = PM_SUBTRACTION_MODE_2;

    if (!subtractionModeTest(stamps2, kernels2, "convolve 2", subMask2, rej)) {
	psError(psErrorCodeLast(), false, "Unable to test subtraction with convolution of image 2");
	psFree(stamps2);
	psFree(kernels2);
	psFree(subMask2);
	psFree(stamps1);
	psFree(kernels1);
	return PM_SUBTRACTION_MODE_ERR;
    }
    psFree(subMask2);


    pmSubtractionStampList *bestStamps = NULL; // Best choice for stamps
    pmSubtractionKernels *bestKernels = NULL; // Best choice for kernels
    psLogMsg("psModules.imcombine", PS_LOG_INFO,
	     "Image 1: %f +/- %f from %d stamps\nImage 2: %f +/- %f from %d stamps\n",
	     kernels1->mean, kernels1->rms, kernels1->numStamps,
	     kernels2->mean, kernels2->rms, kernels2->numStamps);

    if (kernels1->mean < kernels2->mean) {
	bestStamps = stamps1;
	bestKernels = kernels1;
    } else {
	bestStamps = stamps2;
	bestKernels = kernels2;
    }

    psFree(*stamps);
    psFree(*kernels);
    *stamps = psMemIncrRefCounter(bestStamps);
    *kernels = psMemIncrRefCounter(bestKernels);

    psFree(stamps1);
    psFree(stamps2);
    psFree(kernels1);
    psFree(kernels2);

    return bestKernels->mode;
}

static float scaleRefOption = NAN;
static float scaleMinOption = NAN;
static float scaleMaxOption = NAN;
static bool  scaleOption = false;

bool pmSubtractionParamScaleOptions(bool scale, float scaleRef, float scaleMin, float scaleMax) { 

    if (scale) {
	PS_ASSERT_FLOAT_LARGER_THAN(scaleRef, 0.0, false);
	PS_ASSERT_FLOAT_LARGER_THAN(scaleMin, 0.0, false);
	PS_ASSERT_FLOAT_LARGER_THAN(scaleMax, 0.0, false);
	PS_ASSERT_FLOAT_LARGER_THAN(scaleMax, scaleMin, false);
    }

    scaleRefOption = scaleRef;
    scaleMinOption = scaleMin;
    scaleMaxOption = scaleMax;
    scaleOption = scale;
    
    return true;
}

bool pmSubtractionParamsScale(int *kernelSize, int *stampSize, psVector *widths, float fwhm1, float fwhm2)
{
    // PS_ASSERT_PTR_NON_NULL(kernelSize, false);
    // PS_ASSERT_PTR_NON_NULL(stampSize, false);
    PS_ASSERT_VECTOR_NON_NULL(widths, false);
    PS_ASSERT_VECTOR_TYPE(widths, PS_TYPE_F32, false);

    if (!scaleOption) return true;

    // pmSubtractionGetFWHMs(&fwhm1, &fwhm2);
    // psAssert(isfinite(fwhm1), "fwhm 1 not set");
    // psAssert(isfinite(fwhm2), "fwhm 2 not set");
    
    // float diff = sqrtf(PS_SQR(PS_MAX(fwhm1, fwhm2)) - PS_SQR(PS_MIN(fwhm1, fwhm2))); // Difference
    float scale = PS_MAX(fwhm1, fwhm2) / scaleRefOption;      // Scaling factor

    if (isfinite(scaleMinOption) && scale < scaleMinOption) {
	scale = scaleMinOption;
    }
    if (isfinite(scaleMaxOption) && scale > scaleMaxOption) {
	scale = scaleMaxOption;
    }

    for (int i = 0; i < widths->n; i++) {
	widths->data.F32[i] *= scale;
    }
    if (kernelSize) {
	*kernelSize = *kernelSize * scale + 0.5;
    }
    if (stampSize) {
	*stampSize = *stampSize * scale + 0.5;
    }

    psLogMsg("psModules.imcombine", PS_LOG_INFO, "Scaling kernel parameters by %f", scale);
    if (kernelSize) psLogMsg("psModules.imcombine", PS_LOG_INFO, " modified kernel size %d", *kernelSize);
    if (stampSize) psLogMsg("psModules.imcombine", PS_LOG_INFO, " modified stamp size %d", *stampSize);

    return true;
}
