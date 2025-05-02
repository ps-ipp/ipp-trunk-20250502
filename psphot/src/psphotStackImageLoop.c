# include "psphotStandAlone.h"

# define ESCAPE(MESSAGE) {				\
	psError(PSPHOT_ERR_DATA, false, MESSAGE);	\
	psFree (view);					\
	return false;					\
    }

// XXX this implementation is not smart about multi-level astrometry headers
bool UpdateHeadersForFPA (pmConfig *config, pmFPAview *view);
bool UpdateHeadersForChip (pmConfig *config, pmFPAview *view);
bool UpdateHeadersForReadout (pmConfig *config, pmFPAview *view);

bool psphotStackImageLoop (pmConfig *config) {

    bool status;
    pmChip *chip;
    pmCell *cell;
    pmReadout *readout;

    psMemDump("startloop");

    pmFPAview *view = pmFPAviewAlloc (0);
    pmFPAfile *input = psMetadataLookupPtr (&status, config->files, "PSPHOT.STACK.INPUT.RAW");

    if (!input) {
        psError(PSPHOT_ERR_PROG, false, "Can't find input data!");
        return false;
    }

    psMetadata *recipe = psMetadataLookupPtr (NULL, config->recipes, PSPHOT_RECIPE);
    bool updateMode = psMetadataLookupBool(&status, config->arguments, "PSPHOT.STACK.UPDATEMODE");

    // just load the full set of images up front except for EXPNUM which we defer
    pmFPAfileActivate (config->files, false, "PSPHOT.STACK.EXPNUM.RAW");
    if (!pmFPAfileIOChecks (config, view, PM_FPA_BEFORE)) ESCAPE ("failed input for fpa in psphot.");

    // for psphot, we force data to be read at the chip level
    while ((chip = pmFPAviewNextChip (view, input->fpa, 1)) != NULL) {
        psLogMsg ("psphot", 4, "Chip %d: %x %x\n", view->chip, chip->file_exists, chip->process);
        if (! chip->process || ! chip->file_exists) { continue; }
        if (!pmFPAfileIOChecks (config, view, PM_FPA_BEFORE)) ESCAPE ("failed input for Chip in psphotStack.");

        // there is now only a single chip (multiple readouts?). loop over it and process
        while ((cell = pmFPAviewNextCell (view, input->fpa, 1)) != NULL) {
            psLogMsg ("psphot", 5, "Cell %d: %x %x\n", view->cell, cell->file_exists, cell->process);
	    if (!pmFPAfileIOChecks (config, view, PM_FPA_BEFORE)) ESCAPE ("failed input for Cell in psphotStack.");

            // process each of the readouts
            while ((readout = pmFPAviewNextReadout (view, input->fpa, 1)) != NULL) {
                psLogMsg ("psphot", 6, "Readout %d: %x %x\n", view->readout, cell->file_exists, cell->process);
                if (! readout->data_exists) { continue; }

		psMemDump("load");

		if (!psphotStackAllocateOutput (config, view, recipe)) {
		    psError(psErrorCodeLast(), false, "failure in psphotStackAllocateOutput for chip %d, cell %d, readout %d\n", view->chip, view->cell, view->readout);
		    psFree (view);
		    return false;
                }
		psMemDump("stackmatch");

		// XXX for now, we assume there is only a single chip in the PHU:
                if (!updateMode) {
                    if (!psphotStackReadout (config, view)) {
                        psError(psErrorCodeLast(), false, "failure in psphotStackReadout for chip %d, cell %d, readout %d\n", view->chip, view->cell, view->readout);
                        psFree (view);
                        return false;
                    }
                } else {
                    if (!psphotStackUpdateReadout (config, view)) {
                        psError(psErrorCodeLast(), false, "failure in psphotStackReadout for chip %d, cell %d, readout %d\n", view->chip, view->cell, view->readout);
                        psFree (view);
                        return false;
                    }
                }

		UpdateHeadersForReadout(config, view);

		psMemDump("psphot");
	    }
	    // drop all versions of the internal files
	    status = true;
	    status &= pmFPAfileDropInternal (config->files, psphotGetFilerule("PSPHOT.BACKMDL"));
	    status &= pmFPAfileDropInternal (config->files, psphotGetFilerule("PSPHOT.BACKMDL.STDEV"));
	    status &= pmFPAfileDropInternal (config->files, psphotGetFilerule("PSPHOT.BACKGND"));
	    if (!status) {
		psError(PSPHOT_ERR_PROG, false, "trouble dropping internal files");
		psFree (view);
		return false;
	    }
	}

	UpdateHeadersForChip(config, view);

        // Defer output until we have performed psphotSetNFrames()
        pmFPAfileActivate (config->files, false, NULL);

	// Iterate up
	if (!pmFPAfileIOChecks (config, view, PM_FPA_AFTER)) ESCAPE ("failed pmFPAfileIOChecks for Chip in psphot.");
    }
    psMemDump("doneloop");

    UpdateHeadersForFPA(config, view);

    // Iterate up output which is saved at the fpa level
    if (!pmFPAfileIOChecks (config, view, PM_FPA_AFTER)) ESCAPE ("failed ouput pmFPAfileIOChecks FPA in psphot.");

    // Load the appropriate EXPNUM image
    pmFPAfileActivate (config->files, true, "PSPHOT.STACK.EXPNUM.RAW");

    if (!pmFPAfileIOChecks (config, view, PM_FPA_BEFORE)) ESCAPE ("failed input for fpa EXPNUM in psphot.");

    // for psphot, we force data to be read at the chip level
    while ((chip = pmFPAviewNextChip (view, input->fpa, 1)) != NULL) {
        psLogMsg ("psphot", 4, "Chip %d: %x %x\n", view->chip, chip->file_exists, chip->process);
        if (! chip->process || ! chip->file_exists) { continue; }
        if (!pmFPAfileIOChecks (config, view, PM_FPA_BEFORE)) ESCAPE ("failed input for Chip EXPNUM in psphotStack.");

        // there is now only a single chip (multiple readouts?). loop over it and process
        while ((cell = pmFPAviewNextCell (view, input->fpa, 1)) != NULL) {
            psLogMsg ("psphot", 5, "Cell %d: %x %x\n", view->cell, cell->file_exists, cell->process);
	    if (!pmFPAfileIOChecks (config, view, PM_FPA_BEFORE)) ESCAPE ("failed input for Cell in psphotStack.");

            // process each of the readouts
            while ((readout = pmFPAviewNextReadout (view, input->fpa, 1)) != NULL) {
                psLogMsg ("psphot", 6, "Readout %d: %x %x\n", view->readout, cell->file_exists, cell->process);
                if (! readout->data_exists) { continue; }

                if (!psphotSetNFrames (config, view, input->name)) ESCAPE ("failed to setNFrames.");
            }
        }
        // now activate all files to trigger final output
        pmFPAfileActivate (config->files, true, NULL);

        if (!pmFPAfileIOChecks (config, view, PM_FPA_AFTER)) ESCAPE ("failed output for Chip in psphot.");
    }
    if (!pmFPAfileIOChecks (config, view, PM_FPA_AFTER)) ESCAPE ("failed ouput for FPA in psphot.");

    // fail if we failed to handle an error
    if (psErrorCodeLast() != PS_ERR_NONE) psAbort ("failed to handle an error!");

    psFree (view);

    psMemDump("doneoutput");
    return true;
}

