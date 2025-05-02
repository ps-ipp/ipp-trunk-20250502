/*****************************************************************************
This routine must ensure that various psLib functions which fit 3D polynomials
to data work correctly.  There is a function genericTest() which creates
vectors of data points (x and f), and populates them with the values from an
arbitrary function setData().  It then calls appropriate 3D fitting function.
It then evaluates the polynomial with the coefficients generated above and
determines if they are within an error tolerance of the expected values.
 *****************************************************************************/
#include <stdio.h>
#include <math.h>
#include "pslib.h"
#include "psTest.h"
#define NUM_DATA 100
#define POLY_ORDER_X 2
#define POLY_ORDER_Y 3
#define POLY_ORDER_Z 2
#define A 100.0
#define B 2.0
#define C 3.0
#define D 4.0
#define E 5.0
#define F 4.0
#define H 3.0
#define J 3.0
#define K 1.0
#define L 5.0
#define ERROR_TOLERANCE 0.10
#define YERR 10.0
#define VERBOSE 0
#define NUM_ITERATIONS 5
#define CLIP_SIGMA 4.0
#define OUTLIERS true
#define MASK_VALUE 1

#define TS00_F_NULL  0x00000001
#define TS00_F_F32  0x00000002
#define TS00_F_F64  0x00000004
#define TS00_F_S32  0x00000008
#define TS00_X_NULL  0x00000010
#define TS00_X_F32  0x00000020
#define TS00_X_F64  0x00000040
#define TS00_X_S32  0x00000080
#define TS00_FERR_NULL  0x00000100
#define TS00_FERR_F32  0x00000200
#define TS00_FERR_F64  0x00000400
#define TS00_FERR_S32  0x00000800
#define TS00_MASK_NULL  0x00001000
#define TS00_MASK_U8  0x00002000
#define TS00_MASK_S32  0x00004000
#define TS00_POLY_ORD  0x00008000
#define TS00_POLY_CHEB  0x00010000
#define TS00_CLIP_FIT  0x00020000
#define TS00_Y_NULL  0x00100000
#define TS00_Y_F32  0x00200000
#define TS00_Y_F64  0x00400000
#define TS00_Y_S32  0x00800000
#define TS00_Z_NULL  0x01000000
#define TS00_Z_F32  0x02000000
#define TS00_Z_F64  0x04000000
#define TS00_Z_S32  0x08000000

psF32 setData(psF32 x, psF32 y, psF32 z)
{
    if (0) {
        // Linear case, for testing.
        return(A + (B * x) + (D * y) + (H * z));
    } else {
        return(A + (B * x) + (C * x * x) + (D * y) + (E * y * y) + (F * x * y) + (H * z) +
               (J * z * z) + (K * x * z) + (L * y * z));
    }
}

