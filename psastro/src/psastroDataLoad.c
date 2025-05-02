/** @file psastroDataLoad.c
 *
 *  @brief
 *
 *  @ingroup psastro
 *
 *  @author IfA
 *  @version $Revision: 1.12 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-07 02:03:34 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

# include "psastroStandAlone.h"

# define ESCAPE { \
  psError(PS_ERR_UNKNOWN, false, "Failure in psastroDataLoad"); \
  psFree (view); \
  return false; \
}
  
/**
 * \brief this loop loads the data from the input files and selects the
 * brighter stars for astrometry
 *
 * at the end of this function, the complete stellar data is loaded
 * into the correct fpa structure locations (readout.analysis:PSPHOT.SOURCES)
 * all of the different astrometry analysis modes use the same data load loop
 */
bool psastroDataLoad (pmConfig *config) {

    pmChip *chip;
    pmCell *cell;
    pmReadout *readout;

    psTimerStart ("psastro");

    // select the current recipe
    psMetadata *recipe  = psMetadataLookupPtr (NULL, config->recipes, PSASTRO_RECIPE);
    if (!recipe) {
	psError(PSASTRO_ERR_CONFIG, true, "Can't find PSASTRO recipe!\n");
	return false;
    }

    // select the input data sources
    pmFPAfile *input = psMetadataLookupPtr (NULL, config->files, "PSASTRO.INPUT");
    if (!input) {
	psError(PSASTRO_ERR_CONFIG, true, "Can't find input data!\n");
	return false;
    }

    // de-activate all files except PSASTRO.INPUT
    pmFPAfileActivate (config->files, false, NULL);
    pmFPAfileActivate (config->files, true, "PSASTRO.INPUT");

    // if we request KH Correction, activate that file as well
    bool applyKH = psMetadataLookupBool (NULL, recipe, "KH.CORRECT.APPLY.EXP");
    if (applyKH) {
	pmFPAfileActivate (config->files, true, "PSASTRO.KH.CORRECT");
    }

    pmFPAview *view = pmFPAviewAlloc (0);

    // files associated with the science image
    if (!pmFPAfileIOChecks (config, view, PM_FPA_BEFORE)) ESCAPE;

    while ((chip = pmFPAviewNextChip (view, input->fpa, 1)) != NULL) {
        psTrace ("psastro", 4, "Chip %d: %x %x\n", view->chip, chip->file_exists, chip->process);
        if (!chip->process || !chip->file_exists) { continue; }
	if (!pmFPAfileIOChecks (config, view, PM_FPA_BEFORE)) ESCAPE;

	while ((cell = pmFPAviewNextCell (view, input->fpa, 1)) != NULL) {
            psTrace ("psastro", 4, "Cell %d: %x %x\n", view->cell, cell->file_exists, cell->process);
            if (!cell->process || !cell->file_exists) { continue; }
	    if (!pmFPAfileIOChecks (config, view, PM_FPA_BEFORE)) ESCAPE;

	    // process each of the readouts
	    while ((readout = pmFPAviewNextReadout (view, input->fpa, 1)) != NULL) {
		if (!pmFPAfileIOChecks (config, view, PM_FPA_BEFORE)) ESCAPE;
		if (!readout->data_exists) { continue; }

		if (!psastroConvertReadout (config, view, readout, recipe)) ESCAPE;

		if (!pmFPAfileIOChecks (config, view, PM_FPA_AFTER)) ESCAPE;
	    }
	    if (!pmFPAfileIOChecks (config, view, PM_FPA_AFTER)) ESCAPE;
	}
	if (!pmFPAfileIOChecks (config, view, PM_FPA_AFTER)) ESCAPE;
    }
    if (!pmFPAfileIOChecks (config, view, PM_FPA_AFTER)) ESCAPE;

    psLogMsg ("psastro", 3, "load data : %f sec\n", psTimerMark ("psastro"));

    psFree (view);
    return true;
}

