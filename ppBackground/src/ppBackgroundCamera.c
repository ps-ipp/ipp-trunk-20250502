#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <pslib.h>
#include <psmodules.h>

#include "ppBackground.h"

/// Add a single filename to the arguments as an array, so that it can be used with pmFPAfileBindFromArgs, etc
static void fileArguments(const char *file, // The symbolic name for the file
                          const char *name, // The name of the file
                          const char *comment, // Description of the file
                          pmConfig *config // Configuration
    )
{
    psArray *files = psArrayAlloc(1); // Array with file names
    files->data[0] = psStringCopy(name);
    if (psMetadataLookup(config->arguments, file)) {
        psMetadataRemoveKey(config->arguments, file);
    }
    psMetadataAddArray(config->arguments, PS_LIST_TAIL, file, 0, comment, files);
    psFree(files);
    return;
}


bool ppBackgroundCamera(ppBackgroundData *data // Run-time data
    )
{
    bool status;                        // Status of file definition

    if (data->patternName) {
        fileArguments("PATTERN", data->patternName, "Input pattern", data->config);
        pmFPAfileDefineFromArgs(&status, data->config, "PPBACKGROUND.PATTERN", "PATTERN"); // File
        if (!status) {
            psError(psErrorCodeLast(), false, "Failed to build file from PPBACKGROUND.PATTERN");
            return false;
        }
    }

    if (data->backgroundName) {
        fileArguments("BACKGROUND", data->backgroundName, "Input background model", data->config);
        pmFPAfileDefineFromArgs(&status, data->config, "PPBACKGROUND.BACKGROUND", "BACKGROUND"); // File
        if (!status) {
            psError(psErrorCodeLast(), false, "Failed to build file from PPBACKGROUND.BACKGROUND");
            return false;
        }
    }

    fileArguments("IMAGE", data->imageName, "Input uncorrected image", data->config);
    pmFPAfile *image = pmFPAfileDefineFromArgs(&status, data->config, "PPBACKGROUND.IMAGE",
                                               "IMAGE"); // File
    if (!status || !image) {
        psError(psErrorCodeLast(), false, "Failed to build file from PPBACKGROUND.IMAGE");
        return false;
    }

    fileArguments("MASK", data->maskName, "Input uncorrected mask", data->config);
    pmFPAfileBindFromArgs(&status, image, data->config, "PPBACKGROUND.MASK", "MASK"); // File
    if (!status) {
        psError(psErrorCodeLast(), false, "Failed to build file from PPBACKGROUND.MASK");
        return false;
    }

    fileArguments("VARIANCE", data->varianceName, "Input uncorrected variance", data->config);
    pmFPAfileBindFromArgs(&status, image, data->config, "PPBACKGROUND.VARIANCE", "VARIANCE"); // File
    if (!status) {
        psError(psErrorCodeLast(), false, "Failed to build file from PPBACKGROUND.VARIANCE");
        return false;
    }

    if (data->auxMaskName) {
        fileArguments("AUXMASK", data->auxMaskName, "Auxiliary mask", data->config);
        pmFPAfileBindFromArgs(&status, image, data->config, "PPBACKGROUND.AUXMASK", "AUXMASK"); // File
        if (!status) {
            psError(psErrorCodeLast(), false, "Failed to build file from PPBACKGROUND.MASK");
            return false;
        }
    }

    pmFPAfile *output = pmFPAfileDefineOutput(data->config, image->fpa, "PPBACKGROUND.OUTPUT");
    if (!output) {
        psError(psErrorCodeLast(), false, "Unable to define output.");
        return false;
    }
    output->save = true;

    pmFPAfile *outMask = pmFPAfileDefineOutput(data->config, output->fpa, "PPBACKGROUND.OUTPUT.MASK");
    if (!outMask) {
        psError(psErrorCodeLast(), false, "Unable to define output.");
        return false;
    }
    outMask->save = true;

    pmFPAfile *outVariance = pmFPAfileDefineOutput(data->config, output->fpa, "PPBACKGROUND.OUTPUT.VARIANCE");
    if (!outVariance) {
        psError(psErrorCodeLast(), false, "Unable to define output.");
        return false;
    }
    outVariance->save = true;

#if 0
    // Now the camera has been determined, we can read the recipe
    psMetadata *recipe = psMetadataLookupMetadata(NULL, data->config->recipes, PPBACKGROUND_RECIPE); // Recipe
    if (!recipe) {
        psError(PS_ERR_UNEXPECTED_NULL, false, "Unable to find recipe %s", PPBACKGROUND_RECIPE);
        return false;
    }
#endif

    return true;
}
