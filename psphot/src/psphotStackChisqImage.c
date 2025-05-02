# include "psphotInternal.h"

// Chi = sqrt(\sum_i im_i^2/var_i)

static int npass = 0;

// XXX supply filename or keep PSPHOT.INPUT fixed?
bool psphotStackChisqImage (pmConfig *config, const pmFPAview *view, const char *ruleDet, const char *ruleSrc)
{
    psTimerStart ("psphot.chisq.image");

    pmFPAfile *chisqFile = pmFPAfileSelectSingle(config->files, "PSPHOT.CHISQ.IMAGE", 0);
    psAssert (chisqFile, "missing chisq image FPA?");

    // the readout containing the chisq image is generated in the first pass of this loop and
    // used by the successive passes
    pmReadout *chiReadout = NULL;

    int num = psphotFileruleCount(config, "PSPHOT.INPUT");

    // loop over the available readouts
    // generate the chisq image from the 'detection' images
    for (int i = 0; i < num; i++) {
        if (!psphotStackChisqImageAddReadout(config, view, ruleDet, &chiReadout, i)) {
            psError (PSPHOT_ERR_CONFIG, false, "failed to model background for %s entry %d", ruleDet, i);
            return false;
        }
    }

    psMetadataAddS32(config->arguments, PS_LIST_TAIL, "PSPHOT.CHISQ.NUM", PS_META_REPLACE, "", num);

    // we need to increment the counter for ruleDet and ruleSrc:
    num++;

    // save the resulting image in the 'detection' set
    if (!psMetadataAddPtr(config->files, PS_LIST_TAIL, ruleDet, PS_DATA_UNKNOWN | PS_META_DUPLICATE_OK, "", chisqFile)) {
        psError(PM_ERR_CONFIG, false, "could not add chisqFPA to config files");
        return false;
    }
    psphotFileruleCountSet(config, ruleDet, num);

    // also save the resulting image in the 'source' set (analysis set)
    if (strcmp(ruleDet, ruleSrc)) {
	if (!psMetadataAddPtr(config->files, PS_LIST_TAIL, ruleSrc, PS_DATA_UNKNOWN | PS_META_DUPLICATE_OK, "", chisqFile)) {
	    psError(PM_ERR_CONFIG, false, "could not add chisqFPA to config files");
	    return false;
	}
	psphotFileruleCountSet(config, ruleSrc, num);
    }

    psLogMsg ("psphot", PS_LOG_INFO, "built chisq image: %f sec\n", psTimerMark ("psphot.chisq.image"));

    return true;
}

bool psphotStackChisqImageAddReadout(const pmConfig *config, // Configuration
				     const pmFPAview *view,
				     const char *filerule, 
				     pmReadout **chiReadout,
				     int index) 
{
    bool status = true;

    // find the currently selected readout
    pmFPAfile *input = pmFPAfileSelectSingle(config->files, filerule, index); // File of interest
    psAssert (input, "missing file?");

    pmReadout *inReadout = pmFPAviewThisReadout(view, input->fpa);
    psAssert (inReadout, "missing readout?");

    psImage *inImage     = inReadout->image;
    psAssert (inImage, "missing image?");

    psImage *inVariance  = inReadout->variance;
    psAssert (inVariance, "missing variance?");

    psImage *inMask      = inReadout->mask;
    psAssert (inMask, "missing mask?");

    if (*chiReadout == NULL) {
	*chiReadout = pmFPAGenerateReadout(config, view, "PSPHOT.CHISQ.IMAGE", input->fpa, NULL, 0);
    }

    psImage *chiImage = (*chiReadout)->image;
    psAssert (chiImage, "missing chi image");

    psImage *chiVariance = (*chiReadout)->variance;
    psAssert (chiVariance, "missing chi variance");

    psImage *chiMask = (*chiReadout)->mask;
    psAssert (chiMask, "missing chi mask");

    // select the appropriate recipe information
    psMetadata *recipe  = psMetadataLookupPtr (&status, config->recipes, PSPHOT_RECIPE);
    assert (recipe);

    // user-defined masks to test for good/bad pixels (build from recipe list if not yet set)
    psImageMaskType maskVal = psMetadataLookupImageMask(&status, recipe, "MASK.PSPHOT"); // Mask value for bad pixels
    assert (maskVal);

    // generate the chisq image:
    for (int iy = 0; iy < inImage->numRows; iy++) {
        for (int ix = 0; ix < inImage->numCols; ix++) {
	    if (inMask->data.PS_TYPE_IMAGE_MASK_DATA[iy][ix] & maskVal) continue;
	    // XXX TEST chiImage->data.F32[iy][ix] += PS_SQR(inImage->data.F32[iy][ix]) / inVariance->data.F32[iy][ix];
	    chiImage->data.F32[iy][ix] = 0.0;
	    chiVariance->data.F32[iy][ix] = 1.0; // ?? what is the right value?  just init to this?
	    // XXX TEST chiMask->data.PS_TYPE_IMAGE_MASK_DATA[iy][ix] = 0x00; // we have valid data so unmask this pixel
	    chiMask->data.PS_TYPE_IMAGE_MASK_DATA[iy][ix] = 0x01; // we have valid data so unmask this pixel
        }
    }

    if (psTraceGetLevel("psphot") > 5) {
        char name[256];
        sprintf (name, "chiImage.%02d.fits", npass);
        psphotSaveImage (NULL, chiImage, name);
    }
    npass ++;

    return true;
}

bool psphotStackRemoveChisqFromInputs (pmConfig *config, const char *filerule) {

    bool status = false;

    int chisqNum = psMetadataLookupS32 (&status, config->arguments, "PSPHOT.CHISQ.NUM");
    psAssert (status, "programming error: must define PSPHOT.CHISQ.NUM");

    int inputNum = psphotFileruleCount(config, filerule);

    pmFPAfileRemoveSingle (config->files, filerule, chisqNum);

    inputNum --;
    psphotFileruleCountSet(config, filerule, inputNum);

    return true;
}

bool pmFPAfileRemoveSingle(psMetadata *files, const char *name, int num)
{
    PS_ASSERT_PTR_NON_NULL(files, NULL);
    PS_ASSERT_INT_NONNEGATIVE(num, NULL);

    psList* mdList = files->list;
    psHash* mdHash = files->hash;

    // Generate a REGEX to select only items that match 'name'
    psString regex = NULL;              // Regular expression
    if (name) {
        if (!psMetadataLookup(files, name)) {
            // No files match the requested name
            return false;
        }
        psStringAppend(&regex, "^%s$", name);
    }

    psMetadataIterator *iter = psMetadataIteratorAlloc(files, PS_LIST_HEAD, regex); // Iterator
    psFree(regex);
    psMetadataItem *item;               // Item from iteration

    bool found = false;
    int i = 0;                          // Counter
    for (i = 0; !found && (item = psMetadataGetAndIncrement(iter)); i++) {
        if (i == num) found = true;
    }
    psFree(iter);
    if (!found) {
	return false;
    }

    char *key = item->name;

    // look up the name via hash to see if we have a multi or not
    psMetadataItem* hashItem = psHashLookup(mdHash, name);
    if (hashItem == NULL) {
        psError(PS_ERR_UNKNOWN, false, _("Failed to remove metadata item, %s, from metadata table."), key);
        return false;
    }

    if (hashItem->type == PS_DATA_METADATA_MULTI) {
        // multiple entries with same key, remove just the specified one
        psListRemoveData(hashItem->data.list, item);
    } else {
        psHashRemove(mdHash, key);
    }
    psListRemoveData(mdList, item);

    return true;
}
