# include "psphotInternal.h"

bool psphotINpsphotStack = false;

// List of output files
static const char *outputFiles[] = { "PSPHOT.OUTPUT", "PSPHOT.RESID", "PSPHOT.BACKMDL",
                                     "PSPHOT.BACKMDL.STDEV", "PSPHOT.BACKGND", "PSPHOT.BACKSUB",
                                     "PSPHOT.PSF.SAVE", "SOURCE.PLOT.MOMENTS", "SOURCE.PLOT.PSFMODEL",
                                     "SOURCE.PLOT.APRESID", NULL };

// XXX we need to be able to distinguish several cases:
// 1) the particular output data was not requested
// 2) the particular output data was requested but was not generated (skipped)
// 3) the particular output data was requested but was not generated (for valid failure)
// 4) the particular output data was requested but was not generated (surprising failure)

// define the needed / desired I/O files
bool psphotDefineFiles (pmConfig *config, pmFPAfile *input) {

    bool status = false;

    // select recipe options supplied on command line
    psMetadata *recipe  = psMetadataLookupPtr (&status, config->recipes, PSPHOT_RECIPE);

    // the output sources are carried on the input->fpa structures
    pmFPAfile *outsources = pmFPAfileDefineOutputFromFile (config, input, "PSPHOT.OUTPUT");
    if (!outsources) {
        psError(PSPHOT_ERR_CONFIG, false, "Cannot find a rule for PSPHOT.OUTPUT");
        return false;
    }
    outsources->save = true;

    // if we are choosing not to save the output detections, mark this file inactive so it will
    // not be written, but will keep the detections in memory for other functions (eg, astrometry)
    if (!psMetadataLookupBool (NULL, recipe, "SAVE.OUTPUT")) {
        pmFPAfileActivate (config->files, false, "PSPHOT.OUTPUT");
        outsources->save = false;
    }

    // optionally save the residual image
    if (psMetadataLookupBool(NULL, recipe, "SAVE.RESID")) {
        pmFPAfile *output = pmFPAfileDefineOutputFromFile (config, input, "PSPHOT.RESID");
        if (!output) {
            psError(PSPHOT_ERR_CONFIG, false, "Cannot find a rule for PSPHOT.RESID");
            return false;
        }
        output->save = true;
    }
    // optionally save the background model (small FITS image)
    if (psMetadataLookupBool(NULL, recipe, "SAVE.BACKMDL")) {
        int DX = psMetadataLookupS32 (&status, recipe, "BACKGROUND.XBIN");
        int DY = psMetadataLookupS32 (&status, recipe, "BACKGROUND.YBIN");
        pmFPAfile *output = pmFPAfileDefineFromFile (config, input, DX, DY, "PSPHOT.BACKMDL");
        if (!output) {
            psError(PSPHOT_ERR_CONFIG, false, "Cannot find a rule for PSPHOT.BACKMDL");
            return false;
        }
        output->save = true;
    }
    // optionally save the background model's standard deviation (small FITS image)
    if (psMetadataLookupBool(NULL, recipe, "SAVE.BACKMDL.STDEV")) {
        int DX = psMetadataLookupS32 (&status, recipe, "BACKGROUND.XBIN");
        int DY = psMetadataLookupS32 (&status, recipe, "BACKGROUND.YBIN");
        pmFPAfile *output = pmFPAfileDefineFromFile (config, input, DX, DY, "PSPHOT.BACKMDL.STDEV");
        if (!output) {
            psError(PSPHOT_ERR_CONFIG, false, "Cannot find a rule for PSPHOT.BACKMDL.STDEV");
            return false;
        }
        output->save = true;
    }
    // optionally save the full background image
    if (psMetadataLookupBool(NULL, recipe, "SAVE.BACKGND")) {
        pmFPAfile *output = pmFPAfileDefineFromFile (config, input,  1,  1, "PSPHOT.BACKGND");
        if (!output) {
            psError(PSPHOT_ERR_CONFIG, false, "Cannot find a rule for PSPHOT.BACKGND");
            return false;
        }
        output->save = true;
    }
    // optionally save the background-subtracted image
    if (psMetadataLookupBool(NULL, recipe, "SAVE.BACKSUB")) {
        pmFPAfile *output = pmFPAfileDefineFromFile (config, input,  1,  1, "PSPHOT.BACKSUB");
        if (!output) {
            psError(PSPHOT_ERR_CONFIG, false, "Cannot find a rule for PSPHOT.");
            return false;
        }
        output->save = true;
    }
//    // optionally save the smoothed variance model (small FITS image)
//    if (psMetadataLookupBool(NULL, recipe, "SAVE.BACKMDL")) {
//        int DX = psMetadataLookupS32 (&status, recipe, "BACKGROUND.XBIN");
//        int DY = psMetadataLookupS32 (&status, recipe, "BACKGROUND.YBIN");
//        pmFPAfile *output = pmFPAfileDefineFromFile (config, input, DX, DY, "PSPHOT.VARMDL");
//        if (!output) {
//            psError(PSPHOT_ERR_CONFIG, false, "Cannot find a rule for PSPHOT.VARMDL");
//            return false;
//        }
//        output->save = true;
//    }
//    // optionally save the smoothed variance model's standard deviation (small FITS image)
//    if (psMetadataLookupBool(NULL, recipe, "SAVE.VARMDL.STDEV")) {
//        int DX = psMetadataLookupS32 (&status, recipe, "BACKGROUND.XBIN");
//        int DY = psMetadataLookupS32 (&status, recipe, "BACKGROUND.YBIN");
//        pmFPAfile *output = pmFPAfileDefineFromFile (config, input, DX, DY, "PSPHOT.VARMDL.STDEV");
//        if (!output) {
//            psError(PSPHOT_ERR_CONFIG, false, "Cannot find a rule for PSPHOT.VARMDL.STDEV");
//            return false;
//        }
//        output->save = true;
//    }
    // optionally save the PSF Model
    if (psMetadataLookupBool(NULL, recipe, "SAVE.PSF")) {
        pmFPAfile *output = pmFPAfileDefineOutputFromFile (config, input, "PSPHOT.PSF.SAVE");
        if (!output) {
            psError(PSPHOT_ERR_CONFIG, false, "Cannot find a rule for PSPHOT.PSF.SAVE");
            return false;
        }
        output->save = true;
    }

    // optionally save output plots
    // allow specific plots only
    if (psMetadataLookupBool(NULL, recipe, "SAVE.PLOTS")) {
        pmFPAfile *output = NULL;
        output = pmFPAfileDefineOutputFromFile (config, input, "SOURCE.PLOT.MOMENTS");
        if (!output) {
            psError(PSPHOT_ERR_CONFIG, false, "Cannot find a rule for SOURCE.PLOT.MOMENTS");
            return false;
        }
        output->save = true;
        output = pmFPAfileDefineOutputFromFile (config, input, "SOURCE.PLOT.PSFMODEL");
        if (!output) {
            psError(PSPHOT_ERR_CONFIG, false, "Cannot find a rule for SOURCE.PLOT.PSFMODEL");
            return false;
        }
        output->save = true;
        output = pmFPAfileDefineOutputFromFile (config, input, "SOURCE.PLOT.APRESID");
        if (!output) {
            psError(PSPHOT_ERR_CONFIG, false, "Cannot find a rule for SOURCE.PLOT.APRESID");
            return false;
        }
        output->save = true;
    }

    if (psMetadataLookupPtr(NULL, config->arguments, "SRC")) {
        if (!pmFPAfileDefineFromArgs (&status, config, "PSPHOT.INPUT.CMF", "SRC")) {
            psError(PSPHOT_ERR_CONFIG, false, "Failed to find/build PSPHOT.INPUT.CMF");
            return status;
        }
    }

    if (psMetadataLookupPtr(NULL, config->arguments, "FORCE")) {
        if (!pmFPAfileDefineFromArgs (&status, config, "PSPHOT.INPUT.CFF", "FORCE")) {
            psError(PSPHOT_ERR_CONFIG, false, "Failed to find/build PSPHOT.INPUT.CFF");
            return status;
        }
    }

    if (psMetadataLookupPtr(NULL, config->arguments, "SRCTEXT")) {
	// XXX cannot use pmFPAfileDefineFromArgs: this is explicitly a FITS-based I/O function
	// supply the attach the 
	if (!psphotLoadSRCTEXT(input->fpa, config)) {
            psError(PSPHOT_ERR_CONFIG, false, "Failed to load PSPHOT.INPUT.TEXT");
            return status;
        }
    }

    if (psMetadataLookupPtr(NULL, config->arguments, "PSPHOT.PSF")) {
        pmFPAfileBindFromArgs(&status, input, config, "PSPHOT.PSF.LOAD", "PSPHOT.PSF");
        if (!status) {
            psError(PSPHOT_ERR_CONFIG, false, "Failed to find/build PSPHOT.PSF.LOAD");
            return status;
        }
    }

    // XXX add in example PSF image thumbnails
    // pmFPAfileConstruct (config->files, format, config->camera, "PSPHOT.PSF_SAMPLE");

    return true;
}

