# include "psphotInternal.h"
void psphotRadialProfileShowSkips ();

// measure the petrosian parameters for the sources

// for now, let's store the detections on the readout->analysis for each readout
bool psphotExtendedSourceAnalysis (pmConfig *config, const pmFPAview *view, const char *filerule)
{
    bool status = true;

    fprintf (stdout, "\n");
    psLogMsg ("psphot", PS_LOG_INFO, "--- psphot Extended Source Analysis (Petrosians) ---");

    // select the appropriate recipe information
    psMetadata *recipe  = psMetadataLookupPtr (&status, config->recipes, PSPHOT_RECIPE);
    psAssert (recipe, "missing recipe?");

    bool doPetrosian = psMetadataLookupBool (&status, recipe, "EXTENDED_SOURCE_PETROSIAN");
    bool doAnnuli    = psMetadataLookupBool (&status, recipe, "EXTENDED_SOURCE_ANNULI");

    // measure petrosians?
    if (!doPetrosian && !doAnnuli) {
	psLogMsg ("psphot", PS_LOG_INFO, "skipping extended source measurements\n");
	return true;
    }

    int num = psphotFileruleCount(config, filerule);

    // loop over the available readouts
    for (int i = 0; i < num; i++) {
	if (!psphotExtendedSourceAnalysisReadout (config, view, filerule, i, recipe)) {
            psError (PSPHOT_ERR_CONFIG, false, "failed on measure extended source aperture-like parameters for %s entry %d", filerule, i);
	    return false;
	}
    }
    return true;
}

/*** for the moment, this test code : it is not thread safe ***/
static int    Nall = 0;
static int  Nskip1 = 0;
static int  Nskip2 = 0;
static int  Nskip3 = 0;
static int  Nskip4 = 0;
static int  Nskip5 = 0;
static int  Nskip6 = 0;

# define SKIP(VALUE) { VALUE++; continue; }

