/*****************************************************************************
    This routine must ensure that psMinimizeLM() works correctly.
 
    XXX: This code needs a lot of additional test case work.  The minimization
	 currently fails and we don't attempt to check the output values.
    XXX: Add tests for
	covar arg set to non-NULL
	constraint set to non-NULL
        Set x->vectors to NULL, or use wrong types
        yWt (errors) vector set to incorrect size, type.
 *****************************************************************************/
#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include "tap.h"
#include "pstap.h"

#define NUM_ITERATIONS 100
#define ERR_TOL 1e-6
#define NUM_DATA_POINTS 300
#define NUM_PARAMS 3
#define VERBOSE 0
float expectedParm[NUM_PARAMS];

// y = p2 + p0 * e^( x0^2 + x1^2 / (2 * p1^2) )
float function(const psVector *params, const psVector *x)
{
    return params->data.F32[0] *
           expf(- (PS_SQR(x->data.F32[0]) + PS_SQR(x->data.F32[1]) / 2.0 / PS_SQR(params->data.F32[1]))) +
           params->data.F32[2];
}

void derivatives(psVector *deriv, const psVector *params, const psVector *x)
{
    deriv->data.F32[0] = expf(- (PS_SQR(x->data.F32[0]) + PS_SQR(x->data.F32[1]) / 2.0 / PS_SQR(params->data.F32[1])));
    deriv->data.F32[1] = params->data.F32[0] * (PS_SQR(x->data.F32[0]) + PS_SQR(x->data.F32[1]) / 2.0 / PS_SQR(params->data.F32[1])) / params->data.F32[1] / params->data.F32[1] / params->data.F32[1] * expf(- (PS_SQR(x->data.F32[0]) + PS_SQR(x->data.F32[1]) / 2.0 / PS_SQR(params->data.F32[1])));
    deriv->data.F32[2] = 1.0;
}


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


