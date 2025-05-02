#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include <psmodules.h>
#include "tap.h"
#include "pstap.h"

#define CHIP_ALLOC_NAME		"ChipName"
#define CELL_ALLOC_NAME		"CellName"
#define MISC_NUM_SOURCE		32
#define MISC_NUM_TARGET		16
#define MISC_NAME_SOURCE	"META00_SOURCE"
#define MISC_NAME_TARGET	"META00_TARGET"
#define NUM_BIAS_DATA		10
#define SOURCE_NUM_ROWS		16
#define SOURCE_NUM_COLS		8
#define TARGET_NUM_ROWS		15
#define TARGET_NUM_COLS		5
#define NUM_READOUTS		4
#define NUM_CELLS		6
#define NUM_CHIPS		8
#define SOURCE_BASE		10
#define BASE_INC		20
#define TARGET_BASE		(SOURCE_BASE + BASE_INC)
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
pmReadout *generateSimpleReadoutSource(pmCell *cell)
{
    pmReadout *readout = pmReadoutAlloc(cell);
    readout->image = psImageAlloc(SOURCE_NUM_COLS, SOURCE_NUM_ROWS, PS_TYPE_F32);
    readout->mask = psImageAlloc(SOURCE_NUM_COLS, SOURCE_NUM_ROWS, PS_TYPE_U8);
    readout->variance = psImageAlloc(SOURCE_NUM_COLS, SOURCE_NUM_ROWS, PS_TYPE_F32);
    for (int i = 0 ; i < SOURCE_NUM_ROWS ; i++) {
        for (int j = 0 ; j < SOURCE_NUM_COLS ; j++) {
            readout->image->data.F32[i][j] = (float) (i + j + SOURCE_BASE);
            readout->mask->data.U8[i][j] = (psU8) (i + j + SOURCE_BASE);
            readout->variance->data.F32[i][j] = (float) (i + j + SOURCE_BASE);
	}
    }
    for (psS32 i = 0 ; i < NUM_BIAS_DATA ; i++) {
        psImage *tmpImage = psImageAlloc(SOURCE_NUM_COLS, SOURCE_NUM_ROWS, PS_TYPE_F32);
        psImageInit(tmpImage, (double) i);
        psListAdd(readout->bias, PS_LIST_HEAD, tmpImage);
        psFree(tmpImage);
    }
    psMetadataAddS32(readout->analysis, PS_LIST_HEAD, MISC_NAME_SOURCE, 0, NULL, MISC_NUM_SOURCE);
    return(readout);
}

/******************************************************************************
generateSimpleCell(): This function generates a pmCell data structure and then
populates its members with real data.
 *****************************************************************************/
pmCell *generateSimpleCellSource(pmChip *chip)
{
    pmCell *cell = pmCellAlloc(chip, CELL_ALLOC_NAME);
    psMetadataAddS32(cell->analysis, PS_LIST_HEAD, MISC_NAME_SOURCE, 0, NULL, MISC_NUM_SOURCE);
    cell->hdu = pmHDUAlloc(NULL);
    psMetadataAddS32(cell->concepts, PS_LIST_HEAD, MISC_NAME_SOURCE, 0, NULL, MISC_NUM_SOURCE);
    psMetadataAddS32(cell->concepts, PS_LIST_HEAD, "CELL.XPARITY", PS_META_REPLACE, NULL, 1);
    psMetadataAddS32(cell->concepts, PS_LIST_HEAD, "CELL.YPARITY", PS_META_REPLACE, NULL, 1);

    psArrayRealloc(cell->readouts, NUM_READOUTS);
    for (int i = 0 ; i < NUM_READOUTS ; i++) {
        cell->readouts->data[i] = psMemDecrRefCounter((psPtr) generateSimpleReadoutSource(cell));
    }
    cell->data_exists = true;
    return(cell);
}

/******************************************************************************
generateSimpleChip(): This function generates a pmChip data structure and then
populates its members with real data.
 *****************************************************************************/
