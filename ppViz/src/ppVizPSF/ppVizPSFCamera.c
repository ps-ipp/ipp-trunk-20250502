#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <pslib.h>
#include <psmodules.h>

#include "ppVizPSF.h"

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


bool ppVizPSFCamera(ppVizPSFData *data // Run-time data
    )
{
    bool status;                        // Status of file definition

    fileArguments("PSF", data->psfName, "Input PSF", data->config);
    pmFPAfile *psf = pmFPAfileDefineFromArgs(&status, data->config, "PSPHOT.PSF.LOAD", "PSF"); // File
    if (!status || !psf) {
        psError(PS_ERR_IO, false, "Failed to build file from PSPHOT.LOAD.PSF");
        return false;
    }

    if (data->sourcesName) {
        fileArguments("SOURCES", data->sourcesName, "Input sources", data->config);
        pmFPAfile *srcs = pmFPAfileBindFromArgs(&status, psf, data->config,
						"PSPHOT.INPUT.CMF", "SOURCES"); // File
	fprintf(stderr,"%ld %d\n",(long) srcs, status);
        if (!status || !srcs) {
            psError(PS_ERR_IO, false, "Failed to build file from PSPHOT.INPUT.CMF");
            return false;
        }
    }

    pmFPAfile *output = pmFPAfileDefineOutput(data->config, psf->fpa, "PPVIZPSF.OUTPUT");
    if (!output) {
        psError(psErrorCodeLast(), false, "Unable to define output.");
        return false;
    }
    output->save = true;

    // Now the camera has been determined, we can read the recipe
    psMetadata *recipe = psMetadataLookupMetadata(NULL, data->config->recipes, PPVIZPSF_RECIPE); // Recipe
    if (!recipe) {
        psError(PS_ERR_UNEXPECTED_NULL, false, "Unable to find recipe %s", PPVIZPSF_RECIPE);
        return false;
    }

    data->minFlux = psMetadataLookupF32(NULL, recipe, "MINFLUX"); // Minimum flux
    if (!isfinite(data->minFlux) || data->minFlux <= 0) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Unable to find legitimate value for MINFLUX");
        return false;
    }

    if (!data->sourcesName) {
        data->size = psMetadataLookupS32(NULL, recipe, "SIZE"); // Size of PSF
        if (data->size <= 0) {
            psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Unable to find legitimate value for SIZE");
            return false;
        }
    }

    return true;
}
