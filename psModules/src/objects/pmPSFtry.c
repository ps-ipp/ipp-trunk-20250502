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
#include "pmSourceFitModel.h"
#include "pmPSF.h"
#include "pmPSFtry.h"
#include "pmDetections.h"

#include "pmSourcePhotometry.h"
#include "pmSourceVisual.h"

// ********  pmPSFtry functions  **************************************************
// * pmPSFtry holds a single pmPSF model test, with the input sources, the freely
// * fitted version of the model, the pmPSF fit to the fitted model parameters,
// * and the PSF fits to the source. It also includes the statistics from the
// * fits, both the individual sources, and the collection

// free a pmPSFtry structure
static void pmPSFtryFree (pmPSFtry *test)
{
    if (test == NULL) return;

    psFree (test->psf);
    psFree (test->sources);
    psFree (test->metric);
    psFree (test->metricErr);
    psFree (test->fitMag);
    psFree (test->mask);
    return;
}

// allocate a pmPSFtry based on the desired sources and the model (identified by name)
pmPSFtry *pmPSFtryAlloc (const psArray *sources, const pmPSFOptions *options)
{
    pmPSFtry *test = (pmPSFtry *) psAlloc(sizeof(pmPSFtry));
    psMemSetDeallocator(test, (psFreeFunc) pmPSFtryFree);

    test->psf       = NULL; 
    test->metric    = psVectorAlloc (sources->n, PS_TYPE_F32);
    test->metricErr = psVectorAlloc (sources->n, PS_TYPE_F32);
    test->fitMag    = psVectorAlloc (sources->n, PS_TYPE_F32);
    test->mask      = psVectorAlloc (sources->n, PS_TYPE_VECTOR_MASK);

    psVectorInit (test->mask,        0);
    psVectorInit (test->metric,    0.0);
    psVectorInit (test->metricErr, 0.0);
    psVectorInit (test->fitMag,    0.0);

    test->sources   = psArrayAlloc (sources->n);

    for (int i = 0; i < sources->n; i++) {
	pmSource *sourceOld = sources->data[i];
	pmSource *sourceNew = pmSourceCopy (sourceOld);

	// save a reference so we can get back to the original
	// this is specifically used in psphotChooosePSF to unflag the candidate PSF sources
	// which were not actually used to generate a PSF model
	sourceNew->parent = sourceOld; 

        test->sources->data[i] = sourceNew;
    }

    return (test);
}

bool psMemCheckPSFtry(psPtr ptr)
{
    PS_ASSERT_PTR(ptr, false);
    return ( psMemGetDeallocator(ptr) == (psFreeFunc) pmPSFtryFree);
}

float psVectorSystematicError (psVector *residuals, psVector *errors, float clipFraction) {

    psAssert(residuals, "residuals cannot be NULL");
    psAssert(errors, "errors cannot be NULL");
    psAssert(residuals->n == errors->n, "residuals and errors must be the same length");

    // given a vector of residuals and their formal errors, calculated the necessary systematic
    // error needed to yield a reduced chisq of 1.0, after first tossing out the clipFraction
    // highest chi-square contributors (allowed outliers)

    psVector *mask  = psVectorAlloc(residuals->n, PS_TYPE_VECTOR_MASK);
    psVector *chisq = psVectorAlloc(residuals->n, PS_TYPE_F32);

    // calculate the chisq vector:
    int Ngood = 0;
    for (int i = 0; i < residuals->n; i++) {
	chisq->data.F32[i] = PS_MAX_F32;
	if (!isfinite(residuals->data.F32[i])) continue;
	if (!isfinite(errors->data.F32[i])) continue;
	if (errors->data.F32[i] <= 0.0) continue;
	chisq->data.F32[i] = PS_SQR(residuals->data.F32[i] / errors->data.F32[i]);
	Ngood ++;
    }

    psVector *index = psVectorSortIndex(NULL, chisq);

    // toss out the clipFraction highest chisq values
    for (int i = 0; i < residuals->n; i++) {
	int n = index->data.S32[i];
	if (i < (1.0 - clipFraction)*Ngood) {
	    mask->data.PS_TYPE_VECTOR_MASK_DATA[n] = 0;
	} else {
	    mask->data.PS_TYPE_VECTOR_MASK_DATA[n] = 1;
	}
    }

    // Ndof ~= Ngood
    // Chisq_Ndof = sum(residuals_i^2 / error_i^2) / Ndof
    // choose S2 such than Chisq^sys_Ndof = sum(residuals_i^2 / (error_i^2 + S2)) / Ndof = 1.0
    
    // use Newton-Raphson to solve for S2:

    // use median sigma to calculate the initial guess for S2:
    psStats *stats = psStatsAlloc(PS_STAT_SAMPLE_MEDIAN);
    psVectorStats (stats, errors, NULL, mask, 1);
    float errorMedian = stats->sampleMedian;
    
    float nPts = 0.0;
    float res2mean = 0.0;
    float ChiSq = 0.0;
    for (int i = 0; i < residuals->n; i++) {
	int n = index->data.S32[i];
	if (mask->data.PS_TYPE_VECTOR_MASK_DATA[n]) continue;
	res2mean += PS_SQR(residuals->data.F32[n]);
	ChiSq += PS_SQR(residuals->data.F32[n]) / PS_SQR(errors->data.F32[n]);
	nPts += 1.0;
    }
    res2mean /= nPts;
    ChiSq /= nPts;
    
    float S2guess = res2mean - PS_SQR(errorMedian);

    psLogMsg ("psModules", 10, "ChiSquare: %f, Ntotal: %ld, Ngood: %d, Nkeep: %.0f, S2 guess: %f\n", 
	      ChiSq, residuals->n, Ngood, nPts, S2guess);

    for (int iter = 0; iter < 10; iter++) {

	ChiSq = 0.0;
	float dRdS = 0.0;
	for (int i = 0; i < residuals->n; i++) {
	    int n = index->data.S32[i];
	    if (mask->data.PS_TYPE_VECTOR_MASK_DATA[n]) continue;
	    float error2 = PS_SQR(errors->data.F32[n]) + S2guess;
	    ChiSq += PS_SQR(residuals->data.F32[n]) / error2;
	    dRdS += PS_SQR(residuals->data.F32[n]) / PS_SQR(error2);
	}
	ChiSq /= nPts;
	dRdS /= nPts;

	// Note the sign on dS: dRdS above is -1 * dR/dS formally
	float dS = (ChiSq - 1.0) / dRdS;
	S2guess += dS;
	S2guess = PS_MAX(0.0, S2guess);

	psLogMsg ("psModules", 10, "ChiSquare: %f, dS: %f, S2 guess: %f\n", ChiSq, dS, S2guess);
    }

    // free local allocations
    psFree (mask);
    psFree (chisq);
    psFree (stats);
    psFree (index);

    return (sqrt(S2guess));
}

