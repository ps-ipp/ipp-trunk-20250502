#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "dvoMakeCorr.h"

static void dvoMakeCorrOptionsFree(dvoMakeCorrOptions *options)
{
    if (options == NULL) return;

    return;
}

dvoMakeCorrOptions *dvoMakeCorrOptionsAlloc(void)
{
    dvoMakeCorrOptions *options = psAlloc(sizeof(dvoMakeCorrOptions));
    psMemSetDeallocator(options, (psFreeFunc)dvoMakeCorrOptionsFree);

    // Initialise options
    options->test = 0;
    return options;
}

dvoMakeCorrOptions *dvoMakeCorrOptionsParse(pmConfig *config)
{
    dvoMakeCorrOptions *options = dvoMakeCorrOptionsAlloc ();
    return options;
}
