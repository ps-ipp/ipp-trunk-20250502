#include "ppStack.h"

#define BUFFER 16                       // Buffer for name array

bool ppStackSetup(ppStackOptions *options, pmConfig *config)
{
    psAssert(options, "Require options");
    psAssert(config, "Require configuration");

    psMetadata *recipe = psMetadataLookupMetadata(NULL, config->recipes, PPSTACK_RECIPE); // ppStack recipe
    psAssert(recipe, "We've thrown an error on this before.");

    // XXX : switch to this name? options->matchZPs = psMetadataLookupBool(NULL, recipe, "MATCH.ZERO.POINTS"); // Adjust zero points based on tranparency analysis?
    options->matchZPs = psMetadataLookupBool(NULL, recipe, "ZP"); // Adjust zero points?

    options->photometry = psMetadataLookupBool(NULL, recipe, "PHOTOMETRY"); // Perform photometry?

    options->convolve = psMetadataLookupBool(NULL, recipe, "CONVOLVE"); // Convolve images?
    options->doBackground = psMetadataLookupBool(NULL, recipe, "BACKGROUND.MODEL");

    options->clipPercent  = psMetadataLookupBool(NULL, recipe, "COMBINE.PERCENT"); // use percentile range to clip?

    if (!psMetadataLookupBool(NULL, config->arguments, "HAVE.PSF")) {
        psWarning("No PSFs provided --- unable to convolve to common PSF.");
        options->convolve = false;
    }

    int num = psMetadataLookupS32(NULL, config->arguments, "INPUTS.NUM"); // Number of inputs
    options->num = num;

    bool mdok;                          // Status of MD lookup
    const char *statsName = psMetadataLookupStr(&mdok, config->arguments, "STATS"); // Filename for statistics
    if (statsName && strlen(statsName) > 0) {
        psString resolved = pmConfigConvertFilename(statsName, config, true, true); // Resolved filename
        options->statsFile = fopen(resolved, "w");
        if (!options->statsFile) {
            psError(PPSTACK_ERR_IO, true, "Unable to open statistics file %s for writing.\n", resolved);
            psFree(resolved);
            return false;
        }
        psFree(resolved);
        options->stats = psMetadataAlloc();
    }

    // Generate temporary names for convolved images
    const char *tempDir = psMetadataLookupStr(NULL, config->arguments, "-temp-dir"); // Directory for temps
    if (!tempDir) {
        tempDir = psMetadataLookupStr(NULL, config->site, "TEMP.DIR");
    }
    if (!tempDir) {
        psError(PPSTACK_ERR_CONFIG, false, "Unable to find TEMP.DIR in site configuration");
        return false;
    }

    psString outputName = psStringCopy(psMetadataLookupStr(NULL, config->arguments,
                                                           "OUTPUT")); // Name for temporary files
    const char *tempName = psStringFileBasename(outputName);
    if (!tempName) {
        psError(PPSTACK_ERR_ARGUMENTS, false, "Unable to construct basename for temporary files.");
        psFree(outputName);
        return false;
    }

    const char *tempImage = psMetadataLookupStr(NULL, recipe, "TEMP.IMAGE"); // Suffix for images
    const char *tempMask = psMetadataLookupStr(NULL, recipe, "TEMP.MASK"); // Suffix for masks
    const char *tempVariance = psMetadataLookupStr(NULL, recipe, "TEMP.VARIANCE"); // Suffix for var maps
    if (!tempImage || !tempMask || !tempVariance) {
        psError(PPSTACK_ERR_CONFIG, false,
                "Unable to find TEMP.IMAGE, TEMP.MASK and TEMP.VARIANCE in recipe");
        psFree(outputName);
	psFree(tempName);
        return false;
    }

    options->convImages = psArrayAlloc(num);
    options->convMasks = psArrayAlloc(num);
    options->convVariances = psArrayAlloc(num);
    for (int i = 0; i < num; i++) {
        psString imageName = NULL, maskName = NULL, varianceName = NULL; // Names for convolved images
        psStringAppend(&imageName, "%s/%s.%d.%s", tempDir, tempName, i, tempImage);
        psStringAppend(&maskName, "%s/%s.%d.%s", tempDir, tempName, i, tempMask);
        psStringAppend(&varianceName, "%s/%s.%d.%s", tempDir, tempName, i, tempVariance);
        psTrace("ppStack", 5, "Temporary files: %s %s %s\n", imageName, maskName, varianceName);
        options->convImages->data[i] = imageName;
        options->convMasks->data[i] = maskName;
        options->convVariances->data[i] = varianceName;
    }
    psFree(outputName);
    psFree(tempName);

    // Original images
    options->origImages = psArrayAlloc(num);
    options->origMasks = psArrayAlloc(num);
    options->origVariances = psArrayAlloc(num);
    options->bkgImages = psArrayAlloc(num);
    pmFPAview *view = pmFPAviewAlloc(0);
    int nullMasks = 0;
    int nullVariances = 0;
    for (int i = 0; i < num; i++) {
        {
            pmFPAfile *file = pmFPAfileSelectSingle(config->files, "PPSTACK.INPUT", i);
            options->origImages->data[i] = pmFPAfileName(file, view, config);
        }
        {
            // We want the convolved mask, since that defines the area that has been tested for outliers
	  if (options->convolve) {
            options->origMasks->data[i] = psMemIncrRefCounter(options->convMasks->data[i]);
	  }
	  else {
	    pmFPAfile *file = pmFPAfileSelectSingle(config->files, "PPSTACK.INPUT.MASK", i);
	    options->origMasks->data[i] = pmFPAfileName(file, view, config);
	  }
	  if (!(options->origMasks->data[i])) {
	    nullMasks++;
	  }
        }
        {
            pmFPAfile *file = pmFPAfileSelectSingle(config->files, "PPSTACK.INPUT.VARIANCE", i);
            options->origVariances->data[i] = pmFPAfileName(file, view, config);
	    if (!(options->origVariances->data[i])) {
	      nullVariances++;
	    }
        }
/* 	{ */
/* 	  pmFPAfile *file = pmFPAfileSelectSingle(config->files, "PPSTACK.INPUT.BKGMODEL", i); */
/* 	  options->bkgImages->data[i] = pmFPAfileName(file, view, config); */
/* 	} */
    }
    if (nullMasks == num) {
      psFree(options->origMasks);
    }
    if (nullVariances == num) {
      psFree(options->origVariances);
    }
    
    psFree(view);

    if (!pmConfigMaskSetBits(NULL, NULL, config)) {
        psError(PPSTACK_ERR_CONFIG, false, "Unable to determine mask value.");
        return false;
    }

    return true;
}
