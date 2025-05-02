/** @file  pmGrowthCurveGenerate.c
 *
 *  Generate the curve-of-growth
 *
 *  @author EAM, IfA
 *
 *  @version $Revision: 1.5 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-01-27 06:39:38 $
 *
 *  Copyright 2004 Institute for Astronomy, University of Hawaii
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/*****************************************************************************/
/* INCLUDE FILES                                                             */
/*****************************************************************************/

#include <strings.h>  // for strcasecmp
#include <pslib.h>
#include "pmHDU.h"
#include "pmFPA.h"
#include "pmFPAMaskWeight.h"
#include "psVectorBracket.h"
#include "pmErrorCodes.h"

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
#include "pmGrowthCurveGenerate.h"

/*****************************************************************************/
/* FUNCTION IMPLEMENTATION - PUBLIC                                          */
/*****************************************************************************/

// we generate the growth curve for the center of the image with the specified psf model
bool pmGrowthCurveGenerate (pmReadout *readout, pmPSF *psf, bool ignore, psImageMaskType maskVal, psImageMaskType markVal)
{
    PS_ASSERT_PTR_NON_NULL(readout, false);
    PS_ASSERT_PTR_NON_NULL(readout->image, false);

    // maskVal is used to test for rejected pixels, and must include markVal
    maskVal |= markVal;

    // XXX something of a hack: measure the growth curve at a number of points in the field and
    // average them together

    psArray *growths = psArrayAllocEmpty (100);

    for (float ix = -0.4; ix <= +0.4; ix += 0.2) {
	for (float iy = -0.4; iy <= +0.4; iy += 0.2) {

	    // use the center of the center pixel of the image
	    // 0.5 PIX: is this offset needed? probably -- the psf model uses 0.5,0.5 as the center, double check
	    float xc = (int)(ix*readout->image->numCols + 0.5*readout->image->numCols) + readout->image->col0 + 0.5;
	    float yc = (int)(iy*readout->image->numRows + 0.5*readout->image->numRows) + readout->image->row0 + 0.5;

	    pmGrowthCurve *growth = pmGrowthCurveForPosition (readout->image, psf, ignore, maskVal, markVal, xc, yc);
	    if (!growth) continue;

	    psArrayAdd (growths, 100, growth);
	    psFree (growth);
	}
    }
    // psAssert (growths->n, "cannot build growth curve (psf model is invalid everywhere)");
    // if we cannot generate a curve-of-growth, warn but do not raise an error or abort
    if (!growths->n) {
      psWarning ("cannot build growth curve (psf model is invalid everywhere)");
      psFree (growths);
      return false;
    }

    // just use a simple sample median to get the 'best' value from each growth curve...
    psStats *stats = psStatsAlloc (PS_STAT_SAMPLE_MEDIAN);

    psVector *values = psVectorAlloc (growths->n, PS_DATA_F32);

    // median the values for the fitMags
    values->n = 0;
    for (int j = 0; j < growths->n; j++) {
	pmGrowthCurve *growth = growths->data[j];
	if (!isfinite(growth->fitMag)) continue;
	psVectorAppend (values, growth->fitMag);
    }
    if (!psVectorStats (stats, values, NULL, NULL, 0)) {
	// if we cannot generate a curve-of-growth, warn but do not raise an error or abort
	psWarning("failure to measure stats for curve of growth");
	psFree (growths);
	psFree (stats);
	psFree (values);
	return false;
    }
    psf->growth->fitMag = stats->sampleMedian;

    // loop over a range of source fluxes
    // no need to interpolate since we have forced the object center
    // to 0.5, 0.5 above
    for (int i = 0; i < psf->growth->radius->n; i++) {

	// median the values for each radial bin
	values->n = 0;
	for (int j = 0; j < growths->n; j++) {
	    pmGrowthCurve *growth = growths->data[j];
	    if (!isfinite(growth->apMag->data.F32[i])) continue;
	    psVectorAppend (values, growth->apMag->data.F32[i]);
	}
	if (!psVectorStats (stats, values, NULL, NULL, 0)) {
	    // psError(PS_ERR_UNKNOWN, false, "failure to measure stats");
	    psWarning("failure to measure stats for curve of growth");
	    psFree (growths);
	    psFree (stats);
	    psFree (values);
	    return false;
	}
	psf->growth->apMag->data.F32[i] = stats->sampleMedian;
    }
    psf->growth->apRef = psVectorInterpolate (psf->growth->radius, psf->growth->apMag, psf->growth->refRadius);
    psf->growth->apLoss = psf->growth->fitMag - psf->growth->apRef;

    psLogMsg ("psphot.growth", 4, "GrowthCurve : apLoss : %f (fitMag - apMag @ ref : %f - %f)\n", psf->growth->apLoss, psf->growth->fitMag, psf->growth->apRef);

    psFree (growths);
    psFree (stats);
    psFree (values);

    return true;
}

