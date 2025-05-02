#include "ppStack.h"

// This is the doomsday switch.
// #define TESTING                         // Enable test output

bool ppStackCombineInitial(ppStackThreadData *stack, ppStackOptions *options, pmConfig *config)
{
    psAssert(stack, "Require stack");
    psAssert(options, "Require options");
    psAssert(config, "Require configuration");

    psTimerStart("PPSTACK_INITIAL");

    psMetadata *ppsub = psMetadataLookupMetadata(NULL, config->recipes, "PPSUB"); // PPSUB recipe
    int overlap = 2 * psMetadataLookupS32(NULL, ppsub,
                                          "KERNEL.SIZE"); // Overlap by kernel size between consecutive scans


    bool status;                    // Status of read
    int numChunk;                   // Number of chunks
    for (numChunk = 0; true; numChunk++) {
        ppStackThread *thread = ppStackThreadRead(&status, stack, config, numChunk, overlap);
        if (!status) {
            // Something went wrong
            psError(psErrorCodeLast(), false, "Unable to read chunk %d", numChunk);
            return false;
        }
        if (!thread) {
            // Nothing more to read
            break;
        }

        psThreadJob *job = psThreadJobAlloc("PPSTACK_INITIAL_COMBINE"); // Job to start
        psArrayAdd(job->args, 1, thread);
        psArrayAdd(job->args, 1, options);
        psArrayAdd(job->args, 1, config);
        if (!psThreadJobAddPending(job)) {
            return false;
        }
    }

    if (!psThreadPoolWait(false, true)) {
        psError(psErrorCodeLast(), false, "Unable to do initial combination.");
        return false;
    }

    // Harvest the jobs, gathering the inspection lists
    options->inspect = psArrayAlloc(options->num);
    options->rejected = psArrayAlloc(options->num);
    for (int i = 0; i < options->num; i++) {
        if (options->inputMask->data.PS_TYPE_VECTOR_MASK_DATA[i]) {
            continue;
        }
        options->inspect->data[i] = psArrayAllocEmpty(numChunk);
        options->rejected->data[i] = psArrayAllocEmpty(numChunk);
    }
    psThreadJob *job;               // Completed job
    while ((job = psThreadJobGetDone())) {
        psAssert(strcmp(job->type, "PPSTACK_INITIAL_COMBINE") == 0,
                 "Job has incorrect type: %s", job->type);
        psArray *results = job->results; // Results of job
        psAssert(results->n == 2, "Results array has wrong size!");
        psArray *inspect = results->data[0]; // Pixels to inspect
        psArray *reject = results->data[1];  // Pixels to reject
        for (int i = 0; i < options->num; i++) {
            if (options->inputMask->data.PS_TYPE_VECTOR_MASK_DATA[i]) {
                continue;
            }
            options->inspect->data[i] = psArrayAdd(options->inspect->data[i], 1, inspect->data[i]);
            options->rejected->data[i] = psArrayAdd(options->rejected->data[i], 1, reject->data[i]);
        }
        psFree(job);
    }

    ppStackMemDump("initial");

#ifdef TESTING
    ppStackWriteImage("combined_image_initial.fits", NULL, options->outRO->image, config);
    ppStackWriteImage("combined_mask_initial.fits", NULL, options->outRO->mask, config);
    ppStackWriteImage("combined_variance_initial.fits", NULL, options->outRO->variance, config);

    pmStackVisualPlotTestImage(options->outRO->image, "combined_image_initial.fits");
#endif

    if (options->stats) {
        psMetadataAddF32(options->stats, PS_LIST_TAIL, "TIME_INITIAL", PS_META_REPLACE, "Time to make initial stack", psTimerMark("PPSTACK_INITIAL"));
    }

    return true;
}
