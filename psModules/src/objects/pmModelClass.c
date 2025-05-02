/** @file  pmModelClass.c
 *
 *  Functions to define and manipulate object model attributes
 *
 *  @author GLG, MHPCC
 *  @author EAM, IfA
 *
 *  @version $Revision: 1.7 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2008-12-08 02:51:14 $
 *
 *  Copyright 2004 Maui High Performance Computing Center, University of Hawaii
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <pslib.h>
#include "pmHDU.h"
#include "pmFPA.h"

#include "pmTrend2D.h"
#include "pmResiduals.h"
#include "pmGrowthCurve.h"
#include "pmSpan.h"
#include "pmFootprintSpans.h"
#include "pmFootprint.h"
#include "pmPeaks.h"
#include "pmMoments.h"
#include "pmModelFuncs.h"
#include "pmModelClass.h"
#include "pmModel.h"
#include "pmModelUtils.h"

#include "pmErrorCodes.h"

// XXX shouldn't these be defined for us in math.h ???
double hypot(double x, double y);
double sqrt (double x);

# include "models/pmModel_GAUSS.h"
# include "models/pmModel_PGAUSS.h"
# include "models/pmModel_QGAUSS.h"
# include "models/pmModel_PS1_V1.h"
# include "models/pmModel_HSC_V1.h"
# include "models/pmModel_RGAUSS.h"
# include "models/pmModel_SERSIC.h"
# include "models/pmModel_EXP.h"
# include "models/pmModel_DEV.h"
# include "models/pmModel_TRAIL.h"

static pmModelClass defaultModels[] = {
    {"PS_MODEL_GAUSS",        7, 0, (pmModelFunc)pmModelFunc_GAUSS,   (pmModelFlux)pmModelFlux_GAUSS,   (pmModelRadius)pmModelRadius_GAUSS,   (pmModelSetFWHM)pmModelSetFWHM_GAUSS,   (pmModelLimits)pmModelLimits_GAUSS,   (pmModelGuessFunc)pmModelGuess_GAUSS,  (pmModelFromPSFFunc)pmModelFromPSF_GAUSS,  (pmModelParamsFromPSF)pmModelParamsFromPSF_GAUSS,  (pmModelFitStatusFunc)pmModelFitStatus_GAUSS,  (pmModelSetLimitsFunc)pmModelSetLimits_GAUSS  },
    {"PS_MODEL_PGAUSS",       7, 0, (pmModelFunc)pmModelFunc_PGAUSS,  (pmModelFlux)pmModelFlux_PGAUSS,  (pmModelRadius)pmModelRadius_PGAUSS,  (pmModelSetFWHM)pmModelSetFWHM_PGAUSS,  (pmModelLimits)pmModelLimits_PGAUSS,  (pmModelGuessFunc)pmModelGuess_PGAUSS, (pmModelFromPSFFunc)pmModelFromPSF_PGAUSS, (pmModelParamsFromPSF)pmModelParamsFromPSF_PGAUSS, (pmModelFitStatusFunc)pmModelFitStatus_PGAUSS, (pmModelSetLimitsFunc)pmModelSetLimits_PGAUSS },
    {"PS_MODEL_QGAUSS",       8, 0, (pmModelFunc)pmModelFunc_QGAUSS,  (pmModelFlux)pmModelFlux_QGAUSS,  (pmModelRadius)pmModelRadius_QGAUSS,  (pmModelSetFWHM)pmModelSetFWHM_QGAUSS,  (pmModelLimits)pmModelLimits_QGAUSS,  (pmModelGuessFunc)pmModelGuess_QGAUSS, (pmModelFromPSFFunc)pmModelFromPSF_QGAUSS, (pmModelParamsFromPSF)pmModelParamsFromPSF_QGAUSS, (pmModelFitStatusFunc)pmModelFitStatus_QGAUSS, (pmModelSetLimitsFunc)pmModelSetLimits_QGAUSS },
    {"PS_MODEL_PS1_V1",       8, 0, (pmModelFunc)pmModelFunc_PS1_V1,  (pmModelFlux)pmModelFlux_PS1_V1,  (pmModelRadius)pmModelRadius_PS1_V1,  (pmModelSetFWHM)pmModelSetFWHM_PS1_V1,  (pmModelLimits)pmModelLimits_PS1_V1,  (pmModelGuessFunc)pmModelGuess_PS1_V1, (pmModelFromPSFFunc)pmModelFromPSF_PS1_V1, (pmModelParamsFromPSF)pmModelParamsFromPSF_PS1_V1, (pmModelFitStatusFunc)pmModelFitStatus_PS1_V1, (pmModelSetLimitsFunc)pmModelSetLimits_PS1_V1 },
    {"PS_MODEL_RGAUSS",       8, 0, (pmModelFunc)pmModelFunc_RGAUSS,  (pmModelFlux)pmModelFlux_RGAUSS,  (pmModelRadius)pmModelRadius_RGAUSS,  (pmModelSetFWHM)pmModelSetFWHM_RGAUSS,  (pmModelLimits)pmModelLimits_RGAUSS,  (pmModelGuessFunc)pmModelGuess_RGAUSS, (pmModelFromPSFFunc)pmModelFromPSF_RGAUSS, (pmModelParamsFromPSF)pmModelParamsFromPSF_RGAUSS, (pmModelFitStatusFunc)pmModelFitStatus_RGAUSS, (pmModelSetLimitsFunc)pmModelSetLimits_RGAUSS },
    {"PS_MODEL_SERSIC",       8, 1, (pmModelFunc)pmModelFunc_SERSIC,  (pmModelFlux)pmModelFlux_SERSIC,  (pmModelRadius)pmModelRadius_SERSIC,  (pmModelSetFWHM)pmModelSetFWHM_SERSIC,  (pmModelLimits)pmModelLimits_SERSIC,  (pmModelGuessFunc)pmModelGuess_SERSIC, (pmModelFromPSFFunc)pmModelFromPSF_SERSIC, (pmModelParamsFromPSF)pmModelParamsFromPSF_SERSIC, (pmModelFitStatusFunc)pmModelFitStatus_SERSIC, (pmModelSetLimitsFunc)pmModelSetLimits_SERSIC },
    {"PS_MODEL_EXP",          7, 1, (pmModelFunc)pmModelFunc_EXP,     (pmModelFlux)pmModelFlux_EXP,     (pmModelRadius)pmModelRadius_EXP,     (pmModelSetFWHM)pmModelSetFWHM_EXP,     (pmModelLimits)pmModelLimits_EXP,     (pmModelGuessFunc)pmModelGuess_EXP,    (pmModelFromPSFFunc)pmModelFromPSF_EXP,    (pmModelParamsFromPSF)pmModelParamsFromPSF_EXP,    (pmModelFitStatusFunc)pmModelFitStatus_EXP,    (pmModelSetLimitsFunc)pmModelSetLimits_EXP    },
    {"PS_MODEL_DEV",          7, 1, (pmModelFunc)pmModelFunc_DEV,     (pmModelFlux)pmModelFlux_DEV,     (pmModelRadius)pmModelRadius_DEV,     (pmModelSetFWHM)pmModelSetFWHM_DEV,     (pmModelLimits)pmModelLimits_DEV,     (pmModelGuessFunc)pmModelGuess_DEV,    (pmModelFromPSFFunc)pmModelFromPSF_DEV,    (pmModelParamsFromPSF)pmModelParamsFromPSF_DEV,    (pmModelFitStatusFunc)pmModelFitStatus_DEV,    (pmModelSetLimitsFunc)pmModelSetLimits_DEV    },
    {"PS_MODEL_TRAIL",        7, 0, (pmModelFunc)pmModelFunc_TRAIL,   (pmModelFlux)pmModelFlux_TRAIL,   (pmModelRadius)pmModelRadius_TRAIL,   (pmModelSetFWHM)pmModelSetFWHM_TRAIL,   (pmModelLimits)pmModelLimits_TRAIL,   (pmModelGuessFunc)pmModelGuess_TRAIL,  (pmModelFromPSFFunc)pmModelFromPSF_TRAIL,  (pmModelParamsFromPSF)pmModelParamsFromPSF_TRAIL,  (pmModelFitStatusFunc)pmModelFitStatus_TRAIL,  (pmModelSetLimitsFunc)pmModelSetLimits_TRAIL  },
    {"PS_MODEL_HSC_V1",       8, 0, (pmModelFunc)pmModelFunc_HSC_V1,  (pmModelFlux)pmModelFlux_HSC_V1,  (pmModelRadius)pmModelRadius_HSC_V1,  (pmModelSetFWHM)pmModelSetFWHM_HSC_V1,  (pmModelLimits)pmModelLimits_HSC_V1,  (pmModelGuessFunc)pmModelGuess_HSC_V1, (pmModelFromPSFFunc)pmModelFromPSF_HSC_V1, (pmModelParamsFromPSF)pmModelParamsFromPSF_HSC_V1, (pmModelFitStatusFunc)pmModelFitStatus_HSC_V1, (pmModelSetLimitsFunc)pmModelSetLimits_HSC_V1 },
};

static pmModelClass *models = NULL;
static psVector *modelClassLookupTable = NULL;  // translation between model types in header and here
static int Nmodels = 0;

static void ModelClassFree (pmModelClass *modelClass)
{
    if (modelClass == NULL)
        return;
    return;
}

pmModelClass *pmModelClassAlloc (int nModels)
{
    pmModelClass *modelClass = (pmModelClass *) psAlloc (nModels * sizeof(pmModelClass));
    psMemSetDeallocator(modelClass, (psFreeFunc) ModelClassFree);
    return (modelClass);
}

bool psMemCheckModelClass(psPtr ptr)
{
    PS_ASSERT_PTR(ptr, false);
    return ( psMemGetDeallocator(ptr) == (psFreeFunc) ModelClassFree);
}

void pmModelClassAdd (pmModelClass *model)
{
    if (models == NULL) {
        pmModelClassInit();
    }

    Nmodels ++;
    models = (pmModelClass *) psRealloc (models, Nmodels*sizeof(pmModelClass));
    models[Nmodels-1] = model[0];
    return;
}

bool pmModelClassInit (void)
{
    // if we do not need to init, return false;
    if (models != NULL) {
        return false;
    }

    int Nnew = sizeof (defaultModels) / sizeof (pmModelClass);

    models = pmModelClassAlloc (Nnew);
    for (int i = 0; i < Nnew; i++) {
        models[i] = defaultModels[i];
    }
    Nmodels = Nnew;
    return true;
}

pmModelClass *pmModelClassSelect (pmModelType type)
{
    if (models == NULL) {
        pmModelClassInit();
    }

    if ((type < 0) || (type >= Nmodels)) {
        psError(PS_ERR_UNKNOWN, true, "Undefined pmModelType");
        return (NULL);
    }
    return (&models[type]);
}

void pmModelClassCleanup (void)
{
    psFree (models);
    models = NULL;
    Nmodels = 0;
    psFree(modelClassLookupTable);
    modelClassLookupTable = NULL;
    return;
}

psS32 pmModelClassParameterCount (pmModelType type)
{
    if (models == NULL) {
        pmModelClassInit();
    }

    if ((type < 0) || (type >= Nmodels)) {
        psError(PS_ERR_UNKNOWN, true, "Undefined pmModelType");
        return (0);
    }
    return (models[type].nParams);
}

psS32 pmModelClassGetType (const char *name)
{
    if (models == NULL) {
        pmModelClassInit();
    }

    for (int i = 0; i < Nmodels; i++) {
        if (!strcmp(models[i].name, name)) {
            return (i);
        }
    }
    return (-1);
}

char *pmModelClassGetName (pmModelType type)
{
    if (models == NULL) {
        pmModelClassInit();
    }

    if ((type < 0) || (type >= Nmodels)) {
        psError(PS_ERR_UNKNOWN, true, "Undefined pmModelType");
        return (NULL);
    }
    return (models[type].name);
}


void pmModelClassSetLimits(pmModelLimitsType type)
{
    if (!models) {
        pmModelClassInit();
    }

    for (int i = 0; i < Nmodels; i++) {
        if (models[i].modelSetLimits) {
            models[i].modelSetLimits(type);
        }
    }

}


bool pmModelClassWriteHeader(psMetadata *header)
{
    psMetadataAddS32(header, PS_LIST_TAIL, "MTNUM", PS_META_REPLACE, "number of model types", Nmodels);
    for (int i = 0; i < Nmodels; i++) {
        char modelNameKey[16];
        char modelValKey[16];
        sprintf(modelNameKey, "MTNAM%02d", i);
        sprintf(modelValKey,  "MTVAL%02d", i);
        psMetadataAddStr(header, PS_LIST_TAIL, modelNameKey, PS_META_REPLACE, "", models[i].name);
        psMetadataAddS32(header, PS_LIST_TAIL, modelValKey, PS_META_REPLACE, "", i);
    }

    return true;
}

bool pmModelClassReadHeader(psMetadata *header) {
    psFree(modelClassLookupTable);

    bool status;
    int numHeaderModels = psMetadataLookupS32(&status, header, "MTNUM");
    if (!status) {
        return false;
    }

    psVector *inputTypes = psVectorAlloc(numHeaderModels, PS_TYPE_S32);
    psVector *localTypes = psVectorAlloc(numHeaderModels, PS_TYPE_S32);
    int max_val = -1;
    for (int i = 0; i < numHeaderModels; i++) {
        char modelNameKey[16];
        char modelValKey[16];
        sprintf(modelNameKey, "MTNAM%02d", i);
        sprintf(modelValKey,  "MTVAL%02d", i);
        psString thisName = psMetadataLookupStr(&status, header, modelNameKey);
        int thisVal = psMetadataLookupS32(&status, header, modelValKey);
        if (thisVal > max_val) {
            max_val = thisVal;
        }
        inputTypes->data.S32[i] = thisVal;
        localTypes->data.S32[i] = pmModelClassGetType(thisName);
    }
    if (max_val < 0) {
        psFree(inputTypes);
        psFree(localTypes);
        return false;
    }

    modelClassLookupTable = psVectorAlloc(max_val + 1, PS_TYPE_S32);
    psVectorInit(modelClassLookupTable, -1);

    for (int i = 0; i < numHeaderModels; i++) {
        int thisVal = inputTypes->data.S32[i];
        int localVal = localTypes->data.S32[i];
        modelClassLookupTable->data.S32[thisVal] = localVal;
    }
    psFree(inputTypes);
    psFree(localTypes);

    return true;
}

pmModelType pmModelClassGetLocalType(pmModelType inputType) {
    pmModelType localType = -1;

    if (modelClassLookupTable) {
        if (inputType >= 0 && inputType < modelClassLookupTable->n) {
            localType = modelClassLookupTable->data.S32[inputType];
        }
    } else {
        // no lookup table defined
        // for backwards compatability if inputType refers to a defined model, return it
        if (inputType >= 0 && pmModelClassGetName(inputType)) {
            localType = inputType;
        }
    }

    return localType;
}
