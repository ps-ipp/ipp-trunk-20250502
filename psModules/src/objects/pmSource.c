/** @file  pmSource.c
 *
 *  Functions to define and manipulate sources on images
 *
 *  @author EAM, IfA
 *  @author GLG, MHPCC (initial code base)
 *
 *  @version $Revision: 1.70 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-16 22:29:59 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <strings.h>
#include <pslib.h>
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
#include "pmSourcePhotometry.h"
#include "pmSourceSatstar.h"
#include "pmSourceLensing.h"
#include "pmSource.h"

static void sourceFree(pmSource *tmp)
{
    if (!tmp)
        return;

    psTrace("psModules.objects", 10, "---- begin ----\n");
    psFree(tmp->peak);
    psFree(tmp->pixels);
    psFree(tmp->variance);
    psFree(tmp->modelVar);
    psFree(tmp->maskObj);
    psFree(tmp->maskView);
    psFree(tmp->modelFlux);
    psFree(tmp->psfImage);
    psFree(tmp->moments);
    psFree(tmp->modelPSF);
    psFree(tmp->modelEXT);
    psFree(tmp->modelFits);
    psFree(tmp->extFitPars);
    psFree(tmp->blends);
    psFree(tmp->satstar);
    psFree(tmp->extpars);
    psFree(tmp->diffStats);
    psFree(tmp->galaxyFits);
    psFree(tmp->radialAper);
    psFree(tmp->lensingOBJ);
    psFree(tmp->lensingPSF);
    psTrace("psModules.objects", 10, "---- end ----\n");
}

// free only the pixel data associated with this source
void pmSourceFreePixels(pmSource *source)
{

    if (!source)
        return;

    psFree (source->pixels);
    psFree (source->variance);
    psFree (source->modelVar);
    psFree (source->maskObj);
    psFree (source->maskView);
    psFree (source->modelFlux);
    psFree (source->psfImage);

    source->pixels = NULL;
    source->variance = NULL;
    source->modelVar = NULL;
    source->maskObj = NULL;
    source->maskView = NULL;
    source->modelFlux = NULL;
    source->psfImage = NULL;
    return;
}

bool psMemCheckSource(psPtr ptr)
{
    PS_ASSERT_PTR(ptr, false);
    return ( psMemGetDeallocator(ptr) == (psFreeFunc) sourceFree);
}

/******************************************************************************
pmSourceAlloc(): Allocate the pmSource structure and initialize its members
to NULL.
*****************************************************************************/

pmSource *pmSourceAlloc()
{
    psTrace("psModules.objects", 10, "---- begin ----\n");
    static int id = 1;
    pmSource *source = (pmSource *) psAlloc(sizeof(pmSource));
    P_PM_SOURCE_SET_ID(source, id++);

    source->seq = -1;
    source->peak = NULL;
    source->pixels = NULL;
    source->variance = NULL;
    source->modelVar = NULL;
    source->maskObj = NULL;
    source->maskView = NULL;
    source->modelFlux = NULL;
    source->psfImage = NULL;
    source->moments = NULL;
    source->modelPSF = NULL;
    source->modelEXT = NULL;
    source->modelFits = NULL;
    source->extFitPars = NULL;

    source->type = PM_SOURCE_TYPE_UNKNOWN;
    source->mode = PM_SOURCE_MODE_DEFAULT;
    source->mode2 = PM_SOURCE_MODE_DEFAULT;
    source->tmpFlags = 0;

    // default values are NAN
    source->psfMag     	     = NAN;
    source->psfMagErr 	     = NAN;
    source->psfFlux    	     = NAN;
    source->psfFluxErr 	     = NAN;
    source->extMag 	     = NAN;    
    source->apMag  	     = NAN;
    source->apMagRaw  	     = NAN;
    source->apRadius  	     = NAN;
    source->apNpixels  	     = 0;
    source->apFlux    	     = NAN;
    source->apFluxErr 	     = NAN; 

    source->windowRadius     = NAN;
    source->skyRadius  	     = NAN;
    source->skyFlux    	     = NAN;
    source->skySlope   	     = NAN;

    source->pixWeightNotBad  = NAN;
    source->pixWeightNotPoor = NAN;

    source->psfChisq         = NAN;
    source->crNsigma         = NAN;
    source->extNsigma        = NAN;
    source->sky    	     = NAN;
    source->skyErr 	     = NAN;    
    source->extSN  	     = NAN;    

    source->region = psRegionSet(NAN, NAN, NAN, NAN);
    source->blends = NULL;
    source->satstar = NULL;
    source->extpars = NULL;
    source->diffStats = NULL;
    source->galaxyFits = NULL;
    source->lensingOBJ = NULL;
    source->lensingPSF = NULL;
    source->radialAper = NULL;
    source->parent = NULL;
    source->tmpPtr = NULL;
    source->chipNum = -1;
    source->chipX = -1000;
    source->chipY = -1000;
    source->imageID = -1;
    source->nFrames = 0;

    psMemSetDeallocator(source, (psFreeFunc) sourceFree);

    psTrace("psModules.objects", 10, "---- end ----\n");
    return(source);
}

/******************************************************************************
pmSourceCopy(): copy the pmSource, yielding a copy of the source that can be used without
affecting the original.  This Copy can be used to allow multiple fit attempts on the same
object.  The pixels, variance, and mask arrays all point to the same original subarrays.  The
peak and moments point at the original values.  The models, blends, and XXX are NOT copied
*****************************************************************************/
pmSource *pmSourceCopy(pmSource *in)
{
    if (in == NULL) {
        return(NULL);
    }
    pmSource *source = pmSourceAlloc ();

    // peak has the same values as the original
    if (in->peak != NULL) {
        source->peak = pmPeakAlloc (in->peak->x, in->peak->y, in->peak->detValue, in->peak->type);
	pmPeakCopy(source->peak, in->peak);
    }

    // copy the values in the moments structure
    if (in->moments != NULL) {
        source->moments  =  pmMomentsAlloc();
        *source->moments = *in->moments;
    }

    // These images are all views to the parent.  We want a new view, but pointing at the same
    // pixels.  Modifying these pixels (ie, subtracting the model) will affect the pixels seen
    // by all copies.
    source->pixels   = in->pixels   ? psImageCopyView(NULL, in->pixels)   : NULL;
    source->variance = in->variance ? psImageCopyView(NULL, in->variance) : NULL;
    source->modelVar = NULL;
    source->maskView = in->maskView ? psImageCopyView(NULL, in->maskView) : NULL;

    // the maskObj is a unique mask array; create a new mask image
    source->maskObj = in->maskObj   ? psImageCopy (NULL, in->maskObj, PS_TYPE_IMAGE_MASK) : NULL;

    // NOTE : because of the const id element, we cannot just assign *source = *in

    source->imageID          = in->imageID;

    source->type     	     = in->type;
    source->mode     	     = in->mode;
    source->mode2    	     = in->mode2;
    source->tmpFlags 	     = in->tmpFlags;

    source->psfMag     	     = in->psfMag;
    source->psfMagErr 	     = in->psfMagErr;
    source->psfFlux    	     = in->psfFlux;
    source->psfFluxErr 	     = in->psfFluxErr;
    source->extMag 	     = in->extMag;
    source->apMag  	     = in->apMag;
    source->apMagRaw  	     = in->apMagRaw;
    source->apRadius  	     = in->apRadius;
    source->apNpixels  	     = in->apNpixels;
    source->apFlux    	     = in->apFlux;
    source->apFluxErr 	     = in->apFluxErr;

    source->windowRadius     = in->windowRadius;
    source->skyRadius  	     = in->skyRadius;  	
    source->skyFlux    	     = in->skyFlux;    	
    source->skySlope   	     = in->skySlope;   	

    source->pixWeightNotBad  = in->pixWeightNotBad;
    source->pixWeightNotPoor = in->pixWeightNotPoor;

    source->psfChisq         = in->psfChisq;
    source->crNsigma         = in->crNsigma;
    source->extNsigma        = in->extNsigma;
    source->sky    	     = in->sky;
    source->skyErr 	     = in->skyErr;

    source->region           = in->region;

    // XXX I am not copying the pointers to things like the blends, satstar profile, galaxyFits, etc

    return(source);
}

