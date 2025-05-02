#include <stdio.h>
#include <pslib.h>

#include "pmHDU.h"
#include "pmFPA.h"
#include "pmHDUUtils.h"

// return cell pixels bounding the readout
psRegion *pmReadoutExtent(const pmReadout *readout)
{
    PS_ASSERT_PTR_NON_NULL(readout, NULL);

    psImage *image = readout->image;    // Image from which to get dimensions
    if (!image) {
        image = readout->mask;
    }
    if (!image) {
        image = readout->variance;
    }

    int xSize = 0;
    int ySize = 0;

    if (!image) {
        pmHDU *hdu = pmHDUFromReadout (readout);
        if (hdu && hdu->header) {
            bool status;
            xSize = psMetadataLookupS32(&status, hdu->header, "NAXIS1");
            if (!status) {
                xSize = psMetadataLookupS32(&status, hdu->header, "IMNAXIS1");
                if (!status) return NULL;
            }
            ySize = psMetadataLookupS32(&status, hdu->header, "NAXIS2");
            if (!status) {
                ySize = psMetadataLookupS32(&status, hdu->header, "IMNAXIS2");
                if (!status) return NULL;
            }
        } else {
        // Don't have anything to base the true extent on, so have to give the hardwired value (largest possible extent)
            xSize = psMetadataLookupS32(NULL, readout->parent->concepts, "CELL.XSIZE");
            ySize = psMetadataLookupS32(NULL, readout->parent->concepts, "CELL.YSIZE");
        }
        return psRegionAlloc(0, xSize, 0, ySize);
    }

    // Get the offset to the CCD window
    int xWindow = psMetadataLookupS32(NULL, readout->parent->concepts, "CELL.XWINDOW");
    int yWindow = psMetadataLookupS32(NULL, readout->parent->concepts, "CELL.YWINDOW");
    return psRegionAlloc(xWindow, xWindow + image->numCols,
                         yWindow, yWindow + image->numRows);
}

// return chip pixels bounding the cell (all readouts)
psRegion *pmCellExtent(const pmCell *cell)
{
    PS_ASSERT_PTR_NON_NULL(cell, NULL);

    psArray *readouts = cell->readouts; // Array of component readouts
    psRegion *cellExtent = psRegionAlloc(INFINITY, 0, INFINITY, 0); // Extent of cell
    for (long i = 0; i < readouts->n; i++) {
        pmReadout *readout = readouts->data[i]; // Readout of interest
        psRegion *roExtent = pmReadoutExtent(readout); // Extent of readout
        cellExtent->x0 = PS_MIN(cellExtent->x0, roExtent->x0);
        cellExtent->x1 = PS_MAX(cellExtent->x1, roExtent->x1);
        cellExtent->y0 = PS_MIN(cellExtent->y0, roExtent->y0);
        cellExtent->y1 = PS_MAX(cellExtent->y1, roExtent->y1);
        psFree(roExtent);
    }

    // Don't have anything to base the true extent on, so have to give the hardwired value (largest possible extent)
    if (readouts->n == 0) {
        int xSize = psMetadataLookupS32(NULL, cell->concepts, "CELL.XSIZE");
        int ySize = psMetadataLookupS32(NULL, cell->concepts, "CELL.YSIZE");
        cellExtent->x0 = 0;
        cellExtent->x1 = xSize;
        cellExtent->y0 = 0;
        cellExtent->y1 = ySize;
    }

    bool mdok;                          // Status of MD lookup
    int x0 = psMetadataLookupS32(&mdok, cell->concepts, "CELL.X0"); // Cell x offset
    if (!mdok) {
        psError(PS_ERR_UNKNOWN, true, "Unable to find CELL.X0.\n");
        psFree(cellExtent);
        return NULL;
    }
    int y0 = psMetadataLookupS32(&mdok, cell->concepts, "CELL.Y0"); // Cell y offset
    if (!mdok) {
        psError(PS_ERR_UNKNOWN, true, "Unable to find CELL.Y0.\n");
        psFree(cellExtent);
        return NULL;
    }

    int xParityCell = psMetadataLookupS32(NULL, cell->concepts, "CELL.XPARITY");
    int yParityCell = psMetadataLookupS32(NULL, cell->concepts, "CELL.YPARITY");

    // CELL.X0,Y0 are the coordinate of the amp on the chip, subtract size if parity flipped
    if (xParityCell > 0) {
        cellExtent->x0 += x0;
        cellExtent->x1 += x0;
    } else {
        float x0Cell = x0 - cellExtent->x1;
        float x1Cell = x0 - cellExtent->x0;
        cellExtent->x0 = x0Cell;
        cellExtent->x1 = x1Cell;
    }
    if (yParityCell > 0) {
        cellExtent->y0 += y0;
        cellExtent->y1 += y0;
    } else {
        float y0Cell = y0 - cellExtent->y1;
        float y1Cell = y0 - cellExtent->y0;
        cellExtent->y0 = y0Cell;
        cellExtent->y1 = y1Cell;
    }

    return cellExtent;
}

