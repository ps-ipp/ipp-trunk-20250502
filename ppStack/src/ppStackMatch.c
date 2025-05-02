#include "ppStack.h"

#define ARRAY_BUFFER 16                 // Number to add to array at a time
#define MAG_IGNORE 50                   // Ignore magnitudes fainter than this --- they're not real!
#define FAKE_SIZE 1                     // Size of fake convolution kernel
#define COVAR_FRAC 0.01                 // Truncation fraction for covariance matrix

// #define TESTING                      // Enable debugging output
// #define TESTING_REUSE_CONV		// Enable debugging for re-using convolved outputs from previous run
// #define TESTING_CZW 

#ifdef TESTING_REUSE_CONV
// Read a FITS image
static bool readImage(psImage **target, // Target for image
                      const char *name, // Name of FITS file
                      const pmConfig *config // Configuration
    )
{
    psString resolved = pmConfigConvertFilename(name, config, false, false); // Resolved filename
    psFits *fits = psFitsOpen(resolved, "r");
    psFree(resolved);
    if (!fits) {
        psError(PPSTACK_ERR_IO, false, "Unable to open previously produced image: %s", name);
        return false;
    }
    psImage *image = psFitsReadImage(fits, psRegionSet(0,0,0,0), 0); // Image of interest
    if (!image) {
        psError(PPSTACK_ERR_IO, false, "Unable to read previously produced image: %s", name);
        psFitsClose(fits);
        return false;
    }
    psFitsClose(fits);

    psFree(*target);
    *target = image;

    return true;
}
#endif


// Add background into the fake image
// Based on ppSubBackground()
static psImage *stackBackgroundModel(pmReadout *ro, // Readout for which to generate background model
                                     const pmConfig *config // Configuration
    )
{
    psAssert(ro && ro->image, "Need readout image");
    psAssert(config, "Need configuration");

    psImage *image = ro->image;         // Image of interest
    int numCols = image->numCols, numRows = image->numRows; // Size of image

    psMetadata *ppStackRecipe = psMetadataLookupPtr(NULL, config->recipes, PPSTACK_RECIPE);
    psAssert(ppStackRecipe, "Need PPSTACK recipe");
    psMetadata *psphotRecipe = psMetadataLookupPtr(NULL, config->recipes, PSPHOT_RECIPE);
    psAssert(psphotRecipe, "Need PSPHOT recipe");

    psString maskBadStr = psMetadataLookupStr(NULL, ppStackRecipe, "MASK.BAD");// Name of bits to mask for bad
    psMaskType maskBad = pmConfigMaskGet(maskBadStr, config); // Bits to mask for bad pixels

    // user-defined masks to test for good/bad pixels (build from recipe list if not yet set)
    psMetadataAddImageMask(psphotRecipe, PS_LIST_TAIL, "MASK.PSPHOT", PS_META_REPLACE, "user-defined mask", maskBad);

    psImage *binned = psphotModelBackgroundReadoutNoFile(ro, config); // Binned background model
    psImageBinning *binning = psMetadataLookupPtr(NULL, ro->analysis,
                                                  "PSPHOT.BACKGROUND.BINNING"); // Binning for model
    psAssert(binning, "Need binning parameters");
    psImage *unbinned = psImageAlloc(numCols, numRows, PS_TYPE_F32); // Unbinned background model
    if (!psImageUnbin(unbinned, binned, binning)) {
        psError(PPSTACK_ERR_DATA, false, "Unable to unbin background model");
        psFree(binned);
        psFree(unbinned);
        return NULL;
    }
    psFree(binned);

    return unbinned;
}

