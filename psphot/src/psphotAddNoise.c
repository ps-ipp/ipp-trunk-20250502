# include "psphotInternal.h"

bool psphotAddOrSubNoise_Threaded (psThreadJob *job);
bool psphotMaskSource(pmSource *source, bool add, psImageMaskType maskVal);

bool psphotAddNoise (pmConfig *config, const pmFPAview *view, const char *filerule) {
    return psphotAddOrSubNoise (config, view, filerule, true);
}

bool psphotSubNoise (pmConfig *config, const pmFPAview *view, const char *filerule) {
    return psphotAddOrSubNoise (config, view, filerule, false);
}

// for now, let's store the detections on the readout->analysis for each readout
bool psphotAddOrSubNoise (pmConfig *config, const pmFPAview *view, const char *filerule, bool add)
{
    bool status = true;

    // select the appropriate recipe information
    psMetadata *recipe  = psMetadataLookupPtr (&status, config->recipes, PSPHOT_RECIPE);
    psAssert (recipe, "missing recipe?");

    int num = psphotFileruleCount(config, filerule);

    // loop over the available readouts
    for (int i = 0; i < num; i++) {
	if (!psphotAddOrSubNoiseReadout (config, view, filerule, i, recipe, add)) {
            psError (PSPHOT_ERR_CONFIG, false, "failed on to modify noise for %s entry %d", filerule, i);
	    return false;
	}
    }
    return true;
}

// the return state indicates if any sources were actually replaced
bool psphotAddOrSubNoiseReadout (pmConfig *config, const pmFPAview *view, const char *filerule, int index, psMetadata *recipe, bool add) {

    bool status = false;

    psTimerStart ("psphot.noise");

    // find the currently selected readout
    pmFPAfile *file = pmFPAfileSelectSingle(config->files, filerule, index); // File of interest
    psAssert (file, "missing file?");

    pmReadout *readout = pmFPAviewThisReadout(view, file->fpa);
    psAssert (readout, "missing readout?");
    psAssert (readout->parent, "missing cell?");
    psAssert (readout->parent->concepts, "missing concepts?");

    pmDetections *detections = psMetadataLookupPtr (&status, readout->analysis, "PSPHOT.DETECTIONS");
    psAssert (detections, "missing detections?");

    psArray *sources = detections->allSources;
    // if no work to do, should just return true
    if (!sources) return false;

    // determine the number of allowed threads
    int nThreads = psMetadataLookupS32(&status, config->arguments, "NTHREADS"); // Number of threads
    if (!status) {
        nThreads = 0;
    }

    // user-defined masks to test for good/bad pixels (build from recipe list if not yet set)
    psImageMaskType maskVal = psMetadataLookupImageMask(&status, recipe, "MASK.PSPHOT"); // Mask value for bad pixels
    psAssert (maskVal, "missing mask value?");

    psImageMaskType markVal = psMetadataLookupImageMask(&status, recipe, "MARK.PSPHOT"); // Mask value for bad pixels
    psAssert (markVal, "missing mask value?");

    // increase variance by factor*(object noise):
    // weight = flux/gain + rn^2/g^2
    // we are adding factor*flux/gain

    float FACTOR = psMetadataLookupF32 (&status, recipe, "NOISE.FACTOR");
    PS_ASSERT (status, false);
    float SIZE = psMetadataLookupF32 (&status, recipe, "NOISE.SIZE");
    PS_ASSERT (status, false);

    if (SIZE <= 0) {
       return false;
    }

    float GAIN = psMetadataLookupF32(&status, readout->parent->concepts, "CELL.GAIN"); // Cell gain
    PS_ASSERT (status, false);
    if (isfinite(GAIN)) {
        FACTOR /= GAIN;
    }

    psphotVisualShowImage (readout);

    // choose Cx, Cy (see psphotThreadTools.c for overview of the concepts)
    int Cx = 1, Cy = 1;
    psphotChooseCellSizes (&Cx, &Cy, readout, nThreads);

    psArray *cellGroups = psphotAssignSources (Cx, Cy, sources);

    for (int i = 0; i < cellGroups->n; i++) {

        psArray *cells = cellGroups->data[i];

        for (int j = 0; j < cells->n; j++) {

            // allocate a job -- if threads are not defined, this just runs the job
            psThreadJob *job = psThreadJobAlloc ("PSPHOT_ADD_NOISE");
            psArrayAdd(job->args, 1, cells->data[j]); // sources
            PS_ARRAY_ADD_SCALAR(job->args, maskVal,  PS_TYPE_IMAGE_MASK);
            PS_ARRAY_ADD_SCALAR(job->args, markVal,  PS_TYPE_IMAGE_MASK);
            PS_ARRAY_ADD_SCALAR(job->args, FACTOR,   PS_TYPE_F32);
            PS_ARRAY_ADD_SCALAR(job->args, SIZE,     PS_TYPE_F32);
            PS_ARRAY_ADD_SCALAR(job->args, add,      PS_TYPE_U8);

# if (1)
            if (!psThreadJobAddPending(job)) {
                psError(PS_ERR_UNKNOWN, false, "Unable to add/sub noise.");
                return false;
            }
# else
            if (!psphotAddOrSubNoise_Threaded(job)) {
                psError(PS_ERR_UNKNOWN, false, "Unable to add/sub noise.");
                return false;
            }
	    psFree(job);
# endif
        }

        // wait for the threads to finish and manage results
        if (!psThreadPoolWait (false, true)) {
            psFree(cellGroups);
            psError(PS_ERR_UNKNOWN, false, "Unable to add/sub noise.");
            return false;
        }

        // we have only supplied one type of job, so we can assume the types here
        psThreadJob *job = NULL;
        while ((job = psThreadJobGetDone()) != NULL) {
            // we have no returned data from this operation
            if (job->args->n < 1) {
                fprintf (stderr, "error with job\n");
            }
            psFree(job);
        }
    }

    if (add) {
        psLogMsg ("psphot.noise", PS_LOG_WARN, "add noise for %ld objects: %f sec\n", sources->n, psTimerMark ("psphot.noise"));
    } else {
        psLogMsg ("psphot.noise", PS_LOG_WARN, "sub noise for %ld objects: %f sec\n", sources->n, psTimerMark ("psphot.noise"));
    }

    psFree (cellGroups);

    psphotVisualShowImage (readout);

    return true;
}

