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
 *  @version $Revision: 1.6 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2006-01-26 21:10:51 $
 *
 *  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 */

#include "config.h"
#include <math.h>
#include <string.h>
#include "psTest.h"
#include "pslib_strict.h"
#include "pmAstrometry.h"

static psS32 testFPAAlloc(void);
static psS32 testChipAlloc(void);
static psS32 testCellAlloc(void);
static psS32 testReadoutAlloc(void);

testDescription tests[] = {
                              {testFPAAlloc,739,"pmFPAAlloc",0,false},
                              {testChipAlloc,740,"pmChipAlloc",0,false},
                              {testCellAlloc,741,"pmCellAlloc",0,false},
                              {testReadoutAlloc,742,"pmReadoutAlloc",0,false},
                              {NULL}
                          };

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
pmFPA *generateSimpleFPA()
{
    psBool rc;
    pmFPA* fpa = pmFPAAlloc(psMetadataAlloc());

    if (fpa == NULL) {
        psLogMsg(__func__,PS_LOG_ERROR, "TEST ERROR: pmFPAAlloc() returned a NULL.");
        return(NULL);
    }

    //
    // Test and create camera metadata.
    //
    if (fpa->camera == NULL) {
        psLogMsg(__func__, PS_LOG_ERROR, "TEST ERROR: fpa->camera is NULL.");
        psFree(fpa);
        return(NULL);
    } else {
        rc = psMetadataAddS32((psMetadata *) fpa->camera, PS_LIST_HEAD, MISC_NAME, 0, NULL, MISC_NUM);
        if (rc == false) {
            psLogMsg(__func__,PS_LOG_ERROR, "TEST ERROR: could not add metadata to fpa->camera.");
            psFree(fpa);
            return(NULL);
        }
        psS32 tmpS32 = psMetadataLookupS32(&rc, fpa->camera, MISC_NAME);
        if ((rc == false) || (tmpS32 != MISC_NUM)) {
            psLogMsg(__func__,PS_LOG_ERROR, "TEST ERROR: could not read metadata from fpa->camera.");
            psFree(fpa);
            return(NULL);
        }
    }

    //
    // Create various transforms and projections.
    //
    fpa->fromTangentPlane = PS_CREATE_4D_IDENTITY_PLANE_DISTORT();
    fpa->toTangentPlane = PS_CREATE_4D_IDENTITY_PLANE_DISTORT();
    fpa->projection = psProjectionAlloc(0.0,0.0,10.0,10.0,PS_PROJ_TAN);

    //
    // Ensure fpa concepts metadata was allocated properly.
    //
    if (fpa->concepts == NULL) {
        psLogMsg(__func__,PS_LOG_ERROR, "TEST ERROR: pmFPAAlloc did not set fpa->concepts.");
        psFree(fpa);
        return(NULL);
    } else {
        rc = psMetadataAddS32(fpa->concepts, PS_LIST_HEAD, MISC_NAME, 0, NULL, MISC_NUM);
        if (rc == false) {
            psLogMsg(__func__,PS_LOG_ERROR, "TEST ERROR: could not add data to fpa->concepts.");
            psFree(fpa);
            return(NULL);
        }
        psS32 tmpS32 = psMetadataLookupS32(&rc, fpa->concepts, MISC_NAME);
        if ((rc == false) || (tmpS32 != MISC_NUM)) {
            psLogMsg(__func__,PS_LOG_ERROR, "TEST ERROR: could not read metadata from fpa->concepts.");
            psFree(fpa);
            return(NULL);
        }
    }

    //
    // Create ->analysis metadata.
    //
    fpa->analysis = psMetadataAlloc();
    rc = psMetadataAddS32(fpa->analysis, PS_LIST_HEAD, MISC_NAME, 0, NULL, MISC_NUM);

    //
    // We test the fpa->chips array later.
    //

    //
    // How to test the p_pmHDU *hdu member?
    //

    //
    // Create ->phu metadata.
    //
    fpa->phu = psMetadataAlloc();
    rc = psMetadataAddS32(fpa->phu, PS_LIST_HEAD, MISC_NAME, 0, NULL, MISC_NUM);

    return(fpa);
}


psS32 main(psS32 argc, char* argv[])
{
    psLogSetLevel(PS_LOG_INFO);
    psLogSetFormat("HLNM");

    return ! runTestSuite(stderr,"pmAstrometry",tests,argc,argv);
}

