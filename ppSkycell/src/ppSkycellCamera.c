#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <pslib.h>
#include <psmodules.h>

#include "ppSkycell.h"

/// Read list of images
static psArray *fileList(const char *filename // Filename
    )
{
    psString input = psSlurpFilename(filename);
    if (!input) {
        psError(PS_ERR_IO, false, "Unable to read %s", filename);
        return false;
    }
    psArray *inputs = psStringSplitArray(input, "\n", false); // Input filenames
    psFree(input);
    if (!inputs || inputs->n == 0) {
        psError(PS_ERR_IO, false, "Unable to read filenames from %s", filename);
        psFree(inputs);
        return NULL;
    }
    return inputs;
}

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


bool ppSkycellCamera(ppSkycellData *data // Run-time data
    )
{
    psArray *images = fileList(data->imagesName); // Image names
    if (!images) {
        psError(psErrorCodeLast(), false, "No images provided.");
        return false;
    }
    data->numInputs = images->n;

    psArray *wcsref = NULL;             // Names of WCS reference images
    if (data->wcsrefName) {
      wcsref = fileList(data->wcsrefName);
      if (!wcsref) {
	psError(psErrorCodeLast(), false, "No WCSrefs provided.");
	psFree(images);
	return(false);
      }
      if (wcsref->n != data->numInputs) {
	psError(PS_ERR_BAD_PARAMETER_SIZE, true, "Number of images (%ld) and wcsrefs (%ld) do not match",
		images->n, wcsref->n);
	psFree(images);
	psFree(wcsref);
	return(false);
      }
    }
    
    psMetadataAddStr(data->config->arguments, PS_LIST_TAIL, "OUTPUT", 0, "Output root", data->outRoot);

    for (int i = 0; i < data->numInputs; i++) {
        bool status = false;             // Status of file definition
        fileArguments("IMAGE", images->data[i], "Name of the image", data->config);
        pmFPAfile *image = pmFPAfileDefineFromArgs(&status, data->config, "PPSKYCELL.IMAGE", "IMAGE"); // File
        if (!status || !image) {
            psError(PS_ERR_IO, false, "Failed to build file from PPSKYCELL.IMAGE");
            // XXX Cleanup
            return false;
        }

	if (data->wcsrefName) {
	  fileArguments("WCSREF", wcsref->data[i], "Name of the WCS reference", data->config);
	  pmFPAfile *wcsref = pmFPAfileDefineFromArgs(&status, data->config, "PPSKYCELL.WCSREF", "WCSREF");
	  if (!status || !wcsref) {
	    psError(PS_ERR_IO, false, "Failed to build file from PPSKYCELL.WCSREF");
	    return(false);
	  }
	}
    }

    if (!pmFPAfileDefineOutput(data->config, NULL, "PPSKYCELL.JPEG1")) {
        psError(psErrorCodeLast(), false, "Unable to define output.");
        return false;
    }
    if (!pmFPAfileDefineOutput(data->config, NULL, "PPSKYCELL.JPEG2")) {
        psError(psErrorCodeLast(), false, "Unable to define output.");
        return false;
    }
    pmFPAfile *bin1 = pmFPAfileDefineOutput(data->config, NULL, "PPSKYCELL.BIN1");
    if (!bin1) {
        psError(psErrorCodeLast(), false, "Unable to define output.");
        return false;
    }
    pmFPAfile *bin2 = pmFPAfileDefineOutput(data->config, NULL, "PPSKYCELL.BIN2");
    if (!bin2) {
        psError(psErrorCodeLast(), false, "Unable to define output.");
        return false;
    }

    // Now the camera has been determined, we can read the recipe
    psMetadata *recipe = psMetadataLookupMetadata(NULL, data->config->recipes, PPSKYCELL_RECIPE); // Recipe
    if (!recipe) {
        psError(PS_ERR_UNEXPECTED_NULL, false, "Unable to find recipe %s", PPSKYCELL_RECIPE);
        return false;
    }

    psString maskString = psMetadataLookupStr(NULL, recipe, "MASKVAL"); // Mask values
    data->maskVal = pmConfigMaskGet(maskString, data->config);
    data->bin1 = psMetadataLookupS32(NULL, recipe, "BIN1");
    data->bin2 = psMetadataLookupS32(NULL, recipe, "BIN2");

    data->doFits = psMetadataLookupBool(NULL, recipe, "MAKEFITS");
    
    if (data->bin1 <= 0 || data->bin2 <= 0) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Unable to find legitimate values for BIN1 and BIN2");
        return false;
    }

    return true;
}
