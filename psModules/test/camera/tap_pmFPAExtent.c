#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include <psmodules.h>
#include "tap.h"
#include "pstap.h"
/* STATUS:
    All functions are tested.
*/

// XXX: For the genSimpleFPA() code, add IDs to each function so that
// the values set in each chip-?cell-?hdu-?image are unique
// XXX: For the genSimpleFPA() code, write masks and weights as well

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
        rc = pmConfigFileRead(&cell->hdu->format, "../camera0/format0.config", "Camera format 0");
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

    pmConceptsBlankFPA(fpa);
    return(fpa);
}



psS32 main(psS32 argc, char* argv[])
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    psTraceSetLevel("err", ERR_TRACE_LEVEL);
    plan_tests(25);


    // ----------------------------------------------------------------------
    // pmReadoutExtent() tests: NULL input
    {
        psMemId id = psMemGetId();
        ok(NULL == pmReadoutExtent(NULL), "pmReadoutExtent(NULL) returned NULL");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // pmReadoutExtent() tests: acceptable inputs
    // XXX: We should probably test when the images are NULL, and use different size
    // images in each readout.
    {
        psMemId id = psMemGetId();
        psMetadata *camera = psMetadataAlloc();
        pmFPA* fpa = generateSimpleFPA(camera);
        ok(fpa != NULL, "Allocated a pmFPA successfully");
        bool errorFlag = false;
        for (int chipID = 0 ; chipID < fpa->chips->n ; chipID++) {
            pmChip *chip = fpa->chips->data[chipID];
            for (int cellID = 0 ; cellID < chip->cells->n ; cellID++) {
                pmCell *cell = chip->cells->data[cellID];
                for (int readoutID = 0 ; readoutID < cell->readouts->n ; readoutID++) {
                    pmReadout *readout = cell->readouts->data[readoutID];
                    psRegion *region = pmReadoutExtent(readout);
                    int xWindow = psMetadataLookupS32(NULL, readout->parent->concepts, "CELL.XWINDOW");
                    int yWindow = psMetadataLookupS32(NULL, readout->parent->concepts, "CELL.YWINDOW");
                    if (!region || 
                         region->x0 != xWindow ||
                         region->x1 != xWindow + readout->image->numCols ||
                         region->y0 != yWindow ||
                         region->y1 != yWindow + readout->image->numRows) {
                        diag("ERROR: pmReadoutExtent() did not set the psRegion correctly for chip/cell/readout (%d/%d/%d)\n", chipID, cellID, readoutID);
                        errorFlag = true;
		    }
                    psFree(region);
		}
	    }
	}
        ok(!errorFlag, "pmReadoutExtent() passed all tests");

        psFree(fpa);
        psFree(camera);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // pmCellExtent() tests: NULL input
    {
        psMemId id = psMemGetId();
        ok(NULL == pmCellExtent(NULL), "pmCellExtent(NULL) returned NULL");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // pmCellExtent() tests: acceptable inputs
    // XX: We should probably set different region sizes to better test the min/max code
    {
        psMemId id = psMemGetId();
        psMetadata *camera = psMetadataAlloc();
        pmFPA* fpa = generateSimpleFPA(camera);
        ok(fpa != NULL, "Allocated a pmFPA successfully");
        bool errorFlag = false;
        for (int chipID = 0 ; chipID < fpa->chips->n ; chipID++) {
            pmChip *chip = fpa->chips->data[chipID];
            for (int cellID = 0 ; cellID < chip->cells->n ; cellID++) {
                pmCell *cell = chip->cells->data[cellID];
                // Determine the actual extent
                psRegion *cellExtent = psRegionAlloc(INFINITY, 0, INFINITY, 0); // Extent of cell
                for (int readoutID = 0 ; readoutID < cell->readouts->n ; readoutID++) {
                    pmReadout *readout = cell->readouts->data[readoutID];
                    psRegion *roExtent = pmReadoutExtent(readout); // Extent of readout
                    cellExtent->x0 = PS_MIN(cellExtent->x0, roExtent->x0);
                    cellExtent->x1 = PS_MAX(cellExtent->x1, roExtent->x1);
                    cellExtent->y0 = PS_MIN(cellExtent->y0, roExtent->y0);
                    cellExtent->y1 = PS_MAX(cellExtent->y1, roExtent->y1);
                    psFree(roExtent);
		}
                bool mdok;
                int x0 = psMetadataLookupS32(&mdok, cell->concepts, "CELL.X0"); // Cell x offset
                int y0 = psMetadataLookupS32(&mdok, cell->concepts, "CELL.Y0"); // Cell y offset
                cellExtent->x0 += x0;
                cellExtent->x1 += x0;
                cellExtent->y0 += y0;
                cellExtent->y1 += y0;
    
                psRegion *tstExtent = pmCellExtent(cell);
                if (!cell ||
                     tstExtent->x0 != cellExtent->x0 ||
                     tstExtent->x1 != cellExtent->x1 ||
                     tstExtent->y0 != cellExtent->y0 ||
                     tstExtent->y1 != cellExtent->y1) {
                     diag("ERROR: psRegion set incorrectly for chip/cell (%d/%d)", chipID, cellID);
                     errorFlag = true;
		}
                psFree(tstExtent);
                psFree(cellExtent);
	    }
	}
        ok(!errorFlag, "pmCellExtent() passed all tests");

        psFree(fpa);
        psFree(camera);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }



    // ----------------------------------------------------------------------
    // pmChipExtent() tests: NULL input
    {
        psMemId id = psMemGetId();
        ok(NULL == pmChipExtent(NULL), "pmChipExtent(NULL) returned NULL");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // pmChipExtent() tests: acceptable inputs
    // XX: We should probably set different region sizes to better test the min/max code
    {
        psMemId id = psMemGetId();
        psMetadata *camera = psMetadataAlloc();
        pmFPA* fpa = generateSimpleFPA(camera);
        ok(fpa != NULL, "Allocated a pmFPA successfully");
        bool errorFlag = false;
        for (int chipID = 0 ; chipID < fpa->chips->n ; chipID++) {
            pmChip *chip = fpa->chips->data[chipID];
            psRegion *chipExtent = pmChipPixels(chip);
            bool rc;
            int x0 = psMetadataLookupS32(&rc, chip->concepts, "CHIP.X0"); // Chip x offset
            int y0 = psMetadataLookupS32(&rc, chip->concepts, "CHIP.Y0"); // Chip y offset
            chipExtent->x0 += x0;
            chipExtent->x1 += x0;
            chipExtent->y0 += y0;
            chipExtent->y1 += y0;
            psRegion *tstExtent = pmChipExtent(chip);
            if (!chip ||
                tstExtent->x0 != chipExtent->x0 ||
                tstExtent->x1 != chipExtent->x1 ||
                tstExtent->y0 != chipExtent->y0 ||
                tstExtent->y1 != chipExtent->y1) {
                diag("ERROR: psRegion set incorrectly for chip (%d)", chipID);
                errorFlag = true;
	    }
            psFree(tstExtent);
            psFree(chipExtent);
	}
        ok(!errorFlag, "pmChipExtent() passed all tests");

        psFree(fpa);
        psFree(camera);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // pmChipPixels() tests: NULL input
    {
        psMemId id = psMemGetId();
        ok(NULL == pmChipPixels(NULL), "pmChipPixels(NULL) returned NULL");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // pmChipPixels() tests: acceptable inputs
    {
        psMemId id = psMemGetId();
        psMetadata *camera = psMetadataAlloc();
        pmFPA* fpa = generateSimpleFPA(camera);
        ok(fpa != NULL, "Allocated a pmFPA successfully");

        bool errorFlag = false;
        for (int chipID = 0 ; chipID < fpa->chips->n ; chipID++) {
            pmChip *chip = fpa->chips->data[chipID];
            // Determine the actual pixels
            psRegion *actualExtent = psRegionAlloc(INFINITY, 0, INFINITY, 0);
            for (int cellID = 0 ; cellID < chip->cells->n ; cellID++) {
                pmCell *cell = chip->cells->data[cellID];
                psRegion *cellExtent = pmCellExtent(cell);
                actualExtent->x0 = PS_MIN(actualExtent->x0, cellExtent->x0);
                actualExtent->x1 = PS_MAX(actualExtent->x1, cellExtent->x1);
                actualExtent->y0 = PS_MIN(actualExtent->y0, cellExtent->y0);
                actualExtent->y1 = PS_MAX(actualExtent->y1, cellExtent->y1);
                psFree(cellExtent);
	    }

            // Now test if pmChipPixels() determines the same pixels
            psRegion *tstExtent = pmChipPixels(chip);
            if (!tstExtent ||
                 tstExtent->x0 != actualExtent->x0 ||
                 tstExtent->x1 != actualExtent->x1 ||
                 tstExtent->y0 != actualExtent->y0 ||
                 tstExtent->y1 != actualExtent->y1) {
                diag("ERROR: pixels set incorrectly for chip %d", chipID);
                errorFlag = true;
	    }

            // Free temp memory
            psFree(tstExtent);
            psFree(actualExtent);
	}
        ok(!errorFlag, "pmChipPixels() passed all tests");

        psFree(fpa);
        psFree(camera);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // pmFPAPixels() tests: NULL input
    {
        psMemId id = psMemGetId();
        ok(NULL == pmFPAPixels(NULL), "pmFPAPixels(NULL) returned NULL");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // pmFPAPixels() tests: acceptable inputs
    {
        psMemId id = psMemGetId();
        psMetadata *camera = psMetadataAlloc();
        pmFPA* fpa = generateSimpleFPA(camera);
        ok(fpa != NULL, "Allocated a pmFPA successfully");

        psRegion *actualExtent = psRegionAlloc(INFINITY, 0, INFINITY, 0); // Extent of fpa
        for (int chipID = 0 ; chipID < fpa->chips->n ; chipID++) {
            pmChip *chip = fpa->chips->data[chipID];
            psRegion *chipExtent = pmChipExtent(chip); // Extent of chip
            actualExtent->x0 = PS_MIN(actualExtent->x0, chipExtent->x0);
            actualExtent->x1 = PS_MAX(actualExtent->x1, chipExtent->x1);
            actualExtent->y0 = PS_MIN(actualExtent->y0, chipExtent->y0);
            actualExtent->y1 = PS_MAX(actualExtent->y1, chipExtent->y1);
            psFree(chipExtent);
        }

        bool errorFlag = false;
        psRegion *tstExtent = pmFPAPixels(fpa);
        if (!tstExtent ||
             tstExtent->x0 != actualExtent->x0 ||
             tstExtent->x1 != actualExtent->x1 ||
             tstExtent->y0 != actualExtent->y0 ||
             tstExtent->y1 != actualExtent->y1) {
            diag("ERROR: pmFPAPixels() set the pixels incorrectly");
            errorFlag = true;
	}
        ok(!errorFlag, "pmFPAPixels() set the pixels psRegion correctly");

        psFree(tstExtent);
        psFree(actualExtent);
        psFree(fpa);
        psFree(camera);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
}
