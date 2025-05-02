#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include <psmodules.h>
#include "tap.h"
#include "pstap.h"
/* STATUS
    TESTED:
        pmFPASetFileStatus()
        pmChipSetFileStatus()
        pmCellSetFileStatus()

        pmFPACheckFileStatus()
        pmChipCheckFileStatus()
        pmCellCheckFileStatus()

        pmFPASetDataStatus()
        pmChipSetDataStatus()
        pmCellSetDataStatus()

        pmFPACheckDataStatus()
        pmChipCheckDataStatus()
        pmCellCheckDataStatus()
        pmReadoutCheckDataStatus()
    MUST TEST:
        pmFPAviewCheckDataStatus()
        pmFPASelectChip()
        pmChipSelectCell()
        pmFPAExcludeChip()
        pmChipExcludeCell()
*/

// XXX: For the genSimpleFPA() code, add IDs to each function so that
// the values set in each chip-?cell-?hdu-?image are unique
// XXX: For the genSimpleFPA() code, write masks and weights as well
// XXX: Add in the associated CheckStatus tests

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
    psArrayRealloc(chip->cells, NUM_CELLS);
    for (int i = 0 ; i < NUM_CELLS ; i++) {
        chip->cells->data[i] = psMemDecrRefCounter((psPtr) generateSimpleCell(chip));
    }

    // XXX: Add code to initialize chip pmConcepts


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

    // XXX: Eventually, when you finish the pmConcepts tests, add full concept
    // reading code from wherever.
    pmConceptsBlankFPA(fpa);
    return(fpa);
}

void SetCellFileExists(pmCell *cell) {
    cell->file_exists = true;
    for (int i = 0 ; i < cell->readouts->n ; i++) {
        pmReadout *readout = cell->readouts->data[i];
        readout->file_exists = true;
    }
}

void SetChipFileExists(pmChip *chip) {
    chip->file_exists = true;
    for (int i = 0 ; i < chip->cells->n ; i++) {
        pmCell *cell = chip->cells->data[i];
        cell->file_exists = true;
        SetCellFileExists(cell);
    }
}

void SetFPAFileExists(pmFPA *fpa) {
    for (int i = 0 ; i < fpa->chips->n ; i++) {
        pmChip *chip = fpa->chips->data[i];
        chip->file_exists = true;
        SetChipFileExists(chip);
    }
}

void SetReadoutDataExists(pmReadout *readout) {
    readout->data_exists = true;
}

void SetCellDataExists(pmCell *cell) {
    cell->data_exists = true;
    for (int i = 0 ; i < cell->readouts->n ; i++) {
        pmReadout *readout = cell->readouts->data[i];
        readout->data_exists = true;
    }
}

void SetChipDataExists(pmChip *chip) {
    chip->data_exists = true;
    for (int i = 0 ; i < chip->cells->n ; i++) {
        pmCell *cell = chip->cells->data[i];
        cell->data_exists = true;
        SetCellDataExists(cell);
    }
}

void SetFPADataExists(pmFPA *fpa) {
    for (int i = 0 ; i < fpa->chips->n ; i++) {
        pmChip *chip = fpa->chips->data[i];
        chip->data_exists = true;
        SetChipDataExists(chip);
    }
}


