/* @file  pmPCM_MinimizeChisq.c
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

# define SAVE_IMAGES 0
# if (SAVE_IMAGES) 
int psphotSaveImage (psMetadata *header, psImage *image, char *filename);
# endif

# define FACILITY "psModules.objects"

# define USE_FFT 1
# define PRE_CONVOLVE 1
# define TESTCOPY 0

bool pmPCM_MinimizeChisq (
    psMinimization *min,
    psImage *covar,
    psVector *params,
    pmSource *source,
    pmPCMdata *pcm)
{
    psTrace(FACILITY, 3, "---- begin ----\n");
    PS_ASSERT_PTR_NON_NULL(min, false);
    PS_ASSERT_VECTOR_NON_NULL(params, false);
    PS_ASSERT_VECTOR_NON_EMPTY(params, false);
    PS_ASSERT_VECTOR_TYPE(params, PS_TYPE_F32, false);
    PS_ASSERT_PTR_NON_NULL(source, false);
    PS_ASSERT_PTR_NON_NULL(pcm, false);
    PS_ASSERT_VECTOR_TYPE(pcm->constraint->paramMask, PS_TYPE_VECTOR_MASK, false);
    PS_ASSERT_VECTORS_SIZE_EQUAL(params, pcm->constraint->paramMask, false);

    psVector *paramMask = pcm->constraint->paramMask;

    psMinimizeLMLimitFunc checkLimits = pcm->constraint->checkLimits;

    // this function has test values and current values for several things
    // the current value is in lower case
    // the test value is in upper case

    // allocate internal arrays (current vs Guess)
    psImage *Alpha = NULL;
    psVector *Beta = NULL;

    // Alpha & Beta only contain elements to represent the unmasked parameters
    if (!psMinLM_AllocAB (&Alpha, &Beta, params, paramMask)) {
        psAbort ("programming error: no unmasked parameters to be fit\n");
    }
    psAssert (pcm->nPar == Beta->n, "did we set the masked parameters correctly??");

    // allocate internal arrays (current vs Guess)
    psImage *alpha   = psImageAlloc(pcm->nPar, pcm->nPar, PS_TYPE_F32);
    psVector *beta   = psVectorAlloc(pcm->nPar, PS_TYPE_F32);
    psVector *Params = psVectorAlloc(params->n, PS_TYPE_F32);

    psF32 Chisq = 0.0;
    psF32 lambda = 0.001;
    psF32 dLinear = 0.0;
    psF32 nu = 3.0;

# if (USE_FFT && PRE_CONVOLVE)
    if (pcm->psfFFT) {
	psFree (pcm->psfFFT);
    }
# if (!TESTCOPY)
    if (!pcm->use1Dgauss) {
	pcm->psfFFT = psImageConvolveKernelInit(pcm->modelFlux, pcm->psf);
    }
# endif
# endif    

    // calculate initial alpha and beta, set chisq (min->value)
    min->value = pmPCM_SetABX(alpha, beta, params, paramMask, pcm, source);
    if (isnan(min->value)) {
        min->iter = min->maxIter;
        return(false);
    }
    // dump some useful info if trace is defined
    if (psTraceGetLevel(FACILITY) >= 6) {
        p_psImagePrint(psTraceGetDestination(), alpha, "alpha guess (0)");
        p_psVectorPrint(psTraceGetDestination(), beta, "beta guess (0)");
    }
    if (psTraceGetLevel(FACILITY) >= 5) {
        p_psVectorPrint(psTraceGetDestination(), params, "params guess (0)");
    }

    // iterate until the tolerance is reached, or give up
    bool done = (min->iter >= min->maxIter);
    while (!done) {
        psTrace(FACILITY, 5, "Iteration number %d.  (max iterations is %d).\n", min->iter, min->maxIter);

	if (min->chisqConvergence) {
	    psTrace(FACILITY, 5, "Last delta is %f.  stop if < %f, accept if < %f\n", min->lastDelta, min->minTol, min->maxTol);
	} else {
	    psTrace(FACILITY, 5, "Last delta is %f.  stop if < %f, accept if < %f\n", min->rParSigma, min->minTol*pcm->nPar, min->maxTol*pcm->nPar);
	}

	if (min->isInteractive) {
	    fprintf (stderr, "%d : ", min->iter);
	    for (int ti = 0; ti < params->n; ti++) {
		fprintf (stderr, "%f  ", params->data.F32[ti]);
	    }
	    fprintf (stderr, " : %f\n", min->value);
	}

	char key[10]; // used for interactive responses
	bool testValue = false;

        // set a new guess for Alpha, Beta, Params
        if (!psMinLM_GuessABP(Alpha, Beta, Params, alpha, beta, params, paramMask, checkLimits, lambda, &dLinear)) {
	    if (false && min->isInteractive) {
		fprintf (stdout, "guess failed (singular matrix or NaN values), continue? [Y,n] ");
		if (!fgets(key, 8, stdin)) {
		    psWarning("Unable to read option");
		}
		switch (key[0]) {
		  case 'n':
		  case 'N':
		    done = true;
		    break;
		  case 'y':
		  case 'Y':
		  case '\n':
		    lambda *= 10.0;
		    continue;
		  default:
		    lambda *= 10.0;
		    continue;
		}
		if (done) break;
	    }
            min->iter ++;
	    if (min->iter >=  min->maxIter) break;
            lambda *= 10.0;
            continue;
        }

	if (false && min->isInteractive) {
            p_psVectorPrint(psTraceGetDestination(), Params, "current parameters: ");
	    fprintf (stdout, "last chisq : %f\n", min->value);
	    bool getOptions = true;
	    while (getOptions) {
		fprintf (stdout, "options: (m)odify, (g)o, (q)uit: ");
		if (!fgets(key, 8, stdin)) {
		    psWarning("Unable to read option");
		}
		switch (key[0]) {
		  case 'm':
		  case 'M':
		    testValue = TRUE;
		    fprintf (stdout, "enter (Npar) (value): ");
		    int Npar = 0;
		    float value= 0;
		    int Nscan = fscanf (stdin, "%d %f", &Npar, &value);
		    if (Nscan != 2) {
		      fprintf (stderr, "scan failure\n");
		    }
		    Params->data.F32[Npar] = value;
		    break;
		  case 'g':
		  case 'G':
		  case '\n':
		    getOptions = false;
		    break;
		  default:
		    done = true;
		    break;
		}
		fprintf (stderr, "foo\n");
	    }
	    if (done) break;
	}
	    
        // dump some useful info if trace is defined
        if (psTraceGetLevel(FACILITY) >= 6) {
            p_psImagePrint(psTraceGetDestination(), Alpha, "Alpha guess (1)");
            p_psVectorPrint(psTraceGetDestination(), Beta, "Beta guess (1)");
            p_psVectorPrint(psTraceGetDestination(), beta, "beta current (1)");
        }
        if (psTraceGetLevel(FACILITY) >= 5) {
            p_psVectorPrint(psTraceGetDestination(), Params, "params guess (1)");
        }

	// calculate the parameter change (rParDelta) and error radius (rParSigma)
	//    rParDelta : radius of parameter change;
	//    rParSigma : radius of parameter error 
	
	// note that (before SetABX) Alpha[i][i] is the covariance matrix and
	// Beta is the actual parameter change for this pass

	// note that Alpha & Beta only represent unmasked parameters, while params and Params have all 

	// dParSigma = Alpha[i][i] : error (squared) on parameter i
	// dParDelta = Params->data.F32[i] - params->data.F32[i]     : change on parameter i
	float rParSigma = 0.0;
        for (int j = 0, J = 0; j < Params->n; j++) {
	    if (paramMask && (paramMask->data.PS_TYPE_VECTOR_MASK_DATA[j])) {
		continue;
	    }
	    rParSigma += PS_SQR(Params->data.F32[j] - params->data.F32[j]) / Alpha->data.F32[J][J];
	    J++;
	}
	rParSigma = sqrt(rParSigma);
	psTrace(FACILITY, 5, "rParSigma: %f, Niter: %d\n", rParSigma, min->iter);
	// fprintf (stderr, "rParSigma: %f, Niter: %d\n", rParSigma, min->iter);

        // calculate Chisq for new guess, update Alpha & Beta
        Chisq = pmPCM_SetABX(Alpha, Beta, Params, paramMask, pcm, source);
        if (isnan(Chisq)) {
            min->iter ++;
	    if (min->iter >=  min->maxIter) break;
            lambda *= 10.0;
            continue;
        }

        // convergence criterion:
        // compare the delta (min->value - Chisq) with the
        // expected delta from the linear model (dLinear)
        // accept new guess if it is an improvement (rho > 0), or else increase lambda
        psF32 rho = (min->value - Chisq) / dLinear;

        psTrace(FACILITY, 5, "last chisq: %f, new chisq %f, delta: %f, rho: %f\n", min->value, Chisq, min->lastDelta, rho);

        // dump some useful info if trace is defined
        if (psTraceGetLevel(FACILITY) >= 6) {
            p_psImagePrint(psTraceGetDestination(), Alpha, "alpha guess (2)");
            p_psVectorPrint(psTraceGetDestination(), Beta, "beta guess (2)");
        }

	// change in chisq/nDOF since last minimum
	min->lastDelta = (min->value - Chisq) / pcm->nDOF;

        // rho is positive if the new chisq is smaller; allow for some insignificant change (slight negative rho)

	// XXX the old version of lambda changes:
	// XXX : Madsen gives suggestion for better use of rho
        // rho is positive if the new chisq is smaller
        if (testValue || (rho >= -1e-6)) {
            min->value = Chisq;
            alpha  = psImageCopy(alpha, Alpha, PS_TYPE_F32);
            beta   = psVectorCopy(beta, Beta, PS_TYPE_F32);
            params = psVectorCopy(params, Params, PS_TYPE_F32);

            // save the new convolved model image
            psFree (source->modelFlux);
            source->modelFlux = pmPCMdataSaveImage(pcm);
        } 
	switch (min->gainFactorMode) {
	  case 0:
	    if (rho >= -1e-6) {
		lambda *= 0.1;
	    } else {
		lambda *= 10.0;
	    }
	    break;

	  case 1:
	    // adjust the gain ratio (lambda) based on rho
	    if (rho < 0.25) {
		lambda *= 2.0;
	    } 
	    if (rho > 0.75) {
		lambda *= 0.333;
	    }
	    break;

	  case 2:
	    if (rho > 0.0) {
		lambda *= PS_MAX(0.33, (1.0 - pow(2.0*rho - 1.0, 3.0)));
		nu = 3.0;
	    } else {
		lambda *= nu;
		nu *= 3.0;
	    }
	    break;
	}
        min->iter++;

	// ending conditions:
	// 1) hard limit : too many iterations
	done = (min->iter >= min->maxIter);
	
	// 2) require deltaChi > 1e-6 (ie, chisq is decreasing, but accept an insignificant change)
	if (min->lastDelta < -1e-6) {
	    continue;
	}

	// save this value in case we stop iterating
	min->rParSigma = rParSigma;

	// 2) require chisqDOF < maxChisqDOF (if maxChisqDOF is not NAN)
	// keep iterating regardless of rParSigma in this case
	float chisqDOF = Chisq / pcm->nDOF;
	if (isfinite(min->maxChisqDOF) && (chisqDOF > min->maxChisqDOF)) {
	    continue;
	}

	// delta-chisq or rParSigma ?
	if (min->chisqConvergence) {
	    done |= (min->lastDelta < min->minTol);
	} else {
	    done |= (rParSigma < min->minTol*pcm->nPar);
	}
    }
    psTrace(FACILITY, 5, "chisq: %f, last delta: %f, Niter: %d\n", min->value, min->lastDelta, min->iter);

    // construct & return the covariance matrix (if requested)
    if (covar != NULL) {
        if (!psMinLM_GuessABP(Alpha, Beta, Params, alpha, beta, params, paramMask, NULL, 0.0, NULL)) {
            psTrace (FACILITY, 5, "failure to calculate covariance matrix\n");
        }
        // set covar values which are not masked
        psImageInit (covar, 0.0);
        for (int j = 0, J = 0; j < params->n; j++) {
            if (paramMask && (paramMask->data.PS_TYPE_VECTOR_MASK_DATA[j])) {
                covar->data.F32[j][j] = 1.0;
                continue;
            }
            for (int k = 0, K = 0; k < params->n; k++) {
                if (paramMask && (paramMask->data.PS_TYPE_VECTOR_MASK_DATA[k])) continue;
                covar->data.F32[j][k] = Alpha->data.F32[J][K];
                K++;
            }
            J++;
        }
    }

    // free the internal temporary data
    psFree(alpha);
    psFree(Alpha);
    psFree(beta);
    psFree(Beta);
    psFree(Params);

    // if the last improvement was at least as good as maxTol, accept the fit:
    if (min->chisqConvergence) {
	if (min->lastDelta <= min->maxTol) {
	    psTrace(FACILITY, 6, "---- end (true) ----\n");
	    return(true);
	}
    } else {
	if (min->rParSigma <= min->maxTol*pcm->nPar) {
	    psTrace(FACILITY, 6, "---- end (true) ----\n");
	    return(true);
	}
    }
    psTrace(FACILITY, 6, "---- end (false) ----\n");
    return(false);
}

psF32 pmPCM_SetABX(
    psImage  *alpha,
    psVector *beta,
    const psVector *params,
    const psVector *paramMask,
    pmPCMdata *pcm,
    const pmSource *source)
{
    // XXX: Check vector sizes.
    PS_ASSERT_IMAGE_NON_NULL(alpha, NAN);
    PS_ASSERT_VECTOR_NON_NULL(beta, NAN);
    PS_ASSERT_VECTOR_NON_NULL(params, NAN);

    PS_ASSERT_PTR_NON_NULL(source, NAN);
    PS_ASSERT_IMAGE_NON_NULL(source->pixels, NAN);
    PS_ASSERT_IMAGE_NON_NULL(source->variance, NAN);
    PS_ASSERT_IMAGE_NON_NULL(source->maskObj, NAN);

    PS_ASSERT_VECTOR_TYPE(params, PS_TYPE_F32, false);
    if (paramMask) {
        PS_ASSERT_VECTOR_TYPE(paramMask, PS_TYPE_VECTOR_MASK, false);
    }

    // 1 *** generate the model and derivative images for this parameter set

    // storage for model derivatives
    psVector *deriv = psVectorAlloc(params->n, PS_TYPE_F32);

    // working vector to store local coordinate
    psVector *coord = psVectorAlloc(2, PS_TYPE_F32);

    psImageInit (pcm->modelFlux, 0.0);
    for (int n = 0; n < params->n; n++) {
        if (!pcm->dmodelsFlux->data[n]) continue;
        psImageInit (pcm->dmodelsFlux->data[n], 0.0);
    }

    // fill in the coordinate and value entries
    for (psS32 i = 0; i < source->pixels->numRows; i++) {
        for (psS32 j = 0; j < source->pixels->numCols; j++) {

            // XXX can we skip some of the data points where the model
            // is not going to be fitted??

            // skip masked points
            // XXX probably should not skipped masked points:
            // XXX skip if convolution of unmasked pixels will not see this pixel
            // if (source->maskObj->data.PS_TYPE_IMAGE_MASK_DATA[i][j]) {
            // continue;
            // }

            // skip zero-variance points
            // XXX why is this not masked?
            // if (source->variance->data.F32[i][j] == 0) {
            // continue;
            // }
            // skip nan value points
            // XXX why is this not masked?
            // if (!isfinite(source->pixels->data.F32[i][j])) {
            // continue;
            // }

            // Convert i/j to image space:
            coord->data.F32[0] = (psF32) (j + 0.5 + source->pixels->col0);
            coord->data.F32[1] = (psF32) (i + 0.5 + source->pixels->row0);

            pcm->modelFlux->data.F32[i][j] = pcm->modelConv->class->modelFunc (deriv, params, coord);

            for (int n = 0; n < params->n; n++) {
                if ((paramMask != NULL) && (paramMask->data.PS_TYPE_VECTOR_MASK_DATA[n])) { continue; }
                psImage *dmodel = pcm->dmodelsFlux->data[n];
                dmodel->data.F32[i][j] = deriv->data.F32[n];
            }
        }
    }
    psFree(coord);
    psFree(deriv);

    // convolve model and dmodel arrays with PSF
    // XXX speed this up by saving the FFTed psf (for each source, obviously)

    // XXX save the FFT'ed psf
    // XXX create an alternative function which takes a pre-FFTed kernel

# if (USE_FFT)
# if (PRE_CONVOLVE)
    // convolve model image and derivative images with pre-convolved kernel

// XXX for a test, just copy, rather than convolve
# if (TESTCOPY)
    psImageCopy (pcm->modelConvFlux, pcm->modelFlux, pcm->modelFlux->type.type);
# else // TESTCOPY
    if (pcm->use1Dgauss) {

	if (USE_1D_CACHE) {
	    // do not use the threaded, mask-aware version of this code (psImageSmoothMaskPixelsThread):
	    // * the model flux is not masked
	    // * threading takes place above this level
	    pcm->modelConvFlux = psImageCopy (pcm->modelConvFlux, pcm->modelFlux, pcm->modelFlux->type.type);
	    psImageSmoothCache_F32 (pcm->modelConvFlux, pcm->smdata);
	} else {
	    pcm->modelConvFlux = psImageCopy (pcm->modelConvFlux, pcm->modelFlux, pcm->modelFlux->type.type);
	    psImageSmooth2dCache_F32 (pcm->modelConvFlux, pcm->smdata2d);
	}
    } else {
	psImageConvolveKernel (pcm->modelConvFlux, pcm->modelFlux, NULL, 0, pcm->psfFFT);
    }
# endif // TESTCOPY

    for (int n = 0; n < pcm->dmodelsFlux->n; n++) {
        if (pcm->dmodelsFlux->data[n] == NULL) continue;
	if (pcm->constraint->paramMask->data.PS_TYPE_VECTOR_MASK_DATA[n]) continue;
        psImage *dmodel = pcm->dmodelsFlux->data[n];
        psImage *dmodelConv = pcm->dmodelsConvFlux->data[n];
# if (TESTCOPY)
	psImageCopy (dmodelConv, dmodel, dmodel->type.type);
# else // TESTCOPY
	if (pcm->use1Dgauss) {
	    if (USE_1D_CACHE) {
		// do not use the threaded, mask-aware version of this code (psImageSmoothMaskPixelsThread):
		// * the model flux is not masked
		// * threading takes place above this level
		dmodelConv = psImageCopy (dmodelConv, dmodel, dmodel->type.type);
		psImageSmoothCache_F32 (dmodelConv, pcm->smdata);
	    } else {
		dmodelConv = psImageCopy (dmodelConv, dmodel, dmodel->type.type);
		psImageSmooth2dCache_F32 (dmodelConv, pcm->smdata2d);
	    }
	} else {
	    psImageConvolveKernel (dmodelConv, dmodel, NULL, 0, pcm->psfFFT);
	}
# endif // TESTCOPY
    }
# else // PRE_CONVOLVE
    // convolve model image and derivative images with psf via FFT
    psImageConvolveFFT (pcm->modelConvFlux, pcm->modelFlux, NULL, 0, pcm->psf);
    for (int n = 0; n < pcm->dmodelsFlux->n; n++) {
        if (pcm->dmodelsFlux->data[n] == NULL) continue;
	if (pcm->constraint->paramMask->data.PS_TYPE_VECTOR_MASK_DATA[n]) continue;
        psImage *dmodel = pcm->dmodelsFlux->data[n];
        psImage *dmodelConv = pcm->dmodelsConvFlux->data[n];

	if (pcm->use1Dgauss) {
	    if (USE_1D_CACHE) {
		// do not use the threaded, mask-aware version of this code (psImageSmoothMaskPixelsThread):
		// * the model flux is not masked
		// * threading takes place above this level
		dmodelConv = psImageCopy (dmodelConv, dmodel, dmodel->type.type);
		psImageSmoothCache_F32 (dmodelConv, pcm->smdata);
	    } else {
		dmodelConv = psImageCopy (dmodelConv, dmodel, dmodel->type.type);
		psImageSmooth2dCache_F32 (dmodelConv, pcm->smdata2d);
	    }
	} else {
	    psImageConvolveFFT (dmodelConv, dmodel, NULL, 0, pcm->psf);
	}
    }
# endif // PRE-CONVOLVE
# else // USE_FFT
    // convolve model image and derivative images with psf direct
    psImageConvolveDirect (pcm->modelConvFlux, pcm->modelFlux, pcm->psf);
    for (int n = 0; n < pcm->dmodelsFlux->n; n++) {
        if (pcm->dmodelsFlux->data[n] == NULL) continue;
	if (pcm->constraint->paramMask->data.PS_TYPE_VECTOR_MASK_DATA[n]) continue;
        psImage *dmodel = pcm->dmodelsFlux->data[n];
        psImage *dmodelConv = pcm->dmodelsConvFlux->data[n];
        psImageConvolveDirect (dmodelConv, dmodel, pcm->psf);
    }
# endif // USE_FFT

    // XXX TEST : SAVE IMAGES
# if (SAVE_IMAGES)
    static int Npass = 0;
    char name[128]; 
    if (!pcm->use1Dgauss) {
      snprintf (name, 128, "psf.%03d.fits", Npass); psphotSaveImage (NULL, pcm->psf->image, name);
    }
    snprintf (name, 128, "mod.%03d.fits", Npass); psphotSaveImage (NULL, pcm->modelFlux, name);
    snprintf (name, 128, "cnv.%03d.fits", Npass); psphotSaveImage (NULL, pcm->modelConvFlux, name);
    snprintf (name, 128, "obj.%03d.fits", Npass); psphotSaveImage (NULL, source->pixels, name);
    snprintf (name, 128, "msk.%03d.fits", Npass); psphotSaveImage (NULL, source->maskObj, name);
    snprintf (name, 128, "var.%03d.fits", Npass); psphotSaveImage (NULL, source->variance, name);
    for (int n = 0; n < pcm->dmodelsFlux->n; n++) {
        psImage *dmodelConv = pcm->dmodelsConvFlux->data[n];
	if (!dmodelConv) continue;
	snprintf (name, 128, "dpar.%01d.%03d.fits", n, Npass); psphotSaveImage (NULL, dmodelConv, name);
    }
    Npass ++;
# endif

    // 2 *** accumulate alpha & beta

    // zero alpha and beta for summing below
    psImageInit (alpha, 0.0);
    psVectorInit (beta, 0.0);
    float chisq = 0.0;

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

            float ymodel  = pcm->modelConvFlux->data.F32[i][j];

	    // XXXX note this point here:::
            float yweight = pcm->poissonErrors ? 1.0 / source->variance->data.F32[i][j] : 1.0;
            float delta = ymodel - source->pixels->data.F32[i][j];

            chisq += PS_SQR(delta) * yweight;

            if (isnan(delta)) psAbort("nan in delta");
            if (isnan(chisq)) psAbort("nan in chisq");

            // alpha & beta only contain unmasked elements
            for (int n1 = 0, N1 = 0; n1 < params->n; n1++) {
                if ((paramMask != NULL) && (paramMask->data.PS_TYPE_VECTOR_MASK_DATA[n1])) continue;
                psImage *dmodel = pcm->dmodelsConvFlux->data[n1];
                float weight = dmodel->data.F32[i][j] * yweight;
                for (int n2 = 0, N2 = 0; n2 <= n1; n2++) {
                    if ((paramMask != NULL) && (paramMask->data.PS_TYPE_VECTOR_MASK_DATA[n2])) continue;
                    dmodel = pcm->dmodelsConvFlux->data[n2];
                    alpha->data.F32[N1][N2] += weight * dmodel->data.F32[i][j];
                    N2++;
                }
                beta->data.F32[N1] += weight * delta;
                N1++;
            }
        }
    }

    // calculate lower-left half of alpha
    for (psS32 j = 1; j < alpha->numCols; j++) {
        for (psS32 k = 0; k < j; k++) {
            alpha->data.F32[k][j] = alpha->data.F32[j][k];
        }
    }

    return(chisq);
}


/*
 *
 * we have a function func(param; value)

 * basic LMM:

 - fill in the data (x, y)

 chisq = SetABX (alpha, beta, params, paramMask, x, y, dy, func)

 while () {
 GuessABP (Alpha, Beta, Params, alpha, beta, params, paramMask, checkLimits, lambda)
 dLinear = dLinear(Beta, beta, lambda);
 chisq = SetABX (alpha, beta, params, paramMask, x, y, dy, func)
 convergence tests...
 }



 ** GuessABP:

 f_c = sum_i (kern_i * func (x_i; p_o))

 df_c/dp_o = d/dp_o [sum_i (kern_i * func (x_i; p_o))]

 df_c/dp_o = sum_i (d/dp_o [kern_i * func (x_i; p_o)])

 df_c/dp_o = sum_i (kern_i * d/dp_o [func (x_i; p_o)])

 - generate image arrays for func, dfunc/dp_j (not masked)
 - convolve each with psf
 - measure delta = f_conv - data
 - etc
*/


