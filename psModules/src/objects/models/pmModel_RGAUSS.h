#ifndef PM_MODEL_RGAUSS_H

#include "pmModel.h"

psF32 pmModelFunc_RGAUSS(psVector *deriv, const psVector *params, const psVector *pixcoord);
bool pmModelLimits_RGAUSS(psMinConstraintMode mode, int nParam, float *params, float *beta);
bool pmModelGuess_RGAUSS(pmModel *model, pmSource *source, psImageMaskType maskVal, psImageMaskType markVal);
psF64 pmModelFlux_RGAUSS(const psVector *params);
psF64 pmModelRadius_RGAUSS(const psVector *params, psF64 flux);
psF64 pmModelSetFWHM_RGAUSS(const psVector *params, psF64 flux);
bool pmModelFromPSF_RGAUSS(pmModel *modelPSF, pmModel *modelFLT, const pmPSF *psf);
bool  pmModelParamsFromPSF_RGAUSS(pmModel *model, const pmPSF *psf, float Xo, float Yo, float Io);
bool pmModelFitStatus_RGAUSS(pmModel *model);
void pmModelSetLimits_RGAUSS(pmModelLimitsType type);

#endif
