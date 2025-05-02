/** @file tap_pmMaskBadPixels.c
 *
 * XXX: must test pmMaskFlagSuspectPixels() with acceptable input data
 * XXX: must test pmMaskIdentifyBadPixels() with acceptable input data
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
    // pmMaskBadPixels() tests
    // Call pmMaskBadPixels() with NULL pmReadout input
    {
        psMemId id = psMemGetId();
        pmCell *cell = generateSimpleCell(NULL);
        pmReadout *in = generateSimpleReadout(cell);
        pmReadout *mask = generateSimpleReadout(cell);

        ok(!pmMaskBadPixels(NULL, mask, 0), "pmMaskBadPixels(NULL, mask, 0) returned FALSE");
        psFree(in);
        psFree(mask);
        myFreeCell(cell);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmMaskBadPixels() with NULL pmReadout input->mask
    {
        psMemId id = psMemGetId();
        pmCell *cell = generateSimpleCell(NULL);
        pmReadout *in = generateSimpleReadout(cell);
        psFree(in->mask);
        in->mask = NULL;
        pmReadout *mask = generateSimpleReadout(cell);

        ok(!pmMaskBadPixels(in, mask, 0), "pmMaskBadPixels() returned FALSE with NULL pmReadout input->mask");
        psFree(in);
        psFree(mask);
        myFreeCell(cell);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmMaskBadPixels() with incorrect type for input->mask
    {
        psMemId id = psMemGetId();
        pmCell *cell = generateSimpleCell(NULL);
        pmReadout *in = generateSimpleReadout(cell);
        psFree(in->mask);
        in->mask = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_F32);
        pmReadout *mask = generateSimpleReadout(cell);

        ok(!pmMaskBadPixels(in, mask, 0), "pmMaskBadPixels() returned FALSE with NULL pmReadout input->mask");
        psFree(in);
        psFree(mask);
        myFreeCell(cell);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmMaskBadPixels() with NULL pmReadout mask
    {
        psMemId id = psMemGetId();
        pmCell *cell = generateSimpleCell(NULL);
        pmReadout *in = generateSimpleReadout(cell);
        pmReadout *mask = generateSimpleReadout(cell);

        ok(!pmMaskBadPixels(in, NULL, 0), "pmMaskBadPixels(in, NULL, 0) returned FALSE");
        psFree(in);
        psFree(mask);
        myFreeCell(cell);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmMaskBadPixels() with NULL pmReadout mask->mask
    {
        psMemId id = psMemGetId();
        pmCell *cell = generateSimpleCell(NULL);
        pmReadout *in = generateSimpleReadout(cell);
        pmReadout *mask = generateSimpleReadout(cell);
        psFree(mask->mask);
        mask->mask = NULL;

        ok(!pmMaskBadPixels(in, mask, 0), "pmMaskBadPixels() returned FALSE with NULL pmReadout mask->mask");
        psFree(in);
        psFree(mask);
        myFreeCell(cell);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmMaskBadPixels() with incorrect type for mask->mask
    {
        psMemId id = psMemGetId();
        pmCell *cell = generateSimpleCell(NULL);
        pmReadout *in = generateSimpleReadout(cell);
        pmReadout *mask = generateSimpleReadout(cell);
        psFree(mask->mask);
        mask->mask = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_F32);

        ok(!pmMaskBadPixels(in, mask, 0), "pmMaskBadPixels() returned FALSE with incorrect type for mask->mask");
        psFree(in);
        psFree(mask);
        myFreeCell(cell);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmMaskBadPixels() with input mask larger than mask->mask
    {
        psMemId id = psMemGetId();
        pmCell *cell = generateSimpleCell(NULL);
        pmReadout *in = generateSimpleReadout(cell);
        pmReadout *mask = generateSimpleReadout(cell);
        psFree(mask->mask);
        mask->mask = psImageAlloc(TEST_NUM_COLS/2, TEST_NUM_ROWS/2, PS_TYPE_MASK);

        ok(!pmMaskBadPixels(in, mask, 0), "pmMaskBadPixels() returned FALSE with input mask larger than mask->mask");
        psFree(in);
        psFree(mask);
        myFreeCell(cell);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmMaskBadPixels() with acceptable input data
    {
        #define MASK_VAL 1
        psMemId id = psMemGetId();
        pmCell *cell = generateSimpleCell(NULL);
        pmReadout *in = generateSimpleReadout(cell);
        pmReadout *mask = generateSimpleReadout(cell);
        for (int i = 0 ; i < in->mask->numRows ; i++) {
            for (int j = 0 ; j < in->mask->numCols ; j++) {
                in->mask->data.PS_TYPE_MASK_DATA[i][j] = 0;
                mask->mask->data.PS_TYPE_MASK_DATA[i][j] = 0;
                if (i >= in->mask->numRows/2 && j >= in->mask->numCols/2) {
                    mask->mask->data.PS_TYPE_MASK_DATA[i][j] = MASK_VAL;
		}
	    }
	}

        ok(pmMaskBadPixels(in, mask, MASK_VAL), "pmMaskBadPixels() returned TRUE with acceptable input data");
        bool errorFlag = false;     
        for (int i = 0 ; i < in->mask->numRows ; i++) {
            for (int j = 0 ; j < in->mask->numCols ; j++) {
                psU8 expect;
                if (i >= in->mask->numRows/2 && j >= in->mask->numCols/2) {
                    expect = MASK_VAL;
		} else {
                    expect = 0;
		}
                if (in->mask->data.PS_TYPE_MASK_DATA[i][j] != expect) {
                    if (VERBOSE) diag("ERROR: in->mask[%d][%d] is %d, should be %d",
                                       i, j, in->mask->data.PS_TYPE_MASK_DATA[i][j], expect);
                    errorFlag = true;
		}
	    }
	}
        ok(!errorFlag, "pmMaskBadPixels() set the input mask correctly");

        psFree(in);
        psFree(mask);
        myFreeCell(cell);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }




    // ----------------------------------------------------------------------
    // pmMaskFlagSuspectPixels() tests
    // Call pmMaskFlagSuspectPixels() with NULL pmReadout input
    {
        psMemId id = psMemGetId();
        psImage *out = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_S32);
        pmCell *cell = generateSimpleCell(NULL);
        pmReadout *in = generateSimpleReadout(cell);
        psRandom *rng = psRandomAllocSpecific(PS_RANDOM_TAUS, 0);

        ok(!pmMaskFlagSuspectPixels(out, NULL, 1.0, 1, 1.0, rng), "pmMaskFlagSuspectPixels() returned NULL with NULL input pmReadout");
        psFree(rng);
        psFree(out);
        psFree(in);
        myFreeCell(cell);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmMaskFlagSuspectPixels() with NULL pmReadout->image
    {
        psMemId id = psMemGetId();
        psImage *out = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_S32);
        pmCell *cell = generateSimpleCell(NULL);
        pmReadout *in = generateSimpleReadout(cell);
        psFree(in->image);
        in->image = NULL;
        psRandom *rng = psRandomAllocSpecific(PS_RANDOM_TAUS, 0);
        ok(!pmMaskFlagSuspectPixels(out, NULL, 1.0, 1, 1.0, rng), "pmMaskFlagSuspectPixels() returned NULL with NULL input pmReadout->image");
        psFree(rng);
        psFree(out);
        psFree(in);
        myFreeCell(cell);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmMaskFlagSuspectPixels() with empty pmReadout->image
    {
        psMemId id = psMemGetId();
        psImage *out = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_S32);
        pmCell *cell = generateSimpleCell(NULL);
        pmReadout *in = generateSimpleReadout(cell);
        psFree(in->image);
        in->image = psImageAlloc(0, 0, PS_TYPE_F32);
        psRandom *rng = psRandomAllocSpecific(PS_RANDOM_TAUS, 0);
        ok(!pmMaskFlagSuspectPixels(out, NULL, 1.0, 1, 1.0, rng), "pmMaskFlagSuspectPixels() returned NULL with empty input pmReadout->image");
        psFree(rng);
        psFree(out);
        psFree(in);
        myFreeCell(cell);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmMaskFlagSuspectPixels() with incorrect type for pmReadout->image
    {
        psMemId id = psMemGetId();
        psImage *out = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_S32);
        pmCell *cell = generateSimpleCell(NULL);
        pmReadout *in = generateSimpleReadout(cell);
        psFree(in->image);
        in->image = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_S32);
        psRandom *rng = psRandomAllocSpecific(PS_RANDOM_TAUS, 0);
        ok(!pmMaskFlagSuspectPixels(out, NULL, 1.0, 1, 1.0, rng), "pmMaskFlagSuspectPixels() returned NULL with incorrect type for pmReadout->image");
        psFree(rng);
        psFree(out);
        psFree(in);
        myFreeCell(cell);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmMaskFlagSuspectPixels() with empty in->mask
    {
        psMemId id = psMemGetId();
        psImage *out = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_S32);
        pmCell *cell = generateSimpleCell(NULL);
        pmReadout *in = generateSimpleReadout(cell);
        psFree(in->mask);
        in->mask = psImageAlloc(0, 0, PS_TYPE_MASK);
        psRandom *rng = psRandomAllocSpecific(PS_RANDOM_TAUS, 0);
        ok(!pmMaskFlagSuspectPixels(out, NULL, 1.0, 1, 1.0, rng), "pmMaskFlagSuspectPixels() returned NULL with incorrect type for pmReadout->image");
        psFree(rng);
        psFree(out);
        psFree(in);
        myFreeCell(cell);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmMaskFlagSuspectPixels() with bad size for in->mask
    {
        psMemId id = psMemGetId();
        psImage *out = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_S32);
        pmCell *cell = generateSimpleCell(NULL);
        pmReadout *in = generateSimpleReadout(cell);
        psFree(in->mask);
        in->mask = psImageAlloc(TEST_NUM_COLS/2, TEST_NUM_ROWS/2, PS_TYPE_MASK);
        psRandom *rng = psRandomAllocSpecific(PS_RANDOM_TAUS, 0);
        ok(!pmMaskFlagSuspectPixels(out, NULL, 1.0, 1, 1.0, rng), "pmMaskFlagSuspectPixels() returned NULL with bad size for in->mask");
        psFree(rng);
        psFree(out);
        psFree(in);
        myFreeCell(cell);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmMaskFlagSuspectPixels() with bad type for in->mask
    {
        psMemId id = psMemGetId();
        psImage *out = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_S32);
        pmCell *cell = generateSimpleCell(NULL);
        pmReadout *in = generateSimpleReadout(cell);
        psFree(in->mask);
        in->mask = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_S32);
        psRandom *rng = psRandomAllocSpecific(PS_RANDOM_TAUS, 0);
        ok(!pmMaskFlagSuspectPixels(out, NULL, 1.0, 1, 1.0, rng), "pmMaskFlagSuspectPixels() returned NULL with bad type for in->mask");
        psFree(rng);
        psFree(out);
        psFree(in);
        myFreeCell(cell);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmMaskFlagSuspectPixels() with empty out image
    {
        psMemId id = psMemGetId();
        psImage *out = psImageAlloc(0, 0, PS_TYPE_S32);
        pmCell *cell = generateSimpleCell(NULL);
        pmReadout *in = generateSimpleReadout(cell);
        psRandom *rng = psRandomAllocSpecific(PS_RANDOM_TAUS, 0);
        ok(!pmMaskFlagSuspectPixels(out, NULL, 1.0, 1, 1.0, rng), "pmMaskFlagSuspectPixels() returned NULL with empty out image");
        psFree(rng);
        psFree(out);
        psFree(in);
        myFreeCell(cell);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmMaskFlagSuspectPixels() with incorrect type for out image
    {
        psMemId id = psMemGetId();
        psImage *out = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_F32);
        pmCell *cell = generateSimpleCell(NULL);
        pmReadout *in = generateSimpleReadout(cell);
        psRandom *rng = psRandomAllocSpecific(PS_RANDOM_TAUS, 0);
        ok(!pmMaskFlagSuspectPixels(out, NULL, 1.0, 1, 1.0, rng), "pmMaskFlagSuspectPixels() returned NULL with incorrect type for out image");
        psFree(rng);
        psFree(out);
        psFree(in);
        myFreeCell(cell);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmMaskFlagSuspectPixels() with incorrect size for out image
    {
        psMemId id = psMemGetId();
        psImage *out = psImageAlloc(TEST_NUM_COLS/2, TEST_NUM_ROWS/2, PS_TYPE_S32);
        pmCell *cell = generateSimpleCell(NULL);
        pmReadout *in = generateSimpleReadout(cell);
        psRandom *rng = psRandomAllocSpecific(PS_RANDOM_TAUS, 0);
        ok(!pmMaskFlagSuspectPixels(out, NULL, 1.0, 1, 1.0, rng), "pmMaskFlagSuspectPixels() returned NULL with incorrect size for out image");
        psFree(rng);
        psFree(out);
        psFree(in);
        myFreeCell(cell);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmMaskFlagSuspectPixels() with rej input <= 0.0
    {
        psMemId id = psMemGetId();
        psImage *out = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_S32);
        pmCell *cell = generateSimpleCell(NULL);
        pmReadout *in = generateSimpleReadout(cell);
        psRandom *rng = psRandomAllocSpecific(PS_RANDOM_TAUS, 0);

        ok(!pmMaskFlagSuspectPixels(out, in, 0.0, 1, 1.0, rng), "pmMaskFlagSuspectPixels() returned NULL with rej input <= 0.0");
        psFree(rng);
        psFree(out);
        psFree(in);
        myFreeCell(cell);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmMaskFlagSuspectPixels() with frac input outside range [0.0:1.0]
    {
        psMemId id = psMemGetId();
        psImage *out = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_S32);
        pmCell *cell = generateSimpleCell(NULL);
        pmReadout *in = generateSimpleReadout(cell);
        psRandom *rng = psRandomAllocSpecific(PS_RANDOM_TAUS, 0);

        ok(!pmMaskFlagSuspectPixels(out, in, 1.0, 1, -1.0, rng), "pmMaskFlagSuspectPixels() returned NULL with frac input < 0.0");
        ok(!pmMaskFlagSuspectPixels(out, in, 1.0, 1, 2.0, rng), "pmMaskFlagSuspectPixels() returned NULL with frac input > 1.0");
        psFree(rng);
        psFree(out);
        psFree(in);
        myFreeCell(cell);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }




    // ----------------------------------------------------------------------
    // pmMaskIdentifyBadPixels() tests
    // Call pmMaskIdentifyBadPixels() with NULL input image
    {
        psMemId id = psMemGetId();
        psImage *suspects = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_S32);
        ok(!pmMaskIdentifyBadPixels(NULL, 1.0, 0), "pmMaskIdentifyBadPixels() returned NULL with NULL input image");
        psFree(suspects);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmMaskIdentifyBadPixels() with empty input image
    {
        psMemId id = psMemGetId();
        psImage *suspects = psImageAlloc(0, 0, PS_TYPE_S32);
        ok(!pmMaskIdentifyBadPixels(NULL, 1.0, 0), "pmMaskIdentifyBadPixels() returned NULL with empty input image");
        psFree(suspects);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmMaskIdentifyBadPixels() with incorrect type for input image
    {
        psMemId id = psMemGetId();
        psImage *suspects = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_F32);
        ok(!pmMaskIdentifyBadPixels(NULL, 1.0, 0), "pmMaskIdentifyBadPixels() returned NULL with incorrect type for input image");
        psFree(suspects);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

}

