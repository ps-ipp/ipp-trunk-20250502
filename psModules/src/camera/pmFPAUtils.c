#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <pslib.h>

#include "pmHDU.h"
#include "pmFPA.h"
#include "pmFPAUtils.h"

int pmFPAFindChip(const pmFPA *fpa, const char *name)
{
    PS_ASSERT_PTR_NON_NULL(fpa, -1);
    PS_ASSERT_PTR_NON_NULL(name, -1);
    if (strlen(name) == 0) {
        return -1;
    }

    psArray *chips = fpa->chips;        // Array of chips
    for (int i = 0; i < chips->n; i++) {
        pmChip *chip = chips->data[i]; // The chip of interest
        psString testName = psMetadataLookupStr(NULL, chip->concepts, "CHIP.NAME"); // Name of this chip
        if (strcmp(name, testName) == 0) {
            return i;
        }
    }

    psError(PS_ERR_IO, true, "Unable to find chip %s\n", name);
    return -1;
}


int pmChipFindCell(const pmChip *chip, const char *name)
{
    PS_ASSERT_PTR_NON_NULL(chip, -1);
    PS_ASSERT_PTR_NON_NULL(name, -1);
    if (strlen(name) == 0) {
        return -1;
    }

    psArray *cells = chip->cells;    // Array of cells
    for (int i = 0; i < cells->n; i++) {
        pmCell *cell = cells->data[i]; // The cell of interest
        psString testName = psMetadataLookupStr(NULL, cell->concepts, "CELL.NAME"); // Name of this cell
        if (strcmp(name, testName) == 0) {
            return i;
        }
    }

    psError(PS_ERR_IO, true, "Unable to find cell %s\n", name);
    return -1;
}

