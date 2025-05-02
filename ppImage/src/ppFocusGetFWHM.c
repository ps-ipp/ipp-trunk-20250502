#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

# include "ppImage.h"

bool ppFocusGetFWHM (pmConfig *config, psVector *focus, psVector *fwhm) {

    bool status;
    pmChip *chip;
    pmCell *cell;
    pmReadout *readout;
    psMetadata *header;
    float FOCUS, FWHM, FWHM_X, FWHM_Y, FWHMsum;
    int FWHMnum;

    pmFPAfile *input = psMetadataLookupPtr (&status, config->files, "PSPHOT.OUTPUT");
    if (!status) {
	psErrorStackPrint(stderr, "Can't find input data!\n");
	exit(EXIT_FAILURE);
    }

    pmFPAview *view = pmFPAviewAlloc (0);

    // - find readouts with measured PSFs
    // - measure the average central FWHM for each PSF
    FWHMsum = 0.0;
    FWHMnum = 0;

    while ((chip = pmFPAviewNextChip (view, input->fpa, 1)) != NULL) {
        psLogMsg ("ppImageLoop", 4, "Chip %d: %x %x\n", view->chip, chip->file_exists, chip->process);
        if (!chip->process || !chip->file_exists) { continue; }

	while ((cell = pmFPAviewNextCell (view, input->fpa, 1)) != NULL) {
            psLogMsg ("ppImageLoop", 4, "Cell %d: %x %x\n", view->cell, cell->file_exists, cell->process);
            if (!cell->process || !cell->file_exists) { continue; }

	    // process each of the readouts
	    while ((readout = pmFPAviewNextReadout (view, input->fpa, 1)) != NULL) {
		if (!readout->data_exists) { continue; }

		// get average FWHM
		// psphotReadout writes the FWHM values into the PSPHOT.HEADER table
		// the source of this value depends on the psphot options.
		// - if breakPoint is set to PEAKS, the value will not be defined
		// - if breakPoint is set to MOMENTS, the PSFSTAR moments are used
		// - in all other cases, the psf model is used
		header = psMetadataLookupPtr (&status, readout->analysis, "PSPHOT.HEADER");
		if (header == NULL) {
		    psError(PS_ERR_IO, false, "Missing header in ppFocus");
		    continue;
		}

		FWHM_X = psMetadataLookupF32 (&status, header, "FWHM_X");
		FWHM_Y = psMetadataLookupF32 (&status, header, "FWHM_Y");

		FWHMsum += 0.5*(FWHM_X + FWHM_Y);
		FWHMnum ++;

		psLogMsg ("ppFocus", 4, "focus pt: %f,%f, fwhm sum: %f, fwhm num: %d\n", FWHM_X, FWHM_Y, FWHMsum, FWHMnum);
	    }
	}
    }

    FWHM = FWHMsum / FWHMnum;

    FOCUS = psMetadataLookupF32 (&status, input->fpa->concepts, "FPA.FOCUS");

    fwhm->data.F32[fwhm->n] = FWHM;
    focus->data.F32[focus->n] = FOCUS;

    psVectorExtend (fwhm, 10, 1);
    psVectorExtend (focus, 10, 1);

    psLogMsg ("ppFocus", 4, "focus: %f, fwhm: %f (%d)\n", FOCUS, FWHM, FWHMnum);

    psFree (view);
    return true;
}
