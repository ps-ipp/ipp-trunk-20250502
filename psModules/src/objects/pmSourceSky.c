/** @file  pmSourceSky.c
 *
 *  Functions to measure the local sky and sky variance for sources on images
 *
 *  @author GLG, MHPCC
 *  @author EAM, IfA: significant modifications.
 *
 *  @version $Revision: 1.20 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-06 02:31:25 $
 *
 *  Copyright 2004 Maui High Performance Computing Center, University of Hawaii
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <math.h>
#include <string.h>
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

#include "pmSourceSky.h"

/******************************************************************************
pmSource *pmSourceLocalSky(source, statsOptions, Radius): this
routine creates a new pmSource.moments element if needed and sets pmSource.pmMoments.sky

The sky value is set from the pixels in the square annulus surrounding the
peak pixel.

The source.pixels and source.mask must already exist

This function modifies the source mask; it should only be called before the object aperture is defined
*****************************************************************************/

bool pmSourceLocalSky(
    pmSource *source,
    psStatsOptions statsOptions,
    psF32 Radius,
    psImageMaskType maskVal,
    psImageMaskType markVal)
{
    psTrace("psModules.objects", 10, "---- %s() begin ----\n", __func__);
    PS_ASSERT_PTR_NON_NULL(source, false);
    PS_ASSERT_IMAGE_NON_NULL(source->pixels, false);
    PS_ASSERT_IMAGE_NON_NULL(source->maskObj, false);
    PS_ASSERT_PTR_NON_NULL(source->peak, false);
    PS_ASSERT_INT_POSITIVE(Radius, false);

    psStatsOptions statistic = psStatsSingleOption(statsOptions);
    if (statistic == 0) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, false, "Multiple or no statistics specified: %x\n", statsOptions);
        return NULL;
    }

    psImage *image = source->pixels;
    psImage *mask  = source->maskObj;
    pmPeak *peak  = source->peak;
    psRegion srcRegion;

    // maskVal is used to test for rejected pixels, and must include markVal
    maskVal |= markVal;

    srcRegion = psRegionForSquare(peak->x, peak->y, Radius);
    srcRegion = psRegionForImage(mask, srcRegion);

    psImageMaskRegion(mask, srcRegion, "OR", markVal);
    psStats *myStats = psStatsAlloc(statsOptions);
    if (!psImageStats(myStats, image, mask, maskVal)) {
        psError(PS_ERR_UNKNOWN, false, "Unable to get image statistics.\n");
        psFree(myStats);
        return false;
    }
    psImageMaskRegion(mask, srcRegion, "AND", PS_NOT_IMAGE_MASK(markVal));
    double value = psStatsGetValue(myStats, statistic);
    psFree(myStats);

    if (isnan(value)) {
        psTrace("psModules.objects", 10, "---- %s(false) end ----\n", __func__);
        return(false);
    }
    if (source->moments == NULL) {
        source->moments = pmMomentsAlloc();
    }
    source->moments->Sky = value;
    psTrace("psModules.objects", 10, "---- %s(true) end ----\n", __func__);
    return (true);
}

// A complementary function to pmSourceLocalSky: calculate the local median variance
bool pmSourceLocalSkyVariance(
    pmSource *source,
    psStatsOptions statsOptions,
    psF32 Radius,
    psImageMaskType maskVal,
    psImageMaskType markVal
)
{
    psTrace("psModules.objects", 10, "---- %s() begin ----\n", __func__);
    PS_ASSERT_PTR_NON_NULL(source, false);
    PS_ASSERT_IMAGE_NON_NULL(source->maskObj, false);
    PS_ASSERT_PTR_NON_NULL(source->peak, false);
    PS_ASSERT_INT_POSITIVE(Radius, false);

    if (source->variance == NULL) {
      // XXX this is needed so psphotCheckRadiusPSF has a real value
      source->moments->dSky = 1.0;
      return true;
    }

    // maskVal is used to test for rejected pixels, and must include markVal
    maskVal |= markVal;

    psStatsOptions statistic = psStatsSingleOption(statsOptions);
    if (statistic == 0) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, false, "Multiple or no statistics specified: %x\n", statsOptions);
        return NULL;
    }

    psImage *image = source->variance;
    psImage *mask  = source->maskObj;
    pmPeak *peak  = source->peak;
    psRegion srcRegion;

    srcRegion = psRegionForSquare(peak->x, peak->y, Radius);
    srcRegion = psRegionForImage(mask, srcRegion);

    psImageMaskRegion(mask, srcRegion, "OR", markVal);
    psStats *myStats = psStatsAlloc(statsOptions);
    if (!psImageStats(myStats, image, mask, maskVal)) {
        psError(PS_ERR_UNKNOWN, false, "Unable to get image statistics.\n");
        psFree(myStats);
        return false;
    }
    psImageMaskRegion(mask, srcRegion, "AND", PS_NOT_IMAGE_MASK(markVal));
    double value = psStatsGetValue(myStats, statistic);
    psFree(myStats);

    if (isnan(value)) {
        psTrace("psModules.objects", 10, "---- %s(false) end ----\n", __func__);
        return(false);
    }
    if (source->moments == NULL) {
        source->moments = pmMomentsAlloc();
    }
    source->moments->dSky = value;
    psTrace("psModules.objects", 10, "---- %s(true) end ----\n", __func__);
    return (true);
}
