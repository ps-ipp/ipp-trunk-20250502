# include "psphotInternal.h"
# define ARRAY_BUFFER 16                 // Number to add to array at a time

psVector *SetOptWidths (bool *optimum, psMetadata *recipe);
pmReadout *makeFakeReadout(pmConfig *config, pmReadout *raw, psArray *sources, pmPSF *psf, psImageMaskType maskVal, int fullSize);
bool saveMatchData (pmReadout *readout, psphotStackOptions *options, int index);
bool matchKernel(pmConfig *config, pmReadout *cnv, pmReadout *raw, psphotStackOptions *options, int index);
bool dumpImageDiff(pmReadout *readoutConv, pmReadout *readoutFake, pmReadout *readoutRef, int index, char *rootname);
bool dumpImage(pmReadout *readoutOut, pmReadout *readoutRef, int index, char *rootname);

// Get coordinates from a source
void coordsFromSource(float *x, float *y, const pmSource *source)
{
    assert(x && y);
    assert(source);

    if (source->modelPSF) {
        *x = source->modelPSF->params->data.F32[PM_PAR_XPOS];
        *y = source->modelPSF->params->data.F32[PM_PAR_YPOS];
    } else {
        *x = source->peak->xf;
        *y = source->peak->yf;
    }
    return;
}

# define SN_MIN 50.0
psArray *stackSourcesFilter(psArray *sources, // Source list to filter
			    int exclusion // Exclusion zone, pixels
    )
{
    psAssert(sources && sources->n > 0, "Require array of sources");
    if (exclusion <= 0) {
        return psMemIncrRefCounter(sources);
    }

    int num = sources->n;               // Number of sources
    psVector *x = psVectorAlloc(num, PS_TYPE_F32), *y = psVectorAlloc(num, PS_TYPE_F32); // Coordinates
    int numGood = 0;                    // Number of good sources
    for (int i = 0; i < num; i++) {
        pmSource *source = sources->data[i]; // Source of interest
        if (!source) {
            continue;
        }
	if (!source->peak) continue;
	if (sqrt(source->peak->detValue) < SN_MIN) continue;
        coordsFromSource(&x->data.F32[numGood], &y->data.F32[numGood], source);
        numGood++;
    }
    x->n = y->n = numGood;

    psTree *tree = psTreePlant(2, 2, PS_TREE_EUCLIDEAN, x, y); // kd tree

    psArray *filtered = psArrayAllocEmpty(numGood); // Filtered list of sources
    psVector *coords = psVectorAlloc(2, PS_TYPE_F64); // Coordinates of source
    int numFiltered = 0;                // Number of filtered sources
    for (int i = 0; i < num; i++) {
        pmSource *source = sources->data[i]; // Source of interest
        if (!source) {
            continue;
        }
	if (!source->peak) continue;
	if (sqrt(source->peak->detValue) < SN_MIN) continue;
        float xSource, ySource;         // Coordinates of source
        coordsFromSource(&xSource, &ySource, source);

        coords->data.F64[0] = xSource;
        coords->data.F64[1] = ySource;

        long numWithin = psTreeWithin(tree, coords, exclusion); // Number within exclusion zone
        psTrace("psphotStack", 9, "Source at %.0lf,%.0lf has %ld sources in exclusion zone",
                coords->data.F64[0], coords->data.F64[1], numWithin);
        if (numWithin == 1) {
            // Only itself inside the exclusion zone
            filtered = psArrayAdd(filtered, filtered->n, source);
        } else {
            numFiltered++;
        }
    }
    psFree(coords);
    psFree(tree);
    psFree(x);
    psFree(y);

    psLogMsg("psphotStack", PS_LOG_INFO, "Filtered out %d of %d sources", numFiltered, numGood);

    return filtered;
}

