/** @file  pmPSFtryFitPSF.c
 *  @brief Fit the PSF model to the candidate PSF stars (part of PSF model generation)
 *
 *  @author EAM, IfA
 *
 *  @version $Revision: 1.69 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-01-27 06:39:38 $
 *  Copyright 2004 Maui High Performance Computing Center, University of Hawaii
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

# include <pslib.h>
#include "pmHDU.h"
#include "pmFPA.h"
#include "pmFPAMaskWeight.h"

#include "pmTrend2D.h"
#include "pmResiduals.h"
#include "pmGrowthCurve.h"
#include "pmSpan.h"
#include "pmFootprintSpans.h"
#include "pmFootprint.h"
#include "pmPeaks.h"
#include "pmMoments.h"
#include "pmModelFuncs.h"
#include "pmModelClass.h"
#include "pmModel.h"
#include "pmModelUtils.h"
#include "pmSourceMasks.h"
#include "pmSourceExtendedPars.h"
#include "pmSourceDiffStats.h"
#include "pmSourceSatstar.h"
#include "pmSourceLensing.h"
#include "pmSource.h"
#include "pmSourceFitModel.h"
#include "pmPSF.h"
#include "pmPSFtry.h"
#include "pmDetections.h"

#include "pmSourcePhotometry.h"
#include "pmSourceVisual.h"

static int Npsf = 0;

// stage 3: Refit with fixed shape parameters.  This function uses the LMM fitting, but could
// be re-written to use the simultaneous linear fitting (see psphotFitSourcesLinear.c)
bool pmPSFtryFitPSF (pmPSFtry *psfTry, pmPSFOptions *options, psImageMaskType maskVal, psImageMaskType markVal) {

    psTimerStart ("psf.fit");

    // in this segment, we are fitting the fitted PSF model class (shape constrained)
    options->fitOptions->mode = PM_SOURCE_FIT_PSF;

    // maskVal is used to test for rejected pixels, and must include markVal
    maskVal |= markVal;

    Npsf = 0;
    for (int i = 0; i < psfTry->sources->n; i++) {

        pmSource *source = psfTry->sources->data[i];
	psAssert (source->moments, "how can a psf source not have moments?");

        // masked for: bad model fit, outlier in parameters
        if (psfTry->mask->data.PS_TYPE_VECTOR_MASK_DATA[i] & PSFTRY_MASK_ALL) {
            psTrace ("psModules.objects", 4, "dropping %d (%d,%d) : source is masked\n", i, source->peak->x, source->peak->y);
            continue;
        }

	// set shape for this model based on PSF
	psFree (source->modelPSF);
	source->modelPSF = pmModelFromPSF (source->modelEXT, psfTry->psf);
	if (source->modelPSF == NULL) {
	    psfTry->mask->data.PS_TYPE_VECTOR_MASK_DATA[i] = PSFTRY_MASK_BAD_MODEL;
	    psTrace ("psModules.objects", 4, "dropping %d (%d,%d) : bad PSF fit\n", i, source->peak->x, source->peak->y);
	    continue;
	}

	// do some actual work on this source
	psThreadJob *job = psThreadJobAlloc ("PSF_TRY_FIT_PSF");
	psArrayAdd(job->args, 1, source);
	psArrayAdd(job->args, 1, psfTry);
	psArrayAdd(job->args, 1, options);
	
	PS_ARRAY_ADD_SCALAR(job->args, i,        PS_TYPE_S32);

	PS_ARRAY_ADD_SCALAR(job->args, maskVal,  PS_TYPE_IMAGE_MASK);
	PS_ARRAY_ADD_SCALAR(job->args, markVal,  PS_TYPE_IMAGE_MASK);

# if (1)
	if (!psThreadJobAddPending(job)) {
	    psError(PS_ERR_UNKNOWN, false, "Unable to create psf model.");
	    return false;
	}
# else
	if (!pmPSFtryFitPSF_Threaded(job)) {
	    psError(PS_ERR_UNKNOWN, false, "Unable to create psf model.");
	    return false;
	}
	psFree(job);
# endif
    }

    // wait for the threads to finish and manage results
    if (!psThreadPoolWait (false, true)) {
	psError(PS_ERR_UNKNOWN, false, "failure to model psf");
	return false;
    }

    // we have only supplied one type of job, so we can assume the types here
    psThreadJob *job = NULL;
    while ((job = psThreadJobGetDone()) != NULL) {
	// we have no returned data from this operation
	if (job->args->n < 1) fprintf (stderr, "error with job\n");
	psFree(job);
    }
    psfTry->psf->nPSFstars = Npsf;

    pmSourceVisualShowModelFits (psfTry->psf, psfTry->sources, maskVal);

    psLogMsg ("psphot.psftry", PS_LOG_MINUTIA, "fit psf:   %f sec for %d of %ld sources (%d x %d model)\n", psTimerMark ("psf.fit"), Npsf, psfTry->sources->n, psfTry->psf->trendNx, psfTry->psf->trendNy);
    psTrace ("psModules.object", 3, "keeping %d of %ld PSF candidates (PSF)\n", Npsf, psfTry->sources->n);

    if (Npsf == 0) {
#if 0
	// DEBUG code: save the PSF model fit data in detail

	char hostname[256];
	gethostname (hostname, 256);

	int pid = getpid();

	char filename[64];
	snprintf (filename, 64, "psffit.%s.%d.%dx%d.dat", hostname, pid, psfTry->psf->trendNx, psfTry->psf->trendNy);
	FILE *f = fopen (filename, "w");
	psAssert (f, "failed open");

	for (int i = 0; i < psfTry->sources->n; i++) {

	    // skip masked sources
	  // if (psfTry->mask->data.PS_TYPE_VECTOR_MASK_DATA[i] & PSFTRY_MASK_ALL) continue;

	    pmSource *source = psfTry->sources->data[i];

            if (!source->modelPSF) continue;

	    float par7 = (source->modelPSF->params->n == 7) ? -100 : source->modelPSF->params->data.F32[PM_PAR_7];
	    fprintf (f, "%6.1f %6.1f : %6.1f %6.1f : %8.3f %8.3f %8.3f : %f : %f %f %f : %f %d\n",
		     source->peak->xf, source->peak->yf, 
		     source->modelPSF->params->data.F32[PM_PAR_XPOS], source->modelPSF->params->data.F32[PM_PAR_YPOS], 
		     source->psfMag, source->apMag, source->psfMagErr,
		     source->modelPSF->params->data.F32[PM_PAR_I0], 
		     source->modelPSF->params->data.F32[PM_PAR_SXX], source->modelPSF->params->data.F32[PM_PAR_SXY], 
		     source->modelPSF->params->data.F32[PM_PAR_SYY], par7,
		     psfTry->mask->data.PS_TYPE_VECTOR_MASK_DATA[i]);
	}
	fclose (f);
#endif
	psError(PS_ERR_UNKNOWN, false, "No sources with good PSF fits after model is built.");
	return false;
    }

    return true;
}

bool pmPSFtryFitPSF_Threaded (psThreadJob *job) {

    pmSource *source =      job->args->data[0];
    pmPSFtry *psfTry =      job->args->data[1];
    pmPSFOptions *options = job->args->data[2];

    int i = PS_SCALAR_VALUE(job->args->data[3], S32);

    psImageMaskType maskVal = PS_SCALAR_VALUE(job->args->data[4],PS_TYPE_IMAGE_MASK_DATA);
    psImageMaskType markVal = PS_SCALAR_VALUE(job->args->data[5],PS_TYPE_IMAGE_MASK_DATA);

    // PSF fit and aperture mags use different radii
    source->modelPSF->fitRadius = options->fitRadius;
    source->apRadius            = options->apRadius;

    // set object mask to define valid pixels for PSF model fit
    psImageKeepCircle (source->maskObj, source->peak->x, source->peak->y, options->fitRadius, "OR", markVal);

    // fit the PSF model to the source
    bool status = pmSourceFitModel (source, source->modelPSF, options->fitOptions, maskVal);

    // skip poor fits
    if (!status) {
	psImageMaskPixels (source->maskObj, "AND", PS_NOT_IMAGE_MASK(markVal)); // clear the circular mask
	psfTry->mask->data.PS_TYPE_VECTOR_MASK_DATA[i] = PSFTRY_MASK_PSF_FAIL;
	psTrace ("psModules.objects", 4, "dropping %d (%d,%d) : failed PSF fit\n", i, source->peak->x, source->peak->y);
	return true;
    }

    // set object mask to define valid pixels for APERTURE magnitude
    if (options->fitRadius != options->apRadius) {
	psImageMaskPixels (source->maskObj, "AND", PS_NOT_IMAGE_MASK(markVal)); // clear the circular mask
	psImageKeepCircle (source->maskObj, source->peak->x, source->peak->y, options->apRadius, "OR", markVal);
    }

    // This function calculates the psf and aperture magnitudes
    status = pmSourceMagnitudes (source, psfTry->psf, PM_SOURCE_PHOT_INTERP, maskVal, markVal, options->apRadius); // raw PSF mag, AP mag
    psImageMaskPixels (source->maskObj, "AND", PS_NOT_IMAGE_MASK(markVal)); // clear the circular mask

    if (!status || isnan(source->apMag)) {
	psfTry->mask->data.PS_TYPE_VECTOR_MASK_DATA[i] = PSFTRY_MASK_BAD_PHOT;
	psTrace ("psModules.objects", 4, "dropping %d (%d,%d) : poor photometry\n", i, source->peak->x, source->peak->y);
	return true;
    }

    psfTry->fitMag->data.F32[i] = source->psfMag;
    psfTry->metric->data.F32[i] = source->apMag - source->psfMag;
    psfTry->metricErr->data.F32[i] = source->psfMagErr;

    psTrace ("psModules.object", 6, "keeping source %d (%d) of %ld\n", i, Npsf, psfTry->sources->n);
    Npsf ++;
    return true;
}
