#include "ppStack.h"

// This is the doomsday switch.
// # define TESTING                         // Enable test output

//MEH -- adhoc addition to blank mask border of final stack since rejection different/none on order of KERNEL.SIZE with overlap in CombineInitial 
static void stackBorderMask(psImage *image, // Image to mark as blank
                            psImage *mask, // Mask to mark as blank (or NULL)
                            psImage *variance, // Weight map to mark as blank (or NULL)
                            int numCols, int numRows, // Size of image
                            int size, // Size to mark blank
                            psImageMaskType blank // Blank mask value
    )
{
    for (int y = size; y < numRows - size; y++) {
        for (int x = 0; x < size; x++) {
            image->data.F32[y][x] = NAN;
            mask->data.PS_TYPE_IMAGE_MASK_DATA[y][x] = blank;
            variance->data.F32[y][x] = NAN;
        }
        for (int x = numCols - size; x < numCols; x++) {
            image->data.F32[y][x] = NAN;
            mask->data.PS_TYPE_IMAGE_MASK_DATA[y][x] = blank;
            variance->data.F32[y][x] = NAN;
        }
    }
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < numCols; x++) {
            image->data.F32[y][x] = NAN;
            mask->data.PS_TYPE_IMAGE_MASK_DATA[y][x] = blank;
            variance->data.F32[y][x] = NAN;
        }
    }
    for (int y = numRows - size; y < numRows; y++) {
        for (int x = 0; x < numCols; x++) {
            image->data.F32[y][x] = NAN;
            mask->data.PS_TYPE_IMAGE_MASK_DATA[y][x] = blank;
            variance->data.F32[y][x] = NAN;
        }
    }
    return;
}


bool ppStackCombineFinal(ppStackThreadData *stack, psArray *covariances, ppStackOptions *options,
                         pmConfig *config, bool safe, bool normalise, bool grow, bool bscaleoffset)
{
    psAssert(stack, "Require stack");
    psAssert(options, "Require options");
    psAssert(config, "Require configuration");

    psTimerStart("PPSTACK_FINAL");
    
    pmReadout *outRO = options->outRO;                                      // Output readout
    pmReadout *expRO = options->expRO;                                      // Exposure readout
    int numCols = outRO->image->numCols, numRows = outRO->image->numRows; // Size of image

    psMetadata *recipe = psMetadataLookupMetadata(NULL, config->recipes, PPSTACK_RECIPE); // ppStack recipe
    psAssert(recipe, "We've thrown an error on this before.");
    float poorFrac = psMetadataLookupF32(NULL, recipe, "POOR.FRACTION"); // Fraction for "poor"

    int sizeBlank = psMetadataLookupS32(NULL, recipe, "MASK.BLANKBORDER"); // Pixels to mask BLANK from edge 
    psImageMaskType maskBlank = pmConfigMaskGet("BLANK", config); // Bits to mask for bad pixels

    // Grow the list of rejected pixels, if desired
    psArray *reject = psArrayAlloc(options->num); // Pixels rejected for each image
    for (int i = 0; i < options->num; i++) {
        if (options->inputMask->data.U8[i]) {
            continue;
        }
        if (grow) {
            reject->data[i] = pmStackRejectGrow(options->rejected->data[i], numCols, numRows, poorFrac,
                                                options->regions->data[i], options->kernels->data[i]);
        } else {
            reject->data[i] = psMemIncrRefCounter(options->rejected->data[i]);
        }
    }

    if (!outRO->mask) {
        outRO->mask = psImageAlloc(numCols, numRows, PS_TYPE_IMAGE_MASK);
    }
    if (!expRO->mask) {
        expRO->mask = psImageAlloc(numCols, numRows, PS_TYPE_IMAGE_MASK);
    }

    stack->lastScan = 0;            // Reset read
    bool status;                    // Status of read
    for (int numChunk = 0; true; numChunk++) {
        ppStackThread *thread = ppStackThreadRead(&status, stack, config, numChunk, 0);
        if (!status) {
            // Something went wrong
            psError(psErrorCodeLast(), false, "Unable to read chunk %d", numChunk);
            psFree(reject);
            return false;
        }
        if (!thread) {
            // Nothing more to read
            break;
        }

        // calls ppStackReadoutFinal(config, outRO, readouts, rejected) in ppStackReadout.c
        psThreadJob *job = psThreadJobAlloc("PPSTACK_FINAL_COMBINE"); // Job to start
        psArrayAdd(job->args, 1, thread);
        psArrayAdd(job->args, 1, reject);
        psArrayAdd(job->args, 1, options);
        psArrayAdd(job->args, 1, config);
        PS_ARRAY_ADD_SCALAR(job->args, safe, PS_TYPE_U8);
        PS_ARRAY_ADD_SCALAR(job->args, normalise, PS_TYPE_U8);
        PS_ARRAY_ADD_SCALAR(job->args, bscaleoffset, PS_TYPE_U8);
        if (!psThreadJobAddPending(job)) {
            psFree(reject);
            return false;
        }
    }

    if (!psThreadPoolWait(true, true)) {
        psError(psErrorCodeLast(), false, "Unable to do final combination.");
        psFree(reject);
        return false;
    }

    psFree(reject);

    // Sum covariance matrices
    // the array may be defined, but no covariances actually supplied.
    bool haveCovariances = false;
    if (covariances) {
	for (int i = 0; i < covariances->n; i++) {
	    haveCovariances |= (covariances->data[i] != NULL);
	}
    }

    if (haveCovariances) {
        outRO->covariance = psImageCovarianceAverageWeighted(covariances, options->weightings);
    } else {
        outRO->covariance = psImageCovarianceNone();
    }

#ifdef TESTING
    static int pass = 0;                // Pass through
    psString name = NULL;               // Name of file
    psStringAppend(&name, "combined_image_final_%d.fits", pass);
    pass++;
    ppStackWriteImage(name, NULL, outRO->image, config);
    psStringSubstitute(&name, "mask", "image");
    ppStackWriteImage(name, NULL, outRO->mask, config);
    psStringSubstitute(&name, "variance", "mask");
    ppStackWriteImage(name, NULL, outRO->variance, config);
    psFree(name);

    pmStackVisualPlotTestImage(outRO->image, "combined_image_final.fits");
#endif

    //MEH blank mask/manual reject border on final stack -- 
    if (sizeBlank > 0) {
        stackBorderMask(outRO->image,outRO->mask,outRO->variance,numCols,numRows,sizeBlank,maskBlank);
        stackBorderMask(expRO->image,expRO->mask,expRO->variance,numCols,numRows,sizeBlank,0);
    }

    if (options->stats) {
        // only add the timer if it has not been set (convolved stack)
        // XXX this is weak: we should be distinguishing explicitly between convolved and unconvolved stacks
        psMetadataLookupF32 (&status, options->stats, "TIME_FINAL");
        if (!status) {
	  psMetadataAddF32(options->stats, PS_LIST_TAIL, "TIME_FINAL", PS_META_REPLACE, "Time to make final stack", psTimerMark("PPSTACK_FINAL"));
	}
    }
    
    return true;
}