// Add background into the fake image
// Based on ppSubBackground()
psImage *stackBackgroundModel(pmReadout *ro, // Readout for which to generate background model
                                     const pmConfig *config // Configuration
    )
{
    psAssert(ro && ro->image, "Need readout image");
    psAssert(config, "Need configuration");

    psImage *image = ro->image;         // Image of interest
    int numCols = image->numCols, numRows = image->numRows; // Size of image

    psMetadata *ppStackRecipe = psMetadataLookupPtr(NULL, config->recipes, "PPSTACK");
    psAssert(ppStackRecipe, "Need PPSTACK recipe");
    psMetadata *psphotRecipe = psMetadataLookupPtr(NULL, config->recipes, "PSPHOT");
    psAssert(psphotRecipe, "Need PSPHOT recipe");

#if (0) 
    // DON'T OVERWRITE PSPHOT recipe value
    psString maskBadStr = psMetadataLookupStr(NULL, ppStackRecipe, "MASK.BAD");// Name of bits to mask for bad
    psMaskType maskBad = pmConfigMaskGet(maskBadStr, config); // Bits to mask for bad pixels

    // user-defined masks to test for good/bad pixels (build from recipe list if not yet set)
    psMetadataAddImageMask(psphotRecipe, PS_LIST_TAIL, "MASK.PSPHOT", PS_META_REPLACE, "user-defined mask", maskBad);
#endif

    psImage *binned = psphotModelBackgroundReadoutNoFile(ro, config); // Binned background model
    psImageBinning *binning = psMetadataLookupPtr(NULL, ro->analysis,
                                                  "PSPHOT.BACKGROUND.BINNING"); // Binning for model
    psAssert(binning, "Need binning parameters");
    psImage *unbinned = psImageAlloc(numCols, numRows, PS_TYPE_F32); // Unbinned background model
    if (!psImageUnbin(unbinned, binned, binning)) {
        psError(PSPHOT_ERR_DATA, false, "Unable to unbin background model");
        psFree(binned);
        psFree(unbinned);
        return NULL;
    }
    psFree(binned);

    return unbinned;
}

// Renormalise a readout's variance map
bool psphotStackRenormaliseVariance(const pmConfig *config, // Configuration
			      pmReadout *readout      // Readout to renormalise
    )
{
    bool mdok; // Status of metadata lookups

    psMetadata *recipe = psMetadataLookupPtr(NULL, config->recipes, "PPSTACK"); // Recipe for ppStack
    psAssert(recipe, "Need PPSTACK recipe");

    if (!psMetadataLookupBool(&mdok, recipe, "RENORM")) return true;

    int num = psMetadataLookupS32(&mdok, recipe, "RENORM.NUM");
    if (!mdok) {
        psError(PSPHOT_ERR_CONFIG, true, "RENORM.NUM is not set in the recipe");
        return false;
    }
    float minValid = psMetadataLookupF32(&mdok, recipe, "RENORM.MIN");
    if (!mdok) {
        psError(PSPHOT_ERR_CONFIG, true, "RENORM.MIN is not set in the recipe");
        return false;
    }
    float maxValid = psMetadataLookupF32(&mdok, recipe, "RENORM.MAX");
    if (!mdok) {
        psError(PSPHOT_ERR_CONFIG, true, "RENORM.MAX is not set in the recipe");
        return false;
    }

    psImageMaskType maskBad = pmConfigMaskGet("BLANK", config); // Bits to mask

    psImageCovarianceTransfer(readout->variance, readout->covariance);
    return pmReadoutVarianceRenormalise(readout, maskBad, num, minValid, maxValid);
}

bool dumpImage(pmReadout *readoutOut, pmReadout *readoutRef, int index, char *rootname) {

    pmHDU *hdu = pmHDUFromCell(readoutRef->parent);
    psString name = NULL;
    psStringAppend(&name, "%s_%03d.fits", rootname, index);
    pmStackVisualPlotTestImage(readoutOut->image, name);
    psFits *fits = psFitsOpen(name, "w");
    psFree(name);
    psFitsWriteImage(fits, hdu->header, readoutOut->image, 0, NULL);
    psFitsClose(fits);
    return true;
}