bool psphotAddOrSubNoise_Threaded (psThreadJob *job) {

    psArray *sources = job->args->data[0];
    psImageMaskType maskVal = PS_SCALAR_VALUE(job->args->data[1], PS_TYPE_IMAGE_MASK_DATA);
    psImageMaskType markVal = PS_SCALAR_VALUE(job->args->data[2], PS_TYPE_IMAGE_MASK_DATA);
    psF32 FACTOR = PS_SCALAR_VALUE(job->args->data[3], F32);
    psF32 SIZE = PS_SCALAR_VALUE(job->args->data[4], F32);
    psBool add  = PS_SCALAR_VALUE(job->args->data[5], U8);

    // loop over all source
    for (int i = 0; i < sources->n; i++) {
        pmSource *source = sources->data[i];

	// add or subtract noise for a saturated star.  satstars modeled as a radial profile
	// need special handling
	if (source->mode2 & PM_SOURCE_MODE2_SATSTAR_PROFILE) {
	    psphotSatstarProfileOp (source, maskVal, FACTOR, PM_MODEL_OP_NOISE, add);
	    continue;
	}

        // skip sources which were not subtracted
	// NOTE: this bit is not modified when pmSourceOp applies to noise
        if (!(source->tmpFlags & PM_SOURCE_TMPF_SUBTRACTED)) continue;

	pmSourceNoiseOp (source, PM_MODEL_OP_FULL | PM_MODEL_OP_NOISE, FACTOR, SIZE, add, maskVal, 0, 0);

	psphotMaskSource (source, add, markVal);
    }
    return true;
}

bool psphotMaskSource(pmSource *source, bool add, psImageMaskType maskVal) {

    if (!source) return false;
    if (!source->peak) return false; // XXX how can we have a peak-less source?
    if (source->type == PM_SOURCE_TYPE_DEFECT) return false;
    if (source->type == PM_SOURCE_TYPE_SATURATED) return false;

    float Xc = source->peak->xf - source->pixels->col0 - 0.5;
    float Yc = source->peak->yf - source->pixels->row0 - 0.5;

    psImageMaskType notMaskVal = ~maskVal;

    for (int iy = 0; iy < source->pixels->numRows; iy++) {
	for (int ix = 0; ix < source->pixels->numCols; ix++) {

	    float radius = hypot (ix - Xc, iy - Yc) ;

	    if (radius > 4) continue;

	    if (add) {
	      source->maskView->data.PS_TYPE_IMAGE_MASK_DATA[iy][ix] |= maskVal;
	    } else {
	      source->maskView->data.PS_TYPE_IMAGE_MASK_DATA[iy][ix] &= notMaskVal;
	    }
	}
    }
    return true;
}