bool UpdateHeadersForReadout (pmConfig *config, pmFPAview *view) {

    int num = psphotFileruleCount(config, "PSPHOT.INPUT");

    // loop over the available readouts
    for (int i = 0; i < num; i++) {

	// find the currently selected readout
	pmFPAfile *output = pmFPAfileSelectSingle(config->files, "PSPHOT.STACK.OUTPUT.IMAGE", i); // File of interest
	psAssert (output, "missing file?");

	pmFPAfile *input = pmFPAfileSelectSingle(config->files, "PSPHOT.STACK.INPUT.RAW", i); // File of interest
	psAssert (input, "missing input file");

	// just copy the input headers to the output headers, then update version info
	pmReadout *inReadout = pmFPAviewThisReadout(view, input->fpa); ///< Chip in the input
	pmReadout *outReadout = pmFPAviewThisReadout(view, output->fpa); ///< Chip in the output

	psMetadata *header = psMetadataLookupPtr (NULL, inReadout->analysis, "PSPHOT.HEADER");
	psMetadataAdd (outReadout->analysis, PS_LIST_TAIL, "PSPHOT.HEADER",  PS_DATA_METADATA, "header stats", header);
    }
    return true;
}

bool UpdateHeadersForChip (pmConfig *config, pmFPAview *view) {

    int num = psphotFileruleCount(config, "PSPHOT.INPUT");

    // loop over the available readouts
    for (int i = 0; i < num; i++) {

	// find the currently selected readout
	pmFPAfile *output = pmFPAfileSelectSingle(config->files, "PSPHOT.STACK.OUTPUT.IMAGE", i); // File of interest
	psAssert (output, "missing file?");

	pmFPAfile *input = pmFPAfileSelectSingle(config->files, "PSPHOT.STACK.INPUT.RAW", i); // File of interest
	psAssert (input, "missing input file");

	// just copy the input headers to the output headers, then update version info
	pmChip *inChip = pmFPAviewThisChip(view, input->fpa); ///< Chip in the input
        pmHDU *inHDU = pmFPAviewThisHDU (view, input->fpa);

	pmChip *outChip = pmFPAviewThisChip(view, output->fpa); ///< Chip in the output
	pmHDU *outHDU = pmFPAviewThisHDU (view, output->fpa);
	if (!outHDU) {
	    pmFPAAddSourceFromView(output->fpa, view, output->format);
	    outHDU = pmFPAviewThisHDU (view, output->fpa);
	    psAssert (outHDU, "failed to make HDU");
	}
	if (!outHDU->header) {
	    outHDU->header = psMetadataCopy(NULL, inHDU->header);
	}
	psphotVersionHeaderFull(outHDU->header);
	outChip->toFPA = psMemIncrRefCounter(inChip->toFPA);
	outChip->fromFPA = psMemIncrRefCounter(inChip->fromFPA);
    }
    return true;
}