/******************************************************************************
testFPAAlloc()
    1: We ensure that pmFPAAlloc() properly allocates a pmFPA struct.
    2: We populate the members with real data to ensure they are being
       free'ed correctly.
 *****************************************************************************/
static psS32 testFPAAlloc(void)
{
    psMetadata *camera = psMetadataAlloc();
    pmFPA* fpa = pmFPAAlloc(camera);

    if (fpa == NULL) {
        psLogMsg(__func__,PS_LOG_ERROR, "TEST ERROR: pmFPAAlloc() returned a NULL.");
        return 1;
    }

    if (fpa->fromTangentPlane != NULL) {
        psLogMsg(__func__,PS_LOG_ERROR, "TEST ERROR: pmFPAAlloc did not set ->fromTangentPlane to NULL.");
        return 2;
    }

    if (fpa->toTangentPlane != NULL) {
        psLogMsg(__func__,PS_LOG_ERROR, "TEST ERROR: pmFPAAlloc did not set ->toTangentPlane to NULL.");
        return 3;
    }
    if (fpa->projection != NULL) {
        psLogMsg(__func__,PS_LOG_ERROR, "TEST ERROR: pmFPAAlloc did not set ->projection to NULL.");
        return 4;
    }

    if (fpa->concepts == NULL) {
        psLogMsg(__func__,PS_LOG_ERROR, "TEST ERROR: pmFPAAlloc did not set ->concepts.");
        return 5;
    }

    if (fpa->analysis != NULL) {
        psLogMsg(__func__,PS_LOG_ERROR, "TEST ERROR: pmFPAAlloc did not set ->analysis to NULL.");
        return 6;
    }

    if (fpa->camera != camera) {
        psLogMsg(__func__,PS_LOG_ERROR, "TEST ERROR: pmFPAAlloc did not set ->camera.");
        return 7;
    }

    if (fpa->chips == NULL) {
        psLogMsg(__func__,PS_LOG_ERROR, "TEST ERROR: pmFPAAlloc did not set ->chips.");
        return 8;
    }

    if (fpa->hdu != NULL) {
        psLogMsg(__func__,PS_LOG_ERROR, "TEST ERROR: pmFPAAlloc did not set ->hdu to NULL.");
        return 9;
    }
    psFree(fpa);

    //
    // Populate the pmFPA struct with real data to ensure they were
    // psFree()'ed correctly.
    //
    fpa = generateSimpleFPA();
    if (fpa == NULL) {
        psLogMsg(__func__,PS_LOG_ERROR, "TEST ERROR: generateSimpleFPA() returned NULL.");
        return(15);
    }
    psFree(fpa);

    return(0);
}

/******************************************************************************
generateSimpleChip(): This function generates a pmChip data structure and then
populates its members with real data.  We do this to ensure that the data is
later being psFree()'ed correctly.
 *****************************************************************************/
pmChip *generateSimpleChip(pmFPA *fpa)
{
    psBool rc;
    pmChip *chip = pmChipAlloc(fpa, CHIP_ALLOC_NAME);
    if (chip == NULL) {
        psLogMsg(__func__,PS_LOG_ERROR, "TEST ERROR: pmChipAlloc returned a NULL.");
        return(NULL);
    }
    chip->toFPA = PS_CREATE_2D_IDENTITY_PLANE_TRANSFORM();
    chip->fromFPA = PS_CREATE_2D_IDENTITY_PLANE_TRANSFORM();
    //
    // We already ensured that chip->concepts was working properly.
    //

    //
    // Create ->analysis metadata.
    //
    chip->analysis = psMetadataAlloc();
    rc = psMetadataAddS32(chip->analysis, PS_LIST_HEAD, MISC_NAME, 0, NULL, MISC_NUM);

    //
    // We test the chip->cells array later.
    //

    //
    // How to test the p_pmHDU *hdu member?
    //

    return(chip);
}