// x,y are defined in the parent image coords of readout->image
bool pmSourceDefinePixels(pmSource *mySource,
                          const pmReadout *readout,
                          psF32 x,
                          psF32 y,
                          psF32 Radius)
{
    PS_ASSERT_PTR_NON_NULL(mySource, false);
    PS_ASSERT_PTR_NON_NULL(readout, false);
    PS_ASSERT_PTR_NON_NULL(readout->image, false);
    PS_ASSERT_INT_POSITIVE(Radius, false);

    psRegion sourceRegion;

    // Grab a subimage of the original image of size (2 * outerRadius).
    sourceRegion = psRegionForSquare (x, y, Radius);
    sourceRegion = psRegionForImage (readout->image, sourceRegion);

    // these images are subset images of the equivalent parents
    mySource->pixels = psImageSubset(readout->image, sourceRegion);
    if (readout->variance) {
        mySource->variance = psImageSubset(readout->variance, sourceRegion);
    }
    if (readout->mask) {
        mySource->maskView = psImageSubset(readout->mask,  sourceRegion);
        // the object mask is a copy, and used to define the source pixels
        mySource->maskObj = psImageCopy(NULL, mySource->maskView, PS_TYPE_IMAGE_MASK);
    }
    mySource->region   = sourceRegion;
    mySource->windowRadius = Radius;

    return true;
}

bool pmSourceRedefinePixels(pmSource *mySource,
                            const pmReadout *readout,
                            psF32 x,
                            psF32 y,
                            psF32 Radius)
{
    PS_ASSERT_PTR_NON_NULL(mySource, false);
    PS_ASSERT_PTR_NON_NULL(readout, false);
    PS_ASSERT_PTR_NON_NULL(readout->image, false);
    PS_ASSERT_INT_POSITIVE(Radius, false);

    bool extend;
    psRegion newRegion;

    // check to see if new region is completely contained within old region
    newRegion = psRegionForSquare (x, y, Radius);
    newRegion = psRegionForImage (readout->image, newRegion);

    // re-define if required by region or absence of pixels
    extend = false;
    extend |= (int)(newRegion.x0) < (int)(mySource->region.x0);
    extend |= (int)(newRegion.x1) > (int)(mySource->region.x1);
    extend |= (int)(newRegion.y0) < (int)(mySource->region.y0);
    extend |= (int)(newRegion.y1) > (int)(mySource->region.y1);

    extend |= (mySource->pixels == NULL);
    extend |= (mySource->variance == NULL);
    extend |= (mySource->maskObj == NULL);
    extend |= (mySource->maskView == NULL);

    // if ((fabs(x - 2020) < 5) && (fabs(y - 366) < 5)) {
    // 	if (extend) {
    // 	    fprintf (stderr, "extend T, %f, %f : %f, %f vs %f, %f : %f, %f\n", 
    // 		     newRegion.x0, newRegion.y0, newRegion.x1, newRegion.y1, 
    // 		     mySource->region.x0, mySource->region.y0, mySource->region.x1, mySource->region.y1);
    // 	} else {
    // 	    fprintf (stderr, "extend F, %f, %f : %f, %f vs %f, %f : %f, %f\n", 
    // 		     newRegion.x0, newRegion.y0, newRegion.x1, newRegion.y1, 
    // 		     mySource->region.x0, mySource->region.y0, mySource->region.x1, mySource->region.y1);
    // 	}
    // }

    if (extend) {
        // re-create the subimage
        psFree (mySource->pixels);
        psFree (mySource->variance);
        psFree (mySource->maskView);

        mySource->pixels   = psImageSubset(readout->image,  newRegion);
        mySource->variance = psImageSubset(readout->variance, newRegion);
        mySource->maskView = psImageSubset(readout->mask,   newRegion);
        mySource->region   = newRegion;

        // re-copy the main mask pixels.  NOTE: the user will need to reset the object mask
        // pixels (eg, with psImageKeepCircle)
        mySource->maskObj = psImageCopy (mySource->maskObj, mySource->maskView, PS_TYPE_IMAGE_MASK);

        // drop the old modelFlux pixels and force the user to re-create
        psFree (mySource->modelFlux);
        mySource->modelFlux = NULL;

        // drop the old psfImage pixels and force the user to re-create
        psFree (mySource->psfImage);
        mySource->psfImage = NULL;
    }
    mySource->windowRadius = Radius;
    return extend;
}

bool pmSourceRedefinePixelsByRegion(pmSource *mySource,
				    const pmReadout *readout,
				    psRegion newRegion)
{
    PS_ASSERT_PTR_NON_NULL(mySource, false);
    PS_ASSERT_PTR_NON_NULL(readout, false);
    PS_ASSERT_PTR_NON_NULL(readout->image, false);

    // re-create the subimage
    psFree (mySource->pixels);
    psFree (mySource->variance);
    psFree (mySource->maskView);
	
    mySource->pixels   = psImageSubset(readout->image,    newRegion);
    mySource->variance = psImageSubset(readout->variance, newRegion);
    mySource->maskView = psImageSubset(readout->mask,     newRegion);
    mySource->region   = newRegion;

    // re-copy the main mask pixels.  NOTE: the user will need to reset the object mask
    // pixels (eg, with psImageKeepCircle)
    mySource->maskObj = psImageCopy (mySource->maskObj, mySource->maskView, PS_TYPE_IMAGE_MASK);

    // drop the old modelFlux pixels and force the user to re-create
    psFree (mySource->modelFlux);
    mySource->modelFlux = NULL;

    // drop the old psfImage pixels and force the user to re-create
    psFree (mySource->psfImage);
    mySource->psfImage = NULL;

    return true;
}

/******************************************************************************
    pmSourcePSFClump(source, recipe): Find the likely PSF clump in the
    sigma-x, sigma-y plane. return 0,0 clump in case of error.
*****************************************************************************/