psS32 main(psS32 argc, char* argv[])
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    psTraceSetLevel("err", ERR_TRACE_LEVEL);
    plan_tests(107);


    // ----------------------------------------------------------------------
    // pmFPASetFileStatus() tests: verify with NULL pmFPA param
    // bool pmFPASetFileStatus(pmFPA *fpa, bool status)
    {
        psMemId id = psMemGetId();
        bool rc = pmFPASetFileStatus(NULL, false);
        ok(!rc, "pmFPASetFileStatus() returned FALSE with NULL pmFPA param");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // pmFPASetFileStatus() tests: verify with acceptable data
    // bool pmFPASetFileStatus(pmFPA *fpa, bool status)
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

        // First, set all flags to FALSE
        bool correctStatus = false;
        bool rc = pmFPASetFileStatus(fpa, correctStatus);
        ok(rc, "pmFPASetFileStatus() returned successfully with acceptable input params");
        bool errorFlag = false;
        for (int k = 0 ; k < fpa->chips->n ; k++) {
            pmChip *chip = fpa->chips->data[k];
            for (int j = 0 ; j < chip->cells->n ; j++) {
                pmCell *cell = chip->cells->data[j];
                if (cell->file_exists != correctStatus) {
                    diag("TEST ERROR: pmFPASetFileStatus() failed to set file status for chip %d cell %d\n", k, j);
                    errorFlag = true;
                }
    
                for (int i = 0; i < cell->readouts->n; i++) {
                    pmReadout *readout = cell->readouts->data[i];
                    if (readout->file_exists != correctStatus) {
                        diag("TEST ERROR: pmFPASetFileStatus() failed to set file status for chip %d cell %d readout %d\n", k, j, i);
                        errorFlag = true;
                    }
                }
            }
        }
        ok(!errorFlag, "pmFPASetFileStatus() set file status in all cells to FALSE");

        // Second, set all flags to TRUE
        correctStatus = true;
        rc = pmFPASetFileStatus(fpa, correctStatus);
        ok(rc, "pmFPASetFileStatus() returned successfully with acceptable input params");
        errorFlag = false;
        for (int k = 0 ; k < fpa->chips->n ; k++) {
            pmChip *chip = fpa->chips->data[k];
            for (int j = 0 ; j < chip->cells->n ; j++) {
                pmCell *cell = chip->cells->data[j];
                if (cell->file_exists != correctStatus) {
                    diag("TEST ERROR: pmFPASetFileStatus() failed to set file status for chip %d cell %d\n", k, j);
                    errorFlag = true;
                }
    
                for (int i = 0; i < cell->readouts->n; i++) {
                    pmReadout *readout = cell->readouts->data[i];
                    if (readout->file_exists != correctStatus) {
                        diag("TEST ERROR: pmFPASetFileStatus() failed to set file status for chip %d cell %d readout %d\n", k, j, i);
                        errorFlag = true;
                    }
                }
            }
        }
        ok(!errorFlag, "pmFPASetFileStatus() set file status in all cells to TRUE");

        // Third, set all flags to FALSE
        correctStatus = false;
        rc = pmFPASetFileStatus(fpa, correctStatus);
        ok(rc, "pmFPASetFileStatus() returned successfully with acceptable input params");
        errorFlag = false;
        for (int k = 0 ; k < fpa->chips->n ; k++) {
            pmChip *chip = fpa->chips->data[k];
            for (int j = 0 ; j < chip->cells->n ; j++) {
                pmCell *cell = chip->cells->data[j];
                if (cell->file_exists != correctStatus) {
                    diag("TEST ERROR: pmFPASetFileStatus() failed to set file status for chip %d cell %d\n", k, j);
                    errorFlag = true;
                }
    
                for (int i = 0; i < cell->readouts->n; i++) {
                    pmReadout *readout = cell->readouts->data[i];
                    if (readout->file_exists != correctStatus) {
                        diag("TEST ERROR: pmFPASetFileStatus() failed to set file status for chip %d cell %d readout %d\n", k, j, i);
                        errorFlag = true;
                    }
                }
            }
        }
        ok(!errorFlag, "pmFPASetFileStatus() set file status in all cells to FALSE");

        psFree(fpa);
        psFree(camera);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // pmChipSetFileStatus() tests: verify with NULL pmChip param
    // bool pmChipSetFileStatus(pmChip *chip, bool status)
    {
        psMemId id = psMemGetId();
        bool rc = pmChipSetFileStatus(NULL, false);
        ok(!rc, "pmChipSetFileStatus() returned FALSE with NULL pmChip param");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // pmChipSetFileStatus() tests: verify with acceptable data
    // bool pmChipSetFileStatus(pmChip *chip, bool status)
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

        // First, set all flags to FALSE
        bool correctStatus = false;
        bool rc = pmChipSetFileStatus(chip, correctStatus);
        ok(rc, "pmChipSetFileStatus() returned successfully with acceptable input params");
        bool errorFlag = false;
        if (chip->file_exists != correctStatus) {
            diag("TEST ERROR: pmChipSetFileStatus() failed to set file status for chip param");
            errorFlag = true;
        }
        for (int j = 0 ; j < chip->cells->n ; j++) {
            pmCell *cell = chip->cells->data[j];
            if (cell->file_exists != correctStatus) {
                diag("TEST ERROR: pmChipSetFileStatus() failed to set file status for cell %d\n", j);
                errorFlag = true;
            }

            for (int i = 0; i < cell->readouts->n; i++) {
                pmReadout *readout = cell->readouts->data[i];
                if (readout->file_exists != correctStatus) {
                    diag("TEST ERROR: pmChipSetFileStatus() failed to set file status for cell %d readout %d\n", j, i);
                    errorFlag = true;
                }
            }
        }
        ok(!errorFlag, "pmChipSetFileStatus() set file status in all cells to FALSE");

        // Second, set all flags to TRUE
        correctStatus = true;
        rc = pmChipSetFileStatus(chip, correctStatus);
        ok(rc, "pmChipSetFileStatus() returned successfully with acceptable input params");
        errorFlag = false;
        if (chip->file_exists != correctStatus) {
            diag("TEST ERROR: pmChipSetFileStatus() failed to set file status for chip param");
            errorFlag = true;
        }
        for (int j = 0 ; j < chip->cells->n ; j++) {
            pmCell *cell = chip->cells->data[j];
            if (cell->file_exists != correctStatus) {
                diag("TEST ERROR: pmChipSetFileStatus() failed to set file status for cell %d\n", j);
                errorFlag = true;
            }

            for (int i = 0; i < cell->readouts->n; i++) {
                pmReadout *readout = cell->readouts->data[i];
                if (readout->file_exists != correctStatus) {
                    diag("TEST ERROR: pmChipSetFileStatus() failed to set file status for cell %d readout %d\n", j, i);
                    errorFlag = true;
                }
            }
        }
        ok(!errorFlag, "pmChipSetFileStatus() set file status in all cells to TRUE");

        // Third, set all flags to FALSE
        correctStatus = false;
        rc = pmChipSetFileStatus(chip, correctStatus);
        ok(rc, "pmChipSetFileStatus() returned successfully with acceptable input params");
        errorFlag = false;
        if (chip->file_exists != correctStatus) {
            diag("TEST ERROR: pmChipSetFileStatus() failed to set file status for chip param");
            errorFlag = true;
        }
        for (int j = 0 ; j < chip->cells->n ; j++) {
            pmCell *cell = chip->cells->data[j];
            if (cell->file_exists != correctStatus) {
                diag("TEST ERROR: pmChipSetFileStatus() failed to set file status for cell %d\n", j);
                errorFlag = true;
            }

            for (int i = 0; i < cell->readouts->n; i++) {
                pmReadout *readout = cell->readouts->data[i];
                if (readout->file_exists != correctStatus) {
                    diag("TEST ERROR: pmChipSetFileStatus() failed to set file status for cell %d readout %d\n", j, i);
                    errorFlag = true;
                }
            }
        }
        ok(!errorFlag, "pmChipSetFileStatus() set file status in all cells to FALSE");


        psFree(fpa);
        psFree(camera);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // pmCellSetFileStatus() tests: verify with NULL pmCell param
    {
        psMemId id = psMemGetId();
        bool rc = pmCellSetFileStatus(NULL, false);
        ok(!rc, "pmCellSetFileStatus() returned FALSE with NULL pmCell param");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // pmCellSetFileStatus() tests: verify with acceptable data
    // bool pmCellSetFileStatus(pmCell *cell, bool status)
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

        // First, set all flags to FALSE
        bool correctStatus = false;
        bool rc = pmCellSetFileStatus(cell, correctStatus);
        ok(rc, "pmCellSetFileStatus() returned successfully with acceptable input params");
        bool errorFlag = false;
        if (cell->file_exists != correctStatus) {
            diag("TEST ERROR: pmCellSetFileStatus() failed to set file status for cell param\n");
            errorFlag = true;
        }
        for (int i = 0; i < cell->readouts->n; i++) {

            pmReadout *readout = cell->readouts->data[i];
            if (readout->file_exists != correctStatus) {
                diag("TEST ERROR: pmCellSetFileStatus() failed to set file status for cell %d\n", i);
                errorFlag = true;
            }
        }
        ok(!errorFlag, "pmCellSetFileStatus() set file status in all cells to FALSE");

        // Second, set all flags to TRUE
        correctStatus = true;
        rc = pmCellSetFileStatus(cell, correctStatus);
        ok(rc, "pmCellSetFileStatus() returned successfully with acceptable input params");
        errorFlag = false;
        if (cell->file_exists != correctStatus) {
            diag("TEST ERROR: pmCellSetFileStatus() failed to set file status for cell param\n");
            errorFlag = true;
        }
        for (int i = 0; i < cell->readouts->n; i++) {
            pmReadout *readout = cell->readouts->data[i];
            if (readout->file_exists != correctStatus) {
                diag("TEST ERROR: pmCellSetFileStatus() failed to set file status for cell %d\n", i);
                errorFlag = true;
            }
        }
        ok(!errorFlag, "pmCellSetFileStatus() set file status in all cells to TRUE");

        // Third, set all flags to FALSE
        correctStatus = false;
        rc = pmCellSetFileStatus(cell, correctStatus);
        ok(rc, "pmCellSetFileStatus() returned successfully with acceptable input params");
        errorFlag = false;
        if (cell->file_exists != correctStatus) {
            diag("TEST ERROR: pmCellSetFileStatus() failed to set file status for cell param\n");
            errorFlag = true;
        }
        for (int i = 0; i < cell->readouts->n; i++) {
            pmReadout *readout = cell->readouts->data[i];
            if (readout->file_exists != correctStatus) {
                diag("TEST ERROR: pmCellSetFileStatus() failed to set file status for readout %d\n", i);
                errorFlag = true;
            }
        }
        ok(!errorFlag, "pmCellSetFileStatus() set file status in all cells to FALSE");

        psFree(fpa);
        psFree(camera);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // pmFPACheckFileStatus() tests
    // bool pmFPACheckFileStatus(const pmFPA *fpa)
    // Call with NULL pmFPA input parameter
    {
        psMemId id = psMemGetId();
        bool rc = pmFPACheckFileStatus(NULL);
        ok(rc == false, "pmFPACheckFileStatus() returned FALSE with NULL pmFPA input parameter");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call with acceptable input parameters
    {
        psMemId id = psMemGetId();
        pmFPA *fpa = generateSimpleFPA(NULL);
        bool rc = pmFPACheckFileStatus(fpa);
        ok(rc == false, "pmFPACheckFileStatus() returned FALSE with NULL pmFPA input parameter");
        SetFPAFileExists(fpa);
        rc = pmFPACheckFileStatus(fpa);
        ok(rc == true, "pmFPACheckFileStatus() returned TRUE with NULL pmFPA input parameter");
        psFree(fpa);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // pmChipCheckFileStatus() tests
    // Call with NULL pmChip input parameter
    {
        psMemId id = psMemGetId();
        bool rc = pmChipCheckFileStatus(NULL);
        ok(rc == false, "pmChipCheckFileStatus() returned FALSE with NULL pmChip input parameter");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call with acceptable input parameter
    {
        psMemId id = psMemGetId();
        pmChip *chip = generateSimpleChip(NULL);
        bool rc = pmChipCheckFileStatus(chip);
        ok(rc == false, "pmChipCheckFileStatus() returned FALSE with NULL pmChip input parameter");
        SetChipFileExists(chip);
        rc = pmChipCheckFileStatus(chip);
        ok(rc == true, "pmChipCheckFileStatus() returned TRUE with NULL pmChip input parameter");
        psFree(chip);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // pmCellCheckFileStatus() tests
    // bool pmCellCheckFileStatus(const pmCell *cell)
    // Call with NULL pmCell input parameter
    {
        psMemId id = psMemGetId();
        bool rc = pmCellCheckFileStatus(NULL);
        ok(rc == false, "pmCellCheckFileStatus() returned FALSE with NULL pmCell input parameter");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call with acceptable input parameters
    {
        psMemId id = psMemGetId();
        pmCell *cell = generateSimpleCell(NULL);
        bool rc = pmCellCheckFileStatus(cell);
        ok(rc == false, "pmCellCheckFileStatus() returned FALSE with acceptable input parameters");
        SetCellFileExists(cell);
        rc = pmCellCheckFileStatus(cell);
        ok(rc == true, "pmCellCheckFileStatus() returned TRUE with acceptable input parameters");
        psFree(cell);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // pmFPASetDataStatus() tests: verify with NULL pmFPA param
    {
        psMemId id = psMemGetId();
        bool rc = pmFPASetDataStatus(NULL, false);
        ok(!rc, "pmFPASetDataStatus() returned FALSE with NULL pmFPA param");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // pmFPASetDataStatus() tests: verify with acceptable data
    // bool pmFPASetDataStatus(pmFPA *fpa, bool status)
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

        // First, set all flags to FALSE
        bool correctStatus = false;
        bool rc = pmFPASetDataStatus(fpa, correctStatus);
        ok(rc, "pmFPASetDataStatus() returned successfully with acceptable input params");
        bool errorFlag = false;
        for (int k = 0 ; k < fpa->chips->n ; k++) {
            pmChip *chip = fpa->chips->data[k];
            for (int j = 0 ; j < chip->cells->n ; j++) {
                pmCell *cell = chip->cells->data[j];
                if (cell->data_exists != correctStatus) {
                    diag("TEST ERROR: pmFPASetDataStatus() failed to set file status for chip %d cell %d\n", k, j);
                    errorFlag = true;
                }
    
                for (int i = 0; i < cell->readouts->n; i++) {
                    pmReadout *readout = cell->readouts->data[i];
                    if (readout->data_exists != correctStatus) {
                        diag("TEST ERROR: pmFPASetDataStatus() failed to set file status for chip %d cell %d readout %d\n", k, j, i);
                        errorFlag = true;
                    }
                }
            }
        }
        ok(!errorFlag, "pmFPASetDataStatus() set file status in all cells to FALSE");

        // Second, set all flags to TRUE
        correctStatus = true;
        rc = pmFPASetDataStatus(fpa, correctStatus);
        ok(rc, "pmFPASetDataStatus() returned successfully with acceptable input params");
        errorFlag = false;
        for (int k = 0 ; k < fpa->chips->n ; k++) {
            pmChip *chip = fpa->chips->data[k];
            for (int j = 0 ; j < chip->cells->n ; j++) {
                pmCell *cell = chip->cells->data[j];
                if (cell->data_exists != correctStatus) {
                    diag("TEST ERROR: pmFPASetDataStatus() failed to set file status for chip %d cell %d\n", k, j);
                    errorFlag = true;
                }
    
                for (int i = 0; i < cell->readouts->n; i++) {
                    pmReadout *readout = cell->readouts->data[i];
                    if (readout->data_exists != correctStatus) {
                        diag("TEST ERROR: pmFPASetDataStatus() failed to set file status for chip %d cell %d readout %d\n", k, j, i);
                        errorFlag = true;
                    }
                }
            }
        }
        ok(!errorFlag, "pmFPASetDataStatus() set file status in all cells to TRUE");

        // Third, set all flags to FALSE
        correctStatus = false;
        rc = pmFPASetDataStatus(fpa, correctStatus);
        ok(rc, "pmFPASetDataStatus() returned successfully with acceptable input params");
        errorFlag = false;
        for (int k = 0 ; k < fpa->chips->n ; k++) {
            pmChip *chip = fpa->chips->data[k];
            for (int j = 0 ; j < chip->cells->n ; j++) {
                pmCell *cell = chip->cells->data[j];
                if (cell->data_exists != correctStatus) {
                    diag("TEST ERROR: pmFPASetDataStatus() failed to set file status for chip %d cell %d\n", k, j);
                    errorFlag = true;
                }
    
                for (int i = 0; i < cell->readouts->n; i++) {
                    pmReadout *readout = cell->readouts->data[i];
                    if (readout->data_exists != correctStatus) {
                        diag("TEST ERROR: pmFPASetDataStatus() failed to set file status for chip %d cell %d readout %d\n", k, j, i);
                        errorFlag = true;
                    }
                }
            }
        }
        ok(!errorFlag, "pmFPASetDataStatus() set file status in all cells to FALSE");

        psFree(fpa);
        psFree(camera);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // pmChipSetDataStatus() tests: verify with NULL pmChip param
    // bool pmChipSetDataStatus(pmChip *chip, bool status)
    {
        psMemId id = psMemGetId();
        bool rc = pmChipSetDataStatus(NULL, false);
        ok(!rc, "pmChipSetDataStatus() returned FALSE with NULL pmChip param");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // pmChipSetDataStatus() tests: verify with acceptable data
    // bool pmChipSetDataStatus(pmChip *chip, bool status)
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

        // First, set all flags to FALSE
        bool correctStatus = false;
        bool rc = pmChipSetDataStatus(chip, correctStatus);
        ok(rc, "pmChipSetDataStatus() returned successfully with acceptable input params");
        bool errorFlag = false;
        if (chip->data_exists != correctStatus) {
            diag("TEST ERROR (a): pmChipSetDataStatus() failed to set file status for chip param\n");
            errorFlag = true;
        }
        for (int j = 0 ; j < chip->cells->n ; j++) {
            pmCell *cell = chip->cells->data[j];
            if (cell->data_exists != correctStatus) {
                diag("TEST ERROR (b): pmChipSetDataStatus() failed to set file status for cell %d\n", j);
                errorFlag = true;
            }

            for (int i = 0; i < cell->readouts->n; i++) {
                pmReadout *readout = cell->readouts->data[i];
                if (readout->data_exists != correctStatus) {
                    diag("TEST ERROR (c): pmChipSetDataStatus() failed to set file status for cell %d readout %d\n", j, i);
                    errorFlag = true;
                }
            }
        }
        ok(!errorFlag, "pmChipSetDataStatus() set data status in all cells to FALSE");

        // Second, set all flags to TRUE
        correctStatus = true;
        rc = pmChipSetDataStatus(chip, correctStatus);
        ok(rc, "pmChipSetDataStatus() returned successfully with acceptable input params");
        errorFlag = false;
        if (chip->data_exists != correctStatus) {
            diag("TEST ERROR (a): pmChipSetDataStatus() failed to set file status for chip param\n");
            errorFlag = true;
        }
        for (int j = 0 ; j < chip->cells->n ; j++) {
            pmCell *cell = chip->cells->data[j];
            if (cell->data_exists != correctStatus) {
                diag("TEST ERROR (b): pmChipSetDataStatus() failed to set file status for cell %d\n", j);
                errorFlag = true;
            }

            for (int i = 0; i < cell->readouts->n; i++) {
                pmReadout *readout = cell->readouts->data[i];
                if (readout->data_exists != correctStatus) {
                    diag("TEST ERROR (c): pmChipSetDataStatus() failed to set file status for cell %d readout %d\n", j, i);
                    errorFlag = true;
                }
            }
        }
        ok(!errorFlag, "pmChipSetDataStatus() set data status in all cells to TRUE");

        // ThirdSecond, set all flags to FALSE
        correctStatus = false;
        rc = pmChipSetDataStatus(chip, correctStatus);
        ok(rc, "pmChipSetDataStatus() returned successfully with acceptable input params");
        errorFlag = false;
        if (chip->data_exists != correctStatus) {
            diag("TEST ERROR (a): pmChipSetDataStatus() failed to set file status for chip param\n");
            errorFlag = true;
        }
        for (int j = 0 ; j < chip->cells->n ; j++) {
            pmCell *cell = chip->cells->data[j];
            if (cell->data_exists != correctStatus) {
                diag("TEST ERROR (b): pmChipSetDataStatus() failed to set file status for cell %d\n", j);
                errorFlag = true;
            }

            for (int i = 0; i < cell->readouts->n; i++) {
                pmReadout *readout = cell->readouts->data[i];
                if (readout->data_exists != correctStatus) {
                    diag("TEST ERROR (c): pmChipSetDataStatus() failed to set file status for cell %d readout %d\n", j, i);
                    errorFlag = true;
                }
            }
        }
        ok(!errorFlag, "pmChipSetDataStatus() set data status in all cells to FALSE");


        psFree(fpa);
        psFree(camera);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // pmCellSetDataStatus() tests: verify with NULL pmCell param
    // bool pmCellSetDataStatus(pmCell *cell, bool status)
    {
        psMemId id = psMemGetId();
        bool rc = pmCellSetDataStatus(NULL, false);
        ok(!rc, "pmCellSetDataStatus() returned FALSE with NULL pmCell param");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // pmCellSetDataStatus() tests: verify with acceptable data
    // bool pmCellSetDataStatus(pmCell *cell, bool status)
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

        // First, set all flags to FALSE
        bool correctStatus = false;
        bool rc = pmCellSetDataStatus(cell, correctStatus);
        ok(rc, "pmCellSetDataStatus() returned successfully with acceptable input params");
        bool errorFlag = false;
        if (cell->data_exists != correctStatus) {
            diag("TEST ERROR: pmCellSetDataStatus() failed to set file status for cell param\n");
            errorFlag = true;
        }
        for (int i = 0; i < cell->readouts->n; i++) {
            pmReadout *readout = cell->readouts->data[i];
            if (readout->data_exists != correctStatus) {
                diag("TEST ERROR: pmCellSetDataStatus() failed to set file status for cell %d\n", i);
                errorFlag = true;
            }
        }
        ok(!errorFlag, "pmCellSetDataStatus() set data status in all cells to FALSE");

        // Second, set all flags to TRUE
        correctStatus = true;
        rc = pmCellSetDataStatus(cell, correctStatus);
        ok(rc, "pmCellSetDataStatus() returned successfully with acceptable input params");
        errorFlag = false;
        if (cell->data_exists != correctStatus) {
            diag("TEST ERROR: pmCellSetDataStatus() failed to set file status for cell param\n");
            errorFlag = true;
        }
        for (int i = 0; i < cell->readouts->n; i++) {
            pmReadout *readout = cell->readouts->data[i];
            if (readout->data_exists != correctStatus) {
                diag("TEST ERROR: pmCellSetDataStatus() failed to set file status for cell %d\n", i);
                errorFlag = true;
            }
        }
        ok(!errorFlag, "pmCellSetDataStatus() set data status in all cells to TRUE");

        // Third, set all flags to FALSE
        correctStatus = false;
        rc = pmCellSetDataStatus(cell, correctStatus);
        ok(rc, "pmCellSetDataStatus() returned successfully with acceptable input params");
        errorFlag = false;
        if (cell->data_exists != correctStatus) {
            diag("TEST ERROR: pmCellSetDataStatus() failed to set file status for cell param\n");
            errorFlag = true;
        }
        for (int i = 0; i < cell->readouts->n; i++) {
            pmReadout *readout = cell->readouts->data[i];
            if (readout->data_exists != correctStatus) {
                diag("TEST ERROR: pmCellSetDataStatus() failed to set file status for readout %d\n", i);
                errorFlag = true;
            }
        }
        ok(!errorFlag, "pmCellSetDataStatus() set data status in all cells to FALSE");

        psFree(fpa);
        psFree(camera);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // pmFPACheckDataStatus() tests
    // bool pmFPACheckDataStatus(const pmFPA *fpa)
    // Call with NULL pmFPA input parameter
    {
        psMemId id = psMemGetId();
        bool rc = pmFPACheckDataStatus(NULL);
        ok(rc == false, "pmFPACheckDataStatus() returned FALSE with NULL pmFPA input parameter");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call with acceptable input parameters
    {
        psMemId id = psMemGetId();
        pmFPA *fpa = generateSimpleFPA(NULL);
        bool rc = pmFPACheckDataStatus(fpa);
        ok(rc == false, "pmFPACheckDataStatus() returned FALSE with NULL pmFPA input parameter");
        SetFPADataExists(fpa);
        rc = pmFPACheckDataStatus(fpa);
        ok(rc == true, "pmFPACheckDataStatus() returned TRUE with NULL pmFPA input parameter");
        psFree(fpa);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // pmChipCheckDataStatus() tests
    // Call with NULL pmChip input parameter
    {
        psMemId id = psMemGetId();
        bool rc = pmChipCheckDataStatus(NULL);
        ok(rc == false, "pmChipCheckDataStatus() returned FALSE with NULL pmChip input parameter");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call with acceptable input parameter
    {
        psMemId id = psMemGetId();
        pmChip *chip = generateSimpleChip(NULL);
        bool rc = pmChipCheckDataStatus(chip);
        ok(rc == false, "pmChipCheckDataStatus() returned FALSE with NULL pmChip input parameter");
        SetChipDataExists(chip);
        rc = pmChipCheckDataStatus(chip);
        ok(rc == true, "pmChipCheckDataStatus() returned TRUE with NULL pmChip input parameter");
        psFree(chip);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // pmCellCheckDataStatus() tests
    // bool pmCellCheckDataStatus(const pmCell *cell)
    // Call with NULL pmCell input parameter
    {
        psMemId id = psMemGetId();
        bool rc = pmCellCheckDataStatus(NULL);
        ok(rc == false, "pmCellCheckDataStatus() returned FALSE with NULL pmCell input parameter");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call with acceptable input parameters
    {
        psMemId id = psMemGetId();
        pmCell *cell = generateSimpleCell(NULL);
        bool rc = pmCellCheckDataStatus(cell);
        ok(rc == false, "pmCellCheckDataStatus() returned FALSE with acceptable input parameters");
        SetCellDataExists(cell);
        rc = pmCellCheckDataStatus(cell);
        ok(rc == true, "pmCellCheckDataStatus() returned TRUE with acceptable input parameters");
        psFree(cell);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // pmReadoutCheckDataStatus() tests
    // bool pmReadoutCheckDataStatus(const pmReadout *readout)
    // Call with NULL pmReadout input parameter
    {
        psMemId id = psMemGetId();
        bool rc = pmReadoutCheckDataStatus(NULL);
        ok(rc == false, "pmReadoutCheckDataStatus() returned FALSE with NULL pmReadout input parameter");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call with acceptable input parameters
    {
        psMemId id = psMemGetId();
        pmReadout *readout = generateSimpleReadout(NULL);
        bool rc = pmReadoutCheckDataStatus(readout);
        ok(rc == false, "pmReadoutCheckDataStatus() returned FALSE with acceptable input parameters");
        SetReadoutDataExists(readout);
        rc = pmReadoutCheckDataStatus(readout);
        ok(rc == true, "pmReadoutCheckDataStatus() returned TRUE with acceptable input parameters");
        psFree(readout);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
}
