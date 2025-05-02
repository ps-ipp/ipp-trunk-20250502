#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include <psmodules.h>
#include "tap.h"
#include "pstap.h"
/* STATUS:
    All functions are tested.  However...

    The source code for pmModelAddWithOffset() and pmModelSubWithOffset() is
    almost exactly the same as the source code for pmModelAdd() and
    pmModelSub().  We do not test them here.
*/

#define MISC_NUM                32
#define MISC_NAME              "META00"
#define NUM_BIAS_DATA           10
#define TEST_NUM_ROWS           (8+1)
#define TEST_NUM_COLS           (8+1)
#define VERBOSE                 0
#define ERR_TRACE_LEVEL         0
#define NUM_ROWS		8
#define NUM_COLS		16
#define TEST_FLOATS_EQUAL(X, Y) (abs(X - Y) < 0.01)

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
    plan_tests(53);

    // ----------------------------------------------------------------------
    // pmModelAlloc() tests
    // call pmModelAlloc() with unallowable model class type
    {
        psMemId id = psMemGetId();
        pmModelClassInit();
        pmModel *model = pmModelAlloc(-1);
        ok(model == NULL, "pmModelAlloc() returned a NULL with unallowable model class type");
        pmModelClassCleanup();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // call pmModelAlloc() with acceptable input params
    {
        #define TEST_MODEL_CLASS_TYPE 1
        psMemId id = psMemGetId();
        pmModelClassInit();
        pmModel *model = pmModelAlloc(TEST_MODEL_CLASS_TYPE);
        ok(model != NULL && psMemCheckModel(model), "pmModelAlloc() returned a non-NULL pmModel");
        skip_start(!model, 1, "Skipping tests because pmModelAlloc() returned NULL");
        ok(model->type == TEST_MODEL_CLASS_TYPE, "pmModelAlloc() set pmModel->type correctly");
        ok(model->chisq == 0.0, "pmModelAlloc() set pmModel->chisq correctly");
        ok(model->nDOF == 0, "pmModelAlloc() set pmModel->nDOF correctly");
        ok(model->nIter == 0, "pmModelAlloc() set pmModel->nIter correctly");
        ok(model->fitRadius == 0, "pmModelAlloc() set pmModel->fitRadius correctly");
        ok(model->flags == PM_MODEL_STATUS_NONE, "pmModelAlloc() set pmModel->flags correctly");
        ok(model->residuals == NULL, "pmModelAlloc() set pmModel->residuals correctly");
        int nParams = pmModelClassParameterCount(TEST_MODEL_CLASS_TYPE);
        ok(model->params != NULL && model->params->n == nParams, "pmModelAlloc() set the pmModel->params psVector correctly");
        ok(model->dparams != NULL && model->dparams->n == nParams, "pmModelAlloc() set the pmModel->dparams psVector correctly");
        bool errorFlag = false;
        for (psS32 i = 0; i < nParams; i++) {
            if (model->params->data.F32[i] != 0.0 || model->dparams->data.F32[i] != 0.0) {
                if (VERBOSE) {
                    diag("ERROR: params/dparams[%d] is (%.2f %.2f) should be (%.2f %.2f)\n", i, model->params->data.F32[i], model->dparams->data.F32[i], 0.0, 0.0);
		}
                errorFlag = true;
	    }
        }
        ok(!errorFlag, "pmModelAlloc() set the members of the model->params and model->dparams psVectors correctly");
        pmModelClass *class = pmModelClassSelect(TEST_MODEL_CLASS_TYPE);
        ok(model->modelFunc == class->modelFunc, "pmModelAlloc() set pmModel->modelFunc correctly");
        ok(model->modelFlux == class->modelFlux, "pmModelAlloc() set pmModel->modelFlux correctly");
        ok(model->modelRadius == class->modelRadius, "pmModelAlloc() set pmModel->modelRadius correctly");
        ok(model->modelLimits == class->modelLimits, "pmModelAlloc() set pmModel->modelLimits correctly");
        ok(model->modelGuess == class->modelGuess, "pmModelAlloc() set pmModel->modelGuess correctly");
        ok(model->modelFromPSF == class->modelFromPSF, "pmModelAlloc() set pmModel->modelFromPSF correctly");
        ok(model->modelParamsFromPSF == class->modelParamsFromPSF, "pmModelAlloc() set pmModel->modelParamsFromPSF correctly");
        ok(model->modelFitStatus == class->modelFitStatus, "pmModelAlloc() set pmModel->modelFitStatus correctly");

        psFree(model);
        skip_end();
        pmModelClassCleanup();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // pmModelCopy() tests
    // call pmModelCopy() with NULL input pmModel parameter
    {
        psMemId id = psMemGetId();
        pmModel *model = pmModelCopy(NULL);
        ok(model == NULL, "pmModelCopy() returned NULL with NULL input pmModel parameter");
        pmModelClassCleanup();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // call pmModelCopy() with acceptable input params
    {
        #define TEST_MODEL_CLASS_TYPE 1
        psMemId id = psMemGetId();
        pmModelClassInit();
        pmModel *modelSrc = pmModelAlloc(TEST_MODEL_CLASS_TYPE);
        ok(modelSrc != NULL && psMemCheckModel(modelSrc), "pmModelAlloc() returned a non-NULL pmModel");
        modelSrc->chisq = 1.0;
        modelSrc->nDOF = 2;
        modelSrc->nIter = 3;
        modelSrc->flags = PM_MODEL_STATUS_NONE;
        modelSrc->fitRadius = 4;
        pmModel *modelDst = pmModelCopy(modelSrc);
        ok(modelDst != NULL && psMemCheckModel(modelDst), "pmModelCopy() returned a non-NULL pmModel");
        ok(modelDst->chisq == 1.0, "pmModelCopy() set the pmModel->chisq member correctly");
        ok(modelDst->nDOF == 2, "pmModelCopy() set the pmModel->nDOF member correctly");
        ok(modelDst->nIter == 3, "pmModelCopy() set the pmModel->nIter member correctly");
        ok(modelDst->flags == PM_MODEL_STATUS_NONE, "pmModelCopy() set the pmModel->flags member correctly");
        ok(modelDst->fitRadius == 4, "pmModelCopy() set the pmModel->fitRadius member correctly");

        psFree(modelSrc);
        psFree(modelDst);
        pmModelClassCleanup();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // pmModelEval() tests
    // call pmModelEval() with NULL input pmModel parameter
    {
        psMemId id = psMemGetId();
        pmModel *model = pmModelAlloc(TEST_MODEL_CLASS_TYPE);
        psImage *img = psImageAlloc(NUM_COLS, NUM_ROWS, PS_TYPE_F32);
        psF32 tmpF;
        tmpF = pmModelEval(NULL, img, 0, 0);
        ok(isnan(tmpF), "pmModelEval() returned NAN with NULL input pmModel parameter");
        psFree(model);
        psFree(img);
        pmModelClassCleanup();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // psF32 pmModelEval(pmModel *model, psImage *image, psS32 col, psS32 row)
    // call pmModelEval() with NULL input psImage parameter
    {
        psMemId id = psMemGetId();
        pmModel *model = pmModelAlloc(TEST_MODEL_CLASS_TYPE);
        psImage *img = psImageAlloc(NUM_COLS, NUM_ROWS, PS_TYPE_F32);
        psF32 tmpF;
        tmpF = pmModelEval(model, NULL, 0, 0);
        ok(isnan(tmpF), "pmModelEval() returned NAN with NULL input psImage parameter");
        psFree(model);
        psFree(img);
        pmModelClassCleanup();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // psF32 pmModelEval(pmModel *model, psImage *image, psS32 col, psS32 row)
    // call pmModelEval() with acceptable input parameters
    {
        psMemId id = psMemGetId();
        pmModel *model = pmModelAlloc(TEST_MODEL_CLASS_TYPE);
        psImage *img = psImageAlloc(NUM_COLS, NUM_ROWS, PS_TYPE_F32);
        for (int i = 0 ; i < NUM_ROWS ; i++) {
            for (int j = 0 ; j < NUM_COLS ; j++) {
                img->data.F32[i][j] = (float) (i + j);
	    }
	}
        psVector *x = psVectorAlloc(2, PS_TYPE_F32);
        x->data.F32[0] = (float) (NUM_COLS / 2);
        x->data.F32[1] = (float) (NUM_ROWS / 2);
        psF32 testF = model->modelFunc (NULL, model->params, x);
        psF32 tmpF;
        tmpF = pmModelEval(model, img, (int) x->data.F32[0], (int) x->data.F32[1]);
        ok(!isnan(tmpF), "pmModelEval() returned successfully");
        ok(testF == tmpF, "pmModelEval() evaluated the model correctly");
        psFree(model);
        psFree(img);
        psFree(x);
        pmModelClassCleanup();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


/* XXX: This seems to have disappeared from pmModel.h
    // ----------------------------------------------------------------------
    // pmModelEvalWithOffset() tests
    // psF32 pmModelEvalWithOffset(pmModel *model, psImage *image, 
    //                             psS32 col, psS32 row, int dx, int dy)
    // call pmModelEvalWithOffset() with NULL input pmModel parameter
    {
        psMemId id = psMemGetId();
        pmModel *model = pmModelAlloc(TEST_MODEL_CLASS_TYPE);
        psImage *img = psImageAlloc(NUM_COLS, NUM_ROWS, PS_TYPE_F32);
        psF32 tmpF;
        tmpF = pmModelEvalWithOffset(NULL, img, 0, 0, 0, 0);
        ok(isnan(tmpF), "pmModelEvalWithOffset() returned NAN with NULL input pmModel parameter");
        psFree(model);
        psFree(img);
        pmModelClassCleanup();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // psF32 pmModelEvalWithOffset(pmModel *model, psImage *image, 
    //                             psS32 col, psS32 row, int dx, int dy)
    // call pmModelEvalWithOffset() with NULL input psImage parameter
    {
        psMemId id = psMemGetId();
        pmModel *model = pmModelAlloc(TEST_MODEL_CLASS_TYPE);
        psImage *img = psImageAlloc(NUM_COLS, NUM_ROWS, PS_TYPE_F32);
        psF32 tmpF;
        tmpF = pmModelEvalWithOffset(model, NULL, 0, 0, 0, 0);
        ok(isnan(tmpF), "pmModelEvalWithOffset() returned NAN with NULL input psImage parameter");
        psFree(model);
        psFree(img);
        pmModelClassCleanup();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // psF32 pmModelEvalWithOffset(pmModel *model, psImage *image, psS32 col, psS32 row)
    // call pmModelEvalWithOffset() with acceptable input parameters
    {
        psMemId id = psMemGetId();
        pmModel *model = pmModelAlloc(TEST_MODEL_CLASS_TYPE);
        psImage *img = psImageAlloc(NUM_COLS, NUM_ROWS, PS_TYPE_F32);
        for (int i = 0 ; i < NUM_ROWS ; i++) {
            for (int j = 0 ; j < NUM_COLS ; j++) {
                img->data.F32[i][j] = (float) (i + j);
	    }
	}
        psVector *x = psVectorAlloc(2, PS_TYPE_F32);
        x->data.F32[0] = (float) ((NUM_COLS / 2) + (NUM_COLS / 4));
        x->data.F32[1] = (float) ((NUM_ROWS / 2) + (NUM_ROWS / 4));
        psF32 testF = model->modelFunc (NULL, model->params, x);
        psF32 tmpF;
        tmpF = pmModelEvalWithOffset(model, img, (int) x->data.F32[0], (int) x->data.F32[1], NUM_COLS/4, NUM_ROWS/4);
        ok(!isnan(tmpF), "pmModelEvalWithOffset() returned successfully");
        ok(testF == tmpF, "pmModelEvalWithOffset() evaluated the model correctly");
        psFree(model);
        psFree(img);
        psFree(x);
        pmModelClassCleanup();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
*/


    // ----------------------------------------------------------------------
    // pmModelAdd() tests
    // call pmModelAdd() with bad input parameters
    {
        psMemId id = psMemGetId();
        psImage *img = psImageAlloc(NUM_COLS, NUM_ROWS, PS_TYPE_F32);
        psImage *imgS32 = psImageAlloc(NUM_COLS, NUM_ROWS, PS_TYPE_S32);
        psImage *mask = psImageAlloc(NUM_COLS, NUM_ROWS, PS_TYPE_U8);
        for (int i = 0 ; i < NUM_ROWS ; i++) {
            for (int j = 0 ; j < NUM_COLS ; j++) {
                img->data.F32[i][j] = 0.0;
                mask->data.U8[i][j] = 0;
            }
        }
        pmModel *model = pmModelAlloc(pmModelClassGetType("PS_MODEL_GAUSS"));
        psF32 *PAR = model->params->data.F32;
        PAR[PM_PAR_XPOS] = (float) (NUM_COLS/2);
        PAR[PM_PAR_YPOS] = (float) (NUM_ROWS/2);
        PAR[PM_PAR_SXX] = 1.0;
        PAR[PM_PAR_SYY] = 1.0;

        // NULL psImage input parameter
        bool rc = pmModelAdd(NULL, mask, model, PM_MODEL_OP_FUNC, 1);
        ok(rc == false, "pmModelAdd() returned FALSE with NULL psImage input parameter");

        // NULL pmModel input parameter
        rc = pmModelAdd(img, mask, NULL, PM_MODEL_OP_FUNC, 1);
        ok(rc == false, "pmModelAdd() returned FALSE with NULL pmModel input parameter");

        // Incorrect type psImage input parameter
        rc = pmModelAdd(imgS32, mask, model, PM_MODEL_OP_FUNC, 1);
        ok(rc == false, "pmModelAdd() returned FALSE with Incorrect type psImage input parameter");

        psFree(img);
        psFree(imgS32);
        psFree(mask);
        psFree(model);
        pmModelClassCleanup();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // call pmModelAdd() with acceptable parameters
    // We only test with a single Gaussian model, with no residuals or masks.
    // For completeness, additional tests should be added.
    {
        psMemId id = psMemGetId();
        psImage *img = psImageAlloc(NUM_COLS, NUM_ROWS, PS_TYPE_F32);
        psImage *mask = psImageAlloc(NUM_COLS, NUM_ROWS, PS_TYPE_U8);
        for (int i = 0 ; i < NUM_ROWS ; i++) {
            for (int j = 0 ; j < NUM_COLS ; j++) {
                img->data.F32[i][j] = 0.0;
                mask->data.U8[i][j] = 0;
            }
        }
        pmModel *model = pmModelAlloc(pmModelClassGetType("PS_MODEL_GAUSS"));
        psF32 *PAR = model->params->data.F32;
        PAR[PM_PAR_I0] = 5.0;
        PAR[PM_PAR_XPOS] = 0.0;
        PAR[PM_PAR_YPOS] = 0.0;
        PAR[PM_PAR_XPOS] = (float) (NUM_COLS/2);
        PAR[PM_PAR_YPOS] = (float) (NUM_ROWS/2);
        PAR[PM_PAR_SXX] = 10.0;
        PAR[PM_PAR_SYY] = 10.0;

        bool rc = pmModelAdd(img, mask, model, PM_MODEL_OP_FUNC, 1);
        ok(rc == true, "pmModelAdd() returned TRUE with acceptable input parameters");
        psVector *x = psVectorAlloc(2, PS_TYPE_F32);
        bool errorFlag = false;
        for (int i = 0 ; i < NUM_ROWS ; i++) {
            for (int j = 0 ; j < NUM_COLS ; j++) {
                x->data.F32[0] = (float) j;
                x->data.F32[1] = (float) i;
                psF32 modF = model->modelFunc (NULL, model->params, x);
                psF32 imgF = img->data.F32[i][j];
                if (!TEST_FLOATS_EQUAL(modF, imgF)) {
                    diag("ERROR: img[%d][%d] is %.2f, should be %.2f\n", i, j, img->data.F32[i][j], modF);
                    errorFlag = true;
                }
            }
        }
        ok(!errorFlag, "pmModelAdd() set the image pixels correctly");
        psFree(x);
        psFree(img);
        psFree(mask);
        psFree(model);
        pmModelClassCleanup();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // pmModelSub() tests
    // call pmModelSub() with bad input parameters
    {
        psMemId id = psMemGetId();
        psImage *img = psImageAlloc(NUM_COLS, NUM_ROWS, PS_TYPE_F32);
        psImage *imgS32 = psImageAlloc(NUM_COLS, NUM_ROWS, PS_TYPE_S32);
        psImage *mask = psImageAlloc(NUM_COLS, NUM_ROWS, PS_TYPE_U8);
        for (int i = 0 ; i < NUM_ROWS ; i++) {
            for (int j = 0 ; j < NUM_COLS ; j++) {
                img->data.F32[i][j] = 0.0;
                mask->data.U8[i][j] = 0;
            }
        }
        pmModel *model = pmModelAlloc(pmModelClassGetType("PS_MODEL_GAUSS"));
        psF32 *PAR = model->params->data.F32;
        PAR[PM_PAR_XPOS] = (float) (NUM_COLS/2);
        PAR[PM_PAR_YPOS] = (float) (NUM_ROWS/2);
        PAR[PM_PAR_SXX] = 1.0;
        PAR[PM_PAR_SYY] = 1.0;

        // NULL psImage input parameter
        bool rc = pmModelSub(NULL, mask, model, PM_MODEL_OP_FUNC, 1);
        ok(rc == false, "pmModelSub() returned FALSE with NULL psImage input parameter");

        // NULL pmModel input parameter
        rc = pmModelSub(img, mask, NULL, PM_MODEL_OP_FUNC, 1);
        ok(rc == false, "pmModelSub() returned FALSE with NULL pmModel input parameter");

        // Incorrect type psImage input parameter
        rc = pmModelSub(imgS32, mask, model, PM_MODEL_OP_FUNC, 1);
        ok(rc == false, "pmModelSub() returned FALSE with Incorrect type psImage input parameter");

        psFree(img);
        psFree(imgS32);
        psFree(mask);
        psFree(model);
        pmModelClassCleanup();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // call pmModelSub() with acceptable parameters
    // We only test with a single Gaussian model, with no residuals or masks.
    // For completeness, additional tests should be added.
    {
        psMemId id = psMemGetId();
        psImage *img = psImageAlloc(NUM_COLS, NUM_ROWS, PS_TYPE_F32);
        psImage *mask = psImageAlloc(NUM_COLS, NUM_ROWS, PS_TYPE_U8);
        for (int i = 0 ; i < NUM_ROWS ; i++) {
            for (int j = 0 ; j < NUM_COLS ; j++) {
                img->data.F32[i][j] = 0.0;
                mask->data.U8[i][j] = 0;
            }
        }
        pmModel *model = pmModelAlloc(pmModelClassGetType("PS_MODEL_GAUSS"));
        psF32 *PAR = model->params->data.F32;
        PAR[PM_PAR_I0] = 5.0;
        PAR[PM_PAR_XPOS] = 0.0;
        PAR[PM_PAR_YPOS] = 0.0;
        PAR[PM_PAR_XPOS] = (float) (NUM_COLS/2);
        PAR[PM_PAR_YPOS] = (float) (NUM_ROWS/2);
        PAR[PM_PAR_SXX] = 10.0;
        PAR[PM_PAR_SYY] = 10.0;

        bool rc = pmModelSub(img, mask, model, PM_MODEL_OP_FUNC, 1);
        ok(rc == true, "pmModelSub() returned TRUE with acceptable input parameters");
        psVector *x = psVectorAlloc(2, PS_TYPE_F32);
        bool errorFlag = false;
        for (int i = 0 ; i < NUM_ROWS ; i++) {
            for (int j = 0 ; j < NUM_COLS ; j++) {
                x->data.F32[0] = (float) j;
                x->data.F32[1] = (float) i;
                psF32 modF = model->modelFunc (NULL, model->params, x);
                psF32 imgF = img->data.F32[i][j];
                if (!TEST_FLOATS_EQUAL(modF, -imgF)) {
                    diag("ERROR: img[%d][%d] is %.2f, should be %.2f\n", i, j, img->data.F32[i][j], modF);
                    errorFlag = true;
                }
            }
        }
        ok(!errorFlag, "pmModelSub() set the image pixels correctly");
        psFree(x);
        psFree(img);
        psFree(mask);
        psFree(model);
        pmModelClassCleanup();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // XXX: The source code for pmModelAddWithOffset() and pmModelSubWithOffset() is
    // almost exactly the same as the source code for pmModelAdd() and pmModelSub().
    // We do not test them here.
}

