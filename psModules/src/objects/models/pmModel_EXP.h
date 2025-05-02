#ifndef PM_MODEL_EXP_H

#include "pmModel.h"

psF32 pmModelFunc_EXP(psVector *deriv, const psVector *params, const psVector *pixcoord);
bool pmModelLimits_EXP(psMinConstraintMode mode, int nParam, float *params, float *beta);
bool pmModelGuess_EXP(pmModel *model, pmSource *source, psImageMaskType maskVal, psImageMaskType markVal);
psF64 pmModelFlux_EXP(const psVector *params);
psF64 pmModelRadius_EXP(const psVector *params, psF64 flux);
psF64 pmModelSetFWHM_EXP(const psVector *params, psF64 flux);
bool pmModelFromPSF_EXP(pmModel *modelPSF, pmModel *modelFLT, const pmPSF *psf);
bool  pmModelParamsFromPSF_EXP(pmModel *model, const pmPSF *psf, float Xo, float Yo, float Io);
bool pmModelFitStatus_EXP(pmModel *model);
void pmModelSetLimits_EXP(pmModelLimitsType type);

#endif
