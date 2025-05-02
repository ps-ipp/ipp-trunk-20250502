/*****************************************************************************
    This routine must ensure that the psGaussian() shall evaluate a
    specified Gaussian at some X.
 
    It also tests the p_psGaussianDev() procedure.
 *****************************************************************************/
#include <stdio.h>
#include "pslib_strict.h"
#include "psTest.h"
#include "psMemory.h"
#include "psPolynomial.h"
#define MY_MEAN 30.0
#define MY_STDEV 2.0
#define N 30
psS32 main()
{
    psLogSetFormat("HLNM");
    psS32 testStatus = true;
    float x = 0.0;
    psS32  memLeaks;
    psS32  currentId = psMemGetId();
    psVector *myGaussData = NULL;
    printPositiveTestHeader(stdout,
                            "psPolynomial functions",
                            "psGaussian()");


    for (x = 0.0 ; x < (MY_MEAN * 2.0) ; x+= 1.0) {
        printf("normal psGaussian(%f) is %f\n", x, psGaussian(x, MY_MEAN, MY_STDEV, true));
        x = x + 1.0;
    }

    for (x = 0.0 ; x < (MY_MEAN * 2.0) ; x+= 1.0) {
        printf("NON-normal psGaussian(%f) is %f\n", x, psGaussian(x, MY_MEAN, MY_STDEV, false));
        x = x + 1.0;
    }

    psMemCheckCorruption(1);
    memLeaks = psMemCheckLeaks(currentId,NULL,stderr,false);
    if (0 != memLeaks) {
        psAbort("Memory Leaks! (%d leaks)", memLeaks);
    }

    printFooter(stdout,
                "psPolynomial functions",
                "psGaussian()",
                testStatus);


    printPositiveTestHeader(stdout,
                            "psPolynomial functions",
                            "p_psGaussianDev()");

    myGaussData = p_psGaussianDev(MY_MEAN, MY_STDEV, N);
    for (psS32 i = 0; i < N ; i++) {
        printf("Gaussian Deviate [%d] is %f\n", i, myGaussData->data.F32[i]);
    }

    if ( myGaussData->type.type != PS_TYPE_F32) {
        psAbort("p_psGaussianDev did not return a vector of type F32");
    }

    psFree(myGaussData);

    psMemCheckCorruption(1);
    memLeaks = psMemCheckLeaks(currentId,NULL,stderr,false);
    if (0 != memLeaks) {
        psAbort("Memory Leaks! (%d leaks)", memLeaks);
    }

    printFooter(stdout,
                "psPolynomial functions",
                "p_psGaussianDev()",
                testStatus);

    return (!testStatus);
}