psS32 genericTest(
    psU32 flags,
    psS32 polyOrderX,
    psS32 polyOrderY,
    psS32 polyOrderZ,
    psS32 numData,
    psBool expectedRC)
{
    psS32 currentId = psMemGetId();
    psS32 testStatus = true;
    psS32 memLeaks = 0;
    psPolynomial3D *myPoly = NULL;
    psVector *x = NULL;
    psVector *y = NULL;
    psVector *z = NULL;
    psVector *f = NULL;
    psVector *mask = NULL;
    psVector *fErr = NULL;
    psStats *stats = psStatsAlloc(PS_STAT_SAMPLE_MEAN);
    stats->clipSigma = CLIP_SIGMA;
    stats->clipIter = NUM_ITERATIONS;

    printPositiveTestHeader(stdout, "psMinimize functions", "3D Polynomial Fitting Functions");

    psVector *xTruth = psVectorAlloc(numData, PS_TYPE_F64);
    psVector *yTruth = psVectorAlloc(numData, PS_TYPE_F64);
    psVector *zTruth = psVectorAlloc(numData, PS_TYPE_F64);
    psVector *fTruth = psVectorAlloc(numData, PS_TYPE_F64);
    xTruth->n = numData;
    yTruth->n = numData;
    zTruth->n = numData;
    fTruth->n = numData;
    psRandom *rng = psRandomAllocSpecific(PS_RANDOM_TAUS, 1); // Using known seed
    for (int i = 0; i < numData; i++) {
        xTruth->data.F64[i] = 2.0*psRandomUniform(rng) - 1.0;
        yTruth->data.F64[i] = 2.0*psRandomUniform(rng) - 1.0;
        zTruth->data.F64[i] = 2.0*psRandomUniform(rng) - 1.0;
        fTruth->data.F64[i] = setData(xTruth->data.F64[i], yTruth->data.F64[i], zTruth->data.F64[i]);
    }
    psFree(rng);

    if (expectedRC == false) {
        printf("This test should generate an error message, and return NULL.\n");
    }

    if (flags & TS00_CLIP_FIT) {
        printf(" performing a clip-fit\n");
    } else {
        printf(" performing a non clip-fit\n");
    }

    if (flags & TS00_POLY_ORD) {
        printf(" using ordinary polynomials\n");
        myPoly = psPolynomial3DAlloc(PS_POLYNOMIAL_ORD, polyOrderX, polyOrderY, polyOrderZ);
    }

    if (flags & TS00_X_NULL) {
        printf(" using a NULL x vector\n");
    }

    if (flags & TS00_X_F32) {
        printf(" using a psF32 x vector\n");
        x = psVectorCopy(NULL, xTruth, PS_TYPE_F32);
    }

    if (flags & TS00_X_S32) {
        printf(" using a psS32 x vector\n");
        x = psVectorCopy(NULL, xTruth, PS_TYPE_S32);
    }

    if (flags & TS00_X_F64) {
        printf(" using a psF64 x vector\n");
        x = psVectorCopy(NULL, xTruth, PS_TYPE_F64);
    }


    if (flags & TS00_Y_NULL) {
        printf(" using a NULL y vector\n");
    }

    if (flags & TS00_Y_F32) {
        printf(" using a psF32 y vector\n");
        y = psVectorCopy(NULL, yTruth, PS_TYPE_F32);
    }

    if (flags & TS00_Y_S32) {
        printf(" using a psS32 y vector\n");
        y = psVectorCopy(NULL, yTruth, PS_TYPE_S32);
    }

    if (flags & TS00_Y_F64) {
        printf(" using a psF64 y vector\n");
        y = psVectorCopy(NULL, yTruth, PS_TYPE_F64);
    }

    if (flags & TS00_Z_NULL) {
        printf(" using a NULL z vector\n");
    }

    if (flags & TS00_Z_F32) {
        printf(" using a psF32 z vector\n");
        z = psVectorCopy(NULL, zTruth, PS_TYPE_F32);
    }

    if (flags & TS00_Z_S32) {
        printf(" using a psS32 z vector\n");
        z = psVectorCopy(NULL, zTruth, PS_TYPE_S32);
    }

    if (flags & TS00_Z_F64) {
        printf(" using a psF64 z vector\n");
        z = psVectorCopy(NULL, zTruth, PS_TYPE_F64);
    }


    if (flags & TS00_F_NULL) {
        printf(" using a NULL f vector\n");
    }

    if (flags & TS00_F_F32) {
        printf(" using a psF32 f vector\n");
        f = psVectorCopy(NULL, fTruth, PS_TYPE_F32);
        // Set a few outliers in the data.
        if (OUTLIERS && (flags & TS00_CLIP_FIT)) {
            f->data.F32[numData/4]*= 2.0;
            f->data.F32[numData/2]*= 2.0;
            f->data.F32[3*numData/4]*= 2.0;
        }

        if (VERBOSE) {
            for (psS32 i=0;i<numData;i++) {
                printf("Original data %d: (%.1f)\n", i, f->data.F32[i]);
            }
        }
    }

    if (flags & TS00_F_S32) {
        printf(" using a psS32 f vector\n");
        f = psVectorCopy(NULL, fTruth, PS_TYPE_S32);
        // Set a few outliers in the data.
        if (OUTLIERS && (flags & TS00_CLIP_FIT)) {
            f->data.S32[numData/4]*= 2.0;
            f->data.S32[numData/2]*= 2.0;
            f->data.S32[3*numData/4]*= 2.0;
        }

        if (VERBOSE) {
            for (psS32 i=0;i<numData;i++) {
                printf("Original data %d: (%d)\n", i, f->data.S32[i]);
            }
        }
    }

    if (flags & TS00_F_F64) {
        printf(" using a psF64 f vector\n");
        f = psVectorCopy(NULL, fTruth, PS_TYPE_F64);
        // Set a few outliers in the data.
        if (OUTLIERS && (flags & TS00_CLIP_FIT)) {
            f->data.F64[numData/4]*= 2.0;
            f->data.F64[numData/2]*= 2.0;
            f->data.F64[3*numData/4]*= 2.0;
        }

        if (VERBOSE) {
            for (psS32 i=0;i<numData;i++) {
                printf("Original data %d: (%.1f)\n", i, f->data.F64[i]);
            }
        }
    }

    if (flags & TS00_FERR_NULL) {
        printf(" using a NULL fErr vector\n");
    }

    if (flags & TS00_FERR_F32) {
        printf(" using a psF32 fErr vector\n");
        fErr = psVectorAlloc(numData, PS_TYPE_F32);
        fErr->n = numData;
        for (psS32 i=0;i<numData;i++) {
            fErr->data.F32[i] = YERR;
        }
    }

    if (flags & TS00_FERR_S32) {
        printf(" using a psS32 fErr vector\n");
        fErr = psVectorAlloc(numData, PS_TYPE_S32);
        fErr->n = numData;
        for (psS32 i=0;i<numData;i++) {
            fErr->data.S32[i] = (psS32) YERR;
        }
    }

    if (flags & TS00_FERR_F64) {
        printf(" using a psF64 fErr vector\n");
        fErr = psVectorAlloc(numData, PS_TYPE_F64);
        fErr->n = numData;
        for (psS32 i=0;i<numData;i++) {
            fErr->data.F64[i] = YERR;
        }
    }

    if (flags & TS00_MASK_NULL) {
        printf(" using a NULL mask vector\n");
    }

    if (flags & TS00_MASK_U8) {
        printf(" using a psU8 mask vector\n");
        mask = psVectorAlloc(numData, PS_TYPE_U8);
        mask->n = numData;
        for (psS32 i=0;i<numData;i++) {
            mask->data.U8[i] = 0;
        }
    }

    if (flags & TS00_MASK_S32) {
        printf(" using a psS32 mask vector\n");
        mask = psVectorAlloc(numData, PS_TYPE_S32);
        mask->n = numData;
        for (psS32 i=0;i<numData;i++) {
            mask->data.S32[i] = 0;
        }
    }

    psPolynomial3D *rc = NULL;
    if (flags & TS00_CLIP_FIT) {
        rc = psVectorClipFitPolynomial3D(myPoly, stats, mask, MASK_VALUE, f, fErr, x, y, z);
    } else {
        rc = psVectorFitPolynomial3D(myPoly, mask, MASK_VALUE, f, fErr, x, y, z);
    }

    if (rc == NULL) {
        if (expectedRC == true) {
            printf("TEST ERROR: the 3D polynomial fitting function returned NULL.\n");
            testStatus = false;
        }
    } else {
        if (expectedRC == false) {
            printf("TEST ERROR: the 3D polynomial fitting function returned non-NULL.\n");
            testStatus = false;
        }

        if (VERBOSE) {
            for (psS32 i=0;i<polyOrderX+1;i++) {
                for (psS32 j=0;j<polyOrderY+1;j++) {
                    for (psS32 k=0;k<polyOrderZ+1;k++) {
                        printf("Polynomial coefficient [%d][%d][%d] is %0.1f\n", i, j, k, myPoly->coeff[i][j][k]);
                    }
                }
            }
        }

        psVector *result = psPolynomial3DEvalVector(myPoly, xTruth, yTruth, zTruth);
        for (psS32 i=0 ;i<numData; i++) {
            // Skip the outliers.
            if ((i == numData/4) || (i == numData/2) || (i == 3*numData/4)) {
                continue;
            }
            psF32 expectData = fTruth->data.F64[i];
            psF32 actualData = result->data.F64[i];

            if (fabs(actualData-expectData) > fabs(ERROR_TOLERANCE * expectData)) {
                printf("TEST ERROR: Fitted data %d: (%.1f), expected was (%.1f)\n",
                       i, actualData, expectData);
                testStatus = false;
            } else {
                if (VERBOSE) {
                    printf("GOOD: Fitted data %d: (%.1f), expected was (%.1f)\n",
                           i, actualData, expectData);
                }
            }
        }
        psFree(result);
    }

    psMemCheckCorruption(1);
    psFree(myPoly);
    psFree(mask);
    psFree(x);
    psFree(y);
    psFree(z);
    psFree(f);
    psFree(xTruth);
    psFree(yTruth);
    psFree(zTruth);
    psFree(fTruth);
    psFree(fErr);
    psFree(stats);
    psMemCheckCorruption(1);
    memLeaks = psMemCheckLeaks(currentId,NULL,stderr,false);
    if (0 != memLeaks) {
        psAbort("Memory Leaks! (%d leaks)", memLeaks);
    }

    printFooter(stdout, "psMinimize functions", "3D Polynomial Fitting Functions", testStatus);

    return (testStatus);
}

