/** @file  pmPSFtry.c
 *  @brief generate a pmPSF from a collection of EXT measurments of likely PSF stars.
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

bool pmPSF_DataDump (char *filename, psVector *x, psVector *y, psVector *e0, psVector *e1, psVector *e2, psVector *mask);

/*****************************************************************************
pmPSFFromPSFtry (psfTry): build a PSF model from a collection of source->modelEXT entries
using the specified order in X,Y.  The PSF ignores the first 4 (independent) model
parameters and constructs a polynomial fit to the remaining as a function of image
coordinate.  Input: psfTry with fitted source->modelEXT collection, pre-allocated psf
Note: some of the array entries may be NULL (failed fits); ignore them.
 *****************************************************************************/
bool pmPSFtryMakePSF (bool *pGoodFit, pmPSFtry *psfTry)
{
    PS_ASSERT_PTR_NON_NULL(psfTry, false);
    PS_ASSERT_PTR_NON_NULL(psfTry->sources, false);

    pmPSF *psf = psfTry->psf;
    psVector *srcMask = psfTry->mask;

    // construct the fit vectors from the collection of objects
    psVector *x  = psVectorAlloc (psfTry->sources->n, PS_TYPE_F32);
    psVector *y  = psVectorAlloc (psfTry->sources->n, PS_TYPE_F32);

    // construct the x,y terms
    for (int i = 0; i < psfTry->sources->n; i++) {
        if (srcMask->data.PS_TYPE_VECTOR_MASK_DATA[i]) continue;

        pmSource *source = psfTry->sources->data[i];
        assert (source->modelEXT); // all unmasked sources should have modelEXT

        x->data.F32[i] = source->modelEXT->params->data.F32[PM_PAR_XPOS];
        y->data.F32[i] = source->modelEXT->params->data.F32[PM_PAR_YPOS];
    }

    // fit the shape parameters (SXX, SYY, SXY) as a function of position
    if (!pmPSFFitShapeParams (pGoodFit, psf, psfTry->sources, x, y, srcMask)) {
        psFree(x);
        psFree(y);
        return false;
    }
    if (!*pGoodFit) {
	psWarning ("poor fit to PSF shape parameters for trend order %d, %d, skipping\n", psf->trendNx, psf->trendNy);
	psFree(x);
	psFree(y);
	return true;
    }

    // vector to store the other parameter values
    psVector *z  = psVectorAlloc (psfTry->sources->n, PS_TYPE_F32);

    // skip the unfitted parameters (X, Y, Io, Sky) and the shape parameters (SXX, SYY, SXY);
    // fit the remaining parameters.
    for (int i = 0; i < psf->params->n; i++) {
        switch (i) {
          case PM_PAR_SKY:
          case PM_PAR_I0:
          case PM_PAR_XPOS:
          case PM_PAR_YPOS:
          case PM_PAR_SXX:
          case PM_PAR_SYY:
          case PM_PAR_SXY:
            continue;
          default:
            break;
        }

        // select the per-object fitted data for this parameter
        for (int j = 0; j < psfTry->sources->n; j++) {
	    // skip any masked sources (failed to fit one of the model steps or get a magnitude)
	    if (srcMask->data.PS_TYPE_VECTOR_MASK_DATA[j]) continue;

            pmSource *source = psfTry->sources->data[j];
	    assert (source->modelEXT); // all unmasked sources should have modelEXT

            z->data.F32[j] = source->modelEXT->params->data.F32[i];
        }

	pmTrend2D *trend = psf->params->data[i];

        // fit the collection of measured parameters to the PSF 2D model
        // the mask is carried from previous steps and updated with this operation
        // the weight is either the flux error or NULL, depending on 'psf->poissonErrorParams'
        if (!pmTrend2DFit (pGoodFit, trend, srcMask, 0xff, x, y, z, NULL)) {
            psError(PS_ERR_UNKNOWN, false, "failed to build psf model for parameter %d", i);
            psFree(x);
            psFree(y);
            psFree(z);
            return false;
        }
	if (!*pGoodFit) {
	    // if we do not get a good fit (but do not actually hit an error), 
	    // tell the calling program to try something else
	    psWarning ("poor fit to PSF parameter %d for trend order %d, %d, skipping\n", i, psf->trendNx, psf->trendNy);
            psFree(x);
            psFree(y);
            psFree(z);
            return true;
	}
	if (trend->mode == PM_TREND_MAP) {
	    psImageMapRepair (trend->map->map);
	}
    }

    // test dump of star parameters vs position (compare with fitted values)
    if (psTraceGetLevel("psModules.objects") >= 4) {
        FILE *f = fopen ("params.dat", "w");

        for (int j = 0; j < psfTry->sources->n; j++) {
            pmSource *source = psfTry->sources->data[j];
            if (source == NULL) continue;
            if (source->modelEXT == NULL) continue;

            pmModel *modelPSF = pmModelFromPSF (source->modelEXT, psf);
            if (!modelPSF) {
                fprintf(f, "modelPSF is NULL\n");
                break;
            }
            if (!source->modelEXT) {
                fprintf(f, "source->modelEXT is NULL\n");
                break;
            }

            fprintf (f, "%f %f : ", source->modelEXT->params->data.F32[PM_PAR_XPOS], source->modelEXT->params->data.F32[PM_PAR_YPOS]);

            for (int i = 0; i < psf->params->n; i++) {
                if (psf->params->data[i] == NULL) continue;
                fprintf (f, "%f %f : ", source->modelEXT->params->data.F32[i], modelPSF->params->data.F32[i]);
            }
            fprintf (f, "%f %d\n", source->modelEXT->chisq, source->modelEXT->nIter);

            psFree(modelPSF);
        }
        fclose (f);
    }

    psFree (x);
    psFree (y);
    psFree (z);
    return true;
}