static psS32 testChipAlloc(void)
{
    pmFPA* fpa = generateSimpleFPA();
    pmChip *chip = pmChipAlloc(fpa, CHIP_ALLOC_NAME);
    if (chip == NULL) {
        psLogMsg(__func__,PS_LOG_ERROR, "TEST ERROR: pmChipAlloc returned a NULL.");
        return 1;
    }

    if (chip->col0 != -1) {
        psLogMsg(__func__, PS_LOG_ERROR, "TEST ERROR: chip->col0 set improperly.\n");
        return 5;
    }

    if (chip->row0 != -1) {
        psLogMsg(__func__, PS_LOG_ERROR, "TEST ERROR: chip->row0 set improperly.\n");
        return 6;
    }

    if (chip->toFPA != NULL) {
        psLogMsg(__func__, PS_LOG_ERROR, "TEST ERROR: chip->toChip set improperly.\n");
        return 7;
    }

    if (chip->fromFPA != NULL) {
        psLogMsg(__func__, PS_LOG_ERROR, "TEST ERROR: chip->toFPA set improperly.\n");
        return 8;
    }

    if (chip->concepts == NULL) {
        psLogMsg(__func__, PS_LOG_ERROR, "TEST ERROR: chip->concepts set improperly.\n");
        return 21;
    } else {
        psMetadataItem *tmpMeta = psMetadataLookup(chip->concepts, "CHIP.NAME");
        if (0 != strcmp((char *) tmpMeta->data.V, CHIP_ALLOC_NAME)) {
            psLogMsg(__func__, PS_LOG_ERROR, "TEST ERROR: The metadata was set improperly.\n");
            return (32);
        }
        // XXX: Code a test to ensure the metadata has the correct type
    }

    if (chip->analysis != NULL) {
        psLogMsg(__func__, PS_LOG_ERROR, "TEST ERROR: chip->analysis set improperly.\n");
        return 10;
    }

    if (chip->cells == NULL) {
        psLogMsg(__func__, PS_LOG_ERROR, "TEST ERROR: chip->cells set improperly.\n");
        return 22;
    }

    if (chip->parent != fpa) {
        psLogMsg(__func__, PS_LOG_ERROR, "TEST ERROR: chip->parent set improperly.\n");
        return 23;
    }

    if (chip->valid != false) {
        psLogMsg(__func__, PS_LOG_ERROR, "TEST ERROR: chip->valid set improperly.\n");
        return 24;
    }

    if (chip->hdu != NULL) {
        psLogMsg(__func__, PS_LOG_ERROR, "TEST ERROR: chip->hdu set improperly.\n");
        return 25;
    }
    psFree(fpa);

    //
    // Populate the pmChip struct with real data to ensure they were
    // psFree()'ed correctly.
    //
    fpa = generateSimpleFPA();
    chip = generateSimpleChip(fpa);
    psFree(fpa);

    return(0);
}

/******************************************************************************
generateSimpleCell(): This function generates a pmCell data structure and then
populates its members with real data.  We do this to ensure that the data is
later being psFree()'ed correctly.
 *****************************************************************************/
pmCell *generateSimpleCell(pmFPA *fpa, pmChip *chip)
{
    psBool rc;
    pmCell *cell = pmCellAlloc(chip, (psMetadata *) fpa->camera, CELL_ALLOC_NAME);
    if (cell == NULL) {
        psLogMsg(__func__,PS_LOG_ERROR, "TEST ERROR: pmCellAlloc returned a NULL.");
        return(NULL);
    }
    cell->toChip = PS_CREATE_2D_IDENTITY_PLANE_TRANSFORM();
    cell->toFPA = PS_CREATE_2D_IDENTITY_PLANE_TRANSFORM();
    cell->toSky = PS_CREATE_2D_IDENTITY_PLANE_TRANSFORM();
    //
    // We already ensured that cell->concepts was working properly.
    //

    //
    // Test camera metadata.
    //
    if (cell->camera == NULL) {
        psLogMsg(__func__, PS_LOG_ERROR, "TEST ERROR: cell->camera is NULL.");
        psFree(fpa);
        return(NULL);
    } else {
        rc = psMetadataAddS32((psMetadata *) cell->camera, PS_LIST_HEAD, MISC_NAME2, 0, NULL, MISC_NUM);
        if (rc == false) {
            psLogMsg(__func__,PS_LOG_ERROR, "TEST ERROR: could not add metadata to cell->camera.");
            psFree(fpa);
            return(NULL);
        }
        psS32 tmpS32 = psMetadataLookupS32(&rc, cell->camera, MISC_NAME2);
        if ((rc == false) || (tmpS32 != MISC_NUM)) {
            psLogMsg(__func__,PS_LOG_ERROR, "TEST ERROR: could not read metadata from cell->camera.");
            psFree(fpa);
            return(NULL);
        }
    }

    //
    // Create ->analysis metadata.
    //
    cell->analysis = psMetadataAlloc();
    rc = psMetadataAddS32(cell->analysis, PS_LIST_HEAD, MISC_NAME, 0, NULL, MISC_NUM);

    //
    // We test the cell->readouts array later.
    //

    //
    // How to test the p_pmHDU *hdu member?
    //

    return(cell);
}

