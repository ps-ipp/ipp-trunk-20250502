#ifndef PM_MODEL_GAUSS_H

#include "pmModel.h"

psF32 pmModelFunc_GAUSS(psVector *deriv, const psVector *params, const psVector *pixcoord);
bool pmModelLimits_GAUSS(psMinConstraintMode mode, int nParam, float *params, float *beta);
bool pmModelGuess_GAUSS(pmModel *model, pmSource *source, psImageMaskType maskVal, psImageMaskType markVal);
psF64 pmModelFlux_GAUSS(const psVector *params);
psF64 pmModelRadius_GAUSS(const psVector *params, psF64 flux);
psF64 pmModelSetFWHM_GAUSS(const psVector *params, psF64 flux);
bool pmModelFromPSF_GAUSS(pmModel *modelPSF, pmModel *modelFLT, const pmPSF *psf);
bool  pmModelParamsFromPSF_GAUSS(pmModel *model, const pmPSF *psf, float Xo, float Yo, float Io);
bool pmModelFitStatus_GAUSS(pmModel *model);
void pmModelSetLimits_GAUSS(pmModelLimitsType type);

#endif
