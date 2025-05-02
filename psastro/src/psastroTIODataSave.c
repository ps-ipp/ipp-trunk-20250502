/** @file psastroTIODataSave.c
 *
 *  @brief
 *
 *  @ingroup psastro
 *
 *  @author IfA
 *  @version $Revision: 1.15 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-07 02:03:34 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

# include "psastroInternal.h"

# define ESCAPE { \
  psError(PS_ERR_UNKNOWN, false, "Failure in psastroTIODataSave"); \
  psFree (view); \
  return false; \
}

void pmSourceIO_ShowTiming(void);

bool psastroTIODataSave (pmConfig *config) {

    pmChip *chip;
    pmCell *cell;
    pmReadout *readout;

    psTimerStart ("psastroDataSave");

    // select the current recipe
    psMetadata *recipe  = psMetadataLookupPtr (NULL, config->recipes, PSASTRO_RECIPE);
    if (!recipe) {
        psError(PSASTRO_ERR_CONFIG, true, "Can't find PSASTRO recipe!\n");
        return false;
    }

    // select the output data sources
    pmFPAfile *output = psMetadataLookupPtr (NULL, config->files, "PSASTRO.OUTPUT");
    if (!output) {
        psError(PSASTRO_ERR_CONFIG, true, "Can't find or interpret output file rule PSASTRO.OUTPUT!\n");
        return false;
    }

    // de-activate all files except PSASTRO.OUTPUT, PSASTRO.OUT.ASTROM, and PSPHOT.OUTPUT.CFF
    pmFPAfileActivate (config->files, false, NULL);
    pmFPAfileActivate (config->files, true, "PSASTRO.OUTPUT");

    pmFPAview *view = pmFPAviewAlloc (0);

    // open/load files as needed
    if (!pmFPAfileIOChecks (config, view, PM_FPA_BEFORE)) ESCAPE;

    while ((chip = pmFPAviewNextChip (view, output->fpa, 1)) != NULL) {
        psTrace ("psastro", 4, "Chip %d: %x %x\n", view->chip, chip->file_exists, chip->process);
        if (!chip->process || !chip->file_exists) { continue; }
        if (!pmFPAfileIOChecks (config, view, PM_FPA_BEFORE)) ESCAPE;

        while ((cell = pmFPAviewNextCell (view, output->fpa, 1)) != NULL) {
            psTrace ("psastro", 4, "Cell %d: %x %x\n", view->cell, cell->file_exists, cell->process);
            if (!cell->process || !cell->file_exists) { continue; }
            if (!pmFPAfileIOChecks (config, view, PM_FPA_BEFORE)) ESCAPE;

            // process each of the readouts
            while ((readout = pmFPAviewNextReadout (view, output->fpa, 1)) != NULL) {
                if (!pmFPAfileIOChecks (config, view, PM_FPA_BEFORE)) ESCAPE;
                if (!readout->data_exists) { continue; }

                if (!pmFPAfileIOChecks (config, view, PM_FPA_AFTER)) ESCAPE;
            }
            if (!pmFPAfileIOChecks (config, view, PM_FPA_AFTER)) ESCAPE;
        }
        if (!pmFPAfileIOChecks (config, view, PM_FPA_AFTER)) ESCAPE;
    }
    if (!pmFPAfileIOChecks (config, view, PM_FPA_AFTER)) ESCAPE;

    psLogMsg ("psastro", 3, "save data : %f sec\n", psTimerMark ("psastroDataSave"));

    psFree (view);

    return true;
}
