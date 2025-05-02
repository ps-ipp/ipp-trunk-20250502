#include <pslib.h>

#include "pmHDU.h"
#include "pmFPA.h"

#include "ppstamp.h"

// Functions to calculate the image space boundaries of a given Chip or Cell
// These are calculated in coordinates that match the wcs transformation
// (adapted from pmFPAExtent.c)

// return cell pixels bounding the readout
static psRegion *ppstampReadoutRegion(const pmReadout *readout)
{
    PS_ASSERT_PTR_NON_NULL(readout, NULL);

    psImage *image = readout->image;    // Image from which to get dimensions
    if (!image) {
        return NULL;
    }

    // This is the difference between this function and pmReadoutExtent. 
    // pmReadoutExtent ignores the col0, row0 of the readout
   
    int col0 = 0;   // should be  image->col0 - readout->col0
    int row0 = 0;   //            image->row0 - readout->row0
   
    return psRegionAlloc(col0, col0 + image->numCols,
                         row0, row0 + image->numRows);
}

// return chip pixels bounding the cell (all readouts)
psRegion *ppstampCellRegion(const pmCell *cell)
{
    PS_ASSERT_PTR_NON_NULL(cell, NULL);

    psArray *readouts = cell->readouts; // Array of component readouts
    psRegion *cellExtent = psRegionAlloc(INFINITY, 0, INFINITY, 0); // Extent of cell
    for (long i = 0; i < readouts->n; i++) {
        pmReadout *readout = readouts->data[i]; // Readout of interest
        psRegion *roExtent = ppstampReadoutRegion(readout); // Extent of readout
        cellExtent->x0 = PS_MIN(cellExtent->x0, roExtent->x0);
        cellExtent->x1 = PS_MAX(cellExtent->x1, roExtent->x1);
        cellExtent->y0 = PS_MIN(cellExtent->y0, roExtent->y0);
        cellExtent->y1 = PS_MAX(cellExtent->y1, roExtent->y1);
        psFree(roExtent);
    }

    bool mdok;                          // Status of MD lookup
    int cellX0 = psMetadataLookupS32(&mdok, cell->concepts, "CELL.X0"); // Cell x offset
    if (!mdok) {
        psError(PS_ERR_UNKNOWN, true, "Unable to find CELL.X0.\n");
        psFree(cellExtent);
        return NULL;
    }

    int cellY0 = psMetadataLookupS32(&mdok, cell->concepts, "CELL.Y0"); // Cell y offset
    if (!mdok) {
        psError(PS_ERR_UNKNOWN, true, "Unable to find CELL.Y0.\n");
        psFree(cellExtent);
        return NULL;
    }
    int xParity = psMetadataLookupS32(NULL, cell->concepts, "CELL.XPARITY");
    int yParity = psMetadataLookupS32(NULL, cell->concepts, "CELL.YPARITY");

    if (xParity >= 0) {
        cellExtent->x0 += cellX0;
        cellExtent->x1 += cellX0;
    } else {
        float x0 = cellX0 - cellExtent->x1;
        float x1 = cellX0 - cellExtent->x0;
        cellExtent->x0 = x0;
        cellExtent->x1 = x1;
    }

    if (yParity >= 0) {
        cellExtent->y0 += cellY0;
        cellExtent->y1 += cellY0;
    } else {
        float y0 = cellY0 - cellExtent->y1;
        float y1 = cellY0 - cellExtent->y0;
        cellExtent->y0 = y0;
        cellExtent->y1 = y1;
    }

    return cellExtent;
}

// return chip pixels included in all cells
psRegion *ppstampChipRegion(const pmChip *chip)
{
    PS_ASSERT_PTR_NON_NULL(chip, NULL);

    if (ppstampMegacamWorkaround) {
        // There is an inconsistency in the megacam parameters. 
        // The offset to the cells is effectively contained in two
        // places.
        // The value of CELL.X0 for the 2 cells is 0 and 1024 respectivly
        // while the two values for readout->image.col0 = 32 and 1056
        // This fact makes it impossible to calculate the Chip bounds 
        // in a way consistent with say gpc1
        // I'm deferring this problem for now.
        // Since all chips on megacam have the same bounds, I just hard code
        // the answer. See bug 986
        return psRegionAlloc(32, 2080, 0, 4612);
    }

    psArray *cells = chip->cells;       // Array of component cells
    psRegion *chipExtent = psRegionAlloc(INFINITY, 0, INFINITY, 0); // Extent of chip
    for (long i = 0; i < cells->n; i++) {
        pmCell *cell = cells->data[i];  // Cell of interest
        psRegion *cellExtent = ppstampCellRegion(cell); // Extent of cell
        chipExtent->x0 = PS_MIN(chipExtent->x0, cellExtent->x0);
        chipExtent->x1 = PS_MAX(chipExtent->x1, cellExtent->x1);
        chipExtent->y0 = PS_MIN(chipExtent->y0, cellExtent->y0);
        chipExtent->y1 = PS_MAX(chipExtent->y1, cellExtent->y1);
        psFree(cellExtent);
    }

    return chipExtent;
}