#define NUM_ITER 10
#define TOL  20.0
psS32 main(psS32 argc, char* argv[])
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    plan_tests(34);


    // Test psMinimizationAlloc()
    {
        psMemId id = psMemGetId();
        psMinimization *tmp = psMinimizationAlloc(NUM_ITER, TOL);
        ok(tmp != NULL, "psMinimizationAlloc() returned non-NULL");
        skip_start(tmp == NULL, 5, "Skipping tests because psMinimizationAlloc() failed");
        ok(tmp->maxIter == NUM_ITER, "psMinimizationAlloc() properly set ->maxIter");
        ok(tmp->tol == TOL, "psMinimizationAlloc() properly set ->tol");
        ok(tmp->value == 0.0, "psMinimizationAlloc() properly set ->value");
        ok(tmp->iter == 0, "psMinimizationAlloc() properly set ->iter (%d)", tmp->iter);
        ok(isnan(tmp->lastDelta), "psMinimizationAlloc() properly set ->lastDelta (%f)", tmp->lastDelta);
        skip_end();
        psFree(tmp);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Test psMinConstraintAlloc()
    {
        psMemId id = psMemGetId();
        psMinConstraint *tmp = psMinConstraintAlloc();
        ok(tmp != NULL, "psMinConstraintAlloc() returned non-NULL");
        skip_start(tmp == NULL, 2, "Skipping tests because psMinConstraintAlloc() failed");
        ok(tmp->paramMask == NULL, "psMinConstraintAlloc() properly set ->paramMask");
        ok(tmp->checkLimits == NULL, "psMinConstraintAlloc() properly set ->checkLimits");
        psFree(tmp);
        skip_end();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Test psMinimizeLMChi2(): unallowed input parameters.
    {
        psMemId id = psMemGetId();
        psRandom *rng = psRandomAllocSpecific(PS_RANDOM_TAUS, 1); // Random number generator; using known seed
        psMinimization *min = psMinimizationAlloc(NUM_ITERATIONS, ERR_TOL);
        psArray *ordinates = psArrayAlloc(NUM_DATA_POINTS);
        psVector *coordinates = psVectorAlloc(NUM_DATA_POINTS, PS_TYPE_F32);
        psVector *coordinatesF64 = psVectorAlloc(NUM_DATA_POINTS, PS_TYPE_F64);
        psVector *errors = psVectorAlloc(NUM_DATA_POINTS, PS_TYPE_F32);
        psVector *params = psVectorAlloc(NUM_PARAMS, PS_TYPE_F32);
        psVector *paramsF64 = psVectorAlloc(NUM_PARAMS, PS_TYPE_F64);
        psVector *trueParams = psVectorAlloc(NUM_PARAMS, PS_TYPE_F32);

        // Test with psMinimization set to NULL
        {
            psMemId id = psMemGetId();
            bool tmpBool = psMinimizeLMChi2(NULL, NULL, params, NULL, ordinates,
                           coordinates, errors, (psMinimizeLMChi2Func)fitFunc);
            ok(!tmpBool, "psMinimizeLMChi2() returned FALSE with NULL psMinimization");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }


        // Test with params set to NULL
        {
            psMemId id = psMemGetId();
            bool tmpBool = psMinimizeLMChi2(min, NULL, NULL, NULL, ordinates,
                           coordinates, errors, (psMinimizeLMChi2Func)fitFunc);
            ok(!tmpBool, "psMinimizeLMChi2() returned FALSE with NULL params");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }


        // Test with params wrong type
        {
            psMemId id = psMemGetId();
            bool tmpBool = psMinimizeLMChi2(min, NULL, paramsF64, NULL, ordinates,
                           coordinates, errors, (psMinimizeLMChi2Func)fitFunc);
            ok(!tmpBool, "psMinimizeLMChi2() returned FALSE with NULL params");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }


        // Test with ordinates (x) set to NULL
        {
            psMemId id = psMemGetId();
            bool tmpBool = psMinimizeLMChi2(min, NULL, params, NULL, NULL,
                           coordinates, errors, (psMinimizeLMChi2Func)fitFunc);
            ok(!tmpBool, "psMinimizeLMChi2() returned FALSE with NULL ordinates (x)");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }


        // Test with coordinates (y) set to NULL
        {
            psMemId id = psMemGetId();
            bool tmpBool = psMinimizeLMChi2(min, NULL, params, NULL, ordinates,
                           NULL, errors, (psMinimizeLMChi2Func)fitFunc);
            ok(!tmpBool, "psMinimizeLMChi2() returned FALSE with NULL coordinates (y)");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }


        // Test with coordinates (y) wrong type (F64)
        {
            psMemId id = psMemGetId();
            bool tmpBool = psMinimizeLMChi2(min, NULL, params, NULL, ordinates,
                           coordinatesF64, errors, (psMinimizeLMChi2Func)fitFunc);
            ok(!tmpBool, "psMinimizeLMChi2() returned FALSE with coordinates (y) wrong type");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }


        // Test with ordinates and coordinates wrong size
        {
            psMemId id = psMemGetId();
            coordinates->n--;
            bool tmpBool = psMinimizeLMChi2(min, NULL, params, NULL, ordinates,
                           coordinates, errors, (psMinimizeLMChi2Func)fitFunc);
            ok(!tmpBool, "psMinimizeLMChi2() returned FALSE with NULL psMinimization");
            coordinates->n++;
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }


        // Test with function set to NULL
        {
            psMemId id = psMemGetId();
            bool tmpBool = psMinimizeLMChi2(min, NULL, params, NULL, ordinates,
                           coordinates, errors, NULL);
            ok(!tmpBool, "psMinimizeLMChi2() returned FALSE with NULL fit function");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }

        psFree(min);
        psFree(params);
        psFree(paramsF64);
        psFree(trueParams);
        psFree(ordinates);
        psFree(coordinates);
        psFree(coordinatesF64);
        psFree(errors);
        psFree(rng);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Test psMinimizeLMChi2() with legitimate input values
    // Currently this fails, and we do not attempt to verify output
    {
        psMemId id = psMemGetId();
        psRandom *rng = psRandomAllocSpecific(PS_RANDOM_TAUS, 1); // Random number generator; using known seed
        psMinimization *min = psMinimizationAlloc(NUM_ITERATIONS, ERR_TOL);
        psArray *ordinates = psArrayAlloc(NUM_DATA_POINTS);
        psVector *coordinates = psVectorAlloc(NUM_DATA_POINTS, PS_TYPE_F32);
        psVector *errors = psVectorAlloc(NUM_DATA_POINTS, PS_TYPE_F32);
        psVector *params = psVectorAlloc(NUM_PARAMS, PS_TYPE_F32);
        psVector *trueParams = psVectorAlloc(NUM_PARAMS, PS_TYPE_F32);
        bool tmpBool;
        trueParams->data.F32[0] = 100.0;    // Normalisation
        trueParams->data.F32[1] = 3.0;      // Width
        trueParams->data.F32[2] = 10.0;     // Background

        // Set parameters
        for (long i = 0; i < NUM_PARAMS; i++)
        {
            // Ensure we're not starting right on the true value:
            params->data.F32[i] = trueParams->data.F32[i] *
                                  (1.0 - psRandomGaussian(rng) / 10.0);
        }

        for (long i = 0; i < NUM_DATA_POINTS; i++)
        {
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

        tmpBool = psMinimizeLMChi2(min, NULL, params, NULL, ordinates, coordinates, errors,
                                   (psMinimizeLMChi2Func)fitFunc);
        ok(tmpBool, "psMinimizeLMChi2() suceeded");
        skip_start(!tmpBool, 4, "Skipping tests because psMinimizeLMChi2() failed");

        printf("Minimisation took %d iterations\n", min->iter);
        printf("chi^2 at the minimum is %.3g\n", min->value);
        for (long i = 0; i < NUM_PARAMS; i++)
        {
            printf("Parameter %ld at the minimum is %.3f, expected: %f\n", i,
                   params->data.F32[i], trueParams->data.F32[i]);
        }
        float diff = 0.0;
        for (long i = 0; i < NUM_DATA_POINTS; i++)
        {
            psVector *x = ordinates->data[i];
            float fitted = function(trueParams, x);
            float expected = function(params, x);
            diff += (fitted - expected) / fabsf(expected);
            if (VERBOSE) {
                printf("Data point %ld: Fitted: %f, expected: %f\n", i, fitted, expected);
            }
        }
        printf("Mean relative difference is %f\n", diff/(float)NUM_DATA_POINTS);
        skip_end();
        psFree(min);
        psFree(params);
        psFree(trueParams);
        psFree(ordinates);
        psFree(coordinates);
        psFree(errors);
        psFree(rng);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
}
