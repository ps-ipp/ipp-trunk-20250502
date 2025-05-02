#include <stdio.h>
#include <pslib.h>
#include <string.h>

#include "pmHDU.h"
#include "pmHDUUtils.h"
#include "pmFPA.h"
#include "pmFPAview.h"
#include "pmFPAfile.h"
#include "pmFPAfileFitsIO.h"
#include "pmFPAHeader.h"
#include "pmConceptsRead.h"
#include "pmConceptsWrite.h"

#include "pmFPAExpNumIO.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////


static bool pmReadoutReadExpNum(pmReadout *ro, psFits *fits)
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

#ifdef notdef
    if (!psFitsMoveExtName(fits, hdu->extname)) {
        psError(PS_ERR_IO, false, "Unable to move to EXPNUM image.");
        return false;
    }
#endif
    psMetadata *header = psFitsReadHeader(NULL, fits); // Header
    if (!header) {
        psError(PS_ERR_UNEXPECTED_NULL, false, "Unable to read header for EXPNUM image.");
        return false;
    }

    bool data = false;                  // Did we find any data?

    int naxis = psMetadataLookupS32(NULL, header, "NAXIS"); // Number of axes
    if (naxis > 0) {
        psImage *expnum = psFitsReadImage(fits, psRegionSet(0, 0, 0, 0), 0);
        psMetadataAddImage(ro->analysis, PS_LIST_TAIL, "EXPNUM", PS_META_REPLACE,
                           "EXPNUM", expnum);
        psFree(expnum);
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

static bool pmCellReadExpNum(pmCell *cell, const pmFPAview *view,
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
        if (!pmReadoutReadExpNum(readout, file->fits)) {
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

static bool pmChipReadExpNum(pmChip *chip, const pmFPAview *view,
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
        pmCellReadExpNum(cell, thisView, file, config);
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

static bool pmFPAReadExpNum(pmFPA *fpa, const pmFPAview *view,
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
        pmChipReadExpNum(chip, thisView, file, config);
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

bool pmExpNumRead(const pmFPAview *view, pmFPAfile *file, pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(view, false);
    PS_ASSERT_PTR_NON_NULL(file, false);
    PS_ASSERT_PTR_NON_NULL(file->fpa, false);

    pmFPA *fpa = file->fpa;

    if (view->chip == -1) {
        pmFPAReadExpNum(fpa, view, file, config);
        return true;
    }

    if (view->chip >= fpa->chips->n) {
        return false;
    }
    pmChip *chip = fpa->chips->data[view->chip];

    if (view->cell == -1) {
        pmChipReadExpNum(chip, view, file, config);
        return true;
    }

    if (view->cell >= chip->cells->n) {
        return false;
    }
    pmCell *cell = chip->cells->data[view->cell];

    if (view->readout == -1) {
        pmCellReadExpNum(cell, view, file, config);
        return true;
    }

    if (view->readout >= cell->readouts->n) {
        return false;
    }
    pmReadout *readout = cell->readouts->data[view->readout];

    return pmReadoutReadExpNum(readout, file->fits);
}
#ifdef notyet
// Currently only ppStack writes an EXPNUM image and it uses file type MASK
bool pmReadoutWriteExpNum(pmReadout *ro, psFits *fits)
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
                         "ExpNum cell correction value", cellCorr);
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

static bool pmCellWriteExpNum(pmCell *cell, const pmFPAview *view,
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
    if (!pmReadoutWriteExpNum(readout, file->fits)) {
        psError(PS_ERR_IO, false, "Failed to write readout");
        return false;
    }
    return true;
}

static bool pmChipWriteExpNum(pmChip *chip, const pmFPAview *view,
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
        if (!pmCellWriteExpNum(cell, thisView, file, config)) {
            psError(PS_ERR_IO, false, "Failed to write %dth cell", i);
            psFree(thisView);
            return false;
        }
    }
    psFree(thisView);
    return true;
}

static bool pmFPAWriteExpNum(pmFPA *fpa, const pmFPAview *view,
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
        if (!pmChipWriteExpNum(chip, thisView, file, config)) {
            psError(PS_ERR_IO, false, "Failed to write %dth chip", i);
            psFree(thisView);
            return false;
        }
    }
    psFree(thisView);
    return true;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Currently only ppStack writes an EXPNUM image and it uses file type MASK
bool pmExpNumWrite(const pmFPAview *view, pmFPAfile *file, pmConfig *config)
{
    PS_ASSERT_PTR_NON_NULL(view, false);
    PS_ASSERT_PTR_NON_NULL(file, false);
    PS_ASSERT_PTR_NON_NULL(file->fpa, false);

    pmFPA *fpa = pmFPAfileSuitableFPA(file, view, config, false); // Suitable FPA for writing

    if (view->chip == -1) {
        if (!pmFPAWriteExpNum(fpa, view, file, config)) {
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
        if (!pmChipWriteExpNum(chip, view, file, config)) {
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
        if (!pmCellWriteExpNum(cell, view, file, config)) {
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

    if (!pmReadoutWriteExpNum(readout, file->fits)) {
        psError(PS_ERR_IO, false, "Failed to write pattern correction from readout %d", view->readout);
        psFree(fpa);
        return false;
    }

    psFree(fpa);
    return true;
}

bool pmExpNumWritePHU(const pmFPAview *view, pmFPAfile *file, pmConfig *config)
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
#endif // notyet

