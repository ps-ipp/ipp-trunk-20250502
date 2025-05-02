#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <pslib.h>

#include "pmConfig.h"
//#include "pmConfigMask.h"
//#include "pmDetrendDB.h"

#include "pmHDU.h"
#include "pmFPA.h"
#include "pmFPALevel.h"
//#include "pmFPARead.h"
//#include "pmFPAWrite.h"
//#include "pmFPAMaskWeight.h"
#include "pmFPAview.h"
#include "pmFPAfile.h"
#include "pmFPAfileFringeIO.h"
#include "pmFPAfileFitsIO.h"
//#include "pmFPACopy.h"
//#include "pmFPAConstruct.h"
//#include "pmConceptsWrite.h"
#include "pmFringeStats.h"

# define FRINGE_TABLE "FRINGE.MEASUREMENTS"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Reading Fringe data
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

// given an already-opened fits file, read the table corresponding to the specified view
bool pmFPAviewReadFringes(const pmFPAview *view, pmFPAfile *file)
{
    PS_ASSERT_PTR_NON_NULL(view, false);
    PS_ASSERT_PTR_NON_NULL(file, false);

    pmFPA *fpa = file->fpa;             // FPA of interest
    psFits *fits = file->fits;          // FITS file

    if (view->chip == -1) {
        return pmFPAReadFringes(fpa, fits) > 0;
    }

    if (view->cell == -1) {
        pmChip *chip = pmFPAviewThisChip(view, fpa); // Chip of interest
        return pmChipReadFringes(chip, fits) > 0;
    }

    pmCell *cell = pmFPAviewThisCell(view, fpa); // Cell of interest
    return pmCellReadFringes(cell, fits) > 0;
}

int pmFPAReadFringes(pmFPA *fpa, psFits *fits)
{
    PS_ASSERT_PTR_NON_NULL(fpa, 0);
    PS_ASSERT_FITS_NON_NULL(fits, 0);

    int numRead = 0;                    // Number of reads
    psArray *chips = fpa->chips;        // Array of chips
    for (int i = 0; i < chips->n; i++) {
        pmChip *chip = chips->data[i];  // Chip of interest
        numRead += pmChipReadFringes(chip, fits);
    }

    return numRead;
}

int pmChipReadFringes(pmChip *chip, psFits *fits)
{
    PS_ASSERT_PTR_NON_NULL(chip, 0);
    PS_ASSERT_FITS_NON_NULL(fits, 0);

    int numRead = 0;                    // Number of reads
    psArray *cells = chip->cells;       // Array of cells
    for (int i = 0; i < cells->n; i++) {
        pmCell *cell = cells->data[i];  // Cell of interest
        numRead += pmCellReadFringes(cell, fits);
    }

    return numRead;
}