// return chip pixels included in all cells
psRegion *pmChipPixels(const pmChip *chip)
{
    PS_ASSERT_PTR_NON_NULL(chip, NULL);

    psArray *cells = chip->cells;       // Array of component cells
    psRegion *chipExtent = psRegionAlloc(INFINITY, 0, INFINITY, 0); // Extent of chip
    for (long i = 0; i < cells->n; i++) {
        pmCell *cell = cells->data[i];  // Cell of interest
        psRegion *cellExtent = pmCellExtent(cell); // Extent of cell
        chipExtent->x0 = PS_MIN(chipExtent->x0, cellExtent->x0);
        chipExtent->x1 = PS_MAX(chipExtent->x1, cellExtent->x1);
        chipExtent->y0 = PS_MIN(chipExtent->y0, cellExtent->y0);
        chipExtent->y1 = PS_MAX(chipExtent->y1, cellExtent->y1);
        psFree(cellExtent);
    }

    return chipExtent;
}

// return pixels in basic FPA grid bounded by chip
// this FPA grid has 0,0 at the 0,0 corner of one chip, and is NOT the same
// as the astrometry focal plane coordinate system
psRegion *pmChipExtent(const pmChip *chip)
{
    PS_ASSERT_PTR_NON_NULL(chip, NULL);

    psRegion *chipExtent = pmChipPixels(chip);
    if (!chipExtent) return NULL;

    bool mdok;                          // Status of MD lookup
    int x0 = psMetadataLookupS32(&mdok, chip->concepts, "CHIP.X0"); // Chip x offset
    if (!mdok) {
        psError(PS_ERR_UNKNOWN, true, "Unable to find CHIP.X0.\n");
        psFree(chipExtent);
        return NULL;
    }
    int y0 = psMetadataLookupS32(&mdok, chip->concepts, "CHIP.Y0"); // Chip y offset
    if (!mdok) {
        psError(PS_ERR_UNKNOWN, true, "Unable to find CHIP.Y0.\n");
        psFree(chipExtent);
        return NULL;
    }

    chipExtent->x0 += x0;
    chipExtent->x1 += x0;
    chipExtent->y0 += y0;
    chipExtent->y1 += y0;

    return chipExtent;
}

// return FPA pixels included in all chips
// this FPA grid has 0,0 at the 0,0 corner of one chip, and is NOT the same
// as the astrometry focal plane coordinate system
psRegion *pmFPAPixels(const pmFPA *fpa)
{
    PS_ASSERT_PTR_NON_NULL(fpa, NULL);

    psArray *chips = fpa->chips;       // Array of component chips
    psRegion *fpaExtent = psRegionAlloc(INFINITY, 0, INFINITY, 0); // Extent of fpa
    for (long i = 0; i < chips->n; i++) {
        pmChip *chip = chips->data[i];  // Chip of interest
        psRegion *chipExtent = pmChipExtent(chip); // Extent of chip
        fpaExtent->x0 = PS_MIN(fpaExtent->x0, chipExtent->x0);
        fpaExtent->x1 = PS_MAX(fpaExtent->x1, chipExtent->x1);
        fpaExtent->y0 = PS_MIN(fpaExtent->y0, chipExtent->y0);
        fpaExtent->y1 = PS_MAX(fpaExtent->y1, chipExtent->y1);
        psFree(chipExtent);
    }

    return fpaExtent;
}
