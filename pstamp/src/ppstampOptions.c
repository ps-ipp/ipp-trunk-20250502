#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <pslib.h>
#include "ppstampOptions.h"


static void pstampOptionsFree(ppstampOptions *options)
{
    psFree(options->chipName);
    psFree(options->headerAdditions);
}

ppstampOptions *ppstampOptionsAlloc(void)
{
    ppstampOptions *options = psAlloc(sizeof(ppstampOptions));
    psMemSetDeallocator(options, (psFreeFunc)pstampOptionsFree);

    options->roip.celestialCenter = false;
    options->roip.centerX         = 0;
    options->roip.centerY         = 0;
    options->roip.centerRA        = 0;
    options->roip.centerDEC       = 0;
    options->roip.celestialRange  = false;
    options->roip.dX   = 0;
    options->roip.dY   = 0;
    options->roip.dRA  = 0;
    options->roip.dDEC = 0;
    options->chipName  = NULL;
    options->cellName  = NULL;
    options->stage  = NULL;
    options->headerAdditions = NULL;
    options->censorMasked = false;
    options->writeJPEG = false;
    options->writeCMF = false;
    options->nocompress = false;
    options->wholeFile = false;
    options->outputFileRule = NULL;

    return options;
}
