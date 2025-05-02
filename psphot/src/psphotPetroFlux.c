# include "psphotInternal.h"

# define PETROSIAN_RADII 2.0

bool psphotPetroFlux (pmConfig *config, const pmFPAview *view, const char *filerule)
{
    bool status = true;

    fprintf (stdout, "\n");
    psLogMsg ("psphot", PS_LOG_INFO, "--- psphot Petro Fluxes ---");

    // select the appropriate recipe information
    psMetadata *recipe  = psMetadataLookupPtr (&status, config->recipes, PSPHOT_RECIPE);
    psAssert (recipe, "missing recipe?");

    int num = psphotFileruleCount(config, filerule);

    // skip the chisq image (optionally?)
    int chisqNum = psMetadataLookupS32 (&status, config->arguments, "PSPHOT.CHISQ.NUM");
    if (!status) chisqNum = -1;

    psMetadataAddBool (recipe, PS_LIST_TAIL, "EXTENDED_SOURCE_ANALYSIS", PS_META_REPLACE, "we measured this, save to disk", true);

    // loop over the available readouts
    for (int i = 0; i < num; i++) {
        if (i == chisqNum) continue; // skip chisq image

        // find the currently selected readout
        pmFPAfile *file = pmFPAfileSelectSingle(config->files, filerule, i); // File of interest
        psAssert (file, "missing file?");

        pmReadout *readout = pmFPAviewThisReadout(view, file->fpa);
        psAssert (readout, "missing readout?");

        pmDetections *detections = psMetadataLookupPtr (&status, readout->analysis, "PSPHOT.DETECTIONS");
        psAssert (detections, "missing detections?");

        psArray *sources = detections->newSources ? detections->newSources : detections->allSources;
        psAssert (sources, "missing sources?");

        if (!psphotPetroFluxReadout (config, recipe, view, filerule, readout, sources)) {
            psError (PSPHOT_ERR_CONFIG, false, "failed to measure magnitudes for %s entry %d", filerule, i);
            return false;
        }
    }
    return true;
}

bool psphotPetroFluxReadout(pmConfig *config, psMetadata *recipe, const pmFPAview *view, const char * filerule, pmReadout *readout, psArray *sources) {

    bool status = false;

    if (!sources->n) {
        psLogMsg ("psphot", PS_LOG_INFO, "no sources, skipping petrosian fluxes");
        return true;
    }

    psTimerStart ("psphot.petro");

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

    // source analysis is done in S/N order (brightest first)
    sources = psArraySort (sources, pmSourceSortByFlux);
    if (!sources->n) {
        psLogMsg ("psphot", PS_LOG_INFO, "no sources, skipping blend");
        return true;
    }

    // threaded measurement of the source magnitudes
    // choose Cx, Cy (see psphotThreadTools.c for overview of the concepts)
    int Cx = 1, Cy = 1;
    psphotChooseCellSizes (&Cx, &Cy, readout, nThreads);

    psArray *cellGroups = psphotAssignSources (Cx, Cy, sources);

    for (int i = 0; i < cellGroups->n; i++) {

        psArray *cells = cellGroups->data[i];

        for (int j = 0; j < cells->n; j++) {

            // allocate a job -- if threads are not defined, this just runs the job
            psThreadJob *job = psThreadJobAlloc ("PSPHOT_PETRO_FLUX");

            psArrayAdd(job->args, 1, readout);
            psArrayAdd(job->args, 1, cells->data[j]); // sources
            PS_ARRAY_ADD_SCALAR(job->args, markVal,            PS_TYPE_IMAGE_MASK);
            PS_ARRAY_ADD_SCALAR(job->args, maskVal,            PS_TYPE_IMAGE_MASK);

// set this to 0 to run without threading
# if (1)
            if (!psThreadJobAddPending(job)) {
                psError(PS_ERR_UNKNOWN, false, "Unable to guess model.");
                return false;
            }
# else
	    if (!psphotPetroFlux_Threaded(job)) {
		psError(PS_ERR_UNKNOWN, false, "Unable to guess model.");
		// psFree(AnalysisRegion);
		return false;
	    }
	    psFree(job);
# endif
        }

        // wait for the threads to finish and manage results
        if (!psThreadPoolWait (false, true)) {
            psError(PS_ERR_UNKNOWN, false, "Unable to guess model.");
            return false;
        }

        // we have only supplied one type of job, so we can assume the types here
        psThreadJob *job = NULL;
        while ((job = psThreadJobGetDone()) != NULL) {
            if (job->args->n < 1) {
                fprintf (stderr, "error with job\n");
            }
            psFree(job);
        }
    }
    psFree (cellGroups);

    psLogMsg ("psphot.petro", PS_LOG_WARN, "measure petro fluxes : %f sec for %ld objects\n", psTimerMark ("psphot.petro"), sources->n);
    return true;
}

