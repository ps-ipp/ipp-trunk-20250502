/** @file psastroDataSave.c
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
  psError(PS_ERR_UNKNOWN, false, "Failure in psastroDataSave"); \
  psFree (view); \
  return false; \
}

/**
 * this loop saves the photometry/astrometry data files
 */
bool psastroDataSave (pmConfig *config, psMetadata *stats) {

    pmChip *chip;
    pmCell *cell;
    pmReadout *readout;

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
    pmFPAfileActivate (config->files, true, "PSASTRO.OUT.ASTROM");
    pmFPAfileActivate (config->files, true, "PSPHOT.OUTPUT.CFF");

    pmFPAview *view = pmFPAviewAlloc (0);
    pmHDU *lastHDU = NULL;              // Last HDU updated

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

		psastroGalaxyShapeErrors (recipe, readout);

                // Put version information into the header
                pmHDU *hdu = pmHDUGetHighest(output->fpa, chip, cell);
                if (hdu && hdu != lastHDU) {
                    psastroVersionHeaderFull(hdu->header);
		    // Append the reference catalog to the header as well.
		    char *catdir = psMetadataLookupStr(NULL, recipe, "PSASTRO.CATDIR");
		    psMetadataAddStr(hdu->header, PS_LIST_TAIL, "PSREFCAT", PS_META_REPLACE, NULL, catdir);
                    lastHDU = hdu;
                }

                if (!pmFPAfileIOChecks (config, view, PM_FPA_AFTER)) ESCAPE;
            }
            if (!pmFPAfileIOChecks (config, view, PM_FPA_AFTER)) ESCAPE;
        }
        if (!pmFPAfileIOChecks (config, view, PM_FPA_AFTER)) ESCAPE;
    }
    if (!pmFPAfileIOChecks (config, view, PM_FPA_AFTER)) ESCAPE;

    // Write out summary statistics
    if (!psastroMetadataStats (config, stats)) ESCAPE;

    bool status;
    psString dump_file = psMetadataLookupStr(&status, config->arguments, "DUMP_CONFIG");
    if (dump_file) {
        pmConfigCamerasCull(config, NULL);
        pmConfigRecipesCull(config, "PPIMAGE,PPSTATS,PSPHOT,MASKS,PSASTRO");

        if (!pmConfigDump(config, dump_file)) {
            psError(psErrorCodeLast(), false, "Unable to dump configuration.");
            psFree(view);
            return false;
        }
    }

    // activate all files except PSASTRO.OUTPUT, and PSPHOT.OUTPUT.CFF
    pmFPAfileActivate (config->files, true, NULL);
    pmFPAfileActivate (config->files, false, "PSASTRO.OUTPUT");
    pmFPAfileActivate (config->files, false, "PSPHOT.OUTPUT.CFF");

    psFree (view);

    return true;
}
