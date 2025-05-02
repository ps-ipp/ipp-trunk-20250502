#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "ppImage.h"

bool ppImageRebinChip (pmConfig *config, pmFPAview *view, ppImageOptions *options, char *outName) {

    pmCell *cell;
    pmReadout *inReadout, *outReadout;

    pmFPAfile *outFile = psMetadataLookupPtr(NULL, config->files, outName);
    if (outFile == NULL) return false;

    // psTimerStart("rebin.chip");

    // XXX double check that chip != -1?

    pmChip *inChip  = pmFPAviewThisChip (view, outFile->src);
    pmChip *outChip = pmFPAviewThisChip (view, outFile->fpa);
    if (!pmChipCopyStructure (outChip, inChip, outFile->xBin, outFile->yBin)) {
        psError(PS_ERR_UNKNOWN, false, "Unable to copy chip structure.");
        return false;
    }

    while ((cell = pmFPAviewNextCell (view, outFile->src, 1)) != NULL) {
        psLogMsg ("ppImageRebinChip", 5, "Cell %d: %x %x\n", view->cell, cell->file_exists, cell->process);
        if (! cell->process || ! cell->file_exists) { continue; }

        // process each of the readouts
        while ((inReadout = pmFPAviewNextReadout (view, outFile->src, 1)) != NULL) {
            if (! inReadout->data_exists) { continue; }

            outReadout = pmFPAviewThisReadout (view, outFile->fpa);

            // run the rebin code
	    // XXX EAM 2022.04.21 : this function rebins the signal image and makes an attempt to
	    // generate a mask only with bits raised in masked pixels that have > 50% of input pixels masked.
	    // this step is very expensive because it must count the input mask bits for each pixel.
	    // What is particularly silly is that the mask is not even used in the jpeg image.
            if (!pmReadoutRebin(outReadout, inReadout, options->maskValue, outFile->xBin, outFile->yBin)) {
                psError(PS_ERR_UNKNOWN, false, "Unable to rebin readout.");
                return false;
            }
        }
    }

    // psLogMsg ("ppImage", 5, "rebin chip for %s: %f sec\n", outName, psTimerMark ("rebin.chip"));

    return true;
}

