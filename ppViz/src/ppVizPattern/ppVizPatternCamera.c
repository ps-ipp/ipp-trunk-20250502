#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <pslib.h>
#include <psmodules.h>

#include "ppVizPattern.h"

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


bool ppVizPatternCamera(ppVizPatternData *data // Run-time data
    )
{
    bool status;                        // Status of file definition

    fileArguments("PATTERN", data->patternName, "Input pattern", data->config);
    pmFPAfile *input = pmFPAfileDefineFromArgs(&status, data->config, "PPVIZPATTERN.INPUT",
                                               "PATTERN"); // File
    if (!status || !input) {
        psError(PS_ERR_IO, false, "Failed to build file from PPVIZPATTERN.INPUT");
        return false;
    }

    pmFPAfile *output = pmFPAfileDefineOutput(data->config, input->fpa, "PPVIZPATTERN.OUTPUT");
    if (!output) {
        psError(psErrorCodeLast(), false, "Unable to define output.");
        return false;
    }
    output->save = true;

#if 0
    // Now the camera has been determined, we can read the recipe
    psMetadata *recipe = psMetadataLookupMetadata(NULL, data->config->recipes, PPVIZPATTERN_RECIPE); // Recipe
    if (!recipe) {
        psError(PS_ERR_UNEXPECTED_NULL, false, "Unable to find recipe %s", PPVIZPATTERN_RECIPE);
        return false;
    }
#endif

    return true;
}
