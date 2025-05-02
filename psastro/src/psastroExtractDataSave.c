/** @file psastroModelDataSave.c
 *
 *  @brief
 *
 *  @ingroup psastroModel
 *
 *  @author IfA
 *  @version $Revision: 1.3 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-07 02:03:34 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

# include "psastroStandAlone.h"
# define NONLIN_TOL 0.001 ///< tolerance in pixels 

# define ESCAPE { \
  psError(PS_ERR_UNKNOWN, false, "Failure in psastroModelDataSave"); \
  psFree (view); \
  return false; \
}
  
bool psastroModelDataSave (pmConfig *config) {

    bool status;

    // select the current recipe
    psMetadata *recipe  = psMetadataLookupPtr (&status, config->recipes, PSASTRO_RECIPE);
    if (!recipe) {
	psError(PSASTRO_ERR_CONFIG, true, "Can't find PSASTRO recipe");
	return false;
    }

    pmFPAfile *output = psMetadataLookupPtr (&status, config->files, "PSASTRO.OUT.MODEL");
    if (!status) psAbort ("Can't find output pmFPAfile PSASTRO.OUT.MODEL");

    pmFPAview *view = pmFPAviewAlloc (0);
    pmChip *chip = NULL;

    while ((chip = pmFPAviewNextChip (view, output->fpa, 1)) != NULL) {
        psTrace ("psastro", 4, "Chip %d: %x %x\n", view->chip, chip->file_exists, chip->process);
        if (!chip->process) { continue; }
	if (!pmFPAfileIOChecks (config, view, PM_FPA_AFTER)) ESCAPE;
    }
    if (!pmFPAfileIOChecks (config, view, PM_FPA_AFTER)) ESCAPE;

    psLogMsg ("psastro", 3, "save headers : %f sec\n", psTimerMark ("psastro"));

    return true;
}