bool dumpImageDiff(pmReadout *readoutConv, pmReadout *readoutFake, pmReadout *readoutRef, int index, char *rootname) {

    pmHDU *hdu = pmHDUFromCell(readoutRef->parent);
    psString name = NULL;
    psStringAppend(&name, "%s_%03d.fits", rootname, index);
    pmStackVisualPlotTestImage(readoutFake->image, name);
    psFits *fits = psFitsOpen(name, "w");
    psFree(name);
    psBinaryOp(readoutFake->image, readoutConv->image, "-", readoutFake->image);
    psFitsWriteImage(fits, hdu->header, readoutFake->image, 0, NULL);
    psFitsClose(fits);
    return true;
}

// perform the bulk of the PSF-matching
bool matchKernel(pmConfig *config, pmReadout *readoutOut, pmReadout *readoutSrc, psphotStackOptions *options, int index) {

    bool mdok;

    psAssert(options->psf, "Require target PSF");
    psAssert(options->sourceLists && options->sourceLists->data[index], "Require source list");

    psMetadata *stackRecipe = psMetadataLookupMetadata(NULL, config->recipes, "PPSTACK"); // ppStack recipe
    psAssert(stackRecipe, "We've thrown an error on this before.");

    // Look up appropriate values from the ppSub recipe
    psMetadata *subRecipe = psMetadataLookupMetadata(NULL, config->recipes, "PPSUB"); // PPSUB recipe
    psAssert(subRecipe, "recipe missing");

    psString maskValStr = psMetadataLookupStr(NULL, subRecipe, "MASK.VAL"); // Name of bits to mask going in
    psString maskPoorStr = psMetadataLookupStr(NULL, stackRecipe, "MASK.POOR"); // Name of bits to mask for poor
    psString maskBadStr = psMetadataLookupStr(NULL, stackRecipe, "MASK.BAD"); // Name of bits to mask for bad

    psImageMaskType maskVal = pmConfigMaskGet(maskValStr, config); // Bits to mask going in to pmSubtractionMatch
    psImageMaskType maskPoor = pmConfigMaskGet(maskPoorStr, config); // Bits to mask for poor pixels
    psImageMaskType maskBad = pmConfigMaskGet(maskBadStr, config); // Bits to mask for bad pixels

    float penalty = psMetadataLookupF32(NULL, subRecipe, "PENALTY"); // Penalty for wideness
    int threads = psMetadataLookupS32(NULL, config->arguments, "NTHREADS"); // Number of threads

    int order = psMetadataLookupS32(NULL, subRecipe, "SPATIAL.ORDER"); // Spatial polynomial order
    float regionSize = psMetadataLookupF32(NULL, subRecipe, "REGION.SIZE"); // Size of iso-kernel regs
    float spacing = psMetadataLookupF32(NULL, subRecipe, "STAMP.SPACING"); // Typical stamp spacing

    int footprint = psMetadataLookupS32(NULL, subRecipe, "STAMP.FOOTPRINT"); // Stamp half-size
    int size = psMetadataLookupS32(NULL, subRecipe, "KERNEL.SIZE"); // Kernel half-size

    float threshold = psMetadataLookupF32(NULL, subRecipe, "STAMP.THRESHOLD"); // Threshold for stmps
    int stride = psMetadataLookupS32(NULL, subRecipe, "STRIDE"); // Size of convolution patches
    int iter = psMetadataLookupS32(NULL, subRecipe, "ITER"); // Rejection iterations
    float rej = psMetadataLookupF32(NULL, subRecipe, "REJ"); // Rejection threshold
    float kernelError = psMetadataLookupF32(NULL, subRecipe, "KERNEL.ERR"); // Relative systematic error in kernel
    float normFrac = psMetadataLookupF32(NULL, subRecipe, "NORM.FRAC"); // Fraction of window for normalisn windw
    float sysError = psMetadataLookupF32(NULL, subRecipe, "SYS.ERR"); // Relative systematic error in images
    float skyErr = psMetadataLookupF32(NULL, subRecipe, "SKY.ERR"); // Additional error in sky
    float covarFrac = psMetadataLookupF32(NULL, subRecipe, "COVAR.FRAC"); // Fraction for covariance calculation

    const char *typeStr = psMetadataLookupStr(NULL, subRecipe, "KERNEL.TYPE"); // Kernel type
    pmSubtractionKernelsType type = pmSubtractionKernelsTypeFromString(typeStr); // Kernel type
    psVector *widths = psMetadataLookupPtr(NULL, subRecipe, "ISIS.WIDTHS"); // ISIS Gaussian widths
    psVector *orders = psMetadataLookupPtr(NULL, subRecipe, "ISIS.ORDERS"); // ISIS Polynomial orders
    int inner = psMetadataLookupS32(NULL, subRecipe, "INNER"); // Inner radius
    int ringsOrder = psMetadataLookupS32(NULL, subRecipe, "RINGS.ORDER"); // RINGS polynomial order
    int binning = psMetadataLookupS32(NULL, subRecipe, "SPAM.BINNING"); // Binning for SPAM kernel
    float badFrac = psMetadataLookupF32(NULL, subRecipe, "BADFRAC"); // Maximum bad fraction
    float optThresh = psMetadataLookupF32(&mdok, subRecipe, "OPTIMUM.TOL"); // Tolerance for search
    int optOrder = psMetadataLookupS32(&mdok, subRecipe, "OPTIMUM.ORDER"); // Order for search
    float poorFrac = psMetadataLookupF32(&mdok, subRecipe, "POOR.FRACTION"); // Fraction for "poor"

    bool scale = psMetadataLookupBool(NULL, subRecipe, "SCALE");        // Scale kernel parameters?
    float scaleRef = psMetadataLookupF32(NULL, subRecipe, "SCALE.REF"); // Reference for scaling
    float scaleMin = psMetadataLookupF32(NULL, subRecipe, "SCALE.MIN"); // Minimum for scaling
    float scaleMax = psMetadataLookupF32(NULL, subRecipe, "SCALE.MAX"); // Maximum for scaling
    if (!isfinite(scaleRef) || !isfinite(scaleMin) || !isfinite(scaleMax)) {
	psError(PSPHOT_ERR_CONFIG, false,
		"Scale parameters (SCALE.REF=%f, SCALE.MIN=%f, SCALE.MAX=%f) not set in PPSUB recipe.",
		scaleRef, scaleMin, scaleMax);
	return false;
    }

    // These values are specified specifically for stacking
    const char *stampsName = psMetadataLookupStr(&mdok, config->arguments, "STAMPS");// Stamps filename

    psVector *widthsCopy = NULL;
    psVector *optWidths = NULL;
    pmReadout *fake = NULL;
    psArray *stampSources = NULL;

    bool optimum = false;
    optWidths = SetOptWidths(&optimum, subRecipe); // Vector with FWHMs for optimum search

    // For the sake of stamps, remove nearby sources
    stampSources = stackSourcesFilter(options->sourceLists->data[index], footprint); // Filtered list of sources

    fake = makeFakeReadout(config, readoutSrc, stampSources, options->psf, maskVal | maskBad, footprint + size);
    if (!fake) goto escape;

    // dumpImage(fake, readoutSrc, index, "fake");
    // dumpImage(readoutSrc,  readoutSrc, index, "real");

    if (threads) pmSubtractionThreadsInit();

    // Do the image matching
    bool rejectReadout = false; 
    pmSubtractionKernels *kernel = psMetadataLookupPtr(&mdok, readoutSrc->analysis, PM_SUBTRACTION_ANALYSIS_KERNEL); // Conv kernel
    if (kernel) {
	if (!pmSubtractionMatchPrecalc(NULL, readoutOut, fake, readoutSrc, readoutSrc->analysis, stride, kernelError, covarFrac, maskVal, maskBad, maskPoor, poorFrac, badFrac)) {
	    psError(psErrorCodeLast(), false, "Unable to convolve images.");
	    goto escape;
	}
    } else {
	// Scale the input parameters
	widthsCopy = psVectorCopy(NULL, widths, PS_TYPE_F32); // Copy of kernel widths

	// we need to register the FWHM values for use by pmSubtraction code
	pmSubtractionSetFWHMs(options->targetSeeing->data.F32[0], options->inputSeeing->data.F32[index]);

	pmSubtractionParamScaleOptions(scale, scaleRef, scaleMin, scaleMax);

	// if (scale && !pmSubtractionParamsScale(&size, &footprint, widthsCopy, scaleRef, scaleMin, scaleMax)) {
	//     psError(psErrorCodeLast(), false, "Unable to scale kernel parameters");
	//     goto escape;
	// }

	if (!pmSubtractionMatch(NULL, readoutOut, fake, readoutSrc, footprint, stride, regionSize, spacing, threshold, stampSources, stampsName, type, size, order, widthsCopy, orders, inner, ringsOrder, binning, penalty, optimum, optWidths, optOrder, optThresh, iter, rej, normFrac, sysError, skyErr, kernelError, covarFrac, maskVal, maskBad, maskPoor, poorFrac, badFrac, PM_SUBTRACTION_MODE_2)) {
            int errorCode = psErrorCodeLast();
            if (errorCode == PM_ERR_SMALL_AREA || errorCode == PM_ERR_STAMPS) {
                // failed to match but, don't fault. Just drop this input from measurements that need the
                // matched readout.
                psErrorClear();
                rejectReadout = true;
                if (errorCode == PM_ERR_SMALL_AREA) {
                    psLogMsg("psphot", PS_LOG_WARN, "PSF match failed for input %d. Too few pixels.", index);
                } else if (errorCode == PM_ERR_STAMPS) {
                    psLogMsg("psphot", PS_LOG_WARN, "PSF match failed for input %d. Failed to find stamps.", index);
                }
            } else {
                psError(psErrorCodeLast(), false, "Unable to match images.");
                goto escape;
            }
	}
    }

    // If the maximum deconvolution fraction exceeds the limit, reject this input for analyses that require PSF matching
    float deconvLimit = psMetadataLookupF32(NULL, stackRecipe, "DECONV.LIMIT"); // Limit on deconvolution fraction
    float deconv = psMetadataLookupF32(NULL, readoutOut->analysis, PM_SUBTRACTION_ANALYSIS_DECONV_MAX); // Max deconvolution fraction
    if (deconv > deconvLimit) {
        rejectReadout = true;
	psLogMsg("psphot", PS_LOG_WARN, "Maximum deconvolution fraction (%f) exceeds limit (%f) --- rejecting input %d for matched psf analysis\n", deconv, deconvLimit, index);
    }
    
    if (rejectReadout) {
        psMetadataAddBool(readoutSrc->analysis, PS_LIST_TAIL, "NOT.PSF.MATCHED", PS_META_REPLACE, "", true);
        psMetadataAddBool(readoutOut->analysis, PS_LIST_TAIL, "NOT.PSF.MATCHED", PS_META_REPLACE, "", true);
    } else {
        psMetadataAddBool(readoutSrc->analysis, PS_LIST_TAIL, "NOT.PSF.MATCHED", PS_META_REPLACE, "", false);
        psMetadataAddBool(readoutOut->analysis, PS_LIST_TAIL, "NOT.PSF.MATCHED", PS_META_REPLACE, "", false);
    }
    // save the PSF on the new readout->analysis:
    // if (!psMetadataAddPtr (readoutOut->analysis, PS_LIST_TAIL, "PSPHOT.PSF", PS_META_REPLACE | PS_DATA_UNKNOWN, "psphot psf model", options->psf)) {
    //     psError (PSPHOT_ERR_UNKNOWN, false, "problem saving sources on readout");
    //     return false;
    // }

    // dumpImage(readoutOut, readoutSrc, index, "conv");
    // dumpImageDiff(readoutOut, fake, readoutSrc, index, "diff");

    psFree(fake);
    psFree(optWidths);
    psFree(stampSources);
    psFree(widthsCopy);
    pmSubtractionThreadsFinalize();
    return true;

escape:
    psFree(fake);
    psFree(optWidths);
    psFree(stampSources);
    psFree(widthsCopy);
    pmSubtractionThreadsFinalize();
    return false;
}

