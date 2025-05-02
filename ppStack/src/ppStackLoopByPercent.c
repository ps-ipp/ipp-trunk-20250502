#include "ppStack.h"

bool ppStackLoopByPercent(pmConfig *config, ppStackOptions *options)
{
    assert(config);

    // Convolve inputs (this must be called even if the convolution is turned off because
    // it performs the normalization of the inputs).  
    psTrace("ppStack", 1, "Convolving inputs to target PSF....\n");
    if (!ppStackConvolve(options, config)) {
        psError(psErrorCodeLast(), false, "Unable to convolve images.");
        return false;
    }
    psLogMsg("ppStack", PS_LOG_INFO, "Stage 2: Generate Convolutions and Save: %f sec", psTimerClear("PPSTACK_STEPS"));
    ppStackMemDump("convolve");
    if (options->quality) return true; // Can't do anything else

    // Define threading elements
    ppStackThreadData *stack = ppStackThreadDataSetup(options, config, true);
    if (!stack) {
        psError(psErrorCodeLast(), false, "Unable to initialise stack threads.");
        return false;
    }

    // Prepare for combination (define outputs, generate output cells)
    // NOTE: expmaps will be skipped if "PPSTACK.OUTPUT.EXP" -> NULL
    ppStackFileList stackFiles = options->convolve     ? PPSTACK_FILES_STACK       : PPSTACK_FILES_UNCONV;
    char *outName              = options->convolve     ? "PPSTACK.OUTPUT"          : "PPSTACK.UNCONV";
    char *expName              = options->convolve     ? "PPSTACK.OUTPUT.EXP"      : "PPSTACK.UNCONV.EXP";
    char *bckName              = options->doBackground ? "PPSTACK.OUTPUT.BKGMODEL" : NULL;

    if (!ppStackCombinePrepare(outName, expName, bckName, stackFiles, stack, options, config)) {
        psError(psErrorCodeLast(), false, "Unable to prepare for combination.");
        psFree(stack);
        return false;
    }

    // Initial combination
    psTrace("ppStack", 1, "Initial stack of convolved images....\n");
    if (!ppStackCombinePercent(stack, options, config)) {
        psError(psErrorCodeLast(), false, "Unable to perform initial combination.");
        psFree(stack);
        return false;
    }
    ppStackMemDump("percent");
    psLogMsg("ppStack", PS_LOG_INFO, "Stage 3: Make Initial Stack: %f sec", psTimerClear("PPSTACK_STEPS"));

    // Done with stack inputs for now
    for (int i = 0; i < options->num; i++) {
        pmCellFreeData(options->cells->data[i]);
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
    if (!ppStackCleanupFiles(stack, options, config, stackFiles, PPSTACK_FILES_PHOT, true)) {
        psError(psErrorCodeLast(), false, "Unable to clean up.");
        psFree(stack);
        return false;
    }
    psFree(stack);
    psLogMsg("ppStack", PS_LOG_INFO, "Stage 7: Cleanup, WCS & JPEGS: %f sec", psTimerClear("PPSTACK_STEPS"));
    ppStackMemDump("cleanup");

    // Unconvolved stack --- it's cheap to calculate, compared to everything else!
    // XXX unconvolved stack is currently using the convolved mask!  oops! (EAM: is this true?)
    if (options->convolve) {
        // Start threading
        ppStackThreadData *stack = ppStackThreadDataSetup(options, config, false);
        if (!stack) {
            psError(psErrorCodeLast(), false, "Unable to initialise stack threads.");
            return false;
        }

        // Prepare for combination
        if (!ppStackCombinePrepare("PPSTACK.UNCONV", "PPSTACK.UNCONV.EXP", NULL, PPSTACK_FILES_UNCONV, stack, options, config)) {
            psError(psErrorCodeLast(), false, "Unable to prepare for combination.");
            psFree(stack);
            return false;
        }

	// generate the unconvolved stack. NOTE: this one must be normalized since the inputs have not been
        psTrace("ppStack", 2, "Stack of unconvolved images....\n");
        if (!ppStackCombinePercent(stack, options, config)) {
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

