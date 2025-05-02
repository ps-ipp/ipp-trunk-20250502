#include "ppStack.h"

// static functions are defined below
/* static int stackSummary(const ppStackOptions *options, const char *place); */

bool ppStackMedianLoop(pmConfig *config, ppStackOptions *options)
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

    // Duplicate code that is in ppStackConvolve
    options->cells = psArrayAlloc(options->num); // Cells for convolved images --- a handle for reading again
    pmFPAview *view = ppStackFilesIterateDown(config);
    for (int i = 0; i < options->num; i++) {
      ppStackFileActivationSingle(config, PPSTACK_FILES_MEDIAN_IN, true, i);
      pmFPAfile *file = pmFPAfileSelectSingle(config->files, "PPSTACK.INPUT", i); // File of interest

      //      pmFPAview *view = pmFPAviewAlloc(0);// Pointer into FPA hierarchy
      if (!view) {
	return false;
      }
      pmReadout *readout = pmFPAviewThisReadout(view, file->fpa); // Input readout
      pmCell *inCell = readout->parent; // Input cell */
      options->cells->data[i] = psMemIncrRefCounter(inCell);

    }
    psFree(view);
    
    ppStackThreadData *stack = ppStackThreadDataSetup(options, config, options->convolve);
    if (!stack) {
      psError(psErrorCodeLast(), false, "Unable to initialise stack threads.");
      return false;
    }
    
    // Preparation for stacking
    if (!ppStackCombinePrepare("PPSTACK.OUTPUT",NULL,NULL,PPSTACK_FILES_MEDIAN_OUT,stack,options,config)) {
      psError(psErrorCodeLast(), false, "Unabel to combine data.");
      psFree(stack);
      return false;
    }
      
    if (!ppStackCombineMedian(stack, options, config)) {
      psError(psErrorCodeLast(), false, "Unabel to combine data.");
      psFree(stack);
      return false;
    }
/*     for (int x = 0; x < options->outRO->image->numCols; x++) { */
/*       for (int y = 0; y < options->outRO->image->numRows; y++) { */
/* 	options->outRO->mask->data.PS_TYPE_IMAGE_MASK_DATA[y][x] = 0; */
/*       } */
/*     } */
    psFree(options->outRO->mask);
    // Do this before performing photometry so that the cmf header gets all of the information.
    options->zpInput = NULL;
    options->expTimeInput = NULL;
    options->airmassInput = NULL;
    if (!ppStackUpdateHeader(stack, options, config)) {
        psError(psErrorCodeLast(), false, "Unable to update header.");
        psFree(stack);
        return false;
    }
/*     // Generate median background stack here. */
/*     if (!ppStackCombineBackground(stack, options, config)) { */
/*       psError(psErrorCodeLast(), false, "Unable to generate median of background images."); */
/*       psFree(stack); */
/*       return false; */
/*     } */
//    ppStackFileActivation(config,PPSTACK_FILES_BKG ,false);    
/*     // Photometry */
/*     psTrace("ppStack", 1, "Photometering stacked image....\n"); */
/*     if (!ppStackPhotometry(options, config)) { */
/*         psError(psErrorCodeLast(), false, "Unable to perform photometry."); */
/*         return false; */
/*     } */
/*     psLogMsg("ppStack", PS_LOG_INFO, "Stage 6: Photometry Analysis: %f sec", psTimerClear("PPSTACK_STEPS")); */
/*     ppStackMemDump("photometry"); */

    // Create JPEGS
/*     if (!ppStackJPEGs(stack, options, config)) { */
/*         psError(psErrorCodeLast(), false, "Unable to make jpegs."); */
/*         psFree(stack); */
/*         return false; */
/*     } */
/*     // Assemble Stats */
/*     if (!ppStackStats(stack, options, config)) { */
/*         psError(psErrorCodeLast(), false, "Unable to assemble statistics."); */
/*         psFree(stack); */
/*         return false; */
/*     } */

   // Clean up
    
    psTrace("ppStack", 2, "Cleaning up after combination....\n");
    if (!ppStackCleanupFiles(stack, options, config, PPSTACK_FILES_MEDIAN_OUT, PPSTACK_FILES_MEDIAN_OUT, false)) {
        psError(psErrorCodeLast(), false, "Unable to clean up.");
        psFree(stack);
        return false;
    }
    //    psFree(stack);
    psLogMsg("ppStack", PS_LOG_INFO, "Stage 7: Cleanup, WCS & JPEGS: %f sec", psTimerClear("PPSTACK_STEPS"));
    ppStackMemDump("cleanup");

