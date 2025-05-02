#ifndef PM_MODEL_PS1_V1_H

#include "pmModel.h"

psF32 pmModelFunc_PS1_V1(psVector *deriv, const psVector *params, const psVector *pixcoord);
bool pmModelLimits_PS1_V1(psMinConstraintMode mode, int nParam, float *params, float *beta);
bool pmModelGuess_PS1_V1(pmModel *model, pmSource *source, psImageMaskType maskVal, psImageMaskType markVal);
psF64 pmModelFlux_PS1_V1(const psVector *params);
psF64 pmModelRadius_PS1_V1(const psVector *params, psF64 flux);
psF64 pmModelSetFWHM_PS1_V1(const psVector *params, psF64 flux);
bool pmModelFromPSF_PS1_V1(pmModel *modelPSF, pmModel *modelFLT, const pmPSF *psf);
bool  pmModelParamsFromPSF_PS1_V1(pmModel *model, const pmPSF *psf, float Xo, float Yo, float Io);
bool pmModelFitStatus_PS1_V1(pmModel *model);
void pmModelSetLimits_PS1_V1(pmModelLimitsType type);

#endif
