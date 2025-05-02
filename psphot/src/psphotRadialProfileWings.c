# include "psphotInternal.h"

// measure the radial profile outside of the core.  the goal is to find the point at which we
// reach sky + X sigma

bool psphotRadialProfileWingsSource (pmSource *source, pmReadout *readout, psImageMaskType maskVal);
bool psphotRadialProfileFluxAtRadius (psVector *flux, psVector *fluxVar, pmSource *source, pmReadout *readout, float Radius, float dRadius, psImageMaskType maskVal);

bool psphotRadialProfileWings (pmConfig *config, const pmFPAview *view, const char *filerule)
{
    bool status = true;

    // return true;

    fprintf (stdout, "\n");
    psLogMsg ("psphot", PS_LOG_INFO, "--- psphot Radial Profile Wings ---");

    // select the appropriate recipe information
    psMetadata *recipe  = psMetadataLookupPtr (&status, config->recipes, PSPHOT_RECIPE);
    psAssert (recipe, "missing recipe?");

    int num = psphotFileruleCount(config, filerule);

    // skip the chisq image (optionally?)
    int chisqNum = psMetadataLookupS32 (&status, config->arguments, "PSPHOT.CHISQ.NUM");
    if (!status) chisqNum = -1;

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

        if (!psphotRadialProfileWingsReadout (config, recipe, view, readout, sources)) {
            psError (PSPHOT_ERR_CONFIG, false, "failed to measure magnitudes for %s entry %d", filerule, i);
            return false;
        }
    }
    return true;
}

// these are set before we fork off to threads and used by all threads as constant values
static float MAX_RADIUS = NAN;
static float MIN_RADIUS = NAN;
static float SKY_STDEV  = NAN;
static float SKY_SLOPE_MIN = NAN;
// static FILE *file = NULL;

