# include "psphotInternal.h"

bool psphotSetMaskAndVariance (pmConfig *config, const pmFPAview *view, const char *filerule) {

    bool status = false;

    // select the appropriate recipe information
    psMetadata *recipe  = psMetadataLookupPtr (&status, config->recipes, PSPHOT_RECIPE);
    psAssert (recipe, "missing recipe?");

    int num = psphotFileruleCount(config, filerule);

    // loop over the available readouts
    for (int i = 0; i < num; i++) {

	// Generate the mask and weight images, including the user-defined analysis region of interest
	if (!psphotSetMaskAndVarianceReadout (config, view, filerule, i, recipe)) {
	    psError (PSPHOT_ERR_CONFIG, false, "failed to generate mask for %s entry %d", filerule, i);
	    return false;
	}
    }
    return true;
}

// generate mask and variance if not defined, additional mask for restricted subregion
bool psphotSetMaskAndVarianceReadout (pmConfig *config, const pmFPAview *view, const char *filerule, int index, psMetadata *recipe) {

    bool status;

    pmFPAfile *file = pmFPAfileSelectSingle(config->files, filerule, index); // File of interest
    psAssert (file, "missing file?");

    // find the currently selected readout
    pmReadout  *readout = pmFPAviewThisReadout (view, file->fpa);
    psAssert (readout, "missing readout?");

    // save maskSat and maskBad on the psphot recipe (mostly for psphotRoughClass)
    psImageMaskType maskSat  = pmConfigMaskGet("SAT", config); // Mask value for saturated pixels
    psMetadataAddImageMask (recipe, PS_LIST_TAIL, "MASK.SAT", PS_META_REPLACE, "user-defined mask", maskSat);

    psImageMaskType maskBad  = pmConfigMaskGet("LOW", config); // Mask value for low pixels
    if (!maskBad) {
        // for backward compatability look up old name
        maskBad  = pmConfigMaskGet("BAD", config);
    }
    psMetadataAddImageMask (recipe, PS_LIST_TAIL, "MASK.BAD", PS_META_REPLACE, "user-defined mask", maskBad);

    // generate mask & variance images if they don't already exit
    if (!readout->mask) {
        if (!pmReadoutGenerateMask(readout, maskSat, maskBad)) {
            psError (PSPHOT_ERR_CONFIG, false, "trouble creating mask");
            return false;
        }
    }
    if (!readout->variance) {
        if (!pmReadoutGenerateVariance(readout, NULL, true)) {
            psError (PSPHOT_ERR_CONFIG, false, "trouble creating variance");
            return false;
        }
    }

    // insure than any non-finite pixels in image or variance are masked
    // get the PSPHOT.MASK value from the config
    psImageMaskType maskValue;
    if (!pmConfigMaskSetBits (&maskValue, NULL, config)) {
	psError (PS_ERR_UNKNOWN, false, "Unable to define the mask bit values");
	return false;
    }
    if (!pmReadoutMaskInvalid(readout, maskValue, maskSat)) {
	psError (PS_ERR_UNKNOWN, false,  "Unable to mask invalid pixels in readout.");
	return false;
    }

    bool softenVariance = psMetadataLookupBool (&status, recipe, "SOFTEN.VARIANCE");
    float softenFraction = psMetadataLookupF32 (&status, recipe, "SOFTEN.VARIANCE.FRACTION");

    // make this an option via the recipe
    if (softenVariance) {
      psImage *im = readout->image;
      psImage *wt = readout->variance;
      for (int j = 0; j < im->numRows; j++) {
        for (int i = 0; i < im->numCols; i++) {
	    if (!isfinite(im->data.F32[j][i])) continue;
	    if (!isfinite(wt->data.F32[j][i])) continue;
	    float sysError = softenFraction * im->data.F32[j][i];
	    wt->data.F32[j][i] += PS_SQR(sysError);
        }
      }
    }

    // mask the excluded outer pixels
    // these coordinates refer to the parent image
    // these bounds will saturate on the subimage
    // negative upper bounds will subtract from the *subimage*
    float XMIN  = psMetadataLookupF32 (&status, recipe, "XMIN");
    float XMAX  = psMetadataLookupF32 (&status, recipe, "XMAX");
    float YMIN  = psMetadataLookupF32 (&status, recipe, "YMIN");
    float YMAX  = psMetadataLookupF32 (&status, recipe, "YMAX");
    psRegion valid = psRegionSet (XMIN, XMAX, YMIN, YMAX);

    // restrict the supplied region above to the valid area on the image
    psRegion keep = psRegionForImage (readout->image, valid);

    // psImageKeepRegion assumes the region refers to the parent coordinates
    psImageKeepRegion (readout->mask, keep, "OR", maskBad);

    // test output of files at this stage
    if (psTraceGetLevel("psphot.imsave") >= 5) {
        psphotSaveImage (NULL, readout->image,  "image.fits");
        psphotSaveImage (NULL, readout->mask,   "mask.fits");
        psphotSaveImage (NULL, readout->variance, "variance.fits");
    }

    // display the image, weight, mask (ch 1,2,3)
    psphotVisualShowImage (readout);

    return true;
}

