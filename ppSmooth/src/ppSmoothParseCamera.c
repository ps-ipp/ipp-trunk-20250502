# include "ppSmooth.h"

bool ppSmoothParseCamera(pmConfig *config)
{
    bool status = false;

    if (!ppSmoothDefineFile(config, NULL, "PPSMOOTH.INPUT", "INPUT", PM_FPA_FILE_IMAGE, PM_DETREND_TYPE_NONE)) {
        psError(PS_ERR_IO, false, "Can't find an input image source");
        return NULL;
    }
    pmFPAfile *input = psMetadataLookupPtr(NULL, config->files, "PPSMOOTH.INPUT"); // Input file
    psAssert(input, "We just put it there!");

    // if MASK or VARIANCE was supplied on command line, bind files to 'PPSMOOTH.INPUT'
    pmFPAfile *inputMask = pmFPAfileBindFromArgs (&status, input, config, "PPSMOOTH.INPUT.MASK", "MASK");
    if (!status) {
        psError (PS_ERR_UNKNOWN, false, "failed to load find definition");
        return NULL;
    }
    pmFPAfile *inputVariance = pmFPAfileBindFromArgs (&status, input, config, "PPSMOOTH.INPUT.VARIANCE", "VARIANCE");
    if (!status) {
        psError (PS_ERR_UNKNOWN, false, "failed to load find definition");
        return NULL;
    }

    // if (!psphotSetMaskBits (config)) {
    //     psError (PS_ERR_UNKNOWN, false, "failed to set mask bit values");
    //     return NULL;
    // }

    // XXX add if we add threading:
    // psMetadata *recipe  = psMetadataLookupPtr(&status, config->recipes, RECIPE_NAME);
    // int nThreads = psMetadataLookupS32(&status, config->arguments, "NTHREADS");
    // if (nThreads > 0) {
    //     int nScanRows = psMetadataLookupS32(&status, recipe, "SCAN.ROWS");
    //     pmDetrendSetThreadTasks(nScanRows);
    // }

    // the following files are output targets
    pmFPAfile *outImage = pmFPAfileDefineOutput(config, input->fpa, "PPSMOOTH.OUTPUT");
    if (!outImage) {
        psError(PS_ERR_IO, false, _("Unable to generate output file from PPSMOOTH.OUTPUT"));
        return NULL;
    }
    if (outImage->type != PM_FPA_FILE_IMAGE) {
        psError(PS_ERR_IO, true, "PPSMOOTH.OUTPUT is not of type IMAGE");
        return NULL;
    }
    pmFPAfile *outMask = pmFPAfileDefineOutput(config, input->fpa, "PPSMOOTH.OUTPUT.MASK");
    if (!outMask) {
        psError(PS_ERR_IO, false, _("Unable to generate output file from PPSMOOTH.OUTPUT.MASK"));
        return NULL;
    }
    if (outMask->type != PM_FPA_FILE_MASK) {
        psError(PS_ERR_IO, true, "PPSMOOTH.OUTPUT.MASK is not of type MASK");
        return NULL;
    }
    pmFPAfile *outVariance = pmFPAfileDefineOutput(config, input->fpa, "PPSMOOTH.OUTPUT.VARIANCE");
    if (!outVariance) {
        psError(PS_ERR_IO, false, _("Unable to generate output file from PPSMOOTH.OUTPUT.VARIANCE"));
        return NULL;
    }
    if (outVariance->type != PM_FPA_FILE_VARIANCE) {
        psError(PS_ERR_IO, true, "PPSMOOTH.OUTPUT.VARIANCE is not of type VARIANCE");
        return NULL;
    }

    // save all of these files
    outImage->save     = true;
    outMask->save      = true;
    outVariance->save  = true;

    // the input data is the same as the outImage data : force the free levels to match
    input->freeLevel = PS_MIN(outImage->freeLevel, input->freeLevel);
    inputMask->freeLevel = PS_MIN(outMask->freeLevel, inputMask->freeLevel);
    inputVariance->freeLevel = PS_MIN(outVariance->freeLevel, inputVariance->freeLevel);

    // Chip selection: turn on only the chips specified (pass status to suppress missing-key log msg)
    char *chipLine = psMetadataLookupStr(&status, config->arguments, "CHIP_SELECTIONS");
    psArray *chips = psStringSplitArray(chipLine, ",", false);
    if (chips->n > 0) {
        pmFPASelectChip (input->fpa, -1, true); // deselect all chips
        for (int i = 0; i < chips->n; i++) {
            int chipNum = atoi(chips->data[i]);
            if (! pmFPASelectChip(input->fpa, chipNum, false)) {
                psError(PS_ERR_IO, false, "Chip number %d doesn't exist in camera.\n", chipNum);
                return false;
            }
        }
    }
    psFree (chips);

    if (psTraceGetLevel("ppSmooth.config") > 0) {
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
