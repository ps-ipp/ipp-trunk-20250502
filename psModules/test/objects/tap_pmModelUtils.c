#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include <psmodules.h>
#include "tap.h"
#include "pstap.h"
/* STATUS:
	All functions are tested.
*/

#define VERBOSE                 0
#define ERR_TRACE_LEVEL         0
#define NUM_ROWS		8
#define NUM_COLS		16
#define TEST_MODEL_CLASS_TYPE 1
#define TEST_FLOATS_EQUAL(X, Y) (abs(X - Y) < 0.0001)

int main(int argc, char* argv[])
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    psTraceSetLevel("err", ERR_TRACE_LEVEL);
    plan_tests(46);

    // ----------------------------------------------------------------------
    // pmModelFromPSF() tests
    // pmModel *pmModelFromPSF (pmModel *modelEXT, pmPSF *psf)
    // call pmModelFromPSF() with NULL pmModel input parameter
    {
        psMemId id = psMemGetId();
        pmModelClassInit();
        pmModel *model = pmModelAlloc(TEST_MODEL_CLASS_TYPE);
        pmPSFOptions *psfOptions = pmPSFOptionsAlloc();
        psfOptions->type = pmModelClassGetType("PS_MODEL_GAUSS");
        pmPSF *psf = pmPSFAlloc(psfOptions);
        pmModel *tmpModel = pmModelFromPSF(NULL, psf);
        ok(tmpModel == NULL, "pmModelFromPSF() returned NULL with NULL pmModel input parameter");
        psFree(model);
        psFree(psfOptions);
        psFree(psf);
        pmModelClassCleanup();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // call pmModelFromPSF() with NULL pmPSF input parameter
    {
        psMemId id = psMemGetId();
        pmModelClassInit();
        pmModel *model = pmModelAlloc(TEST_MODEL_CLASS_TYPE);
        pmPSFOptions *psfOptions = pmPSFOptionsAlloc();
        psfOptions->type = pmModelClassGetType("PS_MODEL_GAUSS");
        pmPSF *psf = pmPSFAlloc(psfOptions);
        pmModel *tmpModel = pmModelFromPSF(model, NULL);
        ok(tmpModel == NULL, "pmModelFromPSF() returned NULL with NULL pmPSF input parameter");
        psFree(model);
        psFree(psfOptions);
        psFree(psf);
        pmModelClassCleanup();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // call pmModelFromPSF() with acceptable input parameters
    {
        psMemId id = psMemGetId();
        pmModelClassInit();
        pmModel *model = pmModelAlloc(TEST_MODEL_CLASS_TYPE);
        pmPSFOptions *psfOptions = pmPSFOptionsAlloc();
        psfOptions->type = pmModelClassGetType("PS_MODEL_GAUSS");
        pmPSF *psf = pmPSFAlloc(psfOptions);
        pmModel *tmpModel = pmModelFromPSF(model, psf);
        ok(tmpModel != NULL, "pmModelFromPSF() returned non-NULL with acceptable input parameters");

        pmModel *testModelPSF = pmModelAlloc(psf->type);
        model->modelFromPSF(testModelPSF, model, psf);
        ok(tmpModel->type == testModelPSF->type, "pmModelFromPSF() set the model->type correctly");
        ok(tmpModel->chisq == testModelPSF->chisq, "pmModelFromPSF() set the model->chisq correctly");
        ok(tmpModel->chisqNorm == testModelPSF->chisqNorm, "pmModelFromPSF() set the model->chisqNorm correctly");
        ok(tmpModel->nDOF == testModelPSF->nDOF, "pmModelFromPSF() set the model->nDOF correctly");
        ok(tmpModel->nIter == testModelPSF->nIter, "pmModelFromPSF() set the model->nIter correctly");
        ok(tmpModel->flags == testModelPSF->flags, "pmModelFromPSF() set the model->flags correctly");
        ok(tmpModel->fitRadius == testModelPSF->fitRadius, "pmModelFromPSF() set the model->fitRadius correctly");
        ok(tmpModel->modelFunc == testModelPSF->modelFunc, "pmModelFromPSF() set the model->modelFunc correctly");
        ok(tmpModel->modelFlux == testModelPSF->modelFlux, "pmModelFromPSF() set the model->modelFlux correctly");
        ok(tmpModel->modelRadius == testModelPSF->modelRadius, "pmModelFromPSF() set the model->modelRadius correctly");
        ok(tmpModel->modelLimits == testModelPSF->modelLimits, "pmModelFromPSF() set the model->modelLimits correctly");
        ok(tmpModel->modelGuess == testModelPSF->modelGuess, "pmModelFromPSF() set the model->modelGuess correctly");
        ok(tmpModel->modelFromPSF == testModelPSF->modelFromPSF, "pmModelFromPSF() set the model->modelFromPSF correctly");
        ok(tmpModel->modelParamsFromPSF == testModelPSF->modelParamsFromPSF, "pmModelFromPSF() set the model->modelParamsFromPSF correctly");
        ok(tmpModel->modelFitStatus == testModelPSF->modelFitStatus, "pmModelFromPSF() set the model->modelFitStatus correctly");

        psFree(testModelPSF);
        psFree(tmpModel);
        psFree(model);
        psFree(psfOptions);
        psFree(psf);
        pmModelClassCleanup();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // pmModelFromPSFforXY() tests
    // pmModel *pmModelFromPSFforXY (pmPSF *psf, float Xo, float Yo, float Io)
    // call pmModelFromPSFforXY() with NULL pmPSF input parameter
    {
        psMemId id = psMemGetId();
        pmModelClassInit();
        pmPSFOptions *psfOptions = pmPSFOptionsAlloc();
        psfOptions->type = pmModelClassGetType("PS_MODEL_GAUSS");
        pmPSF *psf = pmPSFAlloc(psfOptions);
        pmModel *tmpModel = pmModelFromPSFforXY(NULL, 1.0, 2.0, 3.0);
        ok(tmpModel == NULL, "pmModelFromPSFforXY() returned NULL with NULL pmPSF input parameter");
        psFree(psfOptions);
        psFree(psf);
        pmModelClassCleanup();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // call pmModelFromPSFforXY() with acceptable input parameters
    {
        psMemId id = psMemGetId();
        pmModelClassInit();
        pmPSFOptions *psfOptions = pmPSFOptionsAlloc();
        psfOptions->type = pmModelClassGetType("PS_MODEL_GAUSS");
        pmPSF *psf = pmPSFAlloc(psfOptions);

        pmModel *testModelPSF = pmModelAlloc (psf->type);
        testModelPSF->modelParamsFromPSF(testModelPSF, psf, 1.0, 2.0, 3.0);

        pmModel *tmpModel = pmModelFromPSFforXY(psf, 1.0, 2.0, 3.0);
        ok(tmpModel != NULL, "pmModelFromPSFforXY() returned non-NULL with acceptable input parameters");

        ok(tmpModel->type == testModelPSF->type, "pmModelFromPSF() set the model->type correctly");
        ok(tmpModel->chisq == testModelPSF->chisq, "pmModelFromPSF() set the model->chisq correctly");
        ok(TEST_FLOATS_EQUAL(tmpModel->chisqNorm, testModelPSF->chisqNorm), "pmModelFromPSF() set the model->chisqNorm correctly");
        ok(tmpModel->nDOF == testModelPSF->nDOF, "pmModelFromPSF() set the model->nDOF correctly");
        ok(tmpModel->nIter == testModelPSF->nIter, "pmModelFromPSF() set the model->nIter correctly");
        ok(tmpModel->flags == testModelPSF->flags, "pmModelFromPSF() set the model->flags correctly");
        ok(tmpModel->fitRadius == testModelPSF->fitRadius, "pmModelFromPSF() set the model->fitRadius correctly");
        ok(tmpModel->modelFunc == testModelPSF->modelFunc, "pmModelFromPSF() set the model->modelFunc correctly");
        ok(tmpModel->modelFlux == testModelPSF->modelFlux, "pmModelFromPSF() set the model->modelFlux correctly");
        ok(tmpModel->modelRadius == testModelPSF->modelRadius, "pmModelFromPSF() set the model->modelRadius correctly");
        ok(tmpModel->modelLimits == testModelPSF->modelLimits, "pmModelFromPSF() set the model->modelLimits correctly");
        ok(tmpModel->modelGuess == testModelPSF->modelGuess, "pmModelFromPSF() set the model->modelGuess correctly");
        ok(tmpModel->modelFromPSF == testModelPSF->modelFromPSF, "pmModelFromPSF() set the model->modelFromPSF correctly");
        ok(tmpModel->modelParamsFromPSF == testModelPSF->modelParamsFromPSF, "pmModelFromPSF() set the model->modelParamsFromPSF correctly");
        ok(tmpModel->modelFitStatus == testModelPSF->modelFitStatus, "pmModelFromPSF() set the model->modelFitStatus correctly");

        psFree(tmpModel);
        psFree(testModelPSF);
        psFree(psfOptions);
        psFree(psf);
        pmModelClassCleanup();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // pmModelSetFlux() tests
    // bool pmModelSetFlux(pmModel *model, float flux) {
    // call pmModelSetFlux() with NULL pmPSF input parameter
    {
        psMemId id = psMemGetId();
        pmModelClassInit();
        pmModel *model = pmModelAlloc(TEST_MODEL_CLASS_TYPE);
        bool rc = pmModelSetFlux(NULL, 1.0);
        ok(!rc, "pmModelSetFlux(() returned FALSE with NULL pmModel input parameter");
        psFree(model);
        pmModelClassCleanup();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // call pmModelSetFlux() with acceptable input parameters
    // XXX: We should probably test with more input values
    {
        psMemId id = psMemGetId();
        pmModelClassInit();
        pmModel *model = pmModelAlloc(TEST_MODEL_CLASS_TYPE);
        model->params->data.F32[PM_PAR_SXX] = 1.0;
        model->params->data.F32[PM_PAR_SYY] = 1.0;
        model->params->data.F32[PM_PAR_SXY] = 1.0;

        // Compute the test flux value
        float tmpF = model->params->data.F32[PM_PAR_I0];
        model->params->data.F32[PM_PAR_I0] = 1.0;
        float testFlux = model->modelFlux (model->params);
        testFlux = 1.0 / testFlux;    
        model->params->data.F32[PM_PAR_I0] = tmpF;

        bool rc = pmModelSetFlux(model, 1.0);
        ok(rc, "pmModelSetFlux(() returned TRUE with acceptable input parameters");
        ok(TEST_FLOATS_EQUAL(testFlux, model->params->data.F32[PM_PAR_I0]), "pmModelSetFlux() set the flux correctly");
        psFree(model);
        pmModelClassCleanup();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
}

