#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "dvoApplyCorr.h"

static void dvoApplyCorrOptionsFree(dvoApplyCorrOptions *options)
{
    if (options == NULL) return;

    return;
}

dvoApplyCorrOptions *dvoApplyCorrOptionsAlloc(void)
{
    dvoApplyCorrOptions *options = psAlloc(sizeof(dvoApplyCorrOptions));
    psMemSetDeallocator(options, (psFreeFunc)dvoApplyCorrOptionsFree);

    // Initialise options
    options->test = 0;
    return options;
}

dvoApplyCorrOptions *dvoApplyCorrOptionsParse(pmConfig *config)
{
    dvoApplyCorrOptions *options = dvoApplyCorrOptionsAlloc ();
    return options;
}
