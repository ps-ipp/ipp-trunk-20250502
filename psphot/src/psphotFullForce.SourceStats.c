# include "psphotInternal.h"

// construct pixels and measure moments for sources. 
// NOTE: this function differs from psphotSourceStats in the following ways:
// 1) detections->allSources are used instead of newSources
// 2) the sources are used directly (peaks are not assigned to sources here)

// NOTE: this function is meant to be used by psphotFullForce, and the mode of the loaded
// sources should have (MODE_EXTERNAL) set.  If so, the Mx,My values will not be recalculated

bool psphotFullForceSourceStats (pmConfig *config, const pmFPAview *view, const char *filerule, bool setWindow)
{
    bool status = true;

    fprintf (stdout, "\n");
    psLogMsg ("psphot", PS_LOG_INFO, "--- psphot Source Stats ---");

    // select the appropriate recipe information
    psMetadata *recipe  = psMetadataLookupPtr (&status, config->recipes, PSPHOT_RECIPE);
    psAssert (recipe, "missing recipe?");

    int num = psphotFileruleCount(config, filerule);

    // skip the chisq image (optionally?)
    // int chisqNum = psMetadataLookupS32 (&status, config->arguments, "PSPHOT.CHISQ.NUM");
    // if (!status) chisqNum = -1;

    // loop over the available readouts
    for (int i = 0; i < num; i++) {
        // if (i == chisqNum) continue; // skip chisq image
        if (!psphotFullForceSourceStatsReadout (config, view, filerule, i, recipe, setWindow)) {
            psError (PSPHOT_ERR_CONFIG, false, "failed to find initial detections for %s entry %d", filerule, i);
            return false;
        }
    }
    return true;
}

