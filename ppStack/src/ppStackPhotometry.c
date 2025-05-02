#include "ppStack.h"

bool ppStackPhotometry(ppStackOptions *options, pmConfig *config)
{
    psAssert(options, "Require options");
    psAssert(config, "Require configuration");

    psMetadata *recipe = psMetadataLookupMetadata(NULL, config->recipes, PPSTACK_RECIPE); // ppStack recipe
    psAssert(recipe, "We've thrown an error on this before.");

    if (!options->photometry) {
        // Nothing to do
        return true;
    }

    psTimerStart("PPSTACK_PHOT");

    pmFPAfileActivate(config->files, false, NULL);
    ppStackFileActivation(config, PPSTACK_FILES_PHOT, true);
    pmFPAview *photView = ppStackFilesIterateDown(config); // View to readout

    ppStackFileList stackFiles = options->convolve ? PPSTACK_FILES_STACK : PPSTACK_FILES_UNCONV;
    ppStackFileActivation(config, stackFiles, true);

    pmFPAfile *photFile = psMetadataLookupPtr(NULL, config->files, "PSPHOT.INPUT"); // File for photometry
    pmFPACopy(photFile->fpa, options->outRO->parent->parent->parent);

    if (psMetadataLookupBool(NULL, config->arguments, "-visual")) {
        pmVisualSetVisual(true);
    }

    psMetadata *psphot = psMetadataLookupMetadata(NULL, config->recipes, PSPHOT_RECIPE); // Recipe

#if 0
    // Need to ensure aperture residual is not calculated
    psMetadataItem *item = psMetadataLookup(recipe, "MEASURE.APTREND"); // Item determining aptrend
    if (!item) {
        psWarning("Unable to find MEASURE.APTREND in psphot recipe");
        psErrorClear();
    } else {
        item->data.B = false;
    }
#endif

    // set maskValue and markValue in the psphot recipe
    psImageMaskType maskValue = pmConfigMaskGet("BLANK", config); // Bits to mask
    psImageMaskType markValue = pmConfigMaskGet("MARK.VALUE", config); // Bits to use for marking
    psMetadataAddImageMask(psphot, PS_LIST_TAIL, "MASK.PSPHOT", PS_META_REPLACE, "Bits to mask", maskValue);
    psMetadataAddImageMask(psphot, PS_LIST_TAIL, "MARK.PSPHOT", PS_META_REPLACE, "Bits to use for mark", markValue);

    psArray *inSources = options->sources;
    if (!inSources) {
        psError(PPSTACK_ERR_PROG, false, "Unable to find input sources");
        psFree(photView);
        return false;
    }

    // Add the expnum image to the photometry readout's analysis metadata so that it is available to
    // psphotSetNFrames
    pmReadout *photRO = pmFPAviewThisReadout(photView, photFile->fpa);
    if (photRO && options->expRO && options->expRO->mask) {
        psMetadataAddImage(photRO->analysis, PS_LIST_TAIL, "EXPNUM", PS_META_REPLACE, "EXPNUM image", options->expRO->mask);
    }

    pmModelClassSetLimits(PM_MODEL_LIMITS_LAX);
    if (!psphotReadoutKnownSources(config, photView, "PSPHOT.INPUT", inSources)) {
        // This is likely a data quality issue
        // XXX Split into multiple cases using error codes?
        psErrorStackPrint(stderr, "Unable to perform photometry on image");
        psWarning("Unable to perform photometry on image --- suspect bad data quality.");
        if (options->quality == 0) {
            options->quality = psErrorCodeLast();
        }
        psErrorClear();
        psphotFilesActivate(config, false);
    }

    if (!pmFPAfileDropInternal(config->files, "PSPHOT.BACKMDL") ||
        !pmFPAfileDropInternal (config->files, "PSPHOT.BACKMDL.STDEV") ||
        !pmFPAfileDropInternal (config->files, "PSPHOT.BACKGND")) {
        psError(PPSTACK_ERR_PROG, false, "Unable to drop PSPHOT internal files.");
        return false;
    }

    pmFPAfileActivate(config->files, false, "PSPHOT.INPUT");

    if (options->stats) {
        pmReadout *photRO = pmFPAviewThisReadout(photView, photFile->fpa); // Readout with the sources
        pmDetections *detections = psMetadataLookupPtr(NULL, photRO->analysis, "PSPHOT.DETECTIONS"); // detections
        if (detections && detections->allSources) {
            psMetadataAddS32(options->stats, PS_LIST_TAIL, "NUM_SOURCES", 0, "Number of sources detected", detections->allSources->n);
        } else {
            psMetadataAddS32(options->stats, PS_LIST_TAIL, "NUM_SOURCES", 0, "Number of sources detected", 0);
        }
        psMetadataAddF32(options->stats, PS_LIST_TAIL, "TIME_PHOT", PS_META_REPLACE, "Time to do photometry", psTimerMark("PPSTACK_PHOT"));
    }

    psFree(photView);

    psLogMsg("ppStack", PS_LOG_INFO, "Time to do photometry: %f sec", psTimerClear("PPSTACK_PHOT"));

    return true;
}