pmGrowthCurve *pmGrowthCurveForPosition (psImage *image, pmPSF *psf, bool ignore, psImageMaskType maskVal, psImageMaskType markVal, float xc, float yc) {

    float fitMag, apMag;
    float radius;

    assert (psf->growth);

    float minRadius = psf->growth->radius->data.F32[0];
    pmGrowthCurve *growth = pmGrowthCurveAlloc (minRadius, psf->growth->maxRadius, psf->growth->refRadius);

    float dx = growth->maxRadius + 1;
    float dy = growth->maxRadius + 1;

    // create template model
    pmModel *modelRef = pmModelAlloc(psf->type);

    // assign the x and y coords to the image center
    // create an object with center intensity of 1000
    modelRef->params->data.F32[PM_PAR_SKY] = 0;
    modelRef->params->data.F32[PM_PAR_I0] = 1000;
    modelRef->params->data.F32[PM_PAR_XPOS] = xc;
    modelRef->params->data.F32[PM_PAR_YPOS] = yc;

    // create modelPSF from this model
    pmModel *model = pmModelFromPSF (modelRef, psf);
    if (!model) {
	psFree (growth);
	return NULL;
    }

    // measure the fitMag for this model
    pmSourcePhotometryModel (&fitMag, NULL, model);
    growth->fitMag = fitMag;

    // generate working image for this source
    psRegion region = {xc - dx, xc + dx, yc - dy, yc + dy};

    // force region to stop at dimensions of image
    region = psRegionForImage (image, region);

    // the view, image, and mask retain col0,row0
    psImage *view = psImageSubset (image, region);
    psImage *pixels = psImageCopy (NULL, view, PS_TYPE_F32);
    psImage *mask = psImageCopy (NULL, view, PS_TYPE_IMAGE_MASK);

    psImageInit (pixels, 0.0);
    psImageInit (mask, 0);

    // place the reference object in the image center
    // no need to mask the source here
    // XXX should we measure this for the analytical model only or the full model?
    pmModelAdd (pixels, NULL, model, PM_MODEL_OP_FULL, maskVal);

    // Loop over a range of radii.  No need to interpolate since we have forced the object
    // center to 0.5, 0.5 above
    for (int i = 0; i < growth->radius->n; i++) {

        radius = growth->radius->data.F32[i];

        // mask the given aperture and measure the apMag
        psImageKeepCircle (mask, xc, yc, radius, "OR", markVal);
        if (!pmSourcePhotometryAper (NULL, &apMag, NULL, NULL, model, pixels, NULL, mask, maskVal)) {
	    psFree (growth);
	    psFree (view);
	    psFree (pixels);
	    psFree (mask);
	    psFree (model);
	    psFree (modelRef);
	    return NULL;
        }
	psImageMaskPixels (mask, "AND", PS_NOT_IMAGE_MASK(markVal)); // clear the circular mask

        // the 'ignore' mode is for testing
        if (ignore) {
            growth->apMag->data.F32[i] = fitMag;
        } else {
            growth->apMag->data.F32[i] = apMag;
        }
    }
    
    psFree (view);
    psFree (pixels);
    psFree (mask);
    psFree (model);
    psFree (modelRef);

    // psLogMsg ("psModules", 4, "GrowthCurve for %f,%f\n", xc, yc);

    return growth;
}

# define DEBUG 0
# if (DEBUG)
static FILE *fgr = NULL;
# endif