pmPSFClump pmSourcePSFClump(psImage **savedImage, psRegion *region, psArray *sources, float PSF_SN_LIM, float PSF_CLUMP_GRID_SCALE, psF32 SX_MAX, psF32 SY_MAX, psF32 SX_MIN, psF32 SY_MIN, psF32 AR_MAX)
{
    psTrace("psModules.objects", 10, "---- begin ----\n");

    psArray *peaks  = NULL;
    pmPSFClump errorClump = {-1.0, -1.0, 0.0, 0.0, 0, 0.0};
    pmPSFClump emptyClump = {+0.0, +0.0, 0.0, 0.0, 0, 0.0};
    pmPSFClump psfClump;

    PS_ASSERT_PTR_NON_NULL(sources, errorClump);

    // find the sigmaX, sigmaY clump
    {
        psF32 AR_MIN = 1.0 / AR_MAX;

        // construct a sigma-plane image
        int numCols = 1 + SX_MAX / PSF_CLUMP_GRID_SCALE; // Size of sigma-plane image
	int numRows = 1 + SY_MAX / PSF_CLUMP_GRID_SCALE; // Size of sigma-plane image
        psTrace("psModules.objects", 10, "sigma-plane dimensions: %dx%d\n", numCols, numRows);
        psImage *splane = psImageAlloc(numCols, numRows, PS_TYPE_F32); // sigma-plane image
        psImageInit(splane, 0);

        // place the sources in the sigma-plane image (ignore 0,0 values?)
        int nValid = 0;                 // Number of valid sources
        for (int i = 0; i < sources->n; i++) {
            pmSource *source = sources->data[i]; // Source of interest
            if (!source || !source->moments) {
                continue;
            }

	    if (region) {
		int x = source->peak->x, y = source->peak->y; // Coordinates of peak
		if (x < region->x0 || x > region->x1 || y < region->y0 || y > region->y1) {
		    continue;
		}
	    }

            if (source->mode & PM_SOURCE_MODE_BLEND) {
                continue;
            }

            if (!source->moments->nPixels) continue;

            if (source->moments->SN < PSF_SN_LIM) {
                psTrace("psModules.objects", 10, "Rejecting source from clump because of low S/N (%f)\n",
                        source->moments->SN);
                continue;
            }

            float Mxx = source->moments->Mxx, Myy = source->moments->Myy; // Second moments
            float ar = Mxx / Myy;       // Radius

            if (!isfinite(Mxx) || !isfinite(Myy)) {
                psTrace("psModules.objects", 10,
                        "Rejecting source from clump because of non-finite moments (%f,%f)\n",
                        Mxx, Myy);
                continue;
            }

            // Sx,Sy are limited at 0.  a peak at 0,0 is artificial
            if (fabs(Mxx) < SX_MIN || fabs(Myy < SY_MIN)) {
                psTrace("psModules.objects", 10,
                        "Rejecting source from clump because of low moments (%f,%f)\n",
                        Mxx, Myy);
                continue;
            }
            if (Mxx > SX_MAX || Myy > SY_MAX) {
                psTrace("psModules.objects", 10,
                        "Rejecting source from clump because of high moments (%f,%f)\n",
                        Mxx, Myy);
                continue;
            }
            if (ar > AR_MAX || ar < AR_MIN) {
                psTrace("psModules.objects", 10, "Rejecting source from clump because of Ar (%f)\n", ar);
                continue;
            }

            // for the moment, force splane dimensions to be 10x10 image pix
            int binX = Mxx / PSF_CLUMP_GRID_SCALE, binY = Myy /  PSF_CLUMP_GRID_SCALE; // Position on splane
            psAssert(binX >= 0 && binX < numCols && binY >= 0 && binY < numRows, "We checked it already");

            splane->data.F32[binY][binX] += 1.0;
            nValid++;
        }

        // find the peak in this image
        psStats *stats = psStatsAlloc (PS_STAT_MAX | PS_STAT_SAMPLE_STDEV);
        if (!psImageStats (stats, splane, NULL, 0)) {
            psError(PS_ERR_UNKNOWN, false, "Unable to get image statistics.\n");
            psFree(stats);
            psFree(splane);
            return emptyClump;
        }
        peaks = pmPeaksInImage (splane, stats[0].max / 2);
        psTrace ("psModules.objects", 2, "clump threshold is %f\n", stats[0].max/2);

	psfClump.nSigma = stats->sampleStdev;
	psfClump.nTotal = nValid;

	if (savedImage) {
	    *savedImage = psMemIncrRefCounter(splane);
        }
        psFree (splane);
        psFree (stats);

        // if we failed to find a valid peak, return the empty clump (failure signal)
	if (peaks == NULL) {
            psError(PS_ERR_UNKNOWN, false, "failure in peak analysis for PSF clump.\n");
	    psFree (peaks);
            return emptyClump;
	}

        if (peaks->n == 0)
        {
            psLogMsg ("psphot", 3, "failed to find a peak in the PSF clump image\n");
            if (nValid == 0) {
                psLogMsg ("psphot", 3, "no valid sources kept for PSF search\n");
            } else {
                psLogMsg ("psphot", 3, "no significant peak\n");
            }
	    psFree (peaks);
            return (emptyClump);
        }
    }

    // measure statistics on Sx, Sy if Sx, Sy within range of clump
    {
        pmPeak *clump;
        psF32 minSx, maxSx;
        psF32 minSy, maxSy;
        psVector *tmpSx = NULL;
        psVector *tmpSy = NULL;
        psStats *stats  = NULL;

        // select the single highest peak (note that we only have detValue, not rawFlux, etc
        psArraySort (peaks, pmPeaksSortByDetValueDescend);
        clump = peaks->data[0];
        psTrace ("psModules.objects", 2, "clump is at %d, %d (%f)\n", clump->x, clump->y, clump->detValue);

	// XXX store the mean sigma?
	float meanSigma = psfClump.nSigma;
	psfClump.nStars = clump->detValue;
	psfClump.nSigma = clump->detValue / meanSigma;

        // define section window for clump
        minSx = clump->x * PSF_CLUMP_GRID_SCALE - 2.0*PSF_CLUMP_GRID_SCALE;
        maxSx = clump->x * PSF_CLUMP_GRID_SCALE + 2.0*PSF_CLUMP_GRID_SCALE;
        minSy = clump->y * PSF_CLUMP_GRID_SCALE - 2.0*PSF_CLUMP_GRID_SCALE;
        maxSy = clump->y * PSF_CLUMP_GRID_SCALE + 2.0*PSF_CLUMP_GRID_SCALE;

        tmpSx = psVectorAllocEmpty (sources->n, PS_TYPE_F32);
        tmpSy = psVectorAllocEmpty (sources->n, PS_TYPE_F32);

        // create vectors with Sx, Sy values in window
        // clip sources based on S/N
        for (psS32 i = 0; i < sources->n; i++)
        {
            pmSource *tmpSrc = (pmSource *) sources->data[i];

            if (tmpSrc == NULL)
                continue;
            if (tmpSrc->moments == NULL)
                continue;
            if (tmpSrc->moments->SN < PSF_SN_LIM)
                continue;

	    if (region) {
		if (tmpSrc->peak->x < region->x0) continue;
		if (tmpSrc->peak->x > region->x1) continue;
		if (tmpSrc->peak->y < region->y0) continue;
		if (tmpSrc->peak->y > region->y1) continue;
	    }

            if (tmpSrc->moments->Mxx < minSx)
                continue;
            if (tmpSrc->moments->Mxx > maxSx)
                continue;
            if (tmpSrc->moments->Myy < minSy)
                continue;
            if (tmpSrc->moments->Myy > maxSy)
                continue;
            tmpSx->data.F32[tmpSx->n] = tmpSrc->moments->Mxx;
            tmpSy->data.F32[tmpSy->n] = tmpSrc->moments->Myy;
            tmpSx->n++;
            tmpSy->n++;
            if (tmpSx->n == tmpSx->nalloc) {
                psVectorRealloc (tmpSx, tmpSx->nalloc + 100);
                psVectorRealloc (tmpSy, tmpSy->nalloc + 100);
            }
        }

        // measures stats of Sx, Sy
        stats = psStatsAlloc (PS_STAT_CLIPPED_MEAN | PS_STAT_CLIPPED_STDEV);

        if (!psVectorStats (stats, tmpSx, NULL, NULL, 0)) {
            psError(PS_ERR_UNKNOWN, false, "failed to measure Sx stats");
            return (emptyClump);
        }
        psfClump.X  = stats->clippedMean;
        psfClump.dX = hypot(stats->clippedStdev, PSF_CLUMP_GRID_SCALE);

        if (!psVectorStats (stats, tmpSy, NULL, NULL, 0)) {
            psError(PS_ERR_UNKNOWN, false, "failed to measure Sy stats");
            return (emptyClump);
        }
        psfClump.Y  = stats->clippedMean;
        psfClump.dY = hypot(stats->clippedStdev, PSF_CLUMP_GRID_SCALE);

        psTrace ("psModules.objects", 2, "clump  X,  Y: %f, %f\n", psfClump.X, psfClump.Y);
        psTrace ("psModules.objects", 2, "clump DX, DY: %f, %f\n", psfClump.dX, psfClump.dY);

        psFree (stats);
        psFree (peaks);
        psFree (tmpSx);
        psFree (tmpSy);
    }

    psTrace("psModules.objects", 10, "---- end ----\n");
    return (psfClump);
}

