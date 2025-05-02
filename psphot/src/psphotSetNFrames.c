# include "psphotInternal.h"

# define ESCAPE(MESSAGE) {				\
	psError(PSPHOT_ERR_DATA, false, MESSAGE);	\
	psFree (view);					\
	return false;					\
    }


// Set source->nFrames based on the values in the EXPNUM image at the coordinates of each source's peak

bool psphotSetNFrames (pmConfig *config, const pmFPAview *view, const char *filerule)
{
    psLogMsg ("psphot", PS_LOG_INFO, "--- psphot Set N Frames ---");

    int num = psphotFileruleCount(config, filerule);

    // loop over the inputs
    for (int i = 0; i < num; i++) {
        if (!psphotSetNFramesReadout (config, view, filerule, i)) {
            psError (PSPHOT_ERR_CONFIG, false, "failed to set nFrames for %s entry %d", filerule, i);
            return false;
        }
    }

    return true;
}

bool psphotSetNFramesReadout (pmConfig *config, const pmFPAview *view, const char *filerule, int index)
{
    bool status;
    // find the currently selected readout
    pmFPAfile *file = pmFPAfileSelectSingle(config->files, filerule, index); // File of interest
    psAssert (file, "missing file?");

    pmReadout *readout = pmFPAviewThisReadout(view, file->fpa);
    psAssert (readout, "missing readout?");

    psImage *expnum = psMetadataLookupPtr(&status, readout->analysis, "EXPNUM");
    if (!status || !expnum) { 
        psLogMsg ("psphot", PS_LOG_INFO, "No EXPNUM image for input %d", index);
        return true;
    }
    psLogMsg ("psphot", PS_LOG_INFO, "Found EXPNUM image for input %d", index);

    pmDetections *detections = psMetadataLookupPtr (&status, readout->analysis, "PSPHOT.DETECTIONS");
    if (!detections) {
        psLogMsg ("psphot", PS_LOG_INFO, "No detections image input %d", index);
        return true;
    }

    psArray *sources = detections->allSources;
    if (!sources) {
        psLogMsg ("psphot", PS_LOG_INFO, "No sources for input %d", index);
        return true;
    }

    int col0 = expnum->col0;
    int row0 = expnum->row0;
    int numCols = expnum->numCols;
    int numRows = expnum->numRows;
    
    for (int i = 0; i < sources->n; i++) {
        pmSource *source = sources->data[i];
        int x = source->peak->x;
        int y = source->peak->y;
        if (x >= col0 && x < numCols && y >= row0 && y < numRows) {
            source->nFrames = expnum->data.PS_TYPE_IMAGE_MASK_DATA[y][x];
        } else {
            source->nFrames = 0;
        }
    }

    return true;
}
