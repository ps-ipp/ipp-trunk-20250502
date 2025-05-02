# include "psphotInternal.h"
static int npass = 0;

// generate the median in NxN boxes, clipping heavily
// linear interpolation to generate full-scale model
bool psphotSubtractBackgroundReadout (pmConfig *config, const pmFPAview *view, const char *filerule, int index, psMetadata *recipe)
{
    bool status = true;
    pmReadout *background = NULL;
    pmReadout *backSub = NULL;

    psTimerStart ("psphot.background");

    // find the currently selected readout
    pmFPAfile *file = pmFPAfileSelectSingle(config->files, filerule, index); // File of interest

    pmFPA *inFPA = file->fpa;
    pmReadout *readout = pmFPAviewThisReadout (view, inFPA);
    psImage *image = readout->image;
    psImage *mask  = readout->mask;

    // find the currently selected readout (XXX note that the model is saved on PSPHOT.BACKMDL regardless of 'filename'
    pmFPAfile *modelFile = pmFPAfileSelectSingle(config->files, psphotGetFilerule("PSPHOT.BACKMDL"), index); // File of interest
    assert (modelFile);

    pmReadout *model = READOUT_OR_INTERNAL(view, modelFile);
    assert (model);

    // user-defined masks to test for good/bad pixels (build from recipe list if not yet set)
    psImageMaskType maskVal = psMetadataLookupImageMask(&status, recipe, "MASK.PSPHOT"); // Mask value for bad pixels
    psAssert (maskVal, "MASK.PSPHOT missing from recipe");

    psImageBinning *binning = psMetadataLookupPtr(&status, model->analysis, "PSPHOT.BACKGROUND.BINNING");
    assert (binning);

    // select background pixels, from output background file, or create
    // XXX for now, we will only allow a single background image to be generated
    file = psMetadataLookupPtr (&status, config->files, psphotGetFilerule("PSPHOT.BACKGND"));
    if (file) {
        // we are using PSPHOT.BACKGND as an I/O file: select readout or create
	background = READOUT_OR_INTERNAL(view, file);
        if (background == NULL) {
            // readout does not yet exist: create from input
            pmFPAfileCopyStructureView (file->fpa, inFPA, 1, 1, view);
            background = pmFPAviewThisReadout (view, file->fpa);
            if ((image->numCols != background->image->numCols) || (image->numRows != background->image->numRows)) {
                psError (PSPHOT_ERR_PROG, true, "inconsistent sizes for background dimensions");
                return false;
            }
        }
    } else {
        background = pmFPAfileDefineInternal (config->files, psphotGetFilerule("PSPHOT.BACKGND"), image->numCols, image->numRows, PS_TYPE_F32);
    }
    psF32 **backData = background->image->data.F32;

    // linear interpolation to full-scale
    if (!psImageUnbin (background->image, model->image, binning)) {
        psError (PSPHOT_ERR_PROG, true, "inconsistent sizes for unbinning");
        return false;
    }

    psLogMsg ("psphot", PS_LOG_MINUTIA, "build resampled image: %f sec\n", psTimerMark ("psphot.background"));

    // back-sub image pixels, from output background file (don't create if not requested)
    // XXX for now, we will only allow a single background-subtracted image to be generated
    file = psMetadataLookupPtr (&status, config->files, psphotGetFilerule("PSPHOT.BACKSUB"));
    if (file) {
        // we are using PSPHOT.BACKSUB as an I/O file: select readout or create
        backSub = pmFPAviewThisReadout (view, file->fpa);
        if (backSub == NULL) {
            // readout does not yet exist: create from input
            pmFPAfileCopyStructureView (file->fpa, inFPA, 1, 1, view);
            backSub = pmFPAviewThisReadout (view, file->fpa);
        }
    }

    if (psTraceGetLevel("psphot") > 5) {
        char name[256];
        sprintf (name, "image.%02d.fits", npass);
        psphotSaveImage (NULL, image, name);
        sprintf (name, "back.%02d.fits", npass);
        psphotSaveImage (NULL, background->image, name);
        sprintf (name, "mask.%02d.fits", npass);
        psphotSaveImage (NULL, mask, name);
        sprintf (name, "backmdl.%02d.fits", npass);
        psphotSaveImage (NULL, model->image, name);
    }

    // subtract the background model (save in backSub, if requested)
    // XXX if needed, multithread this (fairly trivial)
    for (int j = 0; j < image->numRows; j++) {
        for (int i = 0; i < image->numCols; i++) {
            image->data.F32[j][i] -= backData[j][i];
            if (backSub) {
                backSub->image->data.F32[j][i] = image->data.F32[j][i];
            }
        }
    }

    if (psTraceGetLevel("psphot") > 5) {
        char name[256];
        sprintf (name, "backsub.%02d.fits", npass);
        psphotSaveImage (NULL, image, name);
    }
    psLogMsg ("psphot", PS_LOG_WARN, "subtracted background model: %f sec\n", psTimerMark ("psphot.background"));

    // the pmReadout selected in this function are all view on entries in config->files

    // display the backsub and backgnd images
    // move this inthe the subtract background loop
    psphotVisualShowBackground (config, view, readout);

    npass ++;
    return true;
}

bool psphotSubtractBackground (pmConfig *config, const pmFPAview *view, const char *filerule)
{
    bool status = false;

    // select the appropriate recipe information
    psMetadata *recipe  = psMetadataLookupPtr (&status, config->recipes, PSPHOT_RECIPE);
    psAssert (recipe, "missing recipe?");

    int num = psphotFileruleCount(config, filerule);

    // loop over the available readouts
    for (int i = 0; i < num; i++) {
	if (!psphotSubtractBackgroundReadout (config, view, filerule, i, recipe)) {
	    psError (PSPHOT_ERR_CONFIG, false, "failed to subtract background for %s entry %d", filerule, i);
	    return false;
	}
    }
    return true;
}
