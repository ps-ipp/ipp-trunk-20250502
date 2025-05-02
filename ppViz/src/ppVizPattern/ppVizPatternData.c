#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <pslib.h>
#include <psmodules.h>

#include "ppVizPattern.h"

// Destructor
static void vizPatternDataFree(ppVizPatternData *data // Data to free
    )
{
    psFree(data->patternName);
    psFree(data->outRoot);
    psFree(data->config);
    return;
}


ppVizPatternData *ppVizPatternDataAlloc(void)
{
    ppVizPatternData *data = psAlloc(sizeof(ppVizPatternData)); // Processing data, to return
    psMemSetDeallocator(data, (psFreeFunc)vizPatternDataFree);

    data->patternName = NULL;
    data->outRoot = NULL;
    data->config = NULL;

    return data;
}


ppVizPatternData *ppVizPatternDataInit(int *argc, char **argv)
{
    PS_ASSERT_PTR_NON_NULL(argc, NULL);
    PS_ASSERT_PTR_NON_NULL(argv, NULL);

    ppVizPatternData *data = ppVizPatternDataAlloc(); // Processing data, to return
    data->config = pmConfigRead(argc, argv, PPVIZPATTERN_RECIPE);
    if (!data->config) {
        psError(PS_ERR_UNKNOWN, false, "Unable to read configuration.");
        return NULL;
    }
    return data;
}
