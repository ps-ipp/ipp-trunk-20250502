#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <pslib.h>

#include "pmHDU.h"
#include "pmFPA.h"
#include "pmHDUUtils.h"

pmHDU *pmHDUGetFirst (const pmFPA *fpa) {

    // XXX we probably should have an indicator in pmFPA about the depths.

    if (!fpa) return NULL;
    if (fpa->hdu) return fpa->hdu;

    for (int i = 0; i < fpa->chips->n; i++) {
        pmChip *chip = fpa->chips->data[i];
        if (!chip) continue;
        if (chip->hdu) return chip->hdu;
        if (!chip->cells) continue;
        for (int j = 0; j < chip->cells->n; j++) {
            pmCell *cell = chip->cells->data[j];
            if (!cell) continue;
            if (cell->hdu) return cell->hdu;
        }
    }
    return NULL;
}

pmHDU *pmHDUFromFPA(const pmFPA *fpa)
{
    PS_ASSERT_PTR_NON_NULL(fpa, NULL);
    return fpa->hdu;
}

pmHDU *pmHDUFromChip(const pmChip *chip)
{
    PS_ASSERT_PTR_NON_NULL(chip, NULL);

    pmHDU *hdu = chip->hdu;             // The HDU information
    if (!hdu) {
        hdu = pmHDUFromFPA(chip->parent); // Grab HDU info from the FPA
    }

    return hdu;
}

pmHDU *pmHDUFromCell(const pmCell *cell)
{
    PS_ASSERT_PTR_NON_NULL(cell, NULL);

    pmHDU *hdu = cell->hdu;             // The HDU information
    if (!hdu) {
        hdu = pmHDUFromChip(cell->parent); // Grab HDU info from the chip
    }

    return hdu;
}

pmHDU *pmHDUFromReadout(const pmReadout *readout)
{
    PS_ASSERT_PTR_NON_NULL(readout, NULL);

    pmCell *cell = readout->parent; // cell containing this readout;
    pmHDU *hdu = pmHDUFromCell(cell);
    return hdu;
}

// Get the lowest HDU
pmHDU *pmHDUGetLowest(const pmFPA *fpa, const pmChip *chip, const pmCell *cell)
{
    pmHDU *hdu = NULL;          // The HDU that's at the lowest level
    if (cell) {
        hdu = pmHDUFromCell(cell);
    } else if (chip) {
        hdu = pmHDUFromChip(chip);
    } else if (fpa) {
        hdu = pmHDUFromFPA(fpa);
    }

    return hdu;
}

// Get the highest HDU
pmHDU *pmHDUGetHighest(const pmFPA *fpa, const pmChip *chip, const pmCell *cell)
{
    pmHDU *hdu = NULL;          // The HDU that's at the highest level
    if (fpa) {
        hdu = pmHDUFromFPA(fpa);
    }
    if (!hdu && chip) {
        hdu = pmHDUFromChip(chip);
    }
    if (!hdu && cell) {
        hdu = pmHDUFromCell(cell);
    }

    return hdu;
}

// Print spaces to indent
#define INDENT(FILE, LEVEL) \
{ \
    for (int i = 0; i < (LEVEL); i++) { \
        fprintf(FILE, " "); \
    } \
}

void pmHDUPrint(FILE *fd, const pmHDU *hdu, int level, bool header)
{
    PS_ASSERT_PTR_NON_NULL(hdu,);

    INDENT(fd, level);
    if (hdu->blankPHU) {
        fprintf(fd, "HDU: (PHU)\n");
    } else {
        fprintf(fd, "HDU: %s\n", hdu->extname);
    }

    INDENT(fd, level + 1);
    fprintf(fd, "Format: %p\n", hdu->format);
    if (header) {
        INDENT(fd, level + 1);
        if (hdu->header) {
            fprintf(fd, "Header:\n");
            psMetadataPrint(fd, hdu->header, level + 2);
        } else {
            fprintf(fd, "No header.\n");
        }
    }

    INDENT(fd, level + 1);
    if (hdu->images) {
        fprintf(fd, "Images:\n");
        for (long i = 0; i < hdu->images->n; i++) {
            psImage *image = hdu->images->data[i]; // Image of interest
            INDENT(fd, level + 2);
            fprintf(fd, "%ld: %dx%d\n", i, image->numCols, image->numRows);
        }
    } else {
        fprintf(fd, "NO images.\n");
    }

    INDENT(fd, level + 1);
    if (hdu->masks) {
        fprintf(fd, "Masks:\n");
        for (long i = 0; i < hdu->masks->n; i++) {
            psImage *mask = hdu->masks->data[i]; // Mask of interest
            INDENT(fd, level + 2);
            fprintf(fd, "%ld: %dx%d\n", i, mask->numCols, mask->numRows);
        }
    } else {
        fprintf(fd, "NO masks.\n");
    }

    INDENT(fd, level + 1);
    if (hdu->variances) {
        fprintf(fd, "Variances:\n");
        for (long i = 0; i < hdu->variances->n; i++) {
            psImage *variance = hdu->variances->data[i]; // Variance image of interest
            INDENT(fd, level + 2);
            fprintf(fd, "%ld: %dx%d\n", i, variance->numCols, variance->numRows);
        }
    } else {
        fprintf(fd, "NO variances.\n");
    }

    return;
}
