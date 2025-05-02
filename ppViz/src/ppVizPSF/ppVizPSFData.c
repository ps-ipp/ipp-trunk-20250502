#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <pslib.h>
#include <psmodules.h>

#include "ppVizPSF.h"

// Destructor
static void vizPSFDataFree(ppVizPSFData *data // Data to free
    )
{
    psFree(data->psfName);
    psFree(data->sourcesName);
    psFree(data->outRoot);
    psFree(data->input);
    psFree(data->config);
    return;
}


ppVizPSFData *ppVizPSFDataAlloc(void)
{
    ppVizPSFData *data = psAlloc(sizeof(ppVizPSFData)); // Processing data, to return
    psMemSetDeallocator(data, (psFreeFunc)vizPSFDataFree);

    data->psfName = NULL;
    data->sourcesName = NULL;
    data->fakeNum = 0;
    data->fakeMag = NAN;
    data->outRoot = NULL;
    data->config = NULL;
    data->minFlux = NAN;
    data->size = 0;
    data->x = NAN;
    data->y = NAN;
    data->input = NULL;

    return data;
}


ppVizPSFData *ppVizPSFDataInit(int *argc, char **argv)
{
    PS_ASSERT_PTR_NON_NULL(argc, NULL);
    PS_ASSERT_PTR_NON_NULL(argv, NULL);

    ppVizPSFData *data = ppVizPSFDataAlloc(); // Processing data, to return
    data->config = pmConfigRead(argc, argv, PPVIZPSF_RECIPE);
    if (!data->config) {
        psError(PS_ERR_UNKNOWN, false, "Unable to read configuration.");
        return NULL;
    }
    return data;
}
