#include "ppStack.h"

// static functions are defined below
static int stackSummary(const ppStackOptions *options, const char *place);
bool pmConfigDumpRecipe (pmConfig *config, char *filename);

bool ppStackLoop(pmConfig *config, ppStackOptions *options)
{
    assert(config);

    psTimerStart("PPSTACK_TOTAL");
    psTimerStart("PPSTACK_STEPS");

    // Setup
    psTrace("ppStack", 1, "Setup....\n");
    if (!ppStackSetup(options, config)) {
        psError(psErrorCodeLast(), false, "Unable to setup.");
        return false;
    }
    psLogMsg("ppStack", PS_LOG_INFO, "Stage 0: Setup: %f sec", psTimerClear("PPSTACK_STEPS"));
    ppStackMemDump("setup");

    // Preparation for stacking
    psTrace("ppStack", 1, "Preparation for stacking: merging sources, determining target PSF....\n");

    if (!ppStackPrepare(options, config)) {
        psError(psErrorCodeLast(), false, "Unable to prepare for stacking.");
        return false;
    }
    psLogMsg("ppStack", PS_LOG_INFO, "Stage 1: Load Sources and Generate Target PSF: %f sec", psTimerClear("PPSTACK_STEPS"));
    ppStackMemDump("prepare");
    if (options->quality) return true; // Can't do anything else

    // if we want to skip the rejection steps, and instead use a percentile clipping:
    if (options->clipPercent) {
      bool result = ppStackLoopByPercent (config, options);
      return result;
    }

    // Convolve inputs
    psTrace("ppStack", 1, "Convolving inputs to target PSF....\n");
    if (!ppStackConvolve(options, config)) {
        psError(psErrorCodeLast(), false, "Unable to convolve images.");
        return false;
    }
    psLogMsg("ppStack", PS_LOG_INFO, "Stage 2: Generate Convolutions and Save: %f sec", psTimerClear("PPSTACK_STEPS"));
    ppStackMemDump("convolve");
    if (options->quality) return true; // Can't do anything else

    psMetadata *recipe = psMetadataLookupMetadata(NULL, config->recipes, PPSTACK_RECIPE); // ppStack recipe

    // Ensure sufficient inputs
    {
        int numGood = stackSummary(options, "initial combination");
        bool safe = psMetadataLookupBool(NULL, recipe, "SAFE"); // Be safe when combining
        if (safe && numGood <= 1) {
            options->quality = PPSTACK_ERR_REJECTED;
            psErrorStackPrint(stderr, "Insufficient inputs for combination with safety on");
            psErrorClear();
            psWarning("Insufficient inputs for combination with safety on");
            return true;
        }
    }

    // Define threading elements
    ppStackThreadData *stack = ppStackThreadDataSetup(options, config, true);
    if (!stack) {
        psError(psErrorCodeLast(), false, "Unable to initialise stack threads.");
        return false;
    }

    //    bool doBackground = psMetadataLookupBool(NULL, recipe, "BACKGROUND.MODEL");

    // Prepare for combination
    if (!ppStackCombinePrepare("PPSTACK.OUTPUT", "PPSTACK.OUTPUT.EXP", options->doBackground ? "PPSTACK.OUTPUT.BKGMODEL" : NULL, PPSTACK_FILES_STACK, stack, options, config)) {
        psError(psErrorCodeLast(), false, "Unable to prepare for combination.");
        psFree(stack);
        return false;
    }

    // Initial combination
    psTrace("ppStack", 1, "Initial stack of convolved images....\n");
    if (!ppStackCombineInitial(stack, options, config)) {
        psError(psErrorCodeLast(), false, "Unable to perform initial combination.");
        psFree(stack);
        return false;
    }
    ppStackMemDump("initial");
    psLogMsg("ppStack", PS_LOG_INFO, "Stage 3: Make Initial Stack: %f sec", psTimerClear("PPSTACK_STEPS"));

    // Done with stack inputs for now
    // XXX is this where we are leaking??
    for (int i = 0; i < options->num; i++) {
        pmCellFreeData(options->cells->data[i]);
    }
    // MEH -- must uncomment back out -- unclear is should be moved after pixel rejection
    psFree(stack);

    // Pixel rejection
    psTrace("ppStack", 1, "Reject pixels....\n");
    if (!ppStackReject(options, config)) {
        psError(psErrorCodeLast(), false, "Unable to reject pixels.");
        psFree(stack);
        return false;
    }
    psLogMsg("ppStack", PS_LOG_INFO, "Stage 4: Pixel Rejection: %f sec", psTimerClear("PPSTACK_STEPS"));

    // Check inputs
    {
        int numGood = stackSummary(options, "final combination");
        if (numGood <= 0) {
            options->quality = PPSTACK_ERR_REJECTED;
            psErrorStackPrint(stderr, "Insufficient inputs survived rejection stage");
            psErrorClear();
            psWarning("Insufficient inputs survived rejection stage");
            return true;
        }
    }

    stack = ppStackThreadDataSetup(options, config, true);
    if (!stack) {
        psError(psErrorCodeLast(), false, "Unable to initialise stack threads.");
        return false;
    }

    // Final combination.  This one does NOT need to be normalized since the convolution takes care of that
    psTrace("ppStack", 2, "Final stack of convolved images....\n");
    if (options->convolve) {
      if (!ppStackCombineFinal(stack, options->convCovars, options, config, false, false, true, false)) {
        psError(psErrorCodeLast(), false, "Unable to perform final combination.");
        psFree(stack);
        return false;
      }
    } else {
      // Since we haven't convolved, I believe we do need to normalize here.
      // MEH -- see below for comment on ppStackCombineFinal and bscaleOffset
      // last 4 parameters are: safe, normalize, grow, bscaleoffset
      // EAM : looks like we do NOT need to normalize here eithr because ppStackConvolve is still called and
      // the inputs are normalized
      if (!ppStackCombineFinal(stack, options->convCovars, options, config, false, false, true, true)) {
        psError(psErrorCodeLast(), false, "Unable to perform final combination.");
        psFree(stack);
        return false;
      }
    }
      
    psLogMsg("ppStack", PS_LOG_INFO, "Stage 5: Final Stack: %f sec", psTimerClear("PPSTACK_STEPS"));
    ppStackMemDump("final");

    // Update Header
    // Do this before performing photometry so that the cmf header gets all of the information.
    if (!ppStackUpdateHeader(stack, options, config)) {
        psError(psErrorCodeLast(), false, "Unable to update header.");
        psFree(stack);
        return false;
    }

    // Generate median background stack here.
    if (options->doBackground && !ppStackCombineBackground(stack, options, config)) {
      psError(psErrorCodeLast(), false, "Unable to generate median of background images.");
      psFree(stack);
      return false;
    }
    ppStackFileActivation(config, PPSTACK_FILES_BKG, false);    

    // Photometry
    psTrace("ppStack", 1, "Photometering stacked image....\n");
    if (!ppStackPhotometry(options, config)) {
        psError(psErrorCodeLast(), false, "Unable to perform photometry.");
        return false;
    }
    psLogMsg("ppStack", PS_LOG_INFO, "Stage 6: Photometry Analysis: %f sec", psTimerClear("PPSTACK_STEPS"));
    ppStackMemDump("photometry");

    // Create JPEGS
    if (!ppStackJPEGs(stack, options, config)) {
        psError(psErrorCodeLast(), false, "Unable to make jpegs.");
        psFree(stack);
        return false;
    }
    // Assemble Stats
    if (!ppStackStats(stack, options, config)) {
        psError(psErrorCodeLast(), false, "Unable to assemble statistics.");
        psFree(stack);
        return false;
    }

   // Clean up
    psTrace("ppStack", 2, "Cleaning up after combination....\n");
    if (!ppStackCleanupFiles(stack, options, config, PPSTACK_FILES_STACK, PPSTACK_FILES_PHOT, true)) {
        psError(psErrorCodeLast(), false, "Unable to clean up.");
        psFree(stack);
        return false;
    }
    // MEH -- also must uncomment back out..
    psFree(stack);
    psLogMsg("ppStack", PS_LOG_INFO, "Stage 7: Cleanup, WCS & JPEGS: %f sec", psTimerClear("PPSTACK_STEPS"));
    ppStackMemDump("cleanup");

    // Unconvolved stack --- it's cheap to calculate, compared to everything else!
    // XXX unconvolved stack is currently using the convolved mask!  oops!
    if (options->convolve) {
        // Start threading
        ppStackThreadData *stack = ppStackThreadDataSetup(options, config, false);
        if (!stack) {
            psError(psErrorCodeLast(), false, "Unable to initialise stack threads.");
            return false;
        }

        // Prepare for combination
        if (!ppStackCombinePrepare("PPSTACK.UNCONV", "PPSTACK.UNCONV.EXP", NULL, PPSTACK_FILES_UNCONV,
                                   stack, options, config)) {
            psError(psErrorCodeLast(), false, "Unable to prepare for combination.");
            psFree(stack);
            return false;
        }

	// generate the unconvolved stack. NOTE: this one must be normalized since the inputs have not been
        psTrace("ppStack", 2, "Stack of unconvolved images....\n");
        //MEH -- terrible hack for bscale offset to input warps -- treat like normalization 
        // -- if BSCALEOFFSET TRUE, 0.5*bscale for offset values, otherwise value 0.0
        // -- bscaleOffset set when files read in and applied in CombineFinal (if arg true, only valid for unconv case)
        if (!ppStackCombineFinal(stack, options->origCovars, options, config, false, true, false, true)) {
            psError(psErrorCodeLast(), false, "Unable to perform unconvolved combination.");
            psFree(stack);
            return false;
        }
        psLogMsg("ppStack", PS_LOG_INFO, "Stage 8: Unconvolved Stack: %f sec", psTimerClear("PPSTACK_STEPS"));
        ppStackMemDump("unconv");

	// Update Header
	if (!ppStackUpdateHeader(stack, options, config)) {
	    psError(psErrorCodeLast(), false, "Unable to update header.");
	    psFree(stack);
	    return false;
	}
	// Clean up unconvolved stack
	psTrace("ppStack", 2, "Cleaning up after unconvolved stack....\n");
	if (!ppStackCleanupFiles(stack, options, config, PPSTACK_FILES_UNCONV, PPSTACK_FILES_NONE, false)) {
	    psError(psErrorCodeLast(), false, "Unable to clean up.");
	    psFree(stack);
	    return false;
	}
	psFree(stack);
    }
    psFree(options->cells); options->cells = NULL;

    // Finish up
    psTrace("ppStack", 1, "Finishing up....\n");
    if (!ppStackFinish(options, config)) {
        psError(psErrorCodeLast(), false, "Unable to finish up.");
        return false;
    }
    ppStackMemDump("finish");

    return true;
}

