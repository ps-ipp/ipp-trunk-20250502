# include "psphotInternal.h"

bool psphotSkyReplace (pmConfig *config, const pmFPAview *view, const char *filerule)
{
    bool status = true;

    fprintf (stdout, "\n");
    psLogMsg ("psphot", PS_LOG_INFO, "--- psphot Replace Sky ---");

    int num = psphotFileruleCount(config, filerule);

    // skip the chisq image (optionally?)
    int chisqNum = psMetadataLookupS32 (&status, config->arguments, "PSPHOT.CHISQ.NUM");
    if (!status) chisqNum = -1;

    // loop over the available readouts
    for (int i = 0; i < num; i++) {
	if (i == chisqNum) continue; // skip chisq image
	if (!psphotSkyReplaceReadout (config, view, filerule, i)) {
            psError (PSPHOT_ERR_CONFIG, false, "failed to replace sky for %s entry %d", filerule, i);
	    return false;
	}
    }
    return true;
}

// XXX make this an option?
// in order to  successfully replace the sky, we must define a corresponding file...
bool psphotSkyReplaceReadout (pmConfig *config, const pmFPAview *view, const char *filerule, int index) {

    psTimerStart ("psphot.skyreplace");

    // find the currently selected readout
    pmFPAfile *file = pmFPAfileSelectSingle(config->files, filerule, index); // File of interest
    psAssert (file, "missing file?");

    pmReadout *readout = pmFPAviewThisReadout(view, file->fpa);
    psAssert (readout, "missing readout?");

    // select background pixels, from output background file, or create
    pmReadout *background = pmFPAfileThisReadout (config->files, view, psphotGetFilerule("PSPHOT.BACKGND"));
    if (background == NULL) psAbort("background not defined");

    // select the corresponding images
    psF32 **image = readout->image->data.F32;
    // psImageMaskType  **mask  = readout->mask->data.PS_TYPE_IMAGE_MASK_DATA;
    psF32 **back  = background->image->data.F32;

    // replace the background model
    for (int j = 0; j < readout->image->numRows; j++) {
        for (int i = 0; i < readout->image->numCols; i++) {
            if (isfinite(image[j][i]) && isfinite(back[j][i])) {
                image[j][i] += back[j][i];
            }
        }
    }
    psLogMsg ("psphot.sky", PS_LOG_DETAIL, "replace background flux : %f sec\n", psTimerMark ("psphot.skyreplace"));
    return true;
}

