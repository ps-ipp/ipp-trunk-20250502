/** @file tap_pmFlatField.c
 *
 * XXX: Added a mask argument to pmFlatField().  Must add tests.  For now, all
 * masks are NULL.
 *
 *  XXX: I added the CELL.TRIMSEC region code but there are not tests for it.
 *
 */
#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include <psmodules.h>
#include "tap.h"
#include "pstap.h"
#define BADFLAT			0
#define VERBOSE                 1
#define ERR_TRACE_LEVEL         10
#define TEST_NUM_ROWS		8
#define TEST_NUM_COLS		8
#define NUM_BIAS_DATA		2
#define NUM_HDUS		8
#define MISC_NUM                32
#define MISC_NAME              "META00"
#define CELL_ALLOC_NAME        "CellName"
#define NUM_READOUTS            3
#define BASE_IMAGE              10
#define BASE_MASK               40
#define BASE_WEIGHT             70

/******************************************************************************
generateSimpleReadout(): This function generates a pmReadout data structure and then
populates its members with real data.
 *****************************************************************************/
pmReadout *generateSimpleReadout(pmCell *cell)
{
    pmReadout *readout = pmReadoutAlloc(cell);
    readout->image = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_F32);
    for (int i=0;i<TEST_NUM_ROWS;i++) {
        for (int j=0;j<TEST_NUM_COLS;j++) {
            readout->image->data.F32[i][j] = (float) (i + j);
        }
    }
    readout->mask = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_U8);
    readout->weight = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_F32);
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
        cell->readouts->data[i] = generateSimpleReadout(cell);
    }

    bool rc = pmConfigFileRead(&cell->hdu->format, "../camera/data/camera0/format0.config", "Camera format 0");
    if (!rc) {
        diag("pmConfigFileRead() was unsuccessful (from generateSimpleCell())");
    }

    cell->hdu->images = psArrayAlloc(NUM_HDUS);
    cell->hdu->masks = psArrayAlloc(NUM_HDUS);
    cell->hdu->weights = psArrayAlloc(NUM_HDUS);
    for (int k = 0 ; k < NUM_HDUS ; k++) {
        cell->hdu->images->data[k]  = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_F32);
        cell->hdu->masks->data[k]   = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_MASK);
        cell->hdu->weights->data[k] = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_F32);
        psImageInit(cell->hdu->images->data[k], (float) (BASE_IMAGE+k));
        psImageInit(cell->hdu->masks->data[k], (psU8) (BASE_MASK+k));
        psImageInit(cell->hdu->weights->data[k], (float) (BASE_WEIGHT+k));
    }

    //XXX: Should the region be set some other way?  Like through the various config files?
//    psRegion *region = psRegionAlloc(0.0, TEST_NUM_COLS-1, 0.0, TEST_NUM_ROWS-1);
    psRegion *region = psRegionAlloc(0.0, 0.0, 0.0, 0.0);
    // You shouldn't have to remove the key from the metadata.  Find out how to simply change the key value.
    psMetadataRemoveKey(cell->concepts, "CELL.TRIMSEC");
    psMetadataAddPtr(cell->concepts, PS_LIST_TAIL|PS_META_REPLACE, "CELL.TRIMSEC", PS_DATA_REGION, "I am a region", region);
    psFree(region);
    return(cell);
}

void myFreeCell(pmCell *cell)
{
    for (int k = 0 ; k < cell->readouts->n ; k++) {
        psFree(cell->readouts->data[k]);
    }
    psFree(cell);
}

