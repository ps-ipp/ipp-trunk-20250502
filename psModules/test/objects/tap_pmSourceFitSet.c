#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include <psmodules.h>
#include "tap.h"
#include "pstap.h"
/* STATUS:
    All functions are tested except:
        pmSourceFitSetCheckLimits()
        pmSourceFitSetFunction()
    Those functions set static variables which aer invisible to test code.

    These functions are very lightly tested and must be augmented:
	pmSourceFitSet()
	pmSourceFitSetMasks()
*/

#define MISC_NUM                32
#define MISC_NAME              "META00"
#define NUM_BIAS_DATA           10
#define TEST_NUM_ROWS           (16)
#define TEST_NUM_COLS           (30)
#define VERBOSE                 0
#define ERR_TRACE_LEVEL         0
#define TEST_FLOATS_EQUAL(X, Y) (abs(X - Y) < 0.0001)
#define NUM_MODELS		5

pmSource *create_pmSource() {
    pmSource *src = pmSourceAlloc();
    if (1) {
        src->pixels = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_F32);
        src->variance = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_F32);
        src->maskObj = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_U8);
        if (1) {
            for (int i = 0 ; i < TEST_NUM_ROWS ; i++) {
                for (int j = 0 ; j < TEST_NUM_COLS ; j++) {
                    src->pixels->data.F32[i][j] = 0.0;
                    src->variance->data.F32[i][j] = 1.0;
                    src->maskObj->data.U8[i][j] = 0;
                }
            }
        }
        if (1) {
            int halfRows = TEST_NUM_ROWS/2;
            int halfCols = TEST_NUM_COLS/2;
            for (int i = halfRows-1 ; i < halfRows+1 ; i++) {
                for (int j = halfCols-1 ; j < halfCols+1 ; j++) {
                    src->pixels->data.F32[i][j] = 1.0;
                }
            }
            src->pixels->data.F32[halfRows][halfCols] = 5.0;
        }
    }
    return(src);
}

bool call_pmSourceFitSet() {
    psArray *modelSet = psArrayAlloc(NUM_MODELS);
    for (int i = 0 ; i < NUM_MODELS ; i++) {
        modelSet->data[i] = (psPtr *) pmModelAlloc(i);
    }
    pmSource *src = create_pmSource();
    bool rc = pmSourceFitSet(src, modelSet, PM_SOURCE_FIT_PSF, 1);
    if (!rc) {
        diag("ERROR: pmSourceFitSet() returned FALSE");
        return(false);
    }
    psFree(src);
    for (int i = 0 ; i < NUM_MODELS ; i++) {
        psFree(modelSet->data[i]);
        modelSet->data[i] = NULL;
    }
    psFree(modelSet);
    pmModelClassCleanup();
  
    return(true);
}