// we generate the growth curve for the center of the image with the specified psf model
bool pmGrowthCurveGenerateFromSources (pmReadout *readout, pmPSF *psf, psArray *sources, bool INTERPOLATE_AP, psImageMaskType maskVal, psImageMaskType markVal)
{
    PS_ASSERT_PTR_NON_NULL(readout, false);
    PS_ASSERT_PTR_NON_NULL(readout->image, false);

    // maskVal is used to test for rejected pixels, and must include markVal
    maskVal |= markVal;

    pmSourcePhotometryMode photMode = INTERPOLATE_AP ? PM_SOURCE_PHOT_INTERP : 0;
    
    // measure the growth curve for each PSF source and average them together
    psArray *growths = psArrayAllocEmpty (100);

# if (DEBUG)
    fgr = fopen ("growth.mags.dat", "w");
# endif

    for (int i = 0; i < sources->n; i++) {

        pmSource *source = sources->data[i];

        if (!(source->mode & PM_SOURCE_MODE_PSFSTAR)) continue;

	pmGrowthCurve *growth = pmGrowthCurveForSource (source, psf, photMode, maskVal, markVal);
	if (!growth) continue;
	
	psArrayAdd (growths, 100, growth);
	psFree (growth);
    }
    psAssert (growths->n, "cannot build growth curve (no valid PSF stars?)");

# if (DEBUG)
    fclose (fgr);
# endif

    // just use a simple sample median to get the 'best' value from each growth curve...
    psStats *stats = psStatsAlloc (PS_STAT_SAMPLE_MEDIAN);

    psVector *values = psVectorAlloc (growths->n, PS_DATA_F32);

    // loop over a range of source fluxes
    // no need to interpolate since we have forced the object center
    // to 0.5, 0.5 above
    for (int i = 0; i < psf->growth->radius->n; i++) {

	// median the values for each radial bin
	values->n = 0;
	for (int j = 0; j < growths->n; j++) {
	    pmGrowthCurve *growth = growths->data[j];
	    if (!isfinite(growth->apMag->data.F32[i])) continue;
	    psVectorAppend (values, growth->apMag->data.F32[i] - growth->refMag);
	}
	if (values->n == 0) {
	    psf->growth->apMag->data.F32[i] = NAN;
	} else {
	    if (!psVectorStats (stats, values, NULL, NULL, 0)) {
		// psError(PS_ERR_UNKNOWN, false, "failure to measure stats");
		psWarning("failure to measure stats for curve of growth (from sources)");
		psFree (growths);
		psFree (stats);
		psFree (values);
		return false;
	    }
	    psf->growth->apMag->data.F32[i] = stats->sampleMedian;
	}
    }

    psf->growth->fitMag = psf->growth->apMag->data.F32[psf->growth->radius->n-1];
    psf->growth->apRef = psVectorInterpolate (psf->growth->radius, psf->growth->apMag, psf->growth->refRadius);
    psf->growth->apLoss = psf->growth->fitMag - psf->growth->apRef;

    psLogMsg ("psphot.growth", 4, "GrowthCurve : apLoss : %f (fitMag - apMag @ ref : %f - %f)\n", psf->growth->apLoss, psf->growth->fitMag, psf->growth->apRef);

    psFree (growths);
    psFree (stats);
    psFree (values);

    return true;
}

pmGrowthCurve *pmGrowthCurveForSource (pmSource *source, pmPSF *psf, pmSourcePhotometryMode photMode, psImageMaskType maskVal, psImageMaskType markVal) {

    float radius;

    assert (psf->growth);

    float minRadius = psf->growth->radius->data.F32[0];
    pmGrowthCurve *growth = pmGrowthCurveAlloc (minRadius, psf->growth->maxRadius, psf->growth->refRadius);

    // measure the fitMag for this source (for normalization)
    // pmSourcePhotometryModel (&fitMag, NULL, source->psfModel);
    growth->fitMag = source->psfMag;

    float xc = source->peak->xf;
    float yc = source->peak->yf;

    // Loop over the range of radii
    for (int i = 0; i < growth->radius->n; i++) {

        radius = growth->radius->data.F32[i];

        // mask the given aperture and measure the apMag
        psImageKeepCircle (source->maskObj, xc, yc, radius, "OR", markVal);

        if (!pmSourceMagnitudes (source, psf, photMode, maskVal, markVal, radius)) {
	    psFree (growth);
	    return NULL;
        }

        // if (!pmSourcePhotometryAper (NULL, &apMag, NULL, NULL, NULL, source->pixels, NULL, source->maskObj, maskVal)) {
	//     psFree (growth);
	//     return NULL;
        // }

	psImageMaskPixels (source->maskObj, "AND", PS_NOT_IMAGE_MASK(markVal)); // clear the circular mask

	growth->apMag->data.F32[i] = source->apMag;
    }
    psAssert(growth->refBin >= 0, "invalid growth reference bin");
    psAssert(growth->refBin < growth->apMag->n, "invalid growth reference bin");
    growth->refMag = growth->apMag->data.F32[growth->refBin];

    // Loop over the range of radii
# if (DEBUG)
    for (int i = 0; i < growth->radius->n; i++) {
	fprintf (fgr, "%f %f  %f %f %f %f\n", xc, yc, growth->radius->data.F32[i], growth->apMag->data.F32[i], growth->fitMag, growth->refMag);
    }
# endif

    return growth;
}
