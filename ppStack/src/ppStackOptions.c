#include "ppStack.h"

static void stackOptionsFree(ppStackOptions *options)
{
    psFree(options->stats);
    if (options->statsFile) {
        fclose(options->statsFile);
    }
    psFree(options->origImages);
    psFree(options->origMasks);
    psFree(options->origVariances);
    psFree(options->origCovars);
    psFree(options->convImages);
    psFree(options->convMasks);
    psFree(options->convVariances);
    psFree(options->psf);
    psFree(options->inputSeeing);
    psFree(options->exposures);
    psFree(options->inputMask);
    psFree(options->sourceLists);
    psFree(options->bscaleOffset);
    psFree(options->norm);
    psFree(options->sources);
    psFree(options->cells);
    psFree(options->kernels);
    psFree(options->regions);
    psFree(options->matchChi2);
    psFree(options->weightings);
    psFree(options->convCovars);
    psFree(options->outRO);
    psFree(options->expRO);
    psFree(options->inspect);
    psFree(options->rejected);

    return;
}

ppStackOptions *ppStackOptionsAlloc(void)
{
    ppStackOptions *options = psAlloc(sizeof(ppStackOptions)); // Options, to return
    psMemSetDeallocator(options, (psFreeFunc)stackOptionsFree);

    options->convolve = true;
    options->matchZPs = true;
    options->photometry = false;
    options->doBackground = false;
    options->stats = NULL;
    options->statsFile = NULL;
    options->origImages = NULL;
    options->origMasks = NULL;
    options->origVariances = NULL;
    options->origCovars = NULL;
    options->convImages = NULL;
    options->convMasks = NULL;
    options->convVariances = NULL;
    options->num = 0;
    options->quality = 0;

    options->clipPercent = false;

    options->psf = NULL;
    options->sumExposure = NAN;
    options->zp = NAN;
    options->inputSeeing = NULL;
    options->exposures = NULL;
    options->targetSeeing = NAN;
    options->clippedMean = NAN;	
    options->clippedStdev = NAN;
    options->inputMask = NULL;
    options->sourceLists = NULL;
    options->bscaleOffset = NULL;
    options->norm = NULL;
    options->sources = NULL;
    options->cells = NULL;
    options->kernels = NULL;
    options->regions = NULL;
    options->numCols = 0;
    options->numRows = 0;
    options->matchChi2 = NULL;
    options->weightings = NULL;
    options->convCovars = NULL;
    options->outRO = NULL;
    options->expRO = NULL;
    options->inspect = NULL;
    options->rejected = NULL;

    return options;
}
