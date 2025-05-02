/*****************************************************************************
    This routine must ensure that psMinimizeLM() works correctly.
 
    XXX: This code needs a lot of additional test case work.
    XXX: Use the tst_template.
    XXX: Print headers and footers.
    XXX: Why are we flushing stdout?
 *****************************************************************************/
#include <stdio.h>
#include "pslib_strict.h"
#include "psTest.h"
#include <math.h>
#define NUM_ITERATIONS 100
#define ERR_TOL 1e-6
#define NUM_DATA_POINTS 300
#define NUM_PARAMS 3
#define VERBOSE 0
float expectedParm[NUM_PARAMS];


float function(const psVector *params,      // Paramters
               const psVector *x            // Ordinate
              )
{
    return params->data.F32[0] *
           expf(- (PS_SQR(x->data.F32[0]) + PS_SQR(x->data.F32[1]) / 2.0 / PS_SQR(params->data.F32[1]))) +
           params->data.F32[2];
}

void derivatives(psVector *deriv,       // Derivatives
                 const psVector *params,      // Paramters
                 const psVector *x            // Ordinate
                )
{
    deriv->data.F32[0] = expf(- (PS_SQR(x->data.F32[0]) + PS_SQR(x->data.F32[1]) / 2.0 / PS_SQR(params->data.F32[1])));
    deriv->data.F32[1] = params->data.F32[0] * (PS_SQR(x->data.F32[0]) + PS_SQR(x->data.F32[1]) / 2.0 / PS_SQR(params->data.F32[1])) / params->data.F32[1] / params->data.F32[1] / params->data.F32[1] * expf(- (PS_SQR(x->data.F32[0]) + PS_SQR(x->data.F32[1]) / 2.0 / PS_SQR(params->data.F32[1])));
    deriv->data.F32[2] = 1.0;
}



/*****************************************************************************
myFunc():
    sum = param[0] * x[0] * x[1] +
          param[1] * x[0] +
          param[2] * x[0]^2 +
          param[3] * x[1] +
          param[4] * x[1]^2
 
 *****************************************************************************/
psF32 fitFunc(psVector *deriv,
              psVector *params,
              psVector *x)
{
    if ((deriv == NULL) || (params == NULL) || (x == NULL)) {
        psError(PS_ERR_UNKNOWN, true, "deriv or params or x is NULL.\n");
    }

    derivatives(deriv, params, x);
    return function(params, x);
}

