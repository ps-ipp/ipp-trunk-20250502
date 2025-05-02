# include "psphotInternal.h"


bool psphotSourceMemory (pmConfig *config, const pmFPAview *view, const char *filerule)
{
    int num = psphotFileruleCount(config, filerule);

    // skip the chisq image (optionally?)
    // loop over the available readouts
    for (int i = 0; i < num; i++) {
        if (!psphotSourceMemoryReadout (config, view, filerule, i)) {
	    psError (PSPHOT_ERR_CONFIG, false, "failed on memory usage measurement for %s entry %d", filerule, i);
            return false;
        }
    }
    return true;
}
bool psphotSourceMemoryReadout(pmConfig *config, const pmFPAview *view, const char *filerule, int index)
{
    bool status;

    // find the currently selected readout
    pmFPAfile *file = pmFPAfileSelectSingle(config->files, filerule, index); // File of interest
    psAssert (file, "missing file?");

    pmReadout *readout = pmFPAviewThisReadout(view, file->fpa);
    psAssert (readout, "missing readout?");

    pmDetections *detections = psMetadataLookupPtr (&status, readout->analysis, "PSPHOT.DETECTIONS");
    if (!detections) {
        return true;
    }

    psArray *sources = detections->allSources;
    if (!sources) {
        return true;
    }

    if (!sources->n) {
        psLogMsg ("psphot", PS_LOG_INFO, "no sources, skipping memory measurement");
        return true;
    }

    psU64 bytes = 0;
    for (int i = 0; i < sources->n; i++) {
        pmSource *source = sources->data[i];
        bytes += pmSourceMemoryUse(source);
    }

    psLogMsg ("psphot", PS_LOG_INFO, "input %s %d: %ld sources %.1f MB.   %.1f bytes per source.\n",
        filerule, index, sources->n, (psF32)bytes/1024/1024, (psF32) bytes / sources->n);

    return true;
}
