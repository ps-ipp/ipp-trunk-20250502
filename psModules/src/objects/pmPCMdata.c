/* @file  pmPCMdata.c
 * structures and functions to support PSF-convolved model fitting
 *
 * @author EAM, IfA
 *
 * @version $Revision: 1.29 $
 * @date $Date: 2009-02-16 22:30:50 $
 * Copyright 2010 Institute for Astronomy, University of Hawaii
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
#include "pmSourceSatstar.h"
#include "pmSourceLensing.h"
#include "pmSource.h"
#include "pmSourceFitModel.h"
#include "pmPCMdata.h"

# define USE_DELTA_PSF 0

static void pmPCMdataFree (pmPCMdata *pcm) {

    if (pcm == NULL) return;

    psFree (pcm->modelFlux);
    psFree (pcm->modelConvFlux);
    psFree (pcm->dmodelsFlux);
    psFree (pcm->dmodelsConvFlux);

    psFree (pcm->modelConv);
    psFree (pcm->psf);
    psFree (pcm->psfFFT);
    psFree (pcm->constraint);

    psFree (pcm->smdata); // pre-allocated data for psImageSmooth_PreAlloc
    psFree (pcm->smdata2d); // pre-allocated data for psImageSmooth_PreAlloc
    return;
}

pmPCMdata *pmPCMdataAlloc (
    const psVector *params,
    const psVector *paramMask,
    pmSource *source) {

    pmPCMdata *pcm = (pmPCMdata *) psAlloc(sizeof(pmPCMdata));
    psMemSetDeallocator(pcm, (psFreeFunc) pmPCMdataFree);

    // Allocate storage images for raw model and derivative images
    pcm->modelFlux = psImageCopy (NULL, source->pixels, PS_TYPE_F32);
    pcm->dmodelsFlux = psArrayAlloc (params->n);
    for (psS32 n = 0; n < params->n; n++) {
        pcm->dmodelsFlux->data[n] = NULL;
        if ((paramMask != NULL) && (paramMask->data.PS_TYPE_VECTOR_MASK_DATA[n])) { continue; }
        pcm->dmodelsFlux->data[n] = psImageCopy (NULL, source->pixels, PS_TYPE_F32);
    }

    // Allocate storage images for convolved model and derivative images
    pcm->modelConvFlux = psImageCopy (NULL, source->pixels, PS_TYPE_F32);
    pcm->dmodelsConvFlux = psArrayAlloc (params->n);
    for (psS32 n = 0; n < params->n; n++) {
        pcm->dmodelsConvFlux->data[n] = NULL;
        if ((paramMask != NULL) && (paramMask->data.PS_TYPE_VECTOR_MASK_DATA[n])) { continue; }
        pcm->dmodelsConvFlux->data[n] = psImageCopy (NULL, source->pixels, PS_TYPE_F32);
    }

    pcm->smdata = NULL;
    pcm->smdata2d = NULL;

    pcm->modelConv = NULL;
    pcm->psf = NULL;
    pcm->psfFFT = NULL;
    pcm->constraint = NULL;
    pcm->nDOF = 0;

    pcm->poissonErrors = true;

    // full convolution with the PSF is expensive.  if we have to save time, we can do a 1D
    // convolution with a Gaussian approximation to the kernel
    pcm->use1Dgauss = false;
    pcm->nsigma = NAN; // this is set to something defined by the user
    pcm->sigma = 1.0; // this should be set to something sensible when the psf is known

    return pcm;
}

psImage *pmPCMdataSaveImage (pmPCMdata *pcm) {

    psImage *model = psImageCopy (NULL, pcm->modelConvFlux, PS_TYPE_F32);

    return model;
}

psKernel *pmPCMkernelFromPSF (pmSource *source, int nPix) {

    assert (source);
    assert (source->psfImage); // XXX build if needed?

    int x0 = source->peak->x - source->psfImage->col0;
    int y0 = source->peak->y - source->psfImage->row0;

    // need to decide on the size: dynamically? statically?
    psKernel *psf = psKernelAlloc (-nPix, +nPix, -nPix, +nPix);

    // XXX we should just re-construct a PSF at this location 
    // psModelAdd (psf->image, NULL, source->modelPSF, PM_MODEL_OP_FULL | PM_MODEL_OP_NORM | PM_MODEL_OP_CENTER);
  
    // if the realized PSF for this object does not cover the full kernel, give up for now
    if (x0 + psf->xMin < 0) goto escape;
    if (x0 + psf->xMax >= source->psfImage->numCols) goto escape;
    if (y0 + psf->yMin < 0) goto escape;
    if (y0 + psf->yMax >= source->psfImage->numRows) goto escape;

    double sum = 0.0;
    for (int j = psf->yMin; j <= psf->yMax; j++) {
	for (int i = psf->xMin; i <= psf->xMax; i++) {
	    double value = source->psfImage->data.F32[y0 + j][x0 + i];
	    psf->kernel[j][i] = value;
	    sum += value;
	}
    }

    if (!(sum > 0.0)) {
        // Crazy PSF image print out some debugging information ...
        fprintf(stderr, "invalid kernel sum %f found by pmPCMkernelFromPSF\n", sum);

        if (sum != 0) {
            // don't bother printing the kernel if its sum is zero
            for (int j = psf->yMin; j <= psf->yMax; j++) {
                fprintf(stderr, "Row %d\n", j);
                for (int i = psf->xMin; i <= psf->xMax; i++) {
                    double value = source->psfImage->data.F32[y0 + j][x0 + i];
                    fprintf(stderr, "  %d %f\n", i, value);
                }
            }
        }
        fflush(stderr);
        // ... but avoid the asssertion two lines down by escaping
        goto escape;
    }
    assert (sum > 0.0);

    // psf must be normalized (integral = 1.0)
    for (int i = 0; i < psf->image->numRows; i++) {
	for (int j = 0; j < psf->image->numCols; j++) {
	    psf->image->data.F32[i][j] /= sum;
	}
    }

    return psf;

escape:
    psFree (psf);
    return NULL;
}

int pmPCMsetParams (psMinConstraint *constraint, pmSourceFitMode mode) {

    // set parameter mask based on fitting mode
    int nParams = 0;
    int nParAll = constraint->paramMask->n;

    switch (mode) {
      case PM_SOURCE_FIT_NORM:
        // fits only source normalization (Io)
        nParams = 1;
        psVectorInit (constraint->paramMask, 1);
        constraint->paramMask->data.PS_TYPE_VECTOR_MASK_DATA[PM_PAR_I0] = 0;
        break;

      case PM_SOURCE_FIT_PSF:
        // fits only x,y,Io
        nParams = 3;
        psVectorInit (constraint->paramMask, 1);
        constraint->paramMask->data.PS_TYPE_VECTOR_MASK_DATA[PM_PAR_I0] = 0;
        constraint->paramMask->data.PS_TYPE_VECTOR_MASK_DATA[PM_PAR_XPOS] = 0;
        constraint->paramMask->data.PS_TYPE_VECTOR_MASK_DATA[PM_PAR_YPOS] = 0;
        break;

      case PM_SOURCE_FIT_EXT:
        // fits all params except sky
        nParams = nParAll - 1;
        psVectorInit (constraint->paramMask, 0);
        constraint->paramMask->data.PS_TYPE_VECTOR_MASK_DATA[PM_PAR_SKY] = 1;
        break;

      case PM_SOURCE_FIT_EXT_AND_SKY:
        // fits all params including sky
        nParams = nParAll;
        psVectorInit (constraint->paramMask, 0);
        break;

      case PM_SOURCE_FIT_SHAPE:
	// fits shape (Sxx, Sxy, Syy) and Io
	nParams = 5;
	psVectorInit (constraint->paramMask, 1);
	constraint->paramMask->data.PS_TYPE_VECTOR_MASK_DATA[PM_PAR_SKY] = 0;
	constraint->paramMask->data.PS_TYPE_VECTOR_MASK_DATA[PM_PAR_I0] = 0;
	constraint->paramMask->data.PS_TYPE_VECTOR_MASK_DATA[PM_PAR_SXX] = 0;
	constraint->paramMask->data.PS_TYPE_VECTOR_MASK_DATA[PM_PAR_SXY] = 0;
	constraint->paramMask->data.PS_TYPE_VECTOR_MASK_DATA[PM_PAR_SYY] = 0;
	break;

      case PM_SOURCE_FIT_INDEX:
        // fits only Io, index (PAR7) -- only Io for models with < 8 params
	psVectorInit (constraint->paramMask, 1);
	constraint->paramMask->data.PS_TYPE_VECTOR_MASK_DATA[PM_PAR_I0] = 0;
        if (nParAll == 7) {
	    nParams = 1;
	} else {
	    nParams = 2;
	    constraint->paramMask->data.PS_TYPE_VECTOR_MASK_DATA[PM_PAR_7] = 0;
	}
	break;

      case PM_SOURCE_FIT_NO_INDEX:
        // fits all but index (PAR7) including sky
	psVectorInit (constraint->paramMask, 0);
        if (nParAll == 7) {
	    nParams = nParAll;
	} else {
	    nParams = nParAll - 1;
	    constraint->paramMask->data.PS_TYPE_VECTOR_MASK_DATA[PM_PAR_7] = 1;
	}
	break;
      default:
	psAbort("invalid fitting mode");
    }
    return nParams;
}

static int modelType_GAUSS = -1;
static int modelType_PS1_V1 = -1;

// generate a Gaussian smoothing kernel for supplied sigma.  sigma here does not need to match
// that used to allocate the structure, but it is recommended
bool psImageSmoothCacheKernel_PS1_V1 (psImageSmoothCacheData *smdata, float sigma, float kappa) {
    // check for NULL structure elements?

    int size = smdata->Nrange;

    psFree (smdata->kernel);
    smdata->kernel = psVectorAlloc(2 * smdata->Nrange + 1, PS_TYPE_F32);

    double sum = 0.0;			// Sum of Gaussian, for normalization
    double factor = 1.0 / (sigma * M_SQRT2);	// Multiplier for i -> z

    // PS1_V1 is a power-law with fitted linear term:
    // 1 / (1 + kappa z + z^1.666)  where z = (r/sigma)^2

    // generate the kernel (not normalized)
    for (int i = -size, j = 0; i <= size; i++, j++) {
	float z = PS_SQR(i * factor);
        sum += smdata->kernel->data.F32[j] = 1.0 / (1 + kappa * z + pow(z,1.666));
    }

    // renormalize kernel to integral of 1.0
    for (int i = 0; i < 2 * size + 1; i++) {
        smdata->kernel->data.F32[i] /= sum;
    }

    return true;
}

psImageSmoothCacheData *psImageSmoothCacheSetKernel (float *sigma, float *kappa, float nsigma, psImage *flux, pmModel *modelPSF) {

    psAssert (modelPSF, "psf model must be defined");
    
    psEllipseAxes axes;
    bool useReff = modelPSF->class->useReff;
    psF32 *PAR = modelPSF->params->data.F32;
    pmModelParamsToAxes (&axes, PAR[PM_PAR_SXX], PAR[PM_PAR_SXY], PAR[PM_PAR_SYY], useReff);
    
    *sigma = NAN;
    *kappa = NAN;

    // XXX need to do this more carefully
    if (modelPSF->type == modelType_GAUSS) {
	float FWHM_MAJOR = 2*modelPSF->class->modelRadius (modelPSF->params, 0.5*PAR[PM_PAR_I0]);
	float FWHM_MINOR = FWHM_MAJOR * (axes.minor / axes.major);
	*sigma = 0.50 * (FWHM_MAJOR + FWHM_MINOR) / 2.35;
    }
    if (modelPSF->type == modelType_PS1_V1) {
	*sigma = 0.5 * (axes.major + axes.minor);
	*kappa = PAR[PM_PAR_7];
    }
    psAssert (isfinite(*sigma), "invalid model type");

    // psImageSmoothCacheAlloc generates a structure but does not assign the smoothing vector
    psImageSmoothCacheData *smdata = psImageSmoothCacheAlloc (flux, *sigma, nsigma);

    if (modelPSF->type == modelType_GAUSS) {
	psImageSmoothCacheKernel_Gauss (smdata, *sigma);
    }
    if (modelPSF->type == modelType_PS1_V1) {
	psImageSmoothCacheKernel_PS1_V1 (smdata, *sigma, *kappa);
    }

    return smdata;
}

psImageSmooth2dCacheData *psImageSmooth2dCacheSetKernel (float *sigma, float *kappa, float nsigma, psImage *flux, pmModel *modelPSF) {

    psAssert (modelPSF, "psf model must be defined");
    
    psEllipseAxes axes;
    bool useReff = modelPSF->class->useReff;
    psF32 *PAR = modelPSF->params->data.F32;
    pmModelParamsToAxes (&axes, PAR[PM_PAR_SXX], PAR[PM_PAR_SXY], PAR[PM_PAR_SYY], useReff);
    
    *sigma = NAN;
    *kappa = NAN;

    // XXX need to do this more carefully
    if (modelPSF->type == modelType_GAUSS) {
	float FWHM_MAJOR = 2*modelPSF->class->modelRadius (modelPSF->params, 0.5*PAR[PM_PAR_I0]);
	float FWHM_MINOR = FWHM_MAJOR * (axes.minor / axes.major);
	*sigma = 0.50 * (FWHM_MAJOR + FWHM_MINOR) / 2.35;
    }
    if (modelPSF->type == modelType_PS1_V1) {
	*sigma = 0.5 * (axes.major + axes.minor);
	*kappa = PAR[PM_PAR_7];
    }
    psAssert (isfinite(*sigma), "invalid model type");

    // psImageSmoothCacheAlloc generates a structure but does not assign the smoothing vector
    psImageSmooth2dCacheData *smdata = psImageSmooth2dCacheAlloc (nsigma);

    if (modelPSF->type == modelType_GAUSS) {
	psImageSmooth2dCacheKernel_Gauss (smdata, *sigma);
    }
    if (modelPSF->type == modelType_PS1_V1) {
	psImageSmooth2dCacheKernel_PS1_V1 (smdata, *sigma, *kappa);
    }

    return smdata;
}

pmPCMdata *pmPCMinit(pmSource *source, pmSourceFitOptions *fitOptions, pmModel *model, psImageMaskType maskVal, float psfSize) {

    modelType_GAUSS = pmModelClassGetType ("PS_MODEL_GAUSS");
    modelType_PS1_V1 = pmModelClassGetType ("PS_MODEL_PS1_V1");

    // count the number of unmasked pixels:
    int nPix = 0;
    for (psS32 i = 0; i < source->pixels->numRows; i++) {
        for (psS32 j = 0; j < source->pixels->numCols; j++) {
            // XXX are we doing the right thing with the mask?
            // skip masked points
            if (source->maskObj->data.PS_TYPE_IMAGE_MASK_DATA[i][j]) {
                continue;
            }
            // skip zero-variance points
            if (source->variance->data.F32[i][j] == 0) {
                continue;
            }
            // skip nan value points
            if (!isfinite(source->pixels->data.F32[i][j])) {
                continue;
            }
	    nPix ++;
	}
    }    

    psVector *params  = model->params;

    // create the minimization constraints
    psMinConstraint *constraint = psMinConstraintAlloc();
    constraint->paramMask = psVectorAlloc (params->n, PS_TYPE_VECTOR_MASK);
    constraint->checkLimits = model->class->modelLimits;

    int nParams = pmPCMsetParams (constraint, fitOptions->mode);

    if (nPix <  nParams + 1) {
        psTrace ("psModules.objects", 4, "insufficient valid pixels\n");
	psFree (constraint);
        model->flags |= PM_MODEL_STATUS_BADARGS;
	return NULL;
    }

    // generate PCM data storage structure
    pmPCMdata *pcm = pmPCMdataAlloc (params, constraint->paramMask, source);
    pcm->modelConv = psMemIncrRefCounter(model);
    pcm->constraint = constraint;

    pcm->poissonErrors = fitOptions->poissonErrors;
    pcm->nsigma = fitOptions->nsigma;

    pcm->nPix = nPix;
    pcm->nPar = nParams;
    pcm->nDOF = nPix - nParams;

# if (USE_1D_GAUSS)

    pcm->use1Dgauss = true;
    if (USE_1D_CACHE) {
	pcm->smdata = psImageSmoothCacheSetKernel (&pcm->sigma, &pcm->kappa, pcm->nsigma, source->pixels, source->modelPSF);
    } else {
	pcm->smdata2d = psImageSmooth2dCacheSetKernel (&pcm->sigma, &pcm->kappa, pcm->nsigma, source->pixels, source->modelPSF);
    }

# else
    // make sure we save a cached copy of the psf flux
    pmSourceCachePSF (source, maskVal);

    // convert the cached cached psf model for this source to a psKernel
    psKernel *psf = pmPCMkernelFromPSF (source, psfSize);
    if (!psf) {
	// NOTE: this only happens if the source is too close to an edge
        model->flags |= PM_MODEL_STATUS_BADARGS;
	return NULL;
    }

# if (USE_DELTA_PSF)
    psImageInit (psf->image, 0.0);
    psf->image->data.F32[(int)(0.5*psf->image->numRows)][(int)(0.5*psf->image->numCols)] = 1.0;
# endif
    pcm->psf = psf;
    pcm->smdata = NULL;
# endif

    return pcm;
}

// has the set of fitted terms changed?  has the fitting radius changed?
bool pmPCMupdate(pmPCMdata *pcm, pmSource *source, pmSourceFitOptions *fitOptions, pmModel *model) {

    bool sameWindow = (source->pixels->numRows == pcm->modelFlux->numRows);
    sameWindow     &= (source->pixels->numCols == pcm->modelFlux->numCols);
    sameWindow     &= (source->pixels->col0    == pcm->modelFlux->col0);
    sameWindow     &= (source->pixels->row0    == pcm->modelFlux->row0);

    // re-count the number of unmasked pixels:
    if (!sameWindow) {
	for (psS32 i = 0; i < source->pixels->numRows; i++) {
	    for (psS32 j = 0; j < source->pixels->numCols; j++) {
		// XXX are we doing the right thing with the mask?
		// skip masked points
		if (source->maskObj->data.PS_TYPE_IMAGE_MASK_DATA[i][j]) {
		    continue;
		}
		// skip zero-variance points
		if (source->variance->data.F32[i][j] == 0) {
		    continue;
		}
		// skip nan value points
		if (!isfinite(source->pixels->data.F32[i][j])) {
		    continue;
		}
		pcm->nPix ++;
	    }
	}    
    }

    int nParams = pmPCMsetParams (pcm->constraint, fitOptions->mode);

    if (pcm->nPix <  nParams + 1) {
        psTrace ("psModules.objects", 4, "insufficient valid pixels\n");
        model->flags |= PM_MODEL_STATUS_BADARGS;
	return false;
    }
    pcm->nPar = nParams;
    pcm->nDOF = pcm->nPix - nParams;

    // has the source pixel window changed?
    if (!sameWindow) {

	// adjust all supporting images:
	pcm->modelFlux = psImageCopy (pcm->modelFlux, source->pixels, PS_TYPE_F32);
	for (psS32 n = 0; n < pcm->dmodelsFlux->n; n++) {
	    if ((pcm->constraint->paramMask != NULL) && (pcm->constraint->paramMask->data.PS_TYPE_VECTOR_MASK_DATA[n])) { continue; }
	    pcm->dmodelsFlux->data[n] = psImageCopy (pcm->dmodelsFlux->data[n], source->pixels, PS_TYPE_F32);
	}

	// adjust images for convolved model and derivative images
	pcm->modelConvFlux = psImageCopy (pcm->modelConvFlux, source->pixels, PS_TYPE_F32);
	for (psS32 n = 0; n < pcm->dmodelsConvFlux->n; n++) {
	    if ((pcm->constraint->paramMask != NULL) && (pcm->constraint->paramMask->data.PS_TYPE_VECTOR_MASK_DATA[n])) { continue; }
	    pcm->dmodelsConvFlux->data[n] = psImageCopy (pcm->dmodelsConvFlux->data[n], source->pixels, PS_TYPE_F32);
	}

	// If we have changed the window, we need to redefine the smoothing target vectors (but pcm->sigma,kappa,nsigma remain)
	if (USE_1D_CACHE) {
	    psFree(pcm->smdata);
	    pcm->smdata = psImageSmoothCacheAlloc (source->pixels, pcm->sigma, pcm->nsigma);

	    pmModel *modelPSF = source->modelPSF;
	    if (modelPSF->type == modelType_GAUSS) {
		psImageSmoothCacheKernel_Gauss (pcm->smdata, pcm->sigma);
	    }
	    if (modelPSF->type == modelType_PS1_V1) {
		psImageSmoothCacheKernel_PS1_V1 (pcm->smdata, pcm->sigma, pcm->kappa);
	    }
	} else {
	    psFree(pcm->smdata2d);
	    pcm->smdata2d = psImageSmooth2dCacheAlloc (pcm->nsigma);

	    pmModel *modelPSF = source->modelPSF;
	    if (modelPSF->type == modelType_GAUSS) {
		// psImageSmooth2dCacheKernel_Gauss (pcm->smdata2d, pcm->sigma);
	    }
	    if (modelPSF->type == modelType_PS1_V1) {
		psImageSmooth2dCacheKernel_PS1_V1 (pcm->smdata2d, pcm->sigma, pcm->kappa);
	    }
	}
    }

    return true;
}

// construct a realization of the source model
bool pmPCMCacheModel (pmSource *source, psImageMaskType maskVal, int psfSize, float nsigma) {

    PS_ASSERT_PTR_NON_NULL(source, false);

    // select appropriate model
    pmModel *model = pmSourceGetModel (NULL, source);
    if (model == NULL) return false;  // model must be defined

    // if we already have a cached image, re-use that memory
    source->modelFlux = psImageCopy (source->modelFlux, source->pixels, PS_TYPE_F32);
    psImageInit (source->modelFlux, 0.0);

    // modelFlux always has unity normalization (I0 = 1.0)
    pmModelAdd (source->modelFlux, source->maskObj, model, PM_MODEL_OP_FULL | PM_MODEL_OP_NORM, maskVal);

    // convolve the model image with the PSF
    if (USE_1D_GAUSS) {
	
	float sigma = NAN;
	float kappa = NAN;

	if (USE_1D_CACHE) {
	    psImageSmoothCacheData *smdata = psImageSmoothCacheSetKernel (&sigma, &kappa, nsigma, source->modelFlux, source->modelPSF);
	    psImageSmoothCache_F32 (source->modelFlux, smdata);
	    psFree (smdata);
	} else {
	    psImageSmooth2dCacheData *smdata = psImageSmooth2dCacheSetKernel (&sigma, &kappa, nsigma, source->modelFlux, source->modelPSF);
	    psImageSmooth2dCache_F32 (source->modelFlux, smdata);
	    psFree (smdata);
	}
	// old call: psImageSmooth (source->modelFlux, sigma, nsigma);
    } else {
	// make sure we save a cached copy of the psf flux
	pmSourceCachePSF (source, maskVal);

	// convert the cached cached psf model for this source to a psKernel
	psKernel *psf = pmPCMkernelFromPSF (source, psfSize);
	if (!psf) {
	    // NOTE: this only happens if the source is too close to an edge
	    model->flags |= PM_MODEL_STATUS_BADARGS;
	    return NULL;
	}

	// XXX not sure if I can place the output on top of the input
	psImageConvolveFFT (source->modelFlux, source->modelFlux, NULL, 0, psf);
    }
    return true;
}

// construct a realization of the source model
bool pmPCMMakeModel (pmSource *source, pmModel *model, float Nsigma, psImageMaskType maskVal, int psfSize) {

    PS_ASSERT_PTR_NON_NULL(source, false);

    // if we already have a cached image, re-use that memory
    source->modelFlux = psImageCopy (source->modelFlux, source->pixels, PS_TYPE_F32);
    psImageInit (source->modelFlux, 0.0);

    // modelFlux always has unity normalization (I0 = 1.0)
    // pmModelAdd (source->modelFlux, source->maskObj, model, PM_MODEL_OP_FULL | PM_MODEL_OP_NORM, maskVal);
    pmModelAdd (source->modelFlux, NULL, model, PM_MODEL_OP_FULL | PM_MODEL_OP_SKY | PM_MODEL_OP_NORM, maskVal);

    // convolve the model image with the PSF
    if (USE_1D_GAUSS) {

	float sigma = NAN;
	float kappa = NAN;

	if (USE_1D_CACHE) {
	    psImageSmoothCacheData *smdata = psImageSmoothCacheSetKernel (&sigma, &kappa, Nsigma, source->modelFlux, source->modelPSF);
	    psImageSmoothCache_F32 (source->modelFlux, smdata);
	    psFree (smdata);
	} else {
	    psImageSmooth2dCacheData *smdata = psImageSmooth2dCacheSetKernel (&sigma, &kappa, Nsigma, source->modelFlux, source->modelPSF);
	    psImageSmooth2dCache_F32 (source->modelFlux, smdata);
	    psFree (smdata);
	}
	// old call: psImageSmooth (source->modelFlux, sigma, nsigma);
    } else {
	// make sure we save a cached copy of the psf flux
	pmSourceCachePSF (source, maskVal);

	// convert the cached cached psf model for this source to a psKernel
	psKernel *psf = pmPCMkernelFromPSF (source, psfSize);
	if (!psf) {
	    // NOTE: this only happens if the source is too close to an edge
	    model->flags |= PM_MODEL_STATUS_BADARGS;
	    return NULL;
	}

	// XXX not sure if I can place the output on top of the input
	psImageConvolveFFT (source->modelFlux, source->modelFlux, NULL, 0, psf);
    }
    return true;
}