pmChip *generateSimpleChipSource(pmFPA *fpa)
{
    pmChip *chip = pmChipAlloc(fpa, CHIP_ALLOC_NAME);
    chip->toFPA = PS_CREATE_2D_IDENTITY_PLANE_TRANSFORM();
    chip->fromFPA = PS_CREATE_2D_IDENTITY_PLANE_TRANSFORM();
    psMetadataAddS32(chip->analysis, PS_LIST_HEAD, MISC_NAME_SOURCE, 0, NULL, MISC_NUM_SOURCE);
    psMetadataAddS32(chip->concepts, PS_LIST_HEAD, MISC_NAME_SOURCE, 0, NULL, MISC_NUM_SOURCE);
    chip->hdu = pmHDUAlloc(NULL);
    psArrayRealloc(chip->cells, NUM_CELLS);
    for (int i = 0 ; i < NUM_CELLS ; i++) {
        chip->cells->data[i] = psMemDecrRefCounter((psPtr) generateSimpleCellSource(chip));
    }
    chip->data_exists = true;
    return(chip);
}

/******************************************************************************
generateSimpleFPA(): This function generates a pmFPA data structure and then
populates its members with real data.
 *****************************************************************************/
pmFPA* generateSimpleFPASource(psMetadata *camera)
{
    pmFPA* fpa = pmFPAAlloc(camera, NULL);
    fpa->fromTPA = PS_CREATE_2D_IDENTITY_PLANE_TRANSFORM();
    fpa->toTPA = PS_CREATE_2D_IDENTITY_PLANE_TRANSFORM();
    fpa->toSky = psProjectionAlloc(0.0,0.0,10.0,10.0,PS_PROJ_TAN);
    psMetadataAddS32(fpa->analysis, PS_LIST_HEAD, MISC_NAME_SOURCE, 0, NULL, MISC_NUM_SOURCE);
    if (camera != NULL) {
        psMetadataAddS32((psMetadata *) fpa->camera, PS_LIST_HEAD, MISC_NAME_SOURCE, 0, NULL, MISC_NUM_SOURCE);
    }
    psMetadataAddS32(fpa->concepts, PS_LIST_HEAD, MISC_NAME_SOURCE, 0, NULL, MISC_NUM_SOURCE);
    fpa->hdu = pmHDUAlloc(NULL);
    psArrayRealloc(fpa->chips, NUM_CHIPS);
    for (int i = 0 ; i < NUM_CHIPS ; i++) {
        fpa->chips->data[i] = psMemDecrRefCounter((psPtr) generateSimpleChipSource(fpa));
    }

    return(fpa);
}


/******************************************************************************
generateSimpleReadout(): This function generates a pmReadout data structure and then
populates its members with real data.
 *****************************************************************************/
pmReadout *generateSimpleReadoutTarget(pmCell *cell)
{
    pmReadout *readout = pmReadoutAlloc(cell);
    readout->image = psImageAlloc(TARGET_NUM_COLS, TARGET_NUM_ROWS, PS_TYPE_F32);
    readout->mask = psImageAlloc(TARGET_NUM_COLS, TARGET_NUM_ROWS, PS_TYPE_U8);
    readout->variance = psImageAlloc(TARGET_NUM_COLS, TARGET_NUM_ROWS, PS_TYPE_F32);
    for (int i = 0 ; i < TARGET_NUM_ROWS ; i++) {
        for (int j = 0 ; j < TARGET_NUM_COLS ; j++) {
            readout->image->data.F32[i][j] = (float) (i + j + TARGET_BASE);
            readout->mask->data.U8[i][j] = (psU8) (i + j + TARGET_BASE);
            readout->variance->data.F32[i][j] = (float) (i + j + TARGET_BASE);
	}
    }
    for (psS32 i = 0 ; i < NUM_BIAS_DATA ; i++) {
        psImage *tmpImage = psImageAlloc(TARGET_NUM_COLS, TARGET_NUM_ROWS, PS_TYPE_F32);
        psImageInit(tmpImage, (double) i);
        psListAdd(readout->bias, PS_LIST_HEAD, tmpImage);
        psFree(tmpImage);
    }
    psMetadataAddS32(readout->analysis, PS_LIST_HEAD, MISC_NAME_TARGET, 0, NULL, MISC_NUM_TARGET);
    return(readout);
}

/******************************************************************************
generateSimpleCell(): This function generates a pmCell data structure and then
populates its members with real data.
 *****************************************************************************/