/*****************************************************************************
We test a variety of polynomial fitting routines, types, and polynomial types
here:
    F32 tests: Ordinary polys, non-clip fit
    F64 tests: Ordinary polys, non-clip fit
    F32 tests: Chebyshev polys, non-clip fit
    F64 tests: Chebyshev polys, non-clip fit
    F32 tests: Ordinary polys, clip fit
    F64 tests: Ordinary polys, clip fit
    F32 tests: Chebyshev polys, clip fit
    F64 tests: Chebyshev polys, clip fit
 *****************************************************************************/
psS32 main()
{
    psBool testStatus = true;
    psLogSetFormat("HLNM");
    psTraceSetLevel(".", 0);
    psTraceSetLevel("psVectorClipFitPolynomial3D", 0);
    psTraceSetLevel("VectorFitPolynomial3DOrd", 0);
    psTraceSetLevel("psVectorFitPolynomial3D", 0);

    printPositiveTestHeader(stdout, "psMinimize functions: 3D Polynomial Fitting Functions", "");

    //
    // F32 tests: Ordinary polys, non-clip fit
    //
    // All Vectors non-NULL
    testStatus &= genericTest(TS00_MASK_U8 | TS00_FERR_F32 | TS00_X_F32 | TS00_Y_F32 | TS00_Z_F32 | TS00_F_F32 | TS00_POLY_ORD, POLY_ORDER_X, POLY_ORDER_Y, POLY_ORDER_Z, NUM_DATA, true);
    // Some Vectors NULL
    testStatus &= genericTest(TS00_MASK_U8 | TS00_FERR_NULL | TS00_X_F32 | TS00_Y_F32 | TS00_Z_F32 | TS00_F_F32 | TS00_POLY_ORD, POLY_ORDER_X, POLY_ORDER_Y, POLY_ORDER_Z, NUM_DATA, true);
    testStatus &= genericTest(TS00_MASK_U8 | TS00_FERR_F32 | TS00_X_NULL | TS00_Y_F32 | TS00_Z_F32 | TS00_F_F32 | TS00_POLY_ORD, POLY_ORDER_X, POLY_ORDER_Y, POLY_ORDER_Z, NUM_DATA, false);
    testStatus &= genericTest(TS00_MASK_U8 | TS00_FERR_F32 | TS00_X_F32 | TS00_Y_NULL | TS00_Z_F32 | TS00_F_F32 | TS00_POLY_ORD, POLY_ORDER_X, POLY_ORDER_Y, POLY_ORDER_Z, NUM_DATA, false);
    testStatus &= genericTest(TS00_MASK_NULL | TS00_FERR_F32 | TS00_X_F32 | TS00_Y_F32 | TS00_Z_F32 | TS00_F_F32 | TS00_POLY_ORD, POLY_ORDER_X, POLY_ORDER_Y, POLY_ORDER_Z, NUM_DATA, true);
    // F-vector NULL
    testStatus &= genericTest(TS00_MASK_U8 | TS00_FERR_F32 | TS00_X_F32 | TS00_Y_F32 | TS00_Z_F32 | TS00_F_NULL | TS00_POLY_ORD, POLY_ORDER_X, POLY_ORDER_Y, POLY_ORDER_Z, NUM_DATA, false);
    testStatus &= genericTest(TS00_MASK_NULL | TS00_FERR_F32 | TS00_X_F32 | TS00_Y_F32 | TS00_Z_F32 | TS00_F_F32 | TS00_POLY_ORD, POLY_ORDER_X, POLY_ORDER_Y, POLY_ORDER_Z, NUM_DATA, true);
    // Mismatch vector types
    testStatus &= genericTest(TS00_MASK_U8 | TS00_FERR_F64 | TS00_X_F32 | TS00_Y_F32 | TS00_Z_F32 | TS00_F_F32 | TS00_POLY_ORD, POLY_ORDER_X, POLY_ORDER_Y, POLY_ORDER_Z, NUM_DATA, true);
    testStatus &= genericTest(TS00_MASK_U8 | TS00_FERR_F32 | TS00_X_F64 | TS00_Y_F32 | TS00_Z_F32 | TS00_F_F32 | TS00_POLY_ORD, POLY_ORDER_X, POLY_ORDER_Y, POLY_ORDER_Z, NUM_DATA, true);
    testStatus &= genericTest(TS00_MASK_U8 | TS00_FERR_F32 | TS00_X_F32 | TS00_Y_F64 | TS00_Z_F32 | TS00_F_F32 | TS00_POLY_ORD, POLY_ORDER_X, POLY_ORDER_Y, POLY_ORDER_Z, NUM_DATA, true);
    testStatus &= genericTest(TS00_MASK_U8 | TS00_FERR_F32 | TS00_X_F32 | TS00_Y_F32 | TS00_Z_F64 | TS00_F_F32 | TS00_POLY_ORD, POLY_ORDER_X, POLY_ORDER_Y, POLY_ORDER_Z, NUM_DATA, true);
    testStatus &= genericTest(TS00_MASK_U8 | TS00_FERR_F32 | TS00_X_F32 | TS00_Y_F32 | TS00_Z_F32 | TS00_F_F64 | TS00_POLY_ORD, POLY_ORDER_X, POLY_ORDER_Y, POLY_ORDER_Z, NUM_DATA, true);
    testStatus &= genericTest(TS00_MASK_S32 | TS00_FERR_F32 | TS00_X_F32 | TS00_Y_F32 | TS00_Z_F32 | TS00_F_F32 | TS00_POLY_ORD, POLY_ORDER_X, POLY_ORDER_Y, POLY_ORDER_Z, NUM_DATA, false);

    //
    // F64 tests: Ordinary polys, non-clip fit
    //
    // All Vectors non-NULL
    testStatus &= genericTest(TS00_MASK_U8 | TS00_FERR_F64 | TS00_X_F64 | TS00_Y_F64 | TS00_Z_F64 | TS00_F_F64 | TS00_POLY_ORD, POLY_ORDER_X, POLY_ORDER_Y, POLY_ORDER_Z, NUM_DATA, true);
    // Some Vectors NULL
    testStatus &= genericTest(TS00_MASK_U8 | TS00_FERR_NULL | TS00_X_F64 | TS00_Y_F64 | TS00_Z_F64 | TS00_F_F64 | TS00_POLY_ORD, POLY_ORDER_X, POLY_ORDER_Y, POLY_ORDER_Z, NUM_DATA, true);
    testStatus &= genericTest(TS00_MASK_U8 | TS00_FERR_F64 | TS00_X_NULL | TS00_Y_F64 | TS00_Z_F64 | TS00_F_F64 | TS00_POLY_ORD, POLY_ORDER_X, POLY_ORDER_Y, POLY_ORDER_Z, NUM_DATA, false);
    testStatus &= genericTest(TS00_MASK_U8 | TS00_FERR_F64 | TS00_X_F64 | TS00_Y_NULL | TS00_Z_F64 | TS00_F_F64 | TS00_POLY_ORD, POLY_ORDER_X, POLY_ORDER_Y, POLY_ORDER_Z, NUM_DATA, false);
    testStatus &= genericTest(TS00_MASK_NULL | TS00_FERR_F64 | TS00_X_F64 | TS00_Y_F64 | TS00_Z_F64 | TS00_F_F64 | TS00_POLY_ORD, POLY_ORDER_X, POLY_ORDER_Y, POLY_ORDER_Z, NUM_DATA, true);
    // F-vector NULL
    testStatus &= genericTest(TS00_MASK_U8 | TS00_FERR_F64 | TS00_X_F64 | TS00_Y_F64 | TS00_Z_F64 | TS00_F_NULL | TS00_POLY_ORD, POLY_ORDER_X, POLY_ORDER_Y, POLY_ORDER_Z, NUM_DATA, false);
    testStatus &= genericTest(TS00_MASK_NULL | TS00_FERR_F64 | TS00_X_F64 | TS00_Y_F64 | TS00_Z_F64 | TS00_F_F32 | TS00_POLY_ORD, POLY_ORDER_X, POLY_ORDER_Y, POLY_ORDER_Z, NUM_DATA, true);
    // Mismatch vector types
    testStatus &= genericTest(TS00_MASK_U8 | TS00_FERR_F32 | TS00_X_F64 | TS00_Y_F64 | TS00_Z_F64 | TS00_F_F64 | TS00_POLY_ORD, POLY_ORDER_X, POLY_ORDER_Y, POLY_ORDER_Z, NUM_DATA, true);
    testStatus &= genericTest(TS00_MASK_U8 | TS00_FERR_F64 | TS00_X_F32 | TS00_Y_F64 | TS00_Z_F64 | TS00_F_F64 | TS00_POLY_ORD, POLY_ORDER_X, POLY_ORDER_Y, POLY_ORDER_Z, NUM_DATA, true);
    testStatus &= genericTest(TS00_MASK_U8 | TS00_FERR_F64 | TS00_X_F64 | TS00_Y_F32 | TS00_Z_F64 | TS00_F_F64 | TS00_POLY_ORD, POLY_ORDER_X, POLY_ORDER_Y, POLY_ORDER_Z, NUM_DATA, true);
    testStatus &= genericTest(TS00_MASK_U8 | TS00_FERR_F64 | TS00_X_F64 | TS00_Y_F64 | TS00_Z_F32 | TS00_F_F64 | TS00_POLY_ORD, POLY_ORDER_X, POLY_ORDER_Y, POLY_ORDER_Z, NUM_DATA, true);
    testStatus &= genericTest(TS00_MASK_U8 | TS00_FERR_F64 | TS00_X_F64 | TS00_Y_F64 | TS00_Z_F64 | TS00_F_F32 | TS00_POLY_ORD, POLY_ORDER_X, POLY_ORDER_Y, POLY_ORDER_Z, NUM_DATA, true);
    testStatus &= genericTest(TS00_MASK_S32 | TS00_FERR_F64 | TS00_X_F64 | TS00_Y_F64 | TS00_Z_F64 | TS00_F_F64 | TS00_POLY_ORD, POLY_ORDER_X, POLY_ORDER_Y, POLY_ORDER_Z, NUM_DATA, false);

    //
    // F32 tests: Ordinary polys, clip fit
    //
    // All Vectors non-NULL
    testStatus &= genericTest(TS00_MASK_U8 | TS00_FERR_F32 | TS00_X_F32 | TS00_Y_F32 | TS00_Z_F32 | TS00_F_F32 | TS00_POLY_ORD | TS00_CLIP_FIT, POLY_ORDER_X, POLY_ORDER_Y, POLY_ORDER_Z, NUM_DATA, true);
    // Some Vectors NULL
    testStatus &= genericTest(TS00_MASK_U8 | TS00_FERR_NULL | TS00_X_F32 | TS00_Y_F32 | TS00_Z_F32 | TS00_F_F32 | TS00_POLY_ORD | TS00_CLIP_FIT, POLY_ORDER_X, POLY_ORDER_Y, POLY_ORDER_Z, NUM_DATA, true);
    testStatus &= genericTest(TS00_MASK_U8 | TS00_FERR_F32 | TS00_X_NULL | TS00_Y_F32 | TS00_Z_F32 | TS00_F_F32 | TS00_POLY_ORD | TS00_CLIP_FIT, POLY_ORDER_X, POLY_ORDER_Y, POLY_ORDER_Z, NUM_DATA, false);
    testStatus &= genericTest(TS00_MASK_U8 | TS00_FERR_F32 | TS00_X_F32 | TS00_Y_NULL | TS00_Z_F32 | TS00_F_F32 | TS00_POLY_ORD | TS00_CLIP_FIT, POLY_ORDER_X, POLY_ORDER_Y, POLY_ORDER_Z, NUM_DATA, false);
    testStatus &= genericTest(TS00_MASK_U8 | TS00_FERR_F32 | TS00_X_F32 | TS00_Y_NULL | TS00_Z_NULL | TS00_F_F32 | TS00_POLY_ORD | TS00_CLIP_FIT, POLY_ORDER_X, POLY_ORDER_Y, POLY_ORDER_Z, NUM_DATA, false);
    // F-vector NULL
    testStatus &= genericTest(TS00_MASK_U8 | TS00_FERR_F32 | TS00_X_F32 | TS00_Y_F32 | TS00_Z_F32 | TS00_F_NULL | TS00_POLY_ORD | TS00_CLIP_FIT, POLY_ORDER_X, POLY_ORDER_Y, POLY_ORDER_Z, NUM_DATA, false);
    testStatus &= genericTest(TS00_MASK_NULL | TS00_FERR_F32 | TS00_X_F32 | TS00_Y_F32 | TS00_Z_F32 | TS00_F_F32 | TS00_POLY_ORD | TS00_CLIP_FIT, POLY_ORDER_X, POLY_ORDER_Y, POLY_ORDER_Z, NUM_DATA, false);
    // Mismatch vector types
    testStatus &= genericTest(TS00_MASK_U8 | TS00_FERR_F64 | TS00_X_F32 | TS00_Y_F32 | TS00_Z_F32 | TS00_F_F32 | TS00_POLY_ORD | TS00_CLIP_FIT, POLY_ORDER_X, POLY_ORDER_Y, POLY_ORDER_Z, NUM_DATA, false);
    testStatus &= genericTest(TS00_MASK_U8 | TS00_FERR_F32 | TS00_X_F64 | TS00_Y_F32 | TS00_Z_F32 | TS00_F_F32 | TS00_POLY_ORD | TS00_CLIP_FIT, POLY_ORDER_X, POLY_ORDER_Y, POLY_ORDER_Z, NUM_DATA, false);
    testStatus &= genericTest(TS00_MASK_U8 | TS00_FERR_F32 | TS00_X_F32 | TS00_Y_F64 | TS00_Z_F32 | TS00_F_F32 | TS00_POLY_ORD | TS00_CLIP_FIT, POLY_ORDER_X, POLY_ORDER_Y, POLY_ORDER_Z, NUM_DATA, false);
    testStatus &= genericTest(TS00_MASK_U8 | TS00_FERR_F32 | TS00_X_F32 | TS00_Y_F32 | TS00_Z_F64 | TS00_F_F32 | TS00_POLY_ORD | TS00_CLIP_FIT, POLY_ORDER_X, POLY_ORDER_Y, POLY_ORDER_Z, NUM_DATA, false);
    testStatus &= genericTest(TS00_MASK_U8 | TS00_FERR_F32 | TS00_X_F32 | TS00_Y_F32 | TS00_Z_F32 | TS00_F_F64 | TS00_POLY_ORD | TS00_CLIP_FIT, POLY_ORDER_X, POLY_ORDER_Y, POLY_ORDER_Z, NUM_DATA, false);
    testStatus &= genericTest(TS00_MASK_S32 | TS00_FERR_F32 | TS00_X_F32 | TS00_Y_F32 | TS00_Z_F32 | TS00_F_F32 | TS00_POLY_ORD | TS00_CLIP_FIT, POLY_ORDER_X, POLY_ORDER_Y, POLY_ORDER_Z, NUM_DATA, false);

    //
    // F64 tests: Ordinary polys, clip fit
    //
    // All Vectors non-NULL
    testStatus &= genericTest(TS00_MASK_U8 | TS00_FERR_F64 | TS00_X_F64 | TS00_Y_F64 | TS00_Z_F64 | TS00_F_F64 | TS00_POLY_ORD | TS00_CLIP_FIT, POLY_ORDER_X, POLY_ORDER_Y, POLY_ORDER_Z, NUM_DATA, true);
    // Some Vectors NULL
    testStatus &= genericTest(TS00_MASK_U8 | TS00_FERR_NULL | TS00_X_F64 | TS00_Y_F64 | TS00_Z_F64 | TS00_F_F64 | TS00_POLY_ORD | TS00_CLIP_FIT, POLY_ORDER_X, POLY_ORDER_Y, POLY_ORDER_Z, NUM_DATA, true);
    testStatus &= genericTest(TS00_MASK_U8 | TS00_FERR_F64 | TS00_X_NULL | TS00_Y_F64 | TS00_Z_F64 | TS00_F_F64 | TS00_POLY_ORD | TS00_CLIP_FIT, POLY_ORDER_X, POLY_ORDER_Y, POLY_ORDER_Z, NUM_DATA, false);
    testStatus &= genericTest(TS00_MASK_U8 | TS00_FERR_F64 | TS00_X_F64 | TS00_Y_NULL | TS00_Z_F64 | TS00_F_F64 | TS00_POLY_ORD | TS00_CLIP_FIT, POLY_ORDER_X, POLY_ORDER_Y, POLY_ORDER_Z, NUM_DATA, false);
    testStatus &= genericTest(TS00_MASK_U8 | TS00_FERR_F64 | TS00_X_F64 | TS00_Y_NULL | TS00_Z_NULL | TS00_F_F64 | TS00_POLY_ORD | TS00_CLIP_FIT, POLY_ORDER_X, POLY_ORDER_Y, POLY_ORDER_Z, NUM_DATA, false);
    // F-vector NULL
    testStatus &= genericTest(TS00_MASK_U8 | TS00_FERR_F64 | TS00_X_F64 | TS00_Y_F64 | TS00_Z_F64 | TS00_F_NULL | TS00_POLY_ORD | TS00_CLIP_FIT, POLY_ORDER_X, POLY_ORDER_Y, POLY_ORDER_Z, NUM_DATA, false);
    testStatus &= genericTest(TS00_MASK_NULL | TS00_FERR_F64 | TS00_X_F64 | TS00_Y_F64 | TS00_Z_F64 | TS00_F_F64 | TS00_POLY_ORD | TS00_CLIP_FIT, POLY_ORDER_X, POLY_ORDER_Y, POLY_ORDER_Z, NUM_DATA, false);
    // Mismatch vector types
    testStatus &= genericTest(TS00_MASK_U8 | TS00_FERR_F32 | TS00_X_F64 | TS00_Y_F64 | TS00_Z_F64 | TS00_F_F64 | TS00_POLY_ORD | TS00_CLIP_FIT, POLY_ORDER_X, POLY_ORDER_Y, POLY_ORDER_Z, NUM_DATA, false);
    testStatus &= genericTest(TS00_MASK_U8 | TS00_FERR_F64 | TS00_X_F32 | TS00_Y_F64 | TS00_Z_F64 | TS00_F_F64 | TS00_POLY_ORD | TS00_CLIP_FIT, POLY_ORDER_X, POLY_ORDER_Y, POLY_ORDER_Z, NUM_DATA, false);
    testStatus &= genericTest(TS00_MASK_U8 | TS00_FERR_F64 | TS00_X_F64 | TS00_Y_F32 | TS00_Z_F64 | TS00_F_F64 | TS00_POLY_ORD | TS00_CLIP_FIT, POLY_ORDER_X, POLY_ORDER_Y, POLY_ORDER_Z, NUM_DATA, false);
    testStatus &= genericTest(TS00_MASK_U8 | TS00_FERR_F64 | TS00_X_F64 | TS00_Y_F64 | TS00_Z_F32 | TS00_F_F64 | TS00_POLY_ORD | TS00_CLIP_FIT, POLY_ORDER_X, POLY_ORDER_Y, POLY_ORDER_Z, NUM_DATA, false);
    testStatus &= genericTest(TS00_MASK_U8 | TS00_FERR_F64 | TS00_X_F64 | TS00_Y_F64 | TS00_Z_F64 | TS00_F_F32 | TS00_POLY_ORD | TS00_CLIP_FIT, POLY_ORDER_X, POLY_ORDER_Y, POLY_ORDER_Z, NUM_DATA, false);
    testStatus &= genericTest(TS00_MASK_S32 | TS00_FERR_F64 | TS00_X_F64 | TS00_Y_F64 | TS00_Z_F64 | TS00_F_F64 | TS00_POLY_ORD | TS00_CLIP_FIT, POLY_ORDER_X, POLY_ORDER_Y, POLY_ORDER_Z, NUM_DATA, false);

    printFooter(stdout, "psMinimize functions: 3D Polynomial Fitting Functions", "", testStatus);
}