// Renormalise a readout's variance map
bool stackRenormaliseReadout(const pmConfig *config, // Configuration
                             pmReadout *readout      // Readout to renormalise
    )
{
#if 1
    bool mdok; // Status of metadata lookups

    psMetadata *recipe = psMetadataLookupPtr(NULL, config->recipes, PPSTACK_RECIPE); // Recipe for ppStack
    psAssert(recipe, "Need PPSTACK recipe");

    if (!psMetadataLookupBool(&mdok, recipe, "RENORM")) return true;

    int num = psMetadataLookupS32(&mdok, recipe, "RENORM.NUM");
    if (!mdok) {
        psError(PPSTACK_ERR_CONFIG, true, "RENORM.NUM is not set in the recipe");
        return false;
    }
    float minValid = psMetadataLookupF32(&mdok, recipe, "RENORM.MIN");
    if (!mdok) {
        psError(PPSTACK_ERR_CONFIG, true, "RENORM.MIN is not set in the recipe");
        return false;
    }
    float maxValid = psMetadataLookupF32(&mdok, recipe, "RENORM.MAX");
    if (!mdok) {
        psError(PPSTACK_ERR_CONFIG, true, "RENORM.MAX is not set in the recipe");
        return false;
    }

    psImageMaskType maskBad = pmConfigMaskGet("BLANK", config); // Bits to mask

    psImageCovarianceTransfer(readout->variance, readout->covariance);
    return pmReadoutVarianceRenormalise(readout, maskBad, num, minValid, maxValid);
#else
    return true;
#endif
}