pmCell *generateSimpleCellTarget(pmChip *chip)
{
    pmCell *cell = pmCellAlloc(chip, CELL_ALLOC_NAME);
    psMetadataAddS32(cell->analysis, PS_LIST_HEAD, MISC_NAME_TARGET, 0, NULL, MISC_NUM_TARGET);
    cell->hdu = pmHDUAlloc(NULL);
    psMetadataAddS32(cell->concepts, PS_LIST_HEAD, MISC_NAME_TARGET, 0, NULL, MISC_NUM_TARGET);
    psMetadataAddS32(cell->concepts, PS_LIST_HEAD, "CELL.XPARITY", PS_META_REPLACE, NULL, 1);
    psMetadataAddS32(cell->concepts, PS_LIST_HEAD, "CELL.YPARITY", PS_META_REPLACE, NULL, 1);
    psArrayRealloc(cell->readouts, 0);
    for (int i = 0 ; i < 0 ; i++) {
        cell->readouts->data[i] = psMemDecrRefCounter((psPtr) generateSimpleReadoutTarget(cell));
    }
    return(cell);
}

/******************************************************************************
generateSimpleChip(): This function generates a pmChip data structure and then
populates its members with real data.
 *****************************************************************************/
pmChip *generateSimpleChipTarget(pmFPA *fpa)
{
    pmChip *chip = pmChipAlloc(fpa, CHIP_ALLOC_NAME);
    chip->toFPA = PS_CREATE_2D_IDENTITY_PLANE_TRANSFORM();
    chip->fromFPA = PS_CREATE_2D_IDENTITY_PLANE_TRANSFORM();
    psMetadataAddS32(chip->analysis, PS_LIST_HEAD, MISC_NAME_TARGET, 0, NULL, MISC_NUM_TARGET);
    psMetadataAddS32(chip->concepts, PS_LIST_HEAD, MISC_NAME_TARGET, 0, NULL, MISC_NUM_TARGET);
    chip->hdu = pmHDUAlloc(NULL);
    psArrayRealloc(chip->cells, NUM_CELLS);
    for (int i = 0 ; i < NUM_CELLS ; i++) {
        chip->cells->data[i] = psMemDecrRefCounter((psPtr) generateSimpleCellTarget(chip));
    }
    return(chip);
}

/******************************************************************************
generateSimpleFPA(): This function generates a pmFPA data structure and then
populates its members with real data.
 *****************************************************************************/
pmFPA* generateSimpleFPATarget(psMetadata *camera)
{
    pmFPA* fpa = pmFPAAlloc(camera, NULL);
    fpa->fromTPA = PS_CREATE_2D_IDENTITY_PLANE_TRANSFORM();
    fpa->toTPA = PS_CREATE_2D_IDENTITY_PLANE_TRANSFORM();
    fpa->toSky = psProjectionAlloc(0.0,0.0,10.0,10.0,PS_PROJ_TAN);
    psMetadataAddS32(fpa->analysis, PS_LIST_HEAD, MISC_NAME_TARGET, 0, NULL, MISC_NUM_TARGET);
    if (camera != NULL) {
        psMetadataAddS32((psMetadata *) fpa->camera, PS_LIST_HEAD, MISC_NAME_TARGET, 0, NULL, MISC_NUM_TARGET);
    }
    psMetadataAddS32(fpa->concepts, PS_LIST_HEAD, MISC_NAME_TARGET, 0, NULL, MISC_NUM_TARGET);
    fpa->hdu = pmHDUAlloc(NULL);
    psArrayRealloc(fpa->chips, NUM_CHIPS);
    for (int i = 0 ; i < NUM_CHIPS ; i++) {
        fpa->chips->data[i] = psMemDecrRefCounter((psPtr) generateSimpleChipTarget(fpa));
    }

    return(fpa);
}

