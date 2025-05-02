# include "psphotStandAlone.h"

# define ESCAPE(MESSAGE) {				\
	psError(PSPHOT_ERR_DATA, false, MESSAGE);	\
	psFree (view);					\
	return false;					\
    }

bool psphotFullForceImageLoop (pmConfig *config) {

    bool status;
    pmChip *chip;
    pmCell *cell;
    pmReadout *readout;

    pmFPAfile *load = psMetadataLookupPtr (&status, config->files, "PSPHOT.LOAD");
    if (!status) {
        psError(PSPHOT_ERR_PROG, false, "Can't find input data!");
        return false;
    }
    pmFPAfile *input = psMetadataLookupPtr (&status, config->files, "PSPHOT.INPUT");
    if (!status) {
        psError(PSPHOT_ERR_PROG, false, "Can't find input data!");
        return false;
    }

    pmFPAview *view = pmFPAviewAlloc (0);
    pmHDU *lastHDU = NULL;              // Last HDU updated

    // files associated with the science image
    if (!pmFPAfileIOChecks (config, view, PM_FPA_BEFORE)) ESCAPE ("failed input for fpa in psphot.");

    // select the appropriate recipe information
    psMetadata *recipe  = psMetadataLookupPtr (&status, config->recipes, PSPHOT_RECIPE);
    psAssert (recipe, "missing recipe?");

    psImageMaskType maskTest = psMetadataLookupImageMask(&status, recipe, "MASK.PSPHOT");

    // for psphot, we force data to be read at the chip level
    while ((chip = pmFPAviewNextChip (view, load->fpa, 1)) != NULL) {
        psLogMsg ("psphot", 4, "Chip %d: %x %x\n", view->chip, chip->file_exists, chip->process);
        if (! chip->process || ! chip->file_exists) { continue; }

        // load just the input image data (image, mask, weight)
        pmFPAfileActivate (config->files, false, NULL);
        pmFPAfileActivate (config->files, true, "PSPHOT.LOAD");
        pmFPAfileActivate (config->files, true, "PSPHOT.MASK");
        pmFPAfileActivate (config->files, true, "PSPHOT.VARIANCE");
        if (!pmFPAfileIOChecks (config, view, PM_FPA_BEFORE)) ESCAPE ("failed input for Chip in psphot.");

        // mosaic the cells of a chip into a single contiguous (trimmed) chip
        if (!psphotMosaicChip(config, view, "PSPHOT.INPUT", "PSPHOT.LOAD")) ESCAPE ("Unable to mosaic chip.");

        // Read WCS if easy.
        // XXX Since we're mosaicking cells, we ignore the case where the WCS is defined for a cell.
        {
            pmChip *inChip = pmFPAviewThisChip(view, input->fpa); // Mosaicked chip
            pmHDU *hduLow = pmHDUGetLowest(input->fpa, inChip, NULL);
            if (hduLow && !pmAstromReadWCS(input->fpa, inChip, hduLow->header, 1.0)) {
                psWarning("Unable to read WCS astrometry from header.");
                psErrorClear();
                pmHDU *hduHigh = pmHDUGetHighest(input->fpa, inChip, NULL);
                if (hduHigh && hduHigh != hduLow &&
                    !pmAstromReadWCS(input->fpa, chip, hduHigh->header, 1.0)) {
                    psWarning("Unable to read WCS astrometry from primary header.");
                    psErrorClear();
                }
            }
        }

        // try to load other supporting data (PSF, SRC, etc).
        // do not re-load the following three files
        pmFPAfileActivate (config->files, true, NULL);
        pmFPAfileActivate (config->files, false, "PSPHOT.LOAD");
        pmFPAfileActivate (config->files, false, "PSPHOT.MASK");
        pmFPAfileActivate (config->files, false, "PSPHOT.VARIANCE");
        if (!pmFPAfileIOChecks (config, view, PM_FPA_BEFORE)) ESCAPE ("failed input for Chip in psphot.");

        // re-activate files so they will be closed and freed below
        pmFPAfileActivate (config->files, true, NULL);

        // there is now only a single chip (multiple readouts?). loop over it and process
        while ((cell = pmFPAviewNextCell (view, input->fpa, 1)) != NULL) {
            psLogMsg ("psphot", 5, "Cell %d: %x %x\n", view->cell, cell->file_exists, cell->process);

            // process each of the readouts
            while ((readout = pmFPAviewNextReadout (view, input->fpa, 1)) != NULL) {
                psLogMsg ("psphot", 6, "Cell %d: %x %x\n", view->cell, cell->file_exists, cell->process);
                if (! readout->data_exists) { continue; }

                // Update the header
		pmHDU *hdu = pmHDUGetHighest(input->fpa, chip, cell);
		if (hdu && hdu != lastHDU) {
		    psphotVersionHeaderFull(hdu->header);
		    lastHDU = hdu;
                }

		// if an external mask is supplied, ensure that NAN pixels are also masked
		if (readout->mask) {
		    psImageMaskType maskSat = pmConfigMaskGet("SAT", config); // Mask value for saturated pixels
                    if (!pmReadoutMaskInvalid(readout, maskTest, maskSat)) {
			psError(psErrorCodeLast(), false, "Unable to mask non-finite pixels.");
			psFree(view);
			return false;
		    }
		}

                // run the actual photometry analysis on this chip/cell/readout
                if (!psphotFullForceReadout (config, view, "PSPHOT.INPUT")) {
                    psError(psErrorCodeLast(), false, "failure in psphotReadout for chip %d, cell %d, readout %d\n", view->chip, view->cell, view->readout);
                    psFree (view);
                    return false;
                }
            }

            // drop all versions of the internal files
            status = true;
            status &= pmFPAfileDropInternal (config->files, "PSPHOT.BACKMDL");
            status &= pmFPAfileDropInternal (config->files, "PSPHOT.BACKMDL.STDEV");
            status &= pmFPAfileDropInternal (config->files, "PSPHOT.BACKGND");
            if (!status) {
                psError(PSPHOT_ERR_PROG, false, "trouble dropping internal files");
                psFree (view);
                return false;
            }
        }

        // save output which is saved at the chip level
        if (!pmFPAfileIOChecks (config, view, PM_FPA_AFTER)) ESCAPE ("failed output for Chip in psphot.");
    }
    // save output which is saved at the fpa level
    if (!pmFPAfileIOChecks (config, view, PM_FPA_AFTER)) ESCAPE ("failed ouput for FPA in psphot.");

    // fail if we failed to handle an error
    if (psErrorCodeLast() != PS_ERR_NONE) psAbort ("failed to handle an error!");

    psFree (view);
    return true;
}

// I/O files related to psphot:
// PSPHOT.INPUT   : input image file(s)
// PSPHOT.RESID   : residual image
// PSPHOT.OUTPUT  : output object tables (object)

// PSPHOT.BACKSUB : background subtracted image
// PSPHOT.BACKGND : background model (full-scale image?)
// PSPHOT.BACKMDL : background model (binned image?)
// PSPHOT.PSF     : sample PSF images

// PSPHOT.MASK
// PSPHOT.VARIANCE
//
