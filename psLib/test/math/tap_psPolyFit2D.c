/*****************************************************************************
This routine must ensure that various psLib functions which fit 2D polynomials
to data work correctly.  There is a function genericTest() which creates
vectors of data points (x and f), and populates them with the values from an
arbitrary function setData().  It then calls appropriate 2D fitting function.
It then evaluates the polynomial with the coefficients generated above and
determines if they are within an error tolerance of the expected values.
 *****************************************************************************/
#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include "tap.h"
#include "pstap.h"

#define NUM_DATA 100
#define POLY_ORDER_X 2
#define POLY_ORDER_Y 2
#define A 2.0
#define B 3.0
#define C 4.0
#define D 5.0
#define E 6.0
#define F 4.0
#define ERROR_TOLERANCE 0.10
#define YERR 10.0
#define VERBOSE 0
#define EXTRA_VERBOSE 0
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

psF32 setData(psF32 x, psF32 y)
{
    return(A + (B * x) + (C * x * x) + (D * y) + (E * y * y) + (F * x * y));
}

psS32 genericTest(
    psU32 flags,
    psS32 polyOrderX,
    psS32 polyOrderY,
    psS32 numData,
    bool expectedRC)
{
    psS32 currentId = psMemGetId();
    psS32 testStatus = true;
    psS32 memLeaks = 0;
    psPolynomial2D *myPoly = NULL;
    psVector *x = NULL;
    psVector *y = NULL;
    psVector *f = NULL;
    psVector *mask = NULL;
    psVector *fErr = NULL;
    psStats *stats = psStatsAlloc(PS_STAT_SAMPLE_MEAN);
    stats->clipSigma = CLIP_SIGMA;
    stats->clipIter = NUM_ITERATIONS;
    stats->options |= PS_STAT_SAMPLE_STDEV;

    if (VERBOSE)
        printf("psMinimize functions: 2D Polynomial Fitting Functions");

    if (expectedRC == false && VERBOSE) {
        printf("This test should generate an error message, and return NULL.\n");
    }

    if (VERBOSE) {
        if (flags & TS00_CLIP_FIT) {
            printf(" performing a clip-fit\n");
        } else {
            printf(" performing a non clip-fit\n");
        }
    }

    if (flags & TS00_POLY_ORD) {
        if (VERBOSE)
            printf(" using ordinary polynomials\n");
        myPoly = psPolynomial2DAlloc(PS_POLYNOMIAL_ORD, polyOrderX, polyOrderY);
    }


    psVector *xTruth = psVectorAlloc(numData, PS_TYPE_F64);
    psVector *yTruth = psVectorAlloc(numData, PS_TYPE_F64);
    psVector *fTruth = psVectorAlloc(numData, PS_TYPE_F64);
    xTruth->n = numData;
    yTruth->n = numData;
    fTruth->n = numData;
    psRandom *rng = psRandomAllocSpecific(PS_RANDOM_TAUS, 1); // Using an RNG with a known seed
    for (int i = 0; i < numData; i++) {
        xTruth->data.F64[i] = 2.0*psRandomUniform(rng) - 1.0;
        yTruth->data.F64[i] = 2.0*psRandomUniform(rng) - 1.0;
        fTruth->data.F64[i] = setData(xTruth->data.F64[i], yTruth->data.F64[i]);
    }
    psFree(rng);

    if (flags & TS00_X_NULL) {
        if (VERBOSE)
            printf(" using a NULL x vector\n");
    }

    if (flags & TS00_X_F32) {
        if (VERBOSE)
            printf(" using a psF32 x vector\n");
        x = psVectorCopy(NULL, xTruth, PS_TYPE_F32);
    }

    if (flags & TS00_X_S32) {
        if (VERBOSE)
            printf(" using a psS32 x vector\n");
        x = psVectorCopy(NULL, xTruth, PS_TYPE_S32);
    }

    if (flags & TS00_X_F64) {
        if (VERBOSE)
            printf(" using a psF64 x vector\n");
        x = psVectorCopy(NULL, xTruth, PS_TYPE_F64);
    }

    if (flags & TS00_Y_NULL) {
        if (VERBOSE)
            printf(" using a NULL y vector\n");
    }

    if (flags & TS00_Y_F32) {
        if (VERBOSE)
            printf(" using a psF32 y vector\n");
        y = psVectorCopy(NULL, yTruth, PS_TYPE_F32);
    }

    if (flags & TS00_Y_S32) {
        if (VERBOSE)
            printf(" using a psS32 y vector\n");
        y = psVectorCopy(NULL, yTruth, PS_TYPE_S32);
    }

    if (flags & TS00_Y_F64) {
        if (VERBOSE)
            printf(" using a psF64 y vector\n");
        y = psVectorCopy(NULL, yTruth, PS_TYPE_F64);
    }

    if (flags & TS00_F_NULL) {
        if (VERBOSE)
            printf(" using a NULL f vector\n");
    }

    if (flags & TS00_F_F32) {
        if (VERBOSE)
            printf(" using a psF32 f vector\n");
        f = psVectorCopy(NULL, fTruth, PS_TYPE_F32);
        // Set a few outliers in the data.
        if (OUTLIERS && (flags & TS00_CLIP_FIT)) {
            f->data.F32[numData/4]*= 2.0;
            f->data.F32[numData/2]*= 2.0;
            f->data.F32[3*numData/4]*= 2.0;
        }

        if (EXTRA_VERBOSE) {
            for (psS32 i=0;i<numData;i++) {
                printf("Original data %d: (%.1f %.1f)\n", i, (psF32) i, f->data.F32[i]);
            }
        }
    }

    if (flags & TS00_F_S32) {
        if (VERBOSE)
            printf(" using a psS32 f vector\n");
        f = psVectorCopy(NULL, fTruth, PS_TYPE_S32);
        // Set a few outliers in the data.
        if (OUTLIERS && (flags & TS00_CLIP_FIT)) {
            f->data.S32[numData/4]*= 2.0;
            f->data.S32[numData/2]*= 2.0;
            f->data.S32[3*numData/4]*= 2.0;
        }

        if (EXTRA_VERBOSE) {
            for (psS32 i=0;i<numData;i++) {
                printf("Original data %d: (%.1f %d)\n", i, (psF32) i, f->data.S32[i]);
            }
        }
    }

    if (flags & TS00_F_F64) {
        if (VERBOSE)
            printf(" using a psF64 f vector\n");
        f = psVectorCopy(NULL, fTruth, PS_TYPE_F64);
        // Set a few outliers in the data.
        if (OUTLIERS && (flags & TS00_CLIP_FIT)) {
            f->data.F64[numData/4]*= 2.0;
            f->data.F64[numData/2]*= 2.0;
            f->data.F64[3*numData/4]*= 2.0;
        }

        if (EXTRA_VERBOSE) {
            for (psS32 i=0;i<numData;i++) {
                printf("Original data %d: (%.1f %.1f)\n", i, (psF32) i, f->data.F64[i]);
            }
        }
    }

    if (flags & TS00_FERR_NULL) {
        if (VERBOSE)
            printf(" using a NULL fErr vector\n");
    }

    if (flags & TS00_FERR_F32) {
        if (VERBOSE)
            printf(" using a psF32 fErr vector\n");
        fErr = psVectorAlloc(numData, PS_TYPE_F32);
        fErr->n = numData;
        for (psS32 i=0;i<numData;i++) {
            fErr->data.F32[i] = YERR;
        }
    }

    if (flags & TS00_FERR_S32) {
        if (VERBOSE)
            printf(" using a psS32 fErr vector\n");
        fErr = psVectorAlloc(numData, PS_TYPE_S32);
        for (psS32 i=0;i<numData;i++) {
            fErr->data.S32[i] = (psS32) YERR;
        }
    }

    if (flags & TS00_FERR_F64) {
        if (VERBOSE)
            printf(" using a psF64 fErr vector\n");
        fErr = psVectorAlloc(numData, PS_TYPE_F64);
        fErr->n = numData;
        for (psS32 i=0;i<numData;i++) {
            fErr->data.F64[i] = YERR;
        }
    }

    if (flags & TS00_MASK_NULL) {
        if (VERBOSE)
            printf(" using a NULL mask vector\n");
    }

    if (flags & TS00_MASK_U8) {
        if (VERBOSE)
            printf(" using a psU8 mask vector\n");
        mask = psVectorAlloc(numData, PS_TYPE_U8);
        mask->n = numData;
        for (psS32 i=0;i<numData;i++) {
            mask->data.U8[i] = 0;
        }
    }

    if (flags & TS00_MASK_S32) {
        if (VERBOSE)
            printf(" using a psS32 mask vector\n");
        mask = psVectorAlloc(numData, PS_TYPE_S32);
        mask->n = numData;
        for (psS32 i=0;i<numData;i++) {
            mask->data.S32[i] = 0;
        }
    }

    bool rc = false;
    if (flags & TS00_CLIP_FIT) {
        rc = psVectorClipFitPolynomial2D(myPoly, stats, mask, MASK_VALUE, f, fErr, x, y);
    } else {
        rc = psVectorFitPolynomial2D(myPoly, mask, MASK_VALUE, f, fErr, x, y);
    }

    if (!rc) {
        if (expectedRC == true) {
            diag("TEST ERROR: the 2D polynomial fitting function returned NULL.\n");
            testStatus = false;
        }
    } else {
        if (expectedRC == false) {
            diag("TEST ERROR: the 2D polynomial fitting function returned non-NULL.\n");
            testStatus = false;
        }

        if (EXTRA_VERBOSE) {
            for (psS32 i=0;i<polyOrderX+1;i++) {
                for (psS32 j=0;j<polyOrderY+1;j++) {
                    printf("Polynomial coefficient [%d][%d] is %0.1f\n", i, j, myPoly->coeff[i][j]);
                }
            }
        }

        psVector *result = psPolynomial2DEvalVector(myPoly, xTruth, yTruth);
        for (psS32 i=0 ;i<numData; i++) {
            // Skip the outliers.
            if ((i == numData/4) || (i == numData/2) || (i == 3*numData/4)) {
                continue;
            }
            psF64 actualData = result->data.F64[i];
            psF64 expectData = fTruth->data.F64[i];

            if (fabs(actualData-expectData) > fabs(ERROR_TOLERANCE * expectData)) {
                diag("TEST ERROR: Fitted data %d: (%.1f), expected was (%.1f)\n",
                     i, actualData, expectData);
                testStatus = false;
            } else {
                if (EXTRA_VERBOSE) {
                    printf("GOOD: Fitted data %d: (%.1f), expected was (%.1f)\n",
                           i, actualData, expectData);
                }
            }
        }
        psFree(result);
    }

    psMemCheckCorruption(stderr, false);
    psFree(myPoly);
    psFree(mask);
    psFree(xTruth);
    psFree(yTruth);
    psFree(fTruth);
    psFree(x);
    psFree(y);
    psFree(f);
    psFree(fErr);
    psFree(stats);
    psMemCheckCorruption(stderr, false);
    memLeaks = psMemCheckLeaks(currentId,NULL,stderr,false);
    if (0 != memLeaks) {
        psAbort("Memory Leaks! (%d leaks)", memLeaks);
    }
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
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    plan_tests(88);


    // psVectorFitPolynomial2D()
    // Test various erroneous input paramater configurations
    {
        psMemId id = psMemGetId();
        psPolynomial2D *myPoly = psPolynomial2DAlloc(PS_POLYNOMIAL_ORD, POLY_ORDER_X, POLY_ORDER_Y);
        psVector *x = psVectorAlloc(NUM_DATA, PS_TYPE_F32);
        psVector *xS32 = psVectorAlloc(NUM_DATA, PS_TYPE_S32);
        psVector *y = psVectorAlloc(NUM_DATA, PS_TYPE_F32);
        psVector *yS32 = psVectorAlloc(NUM_DATA, PS_TYPE_S32);
        psVector *f = psVectorAlloc(NUM_DATA, PS_TYPE_F32);
        psVector *fS32 = psVectorAlloc(NUM_DATA, PS_TYPE_S32);
        psVector *mask = psVectorAlloc(NUM_DATA, PS_TYPE_U8);
        psVector *maskS8 = psVectorAlloc(NUM_DATA, PS_TYPE_S8);
        psVector *fErr = psVectorAlloc(NUM_DATA, PS_TYPE_F32);
        psVector *fErrS32 = psVectorAlloc(NUM_DATA, PS_TYPE_S32);


        // Set psPolynomial2D to NULL, should cause error
        {
            psMemId id = psMemGetId();
            bool rc = psVectorFitPolynomial2D(NULL, mask, MASK_VALUE, f, fErr, x, y);
            ok(rc == false, "psVectorFitPolynomial2D() returned FALSE with NULL psPolynomial2D");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }


        // Set mask to incorrect type, should cause error
        {
            psMemId id = psMemGetId();
            bool rc = psVectorFitPolynomial2D(myPoly, maskS8, MASK_VALUE, f, fErr, x, y);
            ok(rc == false, "psVectorFitPolynomial2D() returned FALSE with mask set to incorrect type");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }


        // Set f psVector to incorrect type, should cause error
        {
            psMemId id = psMemGetId();
            bool rc = psVectorFitPolynomial2D(myPoly, mask, MASK_VALUE, fS32, fErr, x, y);
            ok(rc == false, "psVectorFitPolynomial2D() returned FALSE: Set f psVector to incorrect type");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }


        // Set fError vector to incorrect type, should cause error
        {
            psMemId id = psMemGetId();
            bool rc = psVectorFitPolynomial2D(myPoly, mask, MASK_VALUE, f, fErrS32, x, y);
            ok(rc == false, "psVectorFitPolynomial2D() returned FALSE: Set fError vector to incorrect type");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }


        // Set x vector to incorrect type, should cause error
        {
            psMemId id = psMemGetId();
            bool rc = psVectorFitPolynomial2D(myPoly, mask, MASK_VALUE, f, fErr, xS32, y);
            ok(rc == true, "psVectorFitPolynomial2D() returned TRUE: x vector may be S32");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }


        // Set y vector to incorrect type, should cause error
        {
            psMemId id = psMemGetId();
            bool rc = psVectorFitPolynomial2D(myPoly, mask, MASK_VALUE, f, fErr, x, yS32);
            ok(rc == true, "psVectorFitPolynomial2D() returned TRUE: y vector may be S32");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }


        // Incorrect mask psVector size, should cause error
        {
            psMemId id = psMemGetId();
            mask->n++;
            bool rc = psVectorFitPolynomial2D(myPoly, mask, MASK_VALUE, f, fErr, x, y);
            mask->n--;
            ok(rc == false, "psVectorFitPolynomial2D() returned FALSE: Incorrect mask psVector size");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }


        // Incorrect f psVector size, should cause error
        {
            psMemId id = psMemGetId();
            f->n++;
            bool rc = psVectorFitPolynomial2D(myPoly, mask, MASK_VALUE, f, fErr, x, y);
            f->n--;
            ok(rc == false, "psVectorFitPolynomial2D() returned FALSE: Incorrect f psVector size");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }


        // Incorrect fErr psVector size, should cause error
        {
            psMemId id = psMemGetId();
            fErr->n++;
            bool rc = psVectorFitPolynomial2D(myPoly, mask, MASK_VALUE, f, fErr, x, y);
            fErr->n--;
            ok(rc == false, "psVectorFitPolynomial2D() returned FALSE: Incorrect fErr psVector size");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }


        // Incorrect x psVector size, should cause error
        {
            psMemId id = psMemGetId();
            x->n++;
            bool rc = psVectorFitPolynomial2D(myPoly, mask, MASK_VALUE, f, fErr, x, y);
            x->n--;
            ok(rc == false, "psVectorFitPolynomial2D() returned FALSE: Incorrect x psVector size");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }

        psFree(myPoly);
        psFree(x);
        psFree(xS32);
        psFree(y);
        psFree(yS32);
        psFree(f);
        psFree(fS32);
        psFree(mask);
        psFree(maskS8);
        psFree(fErr);
        psFree(fErrS32);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // psVectorClipFitPolynomial2D()
    // Test various erroneous input paramater configurations
    {
        psMemId id = psMemGetId();
        psPolynomial2D *myPoly = psPolynomial2DAlloc(PS_POLYNOMIAL_ORD, POLY_ORDER_X, POLY_ORDER_Y);
        psVector *x = psVectorAlloc(NUM_DATA, PS_TYPE_F32);
        psVector *xS32 = psVectorAlloc(NUM_DATA, PS_TYPE_S32);
        psVector *y = psVectorAlloc(NUM_DATA, PS_TYPE_F32);
        psVector *yS32 = psVectorAlloc(NUM_DATA, PS_TYPE_S32);
        psVector *f = psVectorAlloc(NUM_DATA, PS_TYPE_F32);
        psVector *fS32 = psVectorAlloc(NUM_DATA, PS_TYPE_S32);
        psVector *mask = psVectorAlloc(NUM_DATA, PS_TYPE_U8);
        psVector *maskS8 = psVectorAlloc(NUM_DATA, PS_TYPE_S8);
        psVector *fErr = psVectorAlloc(NUM_DATA, PS_TYPE_F32);
        psVector *fErrS32 = psVectorAlloc(NUM_DATA, PS_TYPE_S32);
        psStats *stats = psStatsAlloc(PS_STAT_SAMPLE_MEAN);


        // Set psPolynomial2D to NULL, should cause error
        {
            psMemId id = psMemGetId();
            bool rc = psVectorClipFitPolynomial2D(NULL, stats, mask, MASK_VALUE, f, fErr, x, y);
            ok(rc == false, "psVectorClipFitPolynomial2D() returned FALSE with NULL psPolynomial2D");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }


        // Set psStats to NULL, should cause error
        {
            psMemId id = psMemGetId();
            bool rc = psVectorClipFitPolynomial2D(myPoly, NULL, mask, MASK_VALUE, f, fErr, x, y);
            ok(rc == false, "psVectorClipFitPolynomial2D() returned FALSE with NULL psStats");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }


        // Set mask to incorrect type, should cause error
        {
            psMemId id = psMemGetId();
            bool rc = psVectorClipFitPolynomial2D(myPoly, stats, maskS8, MASK_VALUE, f, fErr, x, y);
            ok(rc == false, "psVectorClipFitPolynomial2D() returned FALSE with mask set to incorrect type");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }


        // Set f psVector to incorrect type, should cause error
        {
            psMemId id = psMemGetId();
            bool rc = psVectorClipFitPolynomial2D(myPoly, stats, mask, MASK_VALUE, fS32, fErr, x, y);
            ok(rc == false, "psVectorClipFitPolynomial2D() returned FALSE: Set f psVector to incorrect type");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }


        // Set fError vector to incorrect type, should cause error
        {
            psMemId id = psMemGetId();
            bool rc = psVectorClipFitPolynomial2D(myPoly, stats, mask, MASK_VALUE, f, fErrS32, x, y);
            ok(rc == false, "psVectorClipFitPolynomial2D() returned FALSE: Set fError vector to incorrect type");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }


        // Set x vector to incorrect type, should cause error
        {
            psMemId id = psMemGetId();
            bool rc = psVectorClipFitPolynomial2D(myPoly, stats, mask, MASK_VALUE, f, fErr, xS32, y);
            ok(rc == false, "psVectorClipFitPolynomial2D() returned FALSE: Set x vector to incorrect type");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }


        // Set y vector to incorrect type, should cause error
        {
            psMemId id = psMemGetId();
            bool rc = psVectorClipFitPolynomial2D(myPoly, stats, mask, MASK_VALUE, f, fErr, x, yS32);
            ok(rc == false, "psVectorClipFitPolynomial2D() returned FALSE: Set y vector to incorrect type");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }


        // Incorrect mask psVector size, should cause error
        {
            psMemId id = psMemGetId();
            mask->n++;
            bool rc = psVectorClipFitPolynomial2D(myPoly, stats, mask, MASK_VALUE, f, fErr, x, y);
            mask->n--;
            ok(rc == false, "psVectorClipFitPolynomial2D() returned FALSE: Incorrect mask psVector size");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }


        // Incorrect f psVector size, should cause error
        {
            psMemId id = psMemGetId();
            f->n++;
            bool rc = psVectorClipFitPolynomial2D(myPoly, stats, mask, MASK_VALUE, f, fErr, x, y);
            f->n--;
            ok(rc == false, "psVectorClipFitPolynomial2D() returned FALSE: Incorrect f psVector size");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }


        // Incorrect fErr psVector size, should cause error
        {
            psMemId id = psMemGetId();
            fErr->n++;
            bool rc = psVectorClipFitPolynomial2D(myPoly, stats, mask, MASK_VALUE, f, fErr, x, y);
            fErr->n--;
            ok(rc == false, "psVectorClipFitPolynomial2D() returned FALSE: Incorrect fErr psVector size");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }


        // Incorrect x psVector size, should cause error
        {
            psMemId id = psMemGetId();
            x->n++;
            bool rc = psVectorClipFitPolynomial2D(myPoly, stats, mask, MASK_VALUE, f, fErr, x, y);
            x->n--;
            ok(rc == false, "psVectorClipFitPolynomial2D() returned FALSE: Incorrect x psVector size");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }

        psFree(myPoly);
        psFree(x);
        psFree(xS32);
        psFree(y);
        psFree(yS32);
        psFree(f);
        psFree(fS32);
        psFree(mask);
        psFree(maskS8);
        psFree(fErr);
        psFree(fErrS32);
        psFree(stats);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    //
    // F32 tests: Ordinary polys, non-clip fit
    //
    // All Vectors non-NULL
    ok(genericTest(TS00_MASK_U8 | TS00_FERR_F32 | TS00_X_F32 | TS00_Y_F32 | TS00_F_F32 | TS00_POLY_ORD, POLY_ORDER_X, POLY_ORDER_Y, NUM_DATA, true), "F32 tests: Ordinary polys, non-clip fit");
    // Some Vectors NULL
    ok(genericTest(TS00_MASK_U8 | TS00_FERR_NULL | TS00_X_F32 | TS00_Y_F32 | TS00_F_F32 | TS00_POLY_ORD, POLY_ORDER_X, POLY_ORDER_Y, NUM_DATA, true), "F32 tests: Ordinary polys, non-clip fit.  Some Vectors NULL");
    ok(genericTest(TS00_MASK_U8 | TS00_FERR_F32 | TS00_X_NULL | TS00_Y_F32 | TS00_F_F32 | TS00_POLY_ORD, POLY_ORDER_X, POLY_ORDER_Y, NUM_DATA, false), "F32 tests: Ordinary polys, non-clip fit.  Some Vectors NULL");
    ok(genericTest(TS00_MASK_U8 | TS00_FERR_F32 | TS00_X_F32 | TS00_Y_NULL | TS00_F_F32 | TS00_POLY_ORD, POLY_ORDER_X, POLY_ORDER_Y, NUM_DATA, false), "F32 tests: Ordinary polys, non-clip fit.  Some Vectors NULL");
    // F-vector NULL
    ok(genericTest(TS00_MASK_U8 | TS00_FERR_F32 | TS00_X_F32 | TS00_Y_F32 | TS00_F_NULL | TS00_POLY_ORD, POLY_ORDER_X, POLY_ORDER_Y, NUM_DATA, false), "F32 tests: Ordinary polys, non-clip fit.  F Vector NULL");
    ok(genericTest(TS00_MASK_NULL | TS00_FERR_F32 | TS00_X_F32 | TS00_Y_F32 | TS00_F_F32 | TS00_POLY_ORD, POLY_ORDER_X, POLY_ORDER_Y, NUM_DATA, true), "F32 tests: Ordinary polys, non-clip fit.  F Vector NULL");
    // Mismatch vector types
    ok(genericTest(TS00_MASK_U8 | TS00_FERR_F64 | TS00_X_F32 | TS00_Y_F32 | TS00_F_F32 | TS00_POLY_ORD, POLY_ORDER_X, POLY_ORDER_Y, NUM_DATA, true), "F32 tests: Ordinary polys, non-clip fit.  Mismatch vector types");
    ok(genericTest(TS00_MASK_U8 | TS00_FERR_F32 | TS00_X_F64 | TS00_Y_F32 | TS00_F_F32 | TS00_POLY_ORD, POLY_ORDER_X, POLY_ORDER_Y, NUM_DATA, true), "F32 tests: Ordinary polys, non-clip fit.  Mismatch vector types");
    ok(genericTest(TS00_MASK_U8 | TS00_FERR_F32 | TS00_X_F32 | TS00_Y_F64 | TS00_F_F32 | TS00_POLY_ORD, POLY_ORDER_X, POLY_ORDER_Y, NUM_DATA, true), "F32 tests: Ordinary polys, non-clip fit.  Mismatch vector types");
    ok(genericTest(TS00_MASK_U8 | TS00_FERR_F32 | TS00_X_F32 | TS00_Y_F32 | TS00_F_F64 | TS00_POLY_ORD, POLY_ORDER_X, POLY_ORDER_Y, NUM_DATA, true), "F32 tests: Ordinary polys, non-clip fit.  Mismatch vector types");
    ok(genericTest(TS00_MASK_S32 | TS00_FERR_F32 | TS00_X_F32 | TS00_Y_F32 | TS00_F_F32 | TS00_POLY_ORD, POLY_ORDER_X, POLY_ORDER_Y, NUM_DATA, false), "F32 tests: Ordinary polys, non-clip fit.  Mismatch vector types");

    //
    // F64 tests: Ordinary polys, non-clip fit
    //
    // All Vectors non-NULL
    ok(genericTest(TS00_MASK_U8 | TS00_FERR_F64 | TS00_X_F64 | TS00_Y_F64 | TS00_F_F64 | TS00_POLY_ORD, POLY_ORDER_X, POLY_ORDER_Y, NUM_DATA, true), "F64 tests: Ordinary polys, non-clip fit");
    // Some Vectors NULL
    ok(genericTest(TS00_MASK_U8 | TS00_FERR_NULL | TS00_X_F64 | TS00_Y_F64 | TS00_F_F64 | TS00_POLY_ORD, POLY_ORDER_X, POLY_ORDER_Y, NUM_DATA, true), "F64 tests: Ordinary polys, non-clip fit.  Some Vectors NULL");
    ok(genericTest(TS00_MASK_U8 | TS00_FERR_F64 | TS00_X_NULL | TS00_Y_F64 | TS00_F_F64 | TS00_POLY_ORD, POLY_ORDER_X, POLY_ORDER_Y, NUM_DATA, false), "F64 tests: Ordinary polys, non-clip fit.  Some Vectors NULL");
    ok(genericTest(TS00_MASK_U8 | TS00_FERR_F64 | TS00_X_F64 | TS00_Y_NULL | TS00_F_F64 | TS00_POLY_ORD, POLY_ORDER_X, POLY_ORDER_Y, NUM_DATA, false), "F64 tests: Ordinary polys, non-clip fit.  Some Vectors NULL");
    // F-vector NULL
    ok(genericTest(TS00_MASK_U8 | TS00_FERR_F64 | TS00_X_F64 | TS00_Y_F64 | TS00_F_NULL | TS00_POLY_ORD, POLY_ORDER_X, POLY_ORDER_Y, NUM_DATA, false), "F64 tests: Ordinary polys, non-clip fit.  F Vector NULL");
    ok(genericTest(TS00_MASK_NULL | TS00_FERR_F64 | TS00_X_F64 | TS00_Y_F64 | TS00_F_F64 | TS00_POLY_ORD, POLY_ORDER_X, POLY_ORDER_Y, NUM_DATA, true), "F64 tests: Ordinary polys, non-clip fit.  F Vector NULL");
    // Mismatch vector types
    ok(genericTest(TS00_MASK_U8 | TS00_FERR_F32 | TS00_X_F64 | TS00_Y_F64 | TS00_F_F64 | TS00_POLY_ORD, POLY_ORDER_X, POLY_ORDER_Y, NUM_DATA, true), "F64 tests: Ordinary polys, non-clip fit.  Mismatch vector types");
    ok(genericTest(TS00_MASK_U8 | TS00_FERR_F64 | TS00_X_F32 | TS00_Y_F64 | TS00_F_F64 | TS00_POLY_ORD, POLY_ORDER_X, POLY_ORDER_Y, NUM_DATA, true), "F64 tests: Ordinary polys, non-clip fit.  Mismatch vector types");
    ok(genericTest(TS00_MASK_U8 | TS00_FERR_F64 | TS00_X_F64 | TS00_Y_F32 | TS00_F_F64 | TS00_POLY_ORD, POLY_ORDER_X, POLY_ORDER_Y, NUM_DATA, true), "F64 tests: Ordinary polys, non-clip fit.  Mismatch vector types");
    ok(genericTest(TS00_MASK_U8 | TS00_FERR_F64 | TS00_X_F64 | TS00_Y_F64 | TS00_F_F32 | TS00_POLY_ORD, POLY_ORDER_X, POLY_ORDER_Y, NUM_DATA, true), "F64 tests: Ordinary polys, non-clip fit.  Mismatch vector types");
    ok(genericTest(TS00_MASK_S32 | TS00_FERR_F64 | TS00_X_F64 | TS00_Y_F64 | TS00_F_F64 | TS00_POLY_ORD, POLY_ORDER_X, POLY_ORDER_Y, NUM_DATA, false), "F64 tests: Ordinary polys, non-clip fit.  Mismatch vector types");

    //
    // F32 tests: Ordinary polys, clip fit
    //
    // All Vectors non-NULL
    ok(genericTest(TS00_MASK_U8 | TS00_FERR_F32 | TS00_X_F32 | TS00_Y_F32 | TS00_F_F32 | TS00_POLY_ORD | TS00_CLIP_FIT, POLY_ORDER_X, POLY_ORDER_Y, NUM_DATA, true), "F32 tests: Ordinary polys, clip fit");
    // Some Vectors NULL
    ok(genericTest(TS00_MASK_U8 | TS00_FERR_NULL | TS00_X_F32 | TS00_Y_F32 | TS00_F_F32 | TS00_POLY_ORD | TS00_CLIP_FIT, POLY_ORDER_X, POLY_ORDER_Y, NUM_DATA, true), "F32 tests: Ordinary polys, clip fit.  Some Vectors NULL");
    ok(genericTest(TS00_MASK_U8 | TS00_FERR_F32 | TS00_X_NULL | TS00_Y_F32 | TS00_F_F32 | TS00_POLY_ORD | TS00_CLIP_FIT, POLY_ORDER_X, POLY_ORDER_Y, NUM_DATA, false), "F32 tests: Ordinary polys, clip fit.  Some Vectors NULL");
    ok(genericTest(TS00_MASK_U8 | TS00_FERR_F32 | TS00_X_F32 | TS00_Y_NULL | TS00_F_F32 | TS00_POLY_ORD | TS00_CLIP_FIT, POLY_ORDER_X, POLY_ORDER_Y, NUM_DATA, false), "F32 tests: Ordinary polys, clip fit.  Some Vectors NULL");
    // F-vector NULL
    ok(genericTest(TS00_MASK_U8 | TS00_FERR_F32 | TS00_X_F32 | TS00_Y_F32 | TS00_F_NULL | TS00_POLY_ORD | TS00_CLIP_FIT, POLY_ORDER_X, POLY_ORDER_Y, NUM_DATA, false), "F32 tests: Ordinary polys, clip fit.  F Vector NULL");
    ok(genericTest(TS00_MASK_NULL | TS00_FERR_F32 | TS00_X_F32 | TS00_Y_F32 | TS00_F_F32 | TS00_POLY_ORD | TS00_CLIP_FIT, POLY_ORDER_X, POLY_ORDER_Y, NUM_DATA, false), "F32 tests: Ordinary polys, clip fit.  Some Vectors NULL");
    // Mismatch vector types
    ok(genericTest(TS00_MASK_U8 | TS00_FERR_F64 | TS00_X_F32 | TS00_Y_F32 | TS00_F_F32 | TS00_POLY_ORD | TS00_CLIP_FIT, POLY_ORDER_X, POLY_ORDER_Y, NUM_DATA, false), "F32 tests: Ordinary polys, clip fit.  Mismatch vector types");
    ok(genericTest(TS00_MASK_U8 | TS00_FERR_F32 | TS00_X_F64 | TS00_Y_F32 | TS00_F_F32 | TS00_POLY_ORD | TS00_CLIP_FIT, POLY_ORDER_X, POLY_ORDER_Y, NUM_DATA, false), "F32 tests: Ordinary polys, clip fit.  Mismatch vector types");
    ok(genericTest(TS00_MASK_U8 | TS00_FERR_F32 | TS00_X_F32 | TS00_Y_F64 | TS00_F_F32 | TS00_POLY_ORD | TS00_CLIP_FIT, POLY_ORDER_X, POLY_ORDER_Y, NUM_DATA, false), "F32 tests: Ordinary polys, clip fit.  Mismatch vector types");
    ok(genericTest(TS00_MASK_U8 | TS00_FERR_F32 | TS00_X_F32 | TS00_Y_F32 | TS00_F_F64 | TS00_POLY_ORD | TS00_CLIP_FIT, POLY_ORDER_X, POLY_ORDER_Y, NUM_DATA, false), "F32 tests: Ordinary polys, clip fit.  Mismatch vector types");
    ok(genericTest(TS00_MASK_S32 | TS00_FERR_F32 | TS00_X_F32 | TS00_Y_F32 | TS00_F_F32 | TS00_POLY_ORD | TS00_CLIP_FIT, POLY_ORDER_X, POLY_ORDER_Y, NUM_DATA, false), "F32 tests: Ordinary polys, clip fit.  Mismatch vector types");

    //
    // F64 tests: Ordinary polys, clip fit
    //
    // All Vectors non-NULL
    ok(genericTest(TS00_MASK_U8 | TS00_FERR_F64 | TS00_X_F64 | TS00_Y_F64 | TS00_F_F64 | TS00_POLY_ORD | TS00_CLIP_FIT, POLY_ORDER_X, POLY_ORDER_Y, NUM_DATA, true), "F64 tests: Ordinary polys, clip fit");
    // Some Vectors NULL
    ok(genericTest(TS00_MASK_U8 | TS00_FERR_NULL | TS00_X_F64 | TS00_Y_F64 | TS00_F_F64 | TS00_POLY_ORD | TS00_CLIP_FIT, POLY_ORDER_X, POLY_ORDER_Y, NUM_DATA, true), "F64 tests: Ordinary polys, clip fit.  Some Vectors NULL");
    ok(genericTest(TS00_MASK_U8 | TS00_FERR_F64 | TS00_X_NULL | TS00_Y_F64 | TS00_F_F64 | TS00_POLY_ORD | TS00_CLIP_FIT, POLY_ORDER_X, POLY_ORDER_Y, NUM_DATA, false), "F64 tests: Ordinary polys, clip fit.  Some Vectors NULL");
    ok(genericTest(TS00_MASK_U8 | TS00_FERR_F64 | TS00_X_F64 | TS00_Y_NULL | TS00_F_F64 | TS00_POLY_ORD | TS00_CLIP_FIT, POLY_ORDER_X, POLY_ORDER_Y, NUM_DATA, false), "F64 tests: Ordinary polys, clip fit.  Some Vectors NULL");
    // F-vector NULL
    ok(genericTest(TS00_MASK_U8 | TS00_FERR_F64 | TS00_X_F64 | TS00_Y_F64 | TS00_F_NULL | TS00_POLY_ORD | TS00_CLIP_FIT, POLY_ORDER_X, POLY_ORDER_Y, NUM_DATA, false), "F64 tests: Ordinary polys, clip fit.  F Vector NULL");
    ok(genericTest(TS00_MASK_NULL | TS00_FERR_F64 | TS00_X_F64 | TS00_Y_F64 | TS00_F_F32 | TS00_POLY_ORD | TS00_CLIP_FIT, POLY_ORDER_X, POLY_ORDER_Y, NUM_DATA, false), "F64 tests: Ordinary polys, clip fit.  F Vector NULL");
    // Mismatch vector types
    ok(genericTest(TS00_MASK_U8 | TS00_FERR_F32 | TS00_X_F64 | TS00_Y_F64 | TS00_F_F64 | TS00_POLY_ORD | TS00_CLIP_FIT, POLY_ORDER_X, POLY_ORDER_Y, NUM_DATA, false), "F64 tests: Ordinary polys, clip fit.  Mismatch vector types");
    ok(genericTest(TS00_MASK_U8 | TS00_FERR_F64 | TS00_X_F32 | TS00_Y_F64 | TS00_F_F64 | TS00_POLY_ORD | TS00_CLIP_FIT, POLY_ORDER_X, POLY_ORDER_Y, NUM_DATA, false), "F64 tests: Ordinary polys, clip fit.  Mismatch vector types");
    ok(genericTest(TS00_MASK_U8 | TS00_FERR_F64 | TS00_X_F64 | TS00_Y_F32 | TS00_F_F64 | TS00_POLY_ORD | TS00_CLIP_FIT, POLY_ORDER_X, POLY_ORDER_Y, NUM_DATA, false), "F64 tests: Ordinary polys, clip fit.  Mismatch vector types");
    ok(genericTest(TS00_MASK_U8 | TS00_FERR_F64 | TS00_X_F64 | TS00_Y_F64 | TS00_F_F32 | TS00_POLY_ORD | TS00_CLIP_FIT, POLY_ORDER_X, POLY_ORDER_Y, NUM_DATA, false), "F64 tests: Ordinary polys, clip fit.  Mismatch vector types");
    ok(genericTest(TS00_MASK_S32 | TS00_FERR_F64 | TS00_X_F64 | TS00_Y_F64 | TS00_F_F64 | TS00_POLY_ORD | TS00_CLIP_FIT, POLY_ORDER_X, POLY_ORDER_Y, NUM_DATA, false), "F64 tests: Ordinary polys, clip fit.  Mismatch vector types");
}
