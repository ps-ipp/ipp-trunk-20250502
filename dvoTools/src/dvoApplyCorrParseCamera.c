#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

# include "dvoApplyCorr.h"
bool dvoApplyCorrSelectCorrFile (pmConfig *config, pmFPA *input);

dvoApplyCorrOptions *dvoApplyCorrParseCamera (pmConfig *config) {

    bool status = false;

    // the input image defines the camera, and all recipes and options the follow
    pmFPAfile *input = pmFPAfileDefineFromArgs (NULL, config, "DVOFLAT.INPUT", "INPUT");
    if (!input) {
        psError(PS_ERR_IO, false, "Failed to build FPA from DVOFLAT.INPUT");
        return NULL;
    }

    // add recipe options supplied on command line
    // psMetadata *recipe  = psMetadataLookupPtr (&status, config->recipes, RECIPE_NAME);

    // parse the options from the metadata format to the dvoApplyCorrOptions structure
    dvoApplyCorrOptions *options = dvoApplyCorrOptionsParse (config);

    // the following files are output targets
    pmFPAfile *output = pmFPAfileDefineOutput(config, input->fpa, "DVOFLAT.OUTPUT");
    if (!output) {
        psError(PS_ERR_IO, false, _("Unable to generate output file from DVOFLAT.OUTPUT"));
        psFree(options);
        return NULL;
    }
    output->save = TRUE;

    // find the flat-field correction image (from command-line, config file, or detrend db)
    status = dvoApplyCorrSelectCorrFile (config, input->fpa);
    if (!status) {
	psError (PS_ERR_IO, false, "can't find a flat-field correction image source");
	return NULL;
    }
    
    // Chip selection: turn on only the chips specified (pass status to suppress missing-key log msg)
    status = false;
    char *chipLine = psMetadataLookupStr(&status, config->arguments, "CHIP_SELECTIONS");
    psArray *chips = psStringSplitArray (chipLine, ",", false);
    if (chips->n > 0) {
        pmFPASelectChip (output->fpa, -1, true); // deselect all chips
        for (int i = 0; i < chips->n; i++) {
            int chipNum = atoi(chips->data[i]);
            if (! pmFPASelectChip(output->fpa, chipNum, false)) {
                psError(PS_ERR_IO, false, "Chip number %d doesn't exist in camera.\n", chipNum);
                return NULL;
            }
        }
    }
    psFree (chips);
    return (options);
}

bool dvoApplyCorrSelectCorrFile (pmConfig *config, pmFPA *input) {

    bool status;
    pmFPAfile *file = NULL;

    file = pmFPAfileDefineFromArgs  (&status, config, "DVOFLAT.CORR", "CORR");
    if (!status) {
	psError (PS_ERR_UNKNOWN, false, "failed to load file definition");
	return false;
    }
    if (file) {
	if (file->type != PM_FPA_FILE_IMAGE) {
	    psError(PS_ERR_IO, true, "DVOFLAT.CORR is not of type IMAGE");
	    return false;
	}
	return true;
    }

    file = pmFPAfileDefineFromConf  (&status, config, "DVOFLAT.CORR");
    if (!status) {
	psError (PS_ERR_UNKNOWN, false, "failed to load file definition");
	return false;
    }
    if (file) {
	if (file->type != PM_FPA_FILE_IMAGE) {
	    psError(PS_ERR_IO, true, "DVOFLAT.CORR is not of type IMAGE");
	    return false;
	}
	return true;
    }

    file = pmFPAfileDefineFromDetDB (&status, config, "DVOFLAT.CORR", input, PM_DETREND_TYPE_FLATCORR);
    if (!status) {
	psError (PS_ERR_UNKNOWN, false, "failed to load file definition");
	return false;
    }
    if (file) {
	if (file->type != PM_FPA_FILE_IMAGE) {
	    psError(PS_ERR_IO, true, "DVOFLAT.CORR is not of type IMAGE");
	    return false;
	}
	return true;
    }
    return false;
}
