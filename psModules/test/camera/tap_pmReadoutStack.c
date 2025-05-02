#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include <psmodules.h>
#include "tap.h"
#include "pstap.h"
/* STATUS:
    All functions are tested.
*/

#define CELL_ALLOC_NAME		"CellName"
#define MISC_NUM		32
#define MISC_NAME		"META00"
#define NUM_BIAS_DATA		10
#define TEST_NUM_ROWS		5
#define TEST_NUM_COLS		8
#define NUM_INPUTS		10
#define NUM_READOUTS		4
#define VERBOSE			0
#define ERR_TRACE_LEVEL		10

/******************************************************************************
generateSimpleReadout(): This function generates a pmReadout data structure and then
populates its members with real data.
 *****************************************************************************/
pmReadout *generateSimpleReadout(pmCell *cell)
{
    pmReadout *readout = pmReadoutAlloc(cell);
    readout->image = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_F32);
    readout->mask = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_U8);
    readout->weight = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_F32);
    psImageInit(readout->image, 1.0);
    psImageInit(readout->mask, 2);
    psImageInit(readout->weight, 3.0);
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
pmCell *generateSimpleCell(int ID)
{
    pmCell *cell = pmCellAlloc(NULL, CELL_ALLOC_NAME);
    psMetadataAddS32(cell->analysis, PS_LIST_HEAD, MISC_NAME, 0, NULL, MISC_NUM);
    cell->hdu = pmHDUAlloc(NULL);
    psMetadataAddS32(cell->concepts, PS_LIST_HEAD, MISC_NAME, 0, NULL, MISC_NUM);
    psRegion *region = psRegionAlloc(0.0, (float) (10 + ID), 0.0, (float) (20 + ID));
    psMetadataRemoveKey(cell->concepts, "CELL.TRIMSEC");
    psMetadataAddPtr(cell->concepts, PS_LIST_TAIL|PS_META_REPLACE, "CELL.TRIMSEC", PS_DATA_REGION, "I am a region", region);
    psFree(region);
    psArrayRealloc(cell->readouts, NUM_READOUTS);
    for (int i = 0 ; i < NUM_READOUTS ; i++) {
        cell->readouts->data[i] = psMemDecrRefCounter((psPtr) generateSimpleReadout(cell));
    }

    return(cell);
}



