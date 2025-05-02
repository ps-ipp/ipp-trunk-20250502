#ifndef PM_MODEL_DEV_H

#include "pmModel.h"

psF32 pmModelFunc_DEV(psVector *deriv, const psVector *params, const psVector *pixcoord);
bool pmModelLimits_DEV(psMinConstraintMode mode, int nParam, float *params, float *beta);
bool pmModelGuess_DEV(pmModel *model, pmSource *source, psImageMaskType maskVal, psImageMaskType markVal);
psF64 pmModelFlux_DEV(const psVector *params);
psF64 pmModelRadius_DEV(const psVector *params, psF64 flux);
psF64 pmModelSetFWHM_DEV(const psVector *params, psF64 flux);
bool pmModelFromPSF_DEV(pmModel *modelPSF, pmModel *modelFLT, const pmPSF *psf);
bool  pmModelParamsFromPSF_DEV(pmModel *model, const pmPSF *psf, float Xo, float Yo, float Io);
bool pmModelFitStatus_DEV(pmModel *model);
void pmModelSetLimits_DEV(pmModelLimitsType type);

#endif
