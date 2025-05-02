#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include <psmodules.h>
#include "tap.h"
#include "pstap.h"
/* STATUS:
    All functions are tested.
*/

#define CHIP_ALLOC_NAME "ChipName"
#define CELL_ALLOC_NAME "CellName"
#define MISC_NUM 32
#define MISC_NAME "META00"
#define MISC_NAME2 "META01"
#define NUM_BIAS_DATA 10
#define TEST_NUM_ROWS 32
#define TEST_NUM_COLS 32
#define NUM_READOUTS	4
#define NUM_CELLS	6
#define NUM_CHIPS	8

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
    cell->hdu = pmHDUAlloc(NULL);
    psMetadataAddS32(cell->concepts, PS_LIST_HEAD, MISC_NAME, 0, NULL, MISC_NUM);
    psArrayRealloc(cell->readouts, NUM_READOUTS);
    for (int i = 0 ; i < NUM_READOUTS ; i++) {
        cell->readouts->data[i] = psMemDecrRefCounter((psPtr) generateSimpleReadout(cell));
    }
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
    chip->hdu = pmHDUAlloc(NULL);
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
    fpa->hdu = pmHDUAlloc(NULL);
    psArrayRealloc(fpa->chips, NUM_CHIPS);
    for (int i = 0 ; i < NUM_CHIPS ; i++) {
        fpa->chips->data[i] = psMemDecrRefCounter((psPtr) generateSimpleChip(fpa));
    }

    return(fpa);
}