psS32 main(psS32 argc, char* argv[])
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    psTraceSetLevel("err", ERR_TRACE_LEVEL);
    plan_tests(29);

    // ------------------------------------------------------------------------
    // pmReadoutUpdateSize() tests
    // Call pmReadoutUpdateSize() with NULL pmReadout input parameter.
    {
        psMemId id = psMemGetId();
        bool rc = pmReadoutUpdateSize(NULL, 0, 0, TEST_NUM_COLS, TEST_NUM_ROWS, false);
        ok(rc == false, "pmReadoutUpdateSize() returned FALSE with NULL pmReadout input parameter");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmReadoutUpdateSize() with acceptable input parameters (mask == false).
    {
        psMemId id = psMemGetId();
        pmReadout *readout = generateSimpleReadout(NULL);
        bool rc = pmReadoutUpdateSize(readout, 0, 0, 2*TEST_NUM_COLS, 2*TEST_NUM_ROWS, false);
        ok(rc == true, "pmReadoutUpdateSize() returned TRUE with acceptable input parameters");
        ok(readout->image->numCols == (2*TEST_NUM_COLS) &&
           readout->image->numRows == (2*TEST_NUM_ROWS), "pmReadoutUpdateSize() generated the correct size pmReadout->image");
        bool errorFlag = false;
        for (int i = 0 ; i < readout->image->numRows ; i++) {
            for (int j = 0 ; j < readout->image->numCols ; j++) {
                psF32 correctF32;
                if (i < TEST_NUM_ROWS && j < TEST_NUM_COLS) {
                   correctF32 = 1.0;
                } else {
                   correctF32 = 0.0;
                }
                if (readout->image->data.F32[i][j] != correctF32) {
                    diag("ERROR: readout->image[%d][%d] is %.2f, should be %.2f", i, j, readout->image->data.F32[i][j], correctF32);
                    errorFlag = true;
                }
            }
        }
        ok(!errorFlag, "pmReadoutUpdateSize() initialized pmReadout->image to zero");
        ok(readout->mask->numCols == (TEST_NUM_COLS) &&
           readout->mask->numRows == (TEST_NUM_ROWS), "pmReadoutUpdateSize() generated the correct size pmReadout->mask");

        psFree(readout);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmReadoutUpdateSize() with acceptable input parameters (mask == true).
    {
        psMemId id = psMemGetId();
        pmReadout *readout = generateSimpleReadout(NULL);
        bool rc = pmReadoutUpdateSize(readout, 0, 0, 2*TEST_NUM_COLS, 2*TEST_NUM_ROWS, true);
        ok(rc == true, "pmReadoutUpdateSize() returned TRUE with acceptable input parameters");
        ok(readout->image->numCols == (2*TEST_NUM_COLS) &&
           readout->image->numRows == (2*TEST_NUM_ROWS), "pmReadoutUpdateSize() generated the correct size pmReadout->image");
        bool errorFlag = false;
        for (int i = 0 ; i < readout->image->numRows ; i++) {
            for (int j = 0 ; j < readout->image->numCols ; j++) {
                psF32 correctF32;
                if (i < TEST_NUM_ROWS && j < TEST_NUM_COLS) {
                   correctF32 = 1.0;
                } else {
                   correctF32 = 0.0;
                }
                if (readout->image->data.F32[i][j] != correctF32) {
                    diag("ERROR: readout->image[%d][%d] is %.2f, should be %.2f", i, j, readout->image->data.F32[i][j], correctF32);
                    errorFlag = true;
                }
            }
        }
        ok(!errorFlag, "pmReadoutUpdateSize() initialized pmReadout->image to zero");
        ok(readout->mask->numCols == (2*TEST_NUM_COLS) &&
           readout->mask->numRows == (2*TEST_NUM_ROWS), "pmReadoutUpdateSize() generated the correct size pmReadout->mask");
        errorFlag = false;
        for (int i = 0 ; i < readout->mask->numRows ; i++) {
            for (int j = 0 ; j < readout->mask->numCols ; j++) {
                psF32 correctU8;
                if (i < TEST_NUM_ROWS && j < TEST_NUM_COLS) {
                   correctU8 = 2;
                } else {
                   correctU8 = 0;
                }
                if (readout->mask->data.U8[i][j] != correctU8) {
                    diag("ERROR: readout->mask[%d][%d] is %d, should be %d", i, j, readout->mask->data.U8[i][j], correctU8);
                    errorFlag = true;
                }
            }
        }
        ok(!errorFlag, "pmReadoutUpdateSize() initialized pmReadout->mask to zero");

        psFree(readout);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }



    // ------------------------------------------------------------------------
    // pmReadoutStackValidate() tests
    // Call pmReadoutStackValidate() with bad input parameters.
    {
        psMemId id = psMemGetId();
        int minInputCols, maxInputCols, minInputRows, maxInputRows, numCols, numRows;
        minInputCols = maxInputCols = minInputRows = maxInputRows = numCols = numRows = 0;
        bool rc = pmReadoutStackValidate(&minInputCols, &maxInputCols, &minInputRows, &maxInputRows,
                                         &numCols, &numRows, NULL);
        psArray *inputs = psArrayAlloc(NUM_INPUTS);
        for (int i = 0 ; i < NUM_INPUTS ; i++) {
            inputs->data[i] = (psPtr *) generateSimpleReadout(NULL);
        }

        // NULL psArray input parameter
        rc = pmReadoutStackValidate(&minInputCols, &maxInputCols, &minInputRows, &maxInputRows,
                                   &numCols, &numRows, NULL);
        ok(rc == false, "pmReadoutStackValidate() returned FALSE with NULL pmArray input parameter");

        // NULL minInputColsPtr
        rc = pmReadoutStackValidate(NULL, &maxInputCols, &minInputRows, &maxInputRows,
                                    &numCols, &numRows, inputs);
        ok(rc == false, "pmReadoutStackValidate() returned FALSE with NULL minInputColsPtr input parameter");

        // NULL maxInputColsPtr
        rc = pmReadoutStackValidate(&minInputCols, NULL, &minInputRows, &maxInputRows,
                                    &numCols, &numRows, inputs);
        ok(rc == false, "pmReadoutStackValidate() returned FALSE with NULL maxInputColsPtr input parameter");

        // NULL minInputRowsPtr
        rc = pmReadoutStackValidate(&minInputCols, &maxInputCols, NULL, &maxInputRows,
                                    &numCols, &numRows, inputs);
        ok(rc == false, "pmReadoutStackValidate() returned FALSE with NULL minInputRowsPtr input parameter");

        // NULL maxInputRowsPtr
        rc = pmReadoutStackValidate(&minInputCols, &maxInputCols, &minInputRows, NULL,
                                    &numCols, &numRows, inputs);
        ok(rc == false, "pmReadoutStackValidate() returned FALSE with NULL maxInputRowsPtr input parameter");

        // NULL numColsPtr
        rc = pmReadoutStackValidate(&minInputCols, &maxInputCols, &minInputRows, &maxInputRows,
                                    NULL, &numRows, inputs);
        ok(rc == false, "pmReadoutStackValidate() returned FALSE with NULL numColsPtr input parameter");

        // NULL numRowsPtr
        rc = pmReadoutStackValidate(&minInputCols, &maxInputCols, &minInputRows, &maxInputRows,
                                    &numCols, NULL, inputs);
        ok(rc == false, "pmReadoutStackValidate() returned FALSE with NULL numRowsPtr input parameter");

        psFree(inputs);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmReadoutStackValidate() with acceptable input parameters.
    {
        psMemId id = psMemGetId();
        int minInputCols, maxInputCols, minInputRows, maxInputRows, numCols, numRows;
        minInputCols = maxInputCols = minInputRows = maxInputRows = numCols = numRows = 0;
        pmCell *cells[NUM_INPUTS];
        psArray *inputs = psArrayAlloc(NUM_INPUTS);
        for (int i = 0 ; i < NUM_INPUTS ; i++) {
            cells[i] = generateSimpleCell(i);
            inputs->data[i] = (psPtr *) generateSimpleReadout(cells[i]);
        }
        bool rc = pmReadoutStackValidate(&minInputCols, &maxInputCols, &minInputRows, &maxInputRows,
                                   &numCols, &numRows, inputs);
        ok(rc == true, "pmReadoutStackValidate() returned TRUE with acceptable input parameters");
        ok(minInputCols == 0, "pmReadoutStackValidate() set minInputCols correctly");
        ok(maxInputCols == TEST_NUM_COLS, "pmReadoutStackValidate() set maxInputCols correctly");
        ok(minInputRows == 0, "pmReadoutStackValidate() set minInputRows correctly");
        ok(maxInputRows == TEST_NUM_ROWS, "pmReadoutStackValidate() set maxInputRows correctly");
        ok(numCols == (10 + NUM_INPUTS - 1), "pmReadoutStackValidate() set numCols correctly");
        ok(numRows == (20 + NUM_INPUTS - 1), "pmReadoutStackValidate() set numRows correctly");

        for (int i = 0 ; i < NUM_INPUTS ; i++) {
            psFree(cells[i]);
        }
        psFree(inputs);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

}