bool ppStackMatch(pmReadout *readout, const psImage *target, ppStackOptions *options,
                  int index, const pmConfig *config)
{
    assert(readout);
    assert(options);
    assert(config);

    psMetadata *recipe = psMetadataLookupMetadata(NULL, config->recipes, PPSTACK_RECIPE); // ppStack recipe
    psAssert(recipe, "We've thrown an error on this before.");

    float deconvLimit = psMetadataLookupF32(NULL, recipe, "DECONV.LIMIT"); // Limit on deconvolution fraction

    // Look up appropriate values from the ppSub recipe
    psMetadata *ppsub = psMetadataLookupMetadata(NULL, config->recipes, "PPSUB"); // PPSUB recipe
    int size = psMetadataLookupS32(NULL, ppsub, "KERNEL.SIZE"); // Kernel half-size

    psString maskValStr = psMetadataLookupStr(NULL, ppsub, "MASK.VAL"); // Name of bits to mask going in
    psImageMaskType maskVal = pmConfigMaskGet(maskValStr, config); // Bits to mask going in to pmSubtractionMatch
    psString maskPoorStr = psMetadataLookupStr(NULL, recipe, "MASK.POOR"); // Name of bits to mask for poor
    psImageMaskType maskPoor = pmConfigMaskGet(maskPoorStr, config); // Bits to mask for poor pixels
    psString maskBadStr = psMetadataLookupStr(NULL, recipe, "MASK.BAD"); // Name of bits to mask for bad
    psImageMaskType maskBad = pmConfigMaskGet(maskBadStr, config); // Bits to mask for bad pixels

    bool mdok;                          // Status of MD lookup
    float penalty = psMetadataLookupF32(NULL, ppsub, "PENALTY"); // Penalty for wideness
    int threads = psMetadataLookupS32(NULL, config->arguments, "NTHREADS"); // Number of threads

    // Replaced pmReadoutMaskNonfinite with pmReadoutMaskInvalid (tests for already masked pixels)
    if (!pmReadoutMaskInvalid(readout, maskVal, maskBad)) {
        psError(psErrorCodeLast(), false, "Unable to mask non-finite pixels in readout.");
        return false;
    }

    // Match the PSF
    if (options->convolve) {
        pmReadout *conv = pmReadoutAlloc(NULL); // Conv readout, for holding results temporarily

        // MEH -- earlier defn needed with new simple mode modifications to  use TESTING_REUSE_CONV...
	//const char *typeStr = psMetadataLookupStr(NULL, ppsub, "KERNEL.TYPE"); // Kernel type
	//pmSubtractionKernelsType type = pmSubtractionKernelsTypeFromString(typeStr); // Kernel type

#ifdef TESTING_REUSE_CONV
        // This is a hack to use the temporary convolved images and kernel generated previously.
        // This makes the 'matching' operation much faster, allowing debugging of the stack process easier.
        // It implicitly assumes the output root name is the same between invocations.

        // Read previously produced kernel
        if (psMetadataLookupBool(NULL, config->arguments, "PPSTACK.DEBUG.STACK")) {
            pmFPAfile *file = pmFPAfileSelectSingle(config->files, "PPSTACK.CONV.KERNEL", index);
            psAssert(file, "Require file");

            pmFPAview *view = pmFPAviewAlloc(0); // View to readout of interest
            view->chip = view->cell = view->readout = 0;
            psString filename = pmFPAfileNameFromRule(file->filerule, file, view); // Filename of interest

            // Read convolution kernel
            psString resolved = pmConfigConvertFilename(filename, config, false, false); // Resolved filename
            psFree(filename);
            psFits *fits = psFitsOpen(resolved, "r"); // FITS file for subtraction kernel
            psFree(resolved);
            if (!fits || !pmReadoutReadSubtractionKernels(conv, fits)) {
                psError(PPSTACK_ERR_IO, false, "Unable to read previously produced kernel");
                psFitsClose(fits);
                return false;
            }
            psFitsClose(fits);

            if (!readImage(&readout->image, options->convImages->data[index], config) ||
                !readImage(&readout->mask, options->convMasks->data[index], config) ||
                !readImage(&readout->variance, options->convVariances->data[index], config)) {
                psError(PPSTACK_ERR_IO, false, "Unable to read previously produced image.");
                return false;
            }

            psRegion *region = psMetadataLookupPtr(NULL, conv->analysis,
                                                   PM_SUBTRACTION_ANALYSIS_REGION); // Convolution region
            pmSubtractionKernels *kernels = psMetadataLookupPtr(NULL, conv->analysis,
                                                                PM_SUBTRACTION_ANALYSIS_KERNEL);

            pmSubtractionAnalysis(conv->analysis, NULL, kernels, region,
                                  readout->image->numCols, readout->image->numRows);

            psKernel *kernel = pmSubtractionKernel(kernels, 0.0, 0.0, false); // Convolution kernel
            bool oldThreads = psImageCovarianceSetThreads(true);              // Old thread setting
            psKernel *covar = psImageCovarianceCalculate(kernel, readout->covariance); // Covariance matrix
            psImageCovarianceSetThreads(oldThreads);
            psFree(readout->covariance);
            readout->covariance = covar;
            psFree(kernel);
        } else {
#endif
            // Normal operations here
            psAssert(target, "Require target PSF image");

            int order = psMetadataLookupS32(NULL, ppsub, "SPATIAL.ORDER"); // Spatial polynomial order
            float regionSize = psMetadataLookupF32(NULL, ppsub, "REGION.SIZE"); // Size of iso-kernel regs
            float spacing = psMetadataLookupF32(NULL, ppsub, "STAMP.SPACING"); // Typical stamp spacing
            int footprint = psMetadataLookupS32(NULL, ppsub, "STAMP.FOOTPRINT"); // Stamp half-size
            float threshold = psMetadataLookupF32(NULL, ppsub, "STAMP.THRESHOLD"); // Threshold for stmps
            int stride = psMetadataLookupS32(NULL, ppsub, "STRIDE"); // Size of convolution patches
            int iter = psMetadataLookupS32(NULL, ppsub, "ITER"); // Rejection iterations
            float rej = psMetadataLookupF32(NULL, ppsub, "REJ"); // Rejection threshold
            float kernelError = psMetadataLookupF32(NULL, ppsub, "KERNEL.ERR"); // Relative systematic error in kernel
            float normFrac = psMetadataLookupF32(NULL, ppsub, "NORM.FRAC"); // Fraction of window for normalisn windw
            float sysError = psMetadataLookupF32(NULL, ppsub, "SYS.ERR"); // Relative systematic error in images
            float skyErr = psMetadataLookupF32(NULL, ppsub, "SKY.ERR"); // Additional error in sky
            float covarFrac = psMetadataLookupF32(NULL, ppsub, "COVAR.FRAC"); // Fraction for covariance calculation

            const char *typeStr = psMetadataLookupStr(NULL, ppsub, "KERNEL.TYPE"); // Kernel type
            pmSubtractionKernelsType type = pmSubtractionKernelsTypeFromString(typeStr); // Kernel type
            psVector *widths = psMetadataLookupPtr(NULL, ppsub, "ISIS.WIDTHS"); // ISIS Gaussian widths
            psVector *orders = psMetadataLookupPtr(NULL, ppsub, "ISIS.ORDERS"); // ISIS Polynomial orders
            int inner = psMetadataLookupS32(NULL, ppsub, "INNER"); // Inner radius
            int ringsOrder = psMetadataLookupS32(NULL, ppsub, "RINGS.ORDER"); // RINGS polynomial order
            int binning = psMetadataLookupS32(NULL, ppsub, "SPAM.BINNING"); // Binning for SPAM kernel
            float badFrac = psMetadataLookupF32(NULL, ppsub, "BADFRAC"); // Maximum bad fraction
            bool optimum = psMetadataLookupBool(&mdok, ppsub, "OPTIMUM"); // Derive optimum parameters?
            float optMin = psMetadataLookupF32(&mdok, ppsub, "OPTIMUM.MIN"); // Minimum width for search
            float optMax = psMetadataLookupF32(&mdok, ppsub, "OPTIMUM.MAX"); // Maximum width for search
            float optStep = psMetadataLookupF32(&mdok, ppsub, "OPTIMUM.STEP"); // Step for search
            float optThresh = psMetadataLookupF32(&mdok, ppsub, "OPTIMUM.TOL"); // Tolerance for search
            int optOrder = psMetadataLookupS32(&mdok, ppsub, "OPTIMUM.ORDER"); // Order for search
            float poorFrac = psMetadataLookupF32(&mdok, ppsub, "POOR.FRACTION"); // Fraction for "poor"

            bool scale = psMetadataLookupBool(NULL, ppsub, "SCALE");        // Scale kernel parameters?
            float scaleRef = psMetadataLookupF32(NULL, ppsub, "SCALE.REF"); // Reference for scaling
            float scaleMin = psMetadataLookupF32(NULL, ppsub, "SCALE.MIN"); // Minimum for scaling
            float scaleMax = psMetadataLookupF32(NULL, ppsub, "SCALE.MAX"); // Maximum for scaling
            if (!isfinite(scaleRef) || !isfinite(scaleMin) || !isfinite(scaleMax)) {
                psError(PPSTACK_ERR_CONFIG, false,
                        "Scale parameters (SCALE.REF=%f, SCALE.MIN=%f, SCALE.MAX=%f) not set in PPSUB recipe.",
                        scaleRef, scaleMin, scaleMax);
                return false;
            }


            // These values are specified specifically for stacking
            const char *stampsName = psMetadataLookupStr(NULL, config->arguments, "STAMPS");// Stamps filename

            psVector *optWidths = NULL;         // Vector with FWHMs for optimum search
            if (optimum) {
                optWidths = psVectorCreate(optWidths, optMin, optMax, optStep, PS_TYPE_F32);
            }

            pmReadout *fake = pmReadoutAlloc(NULL);
            fake->image = psImageCopy(NULL, target, PS_TYPE_F32);
            fake->mask = psImageCopy(NULL, readout->mask, PS_TYPE_IMAGE_MASK);

#if 1
            // Add the background into the target image
            psImage *bgImage = stackBackgroundModel(readout, config); // Image of background
	    psBinaryOp(fake->image, fake->image, "+", bgImage);
            psFree(bgImage);
#endif

#ifdef TESTING_CZW
            {
                pmHDU *hdu = pmHDUFromCell(readout->parent);
                psString name = NULL;
                psStringAppend(&name, "/local/tmp/fake_%03d.fits", index);
                pmStackVisualPlotTestImage(fake->image, name);
                psFits *fits = psFitsOpen(name, "w");
                psFree(name);
                psFitsWriteImage(fits, hdu->header, fake->image, 0, NULL);
                psFitsClose(fits);
            }
            {
                pmHDU *hdu = pmHDUFromCell(readout->parent);
                psString name = NULL;
                psStringAppend(&name, "/local/tmp/real_%03d.fits", index);
                pmStackVisualPlotTestImage(readout->image, name);
                psFits *fits = psFitsOpen(name, "w");
                psFree(name);
                psFitsWriteImage(fits, hdu->header, readout->image, 0, NULL);
                psFitsClose(fits);
            }
            {
                pmHDU *hdu = pmHDUFromCell(readout->parent);
                psString name = NULL;
                psStringAppend(&name, "/local/tmp/real_var_%03d.fits", index);
                pmStackVisualPlotTestImage(readout->variance, name);
                psFits *fits = psFitsOpen(name, "w");
                psFree(name);
                psFitsWriteImage(fits, hdu->header, readout->variance, 0, NULL);
                psFitsClose(fits);
            }
#endif

            if (threads > 0) {
                pmSubtractionThreadsInit();
            }

            // Do the image matching
            pmSubtractionKernels *kernel = psMetadataLookupPtr(&mdok, readout->analysis,
                                                               PM_SUBTRACTION_ANALYSIS_KERNEL); // Conv kernel
            if (kernel) {
                if (!pmSubtractionMatchPrecalc(NULL, conv, fake, readout, readout->analysis,
                                               stride, kernelError, covarFrac, maskVal, maskBad, maskPoor,
                                               poorFrac, badFrac)) {
                    psError(psErrorCodeLast(), false, "Unable to convolve images.");
                    psFree(fake);
                    psFree(optWidths);
                    psFree(conv);
                    if (threads > 0) {
                        pmSubtractionThreadsFinalize();
                    }
                    return false;
                }
            } else {
		// we need to register the FWHM values for use downstream 
		pmSubtractionSetFWHMs(options->targetSeeing, options->inputSeeing->data.F32[index]);

                // Scale the input parameters
                psVector *widthsCopy = psVectorCopy(NULL, widths, PS_TYPE_F32); // Copy of kernel widths

		pmSubtractionParamScaleOptions(scale, scaleRef, scaleMin, scaleMax);

		// XXX EAM : the kernel scaling process has changed: the scale is now set
		// inside pmSubtractionMatch after the normalization window is measured

                // if (scale && !pmSubtractionParamsScale(&size, &footprint, widthsCopy, scaleRef, scaleMin, scaleMax)) {
                //     psError(psErrorCodeLast(), false, "Unable to scale kernel parameters");
                //     psFree(fake);
                //     psFree(optWidths);
                //     psFree(conv);
                //     psFree(widthsCopy);
                //     if (threads > 0) {
                //         pmSubtractionThreadsFinalize();
                //     }
                //     return false;
                // }

                if (!pmSubtractionMatch(NULL, conv, fake, readout, footprint, stride, regionSize, spacing,
                                        threshold, options->sources, stampsName, type, size, order, widthsCopy,
                                        orders, inner, ringsOrder, binning, penalty,
                                        optimum, optWidths, optOrder, optThresh, iter, rej, normFrac,
                                        sysError, skyErr, kernelError, covarFrac, maskVal, maskBad, maskPoor,
                                        poorFrac, badFrac, PM_SUBTRACTION_MODE_2)) {
                    psError(psErrorCodeLast(), false, "Unable to match images.");
                    psFree(fake);
                    psFree(optWidths);
                    psFree(conv);
                    psFree(widthsCopy);
                    if (threads > 0) {
                        pmSubtractionThreadsFinalize();
                    }
                    return false;
                }
                psFree(widthsCopy);
            }


#ifdef TESTING_CZW
            {
                pmHDU *hdu = pmHDUFromCell(readout->parent);
                psString name = NULL;
                psStringAppend(&name, "/local/tmp/conv_%03d.fits", index);
                pmStackVisualPlotTestImage(conv->image, name);
                psFits *fits = psFitsOpen(name, "w");
                psFree(name);
                psFitsWriteImage(fits, hdu->header, conv->image, 0, NULL);
                psFitsClose(fits);
            }
            {
                pmHDU *hdu = pmHDUFromCell(readout->parent);
                psString name = NULL;
                psStringAppend(&name, "/local/tmp/conv_var_%03d.fits", index);
                pmStackVisualPlotTestImage(conv->variance, name);
                psFits *fits = psFitsOpen(name, "w");
                psFree(name);
                psFitsWriteImage(fits, hdu->header, conv->variance, 0, NULL);
                psFitsClose(fits);
            }
            {
                pmHDU *hdu = pmHDUFromCell(readout->parent);
                psString name = NULL;
                psStringAppend(&name, "/local/tmp/diff_%03d.fits", index);
                pmStackVisualPlotTestImage(fake->image, name);
                psFits *fits = psFitsOpen(name, "w");
                psFree(name);
                psBinaryOp(fake->image, conv->image, "-", fake->image);
                psFitsWriteImage(fits, hdu->header, fake->image, 0, NULL);
                psFitsClose(fits);
            }
#endif

            psFree(fake);
            psFree(optWidths);

            if (threads > 0) {
                pmSubtractionThreadsFinalize();
            }

            // Replace original images with convolved
            psFree(readout->image);
            psFree(readout->mask);
            psFree(readout->variance);
            psFree(readout->covariance);
            readout->image  = psMemIncrRefCounter(conv->image);
            readout->mask   = psMemIncrRefCounter(conv->mask);
            readout->variance = psMemIncrRefCounter(conv->variance);
            readout->covariance = psImageCovarianceTruncate(conv->covariance, COVAR_FRAC);
#ifdef TESTING_REUSE_CONV
        }
#endif

        // Extract the regions and solutions used in the image matching
        // This stops them from being freed when we iterate back up the FPA
        psArray *regions = options->regions->data[index] = psArrayAllocEmpty(ARRAY_BUFFER); // Match regions
        {
            psString regex = NULL;          // Regular expression
            psStringAppend(&regex, "^%s$", PM_SUBTRACTION_ANALYSIS_REGION);
            psMetadataIterator *iter = psMetadataIteratorAlloc(conv->analysis, PS_LIST_HEAD, regex);
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
            psMetadataIterator *iter = psMetadataIteratorAlloc(conv->analysis, PS_LIST_HEAD, regex);
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
            psMetadataIterator *iter = psMetadataIteratorAlloc(conv->analysis, PS_LIST_HEAD, regex);
            psFree(regex);
            psMetadataItem *item = NULL;// Item from iteration
            while ((item = psMetadataGetAndIncrement(iter))) {
                assert(item->type == PS_DATA_UNKNOWN);
                pmSubtractionKernels *kernels = item->data.V; // Convolution kernels
		// EAM/MEH sum the RMS instead of the mean now  
		sum += kernels->rms;
                num++;
		//				psLogMsg("ppStack", PS_LOG_INFO, "KERNMATCHDUMP %d %d %g %g %g %g\n",
		//					 index,num,kernels->mean,kernels->rms,sum,psImageCovarianceFactor(readout->covariance));
            }
            psFree(iter);

	    // Apply the covariance factor to the chi^2 value
	    double covar_factor = 1.0;
	    if (type != PM_SUBTRACTION_KERNEL_SIMPLE) { // Except if we're using simple kernels.
	      covar_factor = psImageCovarianceFactor(readout->covariance);
	    }
            options->matchChi2->data.F32[index] = sum / (covar_factor * num);

        }

        // Kernel normalisation
        {
            double sum = 0.0;           // Sum of chi^2
            int num = 0;                // Number of measurements of chi^2
            psString regex = NULL;      // Regular expression
            psStringAppend(&regex, "^%s$", PM_SUBTRACTION_ANALYSIS_NORM);
            psMetadataIterator *iter = psMetadataIteratorAlloc(conv->analysis, PS_LIST_HEAD, regex);
            psFree(regex);
            psMetadataItem *item = NULL;// Item from iteration
            while ((item = psMetadataGetAndIncrement(iter))) {
                assert(item->type == PS_DATA_F32);
                float norm = item->data.F32; // Normalisation
                sum += norm;
                num++;
            }
            psFree(iter);
            float conv = sum/num;       // Mean normalisation from convolution
            float stars = powf(10.0, -0.4 * options->norm->data.F32[index]); // Normalisation from stars
            float renorm =  stars / conv; // Renormalisation to apply
            psLogMsg("ppStack", PS_LOG_INFO, "Renormalising image %d by %f (kernel: %f, stars: %f)\n",
                     index, renorm, conv, stars);
            psBinaryOp(readout->image, readout->image, "*", psScalarAlloc(renorm, PS_TYPE_F32));
            psBinaryOp(readout->variance, readout->variance, "*", psScalarAlloc(PS_SQR(renorm), PS_TYPE_F32));
        }

        // Reject image completely if the maximum deconvolution fraction exceeds the limit
        float deconv = psMetadataLookupF32(NULL, conv->analysis,
                                           PM_SUBTRACTION_ANALYSIS_DECONV_MAX); // Max deconvolution fraction
        if (deconv > deconvLimit) {
            psWarning("Maximum deconvolution fraction (%f) exceeds limit (%f) --- rejecting image %d\n",
                      deconv, deconvLimit, index);
            psFree(conv);
            return NULL;
        }

        readout->analysis = psMetadataCopy(readout->analysis, conv->analysis);

        psFree(conv);
    } else {
	// this branch is done if options->convolve is FALSE
        // Match the normalisation
        float norm = powf(10.0, -0.4 * options->norm->data.F32[index]); // Normalisation
        psBinaryOp(readout->image, readout->image, "*", psScalarAlloc(norm, PS_TYPE_F32));
        psBinaryOp(readout->variance, readout->variance, "*", psScalarAlloc(PS_SQR(norm), PS_TYPE_F32));
    }

    // Ensure the background value is zero
    psStats *bg = psStatsAlloc(PS_STAT_ROBUST_MEDIAN | PS_STAT_ROBUST_STDEV); // Statistics for background
    psRandom *rng = psRandomAlloc(PS_RANDOM_TAUS); // Random number generator
    if (!psImageBackground(bg, NULL, readout->image, readout->mask, maskVal | maskBad, rng)) {
        psWarning("Can't measure background for image.");
        psErrorClear();
    } else {
        if (!psMetadataLookupBool(NULL, config->arguments, "PPSTACK.SKIP.BG.SUB")) {
            psLogMsg("ppStack", PS_LOG_INFO, "Correcting convolved image background by %lf (+/- %lf)",
                     psStatsGetValue(bg, PS_STAT_ROBUST_MEDIAN), psStatsGetValue(bg, PS_STAT_ROBUST_STDEV));
            (void)psBinaryOp(readout->image, readout->image, "-",
                             psScalarAlloc(psStatsGetValue(bg, PS_STAT_ROBUST_MEDIAN), PS_TYPE_F32));
        }
    }

#ifdef TESTING_CZW
    // CZW begin check of weight before renormalize step
    // Measure the variance level for the weighting
    if (psMetadataLookupBool(NULL, recipe, "WEIGHTS")) {
        if (!psImageBackground(bg, NULL, readout->variance, readout->mask, maskVal | maskBad, rng)) {
            psError(PPSTACK_ERR_DATA, false, "Can't measure mean variance for image.");
            psFree(rng);
            psFree(bg);
            return false;
        }
        options->weightings->data.F32[index] = 1.0 / (psStatsGetValue(bg, PS_STAT_ROBUST_MEDIAN) *
                                                      psImageCovarianceFactor(readout->covariance));
    } else {
        options->weightings->data.F32[index] = 1.0;
    }
    psLogMsg("ppStack", PS_LOG_INFO, "Prenorm Weighting for image %d is %g covar %g median %g\n",
             index, options->weightings->data.F32[index],psImageCovarianceFactor(readout->covariance),
	     psStatsGetValue(bg, PS_STAT_ROBUST_MEDIAN));
    // CZW end
#endif
    if (!stackRenormaliseReadout(config, readout)) {
        psFree(rng);
        psFree(bg);
        return false;
    }
    
    // Measure the variance level for the weighting
    if (psMetadataLookupBool(NULL, recipe, "WEIGHTS")) {
        if (!psImageBackground(bg, NULL, readout->variance, readout->mask, maskVal | maskBad, rng)) {
            psError(PPSTACK_ERR_DATA, false, "Can't measure mean variance for image.");
            psFree(rng);
            psFree(bg);
            return false;
        }
        options->weightings->data.F32[index] = 1.0 / (psStatsGetValue(bg, PS_STAT_ROBUST_MEDIAN) *
                                                      psImageCovarianceFactor(readout->covariance));
    } else {
        options->weightings->data.F32[index] = 1.0;
    }
    psLogMsg("ppStack", PS_LOG_INFO, "Weighting for image %d is %g covar %g median %g\n",
             index, options->weightings->data.F32[index],psImageCovarianceFactor(readout->covariance),
	     psStatsGetValue(bg, PS_STAT_ROBUST_MEDIAN));

    psFree(rng);
    psFree(bg);

#ifdef TESTING
    {
        pmHDU *hdu = pmHDUFromCell(readout->parent);
        psString name = NULL;
        psStringAppend(&name, "convolved_%03d.fits", index);
        pmStackVisualPlotTestImage(readout->image, name);
        psFits *fits = psFitsOpen(name, "w");
        psFree(name);
        psFitsWriteImage(fits, hdu->header, readout->image, 0, NULL);
        psFitsClose(fits);
    }
#endif

    return true;
}
