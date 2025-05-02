/** @file tap_pmBias.c
 *
 * XXX: pmBiasSubtractFrame(): Must add tests for CELL.X0, CELL.Y0 stuff:
 * XXX: pmBiasSubtractFrame(): Must add tests with scale != 1.0
 * XXX: pmBiasSubtract(): must test with acceptable data
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
#define ERR_TRACE_LEVEL         0
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
    psTraceSetLevel("err", ERR_TRACE_LEVEL);
    plan_tests(1);


    // ----------------------------------------------------------------------
    // pmBiasSubtractFrame() tests
    // bool pmBiasSubtractFrame(pmReadout *in, pmReadout *sub, float scale)
    // Call pmBiasSubtractFrame() with NULL pmReadout input
    {
        psMemId id = psMemGetId();
        pmCell *cell = generateSimpleCell(NULL);
        pmReadout *in = generateSimpleReadout(cell);
        pmReadout *sub = generateSimpleReadout(cell);

        ok(!pmBiasSubtractFrame(NULL, sub, 1.0), "pmBiasSubtractFrame(NULL, sub, 1.0) returned FALSE");
        psFree(in);
        psFree(sub);
        myFreeCell(cell);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // bool pmBiasSubtractFrame(pmReadout *in, pmReadout *sub, float scale)
    // Call pmBiasSubtractFrame() with NULL pmReadout input->image
    {
        psMemId id = psMemGetId();
        pmCell *cell = generateSimpleCell(NULL);
        pmReadout *in = generateSimpleReadout(cell);
        psFree(in->image);
        in->image = NULL;
        pmReadout *sub = generateSimpleReadout(cell);

        ok(!pmBiasSubtractFrame(in, sub, 1.0), "pmBiasSubtractFrame() returned FALSE with NULL pmReadout input->image");
        psFree(in);
        psFree(sub);
        myFreeCell(cell);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmBiasSubtractFrame() with empty pmReadout input->image
    {
        psMemId id = psMemGetId();
        pmCell *cell = generateSimpleCell(NULL);
        pmReadout *in = generateSimpleReadout(cell);
        psFree(in->image);
        in->image = psImageAlloc(0, 0, PS_TYPE_F32);
        pmReadout *sub = generateSimpleReadout(cell);

        ok(!pmBiasSubtractFrame(in, sub, 1.0), "pmBiasSubtractFrame() returned FALSE with empty input->image");
        psFree(in);
        psFree(sub);
        myFreeCell(cell);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmBiasSubtractFrame() with bad type for pmReadout input->image
    {
        psMemId id = psMemGetId();
        pmCell *cell = generateSimpleCell(NULL);
        pmReadout *in = generateSimpleReadout(cell);
        psFree(in->image);
        in->image = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_S32);
        pmReadout *sub = generateSimpleReadout(cell);

        ok(!pmBiasSubtractFrame(in, sub, 1.0), "pmBiasSubtractFrame() returned FALSE with bad type for pmReadout input->image");
        psFree(in);
        psFree(sub);
        myFreeCell(cell);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmBiasSubtractFrame() with NULL pmReadout sub
    {
        psMemId id = psMemGetId();
        pmCell *cell = generateSimpleCell(NULL);
        pmReadout *in = generateSimpleReadout(cell);
        pmReadout *sub = generateSimpleReadout(cell);

        ok(!pmBiasSubtractFrame(in, NULL, 1.0), "pmBiasSubtractFrame(in, NULL, 1.0) returned FALSE");
        psFree(in);
        psFree(sub);
        myFreeCell(cell);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmBiasSubtractFrame() with NULL pmReadout sub->image
    {
        psMemId id = psMemGetId();
        pmCell *cell = generateSimpleCell(NULL);
        pmReadout *in = generateSimpleReadout(cell);
        pmReadout *sub = generateSimpleReadout(cell);
        psFree(sub->image);
        sub->image = NULL;

        ok(!pmBiasSubtractFrame(in, sub, 1.0), "pmBiasSubtractFrame() returned FALSE with NULL pmReadout sub->image");
        psFree(in);
        psFree(sub);
        myFreeCell(cell);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmBiasSubtractFrame() with empty pmReadout sub->image
    {
        psMemId id = psMemGetId();
        pmCell *cell = generateSimpleCell(NULL);
        pmReadout *in = generateSimpleReadout(cell);
        pmReadout *sub = generateSimpleReadout(cell);
        psFree(sub->image);
        sub->image = psImageAlloc(0, 0, PS_TYPE_F32);

        ok(!pmBiasSubtractFrame(in, sub, 1.0), "pmBiasSubtractFrame() returned FALSE with NULL pmReadout sub->image");
        psFree(in);
        psFree(sub);
        myFreeCell(cell);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmBiasSubtractFrame() with bad type for pmReadout sub->image
    {
        psMemId id = psMemGetId();
        pmCell *cell = generateSimpleCell(NULL);
        pmReadout *in = generateSimpleReadout(cell);
        pmReadout *sub = generateSimpleReadout(cell);
        psFree(sub->image);
        sub->image = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_S32);

        ok(!pmBiasSubtractFrame(in, sub, 1.0), "pmBiasSubtractFrame() returned FALSE with bad type for pmReadout sub->image");
        psFree(in);
        psFree(sub);
        myFreeCell(cell);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmBiasSubtractFrame() with differing input image sizes
    {
        psMemId id = psMemGetId();
        pmCell *cell = generateSimpleCell(NULL);
        pmReadout *in = generateSimpleReadout(cell);
        pmReadout *sub = generateSimpleReadout(cell);
        psFree(sub->image);
        sub->image = psImageAlloc(TEST_NUM_COLS/2, TEST_NUM_ROWS/2, PS_TYPE_S32);

        ok(!pmBiasSubtractFrame(in, sub, 1.0), "pmBiasSubtractFrame() returned FALSE with with differing input image sizes");
        psFree(in);
        psFree(sub);
        myFreeCell(cell);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmBiasSubtractFrame() with differing image X parity
    {
        psMemId id = psMemGetId();
        pmCell *cellIn = generateSimpleCell(NULL);
        pmCell *cellSub = generateSimpleCell(NULL);
        pmReadout *in = generateSimpleReadout(cellIn);
        pmReadout *sub = generateSimpleReadout(cellSub);

        // Set the X parity differently for each pmReadout's parent cell
        psMetadataAddS32(in->parent->concepts, PS_LIST_TAIL, "CELL.XPARITY", PS_META_REPLACE, "", -1);
        psMetadataAddS32(sub->parent->concepts, PS_LIST_TAIL, "CELL.XPARITY", PS_META_REPLACE, "", 1);
        ok(!pmBiasSubtractFrame(in, sub, 1.0), "pmBiasSubtractFrame() returned FALSE with differing image X parity");
        psFree(in);
        psFree(sub);
        myFreeCell(cellIn);
        myFreeCell(cellSub);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmBiasSubtractFrame() with differing image Y parity
    {
        psMemId id = psMemGetId();
        pmCell *cellIn = generateSimpleCell(NULL);
        pmCell *cellSub = generateSimpleCell(NULL);
        pmReadout *in = generateSimpleReadout(cellIn);
        pmReadout *sub = generateSimpleReadout(cellSub);

        // Set the Y parity differently for each pmReadout's parent cell
        psMetadataAddS32(in->parent->concepts, PS_LIST_TAIL, "CELL.YPARITY", PS_META_REPLACE, "", -1);
        psMetadataAddS32(sub->parent->concepts, PS_LIST_TAIL, "CELL.YPARITY", PS_META_REPLACE, "", 1);
        ok(!pmBiasSubtractFrame(in, sub, 1.0), "pmBiasSubtractFrame() returned FALSE with differing image Y parity");
        psFree(in);
        psFree(sub);
        myFreeCell(cellIn);
        myFreeCell(cellSub);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmBiasSubtractFrame() with differing image Y parity
    {
        psMemId id = psMemGetId();
        pmCell *cellIn = generateSimpleCell(NULL);
        pmCell *cellSub = generateSimpleCell(NULL);
        pmReadout *in = generateSimpleReadout(cellIn);
        pmReadout *sub = generateSimpleReadout(cellSub);
        for (int i = 0 ; i < in->image->numRows ; i++) {
            for (int j = 0 ; j < in->image->numCols ; j++) {
                in->image->data.F32[i][j] = (float) (i + j + 10);
	    }
	}

        for (int i = 0 ; i < sub->image->numRows ; i++) {
            for (int j = 0 ; j < sub->image->numCols ; j++) {
                sub->image->data.F32[i][j] = 10.0;
	    }
	}

        ok(pmBiasSubtractFrame(in, sub, 1.0), "pmBiasSubtractFrame() returned TRUE");
        bool errorFlag = false;
        for (int i = 0 ; i < in->image->numRows ; i++) {
            for (int j = 0 ; j < in->image->numCols ; j++) {
                if (in->image->data.F32[i][j] != (float) (i + j)) {
                    if (VERBOSE) diag("ERROR: image[%d][%d] is %.2f, should be %.2f\n",
                        i, j, in->image->data.F32[i][j], (float) (i + j));
                    errorFlag = true;
		}
	    }
	}

        ok(!errorFlag, "pmBiasSubtractFrame() set the pixels correctly");
        psFree(in);
        psFree(sub);
        myFreeCell(cellIn);
        myFreeCell(cellSub);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // pmBiasSubtract() tests
    // bool pmBiasSubtract(pmReadout *in, pmOverscanOptions *overscanOpts,
    //               const pmReadout *bias, const pmReadout *dark, const pmFPAview *view)
    // Call pmBiasSubtract() with NULL pmReadout input
    {
        psMemId id = psMemGetId();
        pmCell *cell = generateSimpleCell(NULL);
        pmReadout *in = generateSimpleReadout(cell);
        pmReadout *bias = generateSimpleReadout(cell);
        pmReadout *dark = generateSimpleReadout(cell);
        pmFPAview *view = pmFPAviewAlloc(TEST_NUM_ROWS);
        // XXX: Must set the following:
        pmOverscanOptions *overscanOpts = NULL;

        ok(!pmBiasSubtract(NULL, overscanOpts, bias, dark, view), "pmBiasSubtract() returned FALSE with NULL input pmReadout: in");
        psFree(in);
        psFree(bias);
        psFree(dark);
        myFreeCell(cell);
        psFree(view);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmBiasSubtract() with NULL pmReadout input->image
    {
        psMemId id = psMemGetId();
        pmCell *cell = generateSimpleCell(NULL);
        pmReadout *in = generateSimpleReadout(cell);
        psFree(in->image);
        in->image = NULL;
        pmReadout *bias = generateSimpleReadout(cell);
        pmReadout *dark = generateSimpleReadout(cell);
        pmFPAview *view = pmFPAviewAlloc(TEST_NUM_ROWS);
        // XXX: Must set the following:
        pmOverscanOptions *overscanOpts = NULL;

        ok(!pmBiasSubtract(in, overscanOpts, bias, dark, view), "pmBiasSubtract() returned FALSE with NULL input image");
        psFree(in);
        psFree(bias);
        psFree(dark);
        myFreeCell(cell);
        psFree(view);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmBiasSubtract() with wrong type for pmReadout input->image
    {
        psMemId id = psMemGetId();
        pmCell *cell = generateSimpleCell(NULL);
        pmReadout *in = generateSimpleReadout(cell);
        psFree(in->image);
        in->image = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_S32);
        pmReadout *bias = generateSimpleReadout(cell);
        pmReadout *dark = generateSimpleReadout(cell);
        pmFPAview *view = pmFPAviewAlloc(TEST_NUM_ROWS);
        // XXX: Must set the following:
        pmOverscanOptions *overscanOpts = NULL;

        ok(!pmBiasSubtract(in, overscanOpts, bias, dark, view), "pmBiasSubtract() returned FALSE with wrong type for pmReadout input->image");
        psFree(in);
        psFree(bias);
        psFree(dark);
        myFreeCell(cell);
        psFree(view);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmBiasSubtract() with NULL image for input pmReadout bias
    {
        psMemId id = psMemGetId();
        pmCell *cell = generateSimpleCell(NULL);
        pmReadout *in = generateSimpleReadout(cell);
        pmReadout *bias = generateSimpleReadout(cell);
        psFree(bias->image);
        bias->image = NULL;
        pmReadout *dark = generateSimpleReadout(cell);
        pmFPAview *view = pmFPAviewAlloc(TEST_NUM_ROWS);
        // XXX: Must set the following:
        pmOverscanOptions *overscanOpts = NULL;

        ok(!pmBiasSubtract(in, overscanOpts, bias, dark, view), "pmBiasSubtract() returned FALSE with NULL image for input pmReadout bias");
        psFree(in);
        psFree(bias);
        psFree(dark);
        myFreeCell(cell);
        psFree(view);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmBiasSubtract() with wrong type image for input pmReadout bias
    {
        psMemId id = psMemGetId();
        pmCell *cell = generateSimpleCell(NULL);
        pmReadout *in = generateSimpleReadout(cell);
        pmReadout *bias = generateSimpleReadout(cell);
        psFree(bias->image);
        bias->image = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_S32);
        pmReadout *dark = generateSimpleReadout(cell);
        pmFPAview *view = pmFPAviewAlloc(TEST_NUM_ROWS);
        // XXX: Must set the following:
        pmOverscanOptions *overscanOpts = NULL;

        ok(!pmBiasSubtract(in, overscanOpts, bias, dark, view), "pmBiasSubtract() returned FALSE with wrong type image for input pmReadout bias");
        psFree(in);
        psFree(bias);
        psFree(dark);
        myFreeCell(cell);
        psFree(view);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmBiasSubtract() with NULL image for input pmReadout dark
    {
        psMemId id = psMemGetId();
        pmCell *cell = generateSimpleCell(NULL);
        pmReadout *in = generateSimpleReadout(cell);
        pmReadout *bias = generateSimpleReadout(cell);
        pmReadout *dark = generateSimpleReadout(cell);
        psFree(dark->image);
        dark->image = NULL;
        pmFPAview *view = pmFPAviewAlloc(TEST_NUM_ROWS);
        // XXX: Must set the following:
        pmOverscanOptions *overscanOpts = NULL;

        ok(!pmBiasSubtract(in, overscanOpts, bias, dark, view), "pmBiasSubtract() returned FALSE with wrong type image for input pmReadout bias");
        psFree(in);
        psFree(bias);
        psFree(dark);
        myFreeCell(cell);
        psFree(view);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmBiasSubtract() with wrong type image for input pmReadout dark
    {
        psMemId id = psMemGetId();
        pmCell *cell = generateSimpleCell(NULL);
        pmReadout *in = generateSimpleReadout(cell);
        pmReadout *bias = generateSimpleReadout(cell);
        pmReadout *dark = generateSimpleReadout(cell);
        psFree(dark->image);
        dark->image = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_S32);
        pmFPAview *view = pmFPAviewAlloc(TEST_NUM_ROWS);
        // XXX: Must set the following:
        pmOverscanOptions *overscanOpts = NULL;

        ok(!pmBiasSubtract(in, overscanOpts, bias, dark, view), "pmBiasSubtract() returned FALSE with wrong type image for input pmReadout dark");
        psFree(in);
        psFree(bias);
        psFree(dark);
        myFreeCell(cell);
        psFree(view);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmBiasSubtract() with NULL pmView (while pmReadout dark non-NULL)
    {
        psMemId id = psMemGetId();
        pmCell *cell = generateSimpleCell(NULL);
        pmReadout *in = generateSimpleReadout(cell);
        pmReadout *bias = generateSimpleReadout(cell);
        pmReadout *dark = generateSimpleReadout(cell);
        pmFPAview *view = pmFPAviewAlloc(TEST_NUM_ROWS);
        // XXX: Must set the following:
        pmOverscanOptions *overscanOpts = NULL;

        ok(!pmBiasSubtract(in, overscanOpts, bias, dark, NULL), "pmBiasSubtract() returned FALSE with NULL pmView (while pmReadout dark non-NULL)");
        psFree(in);
        psFree(bias);
        psFree(dark);
        myFreeCell(cell);
        psFree(view);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
}