bool psphotRadialProfileWingsReadout(pmConfig *config, psMetadata *recipe, const pmFPAview *view, pmReadout *readout, psArray *sources) {

    bool status = false;

    if (!sources->n) {
        psLogMsg ("psphot", PS_LOG_INFO, "no sources, skipping radial profile wings");
        return true;
    }

    psTimerStart ("psphot.wings");

    // determine the number of allowed threads
    int nThreads = psMetadataLookupS32(&status, config->arguments, "NTHREADS"); // Number of threads
    if (!status) {
        nThreads = 0;
    }

    // XXX is this a good recipe value to use for MAX RADIUS??
    MAX_RADIUS = psMetadataLookupF32 (&status, recipe, "EXT_FIT_MAX_RADIUS");
    if (!status) {
        MAX_RADIUS = 50.0;
    }

    MIN_RADIUS = psMetadataLookupF32 (&status, readout->analysis, "PSF_MOMENTS_RADIUS");
    if (!status || (MIN_RADIUS > MAX_RADIUS)) {
        MIN_RADIUS = psMetadataLookupF32 (&status, recipe, "PSF_MOMENTS_RADIUS");
    }

    // SKY_STDEV is the sigma of the sky model (ie, smoothed on large scales)
    SKY_STDEV = psMetadataLookupF32 (&status, readout->analysis, "MSKY_SIG");
    if (!status) {
	SKY_STDEV = 1.0; // a crude default value (why would this not exist?)
    }

    // SKY_SLOPE_MIN is the sigma of the sky model (ie, smoothed on large scales)
    SKY_SLOPE_MIN = psMetadataLookupF32 (&status, recipe, "SKY_SLOPE_MIN");
    if (!status) {
	SKY_SLOPE_MIN = 3.0; 
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

    // file = fopen ("radii.dat", "w");

    // threaded measurement of the source magnitudes
    // choose Cx, Cy (see psphotThreadTools.c for overview of the concepts)
    int Cx = 1, Cy = 1;
    psphotChooseCellSizes (&Cx, &Cy, readout, nThreads);

    psArray *cellGroups = psphotAssignSources (Cx, Cy, sources);

    for (int i = 0; i < cellGroups->n; i++) {

        psArray *cells = cellGroups->data[i];

        for (int j = 0; j < cells->n; j++) {

            // allocate a job -- if threads are not defined, this just runs the job
            psThreadJob *job = psThreadJobAlloc ("PSPHOT_RADIAL_PROFILE_WINGS");

            psArrayAdd(job->args, 1, readout);
            psArrayAdd(job->args, 1, cells->data[j]); // sources
            PS_ARRAY_ADD_SCALAR(job->args, maskVal,       PS_TYPE_IMAGE_MASK);

// set this to 0 to run without threading
# if (1)
            if (!psThreadJobAddPending(job)) {
                psError(PS_ERR_UNKNOWN, false, "Unable to guess model.");
                return false;
            }
# else
	    if (!psphotRadialProfileWings_Threaded(job)) {
		psError(PS_ERR_UNKNOWN, false, "Unable to guess model.");
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

    // fclose (file);

    psLogMsg ("psphot.wings", PS_LOG_WARN, "measure radial profile wings : %f sec for %ld objects\n", psTimerMark ("psphot.wings"), sources->n);
    return true;
}

bool psphotRadialProfileWings_Threaded (psThreadJob *job) {

    pmReadout *readout              = job->args->data[0];
    psArray *sources                = job->args->data[1];
    psImageMaskType maskVal         = PS_SCALAR_VALUE(job->args->data[2],PS_TYPE_IMAGE_MASK_DATA);

    for (int i = 0; i < sources->n; i++) {

	pmSource *source = sources->data[i];
	if (!source->peak) continue; // XXX how can we have a peak-less source?

	// allocate space for moments
	if (!source->moments) continue;

	// skip saturated stars modeled with a radial profile 
        if (source->mode2 & PM_SOURCE_MODE2_SATSTAR_PROFILE) continue;

	// skip non-detected sources matched from other images
	if (source->mode2 & PM_SOURCE_MODE2_MATCHED) return true; // skip matched sources (no signal)

	// XXX unclear if I should run this analysis on both 1st and 2nd pass for 1st pass objects,
	// or just use the 1st pass value.  I think I should just use the 1st pass value, so skip
	// any that have already been assigned
	if (isfinite(source->skyRadius)) return true;

	// replace object in image
	bool reSubtract = false;
	if (source->tmpFlags & PM_SOURCE_TMPF_SUBTRACTED) {
	    pmSourceAdd (source, PM_MODEL_OP_FULL, maskVal);
	    reSubtract = true;
	}

	// re-allocate image, weight, mask arrays for each peak with box big enough to fit BIG_RADIUS
	// XXX don't measure on subraster images:
	// pmSourceRedefinePixels (source, readout, source->peak->x, source->peak->y, windowRadius + 2);
	// psAssert (source->pixels, "WTF?");

	// this function populates moments->Mrf,KronFlux,KronFluxErr
	psphotRadialProfileWingsSource (source, readout, maskVal);

	// if we subtracted it above, re-subtract the object, leave local sky
	if (reSubtract) {
	    pmSourceSub (source, PM_MODEL_OP_FULL, maskVal);
	}
    }
    return true;
}

# define TEST_X 3158
# define TEST_Y 3096

float InterpolateValues (float X0, float Y0, float X1, float Y1, float X);

// XXX use integer radius values?  the rings assume integer values, right? or do they?
bool psphotRadialProfileWingsSource (pmSource *source, pmReadout *readout, psImageMaskType maskVal) {

    PS_ASSERT_PTR_NON_NULL(source, false);
    PS_ASSERT_PTR_NON_NULL(source->peak, false);
    PS_ASSERT_PTR_NON_NULL(source->pixels, false);

    // psImageMaskType **vMsk = (source->maskObj == NULL) ? NULL : source->maskObj->data.PS_TYPE_IMAGE_MASK_DATA;

    // radii will be MIN_RADIUS to MAX_RADIUS in NN log steps:
    float NSTEP = 25.0;
    float MIN_DR = 2;
    float NSIGMA = 1.0;
    float THRESHOLD = 2.0*SKY_STDEV;
    float alpha = pow ((MAX_RADIUS / MIN_RADIUS), 1.0/NSTEP) - 1.0;
    float dRmax = MAX_RADIUS * alpha / (1.0 + alpha); // approximate size of last annulus, to get a rough size for vector allocation

    int iter = 0;

    psVector *flux = psVectorAllocEmpty(7*MAX_RADIUS*dRmax, PS_TYPE_F32);
    psVector *fluxVar = psVectorAllocEmpty(7*MAX_RADIUS*dRmax, PS_TYPE_F32);

    // should I just use sample median here?
    psStats *fluxStats = psStatsAlloc(PS_STAT_ROBUST_MEDIAN);
    psStats *varStats = psStatsAlloc(PS_STAT_SAMPLE_MEAN);

    float lastFlux = NAN; 
    float lastRadius = NAN; 
    float lastSlope = NAN; 
    bool limit = false;
    float limitRadius = NAN;
    float limitFlux = NAN;
    float limitSlope = NAN;

    // note: radius is the inner radius of the annulus; outer radius = inner radius + dR
    for (float radius = MIN_RADIUS; !limit && (radius < MAX_RADIUS); iter ++) {

	float dR = (int)(radius * alpha);
	if (dR < MIN_DR) dR = MIN_DR;
	float outerRadius = radius + dR;
	float meanRadius = (2.0/3.0) * (outerRadius*outerRadius*outerRadius - radius*radius*radius) / (PS_SQR(outerRadius) - PS_SQR(radius));

	// extract a vector of the pixel values (signal, variance) at this radius + dR
	psphotRadialProfileFluxAtRadius (flux, fluxVar, source, readout, radius, dR, maskVal);

	psStatsInit (fluxStats);
	psStatsInit (varStats);

	if ((flux->n == 0) || (fluxVar->n == 0)) {
	    flux->n = 0;
	    fluxVar->n = 0;
	    radius += dR;
	    continue;
	}

	psVectorStats (fluxStats, flux, NULL, NULL, 0);
	psVectorStats (varStats, fluxVar, NULL, NULL, 0);
	
	// get the mean / median flux statistic and significance here
	float meanFlux = fluxStats->robustMedian;
	float meanFluxError = sqrt(varStats->sampleMean / fluxVar->n);
	// this is a bit crude on the flux error: the error. technically, it should be
	// sqrt(\sum(variance)) / Npts.  I am using the identity \sum(variance) =
	// \mean(variance) * Npts and cancelling the Npts term inside and out of the sqrt()
	
	float slope = NAN;
	if (isfinite(lastFlux)) {
	    slope = (meanFlux - lastFlux) / (meanRadius - lastRadius);
	}

	// fprintf (stderr, "%f %f : %f : %f %f  :  %f\n", source->peak->xf, source->peak->yf, radius, meanFlux, meanFluxError, slope);

	if (!limit && (meanFlux - NSIGMA * meanFluxError < THRESHOLD)) {
	    // dropped to sky level
	    limit = true;
	    // linearly interpolate to the radius at which we hit the sky
	    if (isfinite(lastFlux)) {
		limitRadius = InterpolateValues(lastFlux, lastRadius, meanFlux, meanRadius, 0.0);
	    } else {
		limitRadius = meanRadius;
	    }
	    limitFlux = meanFlux;
	    limitSlope = slope;
	}
	if (!limit && isfinite(slope) && (fabs(slope) < SKY_SLOPE_MIN)) { 
	    // SB no longer changing.	    
	    limit = true;
	    // linearly interpolate to the radius at which we hit the sky, using the last flux and the limiting slope
	    if (isfinite(lastFlux)) {
                float interpolatedRadius = lastRadius + lastFlux / SKY_SLOPE_MIN;
                if (interpolatedRadius < MAX_RADIUS) {
                    limitRadius = interpolatedRadius;
                } else {
                    // XXX should we keep going in this case?
                    limitRadius = meanRadius;
                }
	    } else {
		limitRadius = meanRadius;
	    }
	    limitFlux = meanFlux;
	    limitSlope = slope;
	}

	// completion criteria:
	// 1) flux - NSIGMA * dflux <= sky
	// 2) flux rising?
	// 3) flux flat?

	lastFlux = meanFlux;
	lastRadius = meanRadius;
	lastSlope = slope;

	// reset the flux & fluxVar vector length to zero for re-use above:
	flux->n = 0;
	fluxVar->n = 0;
	radius += dR;
    }

    if (!limit) {
	limitRadius = lastRadius;
	limitFlux = lastFlux;
	limitSlope = lastSlope;
    }
    // fprintf (file, "%f %f : %f %f : %f\n", source->peak->xf, source->peak->yf, limitRadius, limitFlux, limitSlope);

    psFree (flux);
    psFree (fluxVar);
    psFree (fluxStats);
    psFree (varStats);

    source->skyRadius = limitRadius;
    source->skyFlux   = limitFlux;
    source->skySlope  = limitSlope;

    // save the max radius (and anything else?)
    // source->moments->Mrf = Mrf;

    return true;
}

// inline this?  macro this?
bool psphotRadialProfileGetFlux (psVector *flux, psVector *fluxVar, pmReadout *readout, int xc, int yc, psImageMaskType maskVal) {

    int Nx = readout->image->numCols;
    int Ny = readout->image->numRows;

    if (xc < 0) return false;
    if (xc >= Nx) return false;
    if (yc < 0) return false;
    if (yc >= Ny) return false;
    
    if (readout->mask && (readout->mask->data.PS_TYPE_IMAGE_MASK_DATA[yc][xc] & maskVal)) return false;
    psVectorAppend (flux, readout->image->data.F32[yc][xc]);
    psVectorAppend (fluxVar, readout->variance->data.F32[yc][xc]);
    return true;
}

// select the pixels at the given radius and extract the flux and variance into the given vectors
// XXX should radius be 'int'?
bool psphotRadialProfileFluxAtRadius (psVector *flux, psVector *fluxVar, pmSource *source, pmReadout *readout, float Radius, float dRadius, psImageMaskType maskVal) {

    psAssert (flux, "must allocate output flux vector");
    psAssert (fluxVar, "must allocate output fluxVar vector");

    // the peak position is less accurate but less subject to extreme deviations
    float dX = source->moments->Mx - source->peak->xf;
    float dY = source->moments->My - source->peak->yf;
    float dR = hypot(dX, dY);
    float Xo = (dR < 2.0) ? source->moments->Mx : source->peak->xf;
    float Yo = (dR < 2.0) ? source->moments->My : source->peak->yf;

    for (int radius = Radius; radius < Radius + dRadius; radius ++) {

	int x = 0;
	int y = radius;
	int d = 5 - 4*radius;

	while (x <= y) {
	    psphotRadialProfileGetFlux(flux, fluxVar, readout, (Xo + x), (Yo + y), maskVal);
	    psphotRadialProfileGetFlux(flux, fluxVar, readout, (Xo + x), (Yo - y), maskVal);
	    psphotRadialProfileGetFlux(flux, fluxVar, readout, (Xo + y), (Yo + x), maskVal);
	    psphotRadialProfileGetFlux(flux, fluxVar, readout, (Xo - y), (Yo + x), maskVal);
	    
	    if (x > 0) {
		psphotRadialProfileGetFlux(flux, fluxVar, readout, (Xo - x), (Yo + y), maskVal);
		psphotRadialProfileGetFlux(flux, fluxVar, readout, (Xo - x), (Yo - y), maskVal);
		psphotRadialProfileGetFlux(flux, fluxVar, readout, (Xo - y), (Yo - x), maskVal);
		psphotRadialProfileGetFlux(flux, fluxVar, readout, (Xo + y), (Yo - x), maskVal);
	    }
	    
	    if (d < 0) {
		d = d + 8*x + 4;
	    } else {
		d = d + 8*(x-y) + 8;
		y--;
	    }
	    x++;
	}
    }
    return true;
}
