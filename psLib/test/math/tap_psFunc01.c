/*****************************************************************************
    This routine must ensure that the psGaussian() shall evaluate a
    specified Gaussian at some X.
 
    It also tests the p_psGaussianDev() procedure.
 *****************************************************************************/
#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include "tap.h"
#include "pstap.h"

#define MY_MEAN 5.0
#define MY_STDEV 2.0
#define TOLERANCE 0.01
psF32 truth1[10] = {0.008764, 0.026995, 0.064759, 0.120985, 0.176033, 0.199471, 0.176033, 0.120985, 0.064759, 0.026995};
psF32 truth2[10] = {0.043937, 0.135335, 0.324652, 0.606531, 0.882497, 1.000000, 0.882497, 0.606531, 0.324652, 0.135335};

int main()
{
    psLogSetFormat("HLNM");
    plan_tests(4);


    // Test the psGaussian(): normalized version
    {
        psMemId id = psMemGetId();
        bool errorFlag = false;
        for (psS32 x = 0 ; x < (int) (MY_MEAN * 2.0) ; x++)
        {
            psF32 actual = psGaussian((psF32) x, MY_MEAN, MY_STDEV, true);
            if (fabs(truth1[x] - actual) > TOLERANCE) {
                diag("ERROR: the Gaussian at %.2f was %f, should be %f", (psF32) x, actual, truth1[x]);
                errorFlag = true;
            }
        }
        ok(!errorFlag, "psGaussian() produced consistent results (normalized)");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Test the psGaussian(): non-normalized version
    {
        psMemId id = psMemGetId();
        bool errorFlag = false;
        for (psS32 x = 0 ; x < (int) (MY_MEAN * 2.0) ; x++)
        {
            psF32 actual = psGaussian((psF32) x, MY_MEAN, MY_STDEV, false);
            if (fabs(truth2[x] - actual) > TOLERANCE) {
                diag("ERROR: the Gaussian at %.2f was %f, should be %f", (psF32) x, actual, truth1[x]);
                errorFlag = true;
            }
        }
        ok(!errorFlag, "psGaussian() produced consistent results (non-normalized)");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
}
