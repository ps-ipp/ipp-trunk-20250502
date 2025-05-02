#include "ppStack.h"

# define USE_THREADS 1
// #define TESTING

bool ppStackCombinePercent(ppStackThreadData *stack, ppStackOptions *options, pmConfig *config)
{
    psAssert(stack, "Require stack");
    psAssert(options, "Require options");
    psAssert(config, "Require configuration");

    psTimerStart("PPSTACK_PERCENT");

    bool status;                    // Status of read
    int numChunk;                   // Number of chunks
    for (numChunk = 0; true; numChunk++) {
        ppStackThread *thread = ppStackThreadRead(&status, stack, config, numChunk, 0);
        if (!status) {
            // Something went wrong
            psError(psErrorCodeLast(), false, "Unable to read chunk %d", numChunk);
            return false;
        }
        if (!thread) {
            // Nothing more to read
            break;
        }

        psThreadJob *job = psThreadJobAlloc("PPSTACK_PERCENT_COMBINE"); // Job to start
        psArrayAdd(job->args, 1, thread);
        psArrayAdd(job->args, 1, options);
        psArrayAdd(job->args, 1, config);

	if (USE_THREADS) {
	    if (!psThreadJobAddPending(job)) {
		return false;
	    }
	} else {
	    ppStackReadoutPercentThread(job);
	}
    }
    
    if (USE_THREADS && !psThreadPoolWait(true, true)) {
        psError(psErrorCodeLast(), false, "Unable to do initial combination.");
        return false;
    }

    ppStackMemDump("initial");

#ifdef TESTING
    ppStackWriteImage("combined_image_initial.fits", NULL, options->outRO->image, config);
    ppStackWriteImage("combined_mask_initial.fits", NULL, options->outRO->mask, config);
    ppStackWriteImage("combined_variance_initial.fits", NULL, options->outRO->variance, config);

    pmStackVisualPlotTestImage(options->outRO->image, "combined_image_initial.fits");
#endif

    if (options->stats) {
      // psMetadataAddF32(options->stats, PS_LIST_TAIL, "TIME_INITIAL", PS_META_REPLACE, "Time to make initial stack", psTimerMark("PPSTACK_INITIAL"));
    }

    return true;
}
