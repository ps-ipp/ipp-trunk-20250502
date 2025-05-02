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

#define NUM_ITER 10
#define MIN_TOL  0.0001
#define MAX_TOL  1.0

#define NUM_ITERATIONS 100
#define NUM_X_POINTS 20
#define NUM_Y_POINTS 20
#define NUM_PARAMS 3
#define VERBOSE 0
float expectedParm[NUM_PARAMS];

// test with a simple 2D Gaussian:
// y = p2 + p0 * e^( -0.5 * (x0^2 + x1^2) / p1^2)
psF32 fitFunc(psVector *deriv,
              psVector *params,
              psVector *x)
{
    if (params == NULL) {
        psError(PS_ERR_UNKNOWN, true, "params is NULL.\n");
    }
    if (x == NULL) {
        psError(PS_ERR_UNKNOWN, true, "x is NULL.\n");
    }

    psF32 X = -0.5*(PS_SQR(x->data.F32[0]) + PS_SQR(x->data.F32[1]));
    psF32 Z = X / PS_SQR(params->data.F32[1]);
    psF32 dZdP1 = -2.0 * Z / params->data.F32[1];

    psF32 Q = exp(Z);
    psF32 R = params->data.F32[0] * Q;
    psF32 F = params->data.F32[2] + R;

    if (deriv) {
	deriv->data.F32[0] = Q;
	deriv->data.F32[1] = R * dZdP1; // dRdPq = R * dZdP1 since R = exp(Z)
	deriv->data.F32[2] = 1.0;
    }

    return F;
}

psS32 main(psS32 argc, char* argv[])
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    plan_tests(35);


    // Test psMinimizationAlloc()
    {
        psMemId id = psMemGetId();
        psMinimization *tmp = psMinimizationAlloc(NUM_ITER, MIN_TOL, MAX_TOL);
        ok(tmp != NULL, "psMinimizationAlloc() returned non-NULL");
        skip_start(tmp == NULL, 5, "Skipping tests because psMinimizationAlloc() failed");
        ok(tmp->maxIter == NUM_ITER, "psMinimizationAlloc() properly set ->maxIter");
        ok_float(tmp->minTol, MIN_TOL, "psMinimizationAlloc() properly set ->minTol");
        ok_float(tmp->maxTol, MAX_TOL, "psMinimizationAlloc() properly set ->maxTol");
        ok_float(tmp->value, 0.0, "psMinimizationAlloc() properly set ->value");
        ok(tmp->iter == 0, "psMinimizationAlloc() properly set ->iter (%d)", tmp->iter);
        ok(isnan(tmp->lastDelta), "psMinimizationAlloc() properly set ->lastDelta (%f)", tmp->lastDelta);
        ok(isnan(tmp->maxChisqDOF), "psMinimizationAlloc() properly set ->maxChisqDOF (%f)", tmp->maxChisqDOF);
        skip_end();
        psFree(tmp);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Test psMinConstraintAlloc()
    {
        psMemId id = psMemGetId();
        psMinConstraint *tmp = psMinConstraintAlloc();
        ok(tmp, "psMinConstraintAlloc() returned non-NULL");
        skip_start(!tmp, 2, "Skipping tests because psMinConstraintAlloc() failed");
        ok(!tmp->paramMask, "psMinConstraintAlloc() properly set ->paramMask");
        ok(!tmp->checkLimits, "psMinConstraintAlloc() properly set ->checkLimits");
        psFree(tmp);
        skip_end();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Test psMinimizeLMChi2(): unallowed input parameters.
    {
	int NUM_DATA_POINTS = NUM_X_POINTS * NUM_Y_POINTS;
        psMemId id = psMemGetId();
        psRandom *rng = psRandomAllocSpecific(PS_RANDOM_TAUS, 1); // Random number generator; using known seed
        psMinimization *min = psMinimizationAlloc(NUM_ITERATIONS, MIN_TOL, MAX_TOL);
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
    {
	int NUM_DATA_POINTS = NUM_X_POINTS * NUM_Y_POINTS;
        psMemId id = psMemGetId();
        psRandom *rng = psRandomAllocSpecific(PS_RANDOM_TAUS, 1); // Random number generator; using known seed
        psMinimization *min = psMinimizationAlloc(NUM_ITERATIONS, MIN_TOL, MAX_TOL);
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

	// create a uniform grid centered at 0,0:
	long i = 0;
        for (long ix = 0; ix < NUM_X_POINTS; ix++) {
	    for (long iy = 0; iy < NUM_Y_POINTS; iy++) {
		psVector *x = psVectorAlloc(2, PS_TYPE_F32);
		x->data.F32[0] = ix - 0.5*NUM_X_POINTS;
		x->data.F32[1] = iy - 0.5*NUM_Y_POINTS;
		ordinates->data[i] = x;

		// generate the data value
		float f = fitFunc(NULL, trueParams, x);
		float df = sqrt(f);

		// Add some noise
		coordinates->data.F32[i] = f + df*psRandomGaussian(rng);

		errors->data.F32[i] = 1.0 / PS_SQR(df);
		
		printf("%f %f  %f %f\n", x->data.F32[0], x->data.F32[1], coordinates->data.F32[i], errors->data.F32[i]);

		if (VERBOSE) {
		    printf("Data %ld: (%f, %f) --> %f\n",
			   i, x->data.F32[0], x->data.F32[1], coordinates->data.F32[i]);
		}
		
		i++;
	    }
	}

# if (1)	
	psTraceSetLevel ("psLib.math.psMinimizeLMChi2_Alt", 5);
        tmpBool = psMinimizeLMChi2_Alt(min, NULL, params, NULL, ordinates, coordinates, errors, (psMinimizeLMChi2Func)fitFunc);
# else
	psTraceSetLevel ("psLib.math.psMinimizeLMChi2", 5);
        tmpBool = psMinimizeLMChi2(min, NULL, params, NULL, ordinates, coordinates, errors, (psMinimizeLMChi2Func)fitFunc);
# endif
        ok(tmpBool, "psMinimizeLMChi2() suceeded");
        skip_start(!tmpBool, 4, "Skipping tests because psMinimizeLMChi2() failed");

        printf("Minimisation took %d iterations\n", min->iter);
        printf("chi^2 at the minimum is %.3g\n", min->value);
        printf("reduced chi^2 at the minimum is %.3g\n", min->value / (float) (NUM_DATA_POINTS - 3));
        for (long i = 0; i < NUM_PARAMS; i++)
        {
            printf("Parameter %ld at the minimum is %.3f, expected: %f\n", i,
                   params->data.F32[i], trueParams->data.F32[i]);
        }
        float diff = 0.0;
        for (long i = 0; i < NUM_DATA_POINTS; i++)
        {
            psVector *x = ordinates->data[i];
            float fitted = fitFunc(NULL, trueParams, x);
            float expected = fitFunc(NULL, params, x);
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
