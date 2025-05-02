#ifndef PM_MODEL_SERSIC_H

#include "pmModel.h"

psF32 pmModelFunc_SERSIC(psVector *deriv, const psVector *params, const psVector *pixcoord);
bool pmModelLimits_SERSIC(psMinConstraintMode mode, int nParam, float *params, float *beta);
bool pmModelGuess_SERSIC(pmModel *model, pmSource *source, psImageMaskType maskVal, psImageMaskType markVal);
psF64 pmModelFlux_SERSIC(const psVector *params);
psF64 pmModelRadius_SERSIC(const psVector *params, psF64 flux);
psF64 pmModelSetFWHM_SERSIC(const psVector *params, psF64 flux);
bool pmModelFromPSF_SERSIC(pmModel *modelPSF, pmModel *modelFLT, const pmPSF *psf);
bool  pmModelParamsFromPSF_SERSIC(pmModel *model, const pmPSF *psf, float Xo, float Yo, float Io);
bool pmModelFitStatus_SERSIC(pmModel *model);
void pmModelSetLimits_SERSIC(pmModelLimitsType type);

#endif
