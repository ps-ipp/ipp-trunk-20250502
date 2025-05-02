#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include <psmodules.h>
#include "tap.h"
#include "pstap.h"
/* STATUS:
    All functions are tested.
*/

#define CHIP_ALLOC_NAME        "ChipName"
#define CELL_ALLOC_NAME        "CellName"
#define MISC_NUM                32
#define MISC_NAME              "META00"
#define MISC_NAME2             "META01"
#define NUM_BIAS_DATA           10
#define TEST_NUM_ROWS           4
#define TEST_NUM_COLS           4
#define NUM_READOUTS            3
#define NUM_CELLS               10
#define NUM_CHIPS               8
#define NUM_HDUS                5
#define BASE_IMAGE              10
#define BASE_MASK               40
#define BASE_WEIGHT             70
#define VERBOSE                 0
#define ERR_TRACE_LEVEL         0

psPlaneTransform *PS_CREATE_2D_IDENTITY_PLANE_TRANSFORM()
{
    psPlaneTransform *pt = psPlaneTransformAlloc(1, 1);
    pt->x->coeff[1][0] = 1.0;
    pt->y->coeff[0][1] = 1.0;
    return(pt);
}

psPlaneDistort *PS_CREATE_4D_IDENTITY_PLANE_DISTORT()
{
    psPlaneDistort *pd = psPlaneDistortAlloc(1, 1, 1, 1);
    pd->x->coeff[1][0][0][0] = 1.0;
    pd->y->coeff[0][1][0][0] = 1.0;
    return(pd);
}

/******************************************************************************
generateSimpleReadout(): This function generates a pmReadout data structure and then
populates its members with real data.
 *****************************************************************************/
pmReadout *generateSimpleReadout(pmCell *cell)
{
    pmReadout *readout = pmReadoutAlloc(cell);
    readout->image = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_F32);
    readout->mask = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_U8);
    readout->variance = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_F32);
    for (psS32 i = 0 ; i < NUM_BIAS_DATA ; i++) {
        psImage *tmpImage = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_F32);
        psImageInit(tmpImage, (double) i);
        psListAdd(readout->bias, PS_LIST_HEAD, tmpImage);
        psFree(tmpImage);
    }
    psMetadataAddS32(readout->analysis, PS_LIST_HEAD, MISC_NAME, 0, NULL, MISC_NUM);
    return(readout);
}

/******************************************************************************
generateSimpleCell(): This function generates a pmCell data structure and then
populates its members with real data.
 *****************************************************************************/
pmCell *generateSimpleCell(pmChip *chip)
{
    pmCell *cell = pmCellAlloc(chip, CELL_ALLOC_NAME);

    psMetadataAddS32(cell->analysis, PS_LIST_HEAD, MISC_NAME, 0, NULL, MISC_NUM);
    psMetadataAddS32(cell->concepts, PS_LIST_HEAD, MISC_NAME, 0, NULL, MISC_NUM);
    psArrayRealloc(cell->readouts, NUM_READOUTS);
    cell->hdu = pmHDUAlloc("cellExtName");
    for (int i = 0 ; i < NUM_READOUTS ; i++) {
        cell->readouts->data[i] = psMemDecrRefCounter((psPtr) generateSimpleReadout(cell));
    }

    // First try to read data from ../dataFiles, then try dataFiles.
    bool rc = pmConfigFileRead(&cell->hdu->format, "../dataFiles/camera0/format0.config", "Camera format 0");
    if (!rc) {
        rc = pmConfigFileRead(&cell->hdu->format, "dataFiles/camera0/format0.config", "Camera format 0");
        if (!rc) {
            diag("pmConfigFileRead() was unsuccessful (from generateSimpleCell())");
	}
    }

    cell->hdu->images = psArrayAlloc(NUM_HDUS);
    cell->hdu->masks = psArrayAlloc(NUM_HDUS);
    cell->hdu->variances = psArrayAlloc(NUM_HDUS);
    for (int k = 0 ; k < NUM_HDUS ; k++) {
        cell->hdu->images->data[k]  = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_F32);
        cell->hdu->masks->data[k]   = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_MASK);
        cell->hdu->variances->data[k] = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_F32);
        psImageInit(cell->hdu->images->data[k], (float) (BASE_IMAGE+k));
        psImageInit(cell->hdu->masks->data[k], (psU8) (BASE_MASK+k));
        psImageInit(cell->hdu->variances->data[k], (float) (BASE_WEIGHT+k));
    }

    //XXX: Should the region be set some other way?  Like through the various config files?
    psRegion *region = psRegionAlloc(0.0, 0.0, 0.0, 0.0);
    // You shouldn't have to remove the key from the metadata.  Find out how to simply change the key value.
    psMetadataRemoveKey(cell->concepts, "CELL.TRIMSEC");
    psMetadataAddPtr(cell->concepts, PS_LIST_TAIL|PS_META_REPLACE, "CELL.TRIMSEC", PS_DATA_REGION, "I am a region", region);
    psFree(region);
    return(cell);
}

