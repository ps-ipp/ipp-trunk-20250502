#ifndef PM_MODEL_HSC_V1_H

#include "pmModel.h"

psF32 pmModelFunc_HSC_V1(psVector *deriv, const psVector *params, const psVector *pixcoord);
bool pmModelLimits_HSC_V1(psMinConstraintMode mode, int nParam, float *params, float *beta);
bool pmModelGuess_HSC_V1(pmModel *model, pmSource *source, psImageMaskType maskVal, psImageMaskType markVal);
psF64 pmModelFlux_HSC_V1(const psVector *params);
psF64 pmModelRadius_HSC_V1(const psVector *params, psF64 flux);
psF64 pmModelSetFWHM_HSC_V1(const psVector *params, psF64 flux);
bool pmModelFromPSF_HSC_V1(pmModel *modelPSF, pmModel *modelFLT, const pmPSF *psf);
bool  pmModelParamsFromPSF_HSC_V1(pmModel *model, const pmPSF *psf, float Xo, float Yo, float Io);
bool pmModelFitStatus_HSC_V1(pmModel *model);
void pmModelSetLimits_HSC_V1(pmModelLimitsType type);

#endif