bool psphotPetroFlux_Threaded (psThreadJob *job) {

    pmReadout *readout              = job->args->data[0];
    psArray *sources                = job->args->data[1];
    psImageMaskType markVal         = PS_SCALAR_VALUE(job->args->data[2],PS_TYPE_IMAGE_MASK_DATA);
    psImageMaskType maskVal         = PS_SCALAR_VALUE(job->args->data[3],PS_TYPE_IMAGE_MASK_DATA);

    for (int i = 0; i < sources->n; i++) {

	pmSource *source = sources->data[i];
	if (!source->peak) continue; // XXX how can we have a peak-less source?
	if (!source->extpars) continue; // if this is not set, we did not have a valid petRadius

	// check status of this source's moments
	if (!source->moments) continue;
	if (!(source->tmpFlags & PM_SOURCE_TMPF_MOMENTS_MEASURED)) continue;
	if (source->mode & PM_SOURCE_MODE_MOMENTS_FAILURE) continue;

	// XXX where are we storing the supplied petro radius?
	if (!isfinite(source->moments->Mrf)) continue;

	// skip saturated stars modeled with a radial profile 
	if (source->mode2 & PM_SOURCE_MODE2_SATSTAR_PROFILE) continue;

	// replace object in image
	if (source->tmpFlags & PM_SOURCE_TMPF_SUBTRACTED) {
	    pmSourceAdd (source, PM_MODEL_OP_FULL, maskVal);
	}

	// is the right??  XXX need the correct location for Rpet
	float windowRadius = 2.0*source->moments->Mrf ;

	// re-allocate image, weight, mask arrays for each peak with box big enough to fit BIG_RADIUS
	pmSourceRedefinePixels (source, readout, source->peak->x, source->peak->y, windowRadius + 2);
	psAssert (source->pixels, "WTF?");

	// this function populates moments->Mrf,PetroFlux,PetroFluxErr
	psphotPetroFluxSource (source, maskVal);
	psImageMaskPixels (source->maskObj, "AND", PS_NOT_IMAGE_MASK(markVal));

	pmSourceSub (source, PM_MODEL_OP_FULL, maskVal);
    }

    return true;
}

// measure just the flux in the given aperture (this is probably a single common function -- can I use one of the pmSourcePhotometry functions?)
bool psphotPetroFluxSource (pmSource *source, psImageMaskType maskVal) {

    PS_ASSERT_PTR_NON_NULL(source, false);
    PS_ASSERT_PTR_NON_NULL(source->peak, false);
    PS_ASSERT_PTR_NON_NULL(source->pixels, false);
    PS_ASSERT_PTR_NON_NULL(source->variance, false);
    PS_ASSERT_PTR_NON_NULL(source->moments, false);
    PS_ASSERT_PTR_NON_NULL(source->extpars, false);

    // the peak position is less accurate but less subject to extreme deviations
    float dX = source->moments->Mx - source->peak->xf;
    float dY = source->moments->My - source->peak->yf;
    float dR = hypot(dX, dY);
    float Xo = (dR < 2.0) ? source->moments->Mx : source->peak->xf;
    float Yo = (dR < 2.0) ? source->moments->My : source->peak->yf;

    // center of mass in subimage.  Note: the calculation below uses pixel index, so we correct
    // xCM, yCM from pixel coords to pixel index here.
    psF32 xCM = Xo - 0.5 - source->pixels->col0; // coord of peak in subimage
    psF32 yCM = Yo - 0.5 - source->pixels->row0; // coord of peak in subimage

    // Calculate the Petro magnitude (make this block optional?)
    // XXX set the aperture here
    float radPetro  = PETROSIAN_RADII*source->extpars->petrosianRadius;
    float radPetro2 = radPetro*radPetro;

    int nPetroPix = 0;
    float Sum = 0.0;
    float Var = 0.0;

    // set vPix to the source pixels (it may have been set to the
    // smoothed image above)
    psF32 **vPix = source->pixels->data.F32;
    psF32 **vWgt = source->variance->data.F32;
    psImageMaskType **vMsk = (source->maskObj == NULL) ? NULL : source->maskObj->data.PS_TYPE_IMAGE_MASK_DATA;

    for (psS32 row = 0; row < source->pixels->numRows ; row++) {

	psF32 yDiff = row - yCM;
	if (fabs(yDiff) > radPetro) continue;

	for (psS32 col = 0; col < source->pixels->numCols ; col++) {
	    // check mask and value for this pixel
	    if (vMsk && (vMsk[row][col] & maskVal)) continue;
	    if (isnan(vPix[row][col])) continue;

	    psF32 xDiff = col - xCM;
	    if (fabs(xDiff) > radPetro) continue;

	    // radPetro is just a function of (xDiff, yDiff)
	    psF32 r2  = PS_SQR(xDiff) + PS_SQR(yDiff);
	    if (r2 > radPetro2) continue;

	    float pDiff = vPix[row][col];
	    psF32 wDiff = vWgt[row][col];

	    Sum += pDiff;
	    Var += wDiff;
	    nPetroPix ++;
	}
    }

    // return the flux and error to parameters?
    source->extpars->petrosianFlux    = Sum;
    source->extpars->petrosianFluxErr = sqrt(Var);
    source->extpars->petrosianFill = nPetroPix / (M_PI * radPetro2);

    // fprintf (stderr, "petro flux: %f +/- %f\n", Sum, sqrt(Var));

    return true;
}
