#include <stdio.h>
#include <pslib.h>
#include <string.h>

#include "pmHDU.h"
#include "pmHDUUtils.h"
#include "pmFPA.h"
#include "pmFPAview.h"
#include "pmFPAfile.h"
#include "pmFPAUtils.h"
#include "pmFPAfileFitsIO.h"
#include "pmFPAHeader.h"
#include "pmConceptsRead.h"
#include "pmConceptsWrite.h"

#include "pmPattern.h"
#include "pmPatternIO.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool pmReadoutWritePattern(pmReadout *ro, psFits *fits)
{
    PM_ASSERT_READOUT_NON_NULL(ro, false);
    PS_ASSERT_FITS_NON_NULL(fits, false);

    bool gotRow, gotCell;
    psImage *rowCorr = psMetadataLookupPtr(&gotRow, ro->analysis, PM_PATTERN_ROW_CORRECTION); // Row correction
    float cellCorr = psMetadataLookupF32(&gotCell, ro->analysis, PM_PATTERN_CELL_CORRECTION); // Cell corr.

    pmCell *cell = ro->parent;          // Cell of interest
    if (!cell) {
        psError(PS_ERR_UNEXPECTED_NULL, true, "No cell associated with readout.");
        return false;
    }
    pmChip *chip = cell->parent;    // Chip of interest
    pmFPA *fpa = chip->parent;      // FPA of interest
    pmHDU *hdu = pmHDUGetLowest(fpa, chip, cell); // HDU for readout
    if (!hdu) {
        psError(PS_ERR_UNEXPECTED_NULL, true, "No HDU associated with readout.");
        return false;
    }

    psMetadata *header = psMetadataCopy(NULL, hdu->header); // Header for output

    if (gotCell) {
        psMetadataAddF32(header, PS_LIST_TAIL, PM_PATTERN_CELL_CORRECTION, PS_META_REPLACE,
                         "Pattern cell correction value", cellCorr);
    }

    if (gotRow) {
        if (!psFitsWriteImage(fits, header, rowCorr, 0, hdu->extname)) {
            psError(PS_ERR_IO, false, "Unable to write pattern row correction.");
            psFree(header);
            return false;
        }
    } else {
        if (!psFitsWriteBlank(fits, header, hdu->extname)) {
            psError(PS_ERR_IO, false, "Unable to write pattern cell correction.");
            psFree(header);
            return false;
        }
    }

    psFree(header);
    return true;
}

static bool pmCellWritePattern(pmCell *cell, const pmFPAview *view,
                               pmFPAfile *file, pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(cell, false);
    PS_ASSERT_PTR_NON_NULL(cell->readouts, false);
    PS_ASSERT_PTR_NON_NULL(view, false);

    if (!pmConceptsWriteCell(cell, true, config)) {
        psError(PS_ERR_IO, false, "Unable to write concepts for cell.");
        return false;
    }

    // Only do the FIRST readout --- don't want to write lots of headers
    pmReadout *readout = cell->readouts->data[0];
    if (!pmReadoutWritePattern(readout, file->fits)) {
        psError(PS_ERR_IO, false, "Failed to write readout");
        return false;
    }
    return true;
}

static bool pmChipWritePattern(pmChip *chip, const pmFPAview *view,
                               pmFPAfile *file, pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(chip, false);
    PS_ASSERT_PTR_NON_NULL(chip->cells, false);
    PS_ASSERT_PTR_NON_NULL(view, false);

    if (!pmConceptsWriteChip(chip, true, true, config)) {
        psError(PS_ERR_IO, false, "Unable to write concepts for chip.\n");
        return false;
    }

    pmFPAview *thisView = pmFPAviewAlloc(view->nRows); // Copy of input view
    *thisView = *view;

    for (int i = 0; i < chip->cells->n; i++) {
        pmCell *cell = chip->cells->data[i];
        thisView->cell = i;
        if (!pmCellWritePattern(cell, thisView, file, config)) {
            psError(PS_ERR_IO, false, "Failed to write %dth cell", i);
            psFree(thisView);
            return false;
        }
    }
    psFree(thisView);
    return true;
}