int main(int argc, char* argv[])
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    psTraceSetLevel("err", ERR_TRACE_LEVEL);
    plan_tests(70);


    // ----------------------------------------------------------------------
    // pmSourceFitSetDataAlloc() tests
    // Call pmSourceFitSetDataAlloc() with NULL psArray input parameter
    {
        psMemId id = psMemGetId();
        psArray *modelSet = psArrayAlloc(NUM_MODELS);
        for (int i = 0 ; i < modelSet->n ; i++) {
            modelSet->data[i] = (psPtr *) pmModelAlloc(1);
	}
        pmSourceFitSetData *fitSetData = pmSourceFitSetDataAlloc(NULL);
        ok(fitSetData == NULL, "pmSourceFitSetDataAlloc() returned NULL with NULL psArray input parameter");
        psFree(fitSetData);
        pmModelClassCleanup();
        psFree(modelSet); 
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmSourceFitSetDataAlloc() with acceptable input parameters
    {
        psMemId id = psMemGetId();
        psArray *modelSet = psArrayAlloc(NUM_MODELS);
        for (int i = 0 ; i < modelSet->n ; i++) {
            modelSet->data[i] = (psPtr *) pmModelAlloc(i);
	}
        pmSourceFitSetData *set = pmSourceFitSetDataAlloc(modelSet);
        ok(set != NULL && psMemCheckSourceFitSetData(set),
          "pmSourceFitSetDataAlloc() returned non-NULL with acceptable input parameters");
        ok(set->paramSet != NULL && set->paramSet->n == modelSet->n, 
          "pmSourceFitSetDataAlloc() set the set->paramSet psArray correctly");
        ok(set->derivSet != NULL && set->derivSet->n == modelSet->n, 
          "pmSourceFitSetDataAlloc() set the set->derivSet psArray correctly");
        for (int i = 0 ; i < modelSet->n ; i++) {
            int nParams = pmModelClassParameterCount(i);
            psVector *tmpV = (psVector *) (set->paramSet->data[i]);
            ok(tmpV != NULL && psMemCheckVector(tmpV) && tmpV->n == nParams,
               "pmSourceFitSetDataAlloc() set the set->paramSet->data psVector correctly");

            tmpV = (psVector *) (set->derivSet->data[i]);
            ok(tmpV != NULL && psMemCheckVector(tmpV) && tmpV->n == nParams,
               "pmSourceFitSetDataAlloc() set the set->derivSet->data psVector correctly");

            bool errorFlag = false;
            for (int i = 0 ; i < tmpV->n ; i++) {
                if (tmpV->data.F32[i] != 0.0) {
                    errorFlag = true;
		}
	    }
            ok(!errorFlag, "pmSourceFitSetDataAlloc() set the set->derivSet->data correctly");
	}
        
        psFree(set);
        pmModelClassCleanup();
        psFree(modelSet); 
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // pmSourceFitSetCheckLimits() tests
    // Call pmSourceFitSetCheckLimits() with thisSet == NULL
    // XXX: Can not test full functionality of pmSourceFitSetCheckLimits() because it
    // requires that the static variable thisSet be allocated.
    {
        psMemId id = psMemGetId();
        #define NUM_PARAMS 10
        psF32 *params = (psF32 *) psAlloc(NUM_PARAMS * sizeof(psF32));
        psF32 *betas = (psF32 *) psAlloc(NUM_PARAMS * sizeof(psF32));
        bool rc = pmSourceFitSetCheckLimits(PS_MINIMIZE_PARAM_MIN, 10, params, betas);
        ok(rc == false, "pmSourceFitSetCheckLimits() returned NULL with thisSet == FALSE");
        psFree(params);
        psFree(betas);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // pmSourceFitSetFunction() tests
    // Call pmSourceFitSetFunction() with thisSet == NULL
    // XXX: Can not test full functionality of pmSourceFitSetCheckLimits() because it
    // requires that the static variable thisSet be allocated.
    {
        psMemId id = psMemGetId();
        #define NUM_PARAMS 10
        psVector *deriv = psVectorAlloc(NUM_PARAMS, PS_TYPE_F32);
        psVector *param = psVectorAlloc(NUM_PARAMS, PS_TYPE_F32);
        psVector *x = psVectorAlloc(NUM_PARAMS, PS_TYPE_F32);
        psF32 tmpF = pmSourceFitSetFunction(deriv, param, x);
        ok(isnan(tmpF), "pmSourceFitSetFunction() returned NULL with thisSet == FALSE");
        psFree(deriv);
        psFree(param);
        psFree(x);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // pmSourceFitSetJoin() tests
    // Call pmSourceFitSetJoin() with NULL pmSourceFitSetData input parameter
    {
        psMemId id = psMemGetId();
        #define NUM_PARAMS 10
        psVector *deriv = psVectorAlloc(NUM_PARAMS, PS_TYPE_F32);
        psVector *param = psVectorAlloc(NUM_PARAMS, PS_TYPE_F32);
        psArray *modelSet = psArrayAlloc(NUM_MODELS);
        for (int i = 0 ; i < modelSet->n ; i++) {
            modelSet->data[i] = (psPtr *) pmModelAlloc(i);
	}
        pmSourceFitSetData *set = pmSourceFitSetDataAlloc(modelSet);
        bool rc = pmSourceFitSetJoin(deriv, param, NULL);
        ok(rc == false, "pmSourceFitSetJoin() returned FALSE with NULL pmSourceFitSetData input parameter");
        psFree(deriv);
        psFree(param);
        psFree(modelSet); 
        psFree(set);
        pmModelClassCleanup();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmSourceFitSetJoin() with unequal size set->paramSet and set->derivSet input parameters
    {
        psMemId id = psMemGetId();
        psVector *deriv = psVectorAlloc(1000, PS_TYPE_F32);
        psVector *param = psVectorAlloc(1000, PS_TYPE_F32);
        psArray *modelSet = psArrayAlloc(NUM_MODELS);
        for (int i = 0 ; i < modelSet->n ; i++) {
            modelSet->data[i] = (psPtr *) pmModelAlloc(i);
	}
        pmSourceFitSetData *set = pmSourceFitSetDataAlloc(modelSet);
        psFree(set->paramSet);
        set->paramSet = psArrayAlloc(set->derivSet->n + 1);
        bool rc = pmSourceFitSetJoin(deriv, param, set);
        ok(rc == false, "pmSourceFitSetJoin() returned FALSE with unequal size set->paramSet and set->derivSet input parameters");
        psFree(deriv);
        psFree(param);
        psFree(modelSet); 
        psFree(set);
        pmModelClassCleanup();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmSourceFitSetJoin() with deriv and param input psVector too small
    // XXX: Must add a PS_ASSERT to the source code to detect this
    if (0) {
        psMemId id = psMemGetId();
        psVector *small = psVectorAlloc(1, PS_TYPE_F32);
        psVector *big = psVectorAlloc(1000, PS_TYPE_F32);
        psArray *modelSet = psArrayAlloc(NUM_MODELS);
        for (int i = 0 ; i < modelSet->n ; i++) {
            modelSet->data[i] = (psPtr *) pmModelAlloc(i);
	}
        pmSourceFitSetData *set = pmSourceFitSetDataAlloc(modelSet);
        bool rc = pmSourceFitSetJoin(small, big, set);
        ok(rc == false, "pmSourceFitSetJoin() returned FALSE with deriv input psVector too small");
        rc = pmSourceFitSetJoin(big, small, set);
        ok(rc == false, "pmSourceFitSetJoin() returned FALSE with param input psVector too small");
        psFree(small);
        psFree(big);
        psFree(modelSet); 
        psFree(set);
        pmModelClassCleanup();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmSourceFitSetJoin() with acceptable input parameters
    {
        psMemId id = psMemGetId();
        psVector *deriv = psVectorAlloc(1000, PS_TYPE_F32);
        psVector *param = psVectorAlloc(1000, PS_TYPE_F32);
        psArray *modelSet = psArrayAlloc(NUM_MODELS);
        for (int i = 0 ; i < modelSet->n ; i++) {
            modelSet->data[i] = (psPtr *) pmModelAlloc(i);
	}
        pmSourceFitSetData *set = pmSourceFitSetDataAlloc(modelSet);

        psF32 cnt = 0.0;
        for (int i = 0; i < set->paramSet->n; i++) {
            psVector *paramOne = set->paramSet->data[i];
            psVector *derivOne = set->derivSet->data[i];
            for (int j = 0; j < paramOne->n; j++) {
                paramOne->data.F32[j] = cnt;
                derivOne->data.F32[j] = cnt;
                cnt = cnt + 1.0;
            }
        }

        bool rc = pmSourceFitSetJoin(deriv, param, set);
        ok(rc == true, "pmSourceFitSetJoin() returned TRUE acceptable input parameters");

        bool errorFlag = false;
        for (int i = 0; i < (int) cnt ; i++) {
            if (deriv->data.F32[i] != (float) i) {
                diag("ERROR: deriv->data.F32[%d] is %.2ff, should be %.2f\n", i, deriv->data.F32[i], (float) i);
                errorFlag = true;
            }
            if (param->data.F32[i] != (float) i) {
                diag("ERROR: param->data.F32[%d] is %.2ff, should be %.2f\n", i, param->data.F32[i], (float) i);
                errorFlag = true;
            }
        }
        ok(!errorFlag, "pmSourceFitSetJoin() set the deriv and param psVectors correctly");

        psFree(deriv);
        psFree(param);
        psFree(modelSet); 
        psFree(set);
        pmModelClassCleanup();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // pmSourceFitSetSplit() tests
    // Call pmSourceFitSetSplit() with NULL pmSourceFitSetData input parameter
    {
        psMemId id = psMemGetId();
        #define NUM_PARAMS 10
        psVector *deriv = psVectorAlloc(NUM_PARAMS, PS_TYPE_F32);
        psVector *param = psVectorAlloc(NUM_PARAMS, PS_TYPE_F32);
        psArray *modelSet = psArrayAlloc(NUM_MODELS);
        for (int i = 0 ; i < modelSet->n ; i++) {
            modelSet->data[i] = (psPtr *) pmModelAlloc(i);
	}
        pmSourceFitSetData *set = pmSourceFitSetDataAlloc(modelSet);
        bool rc = pmSourceFitSetSplit(NULL, deriv, param);
        ok(rc == false, "pmSourceFitSetSplit() returned FALSE with NULL pmSourceFitSetData input parameter");
        psFree(deriv);
        psFree(param);
        psFree(modelSet); 
        psFree(set);
        pmModelClassCleanup();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmSourceFitSetSplit() with NULL src->paramSet and src->derivSet input parameters and
    // src->paramSet and src->derivSet of unequal size
    {
        psMemId id = psMemGetId();
        #define NUM_PARAMS 10
        psVector *deriv = psVectorAlloc(NUM_PARAMS, PS_TYPE_F32);
        psVector *param = psVectorAlloc(NUM_PARAMS, PS_TYPE_F32);
        psArray *modelSet = psArrayAlloc(NUM_MODELS);
        for (int i = 0 ; i < modelSet->n ; i++) {
            modelSet->data[i] = (psPtr *) pmModelAlloc(i);
	}
        pmSourceFitSetData *set = pmSourceFitSetDataAlloc(modelSet);
        psArray *tmpArray = set->paramSet;
        set->paramSet = NULL;
        bool rc = pmSourceFitSetSplit(set, deriv, param);
        ok(rc == false, "pmSourceFitSetSplit() returned FALSE with NULL src->paramSet input parameter");
        set->paramSet = tmpArray;

        tmpArray = set->derivSet;
        set->derivSet = NULL;
        rc = pmSourceFitSetSplit(set, deriv, param);
        ok(rc == false, "pmSourceFitSetSplit() returned FALSE with NULL src->derivSet input parameter");
        set->derivSet = tmpArray;

        psFree(set->paramSet);
        set->paramSet = psArrayAlloc(set->derivSet->n + 1);
        ok(rc == false, "pmSourceFitSetSplit() returned FALSE with src->paramSet and src->derivSet of unequal size");

        psFree(deriv);
        psFree(param);
        psFree(modelSet); 
        psFree(set);
        pmModelClassCleanup();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmSourceFitSetSplit() with NULL param input parameter
    {
        psMemId id = psMemGetId();
        psVector *deriv = psVectorAlloc(1000, PS_TYPE_F32);
        psVector *param = psVectorAlloc(1000, PS_TYPE_F32);
        for (int i = 0 ; i < 1000 ; i++) {
            param->data.F32[i] = (float) i;
            deriv->data.F32[i] = (float) i;
        }
        psArray *modelSet = psArrayAlloc(NUM_MODELS);
        for (int i = 0 ; i < modelSet->n ; i++) {
            modelSet->data[i] = (psPtr *) pmModelAlloc(i);
	}
        pmSourceFitSetData *set = pmSourceFitSetDataAlloc(modelSet);
        bool rc = pmSourceFitSetSplit(set, deriv, NULL);
        ok(rc == false, "pmSourceFitSetSplit() returned FALSE with NULL param input parameter");
        psFree(deriv);
        psFree(param);
        psFree(modelSet); 
        psFree(set);
        pmModelClassCleanup();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmSourceFitSetSplit() with acceptable input parameters
    {
        psMemId id = psMemGetId();
        psVector *deriv = psVectorAlloc(1000, PS_TYPE_F32);
        psVector *param = psVectorAlloc(1000, PS_TYPE_F32);
        for (int i = 0 ; i < 1000 ; i++) {
            deriv->data.F32[i] = (float) i;
            param->data.F32[i] = (float) i;
        }
        psArray *modelSet = psArrayAlloc(NUM_MODELS);
        for (int i = 0 ; i < modelSet->n ; i++) {
            modelSet->data[i] = (psPtr *) pmModelAlloc(i);
	}
        pmSourceFitSetData *set = pmSourceFitSetDataAlloc(modelSet);
        bool rc = pmSourceFitSetSplit(set, deriv, param);
        ok(rc == true, "pmSourceFitSetSplit() returned FALSE with acceptable input parameters");

        bool errorFlag = false;
        psF32 cnt = 0.0;
        for (int i = 0; i < set->paramSet->n; i++) {
            psVector *paramOne = set->paramSet->data[i];
            psVector *derivOne = set->derivSet->data[i];
            for (int j = 0; j < paramOne->n; j++) {
                if (paramOne->data.F32[j] != cnt) {
                    diag("ERROR: paramOne->data.F32[%d] is %.2ff, should be %.2f\n", i, paramOne->data.F32[i], (float) i);
                    errorFlag = true;
                }
                if (derivOne->data.F32[j] != cnt) {
                    diag("ERROR: derivOne->data.F32[%d] is %.2ff, should be %.2f\n", i, derivOne->data.F32[i], (float) i);
                    errorFlag = true;
                }
                cnt = cnt + 1.0;
            }
        }
        ok(!errorFlag, "pmSourceFitSetSplit() set the deriv and param psVectors correctly");

        psFree(deriv);
        psFree(param);
        psFree(modelSet); 
        psFree(set);
        pmModelClassCleanup();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // pmSourceFitSetValues() tests
    // Call pmSourceFitSetValues() with bad input parameters
    {
        psMemId id = psMemGetId();
        #define VEC_SIZE 1000
        #define NUM_ITER 32
        #define TOL 0.1
        psVector *param = psVectorAlloc(VEC_SIZE, PS_TYPE_F32);
        psVector *dparam = psVectorAlloc(VEC_SIZE, PS_TYPE_F32);
        psArray *modelSet = psArrayAlloc(NUM_MODELS);
        for (int i = 0 ; i < modelSet->n ; i++) {
            modelSet->data[i] = (psPtr *) pmModelAlloc(i);
	}
        pmSourceFitSetData *set = pmSourceFitSetDataAlloc(modelSet);
        pmSource *src = create_pmSource();
        psMinimization *myMin = psMinimizationAlloc(NUM_ITER, TOL);
        int nPix = 10;
        bool fitStatus = true;
        // NULL set input parameter
        bool rc = pmSourceFitSetValues(NULL, dparam, param, src, myMin, nPix, fitStatus);
        ok(rc == false, "pmSourceFitSetValues() returned FALSE with NULL set input parameter");

        // NULL set->paramSet
        psArray *tmpArray = set->paramSet;
        set->paramSet = NULL;
        rc = pmSourceFitSetValues(set, dparam, param, src, myMin, nPix, fitStatus);
        ok(rc == false, "pmSourceFitSetValues() returned FALSE with NULL set->paramSet");
        set->paramSet = tmpArray;

        // NULL dparam input parameter
        rc = pmSourceFitSetValues(set, NULL, param, src, myMin, nPix, fitStatus);
        ok(rc == false, "pmSourceFitSetValues() returned FALSE with NULL dparam input parameter");

        // NULL param input parameter
        rc = pmSourceFitSetValues(set, dparam, NULL, src, myMin, nPix, fitStatus);
        ok(rc == false, "pmSourceFitSetValues() returned FALSE with NULL param input parameter");

        // NULL pmSource input parameter
        rc = pmSourceFitSetValues(set, dparam, param, NULL, myMin, nPix, fitStatus);
        ok(rc == false, "pmSourceFitSetValues() returned FALSE with NULL pmSource input parameter");

        // NULL pmSource->pixels input parameter
        psImage *tmpImg = src->pixels;
        src->pixels = NULL;
        rc = pmSourceFitSetValues(set, dparam, param, src, myMin, nPix, fitStatus);
        ok(rc == false, "pmSourceFitSetValues() returned FALSE with NULL pmSource->pixels input parameter");
        src->pixels = tmpImg;

        // NULL psMinimization input parameter
        rc = pmSourceFitSetValues(set, dparam, param, src, NULL, nPix, fitStatus);
        ok(rc == false, "pmSourceFitSetValues() returned FALSE with NULL psMinimization input parameter");

        psFree(param);
        psFree(dparam);
        psFree(modelSet); 
        psFree(set);
        psFree(src);
        psFree(myMin);
        pmModelClassCleanup();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // pmSourceFitSetValues() tests
    // Call pmSourceFitSetValues() with acceptable input parameters
    {
        psMemId id = psMemGetId();
        #define VEC_SIZE 1000
        #define NUM_ITER 32
        #define TOL 0.1
        #define MIN_VALUE	22.0
        #define NUM_PIX 100
        psVector *param = psVectorAlloc(VEC_SIZE, PS_TYPE_F32);
        psVector *dparam = psVectorAlloc(VEC_SIZE, PS_TYPE_F32);
        for (int i = 0 ; i < VEC_SIZE ; i++) {
            param->data.F32[i] = (float) i;
            dparam->data.F32[i] = (float) i;
        }
        psArray *modelSet = psArrayAlloc(NUM_MODELS);
        for (int i = 0 ; i < modelSet->n ; i++) {
            modelSet->data[i] = (psPtr *) pmModelAlloc(i);
	}
        pmSourceFitSetData *set = pmSourceFitSetDataAlloc(modelSet);
        pmSource *src = create_pmSource();
        psMinimization *myMin = psMinimizationAlloc(NUM_ITER, TOL);
        int nPix = NUM_PIX;
        bool fitStatus = true;

        bool rc = pmSourceFitSetValues(set, dparam, param, src, myMin, nPix, fitStatus);
        ok(rc == true, "pmSourceFitSetValues() returned TRUE with acceptable input paramaters");

        bool errorFlag = false;
        psF32 cnt = 0.0;
        for (int i = 0; i < set->paramSet->n; i++) {
            pmModel *model = set->modelSet->data[i];
            for (int j = 0; j < model->params->n; j++) {
                if (model->params->data.F32[j] != cnt) {
                    diag("ERROR: model->params->data.F32[%d] is %.2ff, should be %.2f\n", i, model->params->data.F32[i], (float) i);
                    errorFlag = true;
                }
                if (model->dparams->data.F32[j] != cnt) {
                    diag("ERROR: model->dparams->data.F32[%d] is %.2ff, should be %.2f\n", i, model->dparams->data.F32[i], (float) i);
                    errorFlag = true;
                }
                if (model->chisq != myMin->value) {
                    diag("ERROR: model->chisq is %.2f, should be %.2f", model->chisq, MIN_VALUE);
                    errorFlag = true;
                }
                if (model->nIter != myMin->iter) {
                    diag("ERROR: model->nIter is %.2f, should be %.2f", model->nIter, NUM_ITER);
                    errorFlag = true;
                }
                if (model->nDOF != NUM_PIX - model->params->n) {
                    diag("ERROR: model->nDOF is %d, should be %d", model->nDOF, NUM_PIX-model->params->n);
                    errorFlag = true;
                }

                cnt = cnt + 1.0;
            }
        }
        ok(!errorFlag, "pmSourceFitSetValues() set the deriv and param psVectors correctly");

        psFree(param);
        psFree(dparam);
        psFree(modelSet); 
        psFree(set);
        psFree(src);
        psFree(myMin);
        pmModelClassCleanup();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }



    // ----------------------------------------------------------------------
    // pmSourceFitSetMasks() tests
    // Call pmSourceFitSetMasks() with bad input parameters
    {
        psMemId id = psMemGetId();
        #define VEC_SIZE 1000
        #define NUM_ITER 32
        #define TOL 0.1
        psArray *modelSet = psArrayAlloc(NUM_MODELS);
        for (int i = 0 ; i < modelSet->n ; i++) {
            modelSet->data[i] = (psPtr *) pmModelAlloc(i);
	}
        pmSourceFitSetData *set = pmSourceFitSetDataAlloc(modelSet);
        psMinConstraint *constraint = psMinConstraintAlloc();

        // NULL psMinConstraint input parameter
        bool rc = pmSourceFitSetMasks(NULL, set, PM_SOURCE_FIT_NORM);
        ok(rc == false, "pmSourceFitSetMasks() returned TRUE with NULL psMinConstraint input parameter");

        // NULL pmSourceFitSetData input parameter
        rc = pmSourceFitSetMasks(constraint, NULL, PM_SOURCE_FIT_NORM);
        ok(rc == false, "pmSourceFitSetMasks() returned TRUE with NULL pmSourceFitSetData input parameter");

        psFree(modelSet); 
        psFree(set);
        psFree(constraint);
        pmModelClassCleanup();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmSourceFitSetMasks() with acceptable input parameters
    // For thoroughness, we should test the PM_SOURCE_FIT_PSF and PM_SOURCE_FIT_EXT mode
    {
        psMemId id = psMemGetId();
        #define VEC_SIZE 1000
        #define NUM_ITER 32
        #define TOL 0.1
        psArray *modelSet = psArrayAlloc(NUM_MODELS);
        for (int i = 0 ; i < modelSet->n ; i++) {
            modelSet->data[i] = (psPtr *) pmModelAlloc(i);
	}
        pmSourceFitSetData *set = pmSourceFitSetDataAlloc(modelSet);
        psMinConstraint *constraint = psMinConstraintAlloc();
        constraint->paramMask = psVectorAlloc(1000, PS_TYPE_F32);

        // Acceptable input parameters
        bool rc = pmSourceFitSetMasks(constraint, set, PM_SOURCE_FIT_NORM);
        ok(rc == true, "pmSourceFitSetMasks() returned TRUE with acceptable input parameters");

        bool errorFlag = false;
        int n = 0;
        for (int i = 0; i < set->paramSet->n; i++) {
            psVector *paramOne = set->paramSet->data[i];
            for (int j = 0; j < paramOne->n; j++) {
                if (j == PM_PAR_I0) continue;
                if (constraint->paramMask->data.U8[n + j] != 1) {
                    diag("ERROR: constraint->paramMask->data.U8[%d] is %d, should be a",
                          n + j, constraint->paramMask->data.U8[n + j]);
                    errorFlag = true;
                }
            }
            n += paramOne->n;
        }
        ok(!errorFlag, "pmSourceFitSetMasks() constraint->paramMask psVector correctly");

        psFree(modelSet); 
        psFree(set);
        psFree(constraint);
        pmModelClassCleanup();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // pmSourceFitSet() tests
    // Call pmSourceFitSet() with NULL psSource input parameter
    {
        psMemId id = psMemGetId();
        psArray *modelSet = psArrayAlloc(NUM_MODELS);
        for (int i = 0 ; i < NUM_MODELS ; i++) {
            modelSet->data[i] = (psPtr *) pmModelAlloc(i);
	}
        pmSource *src = create_pmSource();
        bool rc = pmSourceFitSet(NULL, modelSet, PM_SOURCE_FIT_PSF, 1);
        ok(rc == false, "pmSourceFitSet() returned FALSE with NULL psSource input parameter");
        psFree(src);
        for (int i = 0 ; i < NUM_MODELS ; i++) {
            psFree(modelSet->data[i]);
            modelSet->data[i] = NULL;
        }
        psFree(modelSet);
        pmModelClassCleanup();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmSourceFitSet() with NULL psSource pixels, variance, maskObj input parameters
    {
        psMemId id = psMemGetId();
        psArray *modelSet = psArrayAlloc(NUM_MODELS);
        for (int i = 0 ; i < NUM_MODELS ; i++) {
            modelSet->data[i] = (psPtr *) pmModelAlloc(i);
	}
        pmSource *src = create_pmSource();
        psImage *tmpImg = src->pixels;
        src->pixels = NULL;
        bool rc = pmSourceFitSet(src, modelSet, PM_SOURCE_FIT_PSF, 1);
        ok(rc == false, "pmSourceFitSet() returned FALSE with NULL src->pixels input parameter");
        src->pixels = tmpImg;

        tmpImg = src->variance;
        src->variance = NULL;
        rc = pmSourceFitSet(src, modelSet, PM_SOURCE_FIT_PSF, 1);
        ok(rc == false, "pmSourceFitSet() returned FALSE with NULL src->variance input parameter");
        src->variance = tmpImg;

        tmpImg = src->maskObj;
        src->maskObj = NULL;
        rc = pmSourceFitSet(src, modelSet, PM_SOURCE_FIT_PSF, 1);
        ok(rc == false, "pmSourceFitSet() returned FALSE with NULL src->maskObj input parameter");
        src->maskObj = tmpImg;
        psFree(src);
        for (int i = 0 ; i < NUM_MODELS ; i++) {
            psFree(modelSet->data[i]);
            modelSet->data[i] = NULL;
        }
        psFree(modelSet);
        pmModelClassCleanup();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmSourceFitSet() with acceptable input parameters
    // This is a verly limited test.  It only uses a simple object in the pmSource
    // parameter and simply tests that pmSourceFitSet() returns teh correct type and mode.
    // Must add more extensive input types.
    if (1) {
        psMemId id = psMemGetId();
        psArray *modelSet = psArrayAlloc(NUM_MODELS);
        for (int i = 0 ; i < NUM_MODELS ; i++) {
            modelSet->data[i] = (psPtr *) pmModelAlloc(i);
	}
        pmSource *src = create_pmSource();
        bool rc = pmSourceFitSet(src, modelSet, PM_SOURCE_FIT_PSF, 1);
        ok(rc == true, "pmSourceFitSet() returned TRUE with acceptable parameters");
        ok(src->mode & PM_SOURCE_MODE_FITTED, "pmSourceFitSet() set source->mode |= PM_SOURCE_MODE_FITTED (%d)", src->mode);
        ok(src->type == PM_SOURCE_TYPE_UNKNOWN, "pmSourceFitSet() set source->type correctly (%d)", src->type);

        psFree(src);
        for (int i = 0 ; i < NUM_MODELS ; i++) {
            psFree(modelSet->data[i]);
            modelSet->data[i] = NULL;
        }
        psFree(modelSet);
        pmModelClassCleanup();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
}
