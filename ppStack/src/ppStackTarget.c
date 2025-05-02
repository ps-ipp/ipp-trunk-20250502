#include "ppStack.h"

#define SOURCE_MASK (PM_SOURCE_MODE_FAIL | PM_SOURCE_MODE_DEFECT | PM_SOURCE_MODE_SATURATED | \
                     PM_SOURCE_MODE_CR_LIMIT | PM_SOURCE_MODE_EXT_LIMIT) // Mask to apply to input sources

// Get coordinates from a source
static void coordsFromSource(float *x, float *y, float *mag, const pmSource *source)
{
    assert(x && y);
    assert(source);

    if (source->modelPSF) {
        *x = source->modelPSF->params->data.F32[PM_PAR_XPOS];
        *y = source->modelPSF->params->data.F32[PM_PAR_YPOS];
    } else {
        *x = source->peak->xf;
        *y = source->peak->yf;
    }
    if (mag) {
        *mag = source->psfMag;
    }
    return;
}

// Filter a list of sources to exclude sources with near neighbours
static psArray *stackSourcesFilter(psArray *sources, // Source list to filter
                                   int exclusion, // Exclusion zone, pixels
                                   float minMagDiff // Minimum magnitude difference
    )
{
    psAssert(sources && sources->n > 0, "Require array of sources");
    if (exclusion <= 0) {
        return psMemIncrRefCounter(sources);
    }
    exclusion = 2;

    int num = sources->n;               // Number of sources
    psVector *x = psVectorAlloc(num, PS_TYPE_F32), *y = psVectorAlloc(num, PS_TYPE_F32); // Coordinates
    psVector *mag = psVectorAlloc(num, PS_TYPE_F32);                                     // Magnitudes
    int numGood = 0;                    // Number of good sources
    for (int i = 0; i < num; i++) {
        pmSource *source = sources->data[i]; // Source of interest
        if (!source) {
            continue;
        }
        coordsFromSource(&x->data.F32[numGood], &y->data.F32[numGood], &mag->data.F32[numGood], source);
        numGood++;
    }
    x->n = y->n = mag->n = numGood;

    psTree *tree = psTreePlant(2, 2, PS_TREE_EUCLIDEAN, x, y); // kd tree

    psArray *filtered = psArrayAllocEmpty(numGood); // Filtered list of sources
    psVector *coords = psVectorAlloc(2, PS_TYPE_F64); // Coordinates of source
    int numFiltered = 0;                // Number of filtered sources
    for (int i = 0; i < num; i++) {
        pmSource *source = sources->data[i]; // Source of interest
        if (!source) {
            continue;
        }
        float xSource, ySource;         // Coordinates of source
	float Smag;                      // magnitude of source
        coordsFromSource(&xSource, &ySource, &Smag, source);

        coords->data.F64[0] = xSource;
        coords->data.F64[1] = ySource;

        psVector *indices = psTreeAllWithin(tree, coords, exclusion); // Number within exclusion zone
        psTrace("ppStack", 5, "Filtering: Source at %.0lf %.0lf with mag %f has %ld sources in exclusion zone",
                coords->data.F64[0], coords->data.F64[1], Smag, indices->n);
        if (indices->n == 1) {
            // Only itself inside the exclusion zone
            filtered = psArrayAdd(filtered, filtered->n, source);
        } else {
            float inMag = mag->data.F32[i]; // Input magnitude
            bool filter = false;        // Filter this source?
            for (int j = 0; j < indices->n; j++) {
                long index = indices->data.S64[j]; // Index of matching source
                float compareMag = mag->data.F32[index]; // Magnitude of matching source
                if (fabsf(inMag - compareMag) < minMagDiff) {
                    filter = true;
                    break;
                }
            }
            if (!filter) {
                filtered = psArrayAdd(filtered, filtered->n, source);
            } else {
                numFiltered++;
            }
        }
    }
    psFree(coords);
    psFree(tree);
    psFree(x);
    psFree(y);
    psFree(mag);

    psLogMsg("ppStack", PS_LOG_INFO, "Filtered out %d of %d sources", numFiltered, numGood);

    return filtered;
}