bool testCellCopy(pmCell* cellTarget, pmCell *cellSource) {
    bool errorFlag = false;
    for (int readoutID = 0 ; readoutID < cellTarget->readouts->n ; readoutID++) {
        pmReadout *readoutTarget = cellTarget->readouts->data[readoutID];
        pmReadout *readoutSource = cellSource->readouts->data[readoutID];

        psImage *imageTarget = readoutTarget->image;
        psImage *imageSource = readoutSource->image;
        for (int i = 0 ; i < SOURCE_NUM_ROWS ; i++) {
            for (int j = 0 ; j < SOURCE_NUM_COLS ; j++) {
                if (imageSource->data.F32[i][j] != imageTarget->data.F32[i][j]) {
                    diag("ERROR: target readout[%d] image[%d][%d] is %.2f, should be %.2f",
                          readoutID, i, j, imageTarget->data.F32[i][j], imageSource->data.F32[i][j]);
                    errorFlag = true;
		}
	    }
	}
        if (errorFlag) {
            diag("ERROR: pmCellCopy() did not set the data for readout %d, image correctly", readoutID);
	}

        psImage *maskTarget = readoutTarget->mask;
        psImage *maskSource = readoutSource->mask;
        for (int i = 0 ; i < SOURCE_NUM_ROWS ; i++) {
            for (int j = 0 ; j < SOURCE_NUM_COLS ; j++) {
                if (maskTarget->data.U8[i][j] != maskSource->data.U8[i][j]) {
                    diag("ERROR: target readout[%d] mask[%d][%d] is %d, should be %d",
                          readoutID, i, j, maskTarget->data.U8[i][j], maskSource->data.U8[i][j]);
                    errorFlag = true;
		}
	    }
	}
        if (errorFlag) {
            diag("ERROR: pmCellCopy() did not set the data for readout %d, mask correctly", readoutID);
	}

        psImage *varianceTarget = readoutTarget->variance;
        psImage *varianceSource = readoutSource->variance;
        for (int i = 0 ; i < SOURCE_NUM_ROWS ; i++) {
            for (int j = 0 ; j < SOURCE_NUM_COLS ; j++) {
                if (varianceTarget->data.F32[i][j] != varianceSource->data.F32[i][j]) {
                    diag("ERROR: target readout[%d] variance[%d][%d] is %.2f, should be %.2f",
                          readoutID, i, j, varianceTarget->data.F32[i][j], varianceSource->data.F32[i][j]);
                    errorFlag = true;
		}
	    }
	}
        if (errorFlag) {
            diag("ERROR: pmCellCopy() did not set the data for readout %d, variance correctly", readoutID);
	}
    }
    return(errorFlag);
}


bool testChipCopy(pmChip* chipTarget, pmChip *chipSource) {
    bool errorFlag = false;
    for (int cellID = 0 ; cellID < chipSource->cells->n ; cellID++) {
        pmCell *cellTarget = chipTarget->cells->data[cellID];
        pmCell *cellSource = chipSource->cells->data[cellID];
        errorFlag|= testCellCopy(cellTarget, cellSource);
    }
    return(errorFlag);
}

bool testFPACopy(pmFPA* fpaTarget, pmFPA *fpaSource) {
    bool errorFlag = false;
    for (int chipID = 0 ; chipID < fpaSource->chips->n ; chipID++) {
        pmChip *chipTarget = fpaTarget->chips->data[chipID];
        pmChip *chipSource = fpaSource->chips->data[chipID];
        errorFlag|= testChipCopy(chipTarget, chipSource);
    }
    return(errorFlag);
}


bool testCellCopyStructure(pmCell* cellTarget, pmCell *cellSource) {
    bool errorFlag = false;
    for (int readoutID = 0 ; readoutID < cellTarget->readouts->n ; readoutID++) {
        pmReadout *readoutTarget = cellTarget->readouts->data[readoutID];
        if (readoutTarget->image->numRows != SOURCE_NUM_ROWS ||
            readoutTarget->image->numCols != SOURCE_NUM_COLS) {
            diag("ERROR: readoutTarget->image size is (%d by %d)\n", readoutTarget->image->numRows, 
                  readoutTarget->image->numCols);
            errorFlag = true;
	}

        if (readoutTarget->mask->numRows != SOURCE_NUM_ROWS ||
            readoutTarget->mask->numCols != SOURCE_NUM_COLS) {
            diag("ERROR: readoutTarget->mask size is (%d by %d)\n", readoutTarget->mask->numRows, 
                  readoutTarget->mask->numCols);
            errorFlag = true;
	}
        if (readoutTarget->variance->numRows != SOURCE_NUM_ROWS ||
            readoutTarget->variance->numCols != SOURCE_NUM_COLS) {
            diag("ERROR: readoutTarget->variance size is (%d by %d)\n", readoutTarget->variance->numRows, 
                  readoutTarget->variance->numCols);
            errorFlag = true;
	}
    }
    return(errorFlag);
}


bool testChipCopyStructure(pmChip* chipTarget, pmChip *chipSource) {
    bool errorFlag = false;
    for (int cellID = 0 ; cellID < chipSource->cells->n ; cellID++) {
        pmCell *cellTarget = chipTarget->cells->data[cellID];
        pmCell *cellSource = chipSource->cells->data[cellID];
        errorFlag|= testCellCopyStructure(cellTarget, cellSource);
    }
    return(errorFlag);
}