// XXX this function and support below was created to test the theory that the faint-end
// bias results from the Poisson variation of the background pixels.  This is NOT the
// case.  Using the code below maintains the faint-end bias.
bool psphotUpdateVariance (pmConfig *config, const pmFPAview *view, const char *filerule) {

    bool status = false;

    // select the appropriate recipe information
    psMetadata *recipe  = psMetadataLookupPtr (&status, config->recipes, PSPHOT_RECIPE);
    psAssert (recipe, "missing recipe?");

    int num = psphotFileruleCount(config, filerule);

    // loop over the available readouts
    for (int i = 0; i < num; i++) {

	// Generate the mask and weight images, including the user-defined analysis region of interest
	if (!psphotUpdateVarianceReadout (config, view, filerule, i, recipe)) {
	    psError (PSPHOT_ERR_CONFIG, false, "failed to generate mask for %s entry %d", filerule, i);
	    return false;
	}
    }
    return true;
}

// determine the mean variance image (equivalent to the background model, but for the variance image)
// set the variance image to the MAX(input, mean)
bool psphotUpdateVarianceReadout (pmConfig *config, const pmFPAview *view, const char *filerule, int index, psMetadata *recipe) {

    pmFPAfile *file = pmFPAfileSelectSingle(config->files, filerule, index); // File of interest
    psAssert (file, "missing file?");

    // find the currently selected readout
    pmReadout  *readout = pmFPAviewThisReadout (view, file->fpa);
    psAssert (readout, "missing readout?");

    pmSourceFitVarMode varMode = psphotGetFitVarMode (recipe);
    if (varMode == PM_SOURCE_PHOTFIT_NONE) {
      psError (PSPHOT_ERR_CONFIG, false, "need valid LINEAR_FIT_VARIANCE_MODE");
      return false;
    }

    // make this an option via the recipe
    if (varMode != PM_SOURCE_PHOTFIT_MODEL_SKY) return true;

    // create a model variance image (full-scale image to take result of psImageUnbin below)
    psImage *modelVar = psImageCopy (NULL, readout->variance, PS_TYPE_F32);

    // find the binning information
    psImageBinning *backBinning = psphotBackgroundBinning (modelVar, config);
    assert (backBinning);
    
    psImage *varModel = psImageAlloc(backBinning->nXruff, backBinning->nYruff, PS_TYPE_F32); // Background model
    psImage *varModelStdev = psImageAlloc(backBinning->nXruff, backBinning->nYruff, PS_TYPE_F32); // Background model

    if (!psphotModelBackgroundReadout(varModel, varModelStdev, NULL, readout, backBinning, config, true)) {
	psError(PS_ERR_UNKNOWN, false, "Unable to generate background model");
	psFree (varModel);
	psFree (varModelStdev);
	return false;
    }

    // linear interpolation to full-scale
    if (!psImageUnbin (modelVar, varModel, backBinning)) {
	psError (PSPHOT_ERR_PROG, true, "inconsistent sizes for unbinning");
	psFree (varModel);
	psFree (varModelStdev);
	return false;
    }

    // XXX save these?
    psFree (varModel);
    psFree (varModelStdev);

    psImage *im = readout->image;
    psImage *wt = readout->variance;
    for (int j = 0; j < im->numRows; j++) {
      for (int i = 0; i < im->numCols; i++) {
	if (!isfinite(im->data.F32[j][i])) continue;
	if (!isfinite(wt->data.F32[j][i])) continue;
	// XXX for a test, make variance constant wt->data.F32[j][i] = PS_MAX(wt->data.F32[j][i], modelVar->data.F32[j][i]);
	wt->data.F32[j][i] = modelVar->data.F32[j][i];
      }
    }

    // test output of files at this stage
    if (psTraceGetLevel("psphot.imsave") >= 5) {
        psphotSaveImage (NULL, readout->image,  "image.varsky.fits");
        psphotSaveImage (NULL, readout->mask,   "mask.varsky.fits");
        psphotSaveImage (NULL, readout->variance, "variance.varsky.fits");
    }

    psFree (modelVar);

    return true;
}
