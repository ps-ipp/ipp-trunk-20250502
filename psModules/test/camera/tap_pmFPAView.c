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
    cell->hdu->blankPHU = true;

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
    chip->hdu->blankPHU = true;

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
    fpa->hdu->blankPHU = true;

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
    plan_tests(174);


    // ----------------------------------------------------------------------
    // pmFPAViewAlloc() tests
    {
        psMemId id = psMemGetId();
        pmFPAview *tmpView = pmFPAviewAlloc(32);
        ok(tmpView != NULL && psMemCheckFPAview(tmpView), "pmFPAviewAlloc() returned non-NULL");
        ok(tmpView->nRows == 32, "pmFPAviewAlloc() set ->nRows properly");
        psFree(tmpView);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // pmFPAViewReset() tests: NULL input
    {
        psMemId id = psMemGetId();
        ok(!pmFPAviewReset(NULL), "pmFPAviewReset() returned FALSE with NULL input");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // pmFPAViewReset() tests: acceptable input
    {
        psMemId id = psMemGetId();
        pmFPAview *tmpView = pmFPAviewAlloc(32);
        ok(tmpView != NULL, "pmFPAviewAlloc() returned non-NULL");
        tmpView->chip = 16;
        tmpView->cell = 16;
        tmpView->readout = 16;
        tmpView->iRows = 16;
        ok(pmFPAviewReset(tmpView), "pmFPAviewReset() returned TRUE");
        ok(tmpView->chip == -1, "pmFPAviewReset() set tmpView->chip to -1");
        ok(tmpView->cell == -1, "pmFPAviewReset() set tmpView->cell to -1");
        ok(tmpView->readout == -1, "pmFPAviewReset() set tmpView->readout to 0");
        ok(tmpView->iRows == 0, "pmFPAviewReset() set tmpView->iRows to 0");
        psFree(tmpView);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // pmFPAViewForLevel() tests: NULL input
    // pmFPAview *pmFPAviewForLevel(pmFPALevel level, const pmFPAview *input)

    {
        psMemId id = psMemGetId();
        ok(!pmFPAviewForLevel(PM_FPA_LEVEL_FPA, NULL), "pmFPAviewForLevel() returned FALSE with NULL input");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // pmFPAViewForLevel() tests: acceptable input
    {
        psMemId id = psMemGetId();
        pmFPAview *tmpView = pmFPAviewAlloc(32);
        ok(tmpView != NULL, "pmFPAviewAlloc() returned non-NULL");
        tmpView->chip = 16;
        tmpView->cell = 16;
        tmpView->readout = 16;
        tmpView->iRows = 16;

        pmFPAview *newView = pmFPAviewForLevel(PM_FPA_LEVEL_FPA, tmpView);
        ok(newView, "pmFPAviewReset(PM_FPA_LEVEL_FPA) returned non-NULL");
        ok(newView->chip == -1, "pmFPAviewReset() set tmpView->chip to -1");
        psFree(newView);

        newView = pmFPAviewForLevel(PM_FPA_LEVEL_CHIP, tmpView);
        ok(newView, "pmFPAviewReset(PM_FPA_LEVEL_CHIP) returned non-NULL");
        ok(newView->cell == -1, "pmFPAviewReset() set tmpView->cell to -1");
        psFree(newView);

        newView = pmFPAviewForLevel(PM_FPA_LEVEL_CELL, tmpView);
        ok(newView, "pmFPAviewReset(PM_FPA_LEVEL_CELL) returned non-NULL");
        ok(newView->readout == -1, "pmFPAviewReset() set tmpView->readout to 0");
        psFree(newView);

        psFree(tmpView);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // pmFPAViewLevel() tests: NULL input
    // pmFPALevel pmFPAviewLevel(const pmFPAview *view)
    {
        psMemId id = psMemGetId();
        ok(!pmFPAviewLevel(NULL), "pmFPAviewLevel() returned NULL with NULL input");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // pmFPAViewLevel() tests: acceptable input
    {
        psMemId id = psMemGetId();
        pmFPAview *tmpView = pmFPAviewAlloc(32);
        ok(tmpView != NULL, "pmFPAviewAlloc() returned non-NULL");
        tmpView->chip = 16;
        tmpView->cell = 16;
        tmpView->readout = 16;
        tmpView->iRows = 16;

        tmpView->chip = -1;
        ok(PM_FPA_LEVEL_FPA == pmFPAviewLevel(tmpView), "pmFPAviewReset() returned PM_FPA_LEVEL_FPA");
        tmpView->chip = 16;

        tmpView->cell = -1;
        ok(PM_FPA_LEVEL_CHIP == pmFPAviewLevel(tmpView), "pmFPAviewReset() returned PM_FPA_LEVEL_CHIP");
        tmpView->cell = 16;

        tmpView->readout = -1;
        ok(PM_FPA_LEVEL_CELL == pmFPAviewLevel(tmpView), "pmFPAviewReset() returned PM_FPA_LEVEL_CELL");
        tmpView->readout = 16;

        psFree(tmpView);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }



    // ----------------------------------------------------------------------
    // pmFPAviewThisChip() tests: NULL view input
    {
        psMemId id = psMemGetId();
        psMetadata *camera = psMetadataAlloc();
        pmFPA* fpa = generateSimpleFPA(camera);
        pmChip *chip = fpa->chips->data[0];
        pmCell *cell = chip->cells->data[0];
        ok(fpa != NULL, "Allocated a pmFPA successfully");
        ok(chip != NULL, "Allocated a pmChip successfully");
        ok(cell != NULL, "Allocated a pmCell successfully");
        ok(NULL == pmFPAviewThisChip(NULL, fpa), "pmFPAviewThisChip(NULL, fpa) returned NULL");

        psFree(fpa);
        psFree(camera);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // pmFPAviewThisChip() tests: NULL pmFPA input
    {
        psMemId id = psMemGetId();
        pmFPAview *tmpView = pmFPAviewAlloc(32);
        ok(NULL == pmFPAviewThisChip(tmpView, NULL), "pmFPAviewThisChip(pmFPAView, NULL) returned NULL");
        psFree(tmpView);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // pmFPAviewThisChip() tests: NULL fpa->chips input
    {
        psMemId id = psMemGetId();
        psMetadata *camera = psMetadataAlloc();
        pmFPA* fpa = generateSimpleFPA(camera);
        psArray *chips = fpa->chips;
        fpa->chips = NULL;
        pmFPAview *tmpView = pmFPAviewAlloc(32);
        ok(NULL == pmFPAviewThisChip(tmpView, fpa), "pmFPAviewThisChip(pmFPAView, fpa) returned NULL view input");
        fpa->chips = chips;
        psFree(fpa);
        psFree(camera);
        psFree(tmpView);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // pmFPAviewThisChip() tests: acceptable input
    {
        psMemId id = psMemGetId();
        psMetadata *camera = psMetadataAlloc();
        pmFPA* fpa = generateSimpleFPA(camera);
        pmChip *chip = fpa->chips->data[0];
        pmCell *cell = chip->cells->data[0];
        ok(fpa != NULL, "Allocated a pmFPA successfully");
        ok(chip != NULL, "Allocated a pmChip successfully");
        ok(cell != NULL, "Allocated a pmCell successfully");
        pmFPAview *tmpView = pmFPAviewAlloc(32);

        // Test with tmpView->chip too low
        tmpView->chip = -1;
        ok(NULL == pmFPAviewThisChip(tmpView, fpa), "pmFPAviewThisChip(pmFPAView, fpa) returned NULL with pmFPAView->chip == -1");

        // Test with tmpView->chip too high
        tmpView->chip = fpa->chips->n;
        ok(NULL == pmFPAviewThisChip(tmpView, fpa), "pmFPAviewThisChip(pmFPAView, fpa) returned NULL with pmFPAView->chip larger than the number of chips");
    
        // Test with various tmpView->chip 
        bool errorFlag = false;
        for (int i = 0 ; i < NUM_CHIPS ; i++) {
            tmpView->chip = i;
            pmChip *tmpChip = pmFPAviewThisChip(tmpView, fpa);
            if (!tmpChip || tmpChip != fpa->chips->data[i]) {
                diag("ERROR: pmFPAviewThisChip() returned the incorrect chip (%d)", i);
                errorFlag = true;
	    }
	}
        ok(!errorFlag, "pmFPAviewThisChip() returned the correct pmChip for all tests");
        psFree(fpa);
        psFree(camera);
        psFree(tmpView);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }



    // ----------------------------------------------------------------------
    // pmFPAviewNextChip() tests: NULL view input
    // pmChip *pmFPAviewNextChip(pmFPAview *view, const pmFPA *fpa, int nStep)
    {
        psMemId id = psMemGetId();
        psMetadata *camera = psMetadataAlloc();
        pmFPA* fpa = generateSimpleFPA(camera);
        pmChip *chip = fpa->chips->data[0];
        pmCell *cell = chip->cells->data[0];
        ok(fpa != NULL, "Allocated a pmFPA successfully");
        ok(chip != NULL, "Allocated a pmChip successfully");
        ok(cell != NULL, "Allocated a pmCell successfully");
        ok(NULL == pmFPAviewNextChip(NULL, fpa, 0), "pmFPAviewNextChip(NULL, fpa, 0) returned NULL");

        psFree(fpa);
        psFree(camera);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // pmFPAviewNextChip() tests: NULL pmFPA input
    {
        psMemId id = psMemGetId();
        pmFPAview *tmpView = pmFPAviewAlloc(32);
        ok(NULL == pmFPAviewNextChip(tmpView, NULL, 0), "pmFPAviewNextChip(pmFPAView, NULL, 0) returned NULL");
        psFree(tmpView);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // pmFPAviewNextChip() tests: acceptable input
    {
        psMemId id = psMemGetId();
        psMetadata *camera = psMetadataAlloc();
        pmFPA* fpa = generateSimpleFPA(camera);
        pmChip *chip = fpa->chips->data[0];
        pmCell *cell = chip->cells->data[0];
        ok(fpa != NULL, "Allocated a pmFPA successfully");
        ok(chip != NULL, "Allocated a pmChip successfully");
        ok(cell != NULL, "Allocated a pmCell successfully");
        pmFPAview *tmpView = pmFPAviewAlloc(32);

        // Test with tmpView->chip too low
        // XXX: Source code is wrong: must guard against input tmpView->chip = -1
        tmpView->chip = -1;
        ok(fpa->chips->data[0] == pmFPAviewNextChip(tmpView, fpa, 1), "pmFPAviewNextChip(pmFPAView, fpa, 1) returned NULL with pmFPAView->chip == -1");

        // Test with tmpView->chip too high
        tmpView->chip = fpa->chips->n;
        ok(NULL == pmFPAviewNextChip(tmpView, fpa, 0), "pmFPAviewNextChip(pmFPAView, fpa, 0) returned NULL with pmFPAView->chip larger than the number of chips");
    
        // Test with various tmpView->chip, step = 0
        bool errorFlag = false;
        for (int i = 0 ; i < NUM_CHIPS ; i++) {
            tmpView->chip = i;
            pmChip *tmpChip = pmFPAviewNextChip(tmpView, fpa, 0);
            if (!tmpChip || tmpChip != fpa->chips->data[i]) {
                diag("ERROR: pmFPAviewNextChip() returned the incorrect chip (%d)", i);
                errorFlag = true;
	    }
	}
        ok(!errorFlag, "pmFPAviewNextChip() returned the correct pmChip for all tests (step = 0)");

        // Test with various tmpView->chip, step = 1
        errorFlag = false;
        for (int i = 0 ; i < NUM_CHIPS-1 ; i++) {
            tmpView->chip = i;
            pmChip *tmpChip = pmFPAviewNextChip(tmpView, fpa, 1);
            if (!tmpChip || tmpChip != fpa->chips->data[i+1]) {
                diag("ERROR: pmFPAviewNextChip() returned the incorrect chip (%d)", i);
                errorFlag = true;
	    }
	}
        ok(!errorFlag, "pmFPAviewNextChip() returned the correct pmChip for all tests (step = 1)");

        psFree(fpa);
        psFree(camera);
        psFree(tmpView);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // pmFPAviewThisCell() tests: NULL view input
    {
        psMemId id = psMemGetId();
        psMetadata *camera = psMetadataAlloc();
        pmFPA* fpa = generateSimpleFPA(camera);
        pmChip *chip = fpa->chips->data[0];
        pmCell *cell = chip->cells->data[0];
        ok(fpa != NULL, "Allocated a pmFPA successfully");
        ok(chip != NULL, "Allocated a pmChip successfully");
        ok(cell != NULL, "Allocated a pmCell successfully");
        ok(NULL == pmFPAviewThisCell(NULL, fpa), "pmFPAviewThisCell(NULL, fpa) returned NULL");

        psFree(fpa);
        psFree(camera);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // pmFPAviewThisCell() tests: NULL pmFPA input
    {
        psMemId id = psMemGetId();
        pmFPAview *tmpView = pmFPAviewAlloc(32);
        ok(NULL == pmFPAviewThisCell(tmpView, NULL), "pmFPAviewThisCell(pmFPAView, NULL) returned NULL");
        psFree(tmpView);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // pmFPAviewThisCell() tests: NULL fpa->chips input
    {
        psMemId id = psMemGetId();
        psMetadata *camera = psMetadataAlloc();
        pmFPA* fpa = generateSimpleFPA(camera);
        psArray *chips = fpa->chips;
        fpa->chips = NULL;
        pmFPAview *tmpView = pmFPAviewAlloc(32);
        ok(NULL == pmFPAviewThisCell(tmpView, fpa), "pmFPAviewThisCell(pmFPAView, fpa) returned NULL view input");
        fpa->chips = chips;
        psFree(fpa);
        psFree(camera);
        psFree(tmpView);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // pmFPAviewThisCell() tests: acceptable input
    {
        psMemId id = psMemGetId();
        psMetadata *camera = psMetadataAlloc();
        pmFPA* fpa = generateSimpleFPA(camera);
        pmChip *chip = fpa->chips->data[0];
        pmCell *cell = chip->cells->data[0];
        ok(fpa != NULL, "Allocated a pmFPA successfully");
        ok(chip != NULL, "Allocated a pmChip successfully");
        ok(cell != NULL, "Allocated a pmCell successfully");
        pmFPAview *tmpView = pmFPAviewAlloc(32);

        // Test with tmpView->chip too low
        tmpView->chip = -1;
        ok(NULL == pmFPAviewThisCell(tmpView, fpa), "pmFPAviewThisCell(pmFPAView, fpa) returned NULL with pmFPAView->chip == -1");

        // Test with tmpView->chip too high
        tmpView->chip = fpa->chips->n;
        ok(NULL == pmFPAviewThisCell(tmpView, fpa), "pmFPAviewThisCell(pmFPAView, fpa) returned NULL with pmFPAView->chip larger than the number of chips");
    
        // Test with various tmpView->chip 
        bool errorFlag = false;
        for (int i = 0 ; i < NUM_CHIPS ; i++) {
            pmChip *chip = fpa->chips->data[i];
            for (int j = 0 ; j < NUM_CELLS ; j++) {
                tmpView->chip = i;
                tmpView->cell = j;
                pmCell *tmpCell = pmFPAviewThisCell(tmpView, fpa);
                if (!tmpCell || tmpCell != chip->cells->data[j]) {
                    diag("ERROR: pmFPAviewThisCell() returned the incorrect chip/cell (%d, %d)", i, j);
                    errorFlag = true;
		}

    	    }
	}
        ok(!errorFlag, "pmFPAviewThisCell() returned the correct pmCell for all tests");
        psFree(fpa);
        psFree(camera);
        psFree(tmpView);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // pmFPAviewNextCell() tests: NULL view input
    // pmChip *pmFPAviewNextCell(pmFPAview *view, const pmFPA *fpa, int nStep)
    {
        psMemId id = psMemGetId();
        psMetadata *camera = psMetadataAlloc();
        pmFPA* fpa = generateSimpleFPA(camera);
        pmChip *chip = fpa->chips->data[0];
        pmCell *cell = chip->cells->data[0];
        ok(fpa != NULL, "Allocated a pmFPA successfully");
        ok(chip != NULL, "Allocated a pmChip successfully");
        ok(cell != NULL, "Allocated a pmCell successfully");
        ok(NULL == pmFPAviewNextCell(NULL, fpa, 0), "pmFPAviewNextCell(NULL, fpa, 0) returned NULL");

        psFree(fpa);
        psFree(camera);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // pmFPAviewNextCell() tests: NULL pmFPA input
    {
        psMemId id = psMemGetId();
        pmFPAview *tmpView = pmFPAviewAlloc(32);
        ok(NULL == pmFPAviewNextCell(tmpView, NULL, 0), "pmFPAviewNextCell(pmFPAView, NULL, 0) returned NULL");
        psFree(tmpView);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // pmFPAviewNextCell() tests: acceptable input
    {
        psMemId id = psMemGetId();
        psMetadata *camera = psMetadataAlloc();
        pmFPA* fpa = generateSimpleFPA(camera);
        pmChip *chip = fpa->chips->data[0];
        pmCell *cell = chip->cells->data[0];
        ok(fpa != NULL, "Allocated a pmFPA successfully");
        ok(chip != NULL, "Allocated a pmChip successfully");
        ok(cell != NULL, "Allocated a pmCell successfully");
        pmFPAview *tmpView = pmFPAviewAlloc(32);

        // Test with tmpView->chip too low
        tmpView->chip = -1;
        ok(NULL == pmFPAviewNextCell(tmpView, fpa, 0), "pmFPAviewNextCell(pmFPAView, fpa) returned NULL with pmFPAView->chip == -1");

        // Test with tmpView->chip too high
        tmpView->chip = fpa->chips->n;
        ok(NULL == pmFPAviewNextCell(tmpView, fpa, 0), "pmFPAviewNextCell(pmFPAView, fpa) returned NULL with pmFPAView->chip larger than the number of chips");
    
        // Test with various tmpView->chip (step = 0)
        bool errorFlag = false;
        for (int i = 0 ; i < NUM_CHIPS ; i++) {
            pmChip *chip = fpa->chips->data[i];
            for (int j = 0 ; j < NUM_CELLS ; j++) {
                tmpView->chip = i;
                tmpView->cell = j;
                pmCell *tmpCell = pmFPAviewNextCell(tmpView, fpa, 0);
                if (!tmpCell || tmpCell != chip->cells->data[j]) {
                    diag("ERROR: pmFPAviewNextCell() returned the incorrect chip/cell (%d, %d)", i, j);
                    errorFlag = true;
		}

    	    }
	}
        ok(!errorFlag, "pmFPAviewNextCell() returned the correct pmCell for all tests (step = 0)");

        // Test with various tmpView->chip (step = 1)
        errorFlag = false;
        for (int i = 0 ; i < NUM_CHIPS ; i++) {
            pmChip *chip = fpa->chips->data[i];
            for (int j = 0 ; j < NUM_CELLS-1 ; j++) {
                tmpView->chip = i;
                tmpView->cell = j;
                pmCell *tmpCell = pmFPAviewNextCell(tmpView, fpa, 1);
                if (!tmpCell || tmpCell != chip->cells->data[j+1]) {
                    diag("ERROR: pmFPAviewNextCell() returned the incorrect chip/cell (%d, %d)", i, j);
                    errorFlag = true;
		}

    	    }
	}
        ok(!errorFlag, "pmFPAviewNextCell() returned the correct pmCell for all tests (step = 1)");

        psFree(fpa);
        psFree(camera);
        psFree(tmpView);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }



    // ----------------------------------------------------------------------
    // pmFPAviewThisReadout() tests: NULL view input
    {
        psMemId id = psMemGetId();
        psMetadata *camera = psMetadataAlloc();
        pmFPA* fpa = generateSimpleFPA(camera);
        pmChip *chip = fpa->chips->data[0];
        pmCell *cell = chip->cells->data[0];
        ok(fpa != NULL, "Allocated a pmFPA successfully");
        ok(chip != NULL, "Allocated a pmChip successfully");
        ok(cell != NULL, "Allocated a pmCell successfully");
        ok(NULL == pmFPAviewThisReadout(NULL, fpa), "pmFPAviewThisReadout(NULL, fpa) returned NULL");

        psFree(fpa);
        psFree(camera);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // pmFPAviewThisReadout() tests: NULL pmFPA input
    {
        psMemId id = psMemGetId();
        pmFPAview *tmpView = pmFPAviewAlloc(32);
        ok(NULL == pmFPAviewThisReadout(tmpView, NULL), "pmFPAviewThisReadout(pmFPAView, NULL) returned NULL");
        psFree(tmpView);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // pmFPAviewThisReadout() tests: NULL fpa->chips input
    {
        psMemId id = psMemGetId();
        psMetadata *camera = psMetadataAlloc();
        pmFPA* fpa = generateSimpleFPA(camera);
        psArray *chips = fpa->chips;
        fpa->chips = NULL;
        pmFPAview *tmpView = pmFPAviewAlloc(32);
        ok(NULL == pmFPAviewThisReadout(tmpView, fpa), "pmFPAviewThisReadout(pmFPAView, fpa) returned NULL view input");
        fpa->chips = chips;
        psFree(fpa);
        psFree(camera);
        psFree(tmpView);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // pmFPAviewThisReadout() tests: acceptable input
    {
        psMemId id = psMemGetId();
        psMetadata *camera = psMetadataAlloc();
        pmFPA* fpa = generateSimpleFPA(camera);
        pmChip *chip = fpa->chips->data[0];
        pmCell *cell = chip->cells->data[0];
        ok(fpa != NULL, "Allocated a pmFPA successfully");
        ok(chip != NULL, "Allocated a pmChip successfully");
        ok(cell != NULL, "Allocated a pmCell successfully");
        pmFPAview *tmpView = pmFPAviewAlloc(32);

        // Test with tmpView->chip too low
        tmpView->chip = -1;
        ok(NULL == pmFPAviewThisReadout(tmpView, fpa), "pmFPAviewThisReadout(pmFPAView, fpa) returned NULL with pmFPAView->chip == -1");

        // Test with tmpView->chip too high
        tmpView->chip = fpa->chips->n;
        ok(NULL == pmFPAviewThisReadout(tmpView, fpa), "pmFPAviewThisReadout(pmFPAView, fpa) returned NULL with pmFPAView->chip larger than the number of chips");
    
        // Test with various tmpView->chip 
        bool errorFlag = false;
        for (int i = 0 ; i < NUM_CHIPS ; i++) {
            pmChip *chip = fpa->chips->data[i];
            for (int j = 0 ; j < NUM_CELLS ; j++) {
                pmCell *cell = chip->cells->data[j];
                for (int k = 0 ; k < NUM_READOUTS ; k++) {
                    tmpView->chip = i;
                    tmpView->cell = j;
                    tmpView->readout = k;
                    pmReadout *tmpReadout = pmFPAviewThisReadout(tmpView, fpa);
                    if (!tmpReadout || tmpReadout != cell->readouts->data[k]) {
                        diag("ERROR: pmFPAviewThisReadout() returned the incorrect chip/cell/readout (%d, %d, %d)", i, j, k);
                        errorFlag = true;
		    }
		}
    	    }
	}
        ok(!errorFlag, "pmFPAviewThisReadout() returned the correct pmCell for all tests");

        psFree(fpa);
        psFree(camera);
        psFree(tmpView);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // pmFPAviewNextReadout() tests: NULL view input
    // pmChip *pmFPAviewNextReadout(pmFPAview *view, const pmFPA *fpa, int nStep)
    {
        psMemId id = psMemGetId();
        psMetadata *camera = psMetadataAlloc();
        pmFPA* fpa = generateSimpleFPA(camera);
        pmChip *chip = fpa->chips->data[0];
        pmCell *cell = chip->cells->data[0];
        ok(fpa != NULL, "Allocated a pmFPA successfully");
        ok(chip != NULL, "Allocated a pmChip successfully");
        ok(cell != NULL, "Allocated a pmCell successfully");
        ok(NULL == pmFPAviewNextReadout(NULL, fpa, 0), "pmFPAviewNextReadout(NULL, fpa, 0) returned NULL");

        psFree(fpa);
        psFree(camera);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // pmFPAviewNextReadout() tests: NULL pmFPA input
    {
        psMemId id = psMemGetId();
        pmFPAview *tmpView = pmFPAviewAlloc(32);
        ok(NULL == pmFPAviewNextReadout(tmpView, NULL, 0), "pmFPAviewNextReadout(pmFPAView, NULL, 0) returned NULL");
        psFree(tmpView);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // pmFPAviewNextReadout() tests: acceptable input
    {
        psMemId id = psMemGetId();
        psMetadata *camera = psMetadataAlloc();
        pmFPA* fpa = generateSimpleFPA(camera);
        pmChip *chip = fpa->chips->data[0];
        pmCell *cell = chip->cells->data[0];
        ok(fpa != NULL, "Allocated a pmFPA successfully");
        ok(chip != NULL, "Allocated a pmChip successfully");
        ok(cell != NULL, "Allocated a pmCell successfully");
        pmFPAview *tmpView = pmFPAviewAlloc(32);

        // Test with tmpView->chip too low
        tmpView->chip = -1;
        ok(NULL == pmFPAviewNextReadout(tmpView, fpa, 0), "pmFPAviewNextReadout(pmFPAView, fpa) returned NULL with pmFPAView->chip == -1");

        // Test with tmpView->chip too high
        tmpView->chip = fpa->chips->n;
        ok(NULL == pmFPAviewNextReadout(tmpView, fpa, 0), "pmFPAviewNextReadout(pmFPAView, fpa) returned NULL with pmFPAView->chip larger than the number of chips");
    
        // Test with various tmpView->chip (step = 0)
        bool errorFlag = false;
        for (int i = 0 ; i < NUM_CHIPS ; i++) {
            pmChip *chip = fpa->chips->data[i];
            for (int j = 0 ; j < NUM_CELLS ; j++) {
                pmCell *cell = chip->cells->data[j];
                for (int k = 0 ; k < NUM_READOUTS ; k++) {
                    tmpView->chip = i;
                    tmpView->cell = j;
                    tmpView->readout = k;
                    pmReadout *tmpReadout = pmFPAviewNextReadout(tmpView, fpa, 0);
                    if (!tmpReadout || tmpReadout != cell->readouts->data[k]) {
                        diag("ERROR: pmFPAviewNextReadout() returned the incorrect chip/cell/readout (%d, %d, %d)", i, j, k);
                        errorFlag = true;
		    }
		}
    	    }
	}
        ok(!errorFlag, "pmFPAviewNextReadout() returned the correct pmCell for all tests (step = 0)");

        // Test with various tmpView->chip (step = 1)
        errorFlag = false;
        for (int i = 0 ; i < NUM_CHIPS ; i++) {
            pmChip *chip = fpa->chips->data[i];
            for (int j = 0 ; j < NUM_CELLS ; j++) {
                pmCell *cell = chip->cells->data[j];
                for (int k = 0 ; k < NUM_READOUTS-1 ; k++) {
                    tmpView->chip = i;
                    tmpView->cell = j;
                    tmpView->readout = k;
                    pmReadout *tmpReadout = pmFPAviewNextReadout(tmpView, fpa, 1);
                    if (!tmpReadout || tmpReadout != cell->readouts->data[k+1]) {
                        diag("ERROR: pmFPAviewNextReadout() returned the incorrect chip/cell/readout (%d, %d, %d)", i, j, k);
                        errorFlag = true;
		    }
		}
    	    }
	}
        ok(!errorFlag, "pmFPAviewNextReadout() returned the correct pmCell for all tests (step = 1)");

        psFree(fpa);
        psFree(camera);
        psFree(tmpView);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // pmFPAviewThisHDU() tests: NULL view input
    // pmHDU *pmFPAviewThisHDU(const pmFPAview *view, const pmFPA *fpa)
    {
        psMemId id = psMemGetId();
        psMetadata *camera = psMetadataAlloc();
        pmFPA* fpa = generateSimpleFPA(camera);
        pmChip *chip = fpa->chips->data[0];
        pmCell *cell = chip->cells->data[0];
        ok(fpa != NULL, "Allocated a pmFPA successfully");
        ok(chip != NULL, "Allocated a pmChip successfully");
        ok(cell != NULL, "Allocated a pmCell successfully");
        ok(NULL == pmFPAviewThisHDU(NULL, fpa), "pmFPAviewThisHDU(NULL, fpa) returned NULL");
        psFree(fpa);
        psFree(camera);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // pmFPAviewThisHDU() tests: NULL pmFPA input
    // pmHDU *pmFPAviewThisHDU(const pmFPAview *view, const pmFPA *fpa)
    {
        psMemId id = psMemGetId();
        pmFPAview *tmpView = pmFPAviewAlloc(32);
        ok(NULL == pmFPAviewThisHDU(tmpView, NULL), "pmFPAviewThisHDU(pmFPAView, NULL) returned NULL");
        psFree(tmpView);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // pmFPAviewThisHDU() tests: acceptable input
    {
        psMemId id = psMemGetId();
        psMetadata *camera = psMetadataAlloc();
        pmFPA* fpa = generateSimpleFPA(camera);
        pmChip *chip = fpa->chips->data[0];
        pmCell *cell = chip->cells->data[0];
        ok(fpa != NULL, "Allocated a pmFPA successfully");
        ok(chip != NULL, "Allocated a pmChip successfully");
        ok(cell != NULL, "Allocated a pmCell successfully");
        pmFPAview *tmpView = pmFPAviewAlloc(32);

        tmpView->chip = -1;
        pmHDU *hdu = pmFPAviewThisHDU(tmpView, fpa);
        ok(hdu, "pmFPAviewThisHDU() returned non-NULL with tmpView->chip = -1");
        ok(hdu == pmHDUFromFPA(fpa), "pmFPAviewThisHDU() returned the correct HDU");
        tmpView->chip = NUM_CHIPS/2;

        tmpView->cell = -1;
        hdu = pmFPAviewThisHDU(tmpView, fpa);
        ok(hdu, "pmFPAviewThisHDU() returned non-NULL with tmpView->cell = -1");
        ok(hdu == pmHDUFromChip(pmFPAviewThisChip(tmpView, fpa)), "pmFPAviewThisHDU() returned the correct HDU");
        tmpView->cell = NUM_CELLS/2;

        tmpView->readout = -1;
        hdu = pmFPAviewThisHDU(tmpView, fpa);
        ok(hdu, "pmFPAviewThisHDU() returned non-NULL with tmpView->readout = -1");
        ok(hdu == pmHDUFromCell(pmFPAviewThisCell(tmpView, fpa)), "pmFPAviewThisHDU() returned the correct HDU");
        tmpView->readout = NUM_READOUTS/2;

        hdu = pmFPAviewThisHDU(tmpView, fpa);
        ok(hdu, "pmFPAviewThisHDU() returned non-NULL");
        ok(hdu == pmHDUFromReadout(pmFPAviewThisReadout(tmpView, fpa)), "pmFPAviewThisHDU() returned the correct HDU");

        psFree(fpa);
        psFree(camera);
        psFree(tmpView);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // pmFPAviewThisPHU() tests: NULL view input
    // pmHDU *pmFPAviewThisPHU(const pmFPAview *view, const pmFPA *fpa)
    {
        psMemId id = psMemGetId();
        psMetadata *camera = psMetadataAlloc();
        pmFPA* fpa = generateSimpleFPA(camera);
        pmChip *chip = fpa->chips->data[0];
        pmCell *cell = chip->cells->data[0];
        ok(fpa != NULL, "Allocated a pmFPA successfully");
        ok(chip != NULL, "Allocated a pmChip successfully");
        ok(cell != NULL, "Allocated a pmCell successfully");
        ok(NULL == pmFPAviewThisPHU(NULL, fpa), "pmFPAviewThisPHU(NULL, fpa) returned NULL");
        psFree(fpa);
        psFree(camera);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // pmFPAviewThisPHU() tests: NULL pmFPA input
    // pmHDU *pmFPAviewThisPHU(const pmFPAview *view, const pmFPA *fpa)
    {
        psMemId id = psMemGetId();
        pmFPAview *tmpView = pmFPAviewAlloc(32);
        ok(NULL == pmFPAviewThisPHU(tmpView, NULL), "pmFPAviewThisPHU(pmFPAView, NULL) returned NULL");
        psFree(tmpView);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // pmFPAviewThisPHU() tests: acceptable input
    {
        psMemId id = psMemGetId();
        psMetadata *camera = psMetadataAlloc();
        pmFPA* fpa = generateSimpleFPA(camera);
        pmChip *chip = fpa->chips->data[0];
        pmCell *cell = chip->cells->data[0];
        ok(fpa != NULL, "Allocated a pmFPA successfully");
        ok(chip != NULL, "Allocated a pmChip successfully");
        ok(cell != NULL, "Allocated a pmCell successfully");
        pmFPAview *tmpView = pmFPAviewAlloc(32);

        tmpView->chip = -1;
        pmHDU *hdu = pmFPAviewThisPHU(tmpView, fpa);
        ok(hdu, "pmFPAviewThisPHU() returned non-NULL with tmpView->chip = -1");
        ok(hdu == pmHDUFromFPA(fpa), "pmFPAviewThisPHU() returned the correct HDU");
        tmpView->chip = NUM_CHIPS/2;

        tmpView->cell = -1;
        hdu = pmFPAviewThisPHU(tmpView, fpa);
        ok(hdu, "pmFPAviewThisPHU() returned non-NULL with tmpView->cell = -1");
        ok(hdu == pmHDUFromChip(pmFPAviewThisChip(tmpView, fpa)), "pmFPAviewThisPHU() returned the correct HDU");
        tmpView->cell = NUM_CELLS/2;

        tmpView->readout = -1;
        hdu = pmFPAviewThisPHU(tmpView, fpa);
        ok(hdu, "pmFPAviewThisPHU() returned non-NULL with tmpView->readout = -1");
        ok(hdu == pmHDUFromCell(pmFPAviewThisCell(tmpView, fpa)), "pmFPAviewThisPHU() returned the correct HDU");
        tmpView->readout = NUM_READOUTS/2;

        hdu = pmFPAviewThisPHU(tmpView, fpa);
        ok(hdu == NULL, "pmFPAviewThisPHU() returned NULL");

        psFree(fpa);
        psFree(camera);
        psFree(tmpView);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // pmFPAviewGenerate() tests: NULL pmFPA input
    {
        psMemId id = psMemGetId();
        int chipID = NUM_CHIPS/2;
        int cellID = NUM_CELLS/2;
        int readoutID = NUM_READOUTS/2;
        pmFPA* fpa = generateSimpleFPA(NULL);
        pmChip *chip = fpa->chips->data[chipID];
        pmCell *cell = chip->cells->data[cellID];
        pmReadout *readout = cell->readouts->data[readoutID];
        ok(fpa != NULL, "Allocated a pmFPA successfully");
        ok(chip != NULL, "Allocated a pmChip successfully");
        ok(cell != NULL, "Allocated a pmCell successfully");
        ok(readout != NULL, "Allocated a pmReadout successfully");
        pmFPAview *view = pmFPAviewGenerate(NULL, chip, cell, readout);
        ok(view == NULL, "pmFPAviewGenerate(NULL, chip, cell, readout) returned NULL");
        psFree(fpa);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // pmFPAviewGenerate() tests: acceptable input parameters
    // XX: Add more input combinations.
    {
        psMemId id = psMemGetId();
        int chipID = NUM_CHIPS/2;
        int cellID = NUM_CELLS/2;
        int readoutID = NUM_READOUTS/2;
        pmFPA* fpa = generateSimpleFPA(NULL);
        pmChip *chip = fpa->chips->data[chipID];
        pmCell *cell = chip->cells->data[cellID];
        pmReadout *readout = cell->readouts->data[readoutID];
        ok(fpa != NULL, "Allocated a pmFPA successfully");
        ok(chip != NULL, "Allocated a pmChip successfully");
        ok(cell != NULL, "Allocated a pmCell successfully");
        ok(readout != NULL, "Allocated a pmReadout successfully");
        pmFPAview *view = pmFPAviewGenerate(fpa, chip, cell, readout);
        ok(view != NULL && psMemCheckFPAview(view), "pmFPAviewGenerate() returned an pmFPAviw with acceptable input parameters");
        ok(view->chip == chipID, "pmFPAviewGenerate() set pmFPAview->chip correctly");
        ok(view->cell == cellID, "pmFPAviewGenerate() set pmFPAview->cell correctly");
        ok(view->readout == readoutID, "pmFPAviewGenerate() set pmFPAview->readout correctly");
        psFree(view);
        psFree(fpa);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
}

