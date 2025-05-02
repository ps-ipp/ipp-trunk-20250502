#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <pslib.h>
#include <psmodules.h>

#include "ppBackgroundStack.h"

// Destructor
static void backgroundDataFree(ppBackgroundStackData *data // Data to free
    )
{
  // CZW 2014-03-24
  // The things that are commented out here, and therefore don't get freed are in that state because
  // I'd far rather spend time sorting out actual math issues than dealing with data structures.
  
  //  psFree(data->contents);
  
//  psFree(data->smfs);

  psFree(data->models);
  psFree(data->OTA_solutions);
  psFree(data->OTApath);

  psFree(data->modelMap);


  psFree(data->stack_data);
  psFree(data->stacks);
  psFree(data->outRoot);
  //psFree(data->config);
  return;
}


ppBackgroundStackData *ppBackgroundStackDataAlloc(void)
{
    ppBackgroundStackData *data = psAlloc(sizeof(ppBackgroundStackData)); // Processing data, to return
    psMemSetDeallocator(data, (psFreeFunc)backgroundDataFree);

    data->contents = NULL;
    data->smfs = psArrayAlloc(0);

    data->models = psMetadataAlloc();
    data->OTA_solutions = psMetadataAlloc();
    data->fit_OTAS = false;
    data->OTApath = NULL;

    data->modelMap = NULL;
    data->model_iteration = 0;
    data->ra_min   = 1e9;
    data->ra_max   = -1e9;
    data->dec_min  = 1e9;
    data->dec_max  = -1e9;

    data->stack_data = psArrayAlloc(0);
    data->stacks = psArrayAlloc(0);
    data->outRoot = NULL;
    data->config = NULL;

    return data;
}


ppBackgroundStackData *ppBackgroundStackDataInit(int *argc, char **argv)
{
    PS_ASSERT_PTR_NON_NULL(argc, NULL);
    PS_ASSERT_PTR_NON_NULL(argv, NULL);

    ppBackgroundStackData *data = ppBackgroundStackDataAlloc(); // Processing data, to return
    data->config = pmConfigRead(argc, argv, PPBACKGROUND_RECIPE);
    if (!data->config) {
        psError(psErrorCodeLast(), false, "Unable to read configuration.");
        return NULL;
    }
    return data;
}
