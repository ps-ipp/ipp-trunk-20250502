#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include "tap.h"
#include "pstap.h"

#define NUM_ITER 10
#define MIN_TOL  0.0001
#define MAX_TOL  1.0

#define NUM_ITERATIONS 100
#define NUM_PARAMS 5
#define VERBOSE 0
float expectedParm[NUM_PARAMS];

#define PM_PAR_I0     0 ///< Central intensity
#define PM_PAR_XPOS   1 ///< X center of object
#define PM_PAR_YPOS   2 ///< Y center of object
#define PM_PAR_LENGTH 3 ///< trail length
#define PM_PAR_THETA  4 ///< position angle

static float SIGMA = 0.0; // cross-width is fixed (not fitted)

psF32 fitFunc(psVector *deriv,
	      const psVector *params,
	      const psVector *pixcoord)
{
    psF32 *PAR = params->data.F32;

    psF32 X  = pixcoord->data.F32[0] - PAR[PM_PAR_XPOS];
    psF32 Y  = pixcoord->data.F32[1] - PAR[PM_PAR_YPOS];
    psF32 ST = sin(PAR[PM_PAR_THETA]);
    psF32 CT = cos(PAR[PM_PAR_THETA]);

    psF32 S2 = 2.0 * PS_SQR(SIGMA);

    psF32 Zp = (X*CT + Y*ST + 0.5*PAR[PM_PAR_LENGTH]) / sqrt(S2);
    psF32 Zm = (X*CT + Y*ST - 0.5*PAR[PM_PAR_LENGTH]) / sqrt(S2);

    psF32 Ep = erf(Zp);
    psF32 Em = erf(Zm);

    psF32 Rxy = Y*CT - X*ST;
    psF32 Gxy = exp(-Rxy*Rxy/S2);

    psF32 Pxy = Gxy * (Ep - Em);
    psF32 f = Pxy * PAR[PM_PAR_I0];

    if (deriv != NULL) {
        psF32 *dPAR = deriv->data.F32;

        dPAR[PM_PAR_I0]     = Pxy;

	float dGdR = -2.0 * Rxy * Gxy / S2; // -R Gxy / (2 Sigma^2)

	// are these signs correct? I think so: (dR/dXo = -dR/dX); dRdX below is actually dR/dXo
	float dRdX = +ST;
	float dRdY = -CT;
	float dRdT = -Y*ST - X*CT;

	float dGdX = dGdR * dRdX;
	float dGdY = dGdR * dRdY;
	float dGdT = dGdR * dRdT;

	// are these signs correct? I think so: (dR/dXo = -dR/dX); dRdX below is actually dR/dXo
	float dZpdX = -CT / sqrt(S2);
	float dZmdX = dZpdX; // float dZmdX = -CT / sqrt(S2); dZmdX = dZpdX

	float dZpdY = -ST / sqrt(S2); // float dZmdY = -ST / sqrt(S2); dZmdY = dZpdY
	float dZmdY = dZpdY;

	float dZpdL = +0.5 / sqrt(S2);
	float dZmdL = -0.5 / sqrt(S2);

	float dZpdT = (-X*ST + Y*CT) / sqrt(S2);
	float dZmdT = dZpdT; // dZpdT = dZmdT

	float dEdZp = exp (-Zp*Zp) * M_2_SQRTPI;
	float dEdZm = exp (-Zm*Zm) * M_2_SQRTPI;

	float dEpdX = dEdZp * dZpdX;
	float dEmdX = dEdZm * dZmdX;

	float dEpdY = dEdZp * dZpdY;
	float dEmdY = dEdZm * dZmdY;

	float dEpdL = dEdZp * dZpdL;
	float dEmdL = dEdZm * dZmdL;

	float dEpdT = dEdZp * dZpdT;
	float dEmdT = dEdZm * dZmdT;

	float dPdX = dGdX * (Ep - Em) + Gxy * (dEpdX - dEmdX);
	float dPdY = dGdY * (Ep - Em) + Gxy * (dEpdY - dEmdY);

	// dGdL is 0.0 because dRdL is 0.0
	float dPdL = Gxy * (dEpdL - dEmdL);

	float dPdT = dGdT * (Ep - Em) + Gxy * (dEpdT - dEmdT);

        dPAR[PM_PAR_XPOS]   = PAR[PM_PAR_I0] * dPdX;
        dPAR[PM_PAR_YPOS]   = PAR[PM_PAR_I0] * dPdY;

        dPAR[PM_PAR_LENGTH] = PAR[PM_PAR_I0] * dPdL;
        dPAR[PM_PAR_THETA]  = PAR[PM_PAR_I0] * dPdT;
    }
    return(f);
}

