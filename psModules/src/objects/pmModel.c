/** @file  pmModel.c
 *
 *  Functions to define and manipulate object models
 *
 *  @author GLG, MHPCC
 *  @author EAM, IfA
 *
 *  @version $Revision: 1.28 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-16 22:30:26 $
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

static void modelFree(pmModel *tmp)
{
    psTrace("psModules.objects", 10, "---- %s() begin ----\n", __func__);
    if (!tmp) return;

    psFree(tmp->params);
    psFree(tmp->dparams);
    psFree(tmp->covar);
    psTrace("psModules.objects", 10, "---- %s() end ----\n", __func__);
}

/******************************************************************************
pmModelAlloc(): Allocate the pmModel structure, along with its parameters,
and initialize the type member.  Initialize the params to 0.0.
*****************************************************************************/
pmModel *pmModelAlloc(pmModelType type)
{
    psTrace("psModules.objects", 10, "---- %s() begin ----\n", __func__);

    pmModelClass *class = pmModelClassSelect (type);
    if (class == NULL) {
        psError(PS_ERR_UNKNOWN, true, "Undefined pmModelType");
        return(NULL);
    }

    pmModel *tmp = (pmModel *) psAlloc(sizeof(pmModel));
    psMemSetDeallocator(tmp, (psFreeFunc) modelFree);

    tmp->type = type;
    tmp->mag = NAN;
    tmp->magErr = NAN;
    tmp->chisq = NAN;
    tmp->chisqNorm = NAN;
    tmp->nDOF  = 0;
    tmp->nPar  = 0;
    tmp->nPix  = 0;
    tmp->nIter = 0;
    tmp->fitRadius = 0;
    tmp->flags = PM_MODEL_STATUS_NONE;
    tmp->residuals = NULL;              // do not free: the model does not own this memory
    tmp->covar = NULL;
    tmp->isPCM = false;

    psS32 Nparams = pmModelClassParameterCount(type);
    assert (Nparams);

    tmp->params  = psVectorAlloc(Nparams, PS_TYPE_F32);
    tmp->dparams = psVectorAlloc(Nparams, PS_TYPE_F32);
    assert (tmp->params);
    assert (tmp->dparams);

    for (psS32 i = 0; i < tmp->params->n; i++) {
        tmp->params->data.F32[i] = NAN;
        tmp->dparams->data.F32[i] = NAN;
    }

    tmp->class = class;

    // tmp->modelFunc          = class->modelFunc;
    // tmp->modelFlux          = class->modelFlux;
    // tmp->modelRadius        = class->modelRadius;
    // tmp->modelLimits        = class->modelLimits;
    // tmp->modelGuess         = class->modelGuess;
    // tmp->modelFromPSF       = class->modelFromPSF;
    // tmp->modelParamsFromPSF = class->modelParamsFromPSF;
    // tmp->modelFitStatus     = class->modelFitStatus;
    // tmp->modelSetLimits     = class->modelSetLimits;

    psTrace("psModules.objects", 10, "---- %s() end ----\n", __func__);
    return(tmp);
}

bool psMemCheckModel(psPtr ptr)
{
    PS_ASSERT_PTR(ptr, false);
    return ( psMemGetDeallocator(ptr) == (psFreeFunc) modelFree);
}

// copy model to a new structure
pmModel *pmModelCopy (pmModel *model)
{
    if (model == NULL) {
	return NULL;
    }
    pmModel *new = pmModelAlloc (model->type);

    new->chisq     = model->chisq;
    new->nDOF      = model->nDOF;
    new->nIter     = model->nIter;
    new->flags     = model->flags;
    new->fitRadius = model->fitRadius;

    for (int i = 0; i < new->params->n; i++) {
        new->params->data.F32[i]  = model->params->data.F32[i];
        new->dparams->data.F32[i] = model->dparams->data.F32[i];
    }

    // note that model->residuals is just a reference
    new->residuals = model->residuals;

    return (new);
}

