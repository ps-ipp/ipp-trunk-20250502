#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <strings.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <pslib.h>

#include "pmHDU.h"
#include "pmFPA.h"
#include "pmFPALevel.h"
#include "pmConcepts.h"
#include "pmMaskBadPixels.h"

static void readoutFree(pmReadout *readout)
{
    // if this readout has a parent, drop that instance
    if (readout->parent) {
        psTrace("psModules.camera", 9, "Removing readout %zd from cell %zd...\n",
                (size_t)readout, (size_t)readout->parent);
        psArray *readouts = readout->parent->readouts;
        for (int i = 0; i < readouts->n; i++) {
            if (readouts->data[i] == readout) {
                readouts->data[i] = NULL;
            }
        }
    }
    psTrace("psModules.camera", 9, "Freeing readout %zd\n", (size_t) readout);

    psFree(readout->image);
    psFree(readout->mask);
    psFree(readout->variance);
    psFree(readout->covariance);
    psFree(readout->analysis);
    psFree(readout->bias);
}

static void cellFree(pmCell *cell)
{

    // if this cell has a parent, drop that instance
    if (cell->parent) {
        psTrace("psModules.camera", 9, "Removing cell %zd from chip %zd...\n",
                (size_t)cell, (size_t)cell->parent);
        psArray *cells = cell->parent->cells;
        for (int i = 0; i < cells->n; i++) {
            if (cells->data[i] == cell) {
                cells->data[i] = NULL;
            }
        }
    }
    psTrace("psModules.camera", 9, "Freeing cell %zd\n", (size_t)cell);

    pmCellFreeReadouts(cell);
    psFree(cell->readouts);
    psFree(cell->concepts);
    psFree(cell->analysis);
    psFree(cell->config);
    psFree(cell->hdu);
}

static void chipFree(pmChip* chip)
{
    // if this chip has a parent, drop that instance
    if (chip->parent) {
        psTrace("psModules.camera", 9, "Removing chip %zd from fpa %zd...\n",
                (size_t)chip, (size_t)chip->parent);
        psArray *chips = chip->parent->chips;
        for (int i = 0; i < chips->n; i++) {
            if (chips->data[i] == chip) {
                chips->data[i] = NULL;
            }
        }
    }

    psTrace("psModules.camera", 9, "Freeing chip %zd\n", (size_t)chip);
    pmChipFreeCells(chip);
    psFree(chip->cells);

    psFree(chip->concepts);
    psFree(chip->analysis);
    psFree(chip->hdu);

    psFree(chip->toFPA);
    psFree(chip->fromFPA);
}


static void FPAFree(pmFPA *fpa)
{
    psTrace("psModules.camera", 9, "Freeing fpa %zd\n", (size_t)fpa);

    // NULL the parent pointers
    psArray *chips = fpa->chips;
    for (int i = 0 ; i < chips->n ; i++) {
        pmChip *tmpChip = chips->data[i];
        if (! tmpChip) {
            continue;
        }
        tmpChip->parent = NULL;
    }
    psFree(fpa->chips);
    psFree(fpa->concepts);
    psFree(fpa->analysis);
    psFree(fpa->camera);
    psFree(fpa->hdu);

    psFree(fpa->fromTPA);
    psFree(fpa->toTPA);
    psFree(fpa->toSky);
}

void pmCellFreeReadouts(pmCell *cell)
{
    PS_ASSERT_PTR_NON_NULL(cell,);

    //
    // Set the parent to NULL in all cell->readouts before psFree(cell->readouts)
    // in order to avoid memory reference counter problems.
    //
    psArray *readouts = cell->readouts;
    for (psS32 i = 0 ; i < readouts->n ; i++) {
        pmReadout *tmpReadout = readouts->data[i];
        if (! tmpReadout) {
            continue;
        }
        tmpReadout->parent = NULL;
        psTrace("psModules.camera", 9, "Will now free readout %zd...\n", (size_t)tmpReadout);
    }
    cell->readouts = psArrayRealloc(cell->readouts, 0);
    cell->readouts->n = 0;
}


void pmChipFreeCells(pmChip *chip)
{
    PS_ASSERT_PTR_NON_NULL(chip,);

    //
    // Set the parent to NULL in all chip->cells before psFree(chip->cells)
    // in order to avoid memory reference counter problems.
    //
    psArray *cells = chip->cells;
    for (int i = 0 ; i < cells->n ; i++) {
        pmCell *tmpCell = cells->data[i];
        if (! tmpCell) {
            continue;
        }
        tmpCell->parent = NULL;
        pmCellFreeReadouts(tmpCell);// Drop all readouts the cell holds
    }
    chip->cells = psArrayRealloc(chip->cells, 0);
    chip->cells->n = 0;
}