/******************************************************************************
generateSimpleChip(): This function generates a pmChip data structure and then
populates its members with real data.
 *****************************************************************************/
pmChip *generateSimpleChip(pmFPA *fpa)
{
    pmChip *chip = pmChipAlloc(fpa, CHIP_ALLOC_NAME);
    chip->toFPA = PS_CREATE_2D_IDENTITY_PLANE_TRANSFORM();
    chip->fromFPA = PS_CREATE_2D_IDENTITY_PLANE_TRANSFORM();
    psMetadataAddS32(chip->analysis, PS_LIST_HEAD, MISC_NAME, 0, NULL, MISC_NUM);
    psMetadataAddS32(chip->concepts, PS_LIST_HEAD, MISC_NAME, 0, NULL, MISC_NUM);
    chip->hdu = pmHDUAlloc("chipExtName");

    bool rc = pmConfigFileRead(&chip->hdu->format, "dataFiles/camera0/format0.config", "Camera format 0");
    if (!rc) {
        rc = pmConfigFileRead(&chip->hdu->format, "../dataFiles/camera0/format0.config", "Camera format 0");
        if (!rc) {
            diag("pmConfigFileRead() was unsuccessful (from generateSimpleChip())");
	}
    }

    psArrayRealloc(chip->cells, NUM_CELLS);
    for (int i = 0 ; i < NUM_CELLS ; i++) {
        chip->cells->data[i] = psMemDecrRefCounter((psPtr) generateSimpleCell(chip));
    }
    return(chip);
}

/******************************************************************************
generateSimpleFPA(): This function generates a pmFPA data structure and then
populates its members with real data.
 *****************************************************************************/
pmFPA* generateSimpleFPA(psMetadata *camera)
{
    pmFPA* fpa = pmFPAAlloc(camera, NULL);
    fpa->fromTPA = PS_CREATE_2D_IDENTITY_PLANE_TRANSFORM();
    fpa->toTPA = PS_CREATE_2D_IDENTITY_PLANE_TRANSFORM();
    fpa->toSky = psProjectionAlloc(0.0,0.0,10.0,10.0,PS_PROJ_TAN);
    psMetadataAddS32(fpa->analysis, PS_LIST_HEAD, MISC_NAME, 0, NULL, MISC_NUM);
    if (camera != NULL) {
        psMetadataAddS32((psMetadata *) fpa->camera, PS_LIST_HEAD, MISC_NAME, 0, NULL, MISC_NUM);
    }
    psMetadataAddS32(fpa->concepts, PS_LIST_HEAD, MISC_NAME, 0, NULL, MISC_NUM);
    fpa->hdu = pmHDUAlloc("fpaExtName");
    bool rc = pmConfigFileRead(&fpa->hdu->format, "dataFiles/camera0/format0.config", "Camera format 0");
    if (!rc) {
        rc = pmConfigFileRead(&fpa->hdu->format, "../dataFiles/camera0/format0.config", "Camera format 0");
        if (!rc) {
            diag("pmConfigFileRead() was unsuccessful (from generateSimpleFPA())");
	}
    }

    psArrayRealloc(fpa->chips, NUM_CHIPS);
    for (int i = 0 ; i < NUM_CHIPS ; i++) {
        fpa->chips->data[i] = psMemDecrRefCounter((psPtr) generateSimpleChip(fpa));
    }
    pmConceptsBlankFPA(fpa);
    return(fpa);
}