/******************************************************************************
    pmModelEval(source, level, row): evaluates the model function at the specified coords.

    NOTE: The coords are in subImage source->pixel coords, not image coords.

    XXX: Use static vectors for x (NO: needs to be thread safe)
*****************************************************************************/
psF32 pmModelEval(pmModel *model, psImage *image, psS32 col, psS32 row)
{
    psTrace("psModules.objects", 10, "---- %s() begin ----\n", __func__);
    PS_ASSERT_PTR_NON_NULL(image, NAN);
    PS_ASSERT_PTR_NON_NULL(model, NAN);
    PS_ASSERT_PTR_NON_NULL(model->params, NAN);

    // Allocate the x coordinate structure and convert row/col to image space.
    //
    psVector *x = psVectorAlloc(2, PS_TYPE_F32);
    x->data.F32[0] = (psF32) (col + image->col0);
    x->data.F32[1] = (psF32) (row + image->row0);
    psF32 tmpF;

    tmpF = model->class->modelFunc (NULL, model->params, x);
    psFree(x);
    psTrace("psModules.objects", 10, "---- %s() end ----\n", __func__);
    return(tmpF);
}

psF32 pmModelEvalWithOffset(pmModel *model, psImage *image, psS32 col, psS32 row, int dx, int dy)
{
    psTrace("psModules.objects", 10, "---- %s() begin ----\n", __func__);
    PS_ASSERT_PTR_NON_NULL(image, false);
    PS_ASSERT_PTR_NON_NULL(model, false);
    PS_ASSERT_PTR_NON_NULL(model->params, false);

    // Allocate the x coordinate structure and convert row/col to image space.
    //
    psVector *x = psVectorAlloc(2, PS_TYPE_F32);
    x->data.F32[0] = (psF32) (col + image->col0 + dx);
    x->data.F32[1] = (psF32) (row + image->row0 + dy);
    psF32 tmpF;

    tmpF = model->class->modelFunc (NULL, model->params, x);
    psFree(x);
    psTrace("psModules.objects", 10, "---- %s() end ----\n", __func__);
    return(tmpF);
}

