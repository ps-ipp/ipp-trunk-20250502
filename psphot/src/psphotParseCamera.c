# include "psphotStandAlone.h"

// define the needed / desired I/O files


bool psphotParseCamera (pmConfig *config) {

    bool status = false;

    // the file to be loaded may have subdivisions at the cell and readout level
    // we load into pmFPAfile *load, then reformat into pmFPAfile *input
    pmFPAfile *load = pmFPAfileDefineFromArgs (&status, config, "PSPHOT.LOAD", "INPUT");
    if (!status) {
        psError(PSPHOT_ERR_CONFIG, false, "Failed to build FPA from PSPHOT.LOAD");
        return status;
    }
    load->dataLevel = PM_FPA_LEVEL_CHIP; // force load at the CHIP level

    // if MASK or VARIANCE was supplied on command line, bind files to 'load'
    // the mask and weight will be mosaicked with the image
    pmFPAfileBindFromArgs (&status, load, config, "PSPHOT.MASK", "MASK");
    if (!status) {
        psError (PS_ERR_UNKNOWN, false, "failed to load find definition");
        return NULL;
    }
    if (!psphotSetMaskBits (config)) {
        psError (PS_ERR_UNKNOWN, false, "failed to set mask bit values");
        return NULL;
    }

    pmFPAfileBindFromArgs (&status, load, config, "PSPHOT.VARIANCE", "VARIANCE");
    if (!status) {
        psError (PS_ERR_UNKNOWN, false, "failed to load find definition");
        return NULL;
    }

    // the psphot analysis is performed on chips
    pmFPAfile *input = pmFPAfileDefineChipMosaic(config, load->fpa, "PSPHOT.INPUT");
    if (!input) {
        psError(PSPHOT_ERR_CONFIG, false, _("Unable to generate new file from PSPHOT.INPUT"));
        return NULL;
    }

    pmFPAfileBindFromArgs (&status, input, config, "PSPHOT.EXPNUM", "EXPNUM");
    if (!status) {
        psError (PS_ERR_UNKNOWN, false, "failed to load find definition");
        return NULL;
    }


    // define the additional input/output files associated with psphot
    if (!psphotDefineFiles (config, input)) {
        psError(PSPHOT_ERR_CONFIG, false, "Trouble defining the additional input/output files");
        return false;
    }

    // Chip selection: turn on only the chips specified (pass status to suppress missing-key log msg)
    char *chipLine = psMetadataLookupStr(&status, config->arguments, "CHIP_SELECTIONS");
    psArray *chips = psStringSplitArray (chipLine, ",", false);
    if (chips->n > 0) {
        // select on the basis of extname?
        pmFPASelectChip (load->fpa, -1, true); // deselect all chips
        for (int i = 0; i < chips->n; i++) {
            int chipNum = atoi(chips->data[i]);
            if (! pmFPASelectChip(load->fpa, chipNum, false)) {
                psError(PSPHOT_ERR_CONFIG, false, "Chip number %d doesn't exist in camera.\n", chipNum);
                return false;
            }
        }
    }
    psFree (chips);
    psTrace("psphot", 1, "Done with psphotParseCamera...\n");

    psErrorClear();                     // some metadata lookup may have failed
    return true;
}