psS32 main(psS32 argc, char* argv[])
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    plan_tests(92);

    // ------------------------------------------------------------------------
    // pmFPAAlloc() tests
    // Call pmFPAAlloc() with NULL pmCamera input.
    {
        psMemId id = psMemGetId();
        pmFPA* fpa = pmFPAAlloc(NULL, NULL);
        ok(fpa != NULL && psMemCheckFPA(fpa), "pmFPAAlloc() returned a non-NULL pmFPA with a NULL pmCamera input");
        psFree(fpa);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Call pmFPAAlloc() with acceptable input parameters.
    {
        psMemId id = psMemGetId();
        psMetadata *camera = psMetadataAlloc();
        pmFPA* fpa = pmFPAAlloc(camera, NULL);
        ok(fpa != NULL, "pmFPAAlloc() returned a non-NULL");
        ok(fpa->fromTPA == NULL, "pmFPAAlloc() set ->fromTPA to NULL");
        ok(fpa->toTPA == NULL, "pmFPAAlloc() set ->toTPA to NULL");
        ok(fpa->toSky == NULL, "pmFPAAlloc() set ->toSky to NULL");
        ok(fpa->concepts != NULL &&
           psMemCheckMetadata(fpa->concepts), "pmFPAAlloc() set ->concepts correctly");
        ok(fpa->conceptsRead == PM_CONCEPT_SOURCE_NONE, "pmFPAAlloc() set ->conceptsRead correctly");
        ok(fpa->analysis != NULL &&
           psMemCheckMetadata(fpa->analysis), "pmFPAAlloc() set ->analysis correctly");
        ok(fpa->camera == camera, "pmFPAAlloc() set ->camera correctly");
        ok(fpa->chips != NULL &&
           psMemCheckArray(fpa->chips), "pmFPAAlloc() set ->chips correctly");
        ok(fpa->hdu == NULL, "pmFPAAlloc() set ->hdu to NULL");
        psFree(fpa);
        psFree(camera);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Populate the pmFPA struct with real data to ensure they were psFree()'ed correctly.
    {
        psMemId id = psMemGetId();
        pmFPA* fpa = generateSimpleFPA(NULL);
        psFree(fpa);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks (Populate the pmFPA struct with real data)");
    }


    // ------------------------------------------------------------------------
    // pmFPAFreeData() tests
    // Call pmFPAFreeData() with NULL pmFPA input parameter
    {
        psMemId id = psMemGetId();
        pmFPAFreeData(NULL);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks (Call pmFPAFreeData() with NULL pmFPA input parameter)");
    }


    // Call pmFPAFreeData() with acceptable input parameters
    {
        psMemId id = psMemGetId();
        pmFPA* fpa = generateSimpleFPA(NULL);
        fpa->hdu->images = psArrayAlloc(10);
        fpa->hdu->variances = psArrayAlloc(10);
        fpa->hdu->masks = psArrayAlloc(10);
        pmFPAFreeData(fpa);
        ok(fpa->hdu->images == NULL, "pmFPAFreeData() correctly set fpa->hdu->images to NULL");
        ok(fpa->hdu->variances == NULL, "pmFPAFreeData() correctly set fpa->hdu->weights to NULL");
        ok(fpa->hdu->masks == NULL, "pmFPAFreeData() correctly set fpa->hdu->masks to NULL");
        psFree(fpa);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ------------------------------------------------------------------------
    // pmChipAlloc() tests
    // Call pmChipAlloc() with NULL input parameters
    {
        psMemId id = psMemGetId();
        pmChip *chip = pmChipAlloc(NULL, CHIP_ALLOC_NAME);
        ok(chip != NULL, "pmChipAlloc() returned non-NULL with NULL pmFPA input parameter");
        psFree(chip);
        pmFPA* fpa = generateSimpleFPA(NULL);
        chip = pmChipAlloc(fpa, NULL);
        ok(chip != NULL, "pmChipAlloc() returned non-NULL with NULL chip name input parameter");
        psFree(fpa);
        psFree(chip);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // pmChipAlloc() tests
    // XXX: Add tests for NULL inputs.
    {
        psMemId id = psMemGetId();
        pmFPA* fpa = generateSimpleFPA(NULL);
        pmChip *chip = pmChipAlloc(fpa, CHIP_ALLOC_NAME);
        ok(chip != NULL, "pmChipAlloc() returned non-NULL");
        ok(chip->toFPA == NULL, "pmChipAlloc() set chip->toChip to NULL");
        ok(chip->fromFPA == NULL, "pmChipAlloc() set chip->fromChip to NULL");
        ok(chip->concepts != NULL &&
           psMemCheckMetadata(chip->concepts), "pmChipAlloc() set ->concepts correctly");
        ok(chip->conceptsRead == PM_CONCEPT_SOURCE_NONE, "pmCellAlloc() set ->conceptsRead correctly");
        ok(chip->analysis != NULL &&
           psMemCheckMetadata(chip->analysis), "pmChipAlloc() set ->analysis correctly");
        ok(chip->cells != NULL &&
           psMemCheckArray(chip->cells), "pmChipAlloc() set ->cells correctly");
        ok(chip->parent == fpa, "pmChipAlloc() set ->parent correctly");
        ok(chip->process == true, "pmChipAlloc() set ->process correctly");
        ok(chip->file_exists == false, "pmChipAlloc() set ->file_exists correctly");
        ok(chip->data_exists == false, "pmChipAlloc() set ->data_exists correctly");
        ok(chip->hdu == NULL, "pmChipAlloc() set ->hdu to NULL");
        psFree(fpa);
        psFree(chip);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Populate the pmChip struct with real data to ensure they were free'ed correctly.
    {
        psMemId id = psMemGetId();
        pmChip *chip = generateSimpleChip(NULL);
        psFree(chip);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks (Populate the pmChip struct with real data)");
    }


    // ------------------------------------------------------------------------
    // pmChipFreeData() tests
    // Call pmChipFreeData() with NULL pmFPA input parameter
    {
        psMemId id = psMemGetId();
        pmChipFreeData(NULL);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks (Call pmChipFreeData() with NULL pmFPA input parameter)");
    }


    // Call pmChipFreeData() with acceptable input parameters
    {
        psMemId id = psMemGetId();
        pmChip* chip = generateSimpleChip(NULL);
        chip->hdu->images = psArrayAlloc(10);
        chip->hdu->variances = psArrayAlloc(10);
        chip->hdu->masks = psArrayAlloc(10);
        pmChipFreeData(chip);
        ok(chip->hdu->images == NULL, "pmChipFreeData() correctly set chip->hdu->images to NULL");
        ok(chip->hdu->variances == NULL, "pmChipFreeData() correctly set chip->hdu->weights to NULL");
        ok(chip->hdu->masks == NULL, "pmChipFreeData() correctly set chip->hdu->masks to NULL");
        psFree(chip);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ------------------------------------------------------------------------
    // pmChipFreeCells() tests
    // Call pmChipFreeCells() with NULL pmFPA input parameter
    {
        psMemId id = psMemGetId();
        pmChipFreeCells(NULL);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks (Call pmChipFreeCells() with NULL pmFPA input parameter)");
    }


    // Call pmChipFreeCells() with acceptable input parameters
    {
        psMemId id = psMemGetId();
        pmChip* chip = generateSimpleChip(NULL);
        pmChipFreeCells(chip);
        ok(chip->cells->n == 0, "pmChipFreeCells() free'ed chip->cells correctly");
        psFree(chip);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ------------------------------------------------------------------------
    // pmCellAlloc() tests
    // Call pmCellAlloc() with NULL input parameters
    {
        psMemId id = psMemGetId();
        pmCell *cell = pmCellAlloc(NULL, CELL_ALLOC_NAME);
        ok(cell != NULL, "pmCellAlloc returned non-NULL with NULL pmChip input parameter");
        psFree(cell);

        pmChip *chip = pmChipAlloc(NULL, NULL);
        cell = pmCellAlloc(chip, NULL);
        ok(cell != NULL, "pmCellAlloc returned non-NULL with NULL cell name input parameter");
        psFree(chip);
        psFree(cell);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // pmCellAlloc() tests
    // Call pmCellAlloc() with acceptable input parameters
    {
        psMemId id = psMemGetId();
        psMetadata *camera = psMetadataAlloc();
        pmFPA* fpa = generateSimpleFPA(camera);
        pmChip *chip = pmChipAlloc(fpa, CHIP_ALLOC_NAME);
        pmCell *cell = pmCellAlloc(chip, CELL_ALLOC_NAME);
        ok(cell != NULL, "pmCellAlloc returned non-NULL");
        ok(cell->concepts != NULL &&
           psMemCheckMetadata(cell->concepts), "pmCellpAlloc() set ->concepts correctly");
        ok(cell->conceptsRead == PM_CONCEPT_SOURCE_NONE, "pmCellAlloc() set ->conceptsRead correctly");
        ok(cell->config == NULL, "pmCellpAlloc() set ->config to NULL");
        ok(cell->analysis != NULL &&
           psMemCheckMetadata(cell->analysis), "pmCellAlloc() set ->analysis correctly");
        ok(cell->readouts != NULL &&
           psMemCheckArray(cell->readouts), "pmCellAlloc() set ->readouts correctly");
        ok(cell->parent == chip, "pmCellAlloc() set ->parent correctly");
        ok(cell->process == true, "pmCellAlloc() set ->process correctly");
        ok(cell->file_exists == false, "pmCellAlloc() set ->file_exists correctly");
        ok(cell->data_exists == false, "pmCellAlloc() set ->data_exists correctly");
        ok(cell->hdu == NULL, "pmCellAlloc() set ->hdu to NULL");
        psFree(camera);
        psFree(fpa);
        psFree(chip);
        psFree(cell);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Populate the pmCell struct with real data to ensure they were
    // psFree()'ed correctly.
    {
        psMemId id = psMemGetId();
        pmCell *cell = generateSimpleCell(NULL);
        psFree(cell);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks (Populate the pmCell struct with real data)");
    }


    // ------------------------------------------------------------------------
    // pmCellFreeData() tests
    // Call pmCellFreeData() with NULL pmFPA input parameter
    {
        psMemId id = psMemGetId();
        pmCellFreeData(NULL);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks (Call pmCellFreeData() with NULL pmFPA input parameter)");
    }


    // Call pmCellFreeData() with acceptable input parameters
    {
        psMemId id = psMemGetId();
        pmCell *cell = generateSimpleCell(NULL);
        cell->hdu->images = psArrayAlloc(10);
        cell->hdu->variances = psArrayAlloc(10);
        cell->hdu->masks = psArrayAlloc(10);
        pmCellFreeData(cell);
        ok(cell->hdu->images == NULL, "pmCellFreeData() correctly set cell->hdu->images to NULL");
        ok(cell->hdu->variances == NULL, "pmCellFreeData() correctly set cell->hdu->weights to NULL");
        ok(cell->hdu->masks == NULL, "pmCellFreeData() correctly set cell->hdu->masks to NULL");
        psFree(cell);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ------------------------------------------------------------------------
    // test pmCellFreeReadouts()
    {
        psMemId id = psMemGetId();
        pmCell *cell = generateSimpleCell(NULL);
        pmCellFreeReadouts(cell);
        ok(cell->readouts->n == 0, "pmCellFreeReadouts() correctly set cell->readouts->n to 0");
        psFree(cell);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks (pmCellFreeReadouts)");
    }


    // ------------------------------------------------------------------------
    // pmReadoutAlloc() tests
    // Call pmReadoutAlloc() with NULL input parameters
    {
        psMemId id = psMemGetId();
        pmReadout *readout = pmReadoutAlloc(NULL);
        ok(readout != NULL, "pmReadoutAlloc() returned non-NULL with NULL pmCell input parameter");
        psFree(readout);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmReadoutAlloc() with acceptable parameters
    {
        psMemId id = psMemGetId();
        psMetadata *camera = psMetadataAlloc();
        pmFPA* fpa = generateSimpleFPA(camera);
        pmChip *chip = pmChipAlloc(fpa, CHIP_ALLOC_NAME);
        pmCell *cell = pmCellAlloc(chip, CELL_ALLOC_NAME);
        pmReadout *readout = pmReadoutAlloc(cell);
        ok(readout != NULL, "pmReadoutAlloc() returned non-NULL");
        ok(readout->col0 == 0, "pmReadoutAlloc() set ->col0 correctly");
        ok(readout->row0 == 0, "pmReadoutAlloc() set ->row0 correctly");
        ok(readout->image == NULL, "pmReadoutAlloc() set ->image correctly");
        ok(readout->mask == NULL, "pmReadoutAlloc() set ->mask correctly");
        ok(readout->variance == NULL, "pmReadoutAlloc() set ->weight correctly");
        ok(readout->bias != NULL &&
           psMemCheckList(readout->bias), "pmReadoutAlloc() set ->bias correctly");
        ok(readout->analysis != NULL &&
           psMemCheckMetadata(readout->analysis), "pmReadoutAlloc() set ->analysis correctly");
        ok(readout->parent == cell, "pmReadoutAlloc() set ->parent correctly");
        ok(readout->process == true, "pmReadoutAlloc() set ->process correctly");
        ok(readout->file_exists == false, "pmReadoutAlloc() set ->file_exists correctly");
        ok(readout->data_exists == false, "pmReadoutAlloc() set ->data_exists correctly");
        psFree(camera);
        psFree(fpa);
        psFree(chip);
        psFree(cell);
        psFree(readout);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Populate the pmReadout struct with real data to ensure they were
    // psFree()'ed correctly.
    {
        psMemId id = psMemGetId();
        pmReadout *readout = generateSimpleReadout(NULL);
        psFree(readout);
        // XXX: The pmReadout->bias list is not being free'ed correctly.
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ------------------------------------------------------------------------
    // pmReadoutFreeData() tests
    // Call pmReadoutFreeData() with NULL pmFPA input parameter
    {
        psMemId id = psMemGetId();
        pmReadoutFreeData(NULL);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks (Call pmReadoutFreeData() with NULL pmFPA input parameter)");
    }


    // Call pmReadoutFreeData() with acceptable input parameters
    {
        psMemId id = psMemGetId();
        pmReadout *readout = generateSimpleReadout(NULL);
        pmReadoutFreeData(readout);
        ok(readout->image == NULL, "pmReadoutFreeData() correctly set readout->image to NULL");
        ok(readout->variance == NULL, "pmReadoutFreeData() correctly set readout->weight to NULL");
        ok(readout->mask == NULL, "pmReadoutFreeData() correctly set readout->mask to NULL");
        psFree(readout);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ------------------------------------------------------------------------
    // pmFPACheckParents() tests
    // psBool pmFPACheckParents(pmFPA *fpa)
    // Call pmFPACheckParents() with NULL pmFPA input parameter
    {
        psMemId id = psMemGetId();
        bool rc = pmFPACheckParents(NULL);
        ok(rc == false, "pmFPACheckParents() returned FALSE with NULL pmFPA input parameter");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // pmFPACheckParents() tests
    // Call pmFPACheckParents() with acceptable input parameters
    {
        psMemId id = psMemGetId();
        pmFPA* fpa = generateSimpleFPA(NULL);
        bool rc = pmFPACheckParents(fpa);
        ok(rc == true, "pmFPACheckParents() returned FALSE with acceptable input parameters");
        psFree(fpa);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

}