psS32 main(psS32 argc, char* argv[])
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    plan_tests(35);

    // Test psMinimizeLMChi2() with short-ish trail
    if (0) {
# define NUM_X_POINTS 20
# define NUM_Y_POINTS 20
	int NUM_DATA_POINTS = NUM_X_POINTS * NUM_Y_POINTS;
        psMemId id = psMemGetId();
        psRandom *rng = psRandomAllocSpecific(PS_RANDOM_TAUS, 1); // Random number generator; using known seed
        psMinimization *min = psMinimizationAlloc(NUM_ITERATIONS, MIN_TOL, MAX_TOL);
        psArray *ordinates = psArrayAlloc(NUM_DATA_POINTS);
        psVector *coordinates = psVectorAlloc(NUM_DATA_POINTS, PS_TYPE_F32);
        psVector *errors = psVectorAlloc(NUM_DATA_POINTS, PS_TYPE_F32);
        psVector *params = psVectorAlloc(NUM_PARAMS, PS_TYPE_F32);
        psVector *trueParams = psVectorAlloc(NUM_PARAMS, PS_TYPE_F32);

        trueParams->data.F32[PM_PAR_I0] = 100.0; 
        trueParams->data.F32[PM_PAR_XPOS] = 0.0; 
        trueParams->data.F32[PM_PAR_YPOS] = 0.0; 
        trueParams->data.F32[PM_PAR_LENGTH] = 10.0; 
        trueParams->data.F32[PM_PAR_THETA]  = 0.0; 

        SIGMA = 1.5; 

        // Set parameters (Ensure we're not starting right on the true value)
	
	// scatter flux & length by 10%)
	params->data.F32[PM_PAR_I0] = trueParams->data.F32[PM_PAR_I0] * (1.0 - psRandomGaussian(rng) / 10.0);
	params->data.F32[PM_PAR_LENGTH] = trueParams->data.F32[PM_PAR_LENGTH] * (1.0 - psRandomGaussian(rng) / 10.0);

	// scatter position by +/- 1 pix
	params->data.F32[PM_PAR_XPOS] = trueParams->data.F32[PM_PAR_XPOS] + psRandomGaussian(rng);
	params->data.F32[PM_PAR_YPOS] = trueParams->data.F32[PM_PAR_YPOS] + psRandomGaussian(rng);

	// scatter angle by +/- 10 degrees
	params->data.F32[PM_PAR_THETA] = trueParams->data.F32[PM_PAR_THETA] + psRandomGaussian(rng) * (10.0 * PS_RAD_DEG);

	// create a uniform grid centered at 0,0:
	long i = 0;
        for (long ix = 0; ix < NUM_X_POINTS; ix++) {
	    for (long iy = 0; iy < NUM_Y_POINTS; iy++) {
		psVector *x = psVectorAlloc(2, PS_TYPE_F32);
		x->data.F32[0] = ix - 0.5*NUM_X_POINTS;
		x->data.F32[1] = iy - 0.5*NUM_Y_POINTS;
		ordinates->data[i] = x;

		// generate the data value (high sky @ 100)
		float f = fitFunc(NULL, trueParams, x);
		float df = sqrt(f) + 5;

		// Add some noise
		coordinates->data.F32[i] = f + df*psRandomGaussian(rng);

		errors->data.F32[i] = 1.0 / PS_SQR(df);
		
		// printf("%f %f  %f %f\n", x->data.F32[0], x->data.F32[1], coordinates->data.F32[i], errors->data.F32[i]);

		if (VERBOSE) {
		    printf("Data %ld: (%f, %f) --> %f\n",
			   i, x->data.F32[0], x->data.F32[1], coordinates->data.F32[i]);
		}
		
		i++;
	    }
	}

# if (1)	
	psTraceSetLevel ("psLib.math.psMinimizeLMChi2_Alt", 5);
        bool tmpBool = psMinimizeLMChi2_Alt(min, NULL, params, NULL, ordinates, coordinates, errors, (psMinimizeLMChi2Func)fitFunc);
# else
	psTraceSetLevel ("psLib.math.psMinimizeLMChi2", 5);
        bool tmpBool = psMinimizeLMChi2(min, NULL, params, NULL, ordinates, coordinates, errors, (psMinimizeLMChi2Func)fitFunc);
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
	# undef NUM_X_POINTS
	# undef NUM_Y_POINTS
    }

    // Test psMinimizeLMChi2() with long trail & marginal guess
    if (0) {
# define NUM_X_POINTS 100
# define NUM_Y_POINTS 10
	int NUM_DATA_POINTS = NUM_X_POINTS * NUM_Y_POINTS;
        psMemId id = psMemGetId();
        // psRandom *rng = psRandomAllocSpecific(PS_RANDOM_TAUS, 1); // Random number generator; using known seed
        psRandom *rng = psRandomAlloc(PS_RANDOM_TAUS);
        psMinimization *min = psMinimizationAlloc(NUM_ITERATIONS, MIN_TOL, MAX_TOL);
        psArray *ordinates = psArrayAlloc(NUM_DATA_POINTS);
        psVector *coordinates = psVectorAlloc(NUM_DATA_POINTS, PS_TYPE_F32);
        psVector *errors = psVectorAlloc(NUM_DATA_POINTS, PS_TYPE_F32);
        psVector *params = psVectorAlloc(NUM_PARAMS, PS_TYPE_F32);
        psVector *trueParams = psVectorAlloc(NUM_PARAMS, PS_TYPE_F32);

        trueParams->data.F32[PM_PAR_I0] = 100.0; 
        trueParams->data.F32[PM_PAR_XPOS] = 0.0; 
        trueParams->data.F32[PM_PAR_YPOS] = 0.0; 
        trueParams->data.F32[PM_PAR_LENGTH] = 75.0; 
        trueParams->data.F32[PM_PAR_THETA]  = 0.0; 

        SIGMA = 1.5; 

        // Set parameters (Ensure we're not starting right on the true value)
	
	// scatter flux & length by 10%)
	params->data.F32[PM_PAR_I0] = trueParams->data.F32[PM_PAR_I0] * (1.0 - psRandomGaussian(rng) / 10.0);
	params->data.F32[PM_PAR_LENGTH] = trueParams->data.F32[PM_PAR_LENGTH] * (1.0 - psRandomGaussian(rng) / 3.0);

	// scatter position by +/- 1 pix
	params->data.F32[PM_PAR_XPOS] = trueParams->data.F32[PM_PAR_XPOS] + psRandomGaussian(rng);
	params->data.F32[PM_PAR_YPOS] = trueParams->data.F32[PM_PAR_YPOS] + psRandomGaussian(rng);

	// scatter angle by +/- 10 degrees
	params->data.F32[PM_PAR_THETA] = trueParams->data.F32[PM_PAR_THETA] + psRandomGaussian(rng) * (10.0 * PS_RAD_DEG);

	// create a uniform grid centered at 0,0:
	long i = 0;
        for (long ix = 0; ix < NUM_X_POINTS; ix++) {
	    for (long iy = 0; iy < NUM_Y_POINTS; iy++) {
		psVector *x = psVectorAlloc(2, PS_TYPE_F32);
		x->data.F32[0] = ix - 0.5*NUM_X_POINTS;
		x->data.F32[1] = iy - 0.5*NUM_Y_POINTS;
		ordinates->data[i] = x;

		// generate the data value (high sky @ 100)
		float f = fitFunc(NULL, trueParams, x);
		float df = sqrt(f) + 5;

		// Add some noise
		coordinates->data.F32[i] = f + df*psRandomGaussian(rng);

		errors->data.F32[i] = 1.0 / PS_SQR(df);
		
		// printf("%f %f  %f %f\n", x->data.F32[0], x->data.F32[1], coordinates->data.F32[i], errors->data.F32[i]);

		if (VERBOSE) {
		    printf("Data %ld: (%f, %f) --> %f\n",
			   i, x->data.F32[0], x->data.F32[1], coordinates->data.F32[i]);
		}
		
		i++;
	    }
	}

# if (1)	
	psTraceSetLevel ("psLib.math.psMinimizeLMChi2_Alt", 5);
        bool tmpBool = psMinimizeLMChi2_Alt(min, NULL, params, NULL, ordinates, coordinates, errors, (psMinimizeLMChi2Func)fitFunc);
# else
	psTraceSetLevel ("psLib.math.psMinimizeLMChi2", 5);
        bool tmpBool = psMinimizeLMChi2(min, NULL, params, NULL, ordinates, coordinates, errors, (psMinimizeLMChi2Func)fitFunc);
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


    // Test psMinimizeLMChi2() with long trail & bad length guess
    if (0) {
# define NUM_X_POINTS 100
# define NUM_Y_POINTS 10
	int NUM_DATA_POINTS = NUM_X_POINTS * NUM_Y_POINTS;
        psMemId id = psMemGetId();
        // psRandom *rng = psRandomAllocSpecific(PS_RANDOM_TAUS, 1); // Random number generator; using known seed
        psRandom *rng = psRandomAlloc(PS_RANDOM_TAUS);
        psMinimization *min = psMinimizationAlloc(NUM_ITERATIONS, MIN_TOL, MAX_TOL);
        psArray *ordinates = psArrayAlloc(NUM_DATA_POINTS);
        psVector *coordinates = psVectorAlloc(NUM_DATA_POINTS, PS_TYPE_F32);
        psVector *errors = psVectorAlloc(NUM_DATA_POINTS, PS_TYPE_F32);
        psVector *params = psVectorAlloc(NUM_PARAMS, PS_TYPE_F32);
        psVector *trueParams = psVectorAlloc(NUM_PARAMS, PS_TYPE_F32);

        trueParams->data.F32[PM_PAR_I0] = 100.0; 
        trueParams->data.F32[PM_PAR_XPOS] = 0.0; 
        trueParams->data.F32[PM_PAR_YPOS] = 0.0; 
        trueParams->data.F32[PM_PAR_LENGTH] = 75.0; 
        trueParams->data.F32[PM_PAR_THETA]  = 0.0; 

        SIGMA = 1.5; 

        // Set parameters (Ensure we're not starting right on the true value)
	
	// scatter flux & length by 10%)
	params->data.F32[PM_PAR_I0] = trueParams->data.F32[PM_PAR_I0] * (1.0 - psRandomGaussian(rng) / 10.0);
	params->data.F32[PM_PAR_LENGTH] = 10.0; // way too short

	// scatter position by +/- 1 pix
	params->data.F32[PM_PAR_XPOS] = trueParams->data.F32[PM_PAR_XPOS] + psRandomGaussian(rng);
	params->data.F32[PM_PAR_YPOS] = trueParams->data.F32[PM_PAR_YPOS] + psRandomGaussian(rng);

	// scatter angle by +/- 10 degrees
	params->data.F32[PM_PAR_THETA] = trueParams->data.F32[PM_PAR_THETA] + 0.01*psRandomGaussian(rng);

	// create a uniform grid centered at 0,0:
	long i = 0;
        for (long ix = 0; ix < NUM_X_POINTS; ix++) {
	    for (long iy = 0; iy < NUM_Y_POINTS; iy++) {
		psVector *x = psVectorAlloc(2, PS_TYPE_F32);
		x->data.F32[0] = ix - 0.5*NUM_X_POINTS;
		x->data.F32[1] = iy - 0.5*NUM_Y_POINTS;
		ordinates->data[i] = x;

		// generate the data value (high sky @ 100)
		float f = fitFunc(NULL, trueParams, x);
		float df = sqrt(f) + 5;

		// Add some noise
		coordinates->data.F32[i] = f + df*psRandomGaussian(rng);

		errors->data.F32[i] = 1.0 / PS_SQR(df);
		
		// printf("%f %f  %f %f\n", x->data.F32[0], x->data.F32[1], coordinates->data.F32[i], errors->data.F32[i]);

		if (VERBOSE) {
		    printf("Data %ld: (%f, %f) --> %f\n",
			   i, x->data.F32[0], x->data.F32[1], coordinates->data.F32[i]);
		}
		
		i++;
	    }
	}

# if (1)	
	psTraceSetLevel ("psLib.math.psMinimizeLMChi2_Alt", 5);
        bool tmpBool = psMinimizeLMChi2_Alt(min, NULL, params, NULL, ordinates, coordinates, errors, (psMinimizeLMChi2Func)fitFunc);
# else
	psTraceSetLevel ("psLib.math.psMinimizeLMChi2", 5);
        bool tmpBool = psMinimizeLMChi2(min, NULL, params, NULL, ordinates, coordinates, errors, (psMinimizeLMChi2Func)fitFunc);
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

    // Test psMinimizeLMChi2() with long trail & bad length guess
    if (1) {
# define NUM_X_POINTS 100
# define NUM_Y_POINTS 10
	int NUM_DATA_POINTS = NUM_X_POINTS * NUM_Y_POINTS;
        psMemId id = psMemGetId();
        // psRandom *rng = psRandomAllocSpecific(PS_RANDOM_TAUS, 1); // Random number generator; using known seed
        psRandom *rng = psRandomAlloc(PS_RANDOM_TAUS);
        psMinimization *min = psMinimizationAlloc(NUM_ITERATIONS, MIN_TOL, MAX_TOL);
        psArray *ordinates = psArrayAlloc(NUM_DATA_POINTS);
        psVector *coordinates = psVectorAlloc(NUM_DATA_POINTS, PS_TYPE_F32);
        psVector *errors = psVectorAlloc(NUM_DATA_POINTS, PS_TYPE_F32);
        psVector *params = psVectorAlloc(NUM_PARAMS, PS_TYPE_F32);
        psVector *trueParams = psVectorAlloc(NUM_PARAMS, PS_TYPE_F32);

        trueParams->data.F32[PM_PAR_I0] = 100.0; 
        trueParams->data.F32[PM_PAR_XPOS] = 0.0; 
        trueParams->data.F32[PM_PAR_YPOS] = 0.0; 
        trueParams->data.F32[PM_PAR_LENGTH] = 75.0; 
        trueParams->data.F32[PM_PAR_THETA]  = 0.0; 

        SIGMA = 1.5; 

        // Set parameters (Ensure we're not starting right on the true value)
	
	// scatter flux & length by 10%)
	params->data.F32[PM_PAR_I0] = trueParams->data.F32[PM_PAR_I0] * (1.0 - psRandomGaussian(rng) / 10.0);
	params->data.F32[PM_PAR_LENGTH] = 10.0; // way too short

	// scatter position by +/- 1 pix
	// params->data.F32[PM_PAR_XPOS] = trueParams->data.F32[PM_PAR_XPOS] + psRandomGaussian(rng);
	params->data.F32[PM_PAR_XPOS] = 15.0;
	params->data.F32[PM_PAR_YPOS] = trueParams->data.F32[PM_PAR_YPOS] + psRandomGaussian(rng);

	// scatter angle by +/- 10 degrees
	params->data.F32[PM_PAR_THETA] = trueParams->data.F32[PM_PAR_THETA] + 0.01*psRandomGaussian(rng);

	// create a uniform grid centered at 0,0:
	long i = 0;
        for (long ix = 0; ix < NUM_X_POINTS; ix++) {
	    for (long iy = 0; iy < NUM_Y_POINTS; iy++) {
		psVector *x = psVectorAlloc(2, PS_TYPE_F32);
		x->data.F32[0] = ix - 0.5*NUM_X_POINTS;
		x->data.F32[1] = iy - 0.5*NUM_Y_POINTS;
		ordinates->data[i] = x;

		// generate the data value (high sky @ 100)
		float f = fitFunc(NULL, trueParams, x);
		float df = sqrt(f) + 5;

		// Add some noise
		coordinates->data.F32[i] = f + df*psRandomGaussian(rng);

		errors->data.F32[i] = 1.0 / PS_SQR(df);
		
		// printf("%f %f  %f %f\n", x->data.F32[0], x->data.F32[1], coordinates->data.F32[i], errors->data.F32[i]);

		if (VERBOSE) {
		    printf("Data %ld: (%f, %f) --> %f\n",
			   i, x->data.F32[0], x->data.F32[1], coordinates->data.F32[i]);
		}
		
		i++;
	    }
	}

# if (1)	
	// psTraceSetLevel ("psLib.math.psMinimizeLMChi2_Alt", 5);
        bool tmpBool = psMinimizeLMChi2_Alt(min, NULL, params, NULL, ordinates, coordinates, errors, (psMinimizeLMChi2Func)fitFunc);
# else
	psTraceSetLevel ("psLib.math.psMinimizeLMChi2", 5);
        bool tmpBool = psMinimizeLMChi2(min, NULL, params, NULL, ordinates, coordinates, errors, (psMinimizeLMChi2Func)fitFunc);
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