psImage *ppStackTarget(ppStackOptions *options, pmConfig *config)
{
    psAssert(options, "Require options");
    psAssert(config, "Require configuration");

    psAssert(options->psf, "Require target PSF");
    psAssert(options->sourceLists, "Require source lists");

    psMetadata *recipe = psMetadataLookupMetadata(NULL, config->recipes, PPSTACK_RECIPE); // ppStack recipe
    psAssert(recipe, "We've thrown an error on this before.");
    psMetadata *ppsub = psMetadataLookupMetadata(NULL, config->recipes, "PPSUB"); // PPSUB recipe
    psAssert(recipe, "We've thrown an error on this before.");

    psString maskValStr = psMetadataLookupStr(NULL, recipe, "MASK.VAL"); // Name of input bits to mask
    psImageMaskType maskVal = pmConfigMaskGet(maskValStr, config); // Input bits to mask
    float targetFrac = psMetadataLookupF32(NULL, recipe, "TARGET.FRAC"); // Target min flux fraction of noise

    int num = options->num;             // Number of inputs
    int numCols = 0, numRows = 0;       // Size of image

    psRandom *rng = psRandomAlloc(PS_RANDOM_TAUS); // Random number generator
    float minVariance = INFINITY;       // Minimum variance
    for (int i = 0; i < num; i++) {
        if (options->inputMask->data.U8[i]) {
            continue;
        }
        psTrace("ppStack", 2, "Characterising image %d....\n", i);
        pmFPAfileActivate(config->files, false, NULL);
        ppStackFileActivationSingle(config, PPSTACK_FILES_TARGET, true, i);

        pmFPAfile *file = pmFPAfileSelectSingle(config->files, "PPSTACK.INPUT.VARIANCE", i); // File to read
        pmFPAview *view = ppStackFilesIterateDown(config);
        if (!view) {
            psFree(rng);
            return NULL;
        }
        pmReadout *readout = pmFPAviewThisReadout(view, file->fpa); // Input readout
        psFree(view);

        if (numCols == 0 && numRows == 0) {
            numCols = readout->variance->numCols;
            numRows = readout->variance->numRows;
        } else if (numCols != readout->variance->numCols ||
                   numRows != readout->variance->numRows) {
            psError(PPSTACK_ERR_ARGUMENTS, true, "Sizes of input images don't match: %dx%d vs %dx%d",
                    readout->variance->numCols, readout->variance->numRows, numCols, numRows);
            psFree(rng);
            return NULL;
        }

        psImage *variance = readout->variance, *mask = readout->mask; // Dereference images
        for (int y = 0; y < numRows; y++) {
            for (int x = 0; x < numCols; x++) {
                if (!isfinite(variance->data.F32[y][x])) {
                    mask->data.PS_TYPE_IMAGE_MASK_DATA[y][x] |= maskVal;
                }
            }
        }

        psStats *bg = psStatsAlloc(PS_STAT_ROBUST_MEDIAN); // Statistics for background
        float mean = NAN;                                  // Measured mean variance
        if (!psImageBackground(bg, NULL, variance, mask, maskVal, rng)) {
            psErrorClear();
            // Retry using all the available pixels
            bg->nSubsample = variance->numCols * variance->numRows + 1;
            if (!psImageStats(bg, variance, mask, maskVal)) {
                psLogMsg("ppStack", PS_LOG_DETAIL,
                         "Couldn't measure mean variance for image %d; retrying.", i);
                psErrorClear();
                // Retry with desperate statistic
                bg->options = PS_STAT_SAMPLE_MEAN;
                if (!psImageStats(bg, variance, mask, maskVal)) {
                    psWarning("Unable to measure mean variance for image %d --- rejecting.", i);
                    psErrorStackPrint(stderr, "Unable to measure mean variance for image %d", i);
                    options->inputMask->data.PS_TYPE_VECTOR_MASK_DATA[i] |= PPSTACK_MASK_PSF;
                    goto DONE;
                } else {
                    // Desperate retry
                    mean = bg->sampleMean;
                }
            } else {
                // Retry with all available pixels
                mean = bg->robustMedian;
            }
        } else {
            // First attempt
            mean = bg->robustMedian;
        }

        float norm = powf(10.0, -0.4 * options->norm->data.F32[i]); // Normalisation from stars
        float meanVariance = mean * PS_SQR(norm);       // Mean variance in normalised image

        if (meanVariance < minVariance) {
            minVariance = meanVariance;
        }

    DONE:
        psFree(bg);
        if (!ppStackFilesIterateUp(config)) {
            psFree(rng);
            return NULL;
        }

        ppStackMemDump("target");
    }
    psFree(rng);

    float minFlux = targetFrac * sqrtf(minVariance); // Minimum flux for target image

    int footprint = psMetadataLookupS32(NULL, ppsub, "STAMP.FOOTPRINT"); // Stamp half-size
    int size = psMetadataLookupS32(NULL, ppsub, "KERNEL.SIZE"); // Kernel half-size
    float minMagDiff = psMetadataLookupF32(NULL, recipe, "TARGET.MINMAG"); // Minimum magnitude difference

    // For the sake of stamps, remove nearby sources
    psArray *stampSources = stackSourcesFilter(options->sources, footprint, minMagDiff); // Filtered list

    bool oldThreads = pmReadoutFakeThreads(true); // Old threading state
    pmReadout *fake = pmReadoutAlloc(NULL); // Fake readout with target PSF
    if (!pmReadoutFakeFromSources(fake, numCols, numRows, stampSources, SOURCE_MASK, NULL, NULL, options->psf,
                                  minFlux, footprint + size, false, true)) {
        psError(PPSTACK_ERR_DATA, false, "Unable to generate fake image with target PSF.");
        psFree(fake);
        return NULL;
    }
    pmReadoutFakeThreads(oldThreads);

    psFree(stampSources);

    psImage *target = psMemIncrRefCounter(fake->image);
    psFree(fake);

    return target;
}