bool testFPACopyStructure(pmFPA* fpaTarget, pmFPA *fpaSource) {
    bool errorFlag = false;
    for (int chipID = 0 ; chipID < fpaSource->chips->n ; chipID++) {
        pmChip *chipTarget = fpaTarget->chips->data[chipID];
        pmChip *chipSource = fpaSource->chips->data[chipID];
        errorFlag|= testChipCopyStructure(chipTarget, chipSource);
    }
    return(errorFlag);
}


psS32 main(psS32 argc, char* argv[])
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    psTraceSetLevel("err", ERR_TRACE_LEVEL);
    plan_tests(92);


    // ------------------------------------------------------------------------
    // pmFPACopy() tests
    // Call pmFPACopy() with bad input parameters.
    if (1) {
        psMemId id = psMemGetId();
        pmFPA* fpaSource = generateSimpleFPASource(NULL);
        pmFPA* fpaTarget = generateSimpleFPATarget(NULL);
        bool rc = pmFPACopy(NULL, fpaSource);
        ok(rc == FALSE, "pmFPACopy() returned FALSE with NULL target pmFPA input parameter");
        rc = pmFPACopy(fpaTarget, NULL);
        ok(rc == FALSE, "pmFPACopy() returned FALSE with NULL source pmFPA input parameter");
        psFree(fpaSource);
        psFree(fpaTarget);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmFPACopy() with acceptable input parameters.
    if (1) {
        psMemId id = psMemGetId();
        pmFPA* fpaSource = generateSimpleFPASource(NULL);
        pmFPA* fpaTarget = generateSimpleFPATarget(NULL);
        bool rc = pmFPACopy(fpaTarget, fpaSource);
        ok(rc == true, "pmFPACopy() returned TRUE with acceptable input parameters");
        int tmpS32 = psMetadataLookupS32(&rc, fpaTarget->concepts, MISC_NAME_TARGET);
        ok(rc, "psMetadataLookupStr(NULL, fpaTarget->concepts, MISC_NAME_TARGET) was successful");
        ok(tmpS32 == MISC_NUM_TARGET, "pmFPACopy() copied the source FPA concepts correctly");

        tmpS32 = psMetadataLookupS32(&rc, fpaTarget->concepts, MISC_NAME_SOURCE);
        ok(rc, "psMetadataLookupStr(NULL, fpaTarget->concepts, MISC_NAME_SOURCE) was successful");
        ok(tmpS32 == MISC_NUM_SOURCE, "pmFPACopy() copied the source FPA concepts correctly");
        ok(!testFPACopy(fpaTarget, fpaSource), "pmFPACopy() set the pmReadout data correctly");

        psFree(fpaSource);
        psFree(fpaTarget);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ------------------------------------------------------------------------
    // pmChipCopy() tests
    // Call pmChipCopy() with bad input parameters.
    if (1) {
        psMemId id = psMemGetId();
        pmChip* chipSource = generateSimpleChipSource(NULL);
        pmChip* chipTarget = generateSimpleChipTarget(NULL);
        bool rc = pmChipCopy(NULL, chipSource);
        ok(rc == FALSE, "pmChipCopy() returned FALSE with NULL target pmChip input parameter");
        rc = pmChipCopy(chipTarget, NULL);
        ok(rc == FALSE, "pmChipCopy() returned FALSE with NULL source pmChip input parameter");
        psFree(chipSource);
        psFree(chipTarget);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmChipCopy() with acceptable input parameters.
    if (1) {
        psMemId id = psMemGetId();
        pmFPA *fpaSource = generateSimpleFPASource(NULL);
        pmFPA *fpaTarget = generateSimpleFPATarget(NULL);
        pmChip* chipSource = generateSimpleChipSource(fpaSource);
        pmChip* chipTarget = generateSimpleChipTarget(fpaTarget);
        bool rc = pmChipCopy(chipTarget, chipSource);
        ok(rc == true, "pmChipCopy() returned TRUE with acceptable input parameters");
        ok(!testChipCopy(chipTarget, chipSource), "pmChipCopy() set the pmReadout data correctly");
        psFree(chipSource);
        psFree(chipTarget);
        psFree(fpaSource);
        psFree(fpaTarget);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ------------------------------------------------------------------------
    // pmCellCopy() tests
    // Call pmCellCopy() with bad input parameters.
    if (1) {
        psMemId id = psMemGetId();
        pmCell* cellSource = generateSimpleCellSource(NULL);
        pmCell* cellTarget = generateSimpleCellTarget(NULL);
        bool rc = pmCellCopy(NULL, cellSource);
        ok(rc == FALSE, "pmCellCopy() returned FALSE with NULL target pmCell input parameter");
        rc = pmCellCopy(cellTarget, NULL);
        ok(rc == FALSE, "pmCellCopy() returned FALSE with NULL source pmCell input parameter");
        psFree(cellSource);
        psFree(cellTarget);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }



    // Call pmCellCopy() with acceptable input parameters.
    if (1) {
        psMemId id = psMemGetId();
        pmChip *parentSource = generateSimpleChipSource(NULL);
        pmChip *parentTarget = generateSimpleChipSource(NULL);
        pmCell* cellSource = generateSimpleCellSource(parentSource);
        pmCell* cellTarget = generateSimpleCellTarget(parentTarget);
        bool rc = pmCellCopy(cellTarget, cellSource);
        ok(rc == true, "pmCellCopy() returned TRUE with NULL target pmCell input parameter");
        ok(!testCellCopy(cellTarget, cellSource), "pmChipCopy() set the pmReadout data correctly");
        psFree(cellSource);
        psFree(cellTarget);
        psFree(parentSource);
        psFree(parentTarget);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ------------------------------------------------------------------------
    // pmFPACopyStructure() tests
    // bool pmFPACopyStructure(pmFPA *target, const pmFPA *source, int xBin, int yBin)
    // Call pmFPACopyStructure() with bad input parameters.
    if (1) {
        psMemId id = psMemGetId();
        pmFPA* fpaSource = generateSimpleFPASource(NULL);
        pmFPA* fpaTarget = generateSimpleFPATarget(NULL);
        bool rc = pmFPACopyStructure(NULL, fpaSource, 1.0, 1.0);
        ok(rc == FALSE, "pmFPACopyStructure() returned FALSE with NULL target pmFPA input parameter");
        rc = pmFPACopyStructure(fpaTarget, NULL, 1.0, 1.0);
        ok(rc == FALSE, "pmFPACopyStructure() returned FALSE with NULL source pmFPA input parameter");
        rc = pmFPACopyStructure(fpaTarget, fpaSource, 0.0, 1.0);
        ok(rc == FALSE, "pmFPACopyStructure() returned FALSE with non-positive xBin input parameter");
        rc = pmFPACopyStructure(fpaTarget, fpaSource, 1.0, 0.0);
        ok(rc == FALSE, "pmFPACopyStructure() returned FALSE with non-positive yBin input parameter");
        psFree(fpaSource);
        psFree(fpaTarget);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmFPACopyStructure() with acceptable input parameters.
    if (1) {
        psMemId id = psMemGetId();
        pmFPA* fpaSource = generateSimpleFPASource(NULL);
        pmFPA* fpaTarget = generateSimpleFPATarget(NULL);
        bool rc = pmFPACopyStructure(fpaTarget, fpaSource, 1.0, 1.0);
        ok(rc == true, "pmFPACopyStructure() returned TRUE with acceptable input parameters");
        ok(!testFPACopyStructure(fpaTarget, fpaSource), "pmFPACopyStructure() set the pmReadout data correctly");
        int tmpS32 = psMetadataLookupS32(&rc, fpaTarget->concepts, MISC_NAME_TARGET);
        ok(tmpS32 == MISC_NUM_TARGET, "pmFPACopy() copied the source FPA concepts correctly");
        psFree(fpaSource);
        psFree(fpaTarget);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ------------------------------------------------------------------------
    // pmChipCopyStructure() tests
    // bool pmChipCopyStructure(pmChip *target, const pmChip *source, int xBin, int yBin)
    // Call pmChipCopyStructure() with bad input parameters.
    if (1) {
        psMemId id = psMemGetId();
        pmChip* chipSource = generateSimpleChipSource(NULL);
        pmChip* chipTarget = generateSimpleChipTarget(NULL);
        bool rc = pmChipCopyStructure(NULL, chipSource, 1.0, 1.0);
        ok(rc == FALSE, "pmChipCopyStructure() returned FALSE with NULL target pmChip input parameter");
        rc = pmChipCopyStructure(chipTarget, NULL, 1.0, 1.0);
        ok(rc == FALSE, "pmChipCopyStructure() returned FALSE with NULL source pmChip input parameter");
        rc = pmChipCopyStructure(chipTarget, chipSource, 0.0, 1.0);
        ok(rc == FALSE, "pmChipCopyStructure() returned FALSE with non-positive xBin input parameter");
        rc = pmChipCopyStructure(chipTarget, chipSource, 1.0, 0.0);
        ok(rc == FALSE, "pmChipCopyStructure() returned FALSE with non-positive yBin input parameter");
        psFree(chipSource);
        psFree(chipTarget);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmChipCopyStructure() with acceptable parameters.
    if (1) {
        psMemId id = psMemGetId();
        pmFPA *fpaSource = generateSimpleFPASource(NULL);
        pmFPA *fpaTarget = generateSimpleFPATarget(NULL);
        pmChip* chipSource = generateSimpleChipSource(fpaSource);
        pmChip* chipTarget = generateSimpleChipTarget(fpaTarget);
        bool rc = pmChipCopyStructure(chipTarget, chipSource, 1.0, 1.0);
        ok(rc == true, "pmChipCopyStructure() returned TRUE with acceptable input parameters");
        ok(!testChipCopyStructure(chipTarget, chipSource), "pmChipCopyStructure() set the pmReadout data correctly");
        psFree(chipSource);
        psFree(chipTarget);
        psFree(fpaSource);
        psFree(fpaTarget);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ------------------------------------------------------------------------
    // pmCellCopyStructure() tests
    // bool pmCellCopyStructure(pmCell *target, const pmCell *source, int xBin, int yBin)
    // Call pmCellCopyStructure() with bad input parameters.
    if (1) {
        psMemId id = psMemGetId();
        pmCell* cellSource = generateSimpleCellSource(NULL);
        pmCell* cellTarget = generateSimpleCellTarget(NULL);
        bool rc = pmCellCopyStructure(NULL, cellSource, 1.0, 1.0);
        ok(rc == FALSE, "pmCellCopyStructure() returned FALSE with NULL target pmCell input parameter");
        rc = pmCellCopyStructure(cellTarget, NULL, 1.0, 1.0);
        ok(rc == FALSE, "pmCellCopyStructure() returned FALSE with NULL source pmCell input parameter");
        rc = pmCellCopyStructure(cellTarget, cellSource, 0.0, 1.0);
        ok(rc == FALSE, "pmCellCopyStructure() returned FALSE with non-positive xBin input parameter");
        rc = pmCellCopyStructure(cellTarget, cellSource, 1.0, 0.0);
        ok(rc == FALSE, "pmCellCopyStructure() returned FALSE with non-positive yBin input parameter");
        psFree(cellSource);
        psFree(cellTarget);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmCellCopyStructure() with acceptable input parameters.
    if (1) {
        psMemId id = psMemGetId();
        pmChip *parentSource = generateSimpleChipSource(NULL);
        pmChip *parentTarget = generateSimpleChipSource(NULL);
        pmCell* cellSource = generateSimpleCellSource(parentSource);
        pmCell* cellTarget = generateSimpleCellTarget(parentTarget);
        bool rc = pmCellCopyStructure(cellTarget, cellSource, 1.0, 1.0);
        ok(rc == true, "pmCellCopyStructure() returned TRUE with NULL target pmCell input parameter");
        ok(!testCellCopyStructure(cellTarget, cellSource), "pmCellCopyStructure() set the pmReadout data correctly");
        psFree(cellSource);
        psFree(cellTarget);
        psFree(parentSource);
        psFree(parentTarget);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ------------------------------------------------------------------------
    // pmChipDuplicate() tests
    // pmChip *pmChipDuplicate(pmFPA *fpa, const pmChip *source);
    // Call pmChipDuplicate() with bad input parameters.
    if (0) {
        psMemId id = psMemGetId();
        pmChip *chipSource = generateSimpleChipSource(NULL);
        pmChip *chipTarget = pmChipDuplicate(NULL, NULL);
        ok(chipTarget == NULL, "pmChipDuplicate() returned NULL with NULL pmChip input parameter");
        psFree(chipSource);
        psFree(chipTarget);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
}