/******************************************************************************
    pmSourceRoughClass(source, recipe): make a guess at the source
    classification.
    XXX: How can this function ever return FALSE?
*****************************************************************************/

bool pmSourceRoughClass(psRegion *region, psArray *sources, float PSF_SN_LIM, float PSF_CLUMP_NSIGMA, pmPSFClump clump, psImageMaskType maskSat)
{
    psTrace("psModules.objects", 10, "---- begin ----");

    PS_ASSERT_PTR_NON_NULL(sources, false);

    int Nsat     = 0;
    int Next     = 0;
    int Nstar    = 0;
    int Npsf     = 0;
    int Ncr      = 0;
    int Nsatstar = 0;
    psRegion inner;

    // report stats on S/N values for star-like objects
    psVector *starsn_peaks = psVectorAllocEmpty (sources->n, PS_TYPE_F32);
    psVector *starsn_moments = psVectorAllocEmpty (sources->n, PS_TYPE_F32);

    pmSourceMode noMoments = PM_SOURCE_MODE_MOMENTS_FAILURE | PM_SOURCE_MODE_SKYVAR_FAILURE | PM_SOURCE_MODE_SKY_FAILURE | PM_SOURCE_MODE_BELOW_MOMENTS_SN;

    // XXX allow clump size to be scaled relative to sigmas?
    // make rough IDs based on clumpX,Y,DX,DY
    for (psS32 i = 0 ; i < sources->n ; i++) {

        pmSource *source = (pmSource *) sources->data[i];

        // psf clumps are found for image subregions:
        // skip sources not in this region
        if (source->peak->x <  region->x0) continue;
        if (source->peak->x >= region->x1) continue;
        if (source->peak->y <  region->y0) continue;
        if (source->peak->y >= region->y1) continue;

# define DEBUG 0
# define TEST_X 2792
# define TEST_Y 1741
# if (DEBUG) 
	if ((fabs(source->peak->x - TEST_X) < 5) && (fabs(source->peak->y - TEST_Y) < 5)) {
	  fprintf (stderr, "test peak\n");
	}
# endif	

        // should be set by pmSourceAlloc
        psAssert (source->type == PM_SOURCE_TYPE_UNKNOWN, "source type was not init-ed?");

        // we are basically classifying by moments; use the default if not found
        if (!source->moments) {
            source->type = PM_SOURCE_TYPE_STAR;
            psAssert (source->mode & noMoments, "why is this source missing moments?");
            Nstar++;
            continue;
        }

        psF32 sigX = source->moments->Mxx;
        psF32 sigY = source->moments->Myy;

        // XXX EAM : can we use the value of SATURATE if mask is NULL?
	// XXX a 5x5 box centered on the peak is a rather small region to check for
        inner = psRegionForSquare (source->peak->x, source->peak->y, 2);
        inner = psRegionForImage (source->maskView, inner);
        int Nsatpix = psImageCountPixelMask (source->maskView, inner, maskSat);

        // saturated star (size consistent with PSF or larger)
        // Nsigma should be user-configured parameter
        bool big = (sigX > (clump.X - clump.dX)) && (sigY > (clump.Y - clump.dY));
        big = true;
        if ((Nsatpix > 1) && big) {
            source->type = PM_SOURCE_TYPE_STAR;
            source->mode |= PM_SOURCE_MODE_SATSTAR;
            // why do we recalculate moments here?
	    // we already attempt to do this in psphotSourceStats
            Nsatstar ++;
            continue;
        }

        // saturated object (not a star, eg bleed trails, hot pixels)
        if (Nsatpix > 1) {
            source->type = PM_SOURCE_TYPE_SATURATED;
            source->mode |= PM_SOURCE_MODE_SATURATED;
            Nsat ++;
            continue;
        }

        // The following determinations require the use of moments
        if (!(source->mode & noMoments)) {
            // likely defect (bright, but too small to be stellar)
	    // XXX eliminate the classification?
            if ((source->moments->SN > 10) && (sigX < 0.05 || sigY < 0.05)) {
                source->type = PM_SOURCE_TYPE_DEFECT;
                source->mode |= PM_SOURCE_MODE_DEFECT;
                Ncr ++;
                continue;
            }

	    // check for insignificant sources or excessively low-surface brightness
	    float coreSN = source->moments->KronCore / source->moments->KronCoreErr;
	    float coreKR = source->moments->KronCore / source->moments->KronFlux;

	    // XXX these values need to be in the recipe...
	    if (false && isfinite(coreSN) && (coreSN < 5.0)) {
                source->type = PM_SOURCE_TYPE_DEFECT;
                source->mode |= PM_SOURCE_MODE_DEFECT;
                Ncr ++;
                continue;
            }
	    if (false && isfinite(coreKR) && (coreKR < 0.1)) {
                source->type = PM_SOURCE_TYPE_DEFECT;
                source->mode |= PM_SOURCE_MODE_DEFECT;
                Ncr ++;
                continue;
            }

            // likely unsaturated extended source (too large to be stellar)
            if (sigX > clump.X + 3*clump.dX || sigY > clump.Y + 3*clump.dY) {
                source->type = PM_SOURCE_TYPE_EXTENDED;
                Next ++;
                continue;
            }

            // the rest are probable stellar objects
	    // the vectors below are accumulated to give user feedback on the S/N ranges
            starsn_moments->data.F32[starsn_moments->n] = source->moments->SN;
            starsn_moments->n ++;
            starsn_peaks->data.F32[starsn_peaks->n] = sqrt(source->peak->detValue);
            starsn_peaks->n ++;
            Nstar ++;

            // PSF star (within 1.5 sigma of clump center, S/N > limit)
            psF32 radius = hypot ((sigX-clump.X)/clump.dX, (sigY-clump.Y)/clump.dY);
            if ((source->moments->SN > PSF_SN_LIM) && (radius < PSF_CLUMP_NSIGMA)) {
                source->type = PM_SOURCE_TYPE_STAR;
                source->tmpFlags |= PM_SOURCE_TMPF_CANDIDATE_PSFSTAR;
                Npsf ++;
                continue;
            }
        }

        // random type of star
        source->type = PM_SOURCE_TYPE_STAR;
    }

    psLogMsg("psModules.objects", PS_LOG_INFO, "Rough classifications: %d %d %d %d %d %d",
             Nstar, Npsf, Next, Nsatstar, Nsat, Ncr);

    if (starsn_moments->n) {
        psStats *stats = NULL;
        stats = psStatsAlloc (PS_STAT_MIN | PS_STAT_MAX);

        if (!psVectorStats (stats, starsn_moments, NULL, NULL, 0)) {
	  psErrorClear(); // XXX it is probably excessive for psVectorStats to raise an error here (no valid values)
	  fprintf (stderr, "failed to measure SN / moments stats (no valid moments)\n");
	  // psError(PS_ERR_UNKNOWN, false, "failed to measure SN / moments stats");
	  // psFree (stats);
	  // psFree (starsn_peaks);
	  // return false;
        }
        psLogMsg ("pmObjects", 3, "SN range (moments): %f - %f\n", stats->min, stats->max);
        psFree (stats);
    }
    psFree (starsn_moments);

    if (starsn_peaks->n) {
        psStats *stats = NULL;
        stats = psStatsAlloc (PS_STAT_MIN | PS_STAT_MAX);
        if (!psVectorStats (stats, starsn_peaks, NULL, NULL, 0)) {
	  psErrorClear(); // XXX it is probably excessive for psVectorStats to raise an error here (no valid values)
	  fprintf (stderr, "failed to measure SN / peak stats (no valid peaks)\n");
	  // psError(PS_ERR_UNKNOWN, false, "failed to measure SN / moments stats");
	  // psFree (stats);
	  // psFree (starsn_peaks);
	  // return false;
        }
        psLogMsg ("psModules.objects", 3, "SN range (peaks)  : %f - %f (%ld)\n", stats->min, stats->max, starsn_peaks->n);
        psFree (stats);
    }
    psFree (starsn_peaks);

    psTrace ("psModules.objects", 2, "Nstar:    %3d\n", Nstar);
    psTrace ("psModules.objects", 2, "Npsf:     %3d\n", Npsf);
    psTrace ("psModules.objects", 2, "Next:     %3d\n", Next);
    psTrace ("psModules.objects", 2, "Nsatstar: %3d\n", Nsatstar);
    psTrace ("psModules.objects", 2, "Nsat:     %3d\n", Nsat);
    psTrace ("psModules.objects", 2, "Ncr:      %3d\n", Ncr);

    psTrace("psModules.objects", 10, "---- end ----\n");
    return true;
}