psS32 main(psS32 argc, char* argv[])
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    psTraceSetLevel("err", ERR_TRACE_LEVEL);
    plan_tests(73);


    // ----------------------------------------------------------------------
    // pmHDUFromFPA() tests
    // Call pmHDUFromFPA() with NULL input params
    {
        psMemId id = psMemGetId();
        pmHDU *hdu = pmHDUFromFPA(NULL);
        ok(hdu == NULL, "pmHDUFromFPA(NULL) returned NULL");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmHDUFromFPA() with acceptable input params
    {
        psMemId id = psMemGetId();
        // Generate the pmFPA heirarchy
        psMetadata *camera = psMetadataAlloc();
        pmFPA* fpa = generateSimpleFPA(camera);
        pmChip *chip = fpa->chips->data[0];
        pmCell *cell = chip->cells->data[0];
        ok(fpa != NULL, "Allocated a pmFPA successfully");
        ok(chip != NULL, "Allocated a pmChip successfully");
        ok(cell != NULL, "Allocated a pmCell successfully");
        pmHDU *hdu = pmHDUFromFPA(fpa);
        ok(hdu == fpa->hdu, "pmHDUFromFPA(NULL) returned the correct pmHDU of an pmFPA struct");
        psFree(fpa);
        psFree(camera);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // pmHDUFromChip() tests
    // Call pmHDUFromChip() with NULL input params
    {
        psMemId id = psMemGetId();
        pmHDU *hdu = pmHDUFromChip(NULL);
        ok(hdu == NULL, "pmHDUFromChip(NULL) returned NULL");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmHDUFromChip() with acceptable input params
    {
        psMemId id = psMemGetId();
        // Generate the pmFPA heirarchy
        psMetadata *camera = psMetadataAlloc();
        pmFPA* fpa = generateSimpleFPA(camera);
        pmChip *chip = fpa->chips->data[0];
        pmCell *cell = chip->cells->data[0];
        ok(fpa != NULL, "Allocated a pmFPA successfully");
        ok(chip != NULL, "Allocated a pmChip successfully");
        ok(cell != NULL, "Allocated a pmCell successfully");
        pmHDU *hdu = pmHDUFromChip(chip);
        ok(hdu == chip->hdu, "pmHDUFromChip() returned the correct pmHDU of an pmChip struct");
        psFree(fpa);
        psFree(camera);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmHDUFromChip() with acceptable input params
    // Set chip->hdu to NULL, verify chip->parent->hdu is returned
    {
        psMemId id = psMemGetId();
        // Generate the pmFPA heirarchy
        psMetadata *camera = psMetadataAlloc();
        pmFPA* fpa = generateSimpleFPA(camera);
        pmChip *chip = fpa->chips->data[0];
        pmCell *cell = chip->cells->data[0];
        ok(fpa != NULL, "Allocated a pmFPA successfully");
        ok(chip != NULL, "Allocated a pmChip successfully");
        ok(cell != NULL, "Allocated a pmCell successfully");
        psFree(chip->hdu);
        chip->hdu = NULL;
        pmHDU *hdu = pmHDUFromChip(chip);
        ok(hdu == chip->parent->hdu, "pmHDUFromChip() returned the correct pmHDU of an pmChip struct");
        psFree(fpa);
        psFree(camera);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // pmHDUFromCell() tests
    // Call pmHDUFromCell() with NULL input params
    {
        psMemId id = psMemGetId();
        pmHDU *hdu = pmHDUFromCell(NULL);
        ok(hdu == NULL, "pmHDUFromCell(NULL) returned NULL");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmHDUFromCell() with acceptable input params
    {
        psMemId id = psMemGetId();
        // Generate the pmFPA heirarchy
        psMetadata *camera = psMetadataAlloc();
        pmFPA* fpa = generateSimpleFPA(camera);
        pmChip *chip = fpa->chips->data[0];
        pmCell *cell = chip->cells->data[0];
        ok(fpa != NULL, "Allocated a pmFPA successfully");
        ok(chip != NULL, "Allocated a pmChip successfully");
        ok(cell != NULL, "Allocated a pmCell successfully");
        pmHDU *hdu = pmHDUFromCell(cell);
        ok(hdu == cell->hdu, "pmHDUFromCell() returned the correct pmHDU of an pmCell struct");
        psFree(fpa);
        psFree(camera);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmHDUFromCell() with acceptable input params
    // Set cell->hdu to NULL, verify cell->parent->hdu is returned
    {
        psMemId id = psMemGetId();
        // Generate the pmFPA heirarchy
        psMetadata *camera = psMetadataAlloc();
        pmFPA* fpa = generateSimpleFPA(camera);
        pmChip *chip = fpa->chips->data[0];
        pmCell *cell = chip->cells->data[0];
        ok(fpa != NULL, "Allocated a pmFPA successfully");
        ok(chip != NULL, "Allocated a pmChip successfully");
        ok(cell != NULL, "Allocated a pmCell successfully");
        psFree(cell->hdu);
        cell->hdu = NULL;
        pmHDU *hdu = pmHDUFromCell(cell);
        ok(hdu == cell->parent->hdu, "pmHDUFromCell() returned the correct pmHDU of an pmCell struct");
        psFree(fpa);
        psFree(camera);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // pmHDUFromReadout() tests
    // Call pmHDUFromReadout() with NULL input params
    {
        psMemId id = psMemGetId();
        pmHDU *hdu = pmHDUFromReadout(NULL);
        ok(hdu == NULL, "pmHDUFromReadout(NULL) returned NULL");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmHDUFromReadout() with acceptable input params
    {
        psMemId id = psMemGetId();
        // Generate the pmFPA heirarchy
        psMetadata *camera = psMetadataAlloc();
        pmFPA* fpa = generateSimpleFPA(camera);
        pmChip *chip = fpa->chips->data[0];
        pmCell *cell = chip->cells->data[0];
        pmReadout *readout = cell->readouts->data[0];
        ok(fpa != NULL, "Allocated a pmFPA successfully");
        ok(chip != NULL, "Allocated a pmChip successfully");
        ok(cell != NULL, "Allocated a pmCell successfully");
        ok(readout != NULL, "Allocated a pmReadout successfully");
        pmHDU *hdu = pmHDUFromReadout(readout);
        ok(hdu == readout->parent->hdu, "pmHDUFromReadout() returned the correct pmHDU of an pmReadout struct");
        psFree(fpa);
        psFree(camera);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // pmHDUGetLowest() tests
    // Call pmHDUGetLowest() with all NULL inputs
    {
        psMemId id = psMemGetId();
        pmHDU *hdu = pmHDUGetLowest(NULL, NULL, NULL);
        ok(hdu == NULL, "pmHDUFromReadout(NULL, NULL, NULL)");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmHDUGetLowest() with all acceptable inputs
    {
        psMemId id = psMemGetId();
        // Generate the pmFPA heirarchy
        psMetadata *camera = psMetadataAlloc();
        pmFPA* fpa = generateSimpleFPA(camera);
        pmChip *chip = fpa->chips->data[0];
        pmCell *cell = chip->cells->data[0];
        ok(fpa != NULL, "Allocated a pmFPA successfully");
        ok(chip != NULL, "Allocated a pmChip successfully");
        ok(cell != NULL, "Allocated a pmCell successfully");
        pmHDU *hdu = pmHDUGetLowest(fpa, chip, cell);
        ok(hdu == cell->hdu, "pmHDUGetLowest(fpa, chip, cell)");
        psFree(fpa);
        psFree(camera);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmHDUGetLowest() with all (fpa, chip, NULL) inputs
    {
        psMemId id = psMemGetId();
        // Generate the pmFPA heirarchy
        psMetadata *camera = psMetadataAlloc();
        pmFPA* fpa = generateSimpleFPA(camera);
        pmChip *chip = fpa->chips->data[0];
        pmCell *cell = chip->cells->data[0];
        ok(fpa != NULL, "Allocated a pmFPA successfully");
        ok(chip != NULL, "Allocated a pmChip successfully");
        ok(cell != NULL, "Allocated a pmCell successfully");
        pmHDU *hdu = pmHDUGetLowest(fpa, chip, NULL);
        ok(hdu == chip->hdu, "pmHDUGetLowest(fpa, chip, NULL)");
        psFree(fpa);
        psFree(camera);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmHDUGetLowest() with all (fpa, NULL, NULL) inputs
    {
        psMemId id = psMemGetId();
        // Generate the pmFPA heirarchy
        psMetadata *camera = psMetadataAlloc();
        pmFPA* fpa = generateSimpleFPA(camera);
        pmChip *chip = fpa->chips->data[0];
        pmCell *cell = chip->cells->data[0];
        ok(fpa != NULL, "Allocated a pmFPA successfully");
        ok(chip != NULL, "Allocated a pmChip successfully");
        ok(cell != NULL, "Allocated a pmCell successfully");
        pmHDU *hdu = pmHDUGetLowest(fpa, NULL, NULL);
        ok(hdu == fpa->hdu, "pmHDUGetLowest(fpa, NULL, NULL)");
        psFree(fpa);
        psFree(camera);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    //pmHDUGetHighest() tests
    // Call pmHDUGetHighest() with all NULL inputs
    {
        psMemId id = psMemGetId();
        pmHDU *hdu = pmHDUGetHighest(NULL, NULL, NULL);
        ok(hdu == NULL, "pmHDUFromReadout(NULL, NULL, NULL)");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmHDUGetHighest() with all acceptable inputs
    {
        psMemId id = psMemGetId();
        // Generate the pmFPA heirarchy
        psMetadata *camera = psMetadataAlloc();
        pmFPA* fpa = generateSimpleFPA(camera);
        pmChip *chip = fpa->chips->data[0];
        pmCell *cell = chip->cells->data[0];
        ok(fpa != NULL, "Allocated a pmFPA successfully");
        ok(chip != NULL, "Allocated a pmChip successfully");
        ok(cell != NULL, "Allocated a pmCell successfully");
        pmHDU *hdu = pmHDUGetHighest(fpa, chip, cell);
        ok(hdu == fpa->hdu, "pmHDUGetHighest(fpa, chip, cell)");
        psFree(fpa);
        psFree(camera);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmHDUGetHighest() with (NULL, chip, cell) inputs
    {
        psMemId id = psMemGetId();
        // Generate the pmFPA heirarchy
        psMetadata *camera = psMetadataAlloc();
        pmFPA* fpa = generateSimpleFPA(camera);
        pmChip *chip = fpa->chips->data[0];
        pmCell *cell = chip->cells->data[0];
        ok(fpa != NULL, "Allocated a pmFPA successfully");
        ok(chip != NULL, "Allocated a pmChip successfully");
        ok(cell != NULL, "Allocated a pmCell successfully");
        pmHDU *hdu = pmHDUGetHighest(NULL, chip, cell);
        ok(hdu == chip->hdu, "pmHDUGetHighest(NULL, chip, cell)");
        psFree(fpa);
        psFree(camera);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmHDUGetHighest() with (NULL, NULL, cell) inputs
    {
        psMemId id = psMemGetId();
        // Generate the pmFPA heirarchy
        psMetadata *camera = psMetadataAlloc();
        pmFPA* fpa = generateSimpleFPA(camera);
        pmChip *chip = fpa->chips->data[0];
        pmCell *cell = chip->cells->data[0];
        ok(fpa != NULL, "Allocated a pmFPA successfully");
        ok(chip != NULL, "Allocated a pmChip successfully");
        ok(cell != NULL, "Allocated a pmCell successfully");
        pmHDU *hdu = pmHDUGetHighest(NULL, NULL, cell);
        ok(hdu == cell->hdu, "pmHDUGetHighest(NULL, NULL, cell)");
        psFree(fpa);
        psFree(camera);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
}
