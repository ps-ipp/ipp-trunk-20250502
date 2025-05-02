# include "psphotInternal.h"
static int npass = 0;
static char *defaultStatsName = "FITTED_MEAN";

// Determine the binning characteristics for the background model
// Sets the ruff image size and skip based on recipe values for the binning
psImageBinning *psphotBackgroundBinning(const psImage *image, // Image for which to generate a bg model
                                         const pmConfig *config // Configuration
                                         )
{
    bool status = true;

    // select the appropriate recipe information
    psMetadata *recipe  = psMetadataLookupPtr (&status, config->recipes, PSPHOT_RECIPE);
    assert (recipe);

    // I have the fine image size, I know the binning factor, determine the ruff image size
    psImageBinning *binning = psImageBinningAlloc();
    binning->nXfine = image->numCols;
    binning->nYfine = image->numRows;
    binning->nXbin  = psMetadataLookupS32(&status, recipe, "BACKGROUND.XBIN");
    binning->nYbin  = psMetadataLookupS32(&status, recipe, "BACKGROUND.YBIN");

    psImageBinningSetRuffSize(binning, PS_IMAGE_BINNING_CENTER);
    psImageBinningSetSkip(binning, image);

    return binning;
}

// track background cell failures for log reporting purposes
static int nFailures = 0;

