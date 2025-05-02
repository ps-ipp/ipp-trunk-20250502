/** @file  pmGrowthCurve.c
 *
 *  Measure the curve-of-growth for sources
 *
 *  @author EAM, IfA
 *
 *  @version $Revision: 1.15 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2008-12-08 02:51:14 $
 *
 *  Copyright 2004 Maui High Performance Computing Center, University of Hawaii
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <pslib.h>

#include "pmHDU.h"
#include "pmFPA.h"

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

#include "psVectorBracket.h"

static void pmGrowthCurveFree (pmGrowthCurve *growth)
{
    if (growth == NULL)
        return;

    psFree (growth->radius);
    psFree (growth->apMag);
    return;
}

bool psMemCheckGrowthCurve(psPtr ptr)
{
    PS_ASSERT_PTR(ptr, false);
    return ( psMemGetDeallocator(ptr) == (psFreeFunc)pmGrowthCurveFree );
}

# define NPTS 25
pmGrowthCurve *pmGrowthCurveAlloc (psF32 minRadius, psF32 maxRadius, psF32 refRadius)
{

    pmGrowthCurve *growth = psAlloc (sizeof(pmGrowthCurve));
    psMemSetDeallocator(growth, (psFreeFunc) pmGrowthCurveFree);

    // set the scaling factor
    float dR = log10(maxRadius / minRadius) / (float) NPTS;
    float fR = pow (10.0, dR);

    // Fractional pixel radii are not well defined; use integer pixel radii.  Use 1 pixel steps
    // until the scaling factor steps in intervals larger than 1 pixel
    // float Rlin = 1.0 / (fR - 1.0);

    growth->radius = psVectorAllocEmpty (NPTS, PS_DATA_F32);
    
    // there will be NPTS radii + a few extras 
    float radius = minRadius;
    while (radius < refRadius) {
	// fprintf (stderr, "r: %f\n", radius);
	psVectorAppend (growth->radius, radius - 0.001);
	radius += 1.0;
    }    
    growth->refBin = growth->radius->n - 1;
    while (radius < maxRadius) {
	// fprintf (stderr, "r: %f\n", radius);
	psVectorAppend (growth->radius, radius - 0.001);
	radius *= fR;
	radius = (int) (radius + 0.5);
    }    
    psVectorAppend (growth->radius, radius);
    growth->apMag  = psVectorAlloc (growth->radius->n, PS_TYPE_F32);

    // XXX may want to extend this to allow for a different refRadius;
    growth->refRadius = refRadius;
    growth->maxRadius = maxRadius;
    growth->fitMag = NAN;
    growth->refMag = NAN;
    growth->apRef  = NAN;
    growth->apLoss = NAN;

    return growth;
}

psF32 pmGrowthCurveCorrect(pmGrowthCurve *growth, psF32 radius)
{
    PS_ASSERT_PTR_NON_NULL(growth, NAN);
    float apRad = psVectorInterpolate (growth->radius, growth->apMag, radius);
    float apCor = growth->apRef - apRad;
    return apCor;
}
