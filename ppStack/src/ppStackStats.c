#include "ppStack.h"

bool ppStackStats(ppStackThreadData *stack, ppStackOptions *options, pmConfig *config)
{
    psAssert(stack, "Require stack");
    psAssert(options, "Require options");
    psAssert(config, "Require configuration");

    psMetadata *recipe = psMetadataLookupMetadata(NULL, config->recipes, PPSTACK_RECIPE); // ppStack recipe
    psAssert(recipe, "We've thrown an error on this before.");

    // Statistics on output
    if (options->stats) {
        psTrace("ppStack", 1, "Gathering statistics on stacked image....\n");
        psString maskBadStr = psMetadataLookupStr(NULL, recipe, "MASK.BAD"); // Name of bits for bad
        psImageMaskType maskBad = pmConfigMaskGet(maskBadStr, config); // Bits to mask for bad pixels

        pmFPAview *view = pmFPAviewAlloc(0); // View to readout
        view->chip = view->cell = view->readout = 0;

        ppStatsFPA(options->stats, options->outRO->parent->parent->parent, view, maskBad, config);

        psFree(view);
    }

    return true;
}