// Generate the background model
// generate the median in NxN boxes, clipping heavily
// linear interpolation to generate full-scale model
//
// NOTE that the 'analysis' metedata passed in here is used to store the binning information.
// This may be the analysis for this readout, but it may be the analysis for the pmFPAfile
// corresponding to the model.  Other information about the background model is saved on the
// readout->analysis
bool psphotModelBackgroundReadout(psImage *model,  // Model image
				  psImage *modelStdev, // Model stdev image
				  psMetadata *analysis, // Analysis metadata for outputs
				  pmReadout *readout, // Readout for which to generate a background model
				  psImageBinning *binning, // Binning parameters
				  const pmConfig *config,// Configuration
				  bool useVarianceImage
    )
{
    psTimerStart ("psphot.background");

    bool status = true;

    psImage *image = useVarianceImage ? readout->variance : readout->image;
    psImage *mask = readout->mask; // Image and mask for readout

    // select the appropriate recipe information
    psMetadata *recipe  = psMetadataLookupPtr (&status, config->recipes, PSPHOT_RECIPE);
    assert (recipe);

    // user-defined masks to test for good/bad pixels (build from recipe list if not yet set)
    psImageMaskType maskVal = psMetadataLookupImageMask(&status, recipe, "MASK.PSPHOT"); // Mask value for bad pixels
    assert (maskVal);

    // mark pixel may be used for optional iteration -- is not required by all processes
    psImageMaskType markVal = psMetadataLookupImageMask(&status, recipe, "MARK.PSPHOT"); // Mask value for bad pixels

    // apply both MASK and MARK to make background model
    maskVal |= markVal;

    psRandom *rng = psRandomAlloc(PS_RANDOM_TAUS);

    float dXsample = psMetadataLookupF32(&status, recipe, "BACKGROUND.XSAMPLE");
    if (!status) dXsample = 1.0;
    float dYsample = psMetadataLookupF32(&status, recipe, "BACKGROUND.YSAMPLE");
    if (!status) dYsample = 1.0;

    // subtract this amount extra from the sky
    float SKY_BIAS = psMetadataLookupF32 (&status, recipe, "SKY_BIAS");
    if (!status) {
        SKY_BIAS = 0;
    }

    // supply the sky background statistics options
    char *statsName = psMetadataLookupStr (&status, recipe, "SKY_STAT");
    if (statsName == NULL) {
        statsName = defaultStatsName;
    }
    psStatsOptions statsOptionLocation = psStatsOptionFromString(statsName);
    if (!(statsOptionLocation & (PS_STAT_SAMPLE_MEAN |
                                 PS_STAT_SAMPLE_MEDIAN |
                                 PS_STAT_ROBUST_MEDIAN |
                                 PS_STAT_ROBUST_QUARTILE |
                                 PS_STAT_CLIPPED_MEAN |
                                 PS_STAT_FITTED_MEAN))) {
        statsOptionLocation = PS_STAT_FITTED_MEAN;
    }

    psStatsOptions statsOptionWidth = PS_STAT_NONE;
    if (statsOptionLocation & (PS_STAT_SAMPLE_MEAN | PS_STAT_SAMPLE_MEDIAN)) {
        statsOptionWidth = PS_STAT_SAMPLE_STDEV;
    } else if (statsOptionLocation & (PS_STAT_ROBUST_MEDIAN | PS_STAT_ROBUST_QUARTILE)) {
        statsOptionWidth = PS_STAT_ROBUST_STDEV;
    } else if (statsOptionLocation & PS_STAT_FITTED_MEAN) {
        statsOptionWidth = PS_STAT_FITTED_STDEV;
    } else if (statsOptionLocation & PS_STAT_CLIPPED_MEAN) {
        statsOptionWidth = PS_STAT_CLIPPED_STDEV;
    } else {
        psAbort("Unable to estimate variance of selected statsOptionLocations 0x%x", statsOptionLocation);
    }

    const psStatsOptions statsOption = statsOptionLocation | statsOptionWidth | PS_STAT_MIN | PS_STAT_MAX; // use the requested value, but be sure MIN and MAX are calculated;
    psStats *statsDefaults = psStatsAlloc (statsOption);

    // set range for old-version of sky statistic
    if (statsOptionLocation & PS_STAT_ROBUST_QUARTILE) {
        statsDefaults->min = 0.25;
        statsDefaults->max = 0.75;
    }

    // set user-option for number of pixels per region
    statsDefaults->nSubsample = psMetadataLookupF32 (&status, recipe, "IMSTATS_NPIX");
    if (!status) {
        statsDefaults->nSubsample = 1000;
    }

    // optionally set the binsize
    statsDefaults->binsize = psMetadataLookupF32 (&status, recipe, "SKY_HISTOGRAM_BINS");
    if (status) {
        statsDefaults->options |= PS_STAT_USE_BINSIZE;
    }

    // optionally set the sigma clipping
    statsDefaults->clipSigma = psMetadataLookupF32 (&status, recipe, "SKY_CLIP_SIGMA");
    if (!status) {
        if (statsDefaults->options & PS_STAT_FITTED_MEAN) {
            statsDefaults->clipSigma = 1.0;
        } else {
            statsDefaults->clipSigma = 3.0;
        }
    }

    // we save the binning structure for use in psphotMagnitudes
    if (!useVarianceImage) {
	psMetadataAddPtr(analysis, PS_LIST_TAIL, "PSPHOT.BACKGROUND.BINNING", PS_DATA_UNKNOWN | PS_META_REPLACE, "Background binning", binning);
    }

    psVector *dQ = psVectorAllocEmpty (100, PS_TYPE_F32);
    psF32 **modelData = model->data.F32;
    psF32 **modelStdevData = modelStdev->data.F32;

    // XXXX we can thread this here by running blocks in parallel
    psImageBackgroundInit();
    nFailures = 0; // reset for this pass

    // we have Nx * Ny model points, but we can use a window which is larger (or smaller) than
    // 1 superpixel.  If we have a window of size dXsample * dYsample, then the regions run from:
    // (ix + 0.5) - dX to (ix + 0.5) + dX

    // measure clipped median for subimages
    for (int iy = 0; iy < model->numRows; iy++) {
        for (int ix = 0; ix < model->numCols; ix++) {
	    
            // allocate a job -- if threads are not defined, this just runs the job
            psThreadJob *job = psThreadJobAlloc ("PSPHOT_MODEL_BACKGROUND");

            psArrayAdd(job->args, 1, image);
            psArrayAdd(job->args, 1, mask);
            psArrayAdd(job->args, 1, binning);
            psArrayAdd(job->args, 1, rng);
            psArrayAdd(job->args, 1, statsDefaults);

            psArrayAdd(job->args, 1, modelData);
            psArrayAdd(job->args, 1, modelStdevData);

            PS_ARRAY_ADD_SCALAR(job->args, maskVal,  PS_TYPE_IMAGE_MASK);

            PS_ARRAY_ADD_SCALAR(job->args, ix, PS_TYPE_S32);
            PS_ARRAY_ADD_SCALAR(job->args, iy, PS_TYPE_S32);

            PS_ARRAY_ADD_SCALAR(job->args, dXsample, PS_TYPE_F32);
            PS_ARRAY_ADD_SCALAR(job->args, dYsample, PS_TYPE_F32);

            PS_ARRAY_ADD_SCALAR(job->args, statsOptionLocation, PS_TYPE_S32);
            PS_ARRAY_ADD_SCALAR(job->args, statsOptionWidth, PS_TYPE_S32);

            PS_ARRAY_ADD_SCALAR(job->args, NAN, PS_TYPE_F32); // this is used as a return value for dQvalue

# if (1)
            if (!psThreadJobAddPending(job)) {
                psError(PS_ERR_UNKNOWN, false, "Unable to guess model.");
                return NULL;
            }
# else
            if (!psphotModelBackground_Threaded(job)) {
                psError(PS_ERR_UNKNOWN, false, "Failure to model background.");
                return NULL;
            }
	    if (job->args->n < 1) {
		fprintf (stderr, "error with job\n");
	    } else {
		psScalar *scalar = job->args->data[14];
		float dQvalue = scalar->data.F32;
		psVectorAppend (dQ, dQvalue);
	    }
	    psFree(job);
# endif
        }
    }

    // wait for the threads to finish and manage results
    if (!psThreadPoolWait (false, true)) {
	psError(PS_ERR_UNKNOWN, false, "Unable to guess model.");
	return NULL;
    }

    // we have only supplied one type of job, so we can assume the types here
    psThreadJob *job = NULL;
    while ((job = psThreadJobGetDone()) != NULL) {
	if (job->args->n < 1) {
	    fprintf (stderr, "error with job\n");
	} else {
	    psScalar *scalar = job->args->data[14];
	    float dQvalue = scalar->data.F32;
	    psVectorAppend (dQ, dQvalue);
	}
	psFree(job);
    }

    if (nFailures) {
	psLogMsg ("psphot", PS_LOG_WARN, "Failed to estimate background for %d of %d subimages", nFailures, (model->numRows*model->numCols));
    }

    if (psTraceGetLevel("psphot") > 5) {
        char name[256];
        sprintf (name, "backraw.%02d.fits", npass);
        psphotSaveImage (NULL, model, name);
    }

    // patch over bad regions (use average of 8 possible neighbor pixels)
    // XXX consider testing all pixels against the 8 neighbors and replacing outliers...
    double Count = 0;                   // number of good pixels
    double Value = 0;                   // sum of good pixel's value
    double ValueStdev = 0;              // sum of good pixel's standard deviations
    for (int iy = 0; iy < model->numRows; iy++) {
        for (int ix = 0; ix < model->numCols; ix++) {
            if (isfinite(modelData[iy][ix])) {
                Value += modelData[iy][ix];
                ValueStdev += modelStdevData[iy][ix];
                Count++;
                continue;
            }

            double value = 0;
            double count = 0;
            for (int jy = iy - 1; jy <= iy + 1; jy++) {
                if (jy <   0) continue;
                if (jy >= model->numRows) continue;
                for (int jx = ix - 1; jx <= ix + 1; jx++) {
                    if (!jx && !jy) continue;
                    if (jx   <   0) continue;
                    if (jx   >= model->numCols) continue;
		    if (!isfinite(modelData[jy][jx])) continue;
                    value += modelData[jy][jx];
                    count += 1.0;
                }
            }
            if (count > 0) {
		psLogMsg ("psphot", PS_LOG_DETAIL, "patching background %d, %d: %f (%d pts)\n", ix, iy, (value / count), (int) count);
		modelData[iy][ix] = value / count;
	    }
        }
    }
    if (Count == 0) {
        psError (PSPHOT_ERR_DATA, true, "failed to build background image");
        psFree(statsDefaults);
        psFree(binning);
        psFree(rng);
        psFree(dQ);
        return false;
    }

    Value /= Count;
    ValueStdev /= Count;

    // patch over remaining bad regions (use global average)
    for (int iy = 0; iy < model->numRows; iy++) {
        for (int ix = 0; ix < model->numCols; ix++) {
            if (!isnan(modelData[iy][ix])) continue;
            modelData[iy][ix] = Value;
            modelStdevData[iy][ix] = ValueStdev;
        }
    }

    // apply artificial offset if desired
    for (int iy = 0; iy < model->numRows; iy++) {
        for (int ix = 0; ix < model->numCols; ix++) {
            modelData[iy][ix] += SKY_BIAS;
        }
    }

    if (psTraceGetLevel("psphot") > 5) {
        char name[256];
        sprintf (name, "backfill.%02d.fits", npass);
        psphotSaveImage (NULL, model, name);
        sprintf (name, "backfist.%02d.fits", npass);
        psphotSaveImage (NULL, modelStdev, name);
    }

    psLogMsg ("psphot", PS_LOG_WARN, "built background image: %f sec\n", psTimerMark ("psphot.background"));

    psStats *statsDQ = psStatsAlloc (PS_STAT_SAMPLE_MEDIAN);
    psVectorStats (statsDQ, dQ, NULL, NULL, 0);

    if (!useVarianceImage) {
	psMetadataAddF32(readout->analysis, PS_LIST_TAIL, "SKY_MEAN",  PS_META_REPLACE, "sky mean", Value);
	psMetadataAddF32(readout->analysis, PS_LIST_TAIL, "SKY_STDEV", PS_META_REPLACE, "sky stdev", ValueStdev);
	psMetadataAddF32(readout->analysis, PS_LIST_TAIL, "SKY_DQ",    PS_META_REPLACE, "sky quartile slope", statsDQ->sampleMedian);
	psLogMsg ("psphot", PS_LOG_INFO, "image sky : mean %f, stdev %f, dQ %f", Value, ValueStdev, statsDQ->sampleMedian);

	// measure image and background stats and save for later output
	psStats *statsBck = psStatsAlloc (PS_STAT_SAMPLE_MEAN |
					  PS_STAT_SAMPLE_STDEV |
					  PS_STAT_MIN |
					  PS_STAT_MAX);
	psImageStats (statsBck, model, NULL, 0);
	psMetadataAddF32 (readout->analysis, PS_LIST_TAIL, "MSKY_MN",  PS_META_REPLACE, "sky model mean",          statsBck->sampleMean);
	psMetadataAddF32 (readout->analysis, PS_LIST_TAIL, "MSKY_SIG", PS_META_REPLACE, "sky model stdev",         statsBck->sampleStdev);
	psMetadataAddF32 (readout->analysis, PS_LIST_TAIL, "MSKY_DEV", PS_META_REPLACE, "sky stdev",               ValueStdev);
	psMetadataAddF32 (readout->analysis, PS_LIST_TAIL, "MSKY_DQ",  PS_META_REPLACE, "sky quartile slope",      statsDQ->sampleMedian);
	psMetadataAddF32 (readout->analysis, PS_LIST_TAIL, "MSKY_MAX", PS_META_REPLACE, "sky model maximum value", statsBck->max);
	psMetadataAddF32 (readout->analysis, PS_LIST_TAIL, "MSKY_MIN", PS_META_REPLACE, "sky model minimum value", statsBck->min);
	psMetadataAddS32 (readout->analysis, PS_LIST_TAIL, "MSKY_NX", PS_META_REPLACE, "sky model size (x)",      model->numCols);
	psMetadataAddS32 (readout->analysis, PS_LIST_TAIL, "MSKY_NY", PS_META_REPLACE, "sky model size (y)",      model->numRows);
	psLogMsg ("psphot", PS_LOG_INFO, "background sky : min %f mean %f max %f stdev %f",
		  statsBck->min, statsBck->sampleMean, statsBck->max, statsBck->sampleStdev);
	psFree(statsBck);
    } else {
	psLogMsg ("psphot", PS_LOG_INFO, "variance data : mean %f, stdev %f, dQ %f", Value, ValueStdev, statsDQ->sampleMedian);
    }

    psFree(statsDQ);
    psFree(dQ);

    psFree(statsDefaults);
    psFree(binning);
    psFree(rng);

    return model;
}