// aperture-like measurements for extended sources
bool psphotExtendedSourceAnalysisReadout (pmConfig *config, const pmFPAview *view, const char *filerule, int index, psMetadata *recipe) {

    bool status;
    int Next = 0;
    int Npetro = 0;
    int Nannuli = 0;

    psTimerStart ("psphot.extended");

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
	psLogMsg ("psphot", PS_LOG_INFO, "no sources, skipping source size");
	return true;
    }

    // determine the number of allowed threads
    int nThreads = psMetadataLookupS32(&status, config->arguments, "NTHREADS"); // Number of threads
    if (!status) {
        nThreads = 0;
    }

    // get the sky noise from the background analysis; if this is missing, get the user-supplied value
    float skynoise = psMetadataLookupF32 (&status, readout->analysis, "SKY_STDEV");
    if (!status) {
	skynoise = psMetadataLookupF32 (&status, recipe, "SKY.NOISE");
	psWarning ("failed to get sky noise level from background analysis; defaulting to user supplied value of %f\n", skynoise);
    }

    // source analysis is done in S/N order (brightest first)
    sources = psArraySort (sources, pmSourceSortByFlux);

    // option to limit analysis to a specific region
    char *region = psMetadataLookupStr (&status, recipe, "ANALYSIS_REGION");
    psRegion *AnalysisRegion = psRegionAlloc(0,0,0,0);
    *AnalysisRegion = psRegionForImage (readout->image, psRegionFromString (region));
    if (psRegionIsNaN (*AnalysisRegion)) psAbort("analysis region mis-defined");

    // choose Cx, Cy (see psphotThreadTools.c for overview of the concepts)
    int Cx = 1, Cy = 1;
    psphotChooseCellSizes (&Cx, &Cy, readout, nThreads);

    psArray *cellGroups = psphotAssignSources (Cx, Cy, sources);

    for (int i = 0; i < cellGroups->n; i++) {

        psArray *cells = cellGroups->data[i];

        for (int j = 0; j < cells->n; j++) {

            // allocate a job -- if threads are not defined, this just runs the job
            psThreadJob *job = psThreadJobAlloc ("PSPHOT_EXTENDED_ANALYSIS");

            psArrayAdd(job->args, 1, readout);
            psArrayAdd(job->args, 1, cells->data[j]); // sources
            psArrayAdd(job->args, 1, AnalysisRegion);
            psArrayAdd(job->args, 1, recipe);

            PS_ARRAY_ADD_SCALAR(job->args, skynoise, PS_TYPE_F32);

            PS_ARRAY_ADD_SCALAR(job->args, 0, PS_TYPE_S32); // this is used as a return value for Next
            PS_ARRAY_ADD_SCALAR(job->args, 0, PS_TYPE_S32); // this is used as a return value for Npetro
            PS_ARRAY_ADD_SCALAR(job->args, 0, PS_TYPE_S32); // this is used as a return value for Nannuli

// set this to 0 to run without threading
# if (0)	    
            if (!psThreadJobAddPending(job)) {
                psError(PS_ERR_UNKNOWN, false, "Unable to guess model.");
		psFree(AnalysisRegion);
                return false;
            } 
# else
	    if (!psphotExtendedSourceAnalysis_Threaded(job)) {
		psError(PS_ERR_UNKNOWN, false, "Unable to guess model.");
		psFree(AnalysisRegion);
		return false;
	    }
	    psScalar *scalar = NULL;
	    scalar = job->args->data[5];
	    Next += scalar->data.S32;
	    scalar = job->args->data[6];
	    Npetro += scalar->data.S32;
	    scalar = job->args->data[7];
	    Nannuli += scalar->data.S32;
	    psFree(job);
# endif
	}

        // wait for the threads to finish and manage results
        if (!psThreadPoolWait (false, true)) {
            psError(PS_ERR_UNKNOWN, false, "Unable to guess model.");
	    psFree(AnalysisRegion);
            return false;
        }

        // we have only supplied one type of job, so we can assume the types here
        psThreadJob *job = NULL;
        while ((job = psThreadJobGetDone()) != NULL) {
            if (job->args->n < 1) {
                fprintf (stderr, "error with job\n");
            } else {
		psScalar *scalar = NULL;
		scalar = job->args->data[5];
		Next += scalar->data.S32;
		scalar = job->args->data[6];
		Npetro += scalar->data.S32;
		scalar = job->args->data[7];
		Nannuli += scalar->data.S32;
            }
            psFree(job);
	}
    }
    psFree (cellGroups);
    psFree(AnalysisRegion);

    psLogMsg ("psphot", PS_LOG_WARN, "extended source analysis: %f sec for %d objects\n", psTimerMark ("psphot.extended"), Next);
    psLogMsg ("psphot", PS_LOG_INFO, "  %d petrosian\n", Npetro);
    psLogMsg ("psphot", PS_LOG_INFO, "  %d annuli\n", Nannuli);

# if (PS_TRACE_ON)
    fprintf (stderr, "ext analysis skipped @ 1  : %d\n", Nskip1);
    fprintf (stderr, "ext analysis skipped @ 2  : %d\n", Nskip2);
    fprintf (stderr, "ext analysis skipped @ 3  : %d\n", Nskip3);
    fprintf (stderr, "ext analysis skipped @ 4  : %d\n", Nskip4);
    fprintf (stderr, "ext analysis skipped @ 5  : %d\n", Nskip5);
    fprintf (stderr, "ext analysis skipped @ 6  : %d\n", Nskip6);
#endif

    psphotRadialProfileShowSkips ();

    psphotVisualShowResidualImage (readout, false);

    psphotVisualShowPetrosians (sources);

    return true;
}

