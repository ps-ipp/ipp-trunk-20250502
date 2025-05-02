#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <pslib.h>

#include "pmHDU.h"
#include "pmFPA.h"
#include "pmHDUUtils.h"
#include "pmFPAMaskWeight.h"
#include "pmConceptsAverage.h"
#include "pmReadoutStack.h"

#include "pmReadoutCombine.h"

//#define SHOW_BUSY 1                   // Show that the function is busy


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Public functions
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Allocator for pmCombineParams
pmCombineParams *pmCombineParamsAlloc(psStatsOptions combine)
{
    pmCombineParams *params = psAlloc(sizeof(pmCombineParams));

    params->combine = combine;
    params->maskVal = 0;
    params->blank = 0;
    params->nKeep = 0;
    params->fracLow = 0.0;
    params->fracHigh = 0.0;
    params->iter = 1;
    params->rej = INFINITY;
    params->variances = false;

    return params;
}

// check the input parameters and set up the output images
bool pmReadoutCombinePrepare(pmReadout *output, const psArray *inputs, const pmCombineParams *params)
{
    // Check inputs
    PS_ASSERT_PTR_NON_NULL(output, false);
    PS_ASSERT_ARRAY_NON_NULL(inputs, false);
    PS_ASSERT_PTR_NON_NULL(params, false);
    PS_ASSERT_FLOAT_WITHIN_RANGE(params->fracLow, 0.0, 1.0, false);
    PS_ASSERT_FLOAT_WITHIN_RANGE(params->fracHigh, 0.0, 1.0, false);

    // valid combintion statistic?
    bool valid = false;
    valid |= (params->combine == PS_STAT_SAMPLE_MEAN);
    valid |= (params->combine == PS_STAT_SAMPLE_MEDIAN);
    valid |= (params->combine == PS_STAT_ROBUST_MEDIAN);
    valid |= (params->combine == PS_STAT_FITTED_MEAN);
    valid |= (params->combine == PS_STAT_CLIPPED_MEAN);
    if (!valid) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Combination method is not SAMPLE_MEAN, SAMPLE_MEDIAN, "
                "ROBUST_MEDIAN, FITTED_MEAN or CLIPPED_MEAN.\n");
        return false;
    }

    pmHDU *hdu = pmHDUFromReadout(output); // Output HDU
    if (!hdu) {
        psError(PS_ERR_UNEXPECTED_NULL, false, "Unable to find HDU for readout.\n");
        return false;
    }

    //  set the output header metadata
    psString comment = NULL;        // Comment to add to header
    psStringAppend(&comment, "Combining using statistic: %x", params->combine);
    if (!hdu->header) {
        hdu->header = psMetadataAlloc();
    }
    psMetadataAddStr(hdu->header, PS_LIST_TAIL, "HISTORY", PS_META_DUPLICATE_OK, comment, "");
    psFree(comment);

    // note the clipping parameters, if used
    if (params->combine == PS_STAT_CLIPPED_MEAN) {
        psString comment = NULL;    // Comment to add to header
        psStringAppend(&comment, "Combination clipping: %d iterations, rejection at %f sigma", params->iter, params->rej);
        psMetadataAddStr(hdu->header, PS_LIST_TAIL, "HISTORY", PS_META_DUPLICATE_OK, comment, "");
        psFree(comment);
    }

    // note the use of variances
    if (params->variances) {
        psMetadataAddStr(hdu->header, PS_LIST_TAIL, "HISTORY", PS_META_DUPLICATE_OK,
                         "Using input variances to combine images", "");
    }

    // note the rejection fraction
    float keepFrac = 1.0 - params->fracLow - params->fracHigh; // Fraction of pixels to keep
    if (keepFrac != 1.0) {
        psString comment = NULL;        // Comment to add to header
        psStringAppend(&comment, "Min/max rejection: %f high, %f low, keep %d",
                       params->fracHigh, params->fracLow, params->nKeep);
        psMetadataAddStr(hdu->header, PS_LIST_TAIL, "HISTORY", PS_META_DUPLICATE_OK, comment, "");
        psFree(comment);
    }

    // note the mask value actually used
    psImageMaskType maskVal = params->maskVal; // The mask value
    if (maskVal) {
        psString comment = NULL;        // Comment to add to header
        psStringAppend(&comment, "Mask for combination: %x", maskVal);
        psMetadataAddStr(hdu->header, PS_LIST_TAIL, "HISTORY", PS_META_DUPLICATE_OK, comment, "");
        psFree(comment);
    }

    // determine the output image size based on the input images
    int row0, col0, numCols, numRows;
    if (!pmReadoutStackSetOutputSize(&col0, &row0, &numCols, &numRows, inputs)) {
        psError(PS_ERR_UNKNOWN, false, "problem setting output readout size.");
        return false;
    }

    // generate the required output images based on the specified sizes
    pmReadoutStackDefineOutput(output, col0, row0, numCols, numRows, true, params->variances, params->blank);
    psTrace("psModules.imcombine", 7, "Output minimum: %d,%d\n", output->col0, output->row0);

    // these calls allocate and save the requested images on the output analysis metadata
    psImage *counts = pmReadoutSetAnalysisImage(output, PM_READOUT_STACK_ANALYSIS_COUNT, numCols, numRows, PS_TYPE_U16, 0);
    if (!counts) {
        return false;
    }
    psImage *sigma = pmReadoutSetAnalysisImage(output, PM_READOUT_STACK_ANALYSIS_SIGMA, numCols, numRows, PS_TYPE_F32, NAN);
    if (!sigma) {
        return false;
    }

    // Update the "concepts"
    psList *inputCells = psListAlloc(NULL); // List of cells
    for (long i = 0; i < inputs->n; i++) {
        pmReadout *readout = inputs->data[i]; // Readout of interest
        psListAdd(inputCells, PS_LIST_TAIL, readout->parent);
    }
    bool success = pmConceptsAverageCells(output->parent, inputCells, NULL, NULL, true);
    psFree(inputCells);

    // set these even though the values are not yet set
    output->data_exists = true;
    output->parent->data_exists = true;
    output->parent->parent->data_exists = true;

    return success;
}