// generate a background model for a single readout. do not save an associated pmFPAfile for possible output
psImage *psphotModelBackgroundReadoutNoFile(pmReadout *readout, const pmConfig *config)
{
    PM_ASSERT_READOUT_NON_NULL(readout, NULL);
    PM_ASSERT_READOUT_IMAGE(readout, NULL);
    PS_ASSERT_PTR_NON_NULL(config, NULL);

    psImageBinning *binning = psphotBackgroundBinning(readout->image, config); // Image binning parameters
    psImage *model = psImageAlloc(binning->nXruff, binning->nYruff, PS_TYPE_F32); // Background model
    psImage *modelStdev = psImageAlloc(binning->nXruff, binning->nYruff, PS_TYPE_F32); // Standard deviation

    if (!psphotModelBackgroundReadout(model, modelStdev, readout->analysis, readout, binning, config, false)) {
        psFree(model);
        psFree(modelStdev);
        psError(PS_ERR_UNKNOWN, false, "Unable to generate background model");
        return NULL;
    }
    psFree(modelStdev);
    return model;
}

// generate a background model for readout number index; save an associated pmFPAfile for possible output
bool psphotModelBackgroundReadoutFileIndex (pmConfig *config, const pmFPAview *view, const char *filename, int index)
{
    // find the currently selected readout
    pmFPAfile *file = pmFPAfileSelectSingle(config->files, filename, index); // File of interest
    psAssert (file, "missing file?");

    pmFPA *inFPA = file->fpa;
    pmReadout *readout = pmFPAviewThisReadout(view, inFPA);
    psAssert (readout, "missing readout?");

    psImageBinning *binning = psphotBackgroundBinning(readout->image, config); // Image binning parameters
    pmReadout *model = pmFPAGenerateReadout(config, view, psphotGetFilerule("PSPHOT.BACKMDL"), inFPA, binning, index);
    pmReadout *modelStdev = pmFPAGenerateReadout(config, view, psphotGetFilerule("PSPHOT.BACKMDL.STDEV"), inFPA, binning, index);

    if (!psphotModelBackgroundReadout(model->image, modelStdev->image, model->analysis, readout, binning, config, false)) {
        int lastError = psErrorCodeLast();
        if (lastError == PSPHOT_ERR_DATA) {
            // depending on context this value may or may not be used
            psMetadataAddS32 (readout->analysis, PS_LIST_TAIL, "PSPHOT_QUALITY", PS_META_REPLACE, "quality error due to poor data", lastError);
        }
        psError(lastError, false, "Unable to generate background model");
        return false;
    }

    npass ++;
    return true;
}

