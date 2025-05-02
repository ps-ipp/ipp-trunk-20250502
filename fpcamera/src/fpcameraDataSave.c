# include "fpcamera.h"

# define ESCAPE(ERROR, MSG) { psErrorStackPrint(stderr, MSG); psFree (view); return false; }

/* \brief this loop saves the photometry/astrometry data files */
bool fpcameraDataSave (pmConfig *config, psMetadata *stats) {

    bool status;
    pmChip *chip;
    pmCell *cell;
    pmReadout *readout;
    pmFPAview *view = NULL;

    // select the current recipe
    psMetadata *recipe  = psMetadataLookupPtr (&status, config->recipes, FPCAMERA_RECIPE);
    if (!recipe) ESCAPE (FPCAMERA_ERR_CONFIG, "Can't find FPCAMERA recipe");

    // select the output data sources
    pmFPAfile *output = psMetadataLookupPtr (NULL, config->files, "FPCAMERA.OUTPUT");
    if (!output) ESCAPE(FPCAMERA_ERR_CONFIG, "Can't find or interpret output file rule FPCAMERA.OUTPUT!");

    // de-activate all files except FPCAMERA.OUTPUT
    pmFPAfileActivate (config->files, false, NULL);
    pmFPAfileActivate (config->files, true, "FPCAMERA.OUTPUT");
    pmFPAfileActivate (config->files, true, "FPCAMERA.INPUT.ASTROM");
    // Note: I/O for the image-type files is performed in fpcameraAnalysis

    view = pmFPAviewAlloc (0);

    // open/load files as needed
    if (!pmFPAfileIOChecks (config, view, PM_FPA_BEFORE)) ESCAPE(FPCAMERA_ERR_IO, "failure to save at FPA");

    pmHDU *lastHDU = NULL;              // Last HDU updated
    while ((chip = pmFPAviewNextChip (view, output->fpa, 1)) != NULL) {
        psTrace ("fpcamera", 4, "Chip %d: %x %x\n", view->chip, chip->file_exists, chip->process);
        if (!chip->process || !chip->file_exists) { continue; }
        if (!pmFPAfileIOChecks (config, view, PM_FPA_BEFORE)) ESCAPE(FPCAMERA_ERR_IO, "failure to save at Chip");

        while ((cell = pmFPAviewNextCell (view, output->fpa, 1)) != NULL) {
            psTrace ("fpcamera", 4, "Cell %d: %x %x\n", view->cell, cell->file_exists, cell->process);
            if (!cell->process || !cell->file_exists) { continue; }
            if (!pmFPAfileIOChecks (config, view, PM_FPA_BEFORE)) ESCAPE(FPCAMERA_ERR_IO, "failure to save at Cell");

            // process each of the readouts
            while ((readout = pmFPAviewNextReadout (view, output->fpa, 1)) != NULL) {
                if (!pmFPAfileIOChecks (config, view, PM_FPA_BEFORE)) ESCAPE(FPCAMERA_ERR_IO, "failure in IOChecks(BEFORE) at Readout");
                if (!readout->data_exists) { continue; }

                // Put version information into the header
                pmHDU *hdu = pmHDUGetHighest(output->fpa, chip, cell);
                if (hdu && hdu != lastHDU) {
                    fpcameraVersionHeaderFull(hdu->header);
		    // Append the reference catalog to the header as well.
		    char *catdir = psMetadataLookupStr(NULL,recipe,"FPCAMERA.CATDIR");
		    psMetadataAddStr(hdu->header,PS_LIST_TAIL, "PSREFCAT", PS_META_REPLACE, NULL, catdir);
                    lastHDU = hdu;
                }
                if (!pmFPAfileIOChecks (config, view, PM_FPA_AFTER)) ESCAPE(FPCAMERA_ERR_IO, "failure in IOChecks(AFTER) at Readout");
            }
            if (!pmFPAfileIOChecks (config, view, PM_FPA_AFTER)) ESCAPE(FPCAMERA_ERR_IO, "failure in IOChecks(AFTER) at Cell");
        }
        if (!pmFPAfileIOChecks (config, view, PM_FPA_AFTER)) ESCAPE(FPCAMERA_ERR_IO, "failure in IOChecks(AFTER) at Chip");
    }
    if (!pmFPAfileIOChecks (config, view, PM_FPA_AFTER)) ESCAPE(FPCAMERA_ERR_IO, "failure in IOChecks(AFTER) at FPA");

    // Write out summary statistics
    if (!fpcameraMetadataStats (config, stats)) ESCAPE(FPCAMERA_ERR_UNKNOWN, "failure to save state in Metadata");

    psString dump_file = psMetadataLookupStr(&status, config->arguments, "DUMP_CONFIG");
    if (dump_file) {
        pmConfigCamerasCull(config, NULL);
        pmConfigRecipesCull(config, "PPIMAGE,PPSTATS,PSPHOT,MASKS,FPCAMERA");

        if (!pmConfigDump(config, dump_file)) ESCAPE(FPCAMERA_ERR_IO, "Unable to dump configuration.");
    }

    psFree (view);
    return true;
}