// construct a realization of the source model
bool pmSourceCacheModel (pmSource *source, psImageMaskType maskVal) {
    PS_ASSERT_PTR_NON_NULL(source, false);
    // select appropriate model
    pmModel *model = pmSourceGetModel (NULL, source);
    if (model == NULL) return false;  // model must be defined

    // if we already have a cached image, re-use that memory
    source->modelFlux = psImageCopy (source->modelFlux, source->pixels, PS_TYPE_F32);
    psImageInit (source->modelFlux, 0.0);

    // in some places (psphotEnsemble), we need a normalized version
    // in others, we just want the model.  which is more commonly used?
    // modelFlux always has unity normalization (I0 = 1.0)
    pmModelAdd (source->modelFlux, source->maskObj, model, PM_MODEL_OP_FULL | PM_MODEL_OP_NORM, maskVal);
    return true;
}

// construct a realization of the PSF model at the location of this source
bool pmSourceCachePSF (pmSource *source, psImageMaskType maskVal) {
    PS_ASSERT_PTR_NON_NULL(source, false);

    // select appropriate model
    if (source->modelPSF == NULL) return false;  // model must be defined

    // if we already have a cached image, re-use that memory
    source->psfImage = psImageCopy (source->psfImage, source->pixels, PS_TYPE_F32);
    psImageInit (source->psfImage, 0.0);

    // in some places (psphotEnsemble), we need a normalized version
    // in others, we just want the model.  which is more commonly used?
    // psfImage always has unity normalization (I0 = 1.0)
    pmModelAdd (source->psfImage, source->maskObj, source->modelPSF, PM_MODEL_OP_FULL | PM_MODEL_OP_NORM, maskVal);
    return true;
}

