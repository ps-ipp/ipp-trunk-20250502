#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

# include "ppImage.h"

// In this function, we perform the psphot analysis routine for the chip-mosaicked images
bool ppImagePhotom(psMetadata *stats, pmConfig *config, pmFPAview *view) {

    bool status;
    pmCell *cell;
    pmReadout *readout;

    psphotInit();

    // find or define a pmFPAfile PSPHOT.INPUT
    pmFPAfile *input = psMetadataLookupPtr (&status, config->files, "PSPHOT.INPUT");
    if (!status) {
        psError(PSPHOT_ERR_CONFIG, false, "PSPHOT.INPUT I/O file is not defined");
        return false;
    }

    // we make a new copy of the output chip to keep psphot from modifying the output image
    pmChip *oldChip = pmFPAviewThisChip (view, input->src);
    pmChip *newChip = pmFPAviewThisChip (view, input->fpa);
    pmChipCopy (newChip, oldChip);

    // iterate over the cells and readout for this chip
    while ((cell = pmFPAviewNextCell (view, input->fpa, 1)) != NULL) {
        psLogMsg ("ppImagePhotom", 5, "Cell %d: %x %x\n", view->cell, cell->file_exists, cell->process);
        if (! cell->process || ! cell->file_exists) { continue; }

        // process each of the readouts
        while ((readout = pmFPAviewNextReadout (view, input->fpa, 1)) != NULL) {
            if (! readout->data_exists) { continue; }

            // run the actual photometry analysis
            if (!psphotReadout (config, view, "PSPHOT.INPUT")) {
                // This is likely a data quality issue
                // XXX Split into multiple cases using error codes?
                psErrorStackPrint(stderr, "Unable to perform photometry on image");
                psWarning("Unable to perform photometry on image --- suspect bad data quality.");
                if (stats && psMetadataLookupS32(NULL, stats, "QUALITY") == 0) {
                    psMetadataAddS32(stats, PS_LIST_TAIL, "QUALITY", PS_META_REPLACE,
                                     "Unable to perform photometry on image", psErrorCodeLast());
                }
                psErrorClear();
		//                psphotFilesActivate(config, false);
            }

            // we want to save the MASK as modified by psphot, but not the data or weight
            // free the old mask and replace with a memory copy of the new mask
            pmReadout *oldReadout = pmFPAviewThisReadout(view, input->src);
            pmReadout *newReadout = pmFPAviewThisReadout(view, input->fpa);
            psFree (oldReadout->mask);
            oldReadout->mask = psMemIncrRefCounter(newReadout->mask);
        }
    }

    ppImageMemoryDump("photom");

    return true;
}
