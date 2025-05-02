#include "ppStack.h"

#define THREAD_WAIT 10000               // Microseconds to wait if thread is not available

static void stackThreadFree(ppStackThread *thread)
{
    psFree(thread->readouts);
    return;
}

ppStackThread *ppStackThreadAlloc(psArray *readouts)
{
    ppStackThread *thread = psAlloc(sizeof(ppStackThread));
    psMemSetDeallocator(thread, (psFreeFunc)stackThreadFree);

    thread->readouts = psMemIncrRefCounter(readouts);
    thread->read = false;
    thread->busy = false;
    thread->firstScan = 0;
    thread->lastScan = 0;
    thread->entry = -1; // this gets assigned after allocation
    thread->status = PPSTACK_THREAD_NEW;
    return thread;
}

static void stackThreadDataFree(ppStackThreadData *stack)
{
    psFree(stack->threads);
    for (int i = 0; i < stack->imageFits->n; i++) {
        if (stack->imageFits->data[i]) {
            psFitsClose(stack->imageFits->data[i]);
        }
        if (stack->maskFits->data[i]) {
            psFitsClose(stack->maskFits->data[i]);
        }
        if (stack->varianceFits->data[i]) {
            psFitsClose(stack->varianceFits->data[i]);
        }
        stack->imageFits->data[i] = stack->maskFits->data[i] = stack->varianceFits->data[i] = NULL;
    }
    psFree(stack->imageFits);
    psFree(stack->maskFits);
    psFree(stack->varianceFits);
    psFree(stack->bkgFits);
    return;
}

ppStackThreadData *ppStackThreadDataSetup(const ppStackOptions *options, const pmConfig *config, bool conv)
{
    psAssert(options, "Require options");
    psAssert(config, "Require configuration");

    const psArray *cells = options->cells; // Array of input cells
    const psArray *imageNames = conv ? options->convImages : options->origImages; // Names of images to read
    const psArray *maskNames = conv ? options->convMasks : options->origMasks; // Names of masks to read
    const psArray *varianceNames = conv ? options->convVariances : options->origVariances; // Variance names
    const psArray *covariances = conv ? options->convCovars : options->origCovars; // Covariance matrices

    PS_ASSERT_ARRAY_NON_NULL(cells, NULL);
    if (imageNames) {
        PS_ASSERT_ARRAYS_SIZE_EQUAL(cells, imageNames, NULL);
    }
    if (maskNames) {
        PS_ASSERT_ARRAYS_SIZE_EQUAL(cells, maskNames, NULL);
    }
    if (varianceNames) {
        PS_ASSERT_ARRAYS_SIZE_EQUAL(cells, varianceNames, NULL);
    }
    if (covariances) {
        PS_ASSERT_ARRAYS_SIZE_EQUAL(cells, covariances, NULL);
    }
    
    ppStackThreadData *stack = psAlloc(sizeof(ppStackThreadData)); // Thread data, to return
    psMemSetDeallocator(stack, (psFreeFunc)stackThreadDataFree);

    stack->lastScan = 0;

    int numInputs = cells->n;           // Number of inputs
    stack->imageFits  = psArrayAlloc(numInputs);
    stack->maskFits   = psArrayAlloc(numInputs);
    stack->varianceFits = psArrayAlloc(numInputs);
    stack->bkgFits    = psArrayAlloc(numInputs);
    for (int i = 0; i < numInputs; i++) {
        if (!cells->data[i]) {
            // Bad image
            continue;
        }

// Open an image
#define IMAGE_OPEN(NAMES, FITS, INDEX)          \
        if (NAMES) { \
            psString resolved = pmConfigConvertFilename((NAMES)->data[INDEX], config, false, false); \
            (FITS)->data[INDEX] = psFitsOpen(resolved, "r");                            \
            if (!(FITS)->data[INDEX]) { \
                psError(PPSTACK_ERR_IO, false, "Unable to open file %s", (char*)(NAMES)->data[INDEX]); \
                psFree(resolved); \
                return NULL; \
            } \
            psFree(resolved); \
        }
		
        IMAGE_OPEN(imageNames, stack->imageFits, i);
	if (maskNames) {
	  IMAGE_OPEN(maskNames, stack->maskFits, i);
	}
	if (varianceNames) {
	  IMAGE_OPEN(varianceNames, stack->varianceFits, i);
	}
    }

    int numThreads = psMetadataLookupS32(NULL, config->arguments, "NTHREADS"); // Number of threads

    // Generate readouts for each input file in each file group
    psArray *threads = psArrayAlloc(numThreads + 1); // A stack for each thread
    for (int i = 0; i < threads->n; i++) {
        psArray *readouts = psArrayAlloc(numInputs); // Input readouts
        for (int j = 0; j < numInputs; j++) {
            pmCell *cell = cells->data[j]; // Cell with data
            if (!cell) {
                continue;
            }
            pmReadout *ro = pmReadoutAlloc(cell); // Readout for thread
            if (covariances) {
                ro->covariance = psMemIncrRefCounter(covariances->data[j]);
            }
            readouts->data[j] = ro;
        }
        ppStackThread *thread = ppStackThreadAlloc(readouts);
	thread->entry = i;
        threads->data[i] = thread;
        psFree(readouts);               // Drop reference
    }
    stack->threads = threads;

    return stack;
}