// should we call pmSourceCacheModel if it does not exist?
bool pmSourceOp (pmSource *source, pmModelOpMode mode, bool add, psImageMaskType maskVal, int dx, int dy)
{
    PS_ASSERT_PTR_NON_NULL(source, false);
    PS_ASSERT_PTR_NON_NULL(source->peak, false);
    bool status;

    if (add) {
        psTrace ("psphot", 3, "replacing object at %f,%f\n", source->peak->xf, source->peak->yf);
    } else {
        psTrace ("psphot", 3, "removing object at %f,%f\n", source->peak->xf, source->peak->yf);
    }

    pmModel *model = pmSourceGetModel (NULL, source);
    if (model == NULL) return false;  // model must be defined

    bool addNoise = mode & PM_MODEL_OP_NOISE;

    bool addModelVar = mode & PM_MODEL_OP_MODELVAR;
    if (addModelVar) psAssert (source->modelVar, "programming error");

    // require the use of pmModelAddWithOffset if we are adding noise (because the model size and norm are rescaled)
    if (!addNoise && source->modelFlux) {
        // add in the pixels from the modelFlux image
        int dX = source->modelFlux->col0 - source->pixels->col0;
        int dY = source->modelFlux->row0 - source->pixels->row0;
        assert (dX >= 0);
        assert (dY >= 0);
        assert (dX + source->modelFlux->numCols <= source->pixels->numCols);
        assert (dY + source->modelFlux->numRows <= source->pixels->numRows);

        // modelFlux has unity normalization
        float Io = model->params->data.F32[PM_PAR_I0];
        if (mode & PM_MODEL_OP_NORM) {
            Io = 1.0;
        }

        psImageMaskType **mask = NULL;
        if (source->maskObj) {
            mask = source->maskObj->data.PS_TYPE_IMAGE_MASK_DATA;
        }

	psF32 **target = NULL;
	target = addModelVar ? source->modelVar->data.F32 : source->pixels->data.F32;

        for (int iy = 0; iy < source->modelFlux->numRows; iy++) {
            int oy = iy + dY;
            for (int ix = 0; ix < source->modelFlux->numCols; ix++) {
                int ox = ix + dX;
                if (mask && (mask[iy][ix] & maskVal)) continue;
                float value = Io*source->modelFlux->data.F32[iy][ix];
                if (add) {
                    target[oy][ox] += value;
                } else {
                    target[oy][ox] -= value;
                }
            }
        }
	// do not change the flag here if we are adding/subtracting from modelVar
	if (!addModelVar) {
	  if (add) {
	    source->tmpFlags &= ~PM_SOURCE_TMPF_SUBTRACTED;
	  } else {
	    source->tmpFlags |= PM_SOURCE_TMPF_SUBTRACTED;
	  }
	}
        return true;
    }

    psImage *target = source->pixels;
    if (addNoise) {
        target = source->variance;
    }
    if (addModelVar) {
      target = source->modelVar;
    }

    if (add) {
	status = pmModelAddWithOffset (target, source->maskObj, model, PM_MODEL_OP_FULL, maskVal, dx, dy);
	if (!addNoise && !addModelVar) source->tmpFlags &= ~PM_SOURCE_TMPF_SUBTRACTED;
    } else {
	status = pmModelSubWithOffset (target, source->maskObj, model, PM_MODEL_OP_FULL, maskVal, dx, dy);
	if (!addNoise && !addModelVar) source->tmpFlags |= PM_SOURCE_TMPF_SUBTRACTED;
    }
    if (!status) {
	// XXX maybe raise an error or warning?
    }

    return true;
}

// should we call pmSourceCacheModel if it does not exist?
bool pmSourceNoiseOp (pmSource *source, pmModelOpMode mode, float FACTOR, float SIZE, bool add, psImageMaskType maskVal, int dx, int dy)
{
    assert (mode & PM_MODEL_OP_NOISE);
    PS_ASSERT_PTR_NON_NULL(source, false);
    PS_ASSERT_PTR_NON_NULL(source->peak, false);

    if (add) {
        psTrace ("psphot", 3, "adding noise to object at %f,%f\n", source->peak->xf, source->peak->yf);
    } else {
        psTrace ("psphot", 3, "removing noise from object at %f,%f\n", source->peak->xf, source->peak->yf);
    }

    pmSourceNoiseOpModel (source->modelPSF, source, mode, FACTOR, SIZE, add, maskVal, dx, dy);

    if (source->modelEXT) {
	pmSourceNoiseOpModel (source->modelEXT, source, mode, FACTOR, SIZE, add, maskVal, dx, dy);
    }

    return true;
}

bool pmSourceNoiseOpModel (pmModel *model, pmSource *source, pmModelOpMode mode, float FACTOR, float SIZE, bool add, psImageMaskType maskVal, int dx, int dy) 
{
    bool status;
    psEllipseShape oldshape;
    psEllipseAxes axes;

    if (add) {
	psTrace ("psphot", 4, "adding noise for object at %f,%f\n", model->params->data.F32[PM_PAR_XPOS], model->params->data.F32[PM_PAR_YPOS]);
    } else {
	psTrace ("psphot", 4, "remove noise for object at %f,%f\n", model->params->data.F32[PM_PAR_XPOS], model->params->data.F32[PM_PAR_YPOS]);
    }

    psF32 *PAR = model->params->data.F32;

    // save original values
    float oldI0  = PAR[PM_PAR_I0];
    oldshape.sx  = PAR[PM_PAR_SXX];
    oldshape.sy  = PAR[PM_PAR_SYY];
    oldshape.sxy = PAR[PM_PAR_SXY];

    // XXX can this be done more intelligently?
    if (oldI0 == 0.0) return false;
    if (!isfinite(oldI0)) return false;

    bool useReff = model->class->useReff;
    pmModelParamsToAxes (&axes, PAR[PM_PAR_SXX], PAR[PM_PAR_SXY], PAR[PM_PAR_SYY], useReff);

    // increase size and height of source
    axes.major *= SIZE;
    axes.minor *= SIZE;

    pmModelAxesToParams (&PAR[PM_PAR_SXX], &PAR[PM_PAR_SXY], &PAR[PM_PAR_SYY], axes, useReff);
    PAR[PM_PAR_I0]  = FACTOR*oldI0;

    psImage *target = source->variance;

    if (add) {
	status = pmModelAddWithOffset (target, source->maskObj, model, mode, maskVal, dx, dy);
    } else {
	status = pmModelSubWithOffset (target, source->maskObj, model, mode, maskVal, dx, dy);
    }
    if (!status) {
	// XXX raise an error or warning?
    }

    // restore original values
    PAR[PM_PAR_I0]  = oldI0;
    PAR[PM_PAR_SXX] = oldshape.sx;
    PAR[PM_PAR_SYY] = oldshape.sy;
    PAR[PM_PAR_SXY] = oldshape.sxy;

    return true;
}

bool pmSourceSmoothOp (pmSource *source, pmModelOpMode mode, psImage *target, float sigma, bool add, psImageMaskType maskVal, int dx, int dy)
{
//    assert (mode & PM_MODEL_OP_NOISE);
    PS_ASSERT_PTR_NON_NULL(source, false);
    PS_ASSERT_PTR_NON_NULL(source->peak, false);
    PS_ASSERT_PTR_NON_NULL(target, false);

    if (add) {
        psTrace ("psphot", 3, "adding smoothed object at %f,%f\n", source->peak->xf, source->peak->yf);
    } else {
        psTrace ("psphot", 3, "removing smooted object at %f,%f\n", source->peak->xf, source->peak->yf);
    }

    pmModel *model = pmSourceGetModel(NULL, source);
    if (!model) {
        return false;
    }
    pmSourceSmoothOpModel (model, source, mode, target, sigma, add, maskVal, dx, dy);

    return true;
}