// fit the shape parameters using the supplied order (pmPSF->trendNx,trendNy)
bool pmPSFFitShapeParams (bool *pGoodFit, pmPSF *psf, psArray *sources, psVector *x, psVector *y, psVector *srcMask) {

    // we are doing a robust fit.  after each pass, we drop points which are more deviant than
    // three sigma.  the source mask (srcMask) is updated for each pass.  

    // The shape parameters (SXX, SXY, SYY) are strongly coupled.  We have to handle them very
    // carefully.  First, we convert them to the Ellipse Polarization terms (E0, E1, E2) for
    // each source and fit this set of parameters.  These values are less tightly coupled, but
    // are still inter-related.  The fitted values do a good job of constraining the major axis
    // and the position angle, but the minor axis is weakly measured.  When we apply the PSF
    // model to construct a source model, we convert the fitted values of E0,E1,E2 to the shape
    // parameters, with the constraint that the minor axis must be greater than a minimum
    // threshold.

    // XXX re-read the sextractor manual on handling 'infinitely thin' sources...

    // storage vectors for the polarization terms & mags
    psVector *e0   = psVectorAlloc (sources->n, PS_TYPE_F32);
    psVector *e1   = psVectorAlloc (sources->n, PS_TYPE_F32);
    psVector *e2   = psVectorAlloc (sources->n, PS_TYPE_F32);

    // convert the measured source shape paramters to polarization terms
    for (int i = 0; i < sources->n; i++) {
        // skip any masked sources (failed to fit one of the model steps or get a magnitude)
        if (srcMask->data.PS_TYPE_VECTOR_MASK_DATA[i]) continue;

        pmSource *source = sources->data[i];
        assert (source->modelEXT); // all unmasked sources should have modelEXT

	bool useReff = source->modelEXT->class->useReff;
        psEllipsePol pol = pmPSF_ModelToFit (source->modelEXT->params->data.F32, useReff);

        e0->data.F32[i] = pol.e0;
        e1->data.F32[i] = pol.e1;
        e2->data.F32[i] = pol.e2;
    }

    // weed out extreme e0 outliers here: find the median and exclude points not in the
    // range MEDIAN / 5 < e0 < 5 * MEDIAN
    { 
      psStats *e0stats = psStatsAlloc (PS_STAT_SAMPLE_MEDIAN);
      if (psVectorStats (e0stats, e0, NULL, srcMask, 0xff)) {
	float e0med = e0stats->sampleMedian;
    
	for (int i = 0; i < sources->n; i++) {
	  // skip any masked sources (failed to fit one of the model steps or get a magnitude)
	  if (srcMask->data.PS_TYPE_VECTOR_MASK_DATA[i]) continue;

	  if (e0->data.F32[i] < 0.2*e0med) {
	    srcMask->data.PS_TYPE_VECTOR_MASK_DATA[i] = PSFTRY_MASK_OUTLIER;
	  }
	  if (e0->data.F32[i] > 5.0*e0med) {
	    srcMask->data.PS_TYPE_VECTOR_MASK_DATA[i] = PSFTRY_MASK_OUTLIER;
	  }
	}
      }
      psFree (e0stats);
    }

    // we run 'clipIter' cycles clipping in each of x and y, with only one iteration each.
    // This way, the parameters masked by one of the fits will be applied to the others
    // NOTE : trend->stats (below) points to the same data as psfTrendStats; set the value to 1
    // to limit the iteration within the loop
    int nIter = psf->psfTrendStats->clipIter;
    psf->psfTrendStats->clipIter = 1;
    for (int i = 0; i < nIter; i++) {
	// XXX we are using the same stats structure on each pass: do we need to re-init it?
	// XXX we hardwire this to SAMPLE stats above (psphotChoosePSF.c), hardwire here instead?
     
	pmTrend2D *trend = NULL;

	// XXX we are using the same stats structure on each pass: do we need to re-init it?
	bool status = true;

	trend = psf->params->data[PM_PAR_E0];
	trend->stats->clipIter = 1; // in allocation, this value is set to the value of nIter, but we should use 1 here
	status &= pmTrend2DFit (pGoodFit, trend, srcMask, 0xff, x, y, e0, NULL);
	if (!*pGoodFit) {
	    psFree (e0);
	    psFree (e1);
	    psFree (e2);
	    return true;
	}
	if (trend->mode == PM_TREND_MAP) {
	    psImageMapRepair (trend->map->map);
	}

# if (PS_TRACE_ON)
	float mean, stdev;
	psStatsOptions meanOption = psStatsMeanOption(psf->psfTrendStats->options);
	psStatsOptions stdevOption = psStatsStdevOption(psf->psfTrendStats->options);
	mean = psStatsGetValue (trend->stats, meanOption);
	stdev = psStatsGetValue (trend->stats, stdevOption);
	psTrace ("psModules.objects", 4, "clipped E0 : %f +/- %f keeping %ld of %ld\n", mean, stdev, psf->psfTrendStats->clippedNvalues, e0->n);
# endif

        if (psf->psfTrendMode == PM_TREND_MAP) psImageMapCleanup (trend->map);
	pmSourceVisualPSFModelResid (trend, x, y, e0, srcMask);

	trend = psf->params->data[PM_PAR_E1];
	trend->stats->clipIter = 1; // in allocation, this value is set to the value of nIter, but we should use 1 here
	status &= pmTrend2DFit (pGoodFit, trend, srcMask, 0xff, x, y, e1, NULL);
	if (!*pGoodFit) {
	    psFree (e0);
	    psFree (e1);
	    psFree (e2);
	    return true;
	}
	if (trend->mode == PM_TREND_MAP) {
	    psImageMapRepair (trend->map->map);
	}

# if (PS_TRACE_ON)
	mean = psStatsGetValue (trend->stats, meanOption);
	stdev = psStatsGetValue (trend->stats, stdevOption);
	psTrace ("psModules.objects", 4, "clipped E1 : %f +/- %f keeping %ld of %ld\n", mean, stdev, psf->psfTrendStats->clippedNvalues, e1->n);
# endif
        if (psf->psfTrendMode == PM_TREND_MAP) psImageMapCleanup (trend->map);
	pmSourceVisualPSFModelResid (trend, x, y, e1, srcMask);

	trend = psf->params->data[PM_PAR_E2];
	trend->stats->clipIter = 1; // in allocation, this value is set to the value of nIter, but we should use 1 here
	status &= pmTrend2DFit (pGoodFit, trend, srcMask, 0xff, x, y, e2, NULL);
	if (!*pGoodFit) {
	    psFree (e0);
	    psFree (e1);
	    psFree (e2);
	    return true;
	}
	if (trend->mode == PM_TREND_MAP) {
	    psImageMapRepair (trend->map->map);
	}

# if (PS_TRACE_ON)
	mean = psStatsGetValue (trend->stats, meanOption);
	stdev = psStatsGetValue (trend->stats, stdevOption);
	psTrace ("psModules.objects", 4, "clipped E2 : %f +/- %f keeping %ld of %ld\n", mean, stdev, psf->psfTrendStats->clippedNvalues, e2->n);
# endif
        if (psf->psfTrendMode == PM_TREND_MAP) psImageMapCleanup (trend->map);
	pmSourceVisualPSFModelResid (trend, x, y, e2, srcMask);

	if (!status) {
	    psError (PS_ERR_UNKNOWN, true, "failed to fit PSF shape params");
	    psFree (e0);
	    psFree (e1);
	    psFree (e2);
	    return false;
	}
    }
    psf->psfTrendStats->clipIter = nIter;

    // test dump of the psf parameters
    if (psTraceGetLevel("psModules.objects") >= 4) {
        FILE *f = fopen ("pol.dat", "w");
        fprintf (f, "# x y  :  e0obs e1obs e2obs  : e0fit e1fit e2fit : mask\n");
        for (int i = 0; i < e0->n; i++) {
            fprintf (f, "%f %f  :  %f %f %f  : %f %f %f  : %d\n",
                     x->data.F32[i], y->data.F32[i],
                     e0->data.F32[i], e1->data.F32[i], e2->data.F32[i],
                     pmTrend2DEval (psf->params->data[PM_PAR_E0], x->data.F32[i], y->data.F32[i]),
                     pmTrend2DEval (psf->params->data[PM_PAR_E1], x->data.F32[i], y->data.F32[i]),
                     pmTrend2DEval (psf->params->data[PM_PAR_E2], x->data.F32[i], y->data.F32[i]),
                     srcMask->data.PS_TYPE_VECTOR_MASK_DATA[i]);
        }
        fclose (f);
    }

    psFree (e0);
    psFree (e1);
    psFree (e2);
    return true;
}

bool pmPSF_DataDump (char *filename, psVector *x, psVector *y, psVector *e0, psVector *e1, psVector *e2, psVector *mask) {


  FILE *f = fopen (filename, "w");
  if (!f) return false;

  for (int i = 0; i < x->n; i++) {

    fprintf (f, "%6.1f %6.1f : %5.2f %5.2f %5.2f : %2d\n", 
	     x->data.F32[i], 
	     y->data.F32[i], 
	     e0->data.F32[i], 
	     e1->data.F32[i], 
	     e2->data.F32[i], 
	     mask->data.U8[i]);
  }
  fclose (f);
  return true;
}