// Extract the regions and solutions used in the image matching
// This stops them from being freed when we iterate back up the FPA
// Record the chi-square value
// XXX this function may not be needed for psphotStack
bool saveMatchData (pmReadout *readout, psphotStackOptions *options, int index) {

    psArray *regions = options->regions->data[index] = psArrayAllocEmpty(ARRAY_BUFFER); // Match regions
    {
	psString regex = NULL;          // Regular expression
	psStringAppend(&regex, "^%s$", PM_SUBTRACTION_ANALYSIS_REGION);
	psMetadataIterator *iter = psMetadataIteratorAlloc(readout->analysis, PS_LIST_HEAD, regex);
	psFree(regex);
	psMetadataItem *item = NULL;// Item from iteration
	while ((item = psMetadataGetAndIncrement(iter))) {
	    assert(item->type == PS_DATA_REGION);
	    regions = psArrayAdd(regions, ARRAY_BUFFER, item->data.V);
	}
	psFree(iter);
    }

    psArray *kernels = options->kernels->data[index] = psArrayAllocEmpty(ARRAY_BUFFER); // Match kernels
    {
	psString regex = NULL;          // Regular expression
	psStringAppend(&regex, "^%s$", PM_SUBTRACTION_ANALYSIS_KERNEL);
	psMetadataIterator *iter = psMetadataIteratorAlloc(readout->analysis, PS_LIST_HEAD, regex);
	psFree(regex);
	psMetadataItem *item = NULL;// Item from iteration
	while ((item = psMetadataGetAndIncrement(iter))) {
	    assert(item->type == PS_DATA_UNKNOWN);
	    pmSubtractionKernels *kernel = item->data.V; // Kernel used in subtraction
	    kernels = psArrayAdd(kernels, ARRAY_BUFFER, kernel);
	}
	psFree(iter);
    }
    psAssert((regions)->n == (kernels)->n, "Number of match regions and kernels should match");

    // Record chi^2
    {
	double sum = 0.0;           // Sum of chi^2
	int num = 0;                // Number of measurements of chi^2
	psString regex = NULL;      // Regular expression
	psStringAppend(&regex, "^%s$", PM_SUBTRACTION_ANALYSIS_KERNEL);
	psMetadataIterator *iter = psMetadataIteratorAlloc(readout->analysis, PS_LIST_HEAD, regex);
	psFree(regex);
	psMetadataItem *item = NULL;// Item from iteration
	while ((item = psMetadataGetAndIncrement(iter))) {
	    assert(item->type == PS_DATA_UNKNOWN);
	    pmSubtractionKernels *kernels = item->data.V; // Convolution kernels
	    sum += kernels->mean;
	    num++;
	}
	psFree(iter);
	options->matchChi2->data.F32[index] = sum / (psImageCovarianceFactor(readout->covariance) * num);
    }

    return true;
}

