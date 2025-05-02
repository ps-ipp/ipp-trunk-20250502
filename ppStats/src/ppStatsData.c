#include "ppStatsInternal.h"

static void statsDataFree(ppStatsData *data // Data to free
    )
{
    if (data->fits) {
        psFitsClose(data->fits);
        data->fits = NULL;
    }
    psFree(data->fpa);
    psFree(data->view);
    psFree(data->headers);
    psFree(data->concepts);
    psFree(data->analysis);
    psFree(data->summary);

    psFree(data->stats);
    psFree(data->fileView);

    psFree(data->chips);
    psFree(data->cells);

    return;
}


ppStatsData *ppStatsDataAlloc(void)
{
    ppStatsData *data = psAlloc(sizeof(ppStatsData)); // Newly allocated data
    psMemSetDeallocator(data, (psFreeFunc)statsDataFree);

    data->fits = NULL;
    data->fpa = NULL;
    data->view = NULL;

    data->headers = psListAlloc(NULL);
    data->concepts = psListAlloc(NULL);
    data->analysis = psListAlloc(NULL);
    data->summary = psListAlloc(NULL);
    data->stats = psStatsAlloc(0);
    data->doStats = false;
    data->fileLevel = false;
    data->fileView = NULL;

    data->sample = 0;
    data->maskVal = 0;
    data->doFirstReadout3D = false;
    data->chips = psListAlloc(NULL);
    data->cells = psListAlloc(NULL);

    return data;
}