bool psphotModelBackground (pmConfig *config, const pmFPAview *view, const char *filerule)
{
    int num = psphotFileruleCount(config, filerule);

    fprintf (stdout, "\n");
    psLogMsg ("psphot", PS_LOG_INFO, "--- psphot Model Background ---");

    // loop over the available readouts
    for (int i = 0; i < num; i++) {
        if (!psphotModelBackgroundReadoutFileIndex(config, view, filerule, i)) {
            int lastError = psErrorCodeLast();
	    psError (lastError ? lastError : PSPHOT_ERR_CONFIG, false, "failed to model background for %s entry %d", filerule, i);
            return false;
        }
    }
    return true;
}

bool psphotModelBackground_Threaded (psThreadJob *job) {

    psImage *image          = job->args->data[0];
    psImage *mask           = job->args->data[1];

    psImageBinning *binning = job->args->data[2];

    psRandom *rng           = job->args->data[3];
    psStats *statsDefaults  = job->args->data[4];

    psF32 **modelData       = job->args->data[5];
    psF32 **modelStdevData  = job->args->data[6];

    psImageMaskType maskVal = PS_SCALAR_VALUE(job->args->data[7],PS_TYPE_IMAGE_MASK_DATA);

    int ix                  = PS_SCALAR_VALUE(job->args->data[8],S32);
    int iy                  = PS_SCALAR_VALUE(job->args->data[9],S32);

    float dXsample 	    = PS_SCALAR_VALUE(job->args->data[10],F32);
    float dYsample 	    = PS_SCALAR_VALUE(job->args->data[11],F32);

    psStatsOptions statsOptionLocation = PS_SCALAR_VALUE(job->args->data[12],S32);
    psStatsOptions statsOptionWidth    = PS_SCALAR_VALUE(job->args->data[13],S32);

    // convert the ruff grid cell to the equivalent fine grid cell
    psRegion ruffRegion = psRegionSet (ix + 0.5 - 0.5*dXsample, ix + 0.5 + 0.5*dXsample, iy + 0.5 - 0.5*dYsample, iy + 0.5 + 0.5*dYsample);
    psRegion fineRegion = psImageBinningSetFineRegion (binning, ruffRegion);
    fineRegion = psRegionForImage (image, fineRegion);

    psImage *subset  = psImageSubset (image, fineRegion);
    if (!subset->numCols || !subset->numRows) {
	psFree (subset);
	return false; // XXX do we / should we fail on this?
    }
    psImage *submask = psImageSubset (mask, fineRegion);

    psStats *stats = psStatsAlloc (PS_STAT_NONE);
    *stats = *statsDefaults;

    // Use the selected background statistic for the first pass
    // If it fails, fall back on the "ROBUST_MEDIAN" version
    // If both fail, set the pixel to NAN and (later) interpolate

    psVector *sample = NULL;
    float dQvalue = NAN;

    if (psImageBackground(stats, &sample, subset, submask, maskVal, rng)) {
	if (stats->options & PS_STAT_ROBUST_QUARTILE) {
	    modelData[iy][ix] = stats->robustMedian;
	} else {
	    modelData[iy][ix] = psStatsGetValue(stats, statsOptionLocation);
	}
	modelStdevData[iy][ix] = psStatsGetValue(stats, statsOptionWidth);

	// fprintf (stderr, "background stats: %d, %d : %f > %f > %f > %f > %f : %f\n", ix, iy, stats->min, stats->robustLQ, modelData[iy][ix], stats->robustUQ, stats->max, modelStdevData[iy][ix]);
	// XXX this operation is not thread safe -- move out somehow...
	// psVectorAppend (dQ, stats->robustUQ + stats->robustLQ - 2*stats->robustMedian);
	dQvalue = stats->robustUQ + stats->robustLQ - 2*stats->robustMedian;
	// return dQvalue to main thread

	// supply sample to plotting routing
	// only allow in a non-threaded context -- can plot more than one cell
	// psphotDiagnosticPlots (config, "IMAGE.BACKGROUND.CELL.HISTOGRAM", ix, iy, modelData[iy][ix], modelStdevData[iy][ix], sample);
    } else {
	// psStatsOptions currentOptions = stats->options;
	stats->options = PS_STAT_ROBUST_MEDIAN | PS_STAT_ROBUST_STDEV;
	if (!psImageBackground(stats, &sample, subset, submask, maskVal, rng)) {
	    if ((nFailures < 3) || (nFailures % 100 == 0)) {
		psLogMsg ("psphot", PS_LOG_WARN, "Failed to estimate background using ROBUST_MEDIAN for "
			  "(%dx%d, (row0,col0) = (%d,%d)",
			  subset->numRows, subset->numCols, subset->row0, subset->col0);
	    }
	    nFailures ++; // static
	    modelData[iy][ix] = modelStdevData[iy][ix] = NAN;
	} else {
	    modelData[iy][ix] = psStatsGetValue (stats, PS_STAT_ROBUST_MEDIAN);
	    modelStdevData[iy][ix] = psStatsGetValue(stats, PS_STAT_ROBUST_STDEV);

	    // supply sample to plotting routing
	    // only allow in a non-threaded context -- can plot more than one cell
	    // psphotDiagnosticPlots (config, "IMAGE.BACKGROUND.CELL.HISTOGRAM", ix, iy, modelData[iy][ix], modelStdevData[iy][ix], sample);
	}
	// drop errors caused by psImageBackground failures
	// NOTE : psStats raises errors when it cannot find a solution; this 
	// probably should not be errors but exit stats info only, but c'est la code
	// XXX we probably should trap and exit on serious failures
	psErrorClear();
	// stats->options = currentOptions; // this is not needed (set by init above)
    }
    psFree (stats);
    psFree (sample);
    psFree (subset);
    psFree (submask);

    // return the dQvalue to the calling thread
    psScalar *scalar = job->args->data[14];
    scalar->data.F32 = dQvalue;

    return true;
}