void psphotFilesActivate(pmConfig *config, bool state)
{
    for (int i = 0; outputFiles[i]; i++) {
        if (!pmFPAfileActivate(config->files, state, outputFiles[i])) {
            psErrorClear();
        }
    }

    return;
}

// psphotGetFilerule
// Since psphotStack processes multipe FPAs at a time it has a different file rule structure than regular psphot. 
// For the background output files we define a function psphotGetFilerule which given a base psphot file rule
// returns the corresponding psphotStack rule *if* the program is psphotStack. That is indicated by a global
// boolean which defaults to false, and psphotStack only sets to true

const char *psphotGetFilerule(const char *psphotRule) {
    const char *rule = psphotRule;
    if (psphotINpsphotStack) {
        if (!strcmp(psphotRule, "PSPHOT.BACKMDL")) {
            rule =  "PSPHOT.STACK.BACKMDL";
        } else if (!strcmp(psphotRule, "PSPHOT.BACKMDL.STDEV")) {
            rule = "PSPHOT.STACK.BACKMDL.STDEV";
        } else if (!strcmp(psphotRule, "PSPHOT.BACKSUB")) {
            rule = "PSPHOT.STACK.BACKSUB";
        } else if (!strcmp(psphotRule, "PSPHOT.BACKGND")) {
            rule = "PSPHOT.STACK.BACKGND";
        } else {
            psAssert(0, "unsupported file rule %s", psphotRule);
        }
    }
    return rule;
}