int main(int argc, char* argv[])
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    plan_tests(28);


    // bool pmFlatField(pmReadout *in, pmReadout *flat, psMaskType badFlat)
    // Test pmFlatField() with NULL input psReadout
    {
        psMemId id = psMemGetId();
        pmReadout *in = generateSimpleReadout(NULL);
        pmReadout *flat = generateSimpleReadout(NULL);
        ok(!pmFlatField(NULL, flat, 0), "pmFlatField(NULL, flat, 0) returned FALSE");
        psFree(in);
        psFree(flat);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // bool pmFlatField(pmReadout *in, pmReadout *flat, psMaskType badFlat)
    // Test pmFlatField() with NULL input psReadout->image
    {
        psMemId id = psMemGetId();
        pmReadout *in = generateSimpleReadout(NULL);
        psFree(in->image);
        in->image = NULL;
        pmReadout *flat = generateSimpleReadout(NULL);
        ok(!pmFlatField(in, flat, 0), "pmFlatField(in, flat, 0) returned FALSE with NULL input psReadout->image");
        psFree(in);
        psFree(flat);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // bool pmFlatField(pmReadout *in, pmReadout *flat, psMaskType badFlat)
    // Test pmFlatField() with NULL input psReadout->image
    {
        psMemId id = psMemGetId();
        pmReadout *in = generateSimpleReadout(NULL);
        psFree(in->image);
        in->image = psImageAlloc(0, 0, PS_TYPE_F32);
        pmReadout *flat = generateSimpleReadout(NULL);
        ok(!pmFlatField(in, flat, 0), "pmFlatField(in, flat, 0) returned FALSE with empty input psReadout->image");
        psFree(in);
        psFree(flat);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // bool pmFlatField(pmReadout *in, pmReadout *flat, psMaskType badFlat)
    // Test pmFlatField() with NULL input flat
    {
        psMemId id = psMemGetId();
        pmReadout *in = generateSimpleReadout(NULL);
        pmReadout *flat = generateSimpleReadout(NULL);
        ok(!pmFlatField(in, NULL, 0), "pmFlatField(in, NULL, 0) returned FALSE");
        psFree(in);
        psFree(flat);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // bool pmFlatField(pmReadout *in, pmReadout *flat, psMaskType badFlat)
    // Test pmFlatField() with NULL input flat->image
    {
        psMemId id = psMemGetId();
        pmReadout *in = generateSimpleReadout(NULL);
        pmReadout *flat = generateSimpleReadout(NULL);
        psFree(flat->image);
        flat->image = NULL;
        ok(!pmFlatField(in, NULL, 0), "pmFlatField(in, flat, 0) returned FALSE with NULL input flat->image");
        psFree(in);
        psFree(flat);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // bool pmFlatField(pmReadout *in, pmReadout *flat, psMaskType badFlat)
    // Test pmFlatField() with NULL input flat->image
    {
        psMemId id = psMemGetId();
        pmReadout *in = generateSimpleReadout(NULL);
        pmReadout *flat = generateSimpleReadout(NULL);
        psFree(flat->image);
        flat->image = psImageAlloc(0, 0, PS_TYPE_F32);
        ok(!pmFlatField(in, NULL, 0), "pmFlatField(in, flat, 0) returned FALSE with empty input flat->image");
        psFree(in);
        psFree(flat);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // bool pmFlatField(pmReadout *in, pmReadout *flat, psMaskType badFlat)
    // Test pmFlatField() with differing types of flat/in image
    {
        psMemId id = psMemGetId();
        pmReadout *in = generateSimpleReadout(NULL);
        pmReadout *flat = generateSimpleReadout(NULL);
        psFree(flat->image);
        flat->image = psImageAlloc(0, 0, PS_TYPE_F64);
        ok(!pmFlatField(in, NULL, 0), "pmFlatField(in, flat, 0) returned FALSE with differing types of flat/in image");
        psFree(in);
        psFree(flat);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Test pmFlatField() with input image larger than flat image
    {
        psMemId id = psMemGetId();
        pmReadout *in = generateSimpleReadout(NULL);
        pmReadout *flat = generateSimpleReadout(NULL);
        psFree(flat->image);
        flat->image = psImageAlloc(TEST_NUM_ROWS/2, TEST_NUM_COLS/2, PS_TYPE_F32);
        ok(!pmFlatField(in, flat, 1), "pmFlatField(in, flat, 0) returned FALSE with input image larger than flat image");
        psFree(in);
        psFree(flat);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Test pmFlatField() with input image and flat image of differing types
    {
        psMemId id = psMemGetId();
        pmReadout *in = generateSimpleReadout(NULL);
        pmReadout *flat = generateSimpleReadout(NULL);
        psFree(flat->image);
        flat->image = psImageAlloc(TEST_NUM_ROWS, TEST_NUM_COLS, PS_TYPE_F64);
        ok(!pmFlatField(in, flat, 1), "pmFlatField(in, flat, 0) returned FALSE with input image and flat image of differing types");
        psFree(in);
        psFree(flat);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Test pmFlatField() with input image mask smaller than input image
    {
        psMemId id = psMemGetId();
        pmReadout *in = generateSimpleReadout(NULL);
        pmReadout *flat = generateSimpleReadout(NULL);
        psFree(in->mask);
        in->mask = psImageAlloc(TEST_NUM_ROWS/2, TEST_NUM_COLS/2, PS_TYPE_MASK);
        ok(!pmFlatField(in, flat, 1), "pmFlatField(in, flat, 0) returned FALSE with input image mask smaller than input image");
        psFree(in);
        psFree(flat);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Test pmFlatField() with input image mask of incorrect type
    {
        psMemId id = psMemGetId();
        pmReadout *in = generateSimpleReadout(NULL);
        pmReadout *flat = generateSimpleReadout(NULL);
        psFree(in->mask);
        in->mask = psImageAlloc(TEST_NUM_ROWS, TEST_NUM_COLS, PS_TYPE_F32);
        ok(!pmFlatField(in, flat, 1), "pmFlatField(in, flat, 0) returned FALSE with input image mask of incorrect type");
        psFree(in);
        psFree(flat);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Test pmFlatField() with offset greater than input image
    // XXX: Must rewrite with offsets coming from metadata
    if (0) {
        psMemId id = psMemGetId();
        pmReadout *in = generateSimpleReadout(NULL);
        pmReadout *flat = generateSimpleReadout(NULL);
        *(int*)&in->col0 = 50;
        *(int*)&in->row0 = 50;
        ok(!pmFlatField(in, flat, 1), "pmFlatField(in, flat, 0) returned FALSE with offset greater than input image");
        psFree(in);
        psFree(flat);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Test pmFlatField() with acceptable input data
    // Set the flat field to 1
    {
        psMemId id = psMemGetId();
        pmCell *cell = generateSimpleCell(NULL);
        pmReadout *in = generateSimpleReadout(cell);
        pmReadout *flat = generateSimpleReadout(cell);
        psImageInit(in->image, 6.0);
        psImageInit(in->mask, 0);
        psImageInit(flat->image, 2.0);
        ok(pmFlatField(in, flat, 1), "pmFlatField(in, flat, 0) returned TRUE with acceptable input data");
        bool errorFlag = false;
        for (int i = 0 ; i < in->image->numRows ; i++) {
            for (int j = 0 ; j < in->image->numCols ; j++) {
                if (in->image->data.F32[i][j] != 3.0) {
                    if (VERBOSE) diag("ERROR: image[%d][%d] is %.2f, should be 3.0", i, j,
                        in->image->data.F32[i][j]);
		}
                if (in->mask->data.PS_TYPE_MASK_DATA[i][j] != 0) {
                    if (VERBOSE) diag("ERROR: mask[%d][%d] is %d, should be 0", i, j,
                        in->image->data.PS_TYPE_MASK_DATA[i][j]);
		}
	    }
	}
        ok(!errorFlag, "pmFlatField() set the image data correctly");

        psFree(in);
        psFree(flat);
        myFreeCell(cell);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Test pmFlatField() with acceptable input data
    // Set the flat field to -1
    {
        psMemId id = psMemGetId();
        pmCell *cell = generateSimpleCell(NULL);
        pmReadout *in = generateSimpleReadout(cell);
        pmReadout *flat = generateSimpleReadout(cell);
        psImageInit(in->image, 6.0);
        psImageInit(in->mask, 0);
        psImageInit(flat->image, -1.0);
        ok(pmFlatField(in, flat, 1), "pmFlatField(in, flat, 0) returned TRUE with acceptable input data");
        bool errorFlag = false;
        for (int i = 0 ; i < in->image->numRows ; i++) {
            for (int j = 0 ; j < in->image->numCols ; j++) {
                if (!isnan(in->image->data.F32[i][j])) {
                    if (VERBOSE) diag("ERROR: image[%d][%d] is %.2f, should be NAN", i, j,
                        in->image->data.F32[i][j]);
		}
                if (in->mask->data.PS_TYPE_MASK_DATA[i][j] != 1) {
                    if (VERBOSE) diag("ERROR: mask[%d][%d] is %d, should be 0", i, j,
                        in->image->data.PS_TYPE_MASK_DATA[i][j]);
		}
	    }
	}
        ok(!errorFlag, "pmFlatField() set the image data correctly");

        psFree(in);
        psFree(flat);
        myFreeCell(cell);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
}