// XXX: Maybe add support for S16 and S32 types.  Currently, only F32 supported.
bool pmReadoutCombine(pmReadout *output, const psArray *inputs, const psVector *zero, const psVector *scale,
                      const pmCombineParams *params)
{
    // Check inputs
    PS_ASSERT_PTR_NON_NULL(output, false);
    PS_ASSERT_ARRAY_NON_NULL(inputs, false);
    PS_ASSERT_PTR_NON_NULL(params, false);
    if (zero) {
        PS_ASSERT_VECTOR_TYPE(zero, PS_TYPE_F32, false);
        PS_ASSERT_VECTOR_SIZE(zero, inputs->n, false);
    }
    if (scale) {
        PS_ASSERT_VECTOR_TYPE(scale, PS_TYPE_F32, false);
        PS_ASSERT_VECTOR_SIZE(scale, inputs->n, false);
    }
    PS_ASSERT_FLOAT_WITHIN_RANGE(params->fracLow, 0.0, 1.0, false);
    PS_ASSERT_FLOAT_WITHIN_RANGE(params->fracHigh, 0.0, 1.0, false);

    // does required/desired data exist?
    for (int i = 0; i < inputs->n; i++) {
        pmReadout *readout = inputs->data[i]; // Readout of interest
	psAssert(readout, "readout was not defined");
	if (!readout->process) continue;
        if (!readout->image) {
            psError(PS_ERR_UNEXPECTED_NULL, true, "Image data is missing for image %d.\n", i);
            return false;
        }
        if (params->variances && !readout->variance) {
            psError(PS_ERR_UNEXPECTED_NULL, true,
                    "Rejection based on variances requested, but no variances supplied for image %d.\n", i);
            return false;
        }
    }

    pmHDU *hdu = pmHDUFromReadout(output); // Output HDU
    if (!hdu) {
        psError(PS_ERR_UNEXPECTED_NULL, false, "Unable to find HDU for readout.\n");
        return false;
    }

#if 0
    pthread_t id = pthread_self();
    char name[64];
    sprintf(name, "%x", (unsigned int)id);
    psTimerStart(name);
#endif

    psStatsOptions combineStdev = 0; // Statistics option for variances
    switch (params->combine) {
      case PS_STAT_SAMPLE_MEAN:
      case PS_STAT_SAMPLE_MEDIAN:
        combineStdev = PS_STAT_SAMPLE_STDEV;
        break;
      case PS_STAT_ROBUST_MEDIAN:
        combineStdev = PS_STAT_ROBUST_STDEV;
        break;
      case PS_STAT_FITTED_MEAN:
        combineStdev = PS_STAT_FITTED_STDEV;
        break;
      case PS_STAT_CLIPPED_MEAN:
        combineStdev = PS_STAT_CLIPPED_STDEV;
        break;
      default:
        psAbort("Should never get here --- checked params->combine before.\n");
    }

    psStats *stats = psStatsAlloc(params->combine | combineStdev); // The statistics to use in the combination
    if (params->combine == PS_STAT_CLIPPED_MEAN) {
        stats->clipSigma = params->rej;
        stats->clipIter = params->iter;
    }

    psImage *counts = pmReadoutGetAnalysisImage(output, PM_READOUT_STACK_ANALYSIS_COUNT);
    if (!counts) {
        return false;
    }
    psImage *sigma = pmReadoutGetAnalysisImage(output, PM_READOUT_STACK_ANALYSIS_SIGMA);
    if (!sigma) {
        return false;
    }

    stats->options |= combineStdev;

    int minInputCols, maxInputCols, minInputRows, maxInputRows; // Smallest and largest values to combine
    int xSize, ySize;                   // Size of the output image
    if (!pmReadoutStackValidate(&minInputCols, &maxInputCols, &minInputRows, &maxInputRows, &xSize, &ySize, inputs)) {
        psError(PS_ERR_UNKNOWN, false, "No valid input readouts.");
        return false;
    }

    // We loop through each pixel in the output image.  We loop through each input readout.  We determine if
    // that output pixel is contained in the image from that readout.  If so, we save it in psVector pixels.
    // If not, we set a mask for that element in pixels.  Then, we mask off pixels not between fracLow and
    // fracHigh.  Then we call the vector stats routine on those pixels/mask.  Then we set the output pixel
    // value to the result of the stats call.

    psVector *pixels = psVectorAlloc(inputs->n, PS_TYPE_F32); // Stack of pixels
    psF32 *pixelsData = pixels->data.F32; // Dereference pixels

    psVector *mask   = psVectorAlloc(inputs->n, PS_TYPE_VECTOR_MASK); // Mask for stack
    psVectorMaskType *maskData = mask->data.PS_TYPE_VECTOR_MASK_DATA;     // Dereference mask

    psVector *variances = NULL;           // Stack of variances
    psVector *errors = NULL;            // Stack of errors (sqrt of variance), for psVectorStats
    psF32 *variancesData = NULL;          // Dereference variances
    if (params->variances) {
        variances = psVectorAlloc(inputs->n, PS_TYPE_F32); // Stack of variances
        variancesData = variances->data.F32;
    }
    psVector *index = NULL;             // The indices to sort the pixels

    float keepFrac = 1.0 - params->fracLow - params->fracHigh; // Fraction of pixels to keep
    psImageMaskType maskVal = params->maskVal; // The mask value

    #ifndef PS_NO_TRACE
    psTrace("psModules.imcombine", 3, "Iterating output: %d --> %d, %d --> %d\n",
            minInputCols - output->col0, maxInputCols - output->col0,
            minInputRows - output->row0, maxInputRows - output->row0);
    if (psTraceGetLevel("psModules.imcombine") >= 3) {
        for (int r = 0; r < inputs->n; r++) {
            pmReadout *readout = inputs->data[r]; // Input readout
            if (!readout->process) continue; 
            psTrace("psModules.imcombine", 3, "Iterating input %d: %d --> %d, %d --> %d\n", r,
                    minInputCols - readout->col0, maxInputCols - readout->col0,
                    minInputRows - readout->row0, maxInputRows - readout->row0);
        }
    }
    #endif

    // set up windows for visualization (if selected)
    pmReadoutCombineVisualInit();

    // Dereference output products
    psF32 **outputImage  = output->image->data.F32; // Output image
    psImageMaskType **outputMask   = output->mask->data.PS_TYPE_IMAGE_MASK_DATA; // Output mask
    psF32 **outputVariance = NULL; // Output variance map
    if (output->variance) {
        outputVariance = output->variance->data.F32;
    }

    psVector *invScale = NULL;          // Inverse scale; pre-calculated for efficiency
    if (scale) {
        invScale = (psVector*)psBinaryOp(NULL, psScalarAlloc(1.0, PS_TYPE_F32), "/", (const psPtr)scale);
    }

    for (int i = minInputRows; i < maxInputRows; i++) {
        int yOut = i - output->row0; // y position on output readout

        #ifdef SHOW_BUSY
        if (psTraceGetLevel("psModules.imcombine") > 9) {
            printf("Processing row %d\r", i);
            fflush(stdout);
        }
        #endif

        for (int j = minInputCols; j < maxInputCols; j++) {
            int xOut = j - output->col0; // x position on output readout

            int numValid = 0;           // Number of valid pixels in the stack
            memset(maskData, 0, mask->n * sizeof(psVectorMaskType)); // Reset the mask
            for (int r = 0; r < inputs->n; r++) {
                pmReadout *readout = inputs->data[r]; // Input readout
		if (!readout->process) {
		    maskData[r] = 1;
		    continue;
		}
                int yIn = i - readout->row0; // y position on input readout
                int xIn = j - readout->col0; // x position on input readout
                psImage *image = readout->image; // The readout image

                pixelsData[r] = image->data.F32[yIn][xIn];
                if (!isfinite(pixelsData[r])) {
                    maskData[r] = 1;
                    continue;
                }

                // Check mask
                psImage *roMask = readout->mask; // The mask image
                if (roMask && roMask->data.PS_TYPE_IMAGE_MASK_DATA[yIn][xIn] & maskVal) {
                    maskData[r] = 1;
                    continue;
                }

                if (params->variances) {
                    variancesData[r] = readout->variance->data.F32[yIn][xIn];
                }

                if (zero) {
                    pixelsData[r] -= zero->data.F32[r];
                }
                if (scale) {
                    pixelsData[r] *= invScale->data.F32[r];
                    if (params->variances) {
                        variancesData[r] *= invScale->data.F32[r] * invScale->data.F32[r];
                    }
                }

                numValid++;
            }

            if (numValid == 0) {
                outputMask[yOut][xOut] = params->blank;
                outputImage[yOut][xOut] = NAN;
                counts->data.U16[yOut][xOut] = 0;
                sigma->data.F32[yOut][xOut] = NAN;
                continue;
            }

            // Apply fracLow,fracHigh if there are enough pixels
            if (numValid * keepFrac >= params->nKeep && keepFrac != 1.0) {
                index = psVectorSortIndex(index, pixels);
                int numLow = numValid * params->fracLow; // Number of low pixels to clip
                int numHigh = numValid * params->fracHigh; // Number of high pixels to clip
                // Low pixels
                psS32 *indexData = index->data.S32; // Dereference index
                for (int k = 0, numMasked = 0; numMasked < numLow && k < index->n; k++) {
                    // Don't count the ones that are already masked
                    if (!maskData[indexData[k]]) {
                        maskData[indexData[k]] = 1;
                        numMasked++;
                        numValid--;
                    }
                }
                // High pixels
                for (int k = pixels->n - 1, numMasked = 0; numMasked < numHigh && k >= 0; k--) {
                    // Don't count the ones that are already masked
                    if (!maskData[indexData[k]]) {
                        maskData[indexData[k]] = 1;
                        numMasked++;
                        numValid--;
                    }
                }
            }
            counts->data.U16[yOut][xOut] = numValid;

            // XXXXX this step probably is very expensive : convert errors to variance everywhere?
            if (params->variances) {
                errors = (psVector*)psUnaryOp(errors, variances, "sqrt");
            }

            // Combination
            if (!psVectorStats(stats, pixels, errors, mask, 1)) {
		psError(PS_ERR_UNKNOWN, false, "error in pixel stats");
		return false;
	    }
	    
	    outputImage[yOut][xOut] = psStatsGetValue(stats, params->combine);

	    if (!isfinite(outputImage[yOut][xOut])) {
		pmReadoutCombineVisualPixels(pixels, mask, outputImage[yOut][xOut]);
	    }

	    if (isnan(outputImage[yOut][xOut])) {
                outputImage[yOut][xOut] = NAN;
                outputMask[yOut][xOut] = params->blank;
                sigma->data.F32[yOut][xOut] = NAN;
                if (params->variances) {
                    outputVariance[yOut][xOut] = NAN;
                }
		continue;
	    }
	    outputMask[yOut][xOut] = 0;
	    sigma->data.F32[yOut][xOut] = psStatsGetValue(stats, combineStdev);
	    if (params->variances) {
		float stdev = psStatsGetValue(stats, combineStdev);
		outputVariance[yOut][xOut] = PS_SQR(stdev); // Variance
		// XXXX this is not the correct formal error.
		// also, the weighted mean is not obviously the correct thing here
	    }
        }
    }
    #ifdef SHOW_BUSY
    if (psTraceGetLevel("psModules.imcombine") > 9) {
        printf("\n");
    }
    #endif

    psFree(index);
    psFree(pixels);
    psFree(mask);
    psFree(variances);
    psFree(errors);
    psFree(stats);
    psFree(invScale);

    // fprintf (stderr, "done with combine %x : %f sec\n", (unsigned int) id, psTimerMark (name));
    return true;
}