/// Print a summary of the inputs, and return the number of good inputs
static int stackSummary(const ppStackOptions *options, const char *place)
{
    int numGood = 0;                // Number of good inputs
    psString summary = NULL;        // Summary of images
    for (int i = 0; i < options->num; i++) {
        psString reason = NULL;         // Reason for rejecting
        if (options->inputMask->data.PS_TYPE_VECTOR_MASK_DATA[i] == 0) {
            psStringAppend(&reason, " Good.");
            numGood++;
        } else {
            if (options->inputMask->data.PS_TYPE_VECTOR_MASK_DATA[i] & PPSTACK_MASK_CAL) {
                psStringAppend(&reason, " Calibration failed.");
            }
            if (options->inputMask->data.PS_TYPE_VECTOR_MASK_DATA[i] & PPSTACK_MASK_PSF) {
                psStringAppend(&reason, " PSF measurement failed.");
            }
            if (options->inputMask->data.PS_TYPE_VECTOR_MASK_DATA[i] & PPSTACK_MASK_MATCH) {
                psStringAppend(&reason, " PSF matching failed.");
            }
            if (options->inputMask->data.PS_TYPE_VECTOR_MASK_DATA[i] & PPSTACK_MASK_CHI2) {
                psStringAppend(&reason, " PSF matching chi^2 deviant.");
            }
            if (options->inputMask->data.PS_TYPE_VECTOR_MASK_DATA[i] & PPSTACK_MASK_REJECT) {
                psStringAppend(&reason, " Rejection exceeded threshold.");
            }
        }
        psStringAppend(&summary, "Image %d: %s\n", i, reason);
        psFree(reason);
    }
    psLogMsg("ppStack", PS_LOG_INFO, "Summary of images for %s:\n%s", place, summary);
    psFree(summary);

    return numGood;
}

// Test function if needed:
bool pmConfigDumpRecipe (pmConfig *config, char *filename) {

    psMetadata *recipe = psMetadataLookupMetadata(NULL, config->recipes, PPSTACK_RECIPE); // Recipe for ppSim
    psAssert (recipe, "oops");

    psMetadataConfigWrite (recipe, filename, NULL);

    return true;
}

