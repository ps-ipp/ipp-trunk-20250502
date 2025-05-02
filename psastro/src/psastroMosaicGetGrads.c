# include "psastroInternal.h"

psArray *psastroMosaicGetGrads (pmFPA *fpa, psMetadata *recipe) {

    pmChip *chip = NULL;
    pmCell *cell = NULL;
    pmReadout *readout = NULL;
    psArray *grads = NULL;

    pmFPAview *view = pmFPAviewAlloc (0);

    // this loop selects the matched stars for all chips
    while ((chip = pmFPAviewNextChip (view, fpa, 1)) != NULL) {
        psTrace ("psastro", 4, "Chip %d: %x %x\n", view->chip, chip->file_exists, chip->process);
        if (!chip->process || !chip->file_exists) { continue; }
	
	psRegion *region = pmChipExtent (chip);

	while ((cell = pmFPAviewNextCell (view, fpa, 1)) != NULL) {
            psTrace ("psastro", 4, "Cell %d: %x %x\n", view->cell, cell->file_exists, cell->process);
            if (!cell->process || !cell->file_exists) { continue; }

	    // process each of the readouts
	    // XXX there can only be one readout per chip, right?
	    while ((readout = pmFPAviewNextReadout (view, fpa, 1)) != NULL) {
		if (! readout->data_exists) { continue; }

		// select the raw objects for this readout
		psArray *rawstars = psMetadataLookupPtr (NULL, readout->analysis, "PSASTRO.RAWSTARS.SUBSET");
		if (rawstars == NULL) { continue; }

		// select the raw objects for this readout
		psArray *refstars = psMetadataLookupPtr (NULL, readout->analysis, "PSASTRO.REFSTARS.SUBSET");
		if (refstars == NULL) { continue; }

		psArray *match = psMetadataLookupPtr (NULL, readout->analysis, "PSASTRO.MATCH");
		if (match == NULL) { continue; }

		// measure the local gradients for this set of stars
		// the new elements are added to the incoming gradient structure 
		grads = pmAstromMeasureGradients (grads, rawstars, refstars, match, region, 2, 2);
	    }
	}
	psFree (region);
    }
    psFree (view);
    return (grads);
}