#if (HAVE_KAPA)
#include <kapa.h>
#include "pmKapaPlots.h"
#include "pmVisual.h"

static int kapa = -1;
static bool plotFlag = true;

// this init function only gets the ordinates for the first readout...
bool pmReadoutCombineVisualInit(void) {
    
    if (!pmVisualIsVisual()) return true;

    // skip if we have already opened the windows (or if none are requested...)
    if (kapa != -1) return true;

    pmVisualInitWindow(&kapa, "ppmerge");
    return true;
}

bool pmReadoutCombineVisualPixels(psVector *pixels, psVector *mask, float mean) {

    Graphdata graphdata;
    float xline[2], yline[2];
    
    if (!pmVisualIsVisual()) return true;

    if (!plotFlag) return true;

    KapaInitGraph(&graphdata);

    psVector *xAll = psVectorAlloc(pixels->n, PS_TYPE_F32);
    psVector *xSub = psVectorAlloc(pixels->n, PS_TYPE_F32);
    psVector *ySub = psVectorAlloc(pixels->n, PS_TYPE_F32);

    // generate vectors of the unmasked values
    int nSub = 0;
    for (int j = 0; j < pixels->n; j++) {
	xAll->data.F32[j] = j;
	if (mask && mask->data.PS_TYPE_VECTOR_MASK_DATA[j]) continue;
	xSub->data.F32[nSub] = j;
	ySub->data.F32[nSub] = pixels->data.F32[j];
	nSub ++;
    }
    xSub->n = ySub->n = nSub;
    xAll->n = pixels->n;
	
    xline[0] = 0;
    xline[1] = pixels->n;
    yline[0] = mean;
    yline[1] = mean;

    // plot the unmasked values
    pmVisualScaleGraphdata (&graphdata, xAll, pixels, false);
    KapaSetGraphData(kapa, &graphdata);
    KapaSetLimits(kapa, &graphdata);
    KapaClearPlots (kapa);

    KapaSetFont (kapa, "courier", 14);
    KapaBox (kapa, &graphdata);
    KapaSendLabel (kapa, "ordinate", KAPA_LABEL_XM);
    KapaSendLabel (kapa, "pixel values", KAPA_LABEL_YM);

    graphdata.color = KapaColorByName("black");
    graphdata.style = KAPA_PLOT_POINTS;
    graphdata.ptype = KAPA_POINT_CROSS;
    KapaPrepPlot  (kapa, xSub->n, &graphdata);
    KapaPlotVector(kapa, xSub->n, xSub->data.F32, "x");
    KapaPlotVector(kapa, xSub->n, ySub->data.F32, "y");

    graphdata.color = KapaColorByName("red");
    graphdata.style = KAPA_PLOT_POINTS;
    graphdata.ptype = KAPA_POINT_CIRCLE_OPEN;
    KapaPrepPlot  (kapa, xAll->n, &graphdata);
    KapaPlotVector(kapa, xAll->n, xAll->data.F32, "x");
    KapaPlotVector(kapa, xAll->n, pixels->data.F32, "y");

    graphdata.color = KapaColorByName("blue");
    graphdata.style = KAPA_PLOT_CONNECT;
    graphdata.ptype = KAPA_POINT_CIRCLE_OPEN;
    KapaPrepPlot  (kapa, 2, &graphdata);
    KapaPlotVector(kapa, 2, xline, "x");
    KapaPlotVector(kapa, 2, yline, "y");

    pmVisualAskUser (&plotFlag);
    return true;
}

bool pmReadoutCombineVisualCleanup(void) {

    if (!pmVisualIsVisual()) return true;
    if (kapa == -1) return true;

    KapaClose(kapa);
    return true;
}

# else

bool pmReadoutCombineVisualInit(void) { return true; }
bool pmReadoutCombineVisualPixels(psVector *pixels, psVector *mask, float mean) { return true; }
bool pmReadoutCombineVisualCleanup(void) { return true; }

# endif
