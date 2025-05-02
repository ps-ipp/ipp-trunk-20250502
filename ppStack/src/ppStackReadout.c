#include "ppStack.h"

bool ppStackReadoutInitialThread(psThreadJob *job)
{
    PS_ASSERT_THREAD_JOB_NON_NULL(job, false);

    psArray *args = job->args;          // Arguments
    ppStackThread *thread = args->data[0]; // Thread
    ppStackOptions *options = args->data[1]; // Options
    pmConfig *config = args->data[2];   // Configuration

    pmReadout *outRO = options->outRO;  // Output readout
    psVector *mask = options->inputMask; // Mask for inputs
    psVector *weightings = options->weightings; // Weightings (1/noise^2) for each image
    psVector *exposures = options->exposures;   // Exposure times for each image
    psVector *addVariance = options->matchChi2; // Additional variance when rejecting

    job->results = ppStackReadoutInitial(config, outRO, thread->readouts, mask,
                                         weightings, exposures, addVariance);
    thread->busy = false;

    thread->status = job->results ? PPSTACK_THREAD_SUCCESS : PPSTACK_THREAD_FAILURE;

    psAssert(job->results, "Stacking failed.");

    return job->results ? true : false;
}

bool ppStackReadoutFinalThread(psThreadJob *job)
{
    PS_ASSERT_THREAD_JOB_NON_NULL(job, false);

    psArray *args = job->args;          // Arguments
    ppStackThread *thread = args->data[0]; // Thread
    psArray *reject = args->data[1];    // Rejected pixels for each image
    ppStackOptions *options = args->data[2]; // Options
    pmConfig *config = args->data[3];   // Configuration
    bool safety = PS_SCALAR_VALUE(args->data[4], U8);    // Safety switch on?
    bool normalise = PS_SCALAR_VALUE(args->data[5], U8); // Normalise images?
    bool bscaleoffset = PS_SCALAR_VALUE(args->data[6], U8); // Apply bscale offset?

    psVector *mask = options->inputMask; // Mask for inputs
    psVector *weightings = options->weightings; // Weightings (1/noise^2) for each image
    psVector *exposures = options->exposures;   // Exposure times for each image
    psVector *addVariance = options->matchChi2; // Additional variance when rejecting
    psVector *norm = normalise ? options->norm : NULL; // Normalisations to apply to images
    psVector *bscaleApplyOffset = bscaleoffset ? options->bscaleOffset : NULL; // BSCALE offset to apply to images

    bool status = ppStackReadoutFinal(config, options->outRO, options->expRO, thread->readouts, mask, reject,
                                      weightings, exposures, addVariance, safety, norm, bscaleApplyOffset); // Status of operation

    thread->busy = false;

    thread->status = status ? PPSTACK_THREAD_SUCCESS : PPSTACK_THREAD_FAILURE;

    psAssert(status, "Stacking failed.");

    return status;
}

bool ppStackReadoutPercentThread(psThreadJob *job)
{
    PS_ASSERT_THREAD_JOB_NON_NULL(job, false);

    psArray *args = job->args;          // Arguments
    ppStackThread *thread = args->data[0]; // Thread
    ppStackOptions *options = args->data[1]; // Options
    pmConfig *config = args->data[2];   // Configuration

    pmReadout *outRO = options->outRO;  // Output readout
    pmReadout *expRO = options->expRO;  // Expmap readout
    psVector *mask = options->inputMask; // Mask for inputs
    psVector *weightings = options->weightings; // Weightings (1/noise^2) for each image
    psVector *exposures = options->exposures;   // Exposure times for each image
    psVector *addVariance = options->matchChi2; // Additional variance when rejecting

    // fprintf (stderr, "start thread %d : scan range %d - %d\n", thread->entry, thread->firstScan, thread->lastScan);

    bool status = ppStackReadoutPercent(config, outRO, expRO, thread->readouts, mask, weightings, exposures, addVariance);

    thread->busy = false;

    thread->status = status ? PPSTACK_THREAD_SUCCESS : PPSTACK_THREAD_FAILURE;

    fprintf (stderr, "finished thread %d, status %d : scan range %d - %d\n", thread->entry, thread->status, thread->firstScan, thread->lastScan);

    psAssert(status, "Stacking failed.");

    return status;
}

