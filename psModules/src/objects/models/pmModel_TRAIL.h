#ifndef PM_MODEL_TRAIL_H

#include "pmModel.h"

psF32 pmModelFunc_TRAIL(psVector *deriv, const psVector *params, const psVector *pixcoord);
bool pmModelLimits_TRAIL(psMinConstraintMode mode, int nParam, float *params, float *beta);
bool pmModelGuess_TRAIL(pmModel *model, pmSource *source, psImageMaskType maskVal, psImageMaskType markVal);
psF64 pmModelFlux_TRAIL(const psVector *params);
psF64 pmModelRadius_TRAIL(const psVector *params, psF64 flux);
psF64 pmModelSetFWHM_TRAIL(const psVector *params, psF64 flux);
bool pmModelFromPSF_TRAIL(pmModel *modelPSF, pmModel *modelFLT, const pmPSF *psf);
bool  pmModelParamsFromPSF_TRAIL(pmModel *model, const pmPSF *psf, float Xo, float Yo, float Io);
bool pmModelFitStatus_TRAIL(pmModel *model);
void pmModelSetLimits_TRAIL(pmModelLimitsType type);

#endif
