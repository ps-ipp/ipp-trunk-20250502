# include "fpcamera.h"

# define ESCAPE(ERROR, MSG) { psErrorStackPrint(stderr, MSG); psFree (view); return false; }

/* \brief this function loops over chips and performs forced photometry for the references */
bool fpcameraAnalysis (pmConfig *config, psMetadata *stats) {

    bool status;
    pmChip *chip;
    pmCell *cell;
    pmReadout *readout;
    pmFPAview *view = NULL;

    // measure the total elapsed time in fpcameraAnalysis.
    psTimerStart ("fpcameraAnalysis");

    // select the current recipe
    psMetadata *recipe  = psMetadataLookupPtr (&status, config->recipes, FPCAMERA_RECIPE);
    if (!recipe) ESCAPE (FPCAMERA_ERR_CONFIG, "Can't find FPCAMERA recipe");

    // loop over the input images
    pmFPAfile *input = psMetadataLookupPtr (&status, config->files, "FPCAMERA.INPUT");
    if (!input) ESCAPE(FPCAMERA_ERR_CONFIG, "Can't find or interpret output file rule FPCAMERA.INPUT!");

    // astrometry reference (smf)
    pmFPAfile *astrom = psMetadataLookupPtr (&status, config->files, "FPCAMERA.INPUT.ASTROM");
    if (!astrom) ESCAPE(FPCAMERA_ERR_CONFIG, "Can't find or interpret output file rule FPCAMERA.INPUT.ASTROM!");

    // only activate input image-type files
    pmFPAfileActivate (config->files, false, NULL);
    pmFPAfileActivate (config->files, true, "FPCAMERA.INPUT");
    pmFPAfileActivate (config->files, true, "FPCAMERA.INPUT.MASK");
    pmFPAfileActivate (config->files, true, "FPCAMERA.INPUT.VARIANCE");
    pmFPAfileActivate (config->files, true, "FPCAMERA.RESID");
    pmFPAfileActivate (config->files, true, "PSPHOT.PSF.LOAD"); // if this file is defined, we need to activate it now
    // Note: the output file FPCAMERA.RESID is tied to FPCAMERA.INPUT so they must be active in the same block.

    view = pmFPAviewAlloc (0);

    // load images at FPA level (if appropriate)
    if (!pmFPAfileIOChecks (config, view, PM_FPA_BEFORE)) ESCAPE(FPCAMERA_ERR_DATA, "failed to load images at FPA level");

    // count the number of sucesses and raise bad quality if none succeed
    int nChipGood = 0;
    int nChipTotal = 0;

    // load images at chip level (if appropriate) 
    while ((chip = pmFPAviewNextChip (view, input->fpa, 1)) != NULL) {
	nChipTotal ++;
        psTrace ("fpcamera", 4, "Chip %d: %x %x\n", view->chip, chip->file_exists, chip->process);
        if (!chip->process || !chip->file_exists) { continue; }
	if (!pmFPAfileIOChecks (config, view, PM_FPA_BEFORE)) ESCAPE(FPCAMERA_ERR_DATA, "failed to load images at Chip level");

	// loop over all cells in chip
        while ((cell = pmFPAviewNextCell (view, input->fpa, 1)) != NULL) {
            psLogMsg ("fpcamera", 5, "Cell %d: %x %x\n", view->cell, cell->file_exists, cell->process);
            if (!cell->process || !cell->file_exists) { continue; }

	    // loop over all readouts in cell
            while ((readout = pmFPAviewNextReadout (view, input->fpa, 1)) != NULL) {
                psLogMsg ("fpcamera", 6, "Cell %d: %x %x\n", view->cell, cell->file_exists, cell->process);
                if (!readout->data_exists) { continue; }

		fpcameraChooseRefstars (input, astrom, view);

		// Provide a simple (wrong) PSF as a default if psf model is not supplied with -psf or -psflist.
		// The user-supplied psf model will replace this one on a chip-by-chip basis
		// pmPSF *psf = pmPSFBuildSimple ("PS_MODEL_PS1_V1", 5.0, 5.0, 0.0, 0.5);
		pmPSF *psf = pmPSFBuildSimple ("PS_MODEL_GAUSS", 5.0, 5.0, 0.0);
		psf->fieldNx = readout->image->numCols;
		psf->fieldNy = readout->image->numRows;
		psMetadataAddPtr (readout->analysis, PS_LIST_TAIL, "PSPHOT.PSF", PS_META_REPLACE | PS_DATA_UNKNOWN, "psphot PSF model", psf);
		psFree (psf);

		if (!psphotForcedReadout (config, view, "FPCAMERA.INPUT")) {
		    // This is likely a data quality issue, e.g. no stars on a chip
		    // Do not raise an error, but do not add to count of good chips
		    psErrorStackPrint(stderr, "Unable to perform photometry on image");
		    psWarning("Unable to perform photometry on image --- suspect bad data quality.");
		    psErrorClear();
		    continue;
		}
		fpcameraAstrometryChipHeader (config, view, readout, astrom);
		nChipGood ++;
	    }
            // drop all versions of the internal files
            status = true;
            status &= pmFPAfileDropInternal (config->files, "PSPHOT.BACKMDL");
            status &= pmFPAfileDropInternal (config->files, "PSPHOT.BACKMDL.STDEV");
            status &= pmFPAfileDropInternal (config->files, "PSPHOT.BACKGND");
            if (!status) ESCAPE(FPCAMERA_ERR_PROG, "trouble dropping internal files");
	}
        if (!pmFPAfileIOChecks (config, view, PM_FPA_AFTER)) ESCAPE(FPCAMERA_ERR_IO, "failure in IOChecks(AFTER) at Chip");
    }
    fpcameraAstrometryFPAHeader (input->fpa, astrom, stats);
    if (!pmFPAfileIOChecks (config, view, PM_FPA_AFTER)) ESCAPE(FPCAMERA_ERR_IO, "failure in IOChecks(AFTER) at FPA");

    if (!nChipGood) {
	if (stats && psMetadataLookupS32(NULL, stats, "QUALITY") == 0) {
	    psMetadataAddS32(stats, PS_LIST_TAIL, "QUALITY", PS_META_REPLACE, "Unable to perform photometry on image", FPCAMERA_ERR_DATA);
	}
    }
    psLogMsg("fpcamera", 3, "fpcameraAnalysis: %d of %d chips are good\n", nChipGood, nChipTotal);

    psFree (view);

    psLogMsg("fpcamera", 3, "TIMEMARK: fpcameraAnalysis: %f sec\n", psTimerMark ("analysis"));
    return true;
}

// NOTES
// chip->process is set (unset) based on command-line -chip selections (in fpcameraArguments)
// chip->file_exists is set (in pmFPAFlags.c:pmChipSetFileStatus) by pmFPAfileDefineFromArgs (in pmFPAAddSource...)

/*
        // XXX set sxx, etc from FWHM in recipe
        pmPSF *psf = pmPSFBuildSimple (modelNames->data[0], 1.0, 1.0, 0.0, 1.0);
        psf->fieldNx = readout->image->numCols;
        psf->fieldNy = readout->image->numRows;
        psFree (modelNames);

*/