//////////////////////////////////// 

bool ppStackInspect(psThreadJob *job)
{
    PS_ASSERT_THREAD_JOB_NON_NULL(job, false);

    psArray *args = job->args;  // Input arguments
    psArray *inspects = args->data[0]; // Array of pixel arrays
    psArray *rejects = args->data[1];  // Array of pixel arrays
    int index = PS_SCALAR_VALUE(args->data[2], S32); // Index of interest

    psArray *inInspects = inspects->data[index]; // Array of interest
    psArray *inRejects = rejects->data[index]; // Array of interest
    psAssert(inInspects->n == inRejects->n, "Size should be the same");
    psPixels *outInspect = NULL, *outReject = NULL; // Output pixel lists
    for (int i = 0; i < inInspects->n; i++) {
        psPixels *inInspect = inInspects->data[i]; // Input pixel list
        if (inInspect && inInspect->n > 0) {
            outInspect = psPixelsConcatenate(outInspect, inInspect);
        }
        psPixels *inReject = inRejects->data[i]; // Input pixel list
        if (inReject && inReject->n > 0) {
            outReject = psPixelsConcatenate(outReject, inReject);
        }
    }

    // If there are no pixels to inspect, then just fake it
    if (!outInspect) {
        outInspect = psPixelsAllocEmpty(0);
    }
    if (!outReject) {
        outReject = psPixelsAllocEmpty(0);
    }

    psFree(inspects->data[index]);
    inspects->data[index] = outInspect;
    psFree(rejects->data[index]);
    rejects->data[index] = outReject;

    return true;
}


