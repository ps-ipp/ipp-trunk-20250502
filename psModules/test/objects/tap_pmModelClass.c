#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include <psmodules.h>
#include "tap.h"
#include "pstap.h"
/* STATUS:
    All functions are tested except pmModelClassCleanup() which is deferred
    because there's no way to test that it frees a static variable, except
    throug hthe memory leak tests
*/

#define MISC_NUM                32
#define MISC_NAME              "META00"
#define NUM_BIAS_DATA           10
#define TEST_NUM_ROWS           (8+1)
#define TEST_NUM_COLS           (8+1)
#define VERBOSE                 0
#define ERR_TRACE_LEVEL         0
#include "models/pmModel_GAUSS.c"
#include "models/pmModel_PGAUSS.c"

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


int main(int argc, char* argv[])
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    psTraceSetLevel("err", ERR_TRACE_LEVEL);
    plan_tests(37);

    // ----------------------------------------------------------------------
    // Test pmModelClassAlloc()
    // pmModelClass *pmModelClassAlloc (int nModels)
    {
        psMemId id = psMemGetId();
        pmModelClass *modelClass = pmModelClassAlloc(4);
        ok(modelClass != NULL && psMemCheckModelClass(modelClass), "pmModelClassAlloc() returned a non-NULL pmModelClass");
        psFree(modelClass);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // Test pmModelClassCleanup(), pmModelClassSelect()
    // Basically, call pmModelClassInit(), then pmModelClassCleanup(), and ensure that
    // various default models are not there.
    // XXX: We don't run this test because the spec changed and pmModelClassSelect() now calls
    // pmModelClassInit().
    if (0) {
        psMemId id = psMemGetId();
        pmModelClassInit();
        pmModelClassCleanup();
        ok(NULL == pmModelClassSelect(0), "pmModelClassCleanup(): removed PS_MODEL_GAUSS");
        ok(NULL == pmModelClassSelect(1), "pmModelClassCleanup(): removed PS_MODEL_PGAUSS");
        ok(NULL == pmModelClassSelect(2), "pmModelClassCleanup(): removed PS_MODEL_QGAUSS");
        ok(NULL == pmModelClassSelect(3), "pmModelClassCleanup(): removed PS_MODEL_RGAUSS");
        ok(NULL == pmModelClassSelect(4), "pmModelClassCleanup(): removed PS_MODEL_SERSIC");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // Test pmModelClassInit(), pmModelClassGetName()
    // Basically, call pmModelClassCleanup(), then pmModelClassInit() and ensure that
    // various default models are there.
    {
        psMemId id = psMemGetId();

        pmModelClassCleanup();
        ok(pmModelClassInit(), "pmModelClassInit() returned TRUE");
        ok(!strcmp(pmModelClassGetName(0), "PS_MODEL_GAUSS"), "pmModelClassInit() added pmModel PS_MODEL_GAUSS");
        ok(!strcmp(pmModelClassGetName(1), "PS_MODEL_PGAUSS"), "pmModelClassInit() added pmModel PS_MODEL_PGAUSS");
        ok(!strcmp(pmModelClassGetName(2), "PS_MODEL_QGAUSS"), "pmModelClassInit() added pmModel PS_MODEL_QGAUSS");
        ok(!strcmp(pmModelClassGetName(3), "PS_MODEL_RGAUSS"), "pmModelClassInit() added pmModel PS_MODEL_RGAUSS");
        ok(!strcmp(pmModelClassGetName(4), "PS_MODEL_SERSIC"), "pmModelClassInit() added pmModel PS_MODEL_SERSIC");
        ok(!pmModelClassInit(), "pmModelClassInit() returned FALSE (2nd time)");
        ok(NULL == pmModelClassGetName(-1), "pmModelClassGetName(-1) returned NULL");
        ok(NULL == pmModelClassGetName(1000), "pmModelClassGetName(1000) returned NULL");
        pmModelClassCleanup();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // Test pmModelClassGetType()
    {
        psMemId id = psMemGetId();
        pmModelClassInit();
        ok(-1 == pmModelClassGetType("BOGUS"), "pmModelClassGetType(BOGUS) returned -1");
        ok(0 == pmModelClassGetType("PS_MODEL_GAUSS"), "pmModelClassGetType(PS_MODEL_GAUSS) successful");
        ok(1 == pmModelClassGetType("PS_MODEL_PGAUSS"), "pmModelClassGetType(PS_MODEL_PGAUSS) successful");
        ok(2 == pmModelClassGetType("PS_MODEL_QGAUSS"), "pmModelClassGetType(PS_MODEL_QGAUSS) successful");
        ok(3 == pmModelClassGetType("PS_MODEL_RGAUSS"), "pmModelClassGetType(PS_MODEL_RGAUSS) successful");
        ok(4 == pmModelClassGetType("PS_MODEL_SERSIC"), "pmModelClassGetType(PS_MODEL_SERSIC) successful");
        pmModelClassCleanup();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // Test pmModelClassParameterCount()
    {
        psMemId id = psMemGetId();
        pmModelClassInit();
        ok(0 == pmModelClassParameterCount(-1), "pmModelClassParameterCount(-1) returned 0");
        ok(0 == pmModelClassParameterCount(1000), "pmModelClassParameterCount(1000) returned 0");
        ok(7 == pmModelClassParameterCount(0), "pmModelClassParameterCount(0) returned 7");
        ok(7 == pmModelClassParameterCount(1), "pmModelClassParameterCount(1) returned 7");
        ok(8 == pmModelClassParameterCount(2), "pmModelClassParameterCount(2) returned 8");
        ok(8 == pmModelClassParameterCount(3), "pmModelClassParameterCount(3) returned 8");
        ok(8 == pmModelClassParameterCount(4), "pmModelClassParameterCount(4) returned 8");
        pmModelClassCleanup();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // Test pmModelClassSelect()
    // pmModelClass *pmModelClassSelect (pmModelType type)
    {
        psMemId id = psMemGetId();
        pmModelClassInit();
        pmModelClass *model = NULL;
        model = pmModelClassSelect(-1);
        ok(model == NULL, "pmModelClassSelect(-1) successful");
        model = pmModelClassSelect(1000);
        ok(model == NULL, "pmModelClassSelect(1000) successful");
        model = pmModelClassSelect(0);
        ok(model != NULL && !strcmp(model->name, "PS_MODEL_GAUSS"), "pmModelClassSelect(0) successful");
        model = pmModelClassSelect(1);
        ok(model != NULL && !strcmp(model->name, "PS_MODEL_PGAUSS"), "pmModelClassSelect(1) successful");
        model = pmModelClassSelect(2);
        ok(model != NULL && !strcmp(model->name, "PS_MODEL_QGAUSS"), "pmModelClassSelect(2) successful");
        model = pmModelClassSelect(3);
        ok(model != NULL && !strcmp(model->name, "PS_MODEL_RGAUSS"), "pmModelClassSelect(3) successful");
        model = pmModelClassSelect(4);
        ok(model != NULL && !strcmp(model->name, "PS_MODEL_SERSIC"), "pmModelClassSelect(4) successful");
        pmModelClassCleanup();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // Test pmModelClassAdd()
    // We create a new modelClass, then add it, then ensure that it exists.
    {
        psMemId id = psMemGetId();
        pmModelClassInit();
        pmModelClass *newModel = pmModelClassAlloc(1);
        newModel->name = psStringCopy("PS_MODEL_NEW00");
        newModel->nParams = 22;
        pmModelClassAdd(newModel);
        int ID = pmModelClassGetType("PS_MODEL_NEW00");
        ok(ID != -1 && !strcmp("PS_MODEL_NEW00", pmModelClassGetName(ID)), "pmModelClassAdd() added the new model successfully");
        psFree(newModel->name);
        psFree(newModel);
        pmModelClassCleanup();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
}