bool pmSourceSmoothOpModel (pmModel *model, pmSource *source, pmModelOpMode mode, psImage *target, float sigma, bool add, psImageMaskType maskVal, int dx, int dy) 
{
    bool status;
    psEllipseShape oldshape;
    psEllipseShape newshape;
    psEllipseAxes axes;

    if (add) {
	psTrace ("psphot", 4, "adding smoothed object at %f,%f\n", model->params->data.F32[PM_PAR_XPOS], model->params->data.F32[PM_PAR_YPOS]);
    } else {
	psTrace ("psphot", 4, "removing smoothed object at %f,%f\n", model->params->data.F32[PM_PAR_XPOS], model->params->data.F32[PM_PAR_YPOS]);
    }

    psF32 *PAR = model->params->data.F32;

    // Isn't this hanging around somewhere?
    float oldFlux = NAN;
    if (!pmSourcePhotometryModel (NULL, &oldFlux, model)) return false;

    // save original values
    float oldI0  = PAR[PM_PAR_I0];
    float oldsxx = PAR[PM_PAR_SXX];
    float oldsyy = PAR[PM_PAR_SYY];
    float oldsxy = PAR[PM_PAR_SXY];

    // Since we are going to scale the flux correctly we need to get our
    // factors of sqrt(2) right
    oldshape.sx  = oldsxx / M_SQRT2;
    oldshape.sy  = oldsyy / M_SQRT2;
    oldshape.sxy = oldsxy;

    // XXX can this be done more intelligently?
    if (oldI0 == 0.0) return false;
    if (!isfinite(oldI0)) return false;

    // increase size and height of source
    axes = psEllipseShapeToAxes (oldshape, 20.0);
    axes.major = sqrt(PS_SQR(axes.major) + PS_SQR(sigma));
    axes.minor = sqrt(PS_SQR(axes.minor) + PS_SQR(sigma));
    newshape = psEllipseAxesToShape (axes);
    PAR[PM_PAR_SXX] = newshape.sx * M_SQRT2;
    PAR[PM_PAR_SYY] = newshape.sy * M_SQRT2;
    PAR[PM_PAR_SXY] = newshape.sxy;

    PAR[PM_PAR_I0]  = 1.0;

    float newFlux;
    if (!pmSourcePhotometryModel (NULL, &newFlux, model)) return false;

    PAR[PM_PAR_I0]  = oldFlux / newFlux;

    if (add) {
	status = pmModelAddWithOffset (target, source->maskObj, model, mode, maskVal, dx, dy);
    } else {
	status = pmModelSubWithOffset (target, source->maskObj, model, mode, maskVal, dx, dy);
    }
    if (!status) {
	// XXX raise an error or warning?
    }

    // restore original values
    PAR[PM_PAR_I0]  = oldI0;
    PAR[PM_PAR_SXX] = oldsxx;
    PAR[PM_PAR_SYY] = oldsyy;
    PAR[PM_PAR_SXY] = oldsxy;

    return true;
}

bool pmSourceAdd (pmSource *source, pmModelOpMode mode, psImageMaskType maskVal) {
    return pmSourceOp (source, mode, true, maskVal, 0, 0);
}

bool pmSourceSub (pmSource *source, pmModelOpMode mode, psImageMaskType maskVal) {
    return pmSourceOp (source, mode, false, maskVal, 0, 0);
}

bool pmSourceAddWithOffset (pmSource *source, pmModelOpMode mode, psImageMaskType maskVal, int dx, int dy) {
    return pmSourceOp (source, mode, true, maskVal, dx, dy);
}

bool pmSourceSubWithOffset (pmSource *source, pmModelOpMode mode, psImageMaskType maskVal, int dx, int dy) {
    return pmSourceOp (source, mode, false, maskVal, dx, dy);
}

// given a source, which model is currently appropriate?
// choose PSF or EXT based on source->type, but fall back on PSF
// if the EXT model is NULL
pmModel *pmSourceGetModel (bool *isPSF, const pmSource *source)
{
    PS_ASSERT_PTR_NON_NULL(source, NULL);

    pmModel *model;

    if (isPSF) {
        *isPSF = false;
    }

    switch (source->type) {
      case PM_SOURCE_TYPE_STAR:
        model = source->modelPSF;
        if (model == NULL)
            return NULL;
        if (isPSF) {
            *isPSF = true;
        }
        return model;

        // the 'best' extended model is saved in source->modelEXT (may be a pointer to one of
        // the elements of source->modelFits)
      case PM_SOURCE_TYPE_EXTENDED:
        model = source->modelEXT;
        if (!model && source->modelPSF) {
            // XXX raise an error or warning here?
            if (isPSF) {
                *isPSF = true;
            }
            return source->modelPSF;
        }
        return (model);
        break;

      default:
        return NULL;
    }
    return NULL;
}

// this function decides if the source position should be based on the peak or the moments.
// this is only used if we know we should not use a model fit position (eg, no model, or no
// model yet)
bool pmSourcePositionUseMoments(pmSource *source) {

    if (!source->moments) return false;		 // can't if there are no moments
    if (!source->moments->nPixels) return false; // can't if the moments were not measured
    if (source->mode & PM_SOURCE_MODE_MOMENTS_FAILURE) return false; // can't if the moments failed...

    if (source->mode & PM_SOURCE_MODE_SATSTAR) return true; // moments are best for SATSTARs

    float dX = source->moments->Mx - source->peak->xf;
    float dY = source->moments->My - source->peak->yf;
    float dR = hypot(dX, dY);
    
    // only use the moments position if the moment-peak offset is small or the star is saturated
    if (dR > 1.5) return false;

    return true;
}

// sort by SN (descending)
int pmSourceSortByFlux (const void **a, const void **b)
{
    pmSource *A = *(pmSource **)a;
    pmSource *B = *(pmSource **)b;

    psF32 fA = (A->peak == NULL) ? 0 : A->peak->rawFlux;
    psF32 fB = (B->peak == NULL) ? 0 : B->peak->rawFlux;
    if (isnan (fA)) fA = 0;
    if (isnan (fB)) fB = 0;

    psF32 diff = fA - fB;
    if (diff > FLT_EPSILON) return (-1);
    if (diff < FLT_EPSILON) return (+1);
    return (0);
}

// sort by SN (descending)
int pmSourceSortByParentFlux (const void **a, const void **b)
{
    pmSource *Ao = *(pmSource **)a;
    pmSource *Bo = *(pmSource **)b;
    pmSource *A  = Ao->parent;
    pmSource *B  = Bo->parent;

    psF32 fA = (A->peak == NULL) ? 0 : A->peak->rawFlux;
    psF32 fB = (B->peak == NULL) ? 0 : B->peak->rawFlux;
    if (isnan (fA)) fA = 0;
    if (isnan (fB)) fB = 0;

    psF32 diff = fA - fB;
    if (diff > FLT_EPSILON) return (-1);
    if (diff < FLT_EPSILON) return (+1);
    return (0);
}

// sort by Y (ascending)
int pmSourceSortByY (const void **a, const void **b)
{
    pmSource *A = *(pmSource **)a;
    pmSource *B = *(pmSource **)b;

    psF32 fA = (A->peak == NULL) ? 0 : A->peak->y;
    psF32 fB = (B->peak == NULL) ? 0 : B->peak->y;

    psF32 diff = fA - fB;
    if (diff > FLT_EPSILON) return (+1);
    if (diff < FLT_EPSILON) return (-1);
    return (0);
}

// sort by X (ascending)
int pmSourceSortByX (const void **a, const void **b)
{
    pmSource *A = *(pmSource **)a;
    pmSource *B = *(pmSource **)b;

    psF32 fA = (A->peak == NULL) ? 0 : A->peak->x;
    psF32 fB = (B->peak == NULL) ? 0 : B->peak->x;

    psF32 diff = fA - fB;
    if (diff > FLT_EPSILON) return (+1);
    if (diff < FLT_EPSILON) return (-1);
    return (0);
}

