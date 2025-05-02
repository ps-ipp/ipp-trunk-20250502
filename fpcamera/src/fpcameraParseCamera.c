# include "fpcamera.h"

# define ESCAPE(ERROR,...) { p_psError(__FILE__,__LINE__,__func__,ERROR,false,__VA_ARGS__); return false; }

bool fpcameraParseCamera (pmConfig *config) {

    bool status = false;

    // the input image(s) are required arguments; they define the camera (or use the smf?)
    pmFPAfile *input = pmFPAfileDefineFromArgs (&status, config, "FPCAMERA.INPUT", "INPUT");
    if (!status) ESCAPE(FPCAMERA_ERR_CONFIG, "Failed to build FPA from FPCAMERA.INPUT");

    // if MASK was supplied on command line, bind files to 'input'
    pmFPAfileBindFromArgs (&status, input, config, "FPCAMERA.INPUT.MASK", "MASK");
    if (!status) ESCAPE(PS_ERR_UNKNOWN, "failed to load find definition");

    // if VARIANCE was supplied on command line, bind files to 'input'
    pmFPAfileBindFromArgs (&status, input, config, "FPCAMERA.INPUT.VARIANCE", "VARIANCE");
    if (!status) ESCAPE (PS_ERR_UNKNOWN, "failed to load find definition");

    pmFPAfile *psphotInput = pmFPAfileDefineFromFile (config, input, 1, 1, "PSPHOT.INPUT");
    if (!psphotInput) ESCAPE(FPCAMERA_ERR_CONFIG, "Failed to define FPA for PSPHOT.INPUT");

    if (!psphotSetMaskBits (config)) ESCAPE (PS_ERR_UNKNOWN, "failed to set mask bit values");

    // the input smf file is a required argument.  the smf file is distinct from the input
    // chips (FPCAMERA.INPUT = input).  accept the return argument to set the selected
    // chips in this pmFPAfile (as well as input)

    pmFPAfile *astro = pmFPAfileDefineFromArgs (&status, config, "FPCAMERA.INPUT.ASTROM", "INPUT.ASTROM");
    if (!status) ESCAPE(FPCAMERA_ERR_CONFIG, "Failed to build FPA from FPCAMERA.INPUT.ASTROM");

    // if optional PSF model is supplied, associate with input file
    if (psMetadataLookupPtr(NULL, config->arguments, "INPUT.PSF")) {
        pmFPAfileBindFromArgs(&status, input, config, "PSPHOT.PSF.LOAD", "INPUT.PSF");
        if (!status) ESCAPE(FPCAMERA_ERR_CONFIG, "Failed to find/build PSPHOT.PSF.LOAD");
    }

    // select the current recipe
    psMetadata *recipe  = psMetadataLookupPtr (&status, config->recipes, FPCAMERA_RECIPE);
    if (!recipe) ESCAPE(FPCAMERA_ERR_CONFIG, "Can't find FPCAMERA recipe!");

    // associates the output pmFPA with the file OUTPUT on config->arguments
    pmFPAfile *output = pmFPAfileDefineOutput (config, input->fpa, "FPCAMERA.OUTPUT");
    if (!output) ESCAPE(FPCAMERA_ERR_CONFIG, "Failed to build OUTPUT FPA from FPCAMERA.INPUT");
    output->save = true;

    // optionally save the residual image
    if (psMetadataLookupBool(&status, config->arguments, "SAVE.RESID")) {
        pmFPAfile *output = pmFPAfileDefineOutputFromFile (config, input, "FPCAMERA.RESID");
        if (!output) ESCAPE(FPCAMERA_ERR_CONFIG, "Cannot find a rule for FPCAMERA.RESID");
        output->save = true;
    }

    // Chip selection: turn on only the chips specified (option is not required)
    char *chipLine = psMetadataLookupStr(&status, config->arguments, "CHIP_SELECTIONS"); 
    psArray *chips = psStringSplitArray (chipLine, ",", false);
    if (chips->n > 0) {
	pmFPASelectChip (input->fpa, -1, true); // deselect all chips
	pmFPASelectChip (astro->fpa, -1, true); // deselect all chips
	for (int i = 0; i < chips->n; i++) {
	    int chipNum = atoi(chips->data[i]);
	    if (! pmFPASelectChip(input->fpa, chipNum, false)) ESCAPE(FPCAMERA_ERR_CONFIG, "Chip number %d doesn't exist in input chip images.\n", chipNum);
	    if (! pmFPASelectChip(astro->fpa, chipNum, false)) ESCAPE(FPCAMERA_ERR_CONFIG, "Chip number %d doesn't exist in astrometry file.\n", chipNum);
        }
    }
    psFree (chips);

    psTrace("fpcamera", 1, "Done with fpcameraParseCamera...\n");
    return true;
}

