# include "ppNoiseMap.h"

bool ppNoiseMapParseCamera(pmConfig *config) {

    bool status;

    if (!ppNoiseMapDefineFile(config, NULL, "PPNOISEMAP.INPUT", "INPUT", PM_FPA_FILE_IMAGE)) {
        psError(PS_ERR_IO, false, "Can't find an input image source");
        return NULL;
    }
    pmFPAfile *input = psMetadataLookupPtr(NULL, config->files, "PPNOISEMAP.INPUT"); // Input file
    psAssert(input, "We just put it there!");

    // the recipe is only fully parsed after the camera is first determined
    psMetadata *recipe  = psMetadataLookupPtr(&status, config->recipes, RECIPE_NAME);

    int xBin1 = psMetadataLookupS32(&status, recipe, "XBIN");
    int yBin1 = psMetadataLookupS32(&status, recipe, "YBIN");

    // this generates an output that maps to the same input pixels:
    // pmFPAfile *outImage = pmFPAfileDefineOutput(config, input->fpa, "PPNOISEMAP.OUTPUT");

    // this output results in new pixels with binning:
    pmFPAfile *outImage = pmFPAfileDefineFromFPA(config, input->fpa, xBin1, yBin1, "PPNOISEMAP.OUTPUT");

    if (!outImage) {
        psError(PS_ERR_IO, false, _("Unable to generate output file from PPNOISEMAP.OUTPUT"));
        return NULL;
    }
    if (outImage->type != PM_FPA_FILE_IMAGE) {
        psError(PS_ERR_IO, true, "PPNOISEMAP.OUTPUT is not of type IMAGE");
        return NULL;
    }
    outImage->save = true;

    // outImage is used as a carrier: input to chipImage -> require the data to remain at the CHIP level
    outImage->freeLevel = PS_MIN(outImage->freeLevel, PM_FPA_LEVEL_CHIP);
    outImage->dataLevel = outImage->freeLevel;
    outImage->fileLevel = PS_MIN(outImage->fileLevel, outImage->dataLevel);

    // the input data is the same as the outImage data : force the free levels to match
    input->freeLevel = PS_MIN(outImage->freeLevel, input->freeLevel);

    if (psTraceGetLevel("ppNoiseMap.config") > 0) {
        // Get a look inside all the files.
        psMetadataIterator *filesIter = psMetadataIteratorAlloc(config->files, PS_LIST_HEAD, NULL);
        psMetadataItem *item;               // Item from iteration
        fprintf(stderr, "Files:\n");
        while ((item = psMetadataGetAndIncrement(filesIter))) {
            pmFPAfile *file = item->data.V; // File of interest
            fprintf(stderr, "%s: %p %p %p (%p) %p\n", file->name,
                    file->src, file->fpa,
                    file->camera, file->fpa->camera, file->format);
        }
        psFree(filesIter);
    }

    return true;
}
