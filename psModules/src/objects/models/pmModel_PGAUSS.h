#ifndef PM_MODEL_PGAUSS_H

#include "pmModel.h"

psF32 pmModelFunc_PGAUSS(psVector *deriv, const psVector *params, const psVector *pixcoord);
bool pmModelLimits_PGAUSS(psMinConstraintMode mode, int nParam, float *params, float *beta);
bool pmModelGuess_PGAUSS(pmModel *model, pmSource *source, psImageMaskType maskVal, psImageMaskType markVal);
psF64 pmModelFlux_PGAUSS(const psVector *params);
psF64 pmModelRadius_PGAUSS(const psVector *params, psF64 flux);
psF64 pmModelSetFWHM_PGAUSS(const psVector *params, psF64 flux);
bool pmModelFromPSF_PGAUSS(pmModel *modelPSF, pmModel *modelFLT, const pmPSF *psf);
bool  pmModelParamsFromPSF_PGAUSS(pmModel *model, const pmPSF *psf, float Xo, float Yo, float Io);
bool pmModelFitStatus_PGAUSS(pmModel *model);
void pmModelSetLimits_PGAUSS(pmModelLimitsType type);

#endif