static psS32 testCellAlloc(void)
{
    pmFPA* fpa = generateSimpleFPA();
    pmChip *chip = pmChipAlloc(fpa, CHIP_ALLOC_NAME);
    pmCell *cell = pmCellAlloc(chip, (psMetadata *) fpa->camera, CELL_ALLOC_NAME);
    if (cell == NULL) {
        psLogMsg(__func__,PS_LOG_ERROR, "TEST ERROR: pmCellAlloc returned a NULL.n");
        return 3;
    }

    if (cell->col0 != -1) {
        psLogMsg(__func__, PS_LOG_ERROR, "TEST ERROR: cell->col0 set improperly.\n");
        return 5;
    }

    if (cell->row0 != -1) {
        psLogMsg(__func__, PS_LOG_ERROR, "TEST ERROR: cell->row0 set improperly.\n");
        return 6;
    }

    if (cell->toChip != NULL) {
        psLogMsg(__func__, PS_LOG_ERROR, "TEST ERROR: cell->toChip set improperly.\n");
        return 7;
    }

    if (cell->toFPA != NULL) {
        psLogMsg(__func__, PS_LOG_ERROR, "TEST ERROR: cell->toFPA set improperly.\n");
        return 8;
    }

    if (cell->toSky != NULL) {
        psLogMsg(__func__, PS_LOG_ERROR, "TEST ERROR: cell->toSky set improperly.\n");
        return 9;
    }

    if (cell->concepts == NULL) {
        psLogMsg(__func__, PS_LOG_ERROR, "TEST ERROR: cell->concepts set improperly.\n");
        return 21;
    } else {
        psMetadataItem *tmpMeta = psMetadataLookup(cell->concepts, "CELL.NAME");
        if (0 != strcmp((char *) tmpMeta->data.V, CELL_ALLOC_NAME)) {
            psLogMsg(__func__, PS_LOG_ERROR, "TEST ERROR: The metadata was set improperly.\n");
            return (32);
        }
        // XXX: Code a test to ensure the metadata has the correct type
    }

    if (cell->camera != fpa->camera) {
        psLogMsg(__func__, PS_LOG_ERROR, "TEST ERROR: cell->camera set improperly.\n");
        return 20;
    }

    if (cell->analysis != NULL) {
        psLogMsg(__func__, PS_LOG_ERROR, "TEST ERROR: cell->analysis set improperly.\n");
        return 10;
    }

    if (cell->readouts == NULL) {
        psLogMsg(__func__, PS_LOG_ERROR, "TEST ERROR: cell->readouts set improperly.\n");
        return 22;
    }

    if (cell->parent != chip) {
        psLogMsg(__func__, PS_LOG_ERROR, "TEST ERROR: cell->parent set improperly.\n");
        return 23;
    }

    if (cell->valid != false) {
        psLogMsg(__func__, PS_LOG_ERROR, "TEST ERROR: cell->valid set improperly.\n");
        return 24;
    }

    if (cell->hdu != NULL) {
        psLogMsg(__func__, PS_LOG_ERROR, "TEST ERROR: cell->hdu set improperly.\n");
        return 27;
    }
    psFree(fpa);

    //
    // Populate the pmCell struct with real data to ensure they were
    // psFree()'ed correctly.
    //
    fpa = generateSimpleFPA();
    chip = generateSimpleChip(fpa);
    cell = generateSimpleCell(fpa, chip);
    psFree(fpa);

    return(0);
}

/******************************************************************************
generateSimpleReadout(): This function generates a pmReadout data structure and then
populates its members with real data.  We do this to ensure that the data is
later being psFree()'ed correctly.
 *****************************************************************************/