// sort by Seq (ascending)
int pmSourceSortBySeq (const void **a, const void **b)
{
    pmSource *A = *(pmSource **)a;
    pmSource *B = *(pmSource **)b;

    int iA = A->seq;
    int iB = B->seq;

    int diff = iA - iB;
    if (diff > 0) return (+1);
    if (diff < 0) return (-1);
    return (0);
}

// sort by Seq (ascending)
int pmSourceSortByParentSeq (const void **a, const void **b)
{
    pmSource *Ao = *(pmSource **)a;
    pmSource *Bo = *(pmSource **)b;
    pmSource *A  = Ao->parent;
    pmSource *B  = Bo->parent;

    int iA = A->seq;
    int iB = B->seq;

    int diff = iA - iB;
    if (diff > 0) return (+1);
    if (diff < 0) return (-1);
    return (0);
}

pmSourceMode pmSourceModeFromString (const char *name) {
  if (!strcasecmp (name, "DEFAULT"   )) return PM_SOURCE_MODE_DEFAULT;
  if (!strcasecmp (name, "PSFMODEL"  )) return PM_SOURCE_MODE_PSFMODEL;
  if (!strcasecmp (name, "EXTMODEL"  )) return PM_SOURCE_MODE_EXTMODEL;
  if (!strcasecmp (name, "FITTED"    )) return PM_SOURCE_MODE_FITTED;
  if (!strcasecmp (name, "FAIL"      )) return PM_SOURCE_MODE_FAIL;
  if (!strcasecmp (name, "POOR"      )) return PM_SOURCE_MODE_POOR;
  if (!strcasecmp (name, "PAIR"      )) return PM_SOURCE_MODE_PAIR;
  if (!strcasecmp (name, "PSFSTAR"   )) return PM_SOURCE_MODE_PSFSTAR;
  if (!strcasecmp (name, "SATSTAR"   )) return PM_SOURCE_MODE_SATSTAR;
  if (!strcasecmp (name, "BLEND"     )) return PM_SOURCE_MODE_BLEND;
  if (!strcasecmp (name, "EXTERNAL"  )) return PM_SOURCE_MODE_EXTERNAL;
  if (!strcasecmp (name, "BADPSF"    )) return PM_SOURCE_MODE_BADPSF;
  if (!strcasecmp (name, "DEFECT"    )) return PM_SOURCE_MODE_DEFECT;
  if (!strcasecmp (name, "SATURATED" )) return PM_SOURCE_MODE_SATURATED;
  if (!strcasecmp (name, "CRLIMIT"   )) return PM_SOURCE_MODE_CR_LIMIT;
  if (!strcasecmp (name, "EXTLIMIT"  )) return PM_SOURCE_MODE_EXT_LIMIT;
  return PM_SOURCE_MODE_DEFAULT;
}

char *pmSourceModeToString (const pmSourceMode mode) {
  switch (mode) {
    case PM_SOURCE_MODE_DEFAULT    : return psStringCopy ("DEFAULT"   );
    case PM_SOURCE_MODE_PSFMODEL   : return psStringCopy ("PSFMODEL"  );
    case PM_SOURCE_MODE_EXTMODEL   : return psStringCopy ("EXTMODEL"  );
    case PM_SOURCE_MODE_FITTED     : return psStringCopy ("FITTED"    );
    case PM_SOURCE_MODE_FAIL       : return psStringCopy ("FAIL"      );
    case PM_SOURCE_MODE_POOR       : return psStringCopy ("POOR"      );
    case PM_SOURCE_MODE_PAIR       : return psStringCopy ("PAIR"      );
    case PM_SOURCE_MODE_PSFSTAR    : return psStringCopy ("PSFSTAR"   );
    case PM_SOURCE_MODE_SATSTAR    : return psStringCopy ("SATSTAR"   );
    case PM_SOURCE_MODE_BLEND      : return psStringCopy ("BLEND"     );
    case PM_SOURCE_MODE_EXTERNAL   : return psStringCopy ("EXTERNAL"  );
    case PM_SOURCE_MODE_BADPSF     : return psStringCopy ("BADPSF"    );
    case PM_SOURCE_MODE_DEFECT     : return psStringCopy ("DEFECT"    );
    case PM_SOURCE_MODE_SATURATED  : return psStringCopy ("SATURATED" );
    case PM_SOURCE_MODE_CR_LIMIT   : return psStringCopy ("CRLIMIT"   );
    case PM_SOURCE_MODE_EXT_LIMIT  : return psStringCopy ("EXTLIMIT"  );
    default:
      return NULL;
  }
  return NULL;
}

// Function to estimate the memory consumed by a source.
#define IMAGE_BYTES(_im, _pix_size) (_im ? (sizeof(psImage) + (_im->numRows * sizeof(void*)) + (_im->numRows * _im->numCols * _pix_size * (_im->parent ? 0 : 1))) : 0)

#define VECTOR_BYTES(_v, _elem_size) (_v ? (sizeof(psVector) + (_v->n * _elem_size)) : 0)

// estimate the memory consumed by this source. 
// It doesn't count everything (a couple of psArrays), the big stuff is counted
psU64 pmSourceMemoryUse (pmSource *source) {
    psU64 bytes = sizeof(pmSource) + sizeof(pmPeak) + sizeof(pmMoments);

    bytes += IMAGE_BYTES(source->pixels, 4);
    bytes += IMAGE_BYTES(source->variance, 4);
    bytes += IMAGE_BYTES(source->modelVar, 4);
    bytes += IMAGE_BYTES(source->maskObj, 2);
    bytes += IMAGE_BYTES(source->maskView, 2);
    bytes += IMAGE_BYTES(source->modelFlux, 4);
    bytes += IMAGE_BYTES(source->psfImage, 4);

    if (source->modelFits) {
        for (int i = 0; i < source->modelFits->n; i++) {
            pmModel *model = source->modelFits->data[i];
            if (!model) continue;
            bytes += sizeof(pmModel);
            bytes += IMAGE_BYTES(model->covar, 4);
            bytes += VECTOR_BYTES(model->params, 4);
            bytes += VECTOR_BYTES(model->dparams, 4);
            if (model->residuals) {
                bytes += sizeof(pmResiduals);
                bytes += IMAGE_BYTES(model->residuals->Ro, 4);
                bytes += IMAGE_BYTES(model->residuals->Rx, 4);
                bytes += IMAGE_BYTES(model->residuals->Ry, 4);
                bytes += IMAGE_BYTES(model->residuals->variance, 4);
                bytes += IMAGE_BYTES(model->residuals->mask, 2);
            }
        }
    }
    if (source->radialAper) {
        for (int i = 0; i < source->radialAper->n; i++) {
            pmSourceRadialApertures *radialAper = source->radialAper->data[i];
            if (radialAper) {
                bytes += sizeof(pmSourceRadialApertures);
                bytes += VECTOR_BYTES(radialAper->flux, 4);
                bytes += VECTOR_BYTES(radialAper->fluxStdev, 4);
                bytes += VECTOR_BYTES(radialAper->fluxErr, 4);
                bytes += VECTOR_BYTES(radialAper->fill, 4);
            }
        }
    }

    return bytes;
}