// load a background model for readout number index; save an associated pmFPAfile for possible output
bool psphotLoadBackgroundModelReadoutFileIndex (pmConfig *config, const pmFPAview *view, const char *filename, int index)
{
    // find the currently selected readout
    pmFPAfile *file = pmFPAfileSelectSingle(config->files, filename, index); // File of interest
    psAssert (file, "missing file?");

    pmFPA *inFPA = file->fpa;
    pmReadout *readout = pmFPAviewThisReadout(view, inFPA);
    psAssert (readout, "missing readout?");

    // Set up the background model file
    // Here we are assuming that the recipe being used is the same as when the original analysis that generated
    // the background model was done
    psImageBinning *binning = psphotBackgroundBinning(readout->image, config); // Image binning parameters
    psMetadataAddPtr(readout->analysis, PS_LIST_TAIL, "PSPHOT.BACKGROUND.BINNING", PS_DATA_UNKNOWN | PS_META_REPLACE, "Background binning", binning);
    psFree(binning);
    pmReadout *model = pmFPAGenerateReadout(config, view, psphotGetFilerule("PSPHOT.BACKMDL"), inFPA, binning, index);
    pmReadout *modelStdev = pmFPAGenerateReadout(config, view, psphotGetFilerule("PSPHOT.BACKMDL.STDEV"), inFPA, binning, index);

    // Copy the image from the input model file to the model file
    // XXX: Check for nulls and that the image sizes match
    pmFPAfile *bgFile = pmFPAfileSelectSingle(config->files, "PSPHOT.STACK.BACKMDL.RAW", 0);   // File with bg
    pmChip    *bgChip  = pmFPAviewThisChip(view, bgFile->fpa);
    pmReadout *bgReadout = pmFPAviewThisReadout(view, bgChip->parent);

    // Copy the image from the input model readout to the model readout
    model->image = psImageCopy(model->image, bgReadout->image, model->image->type.type);
    // For now also copy the input into the stdev image. Is this used for anything?
    modelStdev->image = psImageCopy(modelStdev->image, bgReadout->image, modelStdev->image->type.type);

    return true;
}

