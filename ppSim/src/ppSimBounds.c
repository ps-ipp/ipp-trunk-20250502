#include "ppSim.h"

// Return bounds of a chip, based on the concepts
psRegion *ppSimChipBounds(const pmChip *chip, // Chip for which to determine size
                            pmFPAview *view // View for chip
    )
{
    assert(chip);
    assert(view);

    // Bounds of chip
    int xMin = +INT_MAX;
    int xMax = -INT_MAX;
    int yMin = +INT_MAX;
    int yMax = -INT_MAX;

    int x0Chip = psMetadataLookupS32(NULL, chip->concepts, "CHIP.X0");
    int y0Chip = psMetadataLookupS32(NULL, chip->concepts, "CHIP.Y0");
    int xParityChip = psMetadataLookupS32(NULL, chip->concepts, "CHIP.XPARITY");
    int yParityChip = psMetadataLookupS32(NULL, chip->concepts, "CHIP.YPARITY");

    if ((xParityChip != 1 && xParityChip != -1) || (yParityChip != 1 && yParityChip != -1)) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Chip parities are not set.");
        psFree(view);
        return NULL;
    }

    pmCell *cell;                   // Cell from chip
    while ((cell = pmFPAviewNextCell(view, chip->parent, 1))) {
        int xSize = psMetadataLookupS32(NULL, cell->concepts, "CELL.XSIZE");
        int ySize = psMetadataLookupS32(NULL, cell->concepts, "CELL.YSIZE");

        if (xSize == 0 || ySize == 0) {
            psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Cell sizes are not set.");
            psFree(view);
            return NULL;
        }

        int x0Cell = psMetadataLookupS32(NULL, cell->concepts, "CELL.X0");
        int y0Cell = psMetadataLookupS32(NULL, cell->concepts, "CELL.Y0");
        int xParityCell = psMetadataLookupS32(NULL, cell->concepts, "CELL.XPARITY");
        int yParityCell = psMetadataLookupS32(NULL, cell->concepts, "CELL.YPARITY");

        if ((xParityCell != 1 && xParityCell != -1) || (yParityCell != 1 && yParityCell != -1)) {
            psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Cell parities are not set.");
            psFree(view);
            return NULL;
        }

        // "Left", "Right", "Bottom" and "Top" don't take into account the parity
        int cellLeft   = PPSIM_CELL_TO_FPA(0,     x0Cell, xParityCell, 1, x0Chip, xParityChip);
        int cellRight  = PPSIM_CELL_TO_FPA(xSize, x0Cell, xParityCell, 1, x0Chip, xParityChip);
        int cellBottom = PPSIM_CELL_TO_FPA(0,     y0Cell, yParityCell, 1, y0Chip, yParityChip);
        int cellTop    = PPSIM_CELL_TO_FPA(ySize, y0Cell, yParityCell, 1, y0Chip, yParityChip);

        COMPARE(cellLeft,   xMin, xMax);
        COMPARE(cellRight,  xMin, xMax);
        COMPARE(cellBottom, yMin, yMax);
        COMPARE(cellTop,    yMin, yMax);
    }

    return psRegionAlloc(xMin, xMax, yMin, yMax);
}

// Return bounds of an FPA, based on the concepts
psRegion *ppSimFPABounds(const pmFPA *fpa       // FPA for which to determine size
    )
{
    assert(fpa);

    pmFPAview *view = pmFPAviewAlloc(0);// View for iterating over FPA

    // Bounds of focal plane
    int xMin = +INT_MAX;
    int xMax = -INT_MAX;
    int yMin = +INT_MAX;
    int yMax = -INT_MAX;

    pmChip *chip;                       // Chip from FPA
    while ((chip = pmFPAviewNextChip(view, fpa, 1))) {
        psRegion *bounds = ppSimChipBounds(chip, view); // Bounds for chip
        COMPARE(bounds->x0, xMin, xMax);
        COMPARE(bounds->x1, xMin, xMax);
        COMPARE(bounds->y0, yMin, yMax);
        COMPARE(bounds->y1, yMin, yMax);
        psFree(bounds);
    }

    psFree(view);
    psRegion *bounds = psRegionAlloc(xMin, xMax, yMin, yMax);

    if (!bounds || (bounds->x0 == 0.0 && bounds->x1 == 0.0) || (bounds->y0 == 0.0 && bounds->y1 == 0.0)) {
        psError(PS_ERR_UNKNOWN, false, "Unable to determine bounds of FPA");
        return NULL;
    }

    return bounds;
}