# define NOISE_FRACTION 0.01             // Set minimum flux to this fraction of noise
# define SOURCE_MASK (PM_SOURCE_MODE_FAIL | PM_SOURCE_MODE_DEFECT | PM_SOURCE_MODE_SATURATED | PM_SOURCE_MODE_CR_LIMIT | PM_SOURCE_MODE_EXT_LIMIT) // Mask to apply to input sources

// generate a fake readout against which to PSF match
pmReadout *makeFakeReadout(pmConfig *config, pmReadout *readoutSrc, psArray *sources, pmPSF *psf, psImageMaskType maskVal, int fullSize) {

    pmReadout *fake = pmReadoutAlloc(NULL); // Fake readout with target PSF

    psStats *bg = psStatsAlloc(PS_STAT_ROBUST_STDEV); // Statistics for background
    psRandom *rng = psRandomAlloc(PS_RANDOM_TAUS); // Random number generator
    if (!psImageBackground(bg, NULL, readoutSrc->image, readoutSrc->mask, maskVal, rng)) {
	psError(PSPHOT_ERR_DATA, false, "Can't measure background for image.");
	psFree(fake);
	psFree(bg);
	psFree(rng);
	return NULL;
    }
    float minFlux = NOISE_FRACTION * bg->robustStdev; // Minimum flux level for fake image
    psFree(rng);
    psFree(bg);

    bool oldThreads = pmReadoutFakeThreads(true); // Old threading state
    if (!pmReadoutFakeFromSources(fake, readoutSrc->image->numCols, readoutSrc->image->numRows, sources, SOURCE_MASK, NULL, NULL, psf, minFlux, fullSize, false, true)) {
	psError(PSPHOT_ERR_DATA, false, "Unable to generate fake image with target PSF.");
	psFree(fake);
	return NULL;
    }
    pmReadoutFakeThreads(oldThreads);

    fake->mask = psImageCopy(NULL, readoutSrc->mask, PS_TYPE_IMAGE_MASK);

    // Add the background into the target image
    psImage *bgImage = stackBackgroundModel(readoutSrc, config); // Image of background
    psBinaryOp(fake->image, fake->image, "+", bgImage);
    psFree(bgImage);

    return fake;
}

