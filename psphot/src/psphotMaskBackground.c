# include "psphotInternal.h"
# define NSIGMA 2.0

// mask pixel in the input image above model + N*stdev (using mark)
bool psphotMaskBackgroundReadout (pmConfig *config, const pmFPAview *view, const char *filerule, int index, psMetadata *recipe)
{
    bool status = true;

    psTimerStart ("psphot.background");

    // find the currently selected readout
    pmFPAfile *file = pmFPAfileSelectSingle(config->files, filerule, index); // File of interest
    psAssert (file, "missing file?");

    pmReadout *readout = pmFPAviewThisReadout (view, file->fpa);
    psAssert (readout, "missing readout?");

    // find the currently selected readout (XXX note that the model is saved on PSPHOT.BACKMDL regardless of 'filename'
    pmFPAfile *modelFile = pmFPAfileSelectSingle(config->files, psphotGetFilerule("PSPHOT.BACKMDL"), index); // File of interest
    assert (modelFile);

    pmFPAfile *stdevFile = pmFPAfileSelectSingle(config->files, psphotGetFilerule("PSPHOT.BACKMDL.STDEV"), index);
    assert (stdevFile);

    float skyMean = psMetadataLookupF32(&status, readout->analysis, "SKY_MEAN");
    float skyStdv = psMetadataLookupF32(&status, readout->analysis, "SKY_STDEV");

    pmReadout *model = READOUT_OR_INTERNAL(view, modelFile);
    psAssert (model, "this must exist");

    pmReadout *stdev = READOUT_OR_INTERNAL(view, stdevFile);
    psAssert (stdev, "this must exist");

    // user-defined masks to test for good/bad pixels (build from recipe list if not yet set)
    psImageMaskType maskVal = psMetadataLookupImageMask(&status, recipe, "MASK.PSPHOT"); // Mask value for bad pixels
    assert (maskVal);

    // user-defined masks to test for good/bad pixels (build from recipe list if not yet set)
    psImageMaskType markVal = psMetadataLookupImageMask(&status, recipe, "MARK.PSPHOT"); // Mask value for bad pixels
    assert (markVal);

    psImageBinning *binning = psMetadataLookupPtr(&status, model->analysis, "PSPHOT.BACKGROUND.BINNING");
    assert (binning);

    // create a binned version of the threshold image (= model + NSIGMA*stdev)
    psImage *threshBinned = psImageCopy (NULL, model->image, PS_TYPE_F32);
    for (int iy = 0; iy < model->image->numRows; iy++) {
	for (int ix = 0; ix < model->image->numCols; ix++) {
	    // threshBinned->data.F32[iy][ix] = model->image->data.F32[iy][ix] + NSIGMA*stdev->image->data.F32[iy][ix];
	    threshBinned->data.F32[iy][ix] = skyMean + NSIGMA*skyStdv;
	}
    }
    psphotSaveImage (NULL, threshBinned, "threshbin.fits");

    // create a threshold image 
    psImage *threshold = psImageCopy (NULL, readout->image, PS_TYPE_F32);

    // linear interpolation to full-scale
    if (!psImageUnbin (threshold, threshBinned, binning)) {
        psError (PSPHOT_ERR_PROG, true, "inconsistent sizes for unbinning");
        return false;
    }
    psphotSaveImage (NULL, threshold, "threshold.fits");

    psLogMsg ("psphot", PS_LOG_MINUTIA, "build threshold image: %f sec\n", psTimerMark ("psphot.background"));

    // raise the 'markVal' mask bit for pixels above threshold
    for (int iy = 0; iy < readout->image->numRows; iy++) {
        for (int ix = 0; ix < readout->image->numCols; ix++) {
	    if (readout->mask->data.PS_TYPE_IMAGE_MASK_DATA[iy][ix] & maskVal) continue;
            if (readout->image->data.F32[iy][ix] < threshold->data.F32[iy][ix]) continue;
	    readout->mask->data.PS_TYPE_IMAGE_MASK_DATA[iy][ix] |= markVal;
        }
    }
    psphotSaveImage (NULL, readout->mask, "newmask.fits");
    psLogMsg ("psphot", PS_LOG_INFO, "masked image based on 1st pass background model: %f sec\n", psTimerMark ("psphot.background"));

    psFree (threshold);
    psFree (threshBinned);

    return true;
}

bool psphotMaskBackground (pmConfig *config, const pmFPAview *view, const char *filerule)
{
    bool status = false;

    // select the appropriate recipe information
    psMetadata *recipe  = psMetadataLookupPtr (&status, config->recipes, PSPHOT_RECIPE);
    psAssert (recipe, "missing recipe?");

    int num = psphotFileruleCount(config, filerule);

    // loop over the available readouts
    for (int i = 0; i < num; i++) {
	if (!psphotMaskBackgroundReadout (config, view, filerule, i, recipe)) {
	    psError (PSPHOT_ERR_CONFIG, false, "failed to subtract background for %s entry %d", filerule, i);
	    return false;
	}
    }
    return true;
}