static bool pmFPAWritePattern(pmFPA *fpa, const pmFPAview *view,
                              pmFPAfile *file, pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(view, false);
    PS_ASSERT_PTR_NON_NULL(fpa, false);
    PS_ASSERT_PTR_NON_NULL(fpa->chips, false);

    if (!pmConceptsWriteFPA(fpa, true, config)) {
        psError(PS_ERR_IO, false, "Unable to write concepts for FPA.\n");
        return false;
    }

    pmFPAview *thisView = pmFPAviewAlloc(view->nRows); // Copy of input view
    *thisView = *view;

    for (int i = 0; i < fpa->chips->n; i++) {
        pmChip *chip = fpa->chips->data[i];
        thisView->chip = i;
        if (!pmChipWritePattern(chip, thisView, file, config)) {
            psError(PS_ERR_IO, false, "Failed to write %dth chip", i);
            psFree(thisView);
            return false;
        }
    }
    psFree(thisView);
    return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////


bool pmReadoutReadPattern(pmReadout *ro, psFits *fits)
{
    PM_ASSERT_READOUT_NON_NULL(ro, false);
    PS_ASSERT_FITS_NON_NULL(fits, false);

    pmCell *cell = ro->parent;          // Cell of interest
    if (!cell) {
        psError(PS_ERR_UNEXPECTED_NULL, true, "No cell associated with readout.");
        return false;
    }
    pmChip *chip = cell->parent;    // Chip of interest
    pmFPA *fpa = chip->parent;      // FPA of interest
    pmHDU *hdu = pmHDUGetLowest(fpa, chip, cell); // HDU for readout
    if (!hdu) {
        psError(PS_ERR_UNEXPECTED_NULL, true, "No HDU associated with readout.");
        return false;
    }

    if (!psFitsMoveExtName(fits, hdu->extname)) {
        psError(PS_ERR_IO, false, "Unable to move to pattern correction.");
        return false;
    }

    psMetadata *header = psFitsReadHeader(NULL, fits); // Header
    if (!header) {
        psError(PS_ERR_UNEXPECTED_NULL, false, "Unable to read header for pattern correction.");
        return false;
    }

    bool data = true;                  // Did we find any data?

    psMetadataItem *cellCorr = psMetadataLookup(header, PM_PATTERN_CELL_CORRECTION); // Cell pattern correction
    if (cellCorr) {
        psMetadataAddItem(ro->analysis, cellCorr, PS_LIST_TAIL, PS_META_REPLACE);
        data = true;
    }

    int naxis = psMetadataLookupS32(NULL, header, "NAXIS"); // Number of axes
    if (naxis > 0) {
        psImage *rowCorr = psFitsReadImage(fits, psRegionSet(0, 0, 0, 0), 0); // Row pattern correction
        psMetadataAddImage(ro->analysis, PS_LIST_TAIL, PM_PATTERN_ROW_CORRECTION, PS_META_REPLACE,
                           "Pattern row correction", rowCorr);
        psFree(rowCorr);
        data = true;
    }

    if (data) {
        ro->data_exists = true;
        ro->parent->data_exists = true;
        ro->parent->parent->data_exists = true;
    }

    psFree(header);

    return true;
}

static bool pmCellReadPattern(pmCell *cell, const pmFPAview *view,
                              pmFPAfile *file, pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(view, false);
    PS_ASSERT_PTR_NON_NULL(cell, false);
    PS_ASSERT_PTR_NON_NULL(cell->readouts, false);

    pmFPAview *thisView = pmFPAviewAlloc(view->nRows); // Copy of input view
    *thisView = *view;

    // Create a readout if none exists
    if (!cell->readouts || cell->readouts->n == 0) {
        pmReadout *readout = pmReadoutAlloc(cell); // New readout
        psFree(readout);                // Drop reference
    }

    cell->data_exists = false;
    for (int i = 0; i < cell->readouts->n; i++) {
        pmReadout *readout = cell->readouts->data[i];
        thisView->readout = i;
        if (!pmReadoutReadPattern(readout, file->fits)) {
            psError(PS_ERR_IO, false, "Unable to read pattern correction.");
            return false;
        }
    }
    psFree(thisView);

    if (!pmCellReadHeader(cell, file->fits, config)) {
        psError(PS_ERR_IO, false, "Unable to read header for cell.");
        return false;
    }
    // load in the concept information for this cell
    if (!pmConceptsReadCell(cell, PM_CONCEPT_SOURCE_HEADER, true, NULL)) {
        psErrorClear();
        psWarning("Difficulty reading concepts for cell; attempting to proceed.");
    }

    return true;
}

static bool pmChipReadPattern(pmChip *chip, const pmFPAview *view,
                              pmFPAfile *file, pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(view, false);
    PS_ASSERT_PTR_NON_NULL(chip, false);
    PS_ASSERT_PTR_NON_NULL(chip->cells, false);

    pmFPAview *thisView = pmFPAviewAlloc(view->nRows); // Copy of input view
    *thisView = *view;

    chip->data_exists = false;
    for (int i = 0; i < chip->cells->n; i++) {
        pmCell *cell = chip->cells->data[i];
        thisView->cell = i;
        pmCellReadPattern(cell, thisView, file, config);
        if (!cell->data_exists) {
            continue;
        }
        chip->data_exists = true;
    }
    psFree(thisView);

    if (!pmChipReadHeader(chip, file->fits, config)) {
        psError(PS_ERR_IO, false, "Unable to read header for cell.");
        return false;
    }
    if (!pmConceptsReadChip(chip, PM_CONCEPT_SOURCE_HEADER, true, true, NULL)) {
        psError(PS_ERR_IO, false, "Failed to read concepts for chip.\n");
        return false;
    }

    return true;
}

static bool pmFPAReadPattern(pmFPA *fpa, const pmFPAview *view,
                             pmFPAfile *file, pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(view, false);
    PS_ASSERT_PTR_NON_NULL(file, false);
    PS_ASSERT_PTR_NON_NULL(file->fpa, false);
    PS_ASSERT_PTR_NON_NULL(file->fpa->chips, false);

    pmFPAview *thisView = pmFPAviewAlloc(view->nRows); // Copy of input view
    *thisView = *view;

    for (int i = 0; i < fpa->chips->n; i++) {
        pmChip *chip = fpa->chips->data[i];
        thisView->chip = i;
        pmChipReadPattern(chip, thisView, file, config);
    }
    psFree(thisView);

    if (!pmFPAReadHeader(fpa, file->fits, config)) {
        psError(PS_ERR_IO, false, "Unable to read header for cell.");
        return false;
    }
    if (!pmConceptsReadFPA(fpa, PM_CONCEPT_SOURCE_HEADER, true, NULL)) {
        psError(PS_ERR_IO, false, "Failed to read concepts for fpa.\n");
        return false;
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool pmPatternWrite(const pmFPAview *view, pmFPAfile *file, pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(view, false);
    PS_ASSERT_PTR_NON_NULL(file, false);
    PS_ASSERT_PTR_NON_NULL(file->fpa, false);

    pmFPA *fpa = pmFPAfileSuitableFPA(file, view, config, false); // Suitable FPA for writing

    if (view->chip == -1) {
        if (!pmFPAWritePattern(fpa, view, file, config)) {
            psError(PS_ERR_IO, false, "Failed to write pattern correction from fpa");
            psFree(fpa);
            return false;
        }
        psFree(fpa);
        return true;
    }

    if (view->chip >= fpa->chips->n) {
        psError(PS_ERR_UNKNOWN, false, "Writing chip == %d (>= chips->n == %ld)", view->chip, fpa->chips->n);
        psFree(fpa);
        return false;
    }
    pmChip *chip = fpa->chips->data[view->chip];

    if (view->cell == -1) {
        if (!pmChipWritePattern(chip, view, file, config)) {
            psError(PS_ERR_IO, false, "Failed to write pattern correction from chip");
            psFree(fpa);
            return false;
        }
        psFree(fpa);
        return true;
    }

    if (view->cell >= chip->cells->n) {
        psError(PS_ERR_UNKNOWN, false, "Writing cell == %d (>= cells->n == %ld)",
                view->cell, chip->cells->n);
        psFree(fpa);
        return false;
    }
    pmCell *cell = chip->cells->data[view->cell];

    if (view->readout == -1) {
        if (!pmCellWritePattern(cell, view, file, config)) {
            psError(PS_ERR_IO, false, "Failed to write pattern correction from cell");
            psFree(fpa);
            return false;
        }
        psFree(fpa);
        return true;
    }

    if (view->readout >= cell->readouts->n) {
        psError(PS_ERR_UNKNOWN, false, "Writing readout == %d (>= readouts->n == %ld)",
                view->readout, cell->readouts->n);
        psFree(fpa);
        return false;
    }
    pmReadout *readout = cell->readouts->data[view->readout];

    if (!pmReadoutWritePattern(readout, file->fits)) {
        psError(PS_ERR_IO, false, "Failed to write pattern correction from readout %d", view->readout);
        psFree(fpa);
        return false;
    }

    psFree(fpa);
    return true;
}

bool pmPatternWritePHU(const pmFPAview *view, pmFPAfile *file, pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(view, false);
    PS_ASSERT_PTR_NON_NULL(file, false);

    if (file->wrote_phu) {
        return true;
    }

    // find the FPA phu
    pmFPA *fpa = pmFPAfileSuitableFPA(file, view, config, false); // Suitable FPA for writing
    pmHDU *phu = psMemIncrRefCounter(pmFPAviewThisPHU(view, fpa));
    psFree(fpa);

    // if there is no PHU, this is a single header+image (extension-less) file. This could be the case for an
    // input SPLIT set of files being written out as a MEF.  if there is a PHU, write it out as a 'blank'
    psMetadata *outhead = psMetadataAlloc();
    if (phu) {
        psMetadataCopy (outhead, phu->header);
    }
    psFree(phu);

    pmConfigConformHeader(outhead, file->format);

    psFitsWriteBlank(file->fits, outhead, "");
    file->wrote_phu = true;

    psTrace("pmFPAfile", 5, "wrote phu %s (type: %d)\n", file->filename, file->type);
    psFree(outhead);

    return true;
}

bool pmPatternRead(const pmFPAview *view, pmFPAfile *file, pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(view, false);
    PS_ASSERT_PTR_NON_NULL(file, false);
    PS_ASSERT_PTR_NON_NULL(file->fpa, false);

    pmFPA *fpa = file->fpa;

    if (view->chip == -1) {
        pmFPAReadPattern(fpa, view, file, config);
        return true;
    }

    if (view->chip >= fpa->chips->n) {
        return false;
    }
    pmChip *chip = fpa->chips->data[view->chip];

    if (view->cell == -1) {
        pmChipReadPattern(chip, view, file, config);
        return true;
    }

    if (view->cell >= chip->cells->n) {
        return false;
    }
    pmCell *cell = chip->cells->data[view->cell];

    if (view->readout == -1) {
        pmCellReadPattern(cell, view, file, config);
        return true;
    }

    if (view->readout >= cell->readouts->n) {
        return false;
    }
    pmReadout *readout = cell->readouts->data[view->readout];

    return pmReadoutReadPattern(readout, file->fits);
}

/**************** PatternRowAmp(litude) I/O *************************/

bool pmPatternRowAmpRead (const pmFPAview *view, pmFPAfile *file, const pmConfig *config)
    {
        // read the full model in one pass: require the level to be FPA
        if (view->chip != -1) {
            psError(PS_ERR_IO, false, "Pattern Row Amplitude must be read at the FPA level");
            return false;
        }

        if (!pmPatternRowAmpReadFPA (file)) {
            psError(PS_ERR_IO, false, "Failed to read Pattern Row Amplitude for fpa");
            return false;
        }
        return true;
    }

// read in all chip-level Pattern Row Amplitude data for this FPA
bool pmPatternRowAmpReadFPA (pmFPAfile *file) {

    if (!pmPatternRowAmpReadChips (file)) {
        psError(PS_ERR_IO, false, "Failed to read Pattern Row Amplitude for chips");
        return false;
    }

    return true;
}

// Read the set of tables, one for each chip.  The values are saved on the cell->analysis
// metadata of the pmFPAfile associated with the pattern file.  Later, when this is used (e.g.,
// ppImageDetrendPatternRowApply), the values are transferred to the cell->analysis metadata of
// the pmFPAfile for the image being processed.
bool pmPatternRowAmpReadChips (pmFPAfile *file) {

    bool haveData, status;

    // loop over the extensions
    // for each extension, use the extname (eg, XY01.ptn) to assign to a chip

    // move to the start of the file
    haveData = psFitsMoveExtNum (file->fits, 1, false);
    if (!haveData) {
        psError(PS_ERR_IO, false, "Failed to read even the first extension?");
        return false;
    }

    int nGood = 0;
    int nTotal = 0;
    while (haveData) {

	// load the header
	psMetadata *header = psFitsReadHeader(NULL, file->fits); // The FITS header
	if (!header) psAbort("cannot read model header");

	// load the full model in one shot
	psArray *model = psFitsReadTable (file->fits);
	if (!model) psAbort("cannot read model");
	
	// determine the chip:
	char *extname = psMetadataLookupStr (&status, header, "EXTNAME");
	psLogMsg ("psModules.detrend", 8, "read %ld rows from Pattern Row Amplitude file, extname %s\n", model->n, extname);
	nTotal += model->n;

	// I expect to find a name of the form: chipName.ptn (eg, XY01.ptn)
	// where chipName like 'XY01'
	psAssert (strlen(extname) == 8, "invalid extension %s", extname);
	psAssert (extname[5] == 'p', "invalid extension %s", extname);
	psAssert (extname[6] == 't', "invalid extension %s", extname);
	psAssert (extname[7] == 'n', "invalid extension %s", extname);

	char chipName[5];
	strncpy (chipName, extname, 4);
	chipName[4] = 0;

	pmChip *chip = pmConceptsChipFromName (file->fpa, chipName);
	if (!chip) psAbort ("invalid chip?");

	// parse the model entries
	for (int i = 0; i < model->n; i++) {
	    psMetadata *row = model->data[i];
	    psAssert (row, "missing model row");

	    char *cellName = psMetadataLookupStr(&status, row, "CELL_NAME");
	    if (!cellName) continue;

	    float amplitude = psMetadataLookupF32(&status, row, "VALUE_MEDIAN");

	    int cellNumber = pmChipFindCell (chip, cellName);
	    psAssert ((cellNumber >=0) && (cellNumber < chip->cells->n), "invalid cell number");

	    pmCell *cell = chip->cells->data[cellNumber];
	    if (!cell) continue;

	    psAssert (cell->analysis, "oops");

	    psMetadataAddF32 (cell->analysis, PS_LIST_TAIL, "PTN.ROW.AMP", PS_META_REPLACE, "", amplitude);
	    nGood ++;
	}

	psFree (model);
	psFree (header);

	// move to the next extension
	haveData = psFitsMoveExtNum (file->fits, 1, true);
    }
    psLogMsg ("psModules.detrend", 4, "read %d of %d rows from Pattern Row Amplitude file\n", nGood, nTotal);

    return true;
}

/**************** PatternDeadCells I/O *************************/

bool pmPatternDeadCellsRead (const pmFPAview *view, pmFPAfile *file, const pmConfig *config)
    {
        // read the full model in one pass: require the level to be FPA
        if (view->chip != -1) {
            psError(PS_ERR_IO, false, "Pattern Dead Cells must be read at the FPA level");
            return false;
        }

        if (!pmPatternDeadCellsReadFPA (file)) {
            psError(PS_ERR_IO, false, "Failed to read Pattern Dead Cells for fpa");
            return false;
        }
        return true;
    }

// read in all chip-level Pattern Dead Cells data for this FPA
bool pmPatternDeadCellsReadFPA (pmFPAfile *file) {

    if (!pmPatternDeadCellsReadChips (file)) {
        psError(PS_ERR_IO, false, "Failed to read Pattern Dead Cells for chips");
        return false;
    }

    return true;
}

// Read the set of dead cell image cubes, one for each chip.  The values are saved on the
// chip->analysis metadata of the pmFPAfile associated with the pattern file.  Later, when this
// is used (e.g., ppImageDetrendPatternDeadCellsApply), the values are transferred to the
// chip->analysis metadata of the pmFPAfile for the image being processed.
bool pmPatternDeadCellsReadChips (pmFPAfile *file) {

    bool haveData, status;

    // loop over the extensions
    // for each extension, use the extname (eg, XY01.ded) to assign to a chip

    // move to the start of the file
    haveData = psFitsMoveExtNum (file->fits, 1, false);
    if (!haveData) {
        psError(PS_ERR_IO, false, "Failed to read even the first extension?");
        return false;
    }

    int nGood = 0;
    while (haveData) {

	// load the header
	psMetadata *header = psFitsReadHeader(NULL, file->fits); // The FITS header
	if (!header) psAbort("cannot read dead cell header");

	// load the full model in one shot
	psImage *deadCellData = psFitsReadImage(file->fits, psRegionSet(0,0,0,0), 0); // dead cell patterns
	if (!deadCellData) psAbort("cannot read dead cell pattern");
	
	// determine the chip (not all chips have DEAD CELL patterns)
	char *extname = psMetadataLookupStr (&status, header, "EXTNAME");
	psLogMsg ("psModules.detrend", 8, "read dead cell pattern for extname %s\n", extname);

	// I expect to find a name of the form: chipName.ded (eg, XY01.ded)
	// where chipName like 'XY01'
	psAssert (strlen(extname) == 8, "invalid extension %s", extname);
	psAssert (extname[5] == 'd', "invalid extension %s", extname);
	psAssert (extname[6] == 'e', "invalid extension %s", extname);
	psAssert (extname[7] == 'd', "invalid extension %s", extname);

	char chipName[5];
	strncpy (chipName, extname, 4);
	chipName[4] = 0;

	pmChip *chip = pmConceptsChipFromName (file->fpa, chipName);
	if (!chip) psAbort ("invalid chip?");

	psMetadataAddImage (chip->analysis, PS_LIST_TAIL, "PTN.DEAD.CELL", PS_META_REPLACE, "", deadCellData);
	psFree (deadCellData);
	psFree (header);

	// move to the next extension
	haveData = psFitsMoveExtNum (file->fits, 1, true);
	nGood ++;
    }
    psLogMsg ("psModules.detrend", 4, "read patterns for %d chips from Pattern Dead Cells file\n", nGood);

    return true;
}
