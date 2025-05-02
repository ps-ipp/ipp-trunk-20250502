    /** @file  tst_pmAstrometry.c
     *
 *  @brief Contains the tests: pmAstrometry.[ch].  The pmxxxAlloc()
 *  and psFree() functionality are used here.
 *
 *  @author GLG, MHPCC
 *
 *  XXX: Untested: pmFPACheckParents()
 *  XXX: Create the pmHDU alloc/free function, test them here
 *
 *  @version $Revision: 1.2 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-01-29 19:58:01 $
 *
 *  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 */
#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include <psmodules.h>
#include "tap.h"
#include "pstap.h"

#define CHIP_ALLOC_NAME "ChipName"
#define CELL_ALLOC_NAME "CellName"
#define MISC_NUM 32
#define MISC_NAME "META00"
#define MISC_NAME2 "META01"
#define NUM_BIAS_DATA 10
#define TEST_NUM_ROWS 32
#define TEST_NUM_COLS 32

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
generateSimpleFPA(): This function generates a pmFPA data structure and then
populates its members with real data.  We do this to ensure that the data is
later being psFree()'ed correctly.
 *****************************************************************************/
pmFPA* generateSimpleFPA(psMetadata *camera)
{
    pmFPA* fpa = pmFPAAlloc(camera, cameraName);
    fpa->fromTPA = PS_CREATE_2D_IDENTITY_PLANE_TRANSFORM();
    fpa->toTPA = PS_CREATE_2D_IDENTITY_PLANE_TRANSFORM();
    fpa->toSky = psProjectionAlloc(0.0,0.0,10.0,10.0,PS_PROJ_TAN);
    psMetadataAddS32(fpa->analysis, PS_LIST_HEAD, MISC_NAME, 0, NULL, MISC_NUM);
    psMetadataAddS32((psMetadata *) fpa->camera, PS_LIST_HEAD, MISC_NAME, 0, NULL, MISC_NUM);
    psMetadataAddS32(fpa->concepts, PS_LIST_HEAD, MISC_NAME, 0, NULL, MISC_NUM);
    fpa->hdu = pmHDUAlloc(NULL);
    return(fpa);
}


/******************************************************************************
testFPAAlloc()
    1: We ensure that pmFPAAlloc() properly allocates a pmFPA struct.
    2: We populate the members with real data to ensure they are being
       free'ed correctly.
 *****************************************************************************/
void testFPAAlloc(void)
{
    {
        psMemId id = psMemGetId();
        psMetadata *camera = psMetadataAlloc();
        pmFPA* fpa = pmFPAAlloc(camera);
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
        ok(fpa->wrote_phu == false, "pmFPAAlloc() set ->wrote_phu to FALSE");
        psFree(fpa);
        psFree(camera);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Populate the pmFPA struct with real data to ensure they were
    // psFree()'ed correctly.
    {
        psMemId id = psMemGetId();
        pmFPA* fpa = generateSimpleFPA(NULL);
        psFree(fpa);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
}

/******************************************************************************
generateSimpleChip(): This function generates a pmChip data structure and then
populates its members with real data.  We do this to ensure that the data is
later being psFree()'ed correctly.
 *****************************************************************************/
pmChip *generateSimpleChip(pmFPA *fpa)
{
    pmChip *chip = pmChipAlloc(fpa, CHIP_ALLOC_NAME);
    chip->toFPA = PS_CREATE_2D_IDENTITY_PLANE_TRANSFORM();
    chip->fromFPA = PS_CREATE_2D_IDENTITY_PLANE_TRANSFORM();
    psMetadataAddS32(chip->analysis, PS_LIST_HEAD, MISC_NAME, 0, NULL, MISC_NUM);
    psMetadataAddS32(chip->concepts, PS_LIST_HEAD, MISC_NAME, 0, NULL, MISC_NUM);
    chip->hdu = pmHDUAlloc(NULL);
    return(chip);
}

void testChipAlloc(void)
{
    {
        psMemId id = psMemGetId();
        psMetadata *camera = psMetadataAlloc();
        pmFPA* fpa = generateSimpleFPA(camera);
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
        ok(chip->wrote_phu == false, "pmChipAlloc() set ->wrote_phu correctly");
        psFree(camera);
        psFree(fpa);
        psFree(chip);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Populate the pmChip struct with real data to ensure they were
    // psFree()'ed correctly.
    {
        psMemId id = psMemGetId();
        pmChip *chip = generateSimpleChip(NULL);
        psFree(chip);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
}

/******************************************************************************
generateSimpleCell(): This function generates a pmCell data structure and then
populates its members with real data.  We do this to ensure that the data is
later being psFree()'ed correctly.
 *****************************************************************************/
pmCell *generateSimpleCell(pmFPA *fpa, pmChip *chip)
{
    pmCell *cell = pmCellAlloc(chip, CELL_ALLOC_NAME);
    psMetadataAddS32(cell->analysis, PS_LIST_HEAD, MISC_NAME, 0, NULL, MISC_NUM);
    cell->hdu = pmHDUAlloc(NULL);
    psMetadataAddS32(cell->concepts, PS_LIST_HEAD, MISC_NAME, 0, NULL, MISC_NUM);
    return(cell);
}

void testCellAlloc(void)
{
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
        ok(cell->wrote_phu == false, "pmCellAlloc() set ->wrote_phu correctly");
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
        pmCell *cell = generateSimpleCell(NULL, NULL);
        psFree(cell);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // test pmCellFreeReadouts()
    {
        psMemId id = psMemGetId();
        pmCell *cell = generateSimpleCell(NULL, NULL);
        pmCellFreeReadouts(cell);
        ok(cell->readouts->n == 0, "pmCellFreeReadouts() correctly set cell->readouts->n to 0");
        psFree(cell);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks (pmCellFreeReadouts)");
    }

}

/******************************************************************************
generateSimpleReadout(): This function generates a pmReadout data structure and then
populates its members with real data.  We do this to ensure that the data is
later being psFree()'ed correctly.
 *****************************************************************************/
pmReadout *generateSimpleReadout(pmCell *cell)
{
    pmReadout *readout = pmReadoutAlloc(cell);
    readout->image = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_F32);
    readout->mask = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_U8);
    readout->weight = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_F32);
    for (psS32 i = 0 ; i < NUM_BIAS_DATA ; i++) {
        psImage *tmpImage = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_F32);
        psListAdd(readout->bias, PS_LIST_HEAD, tmpImage);
    }
    psMetadataAddS32(readout->analysis, PS_LIST_HEAD, MISC_NAME, 0, NULL, MISC_NUM);
    return(readout);
}


void testReadoutAlloc(void)
{
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
        ok(readout->weight == NULL, "pmReadoutAlloc() set ->weight correctly");
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

/*
    {
        psMemId id = psMemGetId();
        pmReadout *readout = pmReadoutAlloc(NULL);
        readout->image = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_F32);
        readout->mask = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_U8);
        readout->weight = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_F32);
        for (psS32 i = 0 ; i < NUM_BIAS_DATA ; i++) {
            psImage *tmpImage = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_F32);
            psListAdd(readout->bias, PS_LIST_HEAD, tmpImage);
        }
        psMetadataAddS32(readout->analysis, PS_LIST_HEAD, MISC_NAME, 0, NULL, MISC_NUM);
        return(readout);
    }
*/
}

psS32 main(psS32 argc, char* argv[])
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    plan_tests(56);

    testFPAAlloc();
    testChipAlloc();
    testCellAlloc();
    testReadoutAlloc();
}
