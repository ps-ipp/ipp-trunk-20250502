#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

# include "ppstamp.h"

bool ppstampMegacamWorkaround = false;

// Set up the ppstamp output Image file
bool setupOutput(pmConfig *config, pmFPAfile *input, ppstampOptions *options, bool doMask, bool doWeight, pmFPAfile **pOutput)
{
    if (!options->stage || (strcmp(options->stage, "diff") != 0)) {
        if (options->nocompress) {
            options->outputFileRule = "PPSTAMP.OUTPUT.NOCOMP";
        } else {
            options->outputFileRule = "PPSTAMP.OUTPUT";
        }
    } else {
        // need special filerule for diff stage image to allow for negative values
        if (options->nocompress) {
            options->outputFileRule = "PPSTAMP.OUTPUT.DIFF.NOCOMP";
        } else {
            options->outputFileRule = "PPSTAMP.OUTPUT.DIFF";
        }
    }
    pmFPAfile *output = pmFPAfileDefineSkycell(config, NULL, options->outputFileRule);
    if (!output) {
        psError(PS_ERR_UNKNOWN, false, "Unable to setup output.");
        return false;
    }
    *pOutput = output;

    output->save = true;

    if (doMask) {
        char *rule;
        if (options->nocompress) {
            rule =  "PPSTAMP.OUTPUT.MASK.NOCOMP";
        } else {
            rule =  "PPSTAMP.OUTPUT.MASK";
        }
        pmFPAfile *outMask = pmFPAfileDefineSkycell(config, output->fpa, rule);
        outMask->save = true;
    }
    if (doWeight) {
        char *rule;
        if (options->nocompress) {
            rule =  "PPSTAMP.OUTPUT.VARIANCE.NOCOMP";
        } else {
            rule =  "PPSTAMP.OUTPUT.VARIANCE";
        }
        pmFPAfile *outWeight = pmFPAfileDefineSkycell(config, output->fpa, rule);
        outWeight->save = true;
    }

    return true;
}

// This function seems mis-named. What are we doing with regards to the Camera?
// Well we are building the output image based on the camera. Is there
// something else that I'm missing?

bool ppstampParseCamera(pmConfig *config, ppstampOptions *options)
{
    bool status = false;

    // the input image defines the camera, and all recipes and options the follow
    pmFPAfile *input = pmFPAfileDefineFromArgs (&status, config, "PPSTAMP.INPUT", "INPUT");
    if (!status || !input) {
        psError(PS_ERR_IO, false, "Failed to build FPA from PPSTAMP.INPUT");
        return false;
    }
    if (input->type != PM_FPA_FILE_IMAGE) {
        psError(PS_ERR_IO, true, "PPSTAMP.INPUT is not of type IMAGE");
        return false;
    }

    bool doMask = false;
    pmFPAfile *mask = pmFPAfileBindFromArgs (&status, input, config, "PPSTAMP.INPUT.MASK", "MASK");
    if (!status) {
        psError(PS_ERR_IO, false, "Failed to build FPA from PPSTAMP.INPUT.MASK");
        return false;
    }
    if (mask) {
        doMask = true;
    }
    bool doWeight = false;
    pmFPAfile *weight = pmFPAfileBindFromArgs (&status, input, config, "PPSTAMP.INPUT.VARIANCE", "VARIANCE");
    if (!status) {
        psError(PS_ERR_IO, false, "Failed to build FPA from PPSTAMP.INPUT.VARIANCE");
        return false;
    }
    if (weight) {
        doWeight = true;
    }
        
    pmFPAfile *astrom = pmFPAfileDefineFromArgs(&status, config, "PSWARP.ASTROM", "ASTROM");
    if (!status) {
        psError(PS_ERR_UNKNOWN, false, "failed to find astrom definitiion");
        return NULL;
    }
    if (astrom) {
        psLogMsg ("ppstamp", 3, "Using supplied astrometry.\n");
    } else {
        psLogMsg ("ppstamp", 3, "Using header astrometry.\n");
    }

    // Set up the output target
    pmFPAfile *output;
    if (!setupOutput(config, input, options, doMask, doWeight, &output)) {
        psError(PS_ERR_UNKNOWN, false, "Unable to setup output.");
        return false;
    }

    // Set up the input and output sources files if needed
    if (options->writeCMF) {
        bool status;

        // see if -sources file was supplied. 
        // If so define the file.
        psPtr sourcesFile = psMetadataLookupPtr(&status, config->arguments, "SOURCES");
        if (sourcesFile) {
            pmFPAfile *sources = pmFPAfileBindFromArgs(&status, input, config, "PPSTAMP.INPUT.SOURCES", "SOURCES");
            if (!status) {
                psError(psErrorCodeLast(), false, "Failed to load file definition for PPSTAMP.INPUT.SOURCES");
                return false;
            }
            if (!sources && !astrom) {
                psError(psErrorCodeLast(), false, "Failed to define input sources file");
                return false;
            }
        } else {
            psLogMsg ("ppstamp", PS_LOG_INFO, "Output sources file requested but no -sources supplied.\n");
            psLogMsg ("ppstamp", PS_LOG_INFO, "Will use the sources in the supplied astrometry file.\n");
        }
        
        pmFPAfile *outputSources = NULL;
        outputSources = pmFPAfileDefineSkycell(config, output->fpa, "PPSTAMP.OUTPUT.SOURCES");
        if (!outputSources) {
            psError(psErrorCodeLast(), false, "Failed to define output sources file");
            return false;
        }
        outputSources->save = true;
    }

    pmFPAfile *chipImage = pmFPAfileDefineChipMosaic(config, input->fpa, "PPSTAMP.CHIP");
    if (!chipImage) {
        psError(PS_ERR_IO, false, _("Unable to generate new file from PPSTAMP.CHIP"));
        return NULL;
    }
    if (chipImage->type != PM_FPA_FILE_IMAGE) {
        psError(PS_ERR_IO, true, "PPSTAMP.CHIP is not of type IMAGE");
        return NULL;
    }

    if (options->writeJPEG) {
        char *filerule = "PPSTAMP.OUTPUT.JPEG";
        pmFPAfile *jpg = pmFPAfileDefineSkycell(config, output->fpa, filerule);
        if (!jpg) {
            psError(PS_ERR_IO, false, "Unable to generate new file from %s", filerule);
            return NULL;
        }
        if (jpg->type != PM_FPA_FILE_JPEG) {
            psError(PS_ERR_IO, true, "%s is not of type JPEG", filerule);
            return NULL;
        }
        jpg->save = true;
    }

    // XXX: TODO: only do the workaround if the file level is chip, skycells should be fine
    if (!strcmp(config->cameraName, "MEGACAM")) {
        // workaround bug 986 and related
        ppstampMegacamWorkaround = true;
    }
 
    return true;
}
