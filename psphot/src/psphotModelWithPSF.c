# include "psphotInternal.h"
# define SAVE_IMAGES 0

bool psphotModelWithPSF_LMM (
    psMinimization *min,
    psImage *covar,
    psVector *params,
    psMinConstraint *constraint,
    pmSource *source,
    const psKernel *psf,
    psMinimizeLMChi2Func func)
{
    psTrace("psphot", 3, "---- begin ----\n");
    PS_ASSERT_PTR_NON_NULL(min, false);
    PS_ASSERT_VECTOR_NON_NULL(params, false);
    PS_ASSERT_VECTOR_NON_EMPTY(params, false);
    PS_ASSERT_VECTOR_TYPE(params, PS_TYPE_F32, false);
    psVector *paramMask = NULL;
    if (constraint != NULL) {
        paramMask = constraint->paramMask;
        if (paramMask != NULL) {
          PS_ASSERT_VECTOR_TYPE(paramMask, PS_TYPE_VECTOR_MASK, false);
            PS_ASSERT_VECTORS_SIZE_EQUAL(params, paramMask, false);
        }
    }
    PS_ASSERT_PTR_NON_NULL(func, false);
    PS_ASSERT_PTR_NON_NULL(source, false);

    psMinimizeLMLimitFunc checkLimits = NULL;
    if (constraint) {
        checkLimits = constraint->checkLimits;
    }

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

    // allocate internal arrays (current vs Guess)
    psImage *alpha   = psImageAlloc(Alpha->numCols, Alpha->numRows, PS_TYPE_F32);
    psVector *beta   = psVectorAlloc(Beta->n, PS_TYPE_F32);
    psVector *Params = psVectorAlloc(params->n, PS_TYPE_F32);

    psF32 Chisq = 0.0;
    psF32 lambda = 0.001;
    psF32 dLinear = 0.0;

    // generate PCM data storage structure
    pmPCMData *pcm = pmPCMDataAlloc (params, paramMask, source);

    // calculate initial alpha and beta, set chisq (min->value)
    min->value = psphotModelWithPSF_SetABX(alpha, beta, params, paramMask, pcm, source, psf, func);
    if (isnan(min->value)) {
        min->iter = min->maxIter;
        return(false);
    }
    // dump some useful info if trace is defined
    if (psTraceGetLevel("psphot") >= 6) {
        p_psImagePrint(psTraceGetDestination(), alpha, "alpha guess (0)");
        p_psVectorPrint(psTraceGetDestination(), beta, "beta guess (0)");
    }
    if (psTraceGetLevel("psphot") >= 5) {
        p_psVectorPrint(psTraceGetDestination(), params, "params guess (0)");
    }

    // iterate until the tolerance is reached, or give up
    while ((min->iter < min->maxIter) && ((min->lastDelta > min->minTol) || !isfinite(min->lastDelta))) {
        psTrace("psphot", 5, "Iteration number %d.  (max iterations is %d).\n", min->iter, min->maxIter);
        psTrace("psphot", 5, "Last delta is %f.  Min->minTol is %f.\n", min->lastDelta, min->minTol);


        // set a new guess for Alpha, Beta, Params
        if (!psMinLM_GuessABP(Alpha, Beta, Params, alpha, beta, params, paramMask, checkLimits, lambda, &dLinear)) {
            min->iter ++;
            lambda *= 10.0;
            continue;
        }

        // dump some useful info if trace is defined
        if (psTraceGetLevel("psphot") >= 6) {
            p_psImagePrint(psTraceGetDestination(), Alpha, "Alpha guess (1)");
            p_psVectorPrint(psTraceGetDestination(), Beta, "Beta guess (1)");
            p_psVectorPrint(psTraceGetDestination(), beta, "beta current (1)");
        }
        if (psTraceGetLevel("psphot") >= 5) {
            p_psVectorPrint(psTraceGetDestination(), Params, "params guess (1)");
        }

        // calculate Chisq for new guess, update Alpha & Beta
        Chisq = psphotModelWithPSF_SetABX(Alpha, Beta, Params, paramMask, pcm, source, psf, func);
        if (isnan(Chisq)) {
            min->iter ++;
            lambda *= 10.0;
            continue;
        }

        // convergence criterion:
        // compare the delta (min->value - Chisq) with the
        // expected delta from the linear model (dLinear)
        // accept new guess if it is an improvement (rho > 0), or else increase lambda
        psF32 rho = (min->value - Chisq) / dLinear;

        psTrace("psphot", 5, "last chisq: %f, new chisq %f, delta: %f, rho: %f\n", min->value,
                Chisq, min->lastDelta, rho);

        // dump some useful info if trace is defined
        if (psTraceGetLevel("psphot") >= 6) {
            p_psImagePrint(psTraceGetDestination(), Alpha, "alpha guess (2)");
            p_psVectorPrint(psTraceGetDestination(), Beta, "beta guess (2)");
        }

        /* if (Chisq < min->value) {  */
        if (rho > 0.0) {
            min->lastDelta = (min->value - Chisq) / (source->pixels->numCols*source->pixels->numRows - params->n);
            min->value = Chisq;
            alpha  = psImageCopy(alpha, Alpha, PS_TYPE_F32);
            beta   = psVectorCopy(beta, Beta, PS_TYPE_F32);
            params = psVectorCopy(params, Params, PS_TYPE_F32);
            lambda *= 0.25;

            // save the new convolved model image
            psFree (source->modelFlux);
            source->modelFlux = pmPCMDataSaveImage(pcm);
        } else {
            lambda *= 10.0;
        }
        min->iter++;
    }
    psTrace("psphot", 5, "chisq: %f, last delta: %f, Niter: %d\n", min->value, min->lastDelta, min->iter);

    // construct & return the covariance matrix (if requested)
    if (covar != NULL) {
        if (!psMinLM_GuessABP(Alpha, Beta, Params, alpha, beta, params, paramMask, NULL, 0.0, NULL)) {
            psTrace ("psphot", 5, "failure to calculate covariance matrix\n");
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
    psFree(pcm);

    // if the last improvement was at least as good as maxTol, accept the fit:
    if (min->lastDelta <= min->maxTol) {
	psTrace("psphot", 6, "---- end (true) ----\n");
        return(true);
    }
    psTrace("psphot", 6, "---- end (false) ----\n");
    return(false);
}

psF32 psphotModelWithPSF_SetABX(
    psImage  *alpha,
    psVector *beta,
    const psVector *params,
    const psVector *paramMask,
    pmPCMData *pcm,
    const pmSource *source,
    const psKernel *psf,
    psMinimizeLMChi2Func func)
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

    psImageInit (pcm->model, 0.0);
    for (int n = 0; n < params->n; n++) {
        if (!pcm->dmodels->data[n]) continue;
        psImageInit (pcm->dmodels->data[n], 0.0);
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
            coord->data.F32[0] = (psF32) (j + source->pixels->col0);
            coord->data.F32[1] = (psF32) (i + source->pixels->row0);

            pcm->model->data.F32[i][j] = func (deriv, params, coord);

            for (int n = 0; n < params->n; n++) {
                if ((paramMask != NULL) && (paramMask->data.PS_TYPE_VECTOR_MASK_DATA[n])) { continue; }
                psImage *dmodel = pcm->dmodels->data[n];
                dmodel->data.F32[i][j] = deriv->data.F32[n];
            }
        }
    }
    psFree(coord);
    psFree(deriv);

    // convolve model and dmodel arrays with PSF
    psImageConvolveDirect (pcm->modelConv, pcm->model, psf);
    for (int n = 0; n < pcm->dmodels->n; n++) {
        if (pcm->dmodels->data[n] == NULL) continue;
        psImage *dmodel = pcm->dmodels->data[n];
        psImage *dmodelConv = pcm->dmodelsConv->data[n];
        psImageConvolveDirect (dmodelConv, dmodel, psf);
    }

    // XXX TEST : SAVE IMAGES
# if (SAVE_IMAGES)
    psphotSaveImage (NULL, psf->image, "psf.fits");
    psphotSaveImage (NULL, pcm->model, "model.fits");
    psphotSaveImage (NULL, pcm->modelConv, "modelConv.fits");
    psphotSaveImage (NULL, source->pixels, "obj.fits");
    psphotSaveImage (NULL, source->maskObj, "mask.fits");
    psphotSaveImage (NULL, source->variance, "variance.fits");
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

            float ymodel  = pcm->modelConv->data.F32[i][j];
            float yweight = 1.0 / source->variance->data.F32[i][j];
            float delta = ymodel - source->pixels->data.F32[i][j];

            chisq += PS_SQR(delta) * yweight;

            if (isnan(delta)) psAbort("nan in delta");
            if (isnan(chisq)) psAbort("nan in chisq");

            // alpha & beta only contain unmasked elements
            for (int n1 = 0, N1 = 0; n1 < params->n; n1++) {
                if ((paramMask != NULL) && (paramMask->data.PS_TYPE_VECTOR_MASK_DATA[n1])) continue;
                psImage *dmodel = pcm->dmodelsConv->data[n1];
                float weight = dmodel->data.F32[i][j] * yweight;
                for (int n2 = 0, N2 = 0; n2 <= n1; n2++) {
                    if ((paramMask != NULL) && (paramMask->data.PS_TYPE_VECTOR_MASK_DATA[n2])) continue;
                    dmodel = pcm->dmodelsConv->data[n2];
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

static void pmPCMDataFree (pmPCMData *pcm) {

    if (pcm == NULL) return;

    psFree (pcm->model);
    psFree (pcm->modelConv);
    psFree (pcm->dmodels);
    psFree (pcm->dmodelsConv);
    return;
}

pmPCMData *pmPCMDataAlloc (
    const psVector *params,
    const psVector *paramMask,
    pmSource *source) {

    pmPCMData *pcm = (pmPCMData *) psAlloc(sizeof(pmPCMData));
    psMemSetDeallocator(pcm, (psFreeFunc) pmPCMDataFree);

    // Allocate storage images for raw model and derivative images
    pcm->model = psImageCopy (NULL, source->pixels, PS_TYPE_F32);
    pcm->dmodels = psArrayAlloc (params->n);
    for (psS32 n = 0; n < params->n; n++) {
        pcm->dmodels->data[n] = NULL;
        if ((paramMask != NULL) && (paramMask->data.PS_TYPE_VECTOR_MASK_DATA[n])) { continue; }
        pcm->dmodels->data[n] = psImageCopy (NULL, source->pixels, PS_TYPE_F32);
    }

    // Allocate storage images for convolved model and derivative images
    pcm->modelConv = psImageCopy (NULL, source->pixels, PS_TYPE_F32);
    pcm->dmodelsConv = psArrayAlloc (params->n);
    for (psS32 n = 0; n < params->n; n++) {
        pcm->dmodelsConv->data[n] = NULL;
        if ((paramMask != NULL) && (paramMask->data.PS_TYPE_VECTOR_MASK_DATA[n])) { continue; }
        pcm->dmodelsConv->data[n] = psImageCopy (NULL, source->pixels, PS_TYPE_F32);
    }

    return pcm;
}

psImage *pmPCMDataSaveImage (pmPCMData *pcm) {

    psImage *model = psImageCopy (NULL, pcm->modelConv, PS_TYPE_F32);

    return model;
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