psArray *ppStackReadoutInitial(const pmConfig *config, pmReadout *outRO, const psArray *readouts,
                               const psVector *mask, const psVector *weightings, const psVector *exposures,
                               const psVector *addVariance)
{
    assert(config);
    assert(outRO);
    assert(readouts);
    assert(mask && mask->n == readouts->n && mask->type.type == PS_TYPE_VECTOR_MASK);
    assert(weightings && weightings->n == readouts->n && weightings->type.type == PS_TYPE_F32);
    assert(addVariance && addVariance->n == readouts->n && addVariance->type.type == PS_TYPE_F32);
    static int sectionNum = 0;          // Section number; for debugging outputs

    // Get the recipe values
    psMetadata *recipe = psMetadataLookupMetadata(NULL, config->recipes, PPSTACK_RECIPE); // ppStack recipe
    psAssert(recipe, "We've thrown an error on this before.");

    bool mdok;                          // Status of MD lookup
    float iter = psMetadataLookupF32(NULL, recipe, "COMBINE.ITER"); // Rejection iterations
    float combineRej = psMetadataLookupF32(NULL, recipe, "COMBINE.REJ"); // Combination threshold
    float combineSys = psMetadataLookupF32(NULL, recipe, "COMBINE.SYS"); // Combination systematic error
    float combineDiscard = psMetadataLookupF32(NULL, recipe, "COMBINE.DISCARD"); // Olympic discard fraction
    bool useVariance = psMetadataLookupBool(&mdok, recipe, "VARIANCE"); // Use variance for rejection?
    bool safe = psMetadataLookupBool(&mdok, recipe, "SAFE"); // Be safe when combining small numbers of pixels
   
    int nminpix = psMetadataLookupS32(&mdok, recipe, "NMINPIX"); // Minimum input per pixel to combine with

    psMetadata *ppsub = psMetadataLookupMetadata(NULL, config->recipes, "PPSUB"); // PPSUB recipe
    int kernelSize = psMetadataLookupS32(NULL, ppsub, "KERNEL.SIZE"); // Kernel half-size

    psString maskBadStr = psMetadataLookupStr(NULL, recipe, "MASK.VAL"); // Name of bits to mask for bad
    psImageMaskType maskBad = pmConfigMaskGet(maskBadStr, config); // Bits to mask for bad pixels

    psString maskSuspectStr = psMetadataLookupStr(NULL, recipe, "MASK.SUSPECT"); // Name of suspect mask bits
    psImageMaskType maskSuspect = pmConfigMaskGet(maskSuspectStr, config); // Suspect bits

    bool status = false;
    psImageMaskType maskBlank;
    psString maskBlankStr = psMetadataLookupStr(&status, recipe, "MASK.BLANK"); // Name of bits to set for empty pixels
    if (maskBlankStr) {
      maskBlank = pmConfigMaskGet(maskBlankStr, config); // Bits to mask for bad pixels
    } else {
      maskBlankStr = psMetadataLookupStr(&status, recipe, "MASK.BAD"); // Old name for MASK.BLANK
      if (maskBlankStr) {
	maskBlank = pmConfigMaskGet(maskBlankStr, config); // Bits to mask for bad pixels
      } else {
	maskBlank = pmConfigMaskGet("BLANK", config); // Bits to mask for bad pixels
      }
    }

    int num = readouts->n;              // Number of inputs
    psArray *stackData = psArrayAlloc(num); // Array of data to be stacked

    for (int i = 0; i < num; i++) {
        pmReadout *ro = readouts->data[i];
        if (!ro || mask->data.PS_TYPE_VECTOR_MASK_DATA[i]) {
            // Bad image
            continue;
        }

        // Ensure there is a mask, or pmStackCombine will complain
        if (!ro->mask) {
            ro->mask = psImageAlloc(ro->image->numCols, ro->image->numRows, PS_TYPE_IMAGE_MASK);
            psImageInit(ro->mask, 0);
        }

	// stackData is an array of pmStackData structures
        stackData->data[i] = pmStackDataAlloc(ro, weightings->data.F32[i], exposures->data.F32[i],
                                          addVariance->data.F32[i]);
    }

    if (!pmStackCombine(outRO, NULL, stackData, maskBad, maskSuspect, maskBlank, kernelSize, iter,
                        combineRej, combineSys, combineDiscard, useVariance, safe, nminpix, false)) {
        psError(PS_ERR_UNKNOWN, false, "Unable to combine input readouts with rejection.");
        psFree(stackData);
	
	// XXX EAM : if pmStackCombine fails above this return will cause the thread to block.
	// The failure status results in a psThread.fault.  the psThreadLauncher function, which
	// each background thread runs as a loop, will sleep until the fault is cleared externally
	// by the handler.  psThreadPoolWait() catches, counts, and clears these faults, but
	// ppStackThreadRead does not.  
        return false;
    }

    // Save lists of pixels
    psArray *inspect = psArrayAlloc(num); // List of pixels to inspect
    psArray *reject = psArrayAlloc(num);  // List of pixels rejected
    for (int i = 0; i < num; i++) {
        pmStackData *data = stackData->data[i]; // Data for this image
        if (!data) {
            continue;
        }
        pmReadout *readout = data->readout; // Readout of interest
        if (!readout) {
            continue;
        }
        inspect->data[i] = psMemIncrRefCounter(data->inspect);
        reject->data[i] = psMemIncrRefCounter(data->reject);
    }
    psFree(stackData);

    //MEH change to trace
    //psLogMsg("ppStack", PS_LOG_INFO, "initial stack image sectionNum %d", sectionNum);

    sectionNum++;

    psArray *results = psArrayAlloc(2); // Array of results
    results->data[0] = inspect;
    results->data[1] = reject;

    return results;
}


