#include "ppNoiseMap.h"

bool ppNoiseMapReadout (pmConfig *config, pmFPAview *view) {

    pmFPAfile *outFile = psMetadataLookupPtr(NULL, config->files, "PPNOISEMAP.OUTPUT");
    if (outFile == NULL) return false;

    pmChip *inChip  = pmFPAviewThisChip (view, outFile->src);
    pmChip *outChip = pmFPAviewThisChip (view, outFile->fpa);
    if (!pmChipCopyStructure (outChip, inChip, outFile->xBin, outFile->yBin)) {
        psError(PS_ERR_UNKNOWN, false, "Unable to copy chip structure.");
        return false;
    }

    pmCell *cell;
    pmReadout *inReadout, *outReadout;

    while ((cell = pmFPAviewNextCell (view, outFile->src, 1)) != NULL) {
        psLogMsg ("ppNoiseMapReadout", 5, "rebin: Cell %d: %x %x\n", view->cell, cell->file_exists, cell->process);
        if (! cell->process || ! cell->file_exists) { continue; }

        // process each of the readouts
        while ((inReadout = pmFPAviewNextReadout (view, outFile->src, 1)) != NULL) {
            if (! inReadout->data_exists) { continue; }

            outReadout = pmFPAviewThisReadout (view, outFile->fpa);

            // run the rebin code
            if (!ppNoiseMapStats(outReadout, inReadout, 0, outFile->xBin, outFile->yBin)) {
                psError(PS_ERR_UNKNOWN, false, "Unable to rebin readout.");
                return false;
            }
        }
    }

    return true;
}