// XXX this is expensive in terms of malloc calls: the use of image interpolate and the residual images
// makes this somewhat painful.
static bool AddOrSubModel(psImage *image,
                          psImage *mask,
                          pmModel *model,
                          pmModelOpMode mode,
                          bool add,
                          psImageMaskType maskVal,
                          int dx,
                          int dy
    )
{
    psTrace("psModules.objects", 10, "---- %s() begin ----\n", __func__);

    PS_ASSERT_PTR_NON_NULL(model, false);
    PS_ASSERT_IMAGE_NON_NULL(image, false);
    PS_ASSERT_IMAGE_TYPE(image, PS_TYPE_F32, false);

    psVector *x = psVectorAlloc(2, PS_TYPE_F32);
    psVector *params = model->params;

    float imageCol;
    float imageRow;
    float pixelValue;

    // save original values; restore before returning
    // use the true source position for the residual model
    // the PSF model has presumably already been set for this coordinate
    float XoSave  = params->data.F32[PM_PAR_XPOS];
    float YoSave  = params->data.F32[PM_PAR_YPOS];
    float IoSave  = params->data.F32[PM_PAR_I0];
    float skySave = params->data.F32[PM_PAR_SKY];

    // the options allow us to modify various aspects of the model
    if (mode & PM_MODEL_OP_NORM) {
	// if we are including the sky, renormalizing should force use to normalized down the sky flux
	params->data.F32[PM_PAR_SKY] /= params->data.F32[PM_PAR_I0];
        params->data.F32[PM_PAR_I0] = 1.0;
    }
    if (!(mode & PM_MODEL_OP_SKY)) {
        params->data.F32[PM_PAR_SKY] = 0.0;
    } 
    if (mode & PM_MODEL_OP_CENTER) {
        params->data.F32[PM_PAR_XPOS] = image->col0 + 0.5*image->numCols;
        params->data.F32[PM_PAR_YPOS] = image->row0 + 0.5*image->numRows;
    }

    // apply optional relative offset
    // params->data.F32[PM_PAR_XPOS] += dx;
    // params->data.F32[PM_PAR_YPOS] += dy;

    // use these values for this realization
    float xCenter  = params->data.F32[PM_PAR_XPOS];
    float yCenter  = params->data.F32[PM_PAR_YPOS];
    float Io       = params->data.F32[PM_PAR_I0];

    int xBin = 1;
    int yBin = 1;
    float DX = 0.0;
    float DY = 0.0;
    int NX = 0;
    int NY = 0;

    psF32 **Ro = NULL;
    psF32 **Rx = NULL;
    psF32 **Ry = NULL;
    pmResidMaskType **Rm = NULL;

    if (model->residuals) {
	DX = xBin*(image->col0 - xCenter - dx) + model->residuals->xCenter + 0.5;
	DY = yBin*(image->row0 - yCenter - dy) + model->residuals->yCenter + 0.5;
	Ro = (model->residuals->Ro)   ? model->residuals->Ro->data.F32 : NULL;
	Rx = (model->residuals->Rx)   ? model->residuals->Rx->data.F32 : NULL;
	Ry = (model->residuals->Ry)   ? model->residuals->Ry->data.F32 : NULL;
	Rm = (model->residuals->mask) ? model->residuals->mask->data.PM_TYPE_RESID_MASK_DATA : NULL;
	if (Ro) {
	    NX = model->residuals->Ro->numCols;
	    NY = model->residuals->Ro->numRows;
	}	    
    }

    // XXX trying to improve the speed and threadability of this function.
    // note: model->residuals is a view to the item on pmPSF.

    for (psS32 iy = 0; iy < image->numRows; iy++) {
        for (psS32 ix = 0; ix < image->numCols; ix++) {
            if ((mask != NULL) && (mask->data.PS_TYPE_IMAGE_MASK_DATA[iy][ix] & maskVal))
                continue;

            // Convert to coordinate in parent image, with offset (dx,dy)
	    // 0.5 PIX: the model take pixel coordinates so convert the pixel index here
            imageCol = ix + 0.5 + image->col0 - dx;
            imageRow = iy + 0.5 + image->row0 - dy;

            x->data.F32[0] = imageCol;
            x->data.F32[1] = imageRow;

            pixelValue = 0.0;

            // add in the desired components for this coordinate
            if (mode & PM_MODEL_OP_FUNC) {
                pixelValue += model->class->modelFunc (NULL, params, x);
            }

            // get the contribution from the residual model
            if (Ro && (mode & PM_MODEL_OP_RES0)) {
                // residual image position
                float ry = yBin*iy + DY;
                float rx = xBin*ix + DX;

                int rx0 = rx - 0.5;
                int rx1 = rx + 0.5;
                int ry0 = ry - 0.5;
                int ry1 = ry + 0.5;

                if (rx0 < 0) goto skip;
                if (ry0 < 0) goto skip;
                if (rx1 >= NX) goto skip;
                if (ry1 >= NY) goto skip;

                // these go from 0.0 to 1.0 between the centers of the pixels
                float fx = rx - 0.5 - rx0;
                float Fx = 1.0 - fx;
                float fy = ry - 0.5 - ry0;
                float Fy = 1.0 - fy;

                // check the residual image mask (if set). give up if any of the 4 pixels are masked.
                if (Rm) {
                    if (Rm[ry0][rx0]) goto skip;
                    if (Rm[ry0][rx1]) goto skip;
                    if (Rm[ry1][rx0]) goto skip;
                    if (Rm[ry1][rx1]) goto skip;
                }

                // a possible further optimization if we re-use these values
                // XXX allow for masked pixels, and add pixel weights
                float V0 = (Ro[ry0][rx0]*Fx + Ro[ry0][rx1]*fx);
                float V1 = (Ro[ry1][rx0]*Fx + Ro[ry1][rx1]*fx);
                float Vo = V0*Fy + V1*fy;
                if (!isfinite(Vo)) goto skip;

                float Vx = 0.0;
                float Vy = 0.0;

                // skip Rx,Ry if Ro is masked
                if (Rx && Ry && (mode & PM_MODEL_OP_RES1)) {
                    V0 = (Rx[ry0][rx0]*Fx + Rx[ry0][rx1]*fx);
                    V1 = (Rx[ry1][rx0]*Fx + Rx[ry1][rx1]*fx);
                    Vx = V0*Fy + V1*fy;

                    V0 = (Ry[ry0][rx0]*Fx + Ry[ry0][rx1]*fx);
                    V1 = (Ry[ry1][rx0]*Fx + Ry[ry1][rx1]*fx);
                    Vy = V0*Fy + V1*fy;
                }
                if (!isfinite(Vx)) goto skip;
                if (!isfinite(Vy)) goto skip;

                // 2D residual variations are set for the true source position
                pixelValue += Io*(Vo + XoSave*Vx + YoSave*Vy);
            }

        skip:
            // add or subtract the value
            if (add) {
                image->data.F32[iy][ix] += pixelValue;
            } else {
                image->data.F32[iy][ix] -= pixelValue;
            }
        }
    }

    // restore original values
    params->data.F32[PM_PAR_XPOS] = XoSave;
    params->data.F32[PM_PAR_YPOS] = YoSave;
    params->data.F32[PM_PAR_I0]   = IoSave;
    params->data.F32[PM_PAR_SKY]  = skySave;

    psFree(x);
    psTrace("psModules.objects", 10, "---- %s(true) end ----\n", __func__);
    return(true);
}