bool ppStackReadoutFinal(const pmConfig *config, pmReadout *outRO, pmReadout *expRO, const psArray *readouts,
                         const psVector *mask, const psArray *rejected, const psVector *weightings,
                         const psVector *exposures, const psVector *addVariance, bool safety,
                         const psVector *norm, const psVector *bscaleApplyOffset)
{
    assert(config);
    assert(outRO);
    assert(expRO);
    assert(readouts);
    assert(!rejected || readouts->n == rejected->n);
    assert(mask && mask->n == readouts->n && mask->type.type == PS_TYPE_VECTOR_MASK);
    assert(weightings && weightings->n == readouts->n && weightings->type.type == PS_TYPE_F32);

    static int sectionNum = 0;

    // Get the recipe values
    psMetadata *recipe = psMetadataLookupMetadata(NULL, config->recipes, PPSTACK_RECIPE); // ppStack recipe
    psAssert(recipe, "We've thrown an error on this before.");

    bool mdok;                          // Status of MD lookup
    bool useVariance = psMetadataLookupBool(&mdok, recipe, "VARIANCE"); // Use variance for rejection?
    bool safe = psMetadataLookupBool(&mdok, recipe, "SAFE"); // Be safe when combining small numbers of pixels

    int nminpix = psMetadataLookupS32(&mdok, recipe, "NMINPIX"); // Minimum input per pixel to combine with

    psString maskBadStr = psMetadataLookupStr(NULL, recipe, "MASK.VAL"); // Name of bits to mask for bad
    psImageMaskType maskBad = pmConfigMaskGet(maskBadStr, config); // Bits to mask for bad pixels

    psString maskSuspectStr = psMetadataLookupStr(NULL, recipe, "MASK.SUSPECT"); // Name of suspect mask bits
    psImageMaskType maskSuspect = pmConfigMaskGet(maskSuspectStr, config); // Suspect bits

    bool status = false;
    psImageMaskType maskBlank;
    psString maskBlankStr = psMetadataLookupStr(&status, recipe, "MASK.BLANK"); // Name of bits to set for empty pixels
    if (maskBlankStr) {
      maskBlank = pmConfigMaskGet(maskBlankStr, config); // Bits to mask for bad pixels
    } else {
      maskBlankStr = psMetadataLookupStr(&status, recipe, "MASK.BAD"); // Old name for MASK.BLANK
      if (maskBlankStr) {
	maskBlank = pmConfigMaskGet(maskBlankStr, config); // Bits to mask for bad pixels
      } else {
	maskBlank = pmConfigMaskGet("BLANK", config); // Bits to mask for bad pixels
      }
    }

    int num = readouts->n;              // Number of inputs
    psArray *stackData = psArrayAlloc(num); // Array of data to be stacked

    // We have rejection from a previous combination: combine without flagging pixels to inspect
    safe &= safety;
    int iter = 0;
    float combineRej = NAN;
    float combineSys = NAN;
    float combineDiscard = NAN;

    for (int i = 0; i < num; i++) {
        pmReadout *ro = readouts->data[i];
        if (mask->data.U8[i]) {
            // Image completely rejected
            continue;
        }

        // Ensure there is a mask, or pmStackCombine will complain
        if (!ro->mask) {
            ro->mask = psImageAlloc(ro->image->numCols, ro->image->numRows, PS_TYPE_IMAGE_MASK);
            psImageInit(ro->mask, 0);
        }

        pmStackData *data = pmStackDataAlloc(ro, weightings->data.F32[i], exposures->data.F32[i],
                                             addVariance ? addVariance->data.F32[i] : NAN);
        data->reject = rejected ? psMemIncrRefCounter(rejected->data[i]) : NULL;
        stackData->data[i] = data;

        //MEH -- apply bscale offset before norm   
        if (bscaleApplyOffset) {
	    //MEH change to trace
            //psLogMsg("ppStack", PS_LOG_INFO, "bscaleApplyOffset: %d %f", i, bscaleApplyOffset->data.F32[i]);
            psBinaryOp(ro->image, ro->image, "-", psScalarAlloc(bscaleApplyOffset->data.F32[i], PS_TYPE_F32));
        }

        if (norm) {
            float normalise = powf(10.0, -0.4 * norm->data.F32[i]); // Normalisation
            psBinaryOp(ro->image, ro->image, "*", psScalarAlloc(normalise, PS_TYPE_F32));
            psBinaryOp(ro->variance, ro->variance, "*", psScalarAlloc(PS_SQR(normalise), PS_TYPE_F32));
        }
    }

    if (!pmStackCombine(outRO, expRO, stackData, maskBad, maskSuspect, maskBlank, 0, iter, combineRej,
                        combineSys, combineDiscard, useVariance, safe, nminpix, rejected)) {
        psError(PS_ERR_UNKNOWN, false, "Unable to combine input readouts.");
        psFree(stackData);
        return false;
    }

    pmCell *outCell = outRO->parent;    // Output cell
    pmChip *outChip = outCell->parent;  // Output chip
    outRO->data_exists = true;
    outCell->data_exists = true;
    outChip->data_exists = true;

    pmCell *expCell = expRO->parent;    // Exposure cell
    pmChip *expChip = expCell->parent;  // Exposure chip
    expRO->data_exists = true;
    expCell->data_exists = true;
    expChip->data_exists = true;

    psFree(stackData);

    //MEH change to trace
    //psLogMsg("ppStack", PS_LOG_INFO, "final stack image sectionNum %d", sectionNum);

    sectionNum++;

    return true;
}

