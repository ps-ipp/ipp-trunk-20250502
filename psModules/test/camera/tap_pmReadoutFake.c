#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include <psmodules.h>
#include "tap.h"
#include "pstap.h"
/* STATUS:
    Only one function in the source code: pmReadoutFakeFromSources()

    pmReadoutFakeFromSources() is only tested with bad input parameters.
    Tests must be written to exercise it with legitimate input sources, and
    verify the output.
*/

#define MISC_NUM                32
#define MISC_NAME               "META00"
#define NUM_BIAS_DATA           10
#define TEST_NUM_ROWS           5
#define TEST_NUM_COLS           8
#define VERBOSE                 0
#define ERR_TRACE_LEVEL         0
#define NUM_SOURCES		5

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

psS32 main(psS32 argc, char* argv[])
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    psTraceSetLevel("err", ERR_TRACE_LEVEL);
    plan_tests(11);

    // ------------------------------------------------------------------------
    // pmReadoutFakeFromSources() tests
    // bool pmReadoutFakeFromSources(pmReadout *readout, int numCols, int numRows, const psArray *sources,
    //                               const psVector *xOffset, const psVector *yOffset, const pmPSF *psf,
    //                               float minFlux, int radius, bool circularise)
    //
    // Call pmReadoutFakeFromSources() with bad input parameters.
    {
        psMemId id = psMemGetId();
        pmReadout *readout = generateSimpleReadout(NULL);
        psVector *xOffset = psVectorAlloc(NUM_SOURCES, PS_TYPE_S32);
        psVector *yOffset = psVectorAlloc(NUM_SOURCES, PS_TYPE_S32);
        psVector *xOffsetBig = psVectorAlloc(NUM_SOURCES*2, PS_TYPE_S32);
        psVector *xOffsetF32 = psVectorAlloc(NUM_SOURCES, PS_TYPE_F32);
        psVector *yOffsetF32 = psVectorAlloc(NUM_SOURCES, PS_TYPE_F32);
        psArray *sources = psArrayAlloc(NUM_SOURCES);
        for (int i = 0; i < sources->n ; i++) {
            sources->data[i] = pmSourceAlloc();
        }
        pmPSFOptions *psfOptions = pmPSFOptionsAlloc();
        psfOptions->psfTrendNx = 1;
        psfOptions->psfTrendNy = 2;
        psfOptions->psfFieldNx = 3;
        psfOptions->psfFieldNy = 4;
        psfOptions->psfFieldXo = 5;
        psfOptions->psfFieldYo = 6;
        pmModelClassInit();
        psfOptions->type = pmModelClassGetType("PS_MODEL_GAUSS");
        pmPSF *psf = pmPSFAlloc(psfOptions);

        // NULL pmReadout input parameter
        bool rc = pmReadoutFakeFromSources(NULL, TEST_NUM_COLS, TEST_NUM_ROWS, sources,
                                           xOffset, yOffset, psf, 0.0, 1.0, false);
        ok(rc == false, "pmReadoutFakeFromSources() returned FALSE with pmReadout input parameter");

        // Non-positive numCols input parameter
        rc = pmReadoutFakeFromSources(readout, 0, TEST_NUM_ROWS, sources,
                                           xOffset, yOffset, psf, 0.0, 1.0, false);
        ok(rc == false, "pmReadoutFakeFromSources() returned FALSE with Non-positive numCols input parameter");

        // Non-positive numRows input parameter
        rc = pmReadoutFakeFromSources(readout, TEST_NUM_COLS, 0, sources,
                                           xOffset, yOffset, psf, 0.0, 1.0, false);
        ok(rc == false, "pmReadoutFakeFromSources() returned FALSE with Non-positive numRow input parameter");

        // NULL pmSource input parameter
        rc = pmReadoutFakeFromSources(readout, TEST_NUM_COLS, TEST_NUM_ROWS, NULL,
                                           xOffset, yOffset, psf, 0.0, 1.0, false);
        ok(rc == false, "pmReadoutFakeFromSources() returned FALSE with pmSource input parameter");

        // NULL pmPSF input parameter
        rc = pmReadoutFakeFromSources(readout, TEST_NUM_COLS, TEST_NUM_ROWS, sources,
                                           xOffset, yOffset, NULL, 0.0, 1.0, false);
        ok(rc == false, "pmReadoutFakeFromSources() returned FALSE with input parameter");

        // NULL incorrect type xOffset input parameter
        rc = pmReadoutFakeFromSources(readout, TEST_NUM_COLS, TEST_NUM_ROWS, sources,
                                           xOffsetF32, yOffset, psf, 0.0, 1.0, false);
        ok(rc == false, "pmReadoutFakeFromSources() returned FALSE with incorrect type xOffset input parameter");

        // NULL incorrect type yOffset input parameter
        rc = pmReadoutFakeFromSources(readout, TEST_NUM_COLS, TEST_NUM_ROWS, sources,
                                           xOffset, yOffsetF32, psf, 0.0, 1.0, false);
        ok(rc == false, "pmReadoutFakeFromSources() returned FALSE with incorrect type yOffset input parameter");

        // NULL incorrect size xOffset input parameter
        rc = pmReadoutFakeFromSources(readout, TEST_NUM_COLS, TEST_NUM_ROWS, sources,
                                           xOffsetBig, yOffset, psf, 0.0, 1.0, false);
        ok(rc == false, "pmReadoutFakeFromSources() returned FALSE with incorrect size xOffset input parameter");

        psFree(readout);
        psFree(xOffset);
        psFree(yOffset);
        psFree(xOffsetBig);
        psFree(xOffsetF32);
        psFree(yOffsetF32);
        psFree(sources);
        psFree(psfOptions);
        psFree(psf);
        pmModelClassCleanup();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmReadoutFakeFromSources() with acceptable input parameters.
    {
        psMemId id = psMemGetId();
        pmReadout *readout = generateSimpleReadout(NULL);
        psVector *xOffset = psVectorAlloc(NUM_SOURCES, PS_TYPE_S32);
        psVector *yOffset = psVectorAlloc(NUM_SOURCES, PS_TYPE_S32);
        psVector *xOffsetF32 = psVectorAlloc(NUM_SOURCES, PS_TYPE_F32);
        psVector *yOffsetF32 = psVectorAlloc(NUM_SOURCES, PS_TYPE_F32);
        psArray *sources = psArrayAlloc(NUM_SOURCES);
        for (int i = 0; i < sources->n ; i++) {
            sources->data[i] = pmSourceAlloc();
        }
        pmPSFOptions *psfOptions = pmPSFOptionsAlloc();
        psfOptions->psfTrendNx = 1;
        psfOptions->psfTrendNy = 2;
        psfOptions->psfFieldNx = 3;
        psfOptions->psfFieldNy = 4;
        psfOptions->psfFieldXo = 5;
        psfOptions->psfFieldYo = 6;
        pmModelClassInit();
        psfOptions->type = pmModelClassGetType("PS_MODEL_GAUSS");
        pmPSF *psf = pmPSFAlloc(psfOptions);

        bool rc = pmReadoutFakeFromSources(readout, TEST_NUM_COLS, TEST_NUM_ROWS, sources,
                                           xOffset, yOffset, psf, 0.0, 1.0, false);
        ok(rc == true, "pmReadoutFakeFromSources() returned TRUE with acceptable input parameters");

        psFree(readout);
        psFree(xOffset);
        psFree(yOffset);
        psFree(xOffsetF32);
        psFree(yOffsetF32);
        psFree(sources);
        psFree(psfOptions);
        psFree(psf);
        pmModelClassCleanup();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
}