psS32 t01()
{
    psS32 currentId = psMemGetId();
    psBool testStatus = true;

    psRandom *rng = psRandomAllocSpecific(PS_RANDOM_TAUS, 1); // Random number generator; using known seed
    psMinimization *min = psMinimizationAlloc(NUM_ITERATIONS, ERR_TOL);
    psArray *ordinates = psArrayAlloc(NUM_DATA_POINTS);
    psVector *coordinates = psVectorAlloc(NUM_DATA_POINTS, PS_TYPE_F32);
    psVector *errors = psVectorAlloc(NUM_DATA_POINTS, PS_TYPE_F32);
    ordinates->n = NUM_DATA_POINTS;
    coordinates->n = NUM_DATA_POINTS;
    errors->n = NUM_DATA_POINTS;
    psVector *params = psVectorAlloc(NUM_PARAMS, PS_TYPE_F32);
    params->n = NUM_PARAMS;
    psVector *trueParams = psVectorAlloc(NUM_PARAMS, PS_TYPE_F32);
    trueParams->n = NUM_PARAMS;

    trueParams->data.F32[0] = 100.0;    // Normalisation
    trueParams->data.F32[1] = 3.0;      // Width
    trueParams->data.F32[2] = 10.0;     // Background

    // Set parameters
    for (long i = 0; i < NUM_PARAMS; i++) {
        // So we're not starting right on the true value:
        params->data.F32[i] = trueParams->data.F32[i] * (1.0 - psRandomGaussian(rng) / 10.0);
    }

    for (long i = 0; i < NUM_DATA_POINTS; i++) {
        psVector *x = psVectorAlloc(2, PS_TYPE_F32);
        x->data.F32[0] = 10.0 * psRandomUniform(rng) - 5.0;
        x->data.F32[1] = 10.0 * psRandomUniform(rng) - 5.0;
        ordinates->data[i] = x;
        // Add some noise
        coordinates->data.F32[i] = function(params, x) + 0.1 * (2.0 * psRandomGaussian(rng) - 1.0);
        errors->data.F32[i] = 0.1;
        if (VERBOSE) {
            printf("Data %ld: (%f, %f) --> %f\n",
                   i, x->data.F32[0], x->data.F32[1], coordinates->data.F32[i]);
        }
    }

    if (!psMinimizeLMChi2(min, NULL, params, NULL, ordinates, coordinates, errors,
                          (psMinimizeLMChi2Func)fitFunc)) {
        printf("TEST ERROR: psMinimizeLMChi2() returned FALSE.\n");
        fflush(stdout);
        testStatus = false;
    } else {
        printf("Minimisation took %d iterations\n", min->iter);
        printf("chi^2 at the minimum is %.3g\n", min->value);
        for (long i = 0; i < NUM_PARAMS; i++) {
            printf("Parameter %ld at the minimum is %.3f, expected: %f\n", i,
                   params->data.F32[i], trueParams->data.F32[i]);
            fflush(stdout);
        }
        float diff = 0.0;
        for (long i = 0; i < NUM_DATA_POINTS; i++) {
            psVector *x = ordinates->data[i];
            float fitted = function(trueParams, x);
            float expected = function(params, x);
            diff += (fitted - expected) / fabsf(expected);
            if (VERBOSE) {
                printf("Data point %ld: Fitted: %f, expected: %f\n", i, fitted, expected);
            }
            fflush(stdout);
        }
        printf("Mean relative difference is %f\n", diff/(float)NUM_DATA_POINTS);

    }

    psFree(min);
    psFree(params);
    psFree(trueParams);
    psFree(ordinates);
    psFree(coordinates);
    psFree(errors);
    psFree(rng);
    psMemCheckCorruption(1);
    psS32 memLeaks = psMemCheckLeaks(currentId, NULL, NULL, false);
    if (0 != memLeaks) {
        printf("TEST ERROR: Memory Leaks! (%d leaks).\n", memLeaks);
        fflush(stdout);
        // XXX: This is causing a seg fault
        psAbort("Memory Leaks! (%d leaks)", memLeaks);
    }

    return (testStatus);
}

#define NUM_ITER 10
#define TOL  20.0
psS32 tst_psMinimizationAlloc()
{
    psS32 currentId = psMemGetId();
    psBool testStatus = true;

    psMinimization *tmp = psMinimizationAlloc(NUM_ITER, TOL);
    if (tmp == NULL) {
        printf("TEST ERROR: psMinimizationAlloc() returned FALSE.\n");
        testStatus = false;
    } else {
        if (tmp->maxIter != NUM_ITER) {
            printf("TEST ERROR: psMinimizationAlloc() did not properly set ->maxIter.\n");
            testStatus = false;
        }

        if (tmp->tol != TOL) {
            printf("TEST ERROR: psMinimizationAlloc() did not properly set ->tol.\n");
            testStatus = false;
        }

        if (tmp->value != 0.0) {
            printf("TEST ERROR: psMinimizationAlloc() did not properly set ->value.\n");
            testStatus = false;
        }

        if (tmp->iter != 0) {
            printf("TEST ERROR: psMinimizationAlloc() did not properly set ->iter.\n");
            testStatus = false;
        }

        if (!isnan(tmp->lastDelta)) {
            printf("TEST ERROR: psMinimizationAlloc() did not properly set ->lastDelta.\n");
            testStatus = false;
        }
        psFree(tmp);
    }

    psMemCheckCorruption(1);
    psS32 memLeaks = psMemCheckLeaks(currentId, NULL, NULL, false);
    if (0 != memLeaks) {
        printf("TEST ERROR: Memory Leaks! (%d leaks).\n", memLeaks);
        testStatus = false;
    }

    return(testStatus);
}


