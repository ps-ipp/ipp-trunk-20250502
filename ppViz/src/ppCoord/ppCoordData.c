#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <pslib.h>
#include <psmodules.h>

#include "ppCoord.h"

// Destructor
static void coordDataFree(ppCoordData *data // Data to free
    )
{
    psFree(data->astromName);
    psFree(data->rawName);
    psFree(data->pixelsName);
    psFree(data->chipName);
    psFree(data->radecName);
    psFree(data->streaksName);
    psFree(data->config);
    psFree(data->ds9name);
    if (data->ds9) {
        fclose(data->ds9);
    }
    psFree(data->ds9color);
    return;
}


ppCoordData *ppCoordDataAlloc(void)
{
    ppCoordData *data = psAlloc(sizeof(ppCoordData)); // Processing data, to return
    psMemSetDeallocator(data, (psFreeFunc)coordDataFree);

    data->astromName = NULL;
    data->rawName = NULL;
    data->pixelsName = NULL;
    data->chipName = NULL;
    data->radecName = NULL;
    data->streaksName = NULL;
    data->config = NULL;
    data->radians = false;
    data->all = false;
    data->ds9name = NULL;
    data->ds9 = NULL;
    data->ds9radius = NAN;
    data->ds9color = NULL;

    return data;
}


ppCoordData *ppCoordDataInit(int *argc, char **argv)
{
    PS_ASSERT_PTR_NON_NULL(argc, NULL);
    PS_ASSERT_PTR_NON_NULL(argv, NULL);

    ppCoordData *data = ppCoordDataAlloc(); // Processing data, to return
    data->config = pmConfigRead(argc, argv, NULL);
    if (!data->config) {
        psError(PS_ERR_UNKNOWN, false, "Unable to read configuration.");
        return NULL;
    }
    return data;
}
