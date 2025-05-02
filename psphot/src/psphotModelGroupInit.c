# include "psphotInternal.h"

// Add locally-defined models here.  As these mature, they can be moved to
// psModule/src/objects/models

// # include "models/pmModel_TEST1.c"
// # include "models/pmModel_STRAIL.c"

static pmModelClass userModels[] = {
    // {"PS_MODEL_TEST1", 7, pmModelFunc_TEST1,  pmModelFlux_TEST1,  pmModelRadius_TEST1,  pmModelLimits_TEST1,  pmModelGuess_TEST1, pmModelFromPSF_TEST1, pmModelParamsFromPSF_TEST1, pmModelFitStatus_TEST1, NULL},
    // {"PS_MODEL_STRAIL", 9, pmModelFunc_STRAIL,  pmModelFlux_STRAIL,  pmModelRadius_STRAIL,  pmModelLimits_STRAIL,  pmModelGuess_STRAIL, pmModelFromPSF_STRAIL, pmModelParamsFromPSF_STRAIL, pmModelFitStatus_STRAIL, NULL},
};

void psphotModelClassInit (void)
{

    // if pmModelClassInit returns false, we have already init'ed
    if (!pmModelClassInit ()) return;

    int Nmodels = sizeof (userModels) / sizeof (pmModelClass);
    for (int i = 0; i < Nmodels; i++) {
        pmModelClassAdd (&userModels[i]);
    }
    return;
}