int pmCellReadFringes(pmCell *cell, psFits *fits)
{
    PS_ASSERT_PTR_NON_NULL(cell, 0);
    PS_ASSERT_FITS_NON_NULL(fits, 0);

    const char *chipName = psMetadataLookupStr(NULL, cell->parent->concepts, "CHIP.NAME"); // Name of chip
    const char *cellName = psMetadataLookupStr(NULL, cell->concepts, "CELL.NAME"); // Name of cell

    // Fringe Table Extension name
    psString extname = NULL;
    psStringAppend(&extname, "FRINGE_%s_%s", chipName, cellName);

    if (!psFitsMoveExtName(fits, extname)) {
        psError(PS_ERR_IO, false, "Unable to move to extension %s\n", extname);
        psFree(extname);
        return 0;
    }

    psMetadata *header = psFitsReadHeader(NULL, fits); // The FITS header
    if (!header) {
        psError(PS_ERR_IO, false, "Unable to read header for extension %s\n", extname);
        psFree(extname);
        psFree(header);
        return 0;
    }

    psArray *table = psFitsReadTable(fits); // The table
    if (!table) {
	psError(PS_ERR_UNKNOWN, false, "Unable to add read the fringe table data from the file");
	psFree (header);
	psFree (extname);
	return 0;
    }

    psArray *fringes = pmFringesParseTable(table, header);
    psFree(table);
    psFree(header);

    if (!fringes) {
	psError(PS_ERR_UNKNOWN, false, "Unable to add parse the fringe table data");
        psFree(extname);
	return 0;
    }

    if (!psMetadataAdd(cell->analysis, PS_LIST_TAIL, FRINGE_TABLE, PS_DATA_ARRAY, "Fringes", fringes)) {
        psError(PS_ERR_UNKNOWN, false, "Unable to add fringe from extension %s to analysis metadata "
                "for chip %s, cell %s\n", extname, chipName, cellName);
        psFree(fringes);
        psFree(extname);
        return 0;
    }

    psFree(fringes); // drop local reference
    psFree(extname);

    return 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Writing fringe data
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

// given an already-opened fits file, write the table corresponding to the specified view
bool pmFPAviewWriteFringes(const pmFPAview *view, pmFPAfile *file, pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(view, false);
    PS_ASSERT_PTR_NON_NULL(file, false);
    PS_ASSERT_PTR_NON_NULL(config, false);

    pmFPA *fpa = pmFPAfileSuitableFPA(file, view, config, false); // FPA of interest
    psFits *fits = file->fits;          // FITS file

    if (view->chip == -1) {
        return pmFPAWriteFringes(fits, fpa) > 0;
    }

    if (view->cell == -1) {
        pmChip *chip = pmFPAviewThisChip(view, fpa); // Chip of interest
        return pmChipWriteFringes(fits, chip) > 0;
    }

    pmCell *cell = pmFPAviewThisCell(view, fpa); // Cell of interest
    return pmCellWriteFringes(fits, cell) > 0;
}

int pmFPAWriteFringes(psFits *fits, const pmFPA *fpa)
{
    PS_ASSERT_PTR_NON_NULL(fpa, 0);
    PS_ASSERT_PTR_NON_NULL(fits, 0);

    int numWrite = 0;                    // Number of reads
    psArray *chips = fpa->chips;        // Array of chips
    for (int i = 0; i < chips->n; i++) {
        pmChip *chip = chips->data[i];  // Chip of interest
        numWrite += pmChipWriteFringes(fits, chip);
    }

    return numWrite;
}

int pmChipWriteFringes(psFits *fits, const pmChip *chip)
{
    PS_ASSERT_PTR_NON_NULL(chip, 0);
    PS_ASSERT_PTR_NON_NULL(fits, 0);

    int numWrite = 0;                    // Number of reads
    psArray *cells = chip->cells;       // Array of cells
    for (int i = 0; i < cells->n; i++) {
        pmCell *cell = cells->data[i];  // Cell of interest
        numWrite += pmCellWriteFringes(fits, cell);
    }

    return numWrite;
}

int pmCellWriteFringes(psFits *fits, const pmCell *cell)
{
    PS_ASSERT_PTR_NON_NULL(cell, 0);
    PS_ASSERT_PTR_NON_NULL(fits, 0);

    const char *chipName = psMetadataLookupStr(NULL, cell->parent->concepts, "CHIP.NAME"); // Name of chip
    const char *cellName = psMetadataLookupStr(NULL, cell->concepts, "CELL.NAME"); // Name of cell

    psArray *fringes = psMetadataLookupPtr(NULL, cell->analysis, FRINGE_TABLE); // The FITS table
    if (!fringes) {
        // We wrote everything we could find
        return 0;
    }

    psMetadata *header = psMetadataAlloc();
    psArray *table = pmFringesFormatTable(header, fringes);
    if (!table) {
        // We wrote everything we could find
	psFree(header);
        return 0;
    }

    psString extname = NULL;            // Extension name
    psStringAppend(&extname, "FRINGE_%s_%s", chipName, cellName);

    if (!psFitsWriteTable(fits, header, table, extname)) {
        psError(PS_ERR_UNKNOWN, false, "Unable to write table from chip %s, cell %s to extension %s\n",
                chipName, cellName, extname);
        psFree(extname);
	psFree(header);
        return 0;
    }

    psFree(extname);
    psFree(header);
    return 1;
}