/*     // Unconvolved stack --- it's cheap to calculate, compared to everything else! */
/*     // XXX unconvolved stack is currently using the convolved mask!  oops! */
/*     if (options->convolve) { */
/*         // Start threading */
/*         ppStackThreadData *stack = ppStackThreadDataSetup(options, config, false); */
/*         if (!stack) { */
/*             psError(psErrorCodeLast(), false, "Unable to initialise stack threads."); */
/*             return false; */
/*         } */

/*         // Prepare for combination */
/*         if (!ppStackCombinePrepare("PPSTACK.UNCONV", "PPSTACK.UNCONV.EXP", NULL, PPSTACK_FILES_UNCONV, */
/*                                    stack, options, config)) { */
/*             psError(psErrorCodeLast(), false, "Unable to prepare for combination."); */
/*             psFree(stack); */
/*             return false; */
/*         } */

/* 	// generate the unconvolved stack. NOTE: this one must be normalized since the inputs have not been */
/*         psTrace("ppStack", 2, "Stack of unconvolved images....\n"); */
/*         if (!ppStackCombineFinal(stack, options->origCovars, options, config, false, true, false)) { */
/*             psError(psErrorCodeLast(), false, "Unable to perform unconvolved combination."); */
/*             psFree(stack); */
/*             return false; */
/*         } */
/*         psLogMsg("ppStack", PS_LOG_INFO, "Stage 8: Unconvolved Stack: %f sec", psTimerClear("PPSTACK_STEPS")); */
/*         ppStackMemDump("unconv"); */

/* 	// Update Header */
/* 	if (!ppStackUpdateHeader(stack, options, config)) { */
/* 	    psError(psErrorCodeLast(), false, "Unable to update header."); */
/* 	    psFree(stack); */
/* 	    return false; */
/* 	} */
/* 	// Clean up unconvolved stack */
/* 	psTrace("ppStack", 2, "Cleaning up after unconvolved stack....\n"); */
/* 	if (!ppStackCleanupFiles(stack, options, config, PPSTACK_FILES_UNCONV, PPSTACK_FILES_NONE, false)) { */
/* 	    psError(psErrorCodeLast(), false, "Unable to clean up."); */
/* 	    psFree(stack); */
/* 	    return false; */
/* 	} */
/* 	psFree(stack); */
/*     } */
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

/* /// Print a summary of the inputs, and return the number of good inputs */
/* static int stackSummary(const ppStackOptions *options, const char *place) */
/* { */
/*     int numGood = 0;                // Number of good inputs */
/*     psString summary = NULL;        // Summary of images */
/*     for (int i = 0; i < options->num; i++) { */
/*         psString reason = NULL;         // Reason for rejecting */
/*         if (options->inputMask->data.PS_TYPE_VECTOR_MASK_DATA[i] == 0) { */
/*             psStringAppend(&reason, " Good."); */
/*             numGood++; */
/*         } else { */
/*             if (options->inputMask->data.PS_TYPE_VECTOR_MASK_DATA[i] & PPSTACK_MASK_CAL) { */
/*                 psStringAppend(&reason, " Calibration failed."); */
/*             } */
/*             if (options->inputMask->data.PS_TYPE_VECTOR_MASK_DATA[i] & PPSTACK_MASK_PSF) { */
/*                 psStringAppend(&reason, " PSF measurement failed."); */
/*             } */
/*             if (options->inputMask->data.PS_TYPE_VECTOR_MASK_DATA[i] & PPSTACK_MASK_MATCH) { */
/*                 psStringAppend(&reason, " PSF matching failed."); */
/*             } */
/*             if (options->inputMask->data.PS_TYPE_VECTOR_MASK_DATA[i] & PPSTACK_MASK_CHI2) { */
/*                 psStringAppend(&reason, " PSF matching chi^2 deviant."); */
/*             } */
/*             if (options->inputMask->data.PS_TYPE_VECTOR_MASK_DATA[i] & PPSTACK_MASK_REJECT) { */
/*                 psStringAppend(&reason, " Rejection exceeded threshold."); */
/*             } */
/*         } */
/*         psStringAppend(&summary, "Image %d: %s\n", i, reason); */
/*         psFree(reason); */
/*     } */
/*     psLogMsg("ppStack", PS_LOG_INFO, "Summary of images for %s:\n%s", place, summary); */
/*     psFree(summary); */

/*     return numGood; */
/* } */
