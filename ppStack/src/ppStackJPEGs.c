#include "ppStack.h"

bool ppStackJPEGs(ppStackThreadData *stack, ppStackOptions *options, pmConfig *config)
{
    psAssert(stack, "Require stack");
    psAssert(options, "Require options");
    psAssert(config, "Require configuration");

    psMetadata *recipe = psMetadataLookupMetadata(NULL, config->recipes, PPSTACK_RECIPE); // ppStack recipe
    psAssert(recipe, "We've thrown an error on this before.");

    // Generate binned JPEGs
    int bin1 = psMetadataLookupS32(NULL, recipe, "BIN1"); // First binning level
    int bin2 = psMetadataLookupS32(NULL, recipe, "BIN2"); // Second binning level

    // Target cells
    pmFPAview *view = pmFPAviewAlloc(0); // View to cells of interest
    view->chip = view->cell = 0;
    pmCell *cell1 = pmFPAfileThisCell(config->files, view, "PPSTACK.OUTPUT.JPEG1");
    pmCell *cell2 = pmFPAfileThisCell(config->files, view, "PPSTACK.OUTPUT.JPEG2");
    psImageMaskType maskValue = pmConfigMaskGet("BLANK", config); // Bits to mask
    psFree(view);

    pmReadout *ro1 = pmReadoutAlloc(cell1), *ro2 = pmReadoutAlloc(cell2); // Binned readouts
    if (!pmReadoutRebin(ro1, options->outRO, maskValue, bin1, bin1) ||
	!pmReadoutRebin(ro2, ro1, 0, bin2, bin2)) {
	psError(PPSTACK_ERR_DATA, false, "Unable to bin output.");
	psFree(ro1);
	psFree(ro2);
	return false;
    }
    psFree(ro1);
    psFree(ro2);

    pmFPAfileActivate(config->files, true, "PPSTACK.OUTPUT.JPEG1");
    pmFPAfileActivate(config->files, true, "PPSTACK.OUTPUT.JPEG2");

    return true;
}
