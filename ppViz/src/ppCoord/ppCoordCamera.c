#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <pslib.h>
#include <psmodules.h>

#include "ppCoord.h"

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


bool ppCoordCamera(ppCoordData *data // Run-time data
    )
{
    bool status;                        // Status of file definition

    fileArguments("ASTROM", data->astromName, "Input astrometry", data->config);
    pmFPAfile *astrom = pmFPAfileDefineFromArgs(&status, data->config, "PPCOORD.ASTROM", "ASTROM"); // File
    if (!status || !astrom) {
        psError(PS_ERR_IO, false, "Failed to build file from PPCOORD.ASTROM");
        return false;
    }

    if (data->rawName) {
        fileArguments("RAW", data->rawName, "Input raw image", data->config);
        pmFPAfile *raw = pmFPAfileDefineFromArgs(&status, data->config, "PPCOORD.RAW", "RAW"); // File
        if (!status || !raw) {
            psError(PS_ERR_IO, false, "Failed to build file from PPCOORD.RAW");
            return false;
        }
    }

#if 0
    // Now the camera has been determined, we can read the recipe
    psMetadata *recipe = psMetadataLookupMetadata(NULL, data->config->recipes, PPVIZPSF_RECIPE); // Recipe
    if (!recipe) {
        psError(PS_ERR_UNEXPECTED_NULL, false, "Unable to find recipe %s", PPVIZPSF_RECIPE);
        return false;
    }
#endif

    return true;
}