/******************************************************************************
 *****************************************************************************/
bool pmModelAdd(psImage *image,
                psImage *mask,
                pmModel *model,
                pmModelOpMode mode,
                psImageMaskType maskVal)
{
    psTrace("psModules.objects", 10, "---- %s() begin ----\n", __func__);
    psBool rc = AddOrSubModel(image, mask, model, mode, true, maskVal, 0.0, 0.0);
    psTrace("psModules.objects", 10, "---- %s(%d) end ----\n", __func__, rc);
    return(rc);
}

/******************************************************************************
 *****************************************************************************/
bool pmModelSub(psImage *image,
                psImage *mask,
                pmModel *model,
                pmModelOpMode mode,
                psImageMaskType maskVal)
{
    psTrace("psModules.objects", 10, "---- %s() begin ----\n", __func__);
    psBool rc = AddOrSubModel(image, mask, model, mode, false, maskVal, 0.0, 0.0);
    psTrace("psModules.objects", 10, "---- %s(%d) end ----\n", __func__, rc);
    return(rc);
}

/******************************************************************************
 *****************************************************************************/
bool pmModelAddWithOffset(psImage *image,
                          psImage *mask,
                          pmModel *model,
                          pmModelOpMode mode,
                          psImageMaskType maskVal,
                          int dx,
                          int dy)
{
    psTrace("psModules.objects", 10, "---- %s() begin ----\n", __func__);
    psBool rc = AddOrSubModel(image, mask, model, mode, true, maskVal, dx, dy);
    psTrace("psModules.objects", 10, "---- %s(%d) end ----\n", __func__, rc);
    return(rc);
}

/******************************************************************************
 *****************************************************************************/
bool pmModelSubWithOffset(psImage *image,
                          psImage *mask,
                          pmModel *model,
                          pmModelOpMode mode,
                          psImageMaskType maskVal,
                          int dx,
                          int dy)
{
    psTrace("psModules.objects", 10, "---- %s() begin ----\n", __func__);
    psBool rc = AddOrSubModel(image, mask, model, mode, false, maskVal, dx, dy);
    psTrace("psModules.objects", 10, "---- %s(%d) end ----\n", __func__, rc);
    return(rc);
}