pmReadout *generateSimpleReadout(pmFPA *fpa, pmChip *chip, pmCell *cell)
{
    psBool rc;
    pmReadout *readout = pmReadoutAlloc(cell);
    if (readout == NULL) {
        psLogMsg(__func__,PS_LOG_ERROR, "TEST ERROR: pmReadoutAlloc returned a NULL.");
        return(NULL);
    }
    readout->image = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_F32);
    readout->mask = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_U8);
    readout->weight = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_F32);

    //
    // Create a psList of bias data.
    //
    for (psS32 i = 0 ; i < NUM_BIAS_DATA ; i++) {
        psImage *tmpImage = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_F32);
        if (readout->bias == NULL) {
            readout->bias = psListAlloc(tmpImage);
        } else {
            psListAdd(readout->bias, PS_LIST_HEAD, tmpImage);
        }
    }

    //
    // Test readout->analysis metadata.
    //
    if (readout->analysis == NULL) {
        psLogMsg(__func__, PS_LOG_ERROR, "TEST ERROR: readout->analysis is NULL.");
        psFree(fpa);
        return(NULL);
    } else {
        rc = psMetadataAddS32((psMetadata *) readout->analysis, PS_LIST_HEAD, MISC_NAME, 0, NULL, MISC_NUM);
        if (rc == false) {
            psLogMsg(__func__,PS_LOG_ERROR, "TEST ERROR: could not add metadata to readout->analysis.");
            psFree(fpa);
            return(NULL);
        }
    }

    return(readout);
}


static psS32 testReadoutAlloc(void)
{
    pmFPA* fpa = generateSimpleFPA();
    pmChip *chip = pmChipAlloc(fpa, CHIP_ALLOC_NAME);
    pmCell *cell = pmCellAlloc(chip, (psMetadata *) fpa->camera, CELL_ALLOC_NAME);
    pmReadout *readout = pmReadoutAlloc(cell);
    if (readout == NULL) {
        psLogMsg(__func__,PS_LOG_ERROR, "TEST ERROR: pmReadoutAlloc returned a NULL.\n");
        return 4;
    }

    if (readout->col0 != -1) {
        psLogMsg(__func__, PS_LOG_ERROR, "TEST ERROR: pmReadout->col0 set improperly.\n");
        return 5;
    }

    if (readout->row0 != -1) {
        psLogMsg(__func__, PS_LOG_ERROR, "TEST ERROR: pmReadout->row0 set improperly.\n");
        return 6;
    }

    if (readout->colBins != -1) {
        psLogMsg(__func__, PS_LOG_ERROR, "TEST ERROR: pmReadout->colBins set improperly.\n");
        return 7;
    }

    if (readout->rowBins != -1) {
        psLogMsg(__func__, PS_LOG_ERROR, "TEST ERROR: pmReadout->colBins set improperly.\n");
        return 8;
    }

    if (readout->image != NULL) {
        psLogMsg(__func__, PS_LOG_ERROR, "TEST ERROR: pmReadout->image set improperly.\n");
        return 10;
    }

    if (readout->mask != NULL) {
        psLogMsg(__func__, PS_LOG_ERROR, "TEST ERROR: pmReadout->mask set improperly.\n");
        return 12;
    }

    if (readout->weight != NULL) {
        psLogMsg(__func__, PS_LOG_ERROR, "TEST ERROR: pmReadout->weight set improperly.\n");
        return 14;
    }

    if (readout->bias != NULL) {
        psLogMsg(__func__, PS_LOG_ERROR, "TEST ERROR: pmReadout->bias set improperly.\n");
        return 16;
    }

    if (readout->analysis == NULL) {
        psLogMsg(__func__, PS_LOG_ERROR, "TEST ERROR: pmReadout->analysis set improperly.\n");
        return 18;
    }

    if (readout->parent != cell) {
        psLogMsg(__func__, PS_LOG_ERROR, "TEST ERROR: pmReadout->parent set improperly.\n");
        return 20;
    }
    psFree(fpa);

    //
    // Populate the pmReadout struct with real data to ensure they were
    // psFree()'ed correctly.
    //
    fpa = generateSimpleFPA();
    chip = generateSimpleChip(fpa);
    cell = generateSimpleCell(fpa, chip);
    readout = generateSimpleReadout(fpa, chip, cell);
    psFree(fpa);

    return(0);
}
