#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "dvoApplyCorr.h"

bool dvoApplyCorrLoop (pmConfig *config, dvoApplyCorrOptions *options) {

    bool status;
    pmChip *chip;
    pmCell *cell;

    // psMetadata *recipe = psMetadataLookupMetadata(&status, config->recipes, RECIPE_NAME);

    // select the input image
    pmFPAfile *input = psMetadataLookupPtr(&status, config->files, "DVOFLAT.INPUT");
    if (!status) {
        psErrorStackPrint(stderr, "Can't find input flat-field image\n");
        exit(EXIT_FAILURE);
    }

    // select the input image
    pmFPAfile *corr = psMetadataLookupPtr(&status, config->files, "DVOFLAT.CORR");
    if (!status) {
        psErrorStackPrint(stderr, "Can't find correction image\n");
        exit(EXIT_FAILURE);
    }

    pmFPAview *view = pmFPAviewAlloc(0);// View for level of interest

    // load data at FPA level
    if (!pmFPAfileIOChecks(config, view, PM_FPA_BEFORE)) {
	psError(PS_ERR_UNKNOWN, false, "failed IO for fpa in dvoApplyCorr\n");
	psFree(view);
        return false;
    }

    // process each chip in the FPA
    while ((chip = pmFPAviewNextChip(view, input->fpa, 1)) != NULL) {
        psLogMsg ("dvoApplyCorrLoop", 4, "Chip %d: %x %x\n", view->chip, chip->file_exists, chip->process);
        if (!chip->process || !chip->file_exists) continue;
	if (!pmFPAfileIOChecks (config, view, PM_FPA_BEFORE)) {
            psError(PS_ERR_UNKNOWN, false, "failed IO for chip %d in dvoApplyCorr\n", view->chip);
	    psFree (view);
	    return false;
	}

	pmChip *corrChip = pmFPAviewThisChip(view, corr->fpa);

	// process each cell in the Chip
        while ((cell = pmFPAviewNextCell(view, input->fpa, 1)) != NULL) {
            psLogMsg ("dvoApplyCorrLoop", 4, "Cell %d: %x %x\n", view->cell, cell->file_exists, cell->process);
            if (!cell->process || !cell->file_exists) continue;
            if (!pmFPAfileIOChecks(config, view, PM_FPA_BEFORE)) {
		psError(PS_ERR_UNKNOWN, false, "failed IO for chip %d, cell %d in dvoApplyCorr\n", view->chip, view->cell);
		psFree (view);
		return false;
	    }

	    // multiply the input image and the correction image
	    if (!dvoApplyCorrReadout (cell, corrChip)) {
		psError(PS_ERR_UNKNOWN, false, "failed to apply correction to chip %d, cell %d dvoApplyCorr\n", view->chip, view->cell);
		psFree (view);
		return false;
	    }
	    if (!pmFPAfileIOChecks (config, view, PM_FPA_AFTER)) {
		psError(PS_ERR_UNKNOWN, false, "failed IO for chip %d, cell %d in dvoApplyCorr\n", view->chip, view->cell);
		psFree (view);
		return false;
	    }
	}
	if (!pmFPAfileIOChecks (config, view, PM_FPA_AFTER)) {
	    psError(PS_ERR_UNKNOWN, false, "failed IO for chip %d in dvoApplyCorr\n", view->chip);
	    psFree (view);
	    return false;
	}
    }
    if (!pmFPAfileIOChecks (config, view, PM_FPA_AFTER)) {
	psError(PS_ERR_UNKNOWN, false, "failed IO for fpa in dvoApplyCorr\n");
	psFree (view);
	return false;
    }

    psFree (view);

    // fail if we failed to handle an error
    if (psErrorCodeLast() != PS_ERR_NONE) return false;

    return true;
}
