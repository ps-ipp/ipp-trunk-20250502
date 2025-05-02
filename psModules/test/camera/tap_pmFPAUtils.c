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
    bool rc = pmConfigFileRead(&cell->hdu->format, "dataFiles/camera0/format0.config", "Camera format 0");
    if (!rc) {
        rc = pmConfigFileRead(&cell->hdu->format, "../dataFiles/camera0/format0.config", "Camera format 0");
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
    // You shouldn't have to remove the key from the metadata.
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
    plan_tests(40);


    // ----------------------------------------------------------------------
    // pmFPAFindChip() tests
    // Call pmFPAFindChip() with NULL pmFPA input parameter
    {
        psMemId id = psMemGetId();
        int rc = pmFPAFindChip(NULL, "chip-0");
        ok(-1 == rc, "pmFPAFindChip() returns -1 with NULL pmFPA input parameter");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmFPAFindChip() with bogus chip name
    {
        psMemId id = psMemGetId();
        pmFPA* fpa = generateSimpleFPA(NULL);
        int rc = pmFPAFindChip(fpa, "bogus");
        ok(-1 == rc, "pmFPAFindChip() returns -1 with bogus chip name");
        psFree(fpa);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmFPAFindChip() with NULL chip name
    {
        psMemId id = psMemGetId();
        // Generate the pmFPA heirarchy
        pmFPA* fpa = generateSimpleFPA(NULL);
        ok(fpa != NULL, "Allocated a pmFPA successfully");
        int rc = pmFPAFindChip(fpa, NULL);
        ok(-1 == rc, "pmFPAFindChip() returns -1 with NULL name input param");
        psFree(fpa);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmFPAFindChip() with acceptable input parameters
    {
        psMemId id = psMemGetId();
        // Generate the pmFPA heirarchy
        pmFPA* fpa = generateSimpleFPA(NULL);
        ok(fpa != NULL, "Allocated a pmFPA successfully");
        // Set unique chip names
        for (int chipID = 0 ; chipID < fpa->chips->n ; chipID++) {
            pmChip *chip = fpa->chips->data[chipID];
            char tmpName[100];
            sprintf(tmpName, "chip-%d", chipID);
            psMetadataAddStr(chip->concepts, PS_LIST_TAIL, "CHIP.NAME", PS_META_REPLACE, NULL, tmpName);
	}

        // Verify that pmFPAFindChip() finds those chips correctly
        for (int chipID = 0 ; chipID < fpa->chips->n ; chipID++) {
            char tmpName[100];
            sprintf(tmpName, "chip-%d", chipID);
            int rc = pmFPAFindChip(fpa, tmpName);
            ok(rc == chipID, "pmFPAFindChip() correctly found chip %s (%d)", tmpName, rc);
	}

        psFree(fpa);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }



    // ----------------------------------------------------------------------
    // pmChipFindCell() tests
    // Call pmChipFindCell() with bogus cell name
    {
        psMemId id = psMemGetId();
        int rc = pmChipFindCell(NULL, "cell-0");
        ok(-1 == rc, "pmChipFindCell() returns -1 with NULL pmChip input param");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Call pmChipFindCell() with bogus cell name
    {
        psMemId id = psMemGetId();
        pmFPA* fpa = generateSimpleFPA(NULL);
        pmChip *chip = fpa->chips->data[0];
        int rc = pmChipFindCell(chip, "bogus");
        ok(-1 == rc, "pmChipFindCell() returns -1 with bogus cell name");
        psFree(fpa);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmChipFindCell() with NULL cell name
    {
        psMemId id = psMemGetId();
        // Generate the pmChip heirarchy
        pmFPA* fpa = generateSimpleFPA(NULL);
        pmChip *chip = fpa->chips->data[0];
        pmCell *cell = chip->cells->data[0];
        ok(fpa != NULL, "Allocated a pmFPA successfully");
        ok(chip != NULL, "Allocated a pmChip successfully");
        ok(cell != NULL, "Allocated a pmCell successfully");
        int rc = pmChipFindCell(chip, NULL);
        ok(-1 == rc, "pmChipFindCell() returns -1 with NULL name input param");
        psFree(fpa);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmChipFindCell() with acceptable input parameters
    {
        psMemId id = psMemGetId();
        // Generate the pmChip heirarchy
        pmFPA* fpa = generateSimpleFPA(NULL);
        pmChip *chip = fpa->chips->data[0];
        pmCell *cell = chip->cells->data[0];
        ok(fpa != NULL, "Allocated a pmFPA successfully");
        ok(chip != NULL, "Allocated a pmChip successfully");
        ok(cell != NULL, "Allocated a pmCell successfully");
        // Set unique cell names
        for (int cellID = 0 ; cellID < chip->cells->n ; cellID++) {
            pmCell *cell = chip->cells->data[cellID];
            char tmpName[100];
            sprintf(tmpName, "cell-%d", cellID);
            psMetadataAddStr(cell->concepts, PS_LIST_TAIL, "CELL.NAME", PS_META_REPLACE, NULL, tmpName);
	}

        // Verify that pmChipFindCell() finds those cells correctly
        for (int cellID = 0 ; cellID < chip->cells->n ; cellID++) {
            char tmpName[100];
            sprintf(tmpName, "cell-%d", cellID);
            int rc = pmChipFindCell(chip, tmpName);
            ok(rc == cellID, "pmChipFindCell() correctly found cell %s (%d)", tmpName, rc);
	}

        psFree(fpa);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

}