void pmReadoutFreeData (pmReadout *readout)
{
    if (!readout) {
        return;
    }

    psFree(readout->image);
    psFree(readout->mask);
    psFree(readout->variance);
    psFree(readout->covariance);
    psFree(readout->analysis);
    psFree(readout->bias);

    psTrace("psModules.camera", 3, "Freeing readout data for %zd\n", (size_t) readout);

    readout->image = NULL;
    readout->variance = NULL;
    readout->covariance = NULL;
    readout->analysis = NULL;
    readout->mask = NULL;
    readout->bias = NULL;

    readout->col0 = 0;
    readout->row0 = 0;

    readout->thisImageScan = 0;
    readout->thisMaskScan = 0;
    readout->thisVarianceScan = 0;

    readout->lastImageScan = 0;
    readout->lastMaskScan = 0;
    readout->lastVarianceScan = 0;
}

void pmCellFreeData(pmCell *cell)
{
    if (!cell) {
        return;
    }

    for (int i = 0; i < cell->readouts->n; i++) {
        pmReadoutFreeData(cell->readouts->data[i]);
    }
    if (cell->hdu) {
        psFree(cell->hdu->images);
        psFree(cell->hdu->variances);
        psFree(cell->hdu->masks);
        // psFree(cell->hdu->header);

        cell->hdu->images = NULL;
        cell->hdu->variances = NULL;
        cell->hdu->masks = NULL;
        // cell->hdu->header = NULL;
    }
}

void pmChipFreeData(pmChip *chip)
{
    if (!chip) {
        return;
    }

    for (int i = 0; i < chip->cells->n; i++) {
        pmCellFreeData(chip->cells->data[i]);
    }
    if (chip->hdu) {
        psFree(chip->hdu->images);
        psFree(chip->hdu->variances);
        psFree(chip->hdu->masks);
        // psFree(chip->hdu->header);

        chip->hdu->images = NULL;
        chip->hdu->variances = NULL;
        chip->hdu->masks = NULL;
        // chip->hdu->header = NULL;
    }
}

void pmFPAFreeData(pmFPA *fpa)
{
    if (!fpa) {
        return;
    }

    for (int i = 0; i < fpa->chips->n; i++) {
        pmChipFreeData(fpa->chips->data[i]);
    }
    if (fpa->hdu) {
        psFree(fpa->hdu->images);
        psFree(fpa->hdu->variances);
        psFree(fpa->hdu->masks);
        // psFree(fpa->hdu->header);

        fpa->hdu->images = NULL;
        fpa->hdu->variances = NULL;
        fpa->hdu->masks = NULL;
        // fpa->hdu->header = NULL;
    }
}

pmReadout *pmReadoutAlloc(pmCell *cell)
{
    pmReadout *tmpReadout = (pmReadout *)psAlloc(sizeof(pmReadout));
    psMemSetDeallocator(tmpReadout, (psFreeFunc) readoutFree);

    tmpReadout->image = NULL;
    tmpReadout->mask = NULL;
    tmpReadout->variance = NULL;
    tmpReadout->covariance = NULL;
    tmpReadout->bias = psListAlloc(NULL);
    tmpReadout->analysis = psMetadataAlloc();
    tmpReadout->parent = cell;
    if (cell) {
        cell->readouts = psArrayAdd(cell->readouts, 1, (psPtr) tmpReadout);
    }

    tmpReadout->process = true;            // All cells are processed by default
    tmpReadout->file_exists = false;       // file not yet identified
    tmpReadout->data_exists = false;       // data yet read in

    tmpReadout->row0 = 0;
    tmpReadout->col0 = 0;

    tmpReadout->thisImageScan = 0;
    tmpReadout->thisMaskScan = 0;
    tmpReadout->thisVarianceScan = 0;

    tmpReadout->lastImageScan = 0;
    tmpReadout->lastMaskScan = 0;
    tmpReadout->lastVarianceScan = 0;

    tmpReadout->forceScan = false;

    return(tmpReadout);
}

bool psMemCheckReadout(psPtr ptr)
{
    PS_ASSERT_PTR(ptr, false);
    return ( psMemGetDeallocator(ptr) == (psFreeFunc) readoutFree);
}


pmCell *pmCellAlloc(pmChip *chip, const char *name)
{
    pmCell *tmpCell = (pmCell *) psAlloc(sizeof(pmCell));
    psMemSetDeallocator(tmpCell, (psFreeFunc) cellFree);

    tmpCell->config = NULL;
    tmpCell->analysis = psMetadataAlloc();
    tmpCell->readouts = psArrayAlloc(0);
    tmpCell->parent = chip;
    if (chip) {
        chip->cells = psArrayAdd(chip->cells, 1, (psPtr) tmpCell);
    }
    tmpCell->hdu = NULL;
    tmpCell->process = true;            // All cells are processed by default
    tmpCell->file_exists = false;       // Not yet identified
    tmpCell->data_exists = false;       // Not yet read in

    tmpCell->concepts = psMetadataAlloc();
    if (!psMetadataAddStr(tmpCell->concepts, PS_LIST_HEAD, "CELL.NAME", 0, NULL, name)) {
        psErrorClear();
        psWarning("Could not add CELL.NAME to metadata.");
    }
    tmpCell->conceptsRead = PM_CONCEPT_SOURCE_NONE;
    pmConceptsBlankCell(tmpCell);

    return tmpCell;
}

bool psMemCheckCell(psPtr ptr)
{
    PS_ASSERT_PTR(ptr, false);
    return ( psMemGetDeallocator(ptr) == (psFreeFunc) cellFree);
}


