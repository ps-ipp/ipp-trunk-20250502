#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <pslib.h>
#include <psmodules.h>

#include "ppSkycell.h"

// Destructor
static void skycellDataFree(ppSkycellData *data // Data to free
    )
{
    psFree(data->imagesName);
    psFree(data->wcsrefName);
    psFree(data->outRoot);
    psFree(data->config);
    return;
}


ppSkycellData *ppSkycellDataAlloc(void)
{
    ppSkycellData *data = psAlloc(sizeof(ppSkycellData)); // Processing data, to return
    psMemSetDeallocator(data, (psFreeFunc)skycellDataFree);

    data->imagesName = NULL;
    data->wcsrefName = NULL;
    data->outRoot = NULL;
    data->numInputs = 0;
    data->maskVal = 0;
    data->bin1 = 0;
    data->bin2 = 0;
    data->config = NULL;
    data->exptimeOrder = 0;
    return data;
}


ppSkycellData *ppSkycellDataInit(int *argc, char **argv)
{
    PS_ASSERT_PTR_NON_NULL(argc, NULL);
    PS_ASSERT_PTR_NON_NULL(argv, NULL);

    ppSkycellData *data = ppSkycellDataAlloc(); // Processing data, to return
    data->config = pmConfigRead(argc, argv, PPSKYCELL_RECIPE);
    return data;
}