ppStackThread *ppStackThreadRead(bool *status, ppStackThreadData *stack, pmConfig *config, int numChunk,
                                 int overlap)
{
    assert(status);
    PS_ASSERT_PTR_NON_NULL(stack, NULL);
    PS_ASSERT_PTR_NON_NULL(config, NULL);

    *status = true;

    psArray *threads = stack->threads;  // Threads for reading
    psMetadata *recipe = psMetadataLookupMetadata(NULL, config->recipes, PPSTACK_RECIPE); // Recipe
    if (!recipe) {
        psError(PPSTACK_ERR_CONFIG, false, "Unable to find recipe %s", PPSTACK_RECIPE);
        return NULL;
    }
    int rows = psMetadataLookupS32(NULL, recipe, "ROWS"); // Number of rows to read per chunk
    if (rows <= 0) {
        psError(PPSTACK_ERR_CONFIG, false, "ROWS is not set in the recipe.");
        return NULL;
    }

    // Select an available group
    while (true) {
        // check for any groups which can read data
	// NOTE: threads->n is set (in ppStackThreadDataSetup) to be 1 more than the argument to -threads
        for (int j = 0; j < threads->n; j++) {
            ppStackThread *thread = threads->data[j];
	    // fprintf (stderr, "grab thread %d, scan: %d - %d : busy: %d read: %d status: %d\n", thread->entry, thread->firstScan, thread->lastScan, thread->busy, thread->read, thread->status);
            if (thread->read) {
                continue;
            }

            thread->firstScan = stack->lastScan; // Overlap is taken care of in pmReadoutReadChunk
            thread->lastScan = stack->lastScan + rows;
            stack->lastScan = thread->lastScan;
	    thread->status = PPSTACK_THREAD_READ;

            psArray *readouts = thread->readouts;

            psTimerStart ("ppStackReadChunk");

            psTrace("ppStack", 2, "Reading data for chunk %d into group %d....\n", numChunk, j);
            // fprintf (stderr, "Reading data for chunk %d into group %d (thread %d, scan: %d - %d)....\n", numChunk, j, thread->entry, thread->firstScan, thread->lastScan);
            for (int i = 0; i < readouts->n; i++) {
                pmReadout *ro = readouts->data[i]; // Input readout
                if (!ro) {
                    continue;
                }

		// fprintf (stderr, "read data for readout %d\n", i);

                // override the recorded last scan
                ro->thisImageScan    = thread->firstScan;
                ro->thisVarianceScan = thread->firstScan;
                ro->thisMaskScan     = thread->firstScan;
                ro->lastImageScan    = thread->lastScan;
                ro->lastMaskScan     = thread->lastScan;
                ro->lastVarianceScan = thread->lastScan;
                ro->forceScan        = true;

                psFits *imageFits    = stack->imageFits->data[i]; // FITS file for image
                psFits *maskFits     = stack->maskFits->data[i]; // FITS file for mask
                psFits *varianceFits = stack->varianceFits->data[i]; // FITS file for variance

                int zMax = 0;
                bool keepReading = false;

                if (imageFits && pmReadoutMore(ro, imageFits, 0, &zMax, rows, config)) {
                    keepReading = true;
                    if (!pmReadoutReadChunk(ro, imageFits, 0, NULL, rows, overlap, config)) {
                        psError(PPSTACK_ERR_IO, false,
                                "Unable to read chunk %d for file PPSTACK.INPUT %d",
                                numChunk, i);
                        *status = false;
                        return NULL;
                    }
                }

                if (maskFits && pmReadoutMoreMask(ro, maskFits, 0, &zMax, rows, config)) {
                    keepReading = true;
                    if (!pmReadoutReadChunkMask(ro, maskFits, 0, NULL, rows, overlap, config)) {
                        psError(PPSTACK_ERR_IO, false,
                                "Unable to read chunk %d for file PPSTACK.INPUT.MASK %d",
                                numChunk, i);
                        *status = false;
                        return NULL;
                    }
                }

                if (varianceFits && pmReadoutMoreVariance(ro, varianceFits, 0, &zMax, rows, config)) {
                    keepReading = true;
                    if (!pmReadoutReadChunkVariance(ro, varianceFits, 0, NULL, rows, overlap, config)) {
                        psError(PPSTACK_ERR_IO, false,
                                "Unable to read chunk %d for file PPSTACK.INPUT.VARIANCE %d",
                                numChunk, i);
                        *status = false;
                        return NULL;
                    }
                }
                if (!keepReading) {
                    return NULL;
                }
            }

            thread->read = thread->busy = true;
	    thread->status = PPSTACK_THREAD_RUN;
            return thread;
        }

        // Check for threads that are ready to read
        bool wait = true;
        for (int j = 0; j < threads->n; j++) {
            ppStackThread *thread = threads->data[j];
	    // fprintf (stderr, "test thread %d, scan: %d - %d : busy: %d, read: %d, status: %d\n", thread->entry, thread->firstScan, thread->lastScan, thread->busy, thread->read, thread->status);

            if (thread->busy) {
                continue;
            }
	    thread->status = PPSTACK_THREAD_NEW;
            thread->read = false;
            wait = false;
        }
        if (wait) {
            // No threads currently available
            usleep(THREAD_WAIT);
        }
    }
    return NULL;
}


void ppStackSetThreads(void)
{
    static bool threaded = false;       // Are we running threaded?
    if (threaded) {
        psAbort("Already running threaded.");
    }
    threaded = true;

    {
        psThreadTask *task = psThreadTaskAlloc("PPSTACK_INITIAL_COMBINE", 3);
        task->function = &ppStackReadoutInitialThread;
        psThreadTaskAdd(task);
        psFree(task);
    }

    {
        psThreadTask *task = psThreadTaskAlloc("PPSTACK_INSPECT", 3);
        task->function = &ppStackInspect;
        psThreadTaskAdd(task);
        psFree(task);
    }

    {
        psThreadTask *task = psThreadTaskAlloc("PPSTACK_FINAL_COMBINE", 7);
        task->function = &ppStackReadoutFinalThread;
        psThreadTaskAdd(task);
        psFree(task);
    }

    {
        psThreadTask *task = psThreadTaskAlloc("PPSTACK_PERCENT_COMBINE", 3);
        task->function = &ppStackReadoutPercentThread;
        psThreadTaskAdd(task);
        psFree(task);
    }
    return;
}