#define NUM_ITER 10
#define TOL  20.0
psS32 tst_psMinConstrainAlloc()
{
    psS32 currentId = psMemGetId();
    psBool testStatus = true;

    psMinConstrain *tmp = psMinConstrainAlloc(NUM_ITER, TOL);
    if (tmp == NULL) {
        printf("TEST ERROR: psMinConstrainAlloc() returned FALSE.\n");
        testStatus = false;
    } else {
        if (tmp->paramMask != NULL) {
            printf("TEST ERROR: psMinConstrainAlloc() did not properly set ->paramMask.\n");
            testStatus = false;
        }

        if (tmp->paramMax != NULL) {
            printf("TEST ERROR: psMinConstrainAlloc() did not properly set ->paramMax.\n");
            testStatus = false;
        }

        if (tmp->paramMin != NULL) {
            printf("TEST ERROR: psMinConstrainAlloc() did not properly set ->paramMin.\n");
            testStatus = false;
        }

        if (tmp->paramDelta != NULL) {
            printf("TEST ERROR: psMinConstrainAlloc() did not properly set ->paramDelta.\n");
            testStatus = false;
        }

        psFree(tmp);
    }

    psMemCheckCorruption(1);
    psS32 memLeaks = psMemCheckLeaks(currentId, NULL, NULL, false);
    if (0 != memLeaks) {
        printf("TEST ERROR: Memory Leaks! (%d leaks).\n", memLeaks);
        testStatus = false;
    }

    return(testStatus);
}


psS32 main()
{
    psLogSetFormat("HLNM");
    psTraceSetDestination(1);
    psTraceSetLevel(".", 0);
    psTraceSetLevel(__func__, 0);
    psTraceSetLevel("t01", 0);
    psTraceSetLevel("psMinimizeLMChi2_OLD", 0);
    psTraceSetLevel("psMinimizeLMChi2", 0);
    psTraceSetLevel("psMinimizeGaussNewtonDelta", 0);
    psTraceSetLevel("psMinimizeGaussNewtonDelta_EAM", 0);
    psTraceSetLevel("p_psMinLM_GuessABP", 0);
    psTraceSetLevel("p_psMinLM_GuessABP_EAM", 0);
    psTraceSetLevel("p_psMinLM_SetABX", 0);
    psTraceSetLevel("psGaussJordan", 0);
    psTraceSetLevel("psMinimizationAlloc", 0);
    psTraceSetLevel("psMinConstrainAlloc", 0);
    psTraceSetLevel("psMemCheckMinimization", 0);
    psTraceSetLevel("p_psDetermineBracket", 0);
    psTraceSetLevel("p_psDetermineBracket2", 0);
    psTraceSetLevel("p_psLineMin", 0);
    psTraceSetLevel("psMinimizePowell", 0);
    psTraceSetLevel("myPowellChi2Func", 0);
    psTraceSetLevel("psMinimizeChi2Powell", 0);
    psTraceSetLevel("BuildSums1D", 0);
    psTraceSetLevel("BuildSums2D", 0);
    psTraceSetLevel("Polynomial2DEvalVectorD", 0);
    psTraceSetLevel("vectorFitPolynomial1DCheby", 0);
    psTraceSetLevel("VectorFitPolynomial1DOrd", 0);
    psTraceSetLevel("psVectorFitPolynomial1D", 0);
    psTraceSetLevel("psVectorClipFitPolynomial1D", 0);

    psTrace(__func__, 2, "Calling new psMinimize().\n");
    psS32 testStatus = true;


    testStatus &= t01();
    testStatus &= tst_psMinimizationAlloc();
    testStatus &= tst_psMinConstrainAlloc();
    if (testStatus == true) {
        printf("The LMM minimization tests PASSED.\n");
    } else {
        printf("The LMM minimization tests FAILED.\n");
    }
    printf("DONE\n");
    fflush(stdout);
}


