#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <pslib.h>
#include <psmodules.h>
#include <psphot.h>

#include "ppSub.h"


static void subDataFree(ppSubData *data)
{
    psAssert(!data->statsFile, "Statistics file still open.");
    psFree(data->statsName);
    psFree(data->stamps);
    psFree(data->psf);
    psFree(data->stats);

    psFree(data->config);

    return;
}

ppSubData *ppSubDataAlloc(pmConfig *config)
{
    ppSubData *data = psAlloc(sizeof(ppSubData)); // Processing data, to return
    psMemSetDeallocator(data, (psFreeFunc)subDataFree);

    data->config = config;
    data->quality = 0;
    data->photometry = false;
    data->inverse = false;
    data->saveInConv = false;
    data->saveRefConv = false;
    data->stamps = NULL;
    data->psf = NULL;
    data->statsName = NULL;
    data->statsFile = NULL;
    data->stats = psMetadataAlloc();
    psMetadataAddS32(data->stats, PS_LIST_TAIL, "QUALITY", 0, "Data quality", 0);

    return data;
}


void ppSubDataQuality(ppSubData *data, psErrorCode error, ppSubFiles files)
{
    psAssert(data, "Require processing data");

    if (psMetadataLookupS32(NULL, data->stats, "QUALITY") == 0) {
        data->quality = error;
        psMetadataAddS32(data->stats, PS_LIST_TAIL, "QUALITY", PS_META_REPLACE, "Data quality", error);
    }

    ppSubFilesActivate(data->config, files, false);

    psErrorClear();

    return;
}
