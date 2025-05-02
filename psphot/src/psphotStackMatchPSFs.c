# include "psphotInternal.h"

bool psphotStackMatchPSFs (pmConfig *config, const pmFPAview *view)
{
    bool status = true;

    psLogMsg ("psphot", PS_LOG_INFO, "--- psphotStack Match PSFs ---");

    psMetadata *recipe = psMetadataLookupMetadata(NULL, config->recipes, "PSPHOT"); 
    psAssert(recipe, "We've thrown an error on this before.");

    int num = psphotFileruleCount(config, "PSPHOT.INPUT");

    // 'options' carries info needed to perform the stack matching
    psphotStackOptions *options = psphotStackOptionsAlloc(num);

    options->convolve = psMetadataLookupBool (&status, recipe, "PSPHOT.STACK.MATCH.PSF");
    psAssert (status, "PSPHOT.STACK.MATCH.PSF not in recipe");

    if (options->convolve) {
	char *convolveSource = psMetadataLookupStr (&status, recipe, "PSPHOT.STACK.MATCH.PSF.SOURCE");
	options->convolveSource = psphotStackConvolveSourceFromString (convolveSource);
	if (options->convolveSource == PSPHOT_CNV_SRC_NONE) {
	    psError (PSPHOT_ERR_CONFIG, true, "stack convolution source not defined in recipe");
	    return false;
	}
    }

    // loop over the available readouts
    for (int i = 0; i < num; i++) {
	// set up the PSF-matching parameters describing the input images
	if (!psphotStackMatchPSFsPrepare (config, view, options, i)) {
            psError (PSPHOT_ERR_CONFIG, false, "failed to set PSF matching options for entry %d", i);
	    return false;
	}
    }

    // XXX convolve == false might not be valid at the moment
    if (options->convolve) {
	// Determine the 1st target PSF (either AUTO or defined by PSPHOT.STACK.TARGET.PSF.FWHM)
	// NOTE: this also set the full list of target FWHMs (options->targetSeeing)
        if (!psphotStackPSF(config, options)) {
            psError(psErrorCodeLast(), false, "Unable to determine output PSF.");
            return false;
        }
    }

    // loop over the available readouts (ignore chisq image)
    for (int i = 0; i < num; i++) { 
	if (!psphotStackMatchPSFsReadout (config, view, options, i)) {
            psError (PSPHOT_ERR_CONFIG, false, "failed to find initial detections for PSPHOT.INPUT entry %d", i);
	    return false;
	}
    }

    psFree (options);
    return true;
}

// convolve the image to match desired PSF
// XXX is this code consistent with 'convolve' = false?  XXX: No
bool psphotStackMatchPSFsReadout (pmConfig *config, const pmFPAview *view, psphotStackOptions *options, int index) {

    psImageMaskType maskValue;
    psImageMaskType markValue;

    psLogMsg("psphot", PS_LOG_DETAIL, "-- starting PSF matching for readout %d --", index);

    // get the PSPHOT.MASK value from the config
    if (!pmConfigMaskSetBits (&maskValue, &markValue, config)) {
	psError (PS_ERR_UNKNOWN, true, "Unable to define the mask bit values");
	return false;
    }

    pmFPAfile *fileSrc = psphotStackGetConvolveSource(config, options, index);
    if (!fileSrc) {
	psError(PSPHOT_ERR_CONFIG, false, "desired convolution source is missing");
	return false;
    }

    pmFPAfile *fileOut = pmFPAfileSelectSingle(config->files, "PSPHOT.STACK.OUTPUT.IMAGE", index); // File of interest
    psAssert (fileOut, "missing output file?");

    pmReadout *readoutSrc = pmFPAviewThisReadout(view, fileSrc->fpa);
    psAssert (readoutSrc, "missing readout?");

    pmReadout *readoutOut = pmFPAviewThisReadout(view, fileOut->fpa);
    if (readoutOut == NULL) {
	readoutOut = pmFPAGenerateReadout(config, view, "PSPHOT.STACK.OUTPUT.IMAGE", fileSrc->fpa, NULL, index);
	psAssert (readoutOut, "missing readout?");
    }

    // set NAN pixels to 'SAT'
    psImageMaskType maskSat = pmConfigMaskGet("SAT", config);
    if (!pmReadoutMaskInvalid(readoutSrc, maskValue, maskSat)) {
        psError(psErrorCodeLast(), false, "Unable to mask non-finite pixels in readout.");
        return false;
    }

    // Image Matching (PSFs or just flux)
    if (options->convolve) {
	if (!matchKernel(config, readoutOut, readoutSrc, options, index)){
	    psError(psErrorCodeLast(), false, "Unable to match image PSF in readout.");
	    return false;
	}
	saveMatchData(readoutOut, options, index);
    }
    bool notMatched = psMetadataLookupBool(NULL, readoutOut->analysis, "NOT.PSF.MATCHED");
    if (notMatched) {
        psFree(readoutOut->image);
        readoutOut->image = psImageCopy(NULL, readoutSrc->image, PS_TYPE_F32);
        psFree(readoutOut->variance);
        if (readoutSrc->variance) {
            readoutOut->variance = psImageCopy(NULL, readoutSrc->variance, PS_TYPE_F32);
        }
        psFree(readoutOut->mask);
        if (readoutSrc->mask) {
            readoutOut->mask = psImageCopy(NULL, readoutSrc->mask, PS_TYPE_IMAGE_MASK);
        }
    }

    // renormalize the stack variances to have sigma = 1.0
    if (!psphotStackRenormaliseVariance(config, readoutOut)) {
        psError(psErrorCodeLast(), false, "Unable to renormalise variance.");
        return false;
    }

    // save the output fwhm values in the readout->analysis.  we may have / will have multiple output PSF sizes,
    // so we save this in a vector.  if the vector is not yet defined, create it
    // Skip this if psf matching failed for this readout. For example if deconvolution fraction was over the limit.
    // NOTE: fwhmValues as defined here has 1 + nMatched PSF : 0 == unmatched
    psVector *fwhmValues = psVectorAllocEmpty(10, PS_TYPE_F32);
    psVectorAppend(fwhmValues, NAN); // XXX this corresponds to the unmatched image set

    if (!notMatched) {
        for (int i = 0; i < options->targetSeeing->n; i++) {
            psVectorAppend(fwhmValues, options->targetSeeing->data.F32[i]);
        }
    }
    psMetadataAddVector(readoutSrc->analysis, PS_LIST_TAIL, "STACK.PSF.FWHM.VALUES", PS_META_REPLACE, "PSF sizes", fwhmValues);
    psMetadataAddVector(readoutOut->analysis, PS_LIST_TAIL, "STACK.PSF.FWHM.VALUES", PS_META_REPLACE, "PSF sizes", fwhmValues);
    psFree(fwhmValues); // drops this function's reference

    return true;
}
