# include "psphotInternal.h"

// XXX : the threading here is not great.  this may be due to blocks between elements, but
// the selection of the objects in a cell is not optimal.  To fix:
// 1) define the boundaries of the cells up front
// 2) loop over the sources once and associate them with their cell
// 3) define the threaded function to work with sources for a given cell

// for now, let's store the detections on the readout->analysis for each readout
bool psphotGuessModels (pmConfig *config, const pmFPAview *view, const char *filerule)
{
    bool status = true;

    int num = psphotFileruleCount(config, filerule);

    // skip the chisq image (optionally?)
    int chisqNum = psMetadataLookupS32 (&status, config->arguments, "PSPHOT.CHISQ.NUM");
    if (!status) chisqNum = -1;

    // loop over the available readouts
    for (int i = 0; i < num; i++) {
        if (i == chisqNum) continue; // skip chisq image
        if (!psphotGuessModelsReadout (config, view, filerule, i)) {
            psError (PSPHOT_ERR_CONFIG, false, "failed on to guess models for %s entry %d", filerule, i);
            return false;
        }
    }
    return true;
}

int NpixTotal = 0;

// construct an initial PSF model for each object (new sources only)
bool psphotGuessModelsReadout (pmConfig *config, const pmFPAview *view, const char *filerule, int index) {

    bool status;

    psTimerStart ("psphot.models");

    // find the currently selected readout
    pmFPAfile *file = pmFPAfileSelectSingle(config->files, filerule, index); // File of interest
    psAssert (file, "missing file?");

    pmReadout *readout = pmFPAviewThisReadout(view, file->fpa);
    psAssert (readout, "missing readout?");

    pmDetections *detections = psMetadataLookupPtr (&status, readout->analysis, "PSPHOT.DETECTIONS");
    psAssert (detections, "missing detections?");

    psArray *sources = detections->allSources;
    psAssert (sources, "missing sources?");

    if (!sources->n) {
        psLogMsg ("psphot", PS_LOG_INFO, "no sources, skipping model guess");
        return true;
    }

    pmPSF *psf = psMetadataLookupPtr (&status, readout->analysis, "PSPHOT.PSF");
    psAssert (psf, "missing psf?");

    // select the appropriate recipe information
    psMetadata *recipe  = psMetadataLookupPtr (&status, config->recipes, PSPHOT_RECIPE);
    assert (recipe);

    // determine the number of allowed threads
    int nThreads = psMetadataLookupS32(&status, config->arguments, "NTHREADS"); // Number of threads
    if (!status) {
        nThreads = 0;
    }

    // bit-masks to test for good/bad pixels
    psImageMaskType maskVal = psMetadataLookupImageMask(&status, recipe, "MASK.PSPHOT");
    assert (maskVal);

    // bit-mask to mark pixels not used in analysis
    psImageMaskType markVal = psMetadataLookupImageMask(&status, recipe, "MARK.PSPHOT");
    assert (markVal);

    // maskVal is used to test for rejected pixels, and must include markVal
    maskVal |= markVal;

    // setup the PSF fit radius details
    psphotInitRadiusPSF (recipe, readout);

    // choose Cx, Cy (see psphotThreadTools.c for overview of the concepts)
    int Cx = 1, Cy = 1;
    psphotChooseCellSizes (&Cx, &Cy, readout, nThreads);

    psArray *cellGroups = psphotAssignSources (Cx, Cy, sources);

    NpixTotal = 0;

    for (int i = 0; i < cellGroups->n; i++) {

        psArray *cells = cellGroups->data[i];

        for (int j = 0; j < cells->n; j++) {

            // allocate a job -- if threads are not defined, this just runs the job
            psThreadJob *job = psThreadJobAlloc ("PSPHOT_GUESS_MODEL");
            psArrayAdd(job->args, 1, readout);
            psArrayAdd(job->args, 1, cells->data[j]); // sources
            psArrayAdd(job->args, 1, psf);

            // XXX change these to use abstract mask type info
            PS_ARRAY_ADD_SCALAR(job->args, maskVal,  PS_TYPE_IMAGE_MASK);
            PS_ARRAY_ADD_SCALAR(job->args, markVal,  PS_TYPE_IMAGE_MASK);

# if (1)
            if (!psThreadJobAddPending(job)) {
                psError(PS_ERR_UNKNOWN, false, "Unable to guess model.");
                return false;
            }
# else
            if (!psphotGuessModel_Threaded(job)) {
                psError(PS_ERR_UNKNOWN, false, "Unable to guess model.");
                return false;
            }
	    psFree(job);
# endif
        }


        // wait for the threads to finish and manage results
        // wait here for the threaded jobs to finish
        // fprintf (stderr, "wait for threads (%d, %d)\n", jx, jy);
        if (!psThreadPoolWait (false, true)) {
            // harvest our jobs
            psFree(cellGroups);
            psError(PS_ERR_UNKNOWN, false, "Unable to guess model.");
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

    // XXX check that all sources that should have models
    int nMiss = 0;
    for (int i = 0; i < sources->n; i++) {
        pmSource *source = sources->data[i];
        if (source->tmpFlags & PM_SOURCE_TMPF_MODEL_GUESS) {
            continue;
        }
        nMiss ++;
    }
    psAssert (nMiss == 0, "failed to attempt to build models for %d objects\n", nMiss);
    
    psFree (cellGroups);

    psLogMsg ("psphot.models", PS_LOG_WARN, "built models for %ld objects: %f sec (%d pixels)\n", sources->n, psTimerMark ("psphot.models"), NpixTotal);
    return true;
}

// construct models only for sources in the specified region
bool psphotGuessModel_Threaded (psThreadJob *job) {

    pmReadout *readout = job->args->data[0];
    psArray *sources   = job->args->data[1];
    pmPSF *psf         = job->args->data[2];

    psImageMaskType maskVal = PS_SCALAR_VALUE(job->args->data[3],PS_TYPE_IMAGE_MASK_DATA);
    psImageMaskType markVal = PS_SCALAR_VALUE(job->args->data[4],PS_TYPE_IMAGE_MASK_DATA);

    int nSrc = 0;

    for (int i = 0; i < sources->n; i++) {
        pmSource *source = sources->data[i];

	// do not redo sources already guessed
	if (source->tmpFlags & PM_SOURCE_TMPF_MODEL_GUESS) continue;

        // this is used to mark sources for which the model is measured. We check later that
        // all are used.
        source->tmpFlags |= PM_SOURCE_TMPF_MODEL_GUESS;

        // skip non-astronomical objects (very likely defects) and satstars with profiles subtracted
        if (source->mode  & PM_SOURCE_MODE_MOMENTS_FAILURE) continue;
        if (source->mode2 & PM_SOURCE_MODE2_SATSTAR_PROFILE) continue;
        if (source->type == PM_SOURCE_TYPE_DEFECT) continue;
        if (source->type == PM_SOURCE_TYPE_SATURATED) continue;
        if (!source->peak) continue;

	// psAssert (source->peak->footprint, "peak without footprint??");

        nSrc ++;

        // the guess central intensity comes from the peak:
        float Io = source->peak->rawFlux;
        if (!isfinite(Io) && source->moments) {
	  Io = source->moments->Peak;
	}

        // We have two options to get a guess for the object position: the position from the
        // peak and the position from the moments.  Use the peak position if there are no
        // moments

        bool useMoments = pmSourcePositionUseMoments(source);

        float Xo, Yo;
        if (useMoments) {
            Xo = source->moments->Mx;
            Yo = source->moments->My;
        } else {
            Xo = source->peak->xf;
            Yo = source->peak->yf;
        }

# if (0)
	if (source->mode & PM_SOURCE_MODE_SATSTAR) {
	    fprintf (stderr, "satstar: %f,%f vs %f,%f : %c\n", 
		     source->moments->Mx, source->moments->My, 
		     source->peak->xf, source->peak->yf,
		     (useMoments ? 'T' : 'F'));
	}
# endif

        // set PSF parameters for this model (apply 2D shape model to coordinates Xo, Yo)
        pmModel *modelPSF = pmModelFromPSFforXY(psf, Xo, Yo, Io);

        if (modelPSF == NULL) {
            psWarning ("Failed to determine PSF model at (%f,%f); trying image center", Xo, Yo);

            float Xc = 0.5*readout->image->numCols;
            float Yc = 0.5*readout->image->numRows;
            modelPSF = pmModelFromPSFforXY(psf, Xc, Yc, Io);
            if (modelPSF == NULL) {
                psError(PSPHOT_ERR_PSF, false, "Failed to determine PSF model at center of image");
                return false;
            }

            // Now set the object position at the expected location:
            modelPSF->params->data.F32[PM_PAR_XPOS] = Xo;
            modelPSF->params->data.F32[PM_PAR_YPOS] = Yo;
            source->mode |= PM_SOURCE_MODE_BADPSF;
        }

        // set the fit radius based on the object flux limit and the model
        // this function affects the mask pixels
        psphotCheckRadiusPSF (readout, source, modelPSF, markVal);

        // set the source PSF model
        psAssert (source->modelPSF == NULL, "failed to free one of the models?");
        source->modelPSF = modelPSF;
        source->modelPSF->residuals = psf->residuals;

        pmSourceCacheModel (source, maskVal);  // ALLOC x14 (!)
	NpixTotal += source->pixels->numCols * source->pixels->numRows;
    }

    return true;
}
