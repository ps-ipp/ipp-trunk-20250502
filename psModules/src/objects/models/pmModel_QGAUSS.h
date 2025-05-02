#ifndef PM_MODEL_QGAUSS_H

#include "pmModel.h"

psF32 pmModelFunc_QGAUSS(psVector *deriv, const psVector *params, const psVector *pixcoord);
bool pmModelLimits_QGAUSS(psMinConstraintMode mode, int nParam, float *params, float *beta);
bool pmModelGuess_QGAUSS(pmModel *model, pmSource *source, psImageMaskType maskVal, psImageMaskType markVal);
psF64 pmModelFlux_QGAUSS(const psVector *params);
psF64 pmModelRadius_QGAUSS(const psVector *params, psF64 flux);
psF64 pmModelSetFWHM_QGAUSS(const psVector *params, psF64 flux);
bool pmModelFromPSF_QGAUSS(pmModel *modelPSF, pmModel *modelFLT, const pmPSF *psf);
bool  pmModelParamsFromPSF_QGAUSS(pmModel *model, const pmPSF *psf, float Xo, float Yo, float Io);
bool pmModelFitStatus_QGAUSS(pmModel *model);
void pmModelSetLimits_QGAUSS(pmModelLimitsType type);

#endif