bool psphotFullForceSourceStatsReadout (pmConfig *config, const pmFPAview *view, const char *filerule, int index, psMetadata *recipe, bool setWindow) {

    bool status = false;

    psTimerStart ("psphot.stats");

    // find the currently selected readout
    pmFPAfile *file = pmFPAfileSelectSingle(config->files, filerule, index); // File of interest
    psAssert (file, "missing file?");

    pmReadout *readout = pmFPAviewThisReadout(view, file->fpa);
    psAssert (readout, "missing readout?");

    if (psTraceGetLevel("psphot") > 5) {
	static int pass = 0;
        char name[64];
        sprintf (name, "srstats.v%d.fits", pass);
        psphotSaveImage(NULL, readout->image, name);
	pass ++;
    }

    pmDetections *detections = psMetadataLookupPtr (&status, readout->analysis, "PSPHOT.DETECTIONS");
    psAssert (detections, "missing detections?");

    // determine the number of allowed threads
    int nThreads = psMetadataLookupS32(&status, config->arguments, "NTHREADS"); // Number of threads
    if (!status) {
        nThreads = 0;
    }

    // determine properties (sky, moments) of initial sources
    float OUTER = psMetadataLookupF32 (&status, recipe, "SKY_OUTER_RADIUS");
    psAssert (status, "missing SKY_OUTER_RADIUS in recipe?");

    // XXX this seems like an arbitrary number...
    OUTER = PS_MAX(OUTER, 20.0); // XXX Guarantee that we can encompass the max moments radius

    float INNER        = psMetadataLookupF32 (&status, recipe, "SKY_INNER_RADIUS"); psAssert (status, "missing SKY_INNER_RADIUS");
    float MIN_SN       = psMetadataLookupF32 (&status, recipe, "MOMENTS_SN_MIN"); psAssert (status, "missing MOMENTS_SN_MIN");

    // bit-masks to test for good/bad pixels
    psImageMaskType maskVal = psMetadataLookupImageMask(&status, recipe, "MASK.PSPHOT");
    psAssert (maskVal, "missing MASK.PSPHOT");

    // bit-mask to mark pixels not used in analysis
    psImageMaskType markVal = psMetadataLookupImageMask(&status, recipe, "MARK.PSPHOT");
    psAssert (markVal, "missing MARK.PSPHOT");

    // psArray *sources = detections->allSources;
    psArray *sources = detections->newSources ? detections->newSources : detections->allSources;

    // generate the array of sources, define the associated pixel
    for (int i = 0; i < sources->n; i++) {

        pmSource *source = sources->data[i];
        psAssert (source->peak, "how can we have a peak-less source?");
        psAssert (source->moments, "how can we have a moments-less source?");

        // allocate image, weight, mask arrays for each peak (square of radius OUTER)
        pmSourceDefinePixels (source, readout, source->peak->x, source->peak->y, OUTER);
    }

    if (setWindow) {
	// calculate the best gaussian window (keep centroid positions)
	// NOTE: if sources->mode,mode2 has MODE_EXTERNAL,MODE2_MATCHED, Mx,My are not changed
        if (!psphotSetMomentsWindow(recipe, readout->analysis, sources, maskVal)) {
            psError(PS_ERR_UNEXPECTED_NULL, false, "Failed to determine Moments Window!");
            psFree(detections->newSources);
            return false;
        }

        int error = psErrorCodeLast();
        if (error) {
            psLogMsg ("psphot", PS_LOG_WARN, "Clearing error %d that psphotSetMomentsWindow left on the error stack.", error);
            psErrorClear();
        }
    }

    // if we have measured the window, we will be saving the modified version of these recipe values on readout->analysis
    float SIGMA = psMetadataLookupF32 (&status, readout->analysis, "MOMENTS_GAUSS_SIGMA");
    if (!status) {
        SIGMA = psMetadataLookupF32 (&status, recipe, "MOMENTS_GAUSS_SIGMA");
    }
    float RADIUS = psMetadataLookupF32 (&status, readout->analysis, "PSF_MOMENTS_RADIUS");
    if (!status) {
        RADIUS = psMetadataLookupF32 (&status, recipe, "PSF_MOMENTS_RADIUS");
    }
    float MIN_KRON_RADIUS = psMetadataLookupF32 (&status, readout->analysis, "MOMENTS_MIN_KRON");
    if (!status) {
        MIN_KRON_RADIUS = 0.75*SIGMA;
    }

    // threaded measurement of the source magnitudes
    int Nfail = 0;
    int Nmoments = 0;
    int Nfaint = 0;

    // choose Cx, Cy (see psphotThreadTools.c for overview of the concepts)
    int Cx = 1, Cy = 1;
    psphotChooseCellSizes (&Cx, &Cy, readout, nThreads);

    psArray *cellGroups = psphotAssignSources (Cx, Cy, sources);

    for (int i = 0; i < cellGroups->n; i++) {

        psArray *cells = cellGroups->data[i];

        for (int j = 0; j < cells->n; j++) {

            // allocate a job -- if threads are not defined, this just runs the job
            psThreadJob *job = psThreadJobAlloc ("PSPHOT_SOURCE_STATS");

            psArrayAdd(job->args, 1, cells->data[j]); // sources

            PS_ARRAY_ADD_SCALAR(job->args, INNER,   PS_TYPE_F32);
            PS_ARRAY_ADD_SCALAR(job->args, MIN_SN,  PS_TYPE_F32);
            PS_ARRAY_ADD_SCALAR(job->args, RADIUS,  PS_TYPE_F32);
            PS_ARRAY_ADD_SCALAR(job->args, SIGMA,   PS_TYPE_F32);
            PS_ARRAY_ADD_SCALAR(job->args, MIN_KRON_RADIUS, PS_TYPE_F32);

            PS_ARRAY_ADD_SCALAR(job->args, maskVal, PS_TYPE_IMAGE_MASK);
            PS_ARRAY_ADD_SCALAR(job->args, markVal, PS_TYPE_IMAGE_MASK);

            PS_ARRAY_ADD_SCALAR(job->args, 0, PS_TYPE_S32); // this is used as a return value for Nmoments
            PS_ARRAY_ADD_SCALAR(job->args, 0, PS_TYPE_S32); // this is used as a return value for Nfail
            PS_ARRAY_ADD_SCALAR(job->args, 0, PS_TYPE_S32); // this is used as a return value for Nfaint

            if (!psThreadJobAddPending(job)) {
                psError(PS_ERR_UNKNOWN, false, "Unable to launch thread job PSPHOT_SOURCE_STATS");
                psFree(detections->newSources);
                return false;
            }
        }

        // wait for the threads to finish and manage results
        if (!psThreadPoolWait (false, true)) {
            psError(PS_ERR_UNKNOWN, false, "Failure in thread job PSPHOT_SOURCE_STATS");
            psFree(detections->newSources);
            return false;
        }

        // we have only supplied one type of job, so we can assume the types here
        psThreadJob *job = NULL;
        while ((job = psThreadJobGetDone()) != NULL) {
            if (job->args->n < 1) {
                fprintf (stderr, "error with job\n");
            } else {
                psScalar *scalar = NULL;
                scalar = job->args->data[8];
                Nmoments += scalar->data.S32;
                scalar = job->args->data[9];
                Nfail += scalar->data.S32;
                scalar = job->args->data[10];
                Nfaint += scalar->data.S32;
            }
            psFree(job);
        }
    }

    psFree (cellGroups);

    psLogMsg ("psphot", PS_LOG_WARN, "%ld sources, %d moments, %d faint, %d failed: %f sec\n", sources->n, Nmoments, Nfaint, Nfail, psTimerMark ("psphot.stats"));

    psphotVisualShowMoments (sources);

    return true;
}
