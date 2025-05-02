/** @file  pmPSFtry.c
 *
 *  XXX: need description of file purpose
 *
 *  @author EAM, IfA
 *
 *  @version $Revision: 1.69 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-01-27 06:39:38 $
 *  Copyright 2004 Maui High Performance Computing Center, University of Hawaii
 *
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
#include "pmSourceUtils.h"
#include "pmSourceFitModel.h"
#include "pmPSF.h"
#include "pmPSFtry.h"
#include "pmDetections.h"

#include "pmSourcePhotometry.h"
#include "pmSourceVisual.h"

bool pmPSFThreads (void) {

    psThreadTask *task = NULL;

    task = psThreadTaskAlloc("PSF_TRY_FIT_EXT", 6);
    task->function = &pmPSFtryFitEXT_Threaded;
    psThreadTaskAdd(task);
    psFree(task);

    task = psThreadTaskAlloc("PSF_TRY_FIT_PSF", 6);
    task->function = &pmPSFtryFitPSF_Threaded;
    psThreadTaskAdd(task);
    psFree(task);

    return true;
}

static int Next = 0;

// Fit an EXT model to all candidates PSF sources.
// Note: this is independent of the modeled 2D variations in the PSF.
bool pmPSFtryFitEXT (pmPSFtry *psfTry, pmPSFOptions *options, psImageMaskType maskVal, psImageMaskType markVal) {

    psTimerStart ("psf.fit");

    // in this segment, we are fitting the full PSF model class (shape unconstrained)
    options->fitOptions->mode = PM_SOURCE_FIT_EXT;

    // maskVal is used to test for rejected pixels, and must include markVal
    maskVal |= markVal;

    Next = 0;
    for (int i = 0; i < psfTry->sources->n; i++) {

        pmSource *source = psfTry->sources->data[i];

	if (!source->moments) {
	    psfTry->mask->data.PS_TYPE_VECTOR_MASK_DATA[i] = PSFTRY_MASK_EXT_FAIL;
	    psTrace ("psModules.objects", 4, "masking %d (%d,%d) : no moments\n", i, source->peak->x, source->peak->y);
	    continue;
	}
	if (!source->moments->nPixels) {
	    psTrace ("psModules.objects", 4, "masking %d (%d,%d) : no pixels\n", i, source->peak->x, source->peak->y);
	    psfTry->mask->data.PS_TYPE_VECTOR_MASK_DATA[i] = PSFTRY_MASK_EXT_FAIL;
	    continue;
	}
	// If mask object does not exist, mark the source as bad.
	// We cannot proceed with it because psImageMaskPixels leaves an uncleared error code last which causes
	// psphot to exit with a fault. 
	if (source->maskObj == NULL) {
	    psTrace ("psModules.objects", 4, "source %d (%d,%d) : null maskObj\n", i, source->peak->x, source->peak->y);
	    psfTry->mask->data.PS_TYPE_VECTOR_MASK_DATA[i] = PSFTRY_MASK_EXT_FAIL;
	    continue;
	}

	source->modelEXT = pmSourceModelGuess (source, options->type, maskVal, markVal);
	if (source->modelEXT == NULL) {
	    psTrace ("psModules.objects", 4, "masking %d (%d,%d) : failed to generate model guess\n", i, source->peak->x, source->peak->y);
	    psfTry->mask->data.PS_TYPE_VECTOR_MASK_DATA[i] = PSFTRY_MASK_EXT_FAIL;
	    continue;
	}

	// do some actual work on this source
	psThreadJob *job = psThreadJobAlloc ("PSF_TRY_FIT_EXT");
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
	if (!pmPSFtryFitEXT_Threaded(job)) {
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

    psLogMsg ("psphot.psftry", PS_LOG_MINUTIA, "fit ext:   %f sec for %d of %ld sources\n", psTimerMark ("psf.fit"), Next, psfTry->sources->n);
    psTrace ("psModules.object", 3, "keeping %d of %ld PSF candidates (EXT)\n", Next, psfTry->sources->n);

    if (Next == 0) {
        psError(PS_ERR_UNKNOWN, false, "No sources with good extended fits from which to determine PSF.");
        return false;
    }

    return true;
}

bool pmPSFtryFitEXT_Threaded (psThreadJob *job) {

    pmSource *source =      job->args->data[0];
    pmPSFtry *psfTry =      job->args->data[1];
    pmPSFOptions *options = job->args->data[2];

    int i = PS_SCALAR_VALUE(job->args->data[3], S32);

    psImageMaskType maskVal = PS_SCALAR_VALUE(job->args->data[4],PS_TYPE_IMAGE_MASK_DATA);
    psImageMaskType markVal = PS_SCALAR_VALUE(job->args->data[5],PS_TYPE_IMAGE_MASK_DATA);

    // set object mask to define valid pixels
    // XXX 0.5 PIX: is the circle symmetric about the peak coordinate (given 0.5,0.5 center)?
    psImageKeepCircle (source->maskObj, source->peak->x, source->peak->y, options->fitRadius, "OR", markVal);

    // fit model as EXT, not PSF
    bool status = pmSourceFitModel (source, source->modelEXT, options->fitOptions, maskVal);

    // clear object mask to define valid pixels
    psImageMaskPixels (source->maskObj, "AND", PS_NOT_IMAGE_MASK(markVal)); // clear the circular mask

    // exclude the poor fits
    if (!status) {
	psTrace ("psModules.objects", 4, "masking %d (%d,%d) : status is poor\n", i, source->peak->x, source->peak->y);
	psfTry->mask->data.PS_TYPE_VECTOR_MASK_DATA[i] = PSFTRY_MASK_EXT_FAIL;
	return true;
    }
    Next ++;
    
    return true;
}
