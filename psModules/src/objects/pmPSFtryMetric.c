/** @file  pmPSFtry.c
 *  @brief: measure the systematic error in the aperture-psf magnitude
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

// The quality of the PSF model is determined by the systematic scatter in the fit - aperture
// magnitude.  We use the moments->Sum value, calculated with a Gaussian window, as a proxy for
// the aperture magnitude.  We measure the systmatic scatter with the function
// psVectorSystematicError which solves for the value of SysErr that needs to be added in
// quadrature to the errors of a set of residual measurements in order to yield a ChiSq of 1.0.
bool pmPSFtryMetric (pmPSFtry *psfTry)
{
    PS_ASSERT_PTR_NON_NULL(psfTry, false);
    PS_ASSERT_PTR_NON_NULL(psfTry->sources, false);

    psStats *stats = psStatsAlloc (PS_STAT_SAMPLE_MEDIAN);

    if (!psVectorStats (stats, psfTry->metric, NULL, psfTry->mask, PSFTRY_MASK_ALL)) {
        psError(PS_ERR_UNKNOWN, false, "Failed to measured clipped mean");
	psFree (stats);
        return false;
    }

    // generate a residual vector
    psVector *resid = psVectorAllocEmpty (psfTry->metric->n, PS_TYPE_F32);
    psVector *error = psVectorAllocEmpty (psfTry->metric->n, PS_TYPE_F32);
    int n = 0;
    for (int i = 0; i < psfTry->metric->n; i++) {
	if (psfTry->mask && (psfTry->mask->data.PS_TYPE_VECTOR_MASK_DATA[i] && PSFTRY_MASK_ALL)) continue;
	// if (psfTry->metricErr->data.F32[i] > 0.005) continue;
	resid->data.F32[n] = psfTry->metric->data.F32[i] - stats->sampleMedian;
	error->data.F32[n] = psfTry->metricErr->data.F32[i];
	n++;
    }
    resid->n = error->n = n;

    float psfSysErr = psVectorSystematicError (resid, error, 0.05);

    psLogMsg ("pmPSFtryMetric", 4, "apresid: %f +/- %f (systematic error) from statistics of %ld psf stars (%d used)\n", 
	      stats->sampleMedian, psfSysErr, psfTry->sources->n, n);

    psfTry->psf->ApResid  = stats->sampleMedian;
    psfTry->psf->dApResid = psfSysErr;

    pmSourceVisualPlotPSFMetric (psfTry);
    pmSourceVisualPlotPSFMetricSubpix (psfTry);

    psFree (stats);
    psFree (resid);
    psFree (error);

    return true;
}
