#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

# include "dvoMakeCorr.h"

dvoMakeCorrOptions *dvoMakeCorrParseCamera (pmConfig *config) {

    // the REFERENCE image defines the camera, and all recipes and options the follow
    pmFPAfile *refhead = pmFPAfileDefineFromArgs (NULL, config, "DVOCORR.REFHEAD", "REFHEAD");
    if (!refhead) {
        psError(PS_ERR_IO, false, "Failed to build FPA from DVOCORR.REFHEAD");
        return NULL;
    }

    // the input image defines the camera, and all recipes and options the follow
    pmFPAfile *input = pmFPAfileDefineFromArgs (NULL, config, "DVOCORR.INPUT", "INPUT");
    if (!input) {
        psError(PS_ERR_IO, false, "Failed to build FPA from DVOCORR.INPUT");
        return NULL;
    }

    // add recipe options supplied on command line
    // psMetadata *recipe  = psMetadataLookupPtr (&status, config->recipes, RECIPE_NAME);

    // parse the options from the metadata format to the dvoMakeCorrOptions structure
    dvoMakeCorrOptions *options = dvoMakeCorrOptionsParse (config);

    // the following files are output targets
    // XXX get the binning from where?
    pmFPAfile *output = pmFPAfileDefineFromFPA(config, refhead->fpa, 1, 1, "DVOCORR.OUTPUT");
    if (!output) {
        psError(PS_ERR_IO, false, _("Unable to generate output file from DVOCORR.OUTPUT"));
        psFree(options);
        return NULL;
    }
    output->save = TRUE;

    // Chip selection: turn on only the chips specified (pass status to suppress missing-key log msg)
    bool status = false;
    char *chipLine = psMetadataLookupStr(&status, config->arguments, "CHIP_SELECTIONS");
    psArray *chips = psStringSplitArray (chipLine, ",", false);
    if (chips->n > 0) {
        pmFPASelectChip (input->fpa, -1, true); // deselect all chips
        for (int i = 0; i < chips->n; i++) {
            int chipNum = atoi(chips->data[i]);
            if (! pmFPASelectChip(input->fpa, chipNum, false)) {
                psError(PS_ERR_IO, false, "Chip number %d doesn't exist in camera.\n", chipNum);
                return false;
            }
        }
    }
    psFree (chips);
    return (options);
}
