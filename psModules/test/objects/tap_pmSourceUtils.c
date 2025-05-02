#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include <psmodules.h>
#include "tap.h"
#include "pstap.h"
/* STATUS
    All functions are tested.
       pmSourceFromModel(): Must verify the pmSourceDefinePixels() set the
           values correctly.
*/

#define MISC_NUM                32
#define MISC_NAME              "META00"
#define NUM_BIAS_DATA           10
#define TEST_NUM_ROWS           (8)
#define TEST_NUM_COLS           (16)
#define VERBOSE                 0
#define ERR_TRACE_LEVEL         0
#define TEST_FLOATS_EQUAL(X, Y) (abs((X) - (Y)) < 0.0001)
#define NUM_SOURCES		100

#define CELL_ALLOC_NAME        "CellName"
#define NUM_READOUTS            3
#define NUM_CELLS               10
#define NUM_HDUS                5
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
        cell->readouts->data[i] = generateSimpleReadout(cell);
    }

    // First try to read data from ../dataFiles, then try dataFiles.
    bool rc = pmConfigFileRead(&cell->hdu->format, "../dataFiles/camera0/format0.config", "Camera format 0");
    if (!rc) {
        rc = pmConfigFileRead(&cell->hdu->format, "dataFiles/camera0/format0.config", "Camera format 0");
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
    psTraceSetLevel("psModules.objects", 0);
    plan_tests(23);


    // ----------------------------------------------------------------------
    // pmSourceModelGuess() tests
    // Call pmSourceModelGuess() with NULL pmSource input parameter
    {
        psMemId id = psMemGetId();
        pmSource *src = pmSourceAlloc();
        src->peak = pmPeakAlloc(1, 2, 3.0, PM_PEAK_LONE);
        src->moments = pmMomentsAlloc();
        pmModelType type = pmModelClassGetType ("PS_MODEL_GAUSS");
        pmModel *model = pmSourceModelGuess(NULL, type);
        ok(model == NULL, "pmSourceModelGuess() returned NULL with NULL pmSource input parameter");
        psFree(src);
        pmModelClassCleanup();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmSourceModelGuess() with NULL pmSource->peak input parameter
    {
        psMemId id = psMemGetId();
        pmSource *src = pmSourceAlloc();
        src->moments = pmMomentsAlloc();
        pmModelType type = pmModelClassGetType ("PS_MODEL_GAUSS");
        pmModel *model = pmSourceModelGuess(NULL, type);
        ok(model == NULL, "pmSourceModelGuess() returned NULL with NULL pmSource->peak input parameter");
        psFree(src);
        pmModelClassCleanup();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmSourceModelGuess() with NULL pmSource->moments input parameter
    {
        psMemId id = psMemGetId();
        pmSource *src = pmSourceAlloc();
        src->peak = pmPeakAlloc(1, 2, 3.0, PM_PEAK_LONE);
        pmModelType type = pmModelClassGetType ("PS_MODEL_GAUSS");
        pmModel *model = pmSourceModelGuess(NULL, type);
        ok(model == NULL, "pmSourceModelGuess() returned NULL with NULL pmSource->moments input parameter");
        psFree(src);
        pmModelClassCleanup();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // pmModel *pmSourceModelGuess(pmSource *source, pmModelType modelType)
    // Call pmSourceModelGuess() with acceptable input parameters
    // We only test a single model (PS_MODEL_GAUSS), but since this function is mostly
    // a wrapper to the model functions, that will suffice.
    {
        psMemId id = psMemGetId();
        pmSource *src = pmSourceAlloc();
        src->peak = pmPeakAlloc(1, 2, 3.0, PM_PEAK_LONE);
        src->moments = pmMomentsAlloc();

        src->moments->Mx = 1.0;
        src->moments->My = 1.0;
        src->moments->Mxx = 2.0;
        src->moments->Mxy= 2.0;
        src->moments->Myy = 2.0;
        src->moments->Mxxx = 3.0;
        src->moments->Mxxy = 3.0;
        src->moments->Mxyy = 3.0;
        src->moments->Myyy = 3.0;
        src->moments->Mxxxx = 4.0;
        src->moments->Mxxxy = 4.0;
        src->moments->Mxxyy = 4.0;
        src->moments->Mxyyy = 4.0;
        src->moments->Myyyy = 4.0;
        src->moments->Sum = 2.0;
        src->moments->Peak = 3.0;
        src->moments->Sky = 4.0;
        src->moments->dSky = 5.0;
        src->moments->SN = 6.0;
        src->moments->nPixels = 7.0;

        pmModelType type = pmModelClassGetType("PS_MODEL_GAUSS");
        pmModel *testModel = pmModelAlloc(type);
        testModel->modelGuess(testModel, src);
        pmModel *model = pmSourceModelGuess(src, type);
        ok(model != NULL, "pmSourceModelGuess() returned non-NULL with acceptable input parameters");
        psF32 *PAR  = model->params->data.F32;
        psEllipseMoments emoments;
        emoments.x2 = src->moments->Mx;
        emoments.y2 = src->moments->My;
        emoments.xy = src->moments->Mxy;
        // force the axis ratio to be < 20.0
        psEllipseAxes axes = psEllipseMomentsToAxes (emoments, 20.0);
        psEllipseShape shape = psEllipseAxesToShape (axes);
        ok(TEST_FLOATS_EQUAL(PAR[PM_PAR_SKY], src->moments->Sky), "pmSourceModelGuess() returned set model->params[PM_PAR_SKY] correctly");
        ok(TEST_FLOATS_EQUAL(PAR[PM_PAR_I0], src->moments->Peak - src->moments->Sky), "pmSourceModelGuess() returned set model->params[PM_PAR_IO] correctly");
        ok(TEST_FLOATS_EQUAL(PAR[PM_PAR_XPOS], src->moments->Mx), "pmSourceModelGuess() returned set model->params[PM_PAR_XPOS] correctly");
        ok(TEST_FLOATS_EQUAL(PAR[PM_PAR_YPOS], src->moments->My), "pmSourceModelGuess() returned set model->params[PM_PAR_YPOS] correctly");
        ok(TEST_FLOATS_EQUAL(PAR[PM_PAR_SXX], PS_MAX(0.5, M_SQRT2*shape.sx)), "pmSourceModelGuess() returned set model->params[PM_PAR_SXX] correctly");
        ok(TEST_FLOATS_EQUAL(PAR[PM_PAR_SYY], PS_MAX(0.5, M_SQRT2*shape.sy)), "pmSourceModelGuess() returned set model->params[PM_PAR_SYY] correctly");
        ok(TEST_FLOATS_EQUAL(PAR[PM_PAR_SXY], shape.sxy), "pmSourceModelGuess() returned set model->params[PM_PAR_SXY] correctly");
        psFree(src);
        psFree(testModel);
        psFree(model);
        pmModelClassCleanup();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // pmSourceModelGuess() tests
    // Call pmSourceFromModel() with NULL pmModel input parameter
    {
        psMemId id = psMemGetId();
        pmModelType type = pmModelClassGetType ("PS_MODEL_GAUSS");
        pmModel *model = pmModelAlloc(type);
        pmCell *cell = generateSimpleCell(NULL);
        pmReadout *readout = cell->readouts->data[0];
        pmSource *src = pmSourceFromModel(NULL, readout, 10.0, PM_SOURCE_TYPE_STAR);
        ok(src == NULL, "pmSourceFromModel() returned NULL with NULL pmModel input parameter");
        psFree(model);
        psFree(src);
        myFreeCell(cell);
        pmModelClassCleanup();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmSourceFromModel() with NULL pmReadout input parameter
    {
        psMemId id = psMemGetId();

        pmModelType type = pmModelClassGetType ("PS_MODEL_GAUSS");
        pmModel *model = pmModelAlloc(type);
        pmCell *cell = generateSimpleCell(NULL);
        pmSource *src = pmSourceFromModel(model, NULL, 10.0, PM_SOURCE_TYPE_STAR);
        ok(src == NULL, "pmSourceFromModel() returned NULL with NULL pmReadout input parameter");
        psFree(model);
        psFree(src);
        myFreeCell(cell);
        pmModelClassCleanup();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmSourceFromModel() with acceptable input parameters
    // XXX: Must verify the pmSourceDefinePixels() set the values correctly.
    {
        psMemId id = psMemGetId();

        pmModel *model = pmModelAlloc(pmModelClassGetType("PS_MODEL_GAUSS"));
        float Io    = model->params->data.F32[PM_PAR_I0] = 2.0;
        float xChip = model->params->data.F32[PM_PAR_XPOS] = 3.0;
        float yChip = model->params->data.F32[PM_PAR_YPOS] = 5.0;
        pmCell *cell = generateSimpleCell(NULL);
        pmReadout *readout = cell->readouts->data[0];
        pmSource *src = pmSourceFromModel(model, readout, 10.0, PM_SOURCE_TYPE_STAR);
        ok(src != NULL, "pmSourceFromModel() returned non-NULL with acceptable input parameters");
        ok(src->modelPSF == model, "pmSourceFromModel() set pmSource->modelPSF correctly");

        pmPeak *tmpPeak = pmPeakAlloc (xChip, yChip, Io, PM_PEAK_LONE);
        ok(src->peak->x == xChip, "pmSourceFromModel() set pmSource->peak->x correctly (%.2f %.2f)", src->peak->x, xChip);

        psFree(model);
        // XXX: We get psMemory aborts if the following is not done.
        // There is probably an issue with psMemIncrRefCounter() in pmSourceUtils.c

        src->modelPSF = NULL;
        src->modelEXT = NULL;
        psFree(src);
        psFree(tmpPeak);
        myFreeCell(cell);
        pmModelClassCleanup();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
}
