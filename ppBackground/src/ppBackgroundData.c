#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <pslib.h>
#include <psmodules.h>

#include "ppBackground.h"

// Destructor
static void backgroundDataFree(ppBackgroundData *data // Data to free
    )
{
    psFree(data->patternName);
    psFree(data->backgroundName);
    psFree(data->imageName);
    psFree(data->maskName);
    psFree(data->varianceName);
    psFree(data->auxMaskName);
    psFree(data->outRoot);
    psFree(data->stats);
    psFree(data->config);
    return;
}


ppBackgroundData *ppBackgroundDataAlloc(void)
{
    ppBackgroundData *data = psAlloc(sizeof(ppBackgroundData)); // Processing data, to return
    psMemSetDeallocator(data, (psFreeFunc)backgroundDataFree);

    data->patternName = NULL;
    data->backgroundName = NULL;
    data->imageName = NULL;
    data->maskName = NULL;
    data->varianceName = NULL;
    data->auxMaskName = NULL;
    data->outRoot = NULL;
    data->stats = NULL;
    data->statsFile = NULL;
    data->config = NULL;

    return data;
}


ppBackgroundData *ppBackgroundDataInit(int *argc, char **argv)
{
    PS_ASSERT_PTR_NON_NULL(argc, NULL);
    PS_ASSERT_PTR_NON_NULL(argv, NULL);

    ppBackgroundData *data = ppBackgroundDataAlloc(); // Processing data, to return
    data->config = pmConfigRead(argc, argv, PPBACKGROUND_RECIPE);
    if (!data->config) {
        psError(psErrorCodeLast(), false, "Unable to read configuration.");
        return NULL;
    }
    return data;
}