bool psphotExtendedSourceAnalysis_Threaded (psThreadJob *job) {

    bool status;

    int Next = 0;
    int Npetro = 0;
    int Nannuli = 0;

    // arguments: readout, sources, models, region, psfSize, maskVal, markVal
    pmReadout *readout      = job->args->data[0];
    psArray *sources        = job->args->data[1];
    psRegion *region        = job->args->data[2];
    psMetadata *recipe      = job->args->data[3];

    float skynoise          = PS_SCALAR_VALUE(job->args->data[4],F32);

    bool doPetrosian    = psMetadataLookupBool (&status, recipe, "EXTENDED_SOURCE_PETROSIAN");

    // user-defined masks to test for good/bad pixels (build from recipe list if not yet set)
    psImageMaskType maskVal = psMetadataLookupImageMask(&status, recipe, "MASK.PSPHOT"); // Mask value for bad pixels
    assert (maskVal);

    // choose the sources of interest
    for (int i = 0; i < sources->n; i++) {

	pmSource *source = sources->data[i];

	Nall ++;

	// rules for measuring petrosian parameters for specific objects are set in
	// psphotChooseAnalysisOptions.c
	if (!(source->tmpFlags & PM_SOURCE_TMPF_PETRO)) SKIP (Nskip1);

	// limit selection by analysis region (XXX move this into psphotChooseAnalysisOption?)
	if (source->peak->x < region->x0) SKIP (Nskip2);
	if (source->peak->y < region->y0) SKIP (Nskip3);
	if (source->peak->x > region->x1) SKIP (Nskip4);
	if (source->peak->y > region->y1) SKIP (Nskip5);

	// replace object in image
	if (source->tmpFlags & PM_SOURCE_TMPF_SUBTRACTED) {
	    pmSourceAdd (source, PM_MODEL_OP_FULL, maskVal);
	}
	Next ++;

	// force source image to be a bit larger...
	float radius = source->peak->xf - source->pixels->col0;
	radius = PS_MAX (radius, source->peak->yf - source->pixels->row0);
	radius = PS_MAX (radius, source->pixels->numRows - source->peak->yf + source->pixels->row0);
	radius = PS_MAX (radius, source->pixels->numCols - source->peak->xf + source->pixels->col0);
	pmSourceRedefinePixels (source, readout, source->peak->xf, source->peak->yf, 1.5*radius);

	// measure the radial profile
	if (!psphotRadialProfile (source, recipe, skynoise, maskVal)) {
	  // re-subtract the object, leave local sky
	  psTrace ("psphot", 5, "failed to extract radial profile for source at %7.1f, %7.1f", source->moments->Mx, source->moments->My);
	  pmSourceSub (source, PM_MODEL_OP_FULL, maskVal);
	  SKIP (Nskip6);
	}

	Nannuli ++;
	source->mode |= PM_SOURCE_MODE_RADIAL_FLUX;

	// Petrosian Mags
	if (doPetrosian) {
	  if (!psphotPetrosian (source, recipe, skynoise, maskVal)) {
	    psTrace ("psphot", 5, "FAILED petrosian flux & radius for source at %7.1f, %7.1f", source->moments->Mx, source->moments->My);
	  } else {
	    psTrace ("psphot", 5, "measured petrosian flux & radius for source at %7.1f, %7.1f", source->moments->Mx, source->moments->My);
	    Npetro ++;
	    source->mode |= PM_SOURCE_MODE_EXTENDED_STATS;
	  }
	}

	// re-subtract the object, leave local sky
	pmSourceSub (source, PM_MODEL_OP_FULL, maskVal);

	if (source->extpars) {
	    psFree(source->extpars->radFlux);
	    psFree(source->extpars->ellipticalFlux);
	}
    }

    psScalar *scalar = NULL;

    // change the value of a scalar on the array (wrap this and put it in psArray.h)
    scalar = job->args->data[5];
    scalar->data.S32 = Next;

    scalar = job->args->data[6];
    scalar->data.S32 = Npetro;

    scalar = job->args->data[7];
    scalar->data.S32 = Nannuli;

    return true;
}