// set the widths 
psVector *SetOptWidths (bool *optimum, psMetadata *recipe) {

    bool status;

    *optimum = psMetadataLookupBool(&status, recipe, "OPTIMUM"); // Derive optimum parameters?
    psAssert (status, "missing recipe value %s", "OPTIMUM");

    psVector *optWidths = NULL;         // Vector with FWHMs for optimum search

    if (*optimum) {
	float optMin = psMetadataLookupF32(&status,  recipe, "OPTIMUM.MIN"); // Minimum width for search
	psAssert (status, "missing recipe value %s", "OPTIMUM.MIN");
	
	float optMax = psMetadataLookupF32(&status,  recipe, "OPTIMUM.MAX"); // Maximum width for search
	psAssert (status, "missing recipe value %s", "OPTIMUM.MAX");
	
	float optStep = psMetadataLookupF32(&status, recipe, "OPTIMUM.STEP"); // Step for search
	psAssert (status, "missing recipe value %s", "OPTIMUM.STEP");

	optWidths = psVectorCreate(optWidths, optMin, optMax, optStep, PS_TYPE_F32);
    }

    return optWidths;
}

// Set input to be skipped if the analysis reports that psf matching has failed for some reason.
// For exmaple if decovolution fraction was overlimit or the bad fraction was too high.
//
bool psphotStackSetInputsToSkip(pmConfig *config, const pmFPAview *view, const char *filerule, bool set) {
    int num = psphotFileruleCount(config, filerule);
    bool status;
    int chisqNum = psMetadataLookupS32 (&status, config->arguments, "PSPHOT.CHISQ.NUM");
    if (!status) chisqNum = -1;
    for (int i = 0; i < num; i++) {
        if (i == chisqNum) {
            continue;
        }
        pmFPAfile *file = pmFPAfileSelectSingle(config->files, filerule, i); // File of interest
        psAssert (file, "missing file?");

        pmReadout *readout = pmFPAviewThisReadout(view, file->fpa);
        psAssert (readout, "missing readout?");
        if (set) {
            bool notMatched = psMetadataLookupBool(&status, readout->analysis, "NOT.PSF.MATCHED");
            psMetadataAddBool(readout->analysis, PS_LIST_TAIL, "PSPHOT.SKIP.INPUT", PS_META_REPLACE, "Skip analysis", notMatched);
        } else {
            psMetadataAddBool(readout->analysis, PS_LIST_TAIL, "PSPHOT.SKIP.INPUT", PS_META_REPLACE, "Skip analysis", false);
        }
    }

    return true;
}