// NOTE: expRO is allowed to be NULL
bool ppStackReadoutPercent(const pmConfig *config, pmReadout *outRO, pmReadout *expRO, const psArray *readouts,
			       const psVector *mask, const psVector *weightings, const psVector *exposures,
			       const psVector *addVariance)
{
    assert(config);
    assert(outRO);
    assert(readouts);
    assert(mask && mask->n == readouts->n && mask->type.type == PS_TYPE_VECTOR_MASK);
    assert(weightings && weightings->n == readouts->n && weightings->type.type == PS_TYPE_F32);
    assert(addVariance && addVariance->n == readouts->n && addVariance->type.type == PS_TYPE_F32);
    static int sectionNum = 0;          // Section number; for debugging outputs

    // fprintf (stderr, "starting ReadoutPercent, %d\n", sectionNum);

    // Get the recipe values
    psMetadata *recipe = psMetadataLookupMetadata(NULL, config->recipes, PPSTACK_RECIPE); // ppStack recipe
    psAssert(recipe, "We've thrown an error on this before.");

    bool mdok;                          // Status of MD lookup
    psF64 rejectFraction = psMetadataLookupF32(&mdok, recipe, "COMBINE.REJECT.FRACTION"); // fraction of outliers to reject
    int nminpix = psMetadataLookupS32(&mdok, recipe, "NMINPIX"); // Minimum input per pixel to combine with

    char defaultBlankStr[16] = "BLANK";

    psString maskBadStr     = psMetadataLookupStr(&mdok, recipe, "MASK.VAL"); // Name of bits to mask for bad
    psString maskSuspectStr = psMetadataLookupStr(&mdok, recipe, "MASK.SUSPECT"); // Name of suspect mask bits
    psString maskBlankStr   = psMetadataLookupStr(&mdok, recipe, "MASK.BLANK"); // Name of bits to set for empty pixels
    if (!maskBlankStr) {
      maskBlankStr = psMetadataLookupStr(&mdok, recipe, "MASK.BAD"); // Old name for MASK.BLANK
    }
    if (!maskBlankStr) {
      maskBlankStr = defaultBlankStr; // this is statically allocated above
    }

    psImageMaskType maskBad     = pmConfigMaskGet(maskBadStr, config); // Bits to mask for bad pixels
    psImageMaskType maskSuspect = pmConfigMaskGet(maskSuspectStr, config); // Suspect bits
    psImageMaskType maskBlank   = pmConfigMaskGet(maskBlankStr, config); // Bits to mask for bad pixels

    int num = readouts->n;              // Number of inputs
    psArray *stackData = psArrayAlloc(num); // Array of data to be stacked

    for (int i = 0; i < num; i++) {
	stackData->data[i] = NULL;

        pmReadout *ro = readouts->data[i];
        if (!ro || mask->data.PS_TYPE_VECTOR_MASK_DATA[i]) {
            // Bad image
            continue;
        }

        // Ensure there is a mask, or pmStackCombine will complain
        if (!ro->mask) {
            ro->mask = psImageAlloc(ro->image->numCols, ro->image->numRows, PS_TYPE_IMAGE_MASK);
            psImageInit(ro->mask, 0);
        }

        stackData->data[i] = pmStackDataAlloc(ro, weightings->data.F32[i], exposures->data.F32[i],
					      addVariance->data.F32[i]);
    }

      if (!pmStackCombineByPercentile(outRO, expRO, stackData, rejectFraction, nminpix, maskBad, maskSuspect, maskBlank)) {
        psError(PS_ERR_UNKNOWN, false, "Unable to combine input readouts with rejection.");
        psFree(stackData);
        return false;
    }

    outRO->data_exists = true;		       // output readout
    outRO->parent->data_exists = true;	       // output cell
    outRO->parent->parent->data_exists = true; // output chip

    if (expRO) {
      expRO->data_exists = true;                 // expmap readout 
      expRO->parent->data_exists = true;	       // expmap cell	
      expRO->parent->parent->data_exists = true; // expmap chip	
    }
      
    psFree(stackData);
    sectionNum++;

    return true;
}


