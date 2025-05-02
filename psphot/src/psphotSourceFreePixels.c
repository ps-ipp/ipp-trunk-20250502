# include "psphotInternal.h"

bool psphotSourceFreePixels (pmConfig *config, const pmFPAview *view, const char *filerule)
{
    int num = psphotFileruleCount(config, filerule);

    // loop over the available readouts
    for (int i = 0; i < num; i++) {
	if (!psphotSourceFreePixelsReadout (config, view, filerule, i)) {
            psError (PSPHOT_ERR_CONFIG, false, "failed to free source pixels for %s entry %d", filerule, i);
	    return false;
	}
    }
    return true;
}

bool psphotSourceFreePixelsReadout(pmConfig *config, const pmFPAview *view, const char *filerule, int index) {

    bool status;

    // find the currently selected readout
    pmFPAfile *file = pmFPAfileSelectSingle(config->files, filerule, index); // File of interest
    psAssert (file, "missing file?");

    pmReadout *readout = pmFPAviewThisReadout(view, file->fpa);
    psAssert (readout, "missing readout?");

    pmDetections *detections = psMetadataLookupPtr (&status, readout->analysis, "PSPHOT.DETECTIONS");
    psAssert (detections, "missing detections?");

    psArray *sources = detections->allSources;
    psAssert (sources, "missing sources?");

    for (int i = 0; i < sources->n; i++) {
	pmSource *source = sources->data[i];
	pmSourceFreePixels (source);
    }
    return true;
}