pmChip *pmChipAlloc(
    pmFPA *fpa,
    const char *name)
{
    pmChip *tmpChip = (pmChip*)psAlloc(sizeof(pmChip));
    psMemSetDeallocator(tmpChip, (psFreeFunc) chipFree);

    tmpChip->toFPA = NULL;
    tmpChip->fromFPA = NULL;

    tmpChip->analysis = psMetadataAlloc();
    tmpChip->cells = psArrayAlloc(0);
    tmpChip->parent = fpa;
    if (fpa) {
        psArrayAdd(fpa->chips, 1, (psPtr)tmpChip);
    }
    tmpChip->hdu = NULL;
    tmpChip->process = true;            // Work on all chips, by default
    tmpChip->file_exists = false;       // Not yet identified
    tmpChip->data_exists = false;       // Not yet read in

    tmpChip->concepts = psMetadataAlloc();
    if (!psMetadataAddStr(tmpChip->concepts, PS_LIST_HEAD, "CHIP.NAME", 0, NULL, name)) {
        psErrorClear();
        psWarning("Could not add CHIP.NAME %s to concepts.", name);
    }
    tmpChip->conceptsRead = PM_CONCEPT_SOURCE_NONE;
    pmConceptsBlankChip(tmpChip);
    return tmpChip;
}

bool psMemCheckChip(psPtr ptr)
{
    PS_ASSERT_PTR(ptr, false);
    return ( psMemGetDeallocator(ptr) == (psFreeFunc) chipFree);
}

pmFPA *pmFPAAlloc(const psMetadata *camera, const char *cameraName)
{
    pmFPA *tmpFPA = (pmFPA *) psAlloc(sizeof(pmFPA));
    psMemSetDeallocator(tmpFPA, (psFreeFunc) FPAFree);

    tmpFPA->fromTPA = NULL;
    tmpFPA->toTPA = NULL;
    tmpFPA->toSky = NULL;
    tmpFPA->wcsCDkeys = false;

    tmpFPA->analysis = psMetadataAlloc();
    tmpFPA->camera = psMemIncrRefCounter((psPtr) camera);
    tmpFPA->chips = psArrayAlloc(0);
    tmpFPA->hdu = NULL;

    tmpFPA->concepts = psMetadataAlloc();
    if (!psMetadataAddStr(tmpFPA->concepts, PS_LIST_TAIL, "FPA.CAMERA", PS_META_REPLACE,
                          "Camera name (according to configuration)", cameraName)) {
        psErrorClear();
        psWarning("Could not add FPA.CAMERA %s to concepts.", cameraName);
    }
    tmpFPA->conceptsRead = PM_CONCEPT_SOURCE_NONE;
    pmConceptsBlankFPA(tmpFPA);

    // this may be somewhat pedantic, but it makes these things consistent
    if (!psMetadataAddStr(tmpFPA->concepts, PS_LIST_TAIL, "FPA.NAME", PS_META_REPLACE,
                          "name of FPA level", "fpa")) {
        psErrorClear();
        psWarning("Could not add FPA.NAME to concepts.");
    }

    return tmpFPA;
}

bool psMemCheckFPA(psPtr ptr)
{
    PS_ASSERT_PTR(ptr, false);
    return ( psMemGetDeallocator(ptr) == (psFreeFunc) FPAFree);
}


// Check a cell to ensure that all component readouts have the parent pointer set correctly
static bool cellCheckParents(pmCell *cell // Cell to check
                            )
{
    PS_ASSERT_PTR_NON_NULL(cell, true);

    bool flag = true;
    for (long i = 0; i < cell->readouts->n ; i++) {
        pmReadout *tmpReadout = (pmReadout *) cell->readouts->data[i];
        if (!tmpReadout) {
            continue;
        }
        if (tmpReadout->parent != cell) {
            tmpReadout->parent = cell;
            flag = false;
        }
    }
    return flag;
}

// Check a chip to ensure that all component cells have the parent pointer set correctly
static bool chipCheckParents(pmChip *chip // Chip to check
                            )
{
    PS_ASSERT_PTR_NON_NULL(chip, true);

    bool flag = true;
    for (long i = 0; i < chip->cells->n ; i++) {
        pmCell *tmpCell = (pmCell*)chip->cells->data[i];
        if (!tmpCell) {
            continue;
        }
        if (tmpCell->parent != chip) {
            tmpCell->parent = chip;
            flag = false;
        }

        flag &= cellCheckParents(tmpCell);
    }
    return flag;
}

psBool pmFPACheckParents(pmFPA *fpa)
{
    PS_ASSERT_PTR_NON_NULL(fpa, false);

    bool flag = true;
    for (long i = 0; i < fpa->chips->n ; i++) {
        pmChip *tmpChip = (pmChip*)fpa->chips->data[i];
        if (!tmpChip) {
            continue;
        }
        if (tmpChip->parent != fpa) {
            tmpChip->parent = fpa;
            flag = false;
        }

        flag &= chipCheckParents(tmpChip);
    }
    return flag;
}
