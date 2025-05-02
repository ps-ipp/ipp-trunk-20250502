#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>

#include <pslib.h>

#include "pmConfig.h"
#include "pmFPA.h"
#include "pmFPAview.h"


psMetadataItem *pmConfigRecipeValueByView(const pmConfig *config, // Configuration
                                          const char *recipeName, // Name of recipe
                                          const char *valueName,  // Name of value in recipe
                                          const pmFPA *fpa,       // FPA of interest
                                          const pmFPAview *view   // View to component of interest
    )
{
    PS_ASSERT_PTR_NON_NULL(config, NULL);
    PS_ASSERT_METADATA_NON_NULL(config->recipes, NULL);
    PS_ASSERT_STRING_NON_EMPTY(recipeName, NULL);
    PS_ASSERT_STRING_NON_EMPTY(valueName, NULL);
    PS_ASSERT_PTR_NON_NULL(fpa, NULL);
    PS_ASSERT_PTR_NON_NULL(view, NULL);

    psMetadata *recipe = psMetadataLookupMetadata(NULL, config->recipes, recipeName); // Recipe of interest
    if (!recipe) {
        psError(PS_ERR_UNEXPECTED_NULL, false, "Unable to find recipe %s", recipeName);
        return NULL;
    }
    psMetadata *format = config->format;
    
    psMetadataItem *fpaItem;
    fpaItem = psMetadataLookup(format, valueName);
    if (!fpaItem) {
      fpaItem = psMetadataLookup(recipe, valueName); // Value for FPA or menu of chips
    }
    
    if (!fpaItem) {
        psError(PS_ERR_UNEXPECTED_NULL, false, "Unable to find value %s in recipe %s", valueName, recipeName);
        return NULL;
    }
    if (view->chip == -1 && view->cell == -1) {
        // Default value only
        return fpaItem;
    }
    if (fpaItem->type != PS_DATA_METADATA) {
        // Single value appropriate for all
        return fpaItem;
    }

    psMetadata *menu = fpaItem->data.md;   // Menu with values by chip

    pmChip *chip = pmFPAviewThisChip(view, fpa);                                   // Chip of interest
    const char *chipName = psMetadataLookupStr(NULL, chip->concepts, "CHIP.NAME"); // Name of chip

    psMetadataItem *chipItem = psMetadataLookup(menu, chipName); // Value for chip of menu of cells
    if (!chipItem) {
        psError(PS_ERR_UNEXPECTED_NULL, true,
                "Chip %s does not have a menu entry in %s within recipe %s.",
                  chipName, valueName, recipeName);
        return NULL;
    }
    if (view->cell == -1) {
        // Default value for chip
        return chipItem;
    }
    if (chipItem->type != PS_DATA_METADATA) {
        // Single value appropriate for all
        return chipItem;
    }

    menu = chipItem->data.md;   // Menu with values by chip

    pmCell *cell = pmFPAviewThisCell(view, fpa);                                   // Cell of interest
    const char *cellName = psMetadataLookupStr(NULL, cell->concepts, "CELL.NAME"); // Name of cell

    psMetadataItem *cellItem = psMetadataLookup(menu, cellName); // Value for cell of menu of cells
    if (!cellItem) {
        psError(PS_ERR_UNEXPECTED_NULL, true,
                "Chip %s, cell %s does not have a menu entry in %s within recipe %s --- using default.",
                chipName, cellName, valueName, recipeName);
        return NULL;
    }

    return cellItem;
}