bool psphotLoadBackgroundModel (pmConfig *config, const pmFPAview *view, const char *filerule)
{
    int num = psphotFileruleCount(config, filerule);

    fprintf (stdout, "\n");
    psLogMsg ("psphot", PS_LOG_INFO, "--- psphot Load Background Model ---");

    // loop over the available readouts
    for (int i = 0; i < num; i++) {
        if (!psphotLoadBackgroundModelReadoutFileIndex(config, view, filerule, i)) {
            int lastError = psErrorCodeLast();
	    psError (lastError ? lastError : PSPHOT_ERR_CONFIG, false, "failed to model background for %s entry %d", filerule, i);
            return false;
        }
    }
    return true;
}

// code for in-line plotting from above
// turn on stats tracing in desired cells
// XXX this code may need to be re-worked for a threaded context
# if (0)
    psMetadata *plots = psMetadataLookupPtr (&status, recipe, "DIAGNOSTIC.PLOTS");
    assert (plots);

    int xPlot = psMetadataLookupS32 (&status, plots, "IMAGE.BACKGROUND.CELL.HISTOGRAM.X");
    assert (status);
    int yPlot = psMetadataLookupS32 (&status, plots, "IMAGE.BACKGROUND.CELL.HISTOGRAM.Y");
    assert (status);

    bool gotX = (xPlot < 0) || (xPlot == ix);
    bool gotY = (yPlot < 0) || (yPlot == iy);

    if (gotX && gotY) {
	(void) psTraceSetLevel ("psLib.math.vectorFittedStats", 6);
	(void) psTraceSetLevel ("psLib.math.vectorRobustStats", 6);
    } else {
	(void) psTraceSetLevel ("psLib.math.vectorFittedStats", 0);
	(void) psTraceSetLevel ("psLib.math.vectorRobustStats", 0);
    }
# endif
