# include "psphotInternal.h"
void pmSourceMomentsSetVerbose(bool state);

// convert detections to sources and measure their basic properties (moments, local sky, sky
// variance) Note: this function only generates sources for the new peaks (peak->assigned).
// The new sources are added to any existing sources on detections->newSources.  The sources
// on detections->allSources are ignored.
bool psphotSourceStats (pmConfig *config, const pmFPAview *view, const char *filerule, bool setWindow)
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
        if (!psphotSourceStatsReadout (config, view, filerule, i, recipe, setWindow)) {
            psError (PSPHOT_ERR_CONFIG, false, "failed to find initial detections for %s entry %d", filerule, i);
            return false;
        }
    }
    return true;
}

bool psphotSourceStatsReadout (pmConfig *config, const pmFPAview *view, const char *filerule, int index, psMetadata *recipe, bool setWindow) {

    bool status = false;
    psArray *sources = NULL;

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

    char *breakPt  = psMetadataLookupStr (&status, recipe, "BREAK_POINT");
    psAssert (status, "missing BREAK_POINT?");

    float INNER        = psMetadataLookupF32 (&status, recipe, "SKY_INNER_RADIUS"); psAssert (status, "missing SKY_INNER_RADIUS");
    float MIN_SN       = psMetadataLookupF32 (&status, recipe, "MOMENTS_SN_MIN"); psAssert (status, "missing MOMENTS_SN_MIN");

    // bit-masks to test for good/bad pixels
    psImageMaskType maskVal = psMetadataLookupImageMask(&status, recipe, "MASK.PSPHOT");
    psAssert (maskVal, "missing MASK.PSPHOT");

    // bit-mask to mark pixels not used in analysis
    psImageMaskType markVal = psMetadataLookupImageMask(&status, recipe, "MARK.PSPHOT");
    psAssert (markVal, "missing MARK.PSPHOT");

    psArray *peaks = detections->peaks;
    if (!peaks) {
        psError(PS_ERR_UNEXPECTED_NULL, false, "No peaks found!");
        return false;
    }

    // generate the array of sources, define the associated pixel
    psAssert (!detections->newSources, "programming error");
    sources = detections->newSources = psArrayAllocEmpty (peaks->n);

    bool firstPass = (detections->allSources == NULL);

    // if there are no peaks, we save the empty source array and return
    if (!peaks->n) {
        return true;
    }

    for (int i = 0; i < peaks->n; i++) {

        pmPeak *peak = peaks->data[i];
        if (peak->assigned) continue;

        // create a new source
        pmSource *source = pmSourceAlloc();
        source->imageID = index;

        // add the peak
        source->peak = psMemIncrRefCounter(peak);

	// psAssert (source->peak->footprint, "peak without footprint??");

        // allocate space for moments
        source->moments = pmMomentsAlloc();

        if (source->mode & PM_SOURCE_MODE_MOMENTS_FAILURE) {
	    fprintf (stderr, "moment failure\n");
	}

	if (firstPass) {
            source->mode2 |= PM_SOURCE_MODE2_PASS1_SRC;
	}

        // allocate image, weight, mask arrays for each peak (square of radius OUTER)
        pmSourceDefinePixels (source, readout, source->peak->x, source->peak->y, OUTER);

        peak->assigned = true;
        psArrayAdd (sources, 100, source);
        psFree (source);
    }

    if (!strcasecmp (breakPt, "PEAKS")) {
        psLogMsg ("psphot", PS_LOG_INFO, "%ld sources : %f sec\n", sources->n, psTimerMark ("psphot.stats"));
        psLogMsg ("psphot", PS_LOG_INFO, "break point PEAKS, skipping MOMENTS\n");
        psphotVisualShowMoments (sources);
        return true;
    }

    if (setWindow) {
        if (!psphotSetMomentsWindow(recipe, readout->analysis, sources, maskVal)) {
            psError(PS_ERR_UNEXPECTED_NULL, false, "Failed to determine Moments Window!");
            psFree(detections->newSources);
            return false;
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

            psArrayAdd(job->args, 1, cells->data[j]); // 0 : sources

            PS_ARRAY_ADD_SCALAR(job->args, INNER,   PS_TYPE_F32); // 1
            PS_ARRAY_ADD_SCALAR(job->args, MIN_SN,  PS_TYPE_F32); // 2
            PS_ARRAY_ADD_SCALAR(job->args, RADIUS,  PS_TYPE_F32); // 3
            PS_ARRAY_ADD_SCALAR(job->args, SIGMA,   PS_TYPE_F32); // 4
            PS_ARRAY_ADD_SCALAR(job->args, MIN_KRON_RADIUS, PS_TYPE_F32); // 5

            PS_ARRAY_ADD_SCALAR(job->args, maskVal, PS_TYPE_IMAGE_MASK); // 6
            PS_ARRAY_ADD_SCALAR(job->args, markVal, PS_TYPE_IMAGE_MASK); // 7

            PS_ARRAY_ADD_SCALAR(job->args, 0, PS_TYPE_S32); // 8  : this is used as a return value for Nmoments
            PS_ARRAY_ADD_SCALAR(job->args, 0, PS_TYPE_S32); // 9  : this is used as a return value for Nfail
            PS_ARRAY_ADD_SCALAR(job->args, 0, PS_TYPE_S32); // 10 : this is used as a return value for Nfaint

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

// this function is currently only called by psphotCheckExtSources
bool psphotSourceStatsUpdate (psArray *sources, pmConfig *config, pmReadout *readout) {

    bool status = false;

    psTimerStart ("psphot.stats");

    // select the appropriate recipe information
    psMetadata *recipe  = psMetadataLookupPtr (&status, config->recipes, PSPHOT_RECIPE);
    assert (recipe);

    // determine the number of allowed threads
    int nThreads = psMetadataLookupS32(&status, config->arguments, "NTHREADS"); // Number of threads
    if (!status) {
        nThreads = 0;
    }

    // determine properties (sky, moments) of initial sources
    float OUTER    = psMetadataLookupF32 (&status, recipe, "SKY_OUTER_RADIUS");
    if (!status) return false;

    OUTER = PS_MAX(OUTER, 20.0); // XXX Guarantee that we can encompass the max moments radius

    char *breakPt  = psMetadataLookupStr (&status, recipe, "BREAK_POINT");
    if (!status) return false;

    for (int i = 0; i < sources->n; i++) {

        pmSource *source = sources->data[i];
        if (!source->peak) continue; // XXX how can we have a peak-less source?

        // allocate space for moments
        if (!source->moments) {
            source->moments = pmMomentsAlloc();
        }

        // allocate image, weight, mask arrays for each peak (square of radius OUTER)
        pmSourceDefinePixels (source, readout, source->peak->x, source->peak->y, OUTER);
        source->peak->assigned = true;
    }

    if (!strcasecmp (breakPt, "PEAKS")) {
        psLogMsg ("psphot", PS_LOG_INFO, "%ld sources : %f sec\n", sources->n, psTimerMark ("psphot.stats"));
        psLogMsg ("psphot", PS_LOG_INFO, "break point PEAKS, skipping MOMENTS\n");
        psphotVisualShowMoments (sources);
        return sources;
    }

    // XXX how else could we get the window size in?
    if (!psphotSetMomentsWindow(recipe, readout->analysis, sources, 0)) {
        psError(PS_ERR_UNEXPECTED_NULL, false, "Failed to determine Moments Window!");
        return NULL;
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

            // XXX: this must match the above
	    psAbort ("this code is broken: must match psphotSourceStatsReadout above");
            psArrayAdd(job->args, 1, cells->data[j]); // sources
            psArrayAdd(job->args, 1, recipe);
            PS_ARRAY_ADD_SCALAR(job->args, 0, PS_TYPE_S32); // this is used as a return value for Nmoments
            PS_ARRAY_ADD_SCALAR(job->args, 0, PS_TYPE_S32); // this is used as a return value for Nfail
            PS_ARRAY_ADD_SCALAR(job->args, 0, PS_TYPE_S32); // this is used as a return value for Nfaint

            if (!psThreadJobAddPending(job)) {
                psError(PS_ERR_UNKNOWN, false, "Unable to launch thread job PSPHOT_SOURCE_STATS");
                return NULL;
            }
        }

        // wait for the threads to finish and manage results
        if (!psThreadPoolWait (false, true)) {
            psError(PS_ERR_UNKNOWN, false, "Failure in thread job PSPHOT_SOURCE_STATS");
            return NULL;
        }

        // we have only supplied one type of job, so we can assume the types here
        psThreadJob *job = NULL;
        while ((job = psThreadJobGetDone()) != NULL) {
            if (job->args->n < 1) {
                fprintf (stderr, "error with job\n");
            } else {
                psScalar *scalar = NULL;
                scalar = job->args->data[2];
                Nmoments += scalar->data.S32;
                scalar = job->args->data[3];
                Nfail += scalar->data.S32;
                scalar = job->args->data[4];
                Nfaint += scalar->data.S32;
            }
            psFree(job);
        }
    }

    psFree (cellGroups);

    psLogMsg ("psphot", PS_LOG_INFO, "%ld sources, %d moments, %d faint, %d failed: %f sec\n", sources->n, Nmoments, Nfaint, Nfail, psTimerMark ("psphot.stats"));

    psphotVisualShowMoments (sources);

    return (sources);
}

bool psphotSourceStats_Threaded (psThreadJob *job) {

    bool status = false;
    psScalar *scalar = NULL;

    psArray *sources                = job->args->data[0];

    float INNER                     = PS_SCALAR_VALUE(job->args->data[1],F32);
    float MIN_SN                    = PS_SCALAR_VALUE(job->args->data[2],F32);
    float RADIUS                    = PS_SCALAR_VALUE(job->args->data[3],F32);
    float SIGMA                     = PS_SCALAR_VALUE(job->args->data[4],F32);
    float MIN_KRON_RADIUS           = PS_SCALAR_VALUE(job->args->data[5],F32);

    psImageMaskType maskVal         = PS_SCALAR_VALUE(job->args->data[6],PS_TYPE_IMAGE_MASK_DATA);
    psImageMaskType markVal         = PS_SCALAR_VALUE(job->args->data[7],PS_TYPE_IMAGE_MASK_DATA);

    // if no valid pixels, massive swing or very large Mrf, likely saturated source,
    // try a much larger box
    float BIG_RADIUS = 3.0*RADIUS;
    float BIG_SIGMA  = 3.0*SIGMA;

    // maskVal is used to test for rejected pixels, and must include markVal
    maskVal |= markVal;

    // XXX test : pmSourceMomentsSetVerbose(true);

    // threaded measurement of the sources moments
    int Nfail = 0;
    int Nmoments = 0;
    int Nfaint = 0;
    for (int i = 0; i < sources->n; i++) {
        pmSource *source = sources->data[i];

        if (source->tmpFlags & PM_SOURCE_TMPF_MOMENTS_MEASURED) continue;
        source->tmpFlags |= PM_SOURCE_TMPF_MOMENTS_MEASURED;

        // skip faint sources for moments measurement
        if (sqrt(source->peak->detValue) < MIN_SN) {
            source->mode |= PM_SOURCE_MODE_BELOW_MOMENTS_SN;
            Nfaint++;
            continue;
        }

        // measure a local sky value
        // the local sky is now ignored; kept here for reference only
        status = pmSourceLocalSky (source, PS_STAT_SAMPLE_MEDIAN, INNER, maskVal, markVal);
        if (!status) {
            source->mode |= PM_SOURCE_MODE_SKY_FAILURE;
            psErrorClear(); // XXX re-consider the errors raised here
            Nfail ++;
            continue;
        }

        // measure the local sky variance (needed if noise is not sqrt(signal))
        // XXX EAM : this should use ROBUST not SAMPLE median, but it is broken
        status = pmSourceLocalSkyVariance (source, PS_STAT_SAMPLE_MEDIAN, INNER, maskVal, markVal);
        if (!status) {
            source->mode |= PM_SOURCE_MODE_SKYVAR_FAILURE;
            Nfail ++;
            psErrorClear();
            continue;
        }

	// XXX how can this be set: this is raised in pmSourceRoughClass, called after this function?
	if (source->mode & PM_SOURCE_MODE_SATSTAR) {
	    fprintf (stderr, "satstar: %f,%f\n", source->peak->xf, source->peak->yf);
	}

	// skip saturated stars modeled with a radial profile (this probably never happens, since it is set after)
        if (source->mode2 & PM_SOURCE_MODE2_SATSTAR_PROFILE) continue;

	if (!(source->peak->type == PM_PEAK_SUSPECT_SATURATION)) {
	    // measure basic source moments (no S/N clipping on input pixels)
	    status = pmSourceMoments (source, RADIUS, SIGMA, 0.0, MIN_KRON_RADIUS, maskVal);
	} else { 
	    // For saturated stars, choose a much larger box NOTE this is slightly sleazy, but
	    // only slightly: pmSourceRedefinePixels uses the readout to pass the pointers to
	    // the parent image data.  I guess the API could be simplified: we could recover
	    // this from the source in the function

	    pmReadout tmpReadout;
	    tmpReadout.image    = (psImage *)source->pixels->parent;
	    tmpReadout.mask     = (psImage *)source->maskView->parent;
	    tmpReadout.variance = (psImage *)source->variance->parent;

	    // re-allocate image, weight, mask arrays for each peak with box big enough to fit BIG_RADIUS
	    pmSourceRedefinePixels (source, &tmpReadout, source->peak->x, source->peak->y, BIG_RADIUS + 2);

	    psTrace ("psphot", 4, "retrying moments for %d, %d\n", source->peak->x, source->peak->y);
	    status = pmSourceMoments (source, BIG_RADIUS, BIG_SIGMA, 0.0, MIN_KRON_RADIUS, maskVal);
	    source->mode |= PM_SOURCE_MODE_BIG_RADIUS;
	}
	if (!status) {
	    source->mode |= PM_SOURCE_MODE_MOMENTS_FAILURE;
	    Nfail ++;
	    psErrorClear();
	    continue;
	}

	// XXX test of masking neighbors when measureing moments (does this fail?)
	// psEllipseMoments moments = {source->moments->Mxx, source->moments->Myy, source->moments->Mxy};
	// psEllipseAxes axes = psEllipseMomentsToAxes(moments, 20.0);
        // psImageMaskCircle (source->maskView, source->peak->x, source->peak->y, 3.0*axes.major, "OR", markVal);

	Nmoments ++;

	// re-try big sources or not??
	// if (source->moments->Mrf < 2.0*SIGMA)
    }

    // change the value of a scalar on the array (wrap this and put it in psArray.h)
    scalar = job->args->data[8];
    scalar->data.S32 = Nmoments;

    scalar = job->args->data[9];
    scalar->data.S32 = Nfail;

    scalar = job->args->data[10];
    scalar->data.S32 = Nfaint;

    return true;
}

// this function attempts to iteratively determine the best value for sigma of the moments weighting Gaussian
// this function modifies the recipe values MOMENTS_SX_MAX, MOMENTS_SY_MAX, and PSF_CLUMP_GRID_SCALE, used by pmSourcePSFClump
bool psphotSetMomentsWindow (psMetadata *recipe, psMetadata *analysis, psArray *sources, psImageMaskType maskVal) {

    bool status;

    float MIN_SN = psMetadataLookupF32 (&status, recipe, "MOMENTS_SN_MIN"); psAssert (status, "missing MOMENTS_SN_MIN");
    psF32 MOMENTS_AR_MAX = psMetadataLookupF32(&status, recipe, "MOMENTS_AR_MAX"); psAssert (status, "missing MOMENTS_AR_MAX");

    float MOMENTS_SX_MIN = psMetadataLookupF32(&status, recipe, "MOMENTS_SX_MIN");
    if (!status) {
	MOMENTS_SX_MIN = 0.5;
    }
    float MOMENTS_SY_MIN = psMetadataLookupF32(&status, recipe, "MOMENTS_SY_MIN");
    if (!status) {
	MOMENTS_SY_MIN = 0.5;
    }

    // when we set the window, we are not attempting to measure spatial variations; we can use a somewhat higher S/N limit
    // since we are using all sources (true?)
    float PSF_SN_LIM = 2.0*psMetadataLookupF32(&status, recipe, "PSF_SN_LIM"); psAssert (status, "missing PSF_SN_LIM");

    // XXX this will cause an error in the vector length is > 8
    # define NSIGMA 8
    // moved to config file
    psVector  *sigmavec = psMetadataLookupPtr (&status, recipe, "PSF.SIGMA.VALUES"); 

    float sigma[NSIGMA] = {0,0,0,0,0,0,0};
    float Sout[NSIGMA] = {0,0,0,0,0,0,0};
    float Rmin[NSIGMA] = {0,0,0,0,0,0,0};
    int   Nout[NSIGMA]; // number of stars found in clump : use this to control the number of regions measured by psphotRoughClass

    int nsigma;
    if (sigmavec) {
        psAssert(sigmavec->n <= NSIGMA, "too many sigma values in recipe %ld maximum is %d", sigmavec->n, NSIGMA);
        // copy the data from vector to local array to keep the code below easier to read
        for (int i = 0 ; i < sigmavec->n; i++) {
            sigma[i] = sigmavec->data.F32[i];
        }
	assert (sigmavec->n <= 8);
        nsigma = sigmavec->n;
    } else {
        // requiring this causes updates to pop an assertion
        //  psAssert(status, "missing PSF.SIGMA.VALUES");
        float defaultsigma[NSIGMA]  = {1.0, 2.0, 3.0, 4.5, 6.0, 9.0, 12.0, 18.0};
        for (int i = 0 ; i < NSIGMA; i++) {
            sigma[i] = defaultsigma[i];
        }
        nsigma = NSIGMA;
    }

    // this sorts by peak->rawFlux
    sources = psArraySort (sources, pmSourceSortByFlux);

    // loop over radii:
    for (int i = 0; i < nsigma; i++) {

        // XXX move max source number to config
        for (int j = 0; (j < sources->n) && (j < 400); j++) {

            pmSource *source = sources->data[j];
            psAssert (source->moments, "force moments to exist");
            source->moments->nPixels = 0;

            // skip faint sources for moments measurement
            if (sqrt(source->peak->detValue) < MIN_SN) {
                continue;
            }

            // measure basic source moments (no S/N clipping on input pixels)
	    // sources with (mode & MODE_EXTERNAL) or (mode2 & MODE2_MATCHED) use the 
	    // supplied Mx,My value for the centroid (not recalculated)
            status = pmSourceMoments (source, 4*sigma[i], sigma[i], 0.0, 0.0, maskVal);
        }

#if (1)
	if (psTraceGetLevel("psphot.moments.save")) {
	  char name[64];
	  sprintf (name, "moments.v%d.dat", i);
	  FILE *fout = fopen (name, "w");
	  for (int j = 0; j < sources->n; j++) {
            pmSource *source = sources->data[j];
            psAssert (source->moments, "force moments to exist");
            source->moments->nPixels = 0;
            status = pmSourceMoments (source, 20, sigma[i], 0.0, 0.0, maskVal);
	    fprintf (fout, "%f %f | %f %f %f | %f\n", source->moments->Mx, source->moments->My, source->moments->Mxx, source->moments->Mxy, source->moments->Myy, source->moments->Sum);
	  }
	  fclose (fout);
	}
#endif
        // choose a grid scale that is a fixed fraction of the psf sigma^2
        float PSF_CLUMP_GRID_SCALE = 0.1*PS_SQR(sigma[i]);
        float MOMENTS_SX_MAX = 2.0*PS_SQR(sigma[i]);
        float MOMENTS_SY_MAX = 2.0*PS_SQR(sigma[i]);

        // determine the PSF parameters from the source moment values
        pmPSFClump psfClump = pmSourcePSFClump (NULL, NULL, sources, PSF_SN_LIM, PSF_CLUMP_GRID_SCALE, MOMENTS_SX_MAX, MOMENTS_SY_MAX, MOMENTS_SX_MIN, MOMENTS_SY_MIN, MOMENTS_AR_MAX);
        psLogMsg ("psphot", 3, "sigma guess (pix) %.1f, nStars: %d of %d in clump, nSigma: %5.2f, X,  Y: %f, %f (%f, %f)\n", sigma[i], psfClump.nStars, psfClump.nTotal, psfClump.nSigma, psfClump.X, psfClump.Y, sqrt(psfClump.X) / sigma[i], sqrt(psfClump.Y) / sigma[i]);

	Rmin[i] = pmSourceMinKronRadius(sources, PSF_SN_LIM);

#if 0
        // Modifying clump parameters without restoring!
        psMetadataAddS32 (analysis, PS_LIST_TAIL, "PSF.CLUMP.NREGIONS",  PS_META_REPLACE, "psf clump regions", 1);
        psMetadata *regionMD = psMetadataLookupPtr (&status, analysis, "PSF.CLUMP.REGION.000");
        if (!regionMD) {
            regionMD = psMetadataAlloc();
            psMetadataAddMetadata (analysis, PS_LIST_TAIL, "PSF.CLUMP.REGION.000", PS_META_REPLACE, "psf clump region", regionMD);
            psFree (regionMD);
        }
        psMetadataAddF32 (regionMD, PS_LIST_TAIL, "PSF.CLUMP.X",  PS_META_REPLACE, "psf clump center", psfClump.X);
        psMetadataAddF32 (regionMD, PS_LIST_TAIL, "PSF.CLUMP.Y",  PS_META_REPLACE, "psf clump center", psfClump.Y);
        psMetadataAddF32 (regionMD, PS_LIST_TAIL, "PSF.CLUMP.DX", PS_META_REPLACE, "psf clump center", psfClump.dX);
        psMetadataAddF32 (regionMD, PS_LIST_TAIL, "PSF.CLUMP.DY", PS_META_REPLACE, "psf clump center", psfClump.dY);
	if (pmVisualTestLevel("psphot.moments.full", 2)) {
	    psphotVisualPlotMoments (recipe, analysis, sources);
	}
#endif

	// a clump with no stars is not a valid clump
	Nout[i] = psfClump.nStars;
        Sout[i] = (Nout[i] == 0) ? NAN : sqrt(0.5*(psfClump.X + psfClump.Y)) / sigma[i];
    }

    // we are looking for sigma for which Sout = 0.65 (or some other value)
    // EAM 2020.05.19 : this analysis fails if any of the measurements above have no valid stars
    // A result with Nout[] = 0 should be skipped in the analysis
    
    int Nstars = 0;
    float minKronRadius = NAN;
    float Sigma = NAN;

    float minS = NAN;
    float maxS = NAN;
    int maxN = -1;
    int minN = -1;
    for (int i = 0; i < nsigma; i++) {
      if (!isfinite(Sout[i])) continue;
      if (!isfinite(minS)) {
	minS = Sout[i];
	maxS = Sout[i];
	minN = i;
	maxN = i;
      }
      if (Sout[i] < minS) {
	  minS = Sout[i];
	  minN = i;
      }
      if (Sout[i] > maxS) {
	  maxS = Sout[i];
	  maxN = i;
      }
    }
    if (!isfinite(minS)) {
      psLogMsg ("psphot", 3, "failed to find a valid window function\n");
      return false;
    }

    // if all Sout values are above (or below) the threshold, saturate on the minimum (or maximum) valid sigma value
    if (minS > 0.65) { Sigma = sigma[minN]; Nstars = Nout[minN]; minKronRadius = Rmin[minN]; }
    if (maxS < 0.65) { Sigma = sigma[maxN]; Nstars = Nout[maxN]; minKronRadius = Rmin[maxN]; }

    // this loop will be skipped if Sigma is set above
    for (int i = 0; i < nsigma - 1 && isnan(Sigma); i++) {
        if (!isfinite(Sout[i]) || !isfinite(Sout[i+1])) continue;
        if ((Sout[i] > 0.65) && (Sout[i+1] > 0.65)) continue;
        if ((Sout[i] < 0.65) && (Sout[i+1] < 0.65)) continue;
        Sigma = sigma[i] + (0.65 - Sout[i])*(sigma[i+1] - sigma[i])/(Sout[i+1] - Sout[i]);
	Nstars = 0.5*(Nout[i] + Nout[i+1]);
        minKronRadius = Rmin[i] + (0.65 - Sout[i])*(Rmin[i+1] - Rmin[i])/(Sout[i+1] - Sout[i]);
    }
    // EAM: this test will fail if only a single entry above is valid
    if (!isfinite(Sigma)) {
      psLogMsg ("psphot", 3, "failed to find a valid window function (case 2)\n");
      return false;
    }
    if (!isfinite(minKronRadius)) {
        minKronRadius = Sigma;
    }

    // choose a grid scale that is a fixed fraction of the psf sigma^2
    psMetadataAddF32(analysis, PS_LIST_TAIL, "PSF_CLUMP_GRID_SCALE", PS_META_REPLACE, "clump grid", 0.1*PS_SQR(Sigma));
    psMetadataAddF32(analysis, PS_LIST_TAIL, "MOMENTS_SX_MAX", PS_META_REPLACE, "moments limit", 2.0*PS_SQR(Sigma));
    psMetadataAddF32(analysis, PS_LIST_TAIL, "MOMENTS_SY_MAX", PS_META_REPLACE, "moments limit", 2.0*PS_SQR(Sigma));
    psMetadataAddF32(analysis, PS_LIST_TAIL, "MOMENTS_GAUSS_SIGMA", PS_META_REPLACE, "moments limit", Sigma);
    psMetadataAddF32(analysis, PS_LIST_TAIL, "PSF_MOMENTS_RADIUS", PS_META_REPLACE, "moments limit", 4.0*Sigma);
    psMetadataAddF32(analysis, PS_LIST_TAIL, "PSF_CLUMP_NSTARS", PS_META_REPLACE, "number of stars in clump", Nstars);
    psMetadataAddF32(analysis, PS_LIST_TAIL, "MOMENTS_MIN_KRON", PS_META_REPLACE, "minimum Kron Radius", minKronRadius);

    psLogMsg ("psphot", 3, "using window function with sigma = %f (Min Kron Radius = %f, Nstars = %d)\n", Sigma, minKronRadius, Nstars);
    return true;
}

// if we use the footprints, the output peaks list contains both old and new peaks,
// otherwise it only contains the new peaks.