bool UpdateHeadersForFPA (pmConfig *config, pmFPAview *view) {

    int num = psphotFileruleCount(config, "PSPHOT.INPUT");

    // loop over the available readouts
    for (int i = 0; i < num; i++) {

	// find the currently selected readout
	pmFPAfile *output = pmFPAfileSelectSingle(config->files, "PSPHOT.STACK.OUTPUT.IMAGE", i); // File of interest
	psAssert (output, "missing file?");

	pmFPAfile *input = pmFPAfileSelectSingle(config->files, "PSPHOT.STACK.INPUT.RAW", i); // File of interest
	psAssert (input, "missing input file");

	output->fpa->toTPA = psMemIncrRefCounter(input->fpa->toTPA);
	output->fpa->fromTPA = psMemIncrRefCounter(input->fpa->fromTPA);
	output->fpa->toSky = psMemIncrRefCounter(input->fpa->toSky);

	pmConceptsCopyFPA(output->fpa, input->fpa, true, true);

	// XXX TEST
	// pmFPASetFileStatus(output->fpa, true);
	// pmFPASetDataStatus(output->fpa, true);
	// pmChip *chip = output->fpa->chips->data[0];
	// pmCell *cell = chip->cells->data[0];
	// pmReadout *readout = cell->readouts->data[0];
    }
    return true;
}

/* 
   the easiest way to implement this is to assume we can pre-load the full set of images up front.
   with 5 filters and 6000^2 (image, mask, var = 10 byte per pixel), we need 1.8GB, which is not too bad.
*/

