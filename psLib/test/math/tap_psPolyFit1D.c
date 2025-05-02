/*****************************************************************************
This routine must ensure that various psLib functions which fit 1D polynomials
to data work correctly.  There is a function genericTest() which creates
vectors of data points (x and f), and populates them with the values from an
arbitrary function setData().  It then calls appropriate 1D fitting function.
It then evaluates the polynomial with the coefficients generated above and
determines if they are within an error tolerance of the expected values.
 
XXX: Try null stats.
 *****************************************************************************/
#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include "tap.h"
#include "pstap.h"

#define NUM_DATA 100
#define POLY_ORDER 5
#define A 2.0
#define B 3.0
#define C 4.0
#define D 5.0
#define E 6.0
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

psF32 setData(psF32 x)
{
    return(A + (B * x) + (C * x * x) + (D * x * x * x) + (E * x * x * x * x));
}

bool genericTest(
    psU32 flags,
    psS32 polyOrder,
    psS32 numData,
    bool expectedRC)
{
    psS32 currentId = psMemGetId();
    bool testStatus = true;
    psS32 memLeaks = 0;
    psPolynomial1D *myPoly = NULL;
    psVector *x = NULL;
    psVector *f = NULL;
    psVector *mask = NULL;
    psVector *fErr = NULL;
    psStats *stats = psStatsAlloc(PS_STAT_SAMPLE_MEAN);
    stats->clipSigma = CLIP_SIGMA;
    stats->clipIter = NUM_ITERATIONS;
    stats->options |= PS_STAT_SAMPLE_STDEV;

    if (VERBOSE)
        printf("psMinimize functions: 1D Polynomial Fitting Functions");

    if (expectedRC == false && VERBOSE) {
        printf("This test should generate an error message, and return NULL.\n");
    }

    if (VERBOSE) {
        if (flags & TS00_CLIP_FIT) {
            printf("        performing a clip-fit\n");
        } else {
            printf("        performing a non clip-fit\n");
        }
    }

    if (flags & TS00_POLY_ORD) {
        if (VERBOSE)
            printf(" using ordinary polynomials\n");
        myPoly = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, polyOrder);
    }

    if (flags & TS00_POLY_CHEB) {
        if (VERBOSE)
            printf(" using chebyshev polynomials\n");
        myPoly = psPolynomial1DAlloc(PS_POLYNOMIAL_CHEB, polyOrder);
    }

    if (flags & TS00_X_NULL) {
        if (VERBOSE)
            printf(" using a NULL x vector\n");
        numData = 30;
    }

    psVector *xTruth = psVectorAlloc(numData, PS_TYPE_F64);
    psVector *fTruth = psVectorAlloc(numData, PS_TYPE_F64);
    psRandom *rng = psRandomAllocSpecific(PS_RANDOM_TAUS, 1); // Using a known seed
    for (int i = 0; i < numData; i++) {
        xTruth->data.F64[i] = (flags & TS00_X_NULL) ? i : 2.0*psRandomUniform(rng) - 1.0;
        fTruth->data.F64[i] = setData(xTruth->data.F64[i]);
    }
    if (flags & TS00_X_NULL && flags & TS00_POLY_CHEB) {
        // Renormalise the indices
        p_psNormalizeVectorRange(xTruth, -1.0, 1.0);
    }
    psFree(rng);
    if (EXTRA_VERBOSE)
        for (int i = 0; i < numData; i++) {
            printf("Original %d: %f\t%f\n", i, xTruth->data.F64[i], fTruth->data.F64[i]);
        }

    if (flags & TS00_X_F32) {
        if (VERBOSE)
            printf(" using a psF32 x vector\n");
        x = psVectorCopy(NULL, xTruth, PS_TYPE_F32);

        #if 0

        if (flags & TS00_POLY_CHEB) {
            p_psNormalizeVectorRange(x, -1.0, 1.0);
        }
        #endif

    }

    if (flags & TS00_X_S32) {
        if (VERBOSE)
            printf(" using a psS32 x vector\n");
        x = psVectorCopy(NULL, xTruth, PS_TYPE_S32);

        #if 0

        if (flags & TS00_POLY_CHEB) {
            p_psNormalizeVectorRange(x, -1, 1);
        }
        #endif

    }

    if (flags & TS00_X_F64) {
        if (VERBOSE)
            printf(" using a psF64 x vector\n");
        x = psVectorCopy(NULL, xTruth, PS_TYPE_F64);

        #if 0

        if (flags & TS00_POLY_CHEB) {
            p_psNormalizeVectorRange(x, -1.0, 1.0);
        }
        #endif

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
        fErr->n = numData;
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
        rc = psVectorClipFitPolynomial1D(myPoly, stats, mask, MASK_VALUE, f, fErr, x);
    } else {
        rc = psVectorFitPolynomial1D(myPoly, mask, MASK_VALUE, f, fErr, x);
    }

    if (!rc) {
        if (expectedRC == true) {
            diag("TEST ERROR: the 1D polynomial fitting function returned NULL.\n");
            testStatus = false;
        }
    } else {
        if (expectedRC == false) {
            diag("TEST ERROR: the 1D polynomial fitting function returned non-NULL.\n");
            testStatus = false;
        }

        if (EXTRA_VERBOSE) {
            for (psS32 i=0;i<polyOrder+1;i++) {
                printf("Polynomial coefficient %d is %0.1f\n", i, myPoly->coeff[i]);
            }
        }

        psVector *result = psPolynomial1DEvalVector(myPoly, xTruth);
        for (psS32 i=0; i<numData; i++) {
            // Skip the outliers.
            if ((i == numData/4) || (i == numData/2) || (i == 3*numData/4)) {
                continue;
            }
            psF32 expectData = fTruth->data.F64[i];
            psF32 actualData = result->data.F64[i];
            if (fabs(actualData-expectData) > fabs(ERROR_TOLERANCE * expectData)) {
                diag("TEST ERROR: Fitted data %d: %.1f --> %.1f vs %.1f\n",
                     i, xTruth->data.F64[i], actualData, expectData);
                testStatus = false;
            } else {
                if (EXTRA_VERBOSE) {
                    printf("GOOD: Fitted data %d: %1.f --> %.1f vs %.1f\n",
                           i, xTruth->data.F64[i], actualData, expectData);
                }
            }
        }
        psFree(result);
    }

    psMemCheckCorruption(stderr, false);
    psFree(myPoly);
    psFree(mask);
    psFree(x);
    psFree(f);
    psFree(xTruth);
    psFree(fTruth);
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
int main()
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    plan_tests(104);


    // psVectorFitPolynomial1D()
    // Test various erroneous input paramater configurations
    {
        psMemId id = psMemGetId();
        psPolynomial1D *myPoly = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, POLY_ORDER);
        psVector *x = psVectorAlloc(NUM_DATA, PS_TYPE_F32);
        psVector *xS32 = psVectorAlloc(NUM_DATA, PS_TYPE_S32);
        psVector *f = psVectorAlloc(NUM_DATA, PS_TYPE_F32);
        psVector *fS32 = psVectorAlloc(NUM_DATA, PS_TYPE_S32);
        psVector *mask = psVectorAlloc(NUM_DATA, PS_TYPE_U8);
        psVector *maskS8 = psVectorAlloc(NUM_DATA, PS_TYPE_S8);
        psVector *fErr = psVectorAlloc(NUM_DATA, PS_TYPE_F32);
        psVector *fErrS32 = psVectorAlloc(NUM_DATA, PS_TYPE_S32);


        // Set psPolynomial1D to NULL, should cause error
        {
            psMemId id = psMemGetId();
            bool rc = psVectorFitPolynomial1D(NULL, mask, MASK_VALUE, f, fErr, x);
            ok(rc == false, "psVectorFitPolynomial1D() returned FALSE with NULL psPolynomial1D");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }


        // Set mask to incorrect type, should cause error
        {
            psMemId id = psMemGetId();
            bool rc = psVectorFitPolynomial1D(myPoly, maskS8, MASK_VALUE, f, fErr, x);
            ok(rc == false, "psVectorFitPolynomial1D() returned FALSE with mask set to incorrect type");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }


        // Set f psVector to incorrect type, should cause error
        {
            psMemId id = psMemGetId();
            bool rc = psVectorFitPolynomial1D(myPoly, mask, MASK_VALUE, fS32, fErr, x);
            ok(rc == false, "psVectorFitPolynomial1D() returned FALSE: Set f psVector to incorrect type");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }


        // Set fError vector to incorrect type, should cause error
        {
            psMemId id = psMemGetId();
            bool rc = psVectorFitPolynomial1D(myPoly, mask, MASK_VALUE, f, fErrS32, x);
            ok(rc == false, "psVectorFitPolynomial1D() returned FALSE: Set fError vector to incorrect type");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }


        // Set x vector to incorrect type, should cause error
        {
            psMemId id = psMemGetId();
            bool rc = psVectorFitPolynomial1D(myPoly, mask, MASK_VALUE, f, fErr, xS32);
            ok(rc == false, "psVectorFitPolynomial1D() returned FALSE: Set x vector to incorrect type");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }


        // Incorrect mask psVector size, should cause error
        {
            psMemId id = psMemGetId();
            mask->n++;
            bool rc = psVectorFitPolynomial1D(myPoly, mask, MASK_VALUE, f, fErr, x);
            mask->n--;
            ok(rc == false, "psVectorFitPolynomial1D() returned FALSE: Incorrect mask psVector size");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }


        // Incorrect f psVector size, should cause error
        {
            psMemId id = psMemGetId();
            f->n++;
            bool rc = psVectorFitPolynomial1D(myPoly, mask, MASK_VALUE, f, fErr, x);
            f->n--;
            ok(rc == false, "psVectorFitPolynomial1D() returned FALSE: Incorrect f psVector size");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }


        // Incorrect fErr psVector size, should cause error
        {
            psMemId id = psMemGetId();
            fErr->n++;
            bool rc = psVectorFitPolynomial1D(myPoly, mask, MASK_VALUE, f, fErr, x);
            fErr->n--;
            ok(rc == false, "psVectorFitPolynomial1D() returned FALSE: Incorrect fErr psVector size");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }


        // Incorrect x psVector size, should cause error
        {
            psMemId id = psMemGetId();
            x->n++;
            bool rc = psVectorFitPolynomial1D(myPoly, mask, MASK_VALUE, f, fErr, x);
            x->n--;
            ok(rc == false, "psVectorFitPolynomial1D() returned FALSE: Incorrect x psVector size");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }

        psFree(myPoly);
        psFree(x);
        psFree(xS32);
        psFree(f);
        psFree(fS32);
        psFree(mask);
        psFree(maskS8);
        psFree(fErr);
        psFree(fErrS32);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // psVectorClipFitPolynomial1D()
    // Test various erroneous input paramater configurations
    {
        psMemId id = psMemGetId();
        psPolynomial1D *myPoly = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, POLY_ORDER);
        psVector *x = psVectorAlloc(NUM_DATA, PS_TYPE_F32);
        psVector *xS32 = psVectorAlloc(NUM_DATA, PS_TYPE_S32);
        psVector *f = psVectorAlloc(NUM_DATA, PS_TYPE_F32);
        psVector *fS32 = psVectorAlloc(NUM_DATA, PS_TYPE_S32);
        psVector *mask = psVectorAlloc(NUM_DATA, PS_TYPE_U8);
        psVector *maskS8 = psVectorAlloc(NUM_DATA, PS_TYPE_S8);
        psVector *fErr = psVectorAlloc(NUM_DATA, PS_TYPE_F32);
        psVector *fErrS32 = psVectorAlloc(NUM_DATA, PS_TYPE_S32);
        psStats *stats = psStatsAlloc(PS_STAT_SAMPLE_MEAN);


        // Set psPolynomial1D to NULL, should cause error
        {
            psMemId id = psMemGetId();
            bool rc = psVectorClipFitPolynomial1D(NULL, stats, mask, MASK_VALUE, f, fErr, x);
            ok(rc == false, "psVectorClipFitPolynomial1D() returned FALSE with NULL psPolynomial1D");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }


        // Set psStats to NULL, should cause error
        {
            psMemId id = psMemGetId();
            bool rc = psVectorClipFitPolynomial1D(myPoly, NULL, mask, MASK_VALUE, f, fErr, x);
            ok(rc == false, "psVectorClipFitPolynomial1D() returned FALSE with NULL psStats");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }


        // Set mask to incorrect type, should cause error
        {
            psMemId id = psMemGetId();
            bool rc = psVectorClipFitPolynomial1D(myPoly, stats, maskS8, MASK_VALUE, f, fErr, x);
            ok(rc == false, "psVectorClipFitPolynomial1D() returned FALSE with mask set to incorrect type");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }


        // Set f psVector to incorrect type, should cause error
        {
            psMemId id = psMemGetId();
            bool rc = psVectorClipFitPolynomial1D(myPoly, stats, mask, MASK_VALUE, fS32, fErr, x);
            ok(rc == false, "psVectorClipFitPolynomial1D() returned FALSE: Set f psVector to incorrect type");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }


        // Set fError vector to incorrect type, should cause error
        {
            psMemId id = psMemGetId();
            bool rc = psVectorClipFitPolynomial1D(myPoly, stats, mask, MASK_VALUE, f, fErrS32, x);
            ok(rc == false, "psVectorClipFitPolynomial1D() returned FALSE: Set fError vector to incorrect type");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }


        // Set x vector to incorrect type, should cause error
        {
            psMemId id = psMemGetId();
            bool rc = psVectorClipFitPolynomial1D(myPoly, stats, mask, MASK_VALUE, f, fErr, xS32);
            ok(rc == false, "psVectorClipFitPolynomial1D() returned FALSE: Set x vector to incorrect type");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }


        // Incorrect mask psVector size, should cause error
        {
            psMemId id = psMemGetId();
            mask->n++;
            bool rc = psVectorClipFitPolynomial1D(myPoly, stats, mask, MASK_VALUE, f, fErr, x);
            mask->n--;
            ok(rc == false, "psVectorClipFitPolynomial1D() returned FALSE: Incorrect mask psVector size");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }


        // Incorrect f psVector size, should cause error
        {
            psMemId id = psMemGetId();
            f->n++;
            bool rc = psVectorClipFitPolynomial1D(myPoly, stats, mask, MASK_VALUE, f, fErr, x);
            f->n--;
            ok(rc == false, "psVectorClipFitPolynomial1D() returned FALSE: Incorrect f psVector size");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }


        // Incorrect fErr psVector size, should cause error
        {
            psMemId id = psMemGetId();
            fErr->n++;
            bool rc = psVectorClipFitPolynomial1D(myPoly, stats, mask, MASK_VALUE, f, fErr, x);
            fErr->n--;
            ok(rc == false, "psVectorClipFitPolynomial1D() returned FALSE: Incorrect fErr psVector size");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }


        // Incorrect x psVector size, should cause error
        {
            psMemId id = psMemGetId();
            x->n++;
            bool rc = psVectorClipFitPolynomial1D(myPoly, stats, mask, MASK_VALUE, f, fErr, x);
            x->n--;
            ok(rc == false, "psVectorClipFitPolynomial1D() returned FALSE: Incorrect x psVector size");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }

        psFree(myPoly);
        psFree(x);
        psFree(xS32);
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
    ok(genericTest(TS00_MASK_U8 | TS00_FERR_F32 | TS00_X_F32 | TS00_F_F32 | TS00_POLY_ORD, POLY_ORDER, NUM_DATA, true), "F32 tests: Ordinary polys, non-clip fit");
    // Some Vectors NULL
    ok(genericTest(TS00_MASK_NULL | TS00_FERR_F32 | TS00_X_F32 | TS00_F_F32 | TS00_POLY_ORD, POLY_ORDER, NUM_DATA, true), "F32 tests: Ordinary polys, non-clip fit, Some Vectors NULL");
    ok(genericTest(TS00_MASK_U8 | TS00_FERR_NULL | TS00_X_F32 | TS00_F_F32 | TS00_POLY_ORD, POLY_ORDER, NUM_DATA, true), "F32 tests: Ordinary polys, non-clip fit, Some Vectors NULL");
    ok(genericTest(TS00_MASK_U8 | TS00_FERR_F32 | TS00_X_NULL | TS00_F_F32 | TS00_POLY_ORD, POLY_ORDER, NUM_DATA, true), "F32 tests: Ordinary polys, non-clip fit, Some Vectors NULL");
    ok(genericTest(TS00_MASK_NULL | TS00_FERR_NULL | TS00_X_NULL | TS00_F_F32 | TS00_POLY_ORD, POLY_ORDER, NUM_DATA, true), "F32 tests: Ordinary polys, non-clip fit, Some Vectors NULL");
    // F-vector NULL
    ok(genericTest(TS00_MASK_U8 | TS00_FERR_F32 | TS00_X_F32 | TS00_F_NULL | TS00_POLY_ORD, POLY_ORDER, NUM_DATA, false), "F32 tests: Ordinary polys, non-clip fit, F Vector NULL");
    // Unallowable vector types
    ok(genericTest(TS00_MASK_S32 | TS00_FERR_F32 | TS00_X_F32 | TS00_F_F32 | TS00_POLY_ORD, POLY_ORDER, NUM_DATA, false), "F32 tests: Ordinary polys, non-clip fit, Unallowable vector types");
    ok(genericTest(TS00_MASK_U8 | TS00_FERR_S32 | TS00_X_F32 | TS00_F_F32 | TS00_POLY_ORD, POLY_ORDER, NUM_DATA, false), "F32 tests: Ordinary polys, non-clip fit, Unallowable vector types");
    ok(genericTest(TS00_MASK_U8 | TS00_FERR_F32 | TS00_X_S32 | TS00_F_F32 | TS00_POLY_ORD, POLY_ORDER, NUM_DATA, false), "F32 tests: Ordinary polys, non-clip fit, Unallowable vector types");
    ok(genericTest(TS00_MASK_U8 | TS00_FERR_F32 | TS00_X_F32 | TS00_F_S32 | TS00_POLY_ORD, POLY_ORDER, NUM_DATA, false), "F32 tests: Ordinary polys, non-clip fit, Unallowable vector types");
    // Mismatch vector types
    ok(genericTest(TS00_MASK_S32 | TS00_FERR_F32 | TS00_X_F32 | TS00_F_F32 | TS00_POLY_ORD, POLY_ORDER, NUM_DATA, false), "F32 tests: Ordinary polys, non-clip fit, Mismatch vector types");
    ok(genericTest(TS00_MASK_U8 | TS00_FERR_F64 | TS00_X_F32 | TS00_F_F32 | TS00_POLY_ORD, POLY_ORDER, NUM_DATA, true), "F32 tests: Ordinary polys, non-clip fit, Mismatch vector types");
    ok(genericTest(TS00_MASK_U8 | TS00_FERR_F32 | TS00_X_F64 | TS00_F_F32 | TS00_POLY_ORD, POLY_ORDER, NUM_DATA, true), "F32 tests: Ordinary polys, non-clip fit, Mismatch vector types");

    //
    // F64 tests: Ordinary polys, non-clip fit
    //
    // All Vectors non-NULL
    ok(genericTest(TS00_MASK_U8 | TS00_FERR_F64 | TS00_X_F64 | TS00_F_F64 | TS00_POLY_ORD, POLY_ORDER, NUM_DATA, true), "F64 tests: Ordinary polys, non-clip fit");
    // Some Vectors NULL
    ok(genericTest(TS00_MASK_NULL | TS00_FERR_F64 | TS00_X_F64 | TS00_F_F64 | TS00_POLY_ORD, POLY_ORDER, NUM_DATA, true), "F64 tests: Ordinary polys, non-clip fit, Some Vectors NULL");
    ok(genericTest(TS00_MASK_U8 | TS00_FERR_NULL | TS00_X_F64 | TS00_F_F64 | TS00_POLY_ORD, POLY_ORDER, NUM_DATA, true), "F64 tests: Ordinary polys, non-clip fit, Some Vectors NULL");
    ok(genericTest(TS00_MASK_U8 | TS00_FERR_F64 | TS00_X_NULL | TS00_F_F64 | TS00_POLY_ORD, POLY_ORDER, NUM_DATA, true), "F64 tests: Ordinary polys, non-clip fit, Some Vectors NULL");
    ok(genericTest(TS00_MASK_NULL | TS00_FERR_NULL | TS00_X_NULL | TS00_F_F64 | TS00_POLY_ORD, POLY_ORDER, NUM_DATA, true), "F64 tests: Ordinary polys, non-clip fit, Some Vectors NULL");
    // F-vector NULL
    ok(genericTest(TS00_MASK_U8 | TS00_FERR_F64 | TS00_X_F64 | TS00_F_NULL | TS00_POLY_ORD, POLY_ORDER, NUM_DATA, false), "F64 tests: Ordinary polys, non-clip fit, F Vector NULL");
    // Mismatch vector types
    ok(genericTest(TS00_MASK_S32 | TS00_FERR_F64 | TS00_X_F64 | TS00_F_F64 | TS00_POLY_ORD, POLY_ORDER, NUM_DATA, false), "F64 tests: Ordinary polys, non-clip fit, Mismatch data type");
    ok(genericTest(TS00_MASK_U8 | TS00_FERR_F32 | TS00_X_F64 | TS00_F_F64 | TS00_POLY_ORD, POLY_ORDER, NUM_DATA, true), "F64 tests: Ordinary polys, non-clip fit, Mismatch data type");
    ok(genericTest(TS00_MASK_U8 | TS00_FERR_F64 | TS00_X_F32 | TS00_F_F64 | TS00_POLY_ORD, POLY_ORDER, NUM_DATA, true), "F64 tests: Ordinary polys, non-clip fit, Mismatch data type");


    //
    // F32 tests: Chebyshev polys, non-clip fit
    //
    // All Vectors non-NULL
    ok(genericTest(TS00_MASK_U8 | TS00_FERR_F32 | TS00_X_F32 | TS00_F_F32 | TS00_POLY_CHEB, POLY_ORDER, NUM_DATA, true), "F32 tests: Chebyshev polys, non-clip fit");
    // Some Vectors NULL
    ok(genericTest(TS00_MASK_NULL | TS00_FERR_F32 | TS00_X_F32 | TS00_F_F32 | TS00_POLY_CHEB, POLY_ORDER, NUM_DATA, true), "F32 tests: Chebyshev polys, non-clip fit.  Some Vectors NULL.");
    ok(genericTest(TS00_MASK_U8 | TS00_FERR_NULL | TS00_X_F32 | TS00_F_F32 | TS00_POLY_CHEB, POLY_ORDER, NUM_DATA, true), "F32 tests: Chebyshev polys, non-clip fit.  Some Vectors NULL.");
    ok(genericTest(TS00_MASK_U8 | TS00_FERR_F32 | TS00_X_NULL | TS00_F_F32 | TS00_POLY_CHEB, POLY_ORDER, NUM_DATA, true), "F32 tests: Chebyshev polys, non-clip fit.  Some Vectors NULL.");
    ok(genericTest(TS00_MASK_NULL | TS00_FERR_NULL | TS00_X_NULL | TS00_F_F32 | TS00_POLY_CHEB, POLY_ORDER, NUM_DATA, true), "F32 tests: Chebyshev polys, non-clip fit.  Some Vectors NULL.");
    // F-vector NULL
    ok(genericTest(TS00_MASK_U8 | TS00_FERR_F32 | TS00_X_F32 | TS00_F_NULL | TS00_POLY_CHEB, POLY_ORDER, NUM_DATA, false), "F32 tests: Chebyshev polys, non-clip fit.  F Vector NULL.");
    // Mismatch vector types
    ok(genericTest(TS00_MASK_S32 | TS00_FERR_F32 | TS00_X_F32 | TS00_F_F32 | TS00_POLY_CHEB, POLY_ORDER, NUM_DATA, false), "F32 tests: Chebyshev polys, non-clip fit.  Mismatch vector types.");
    ok(genericTest(TS00_MASK_U8 | TS00_FERR_F64 | TS00_X_F32 | TS00_F_F32 | TS00_POLY_CHEB, POLY_ORDER, NUM_DATA, true), "F32 tests: Chebyshev polys, non-clip fit.  Mismatch vector types.");
    ok(genericTest(TS00_MASK_U8 | TS00_FERR_F32 | TS00_X_F64 | TS00_F_F32 | TS00_POLY_CHEB, POLY_ORDER, NUM_DATA, true), "F32 tests: Chebyshev polys, non-clip fit.  Mismatch vector types.");


    //
    // F64 tests: Chebyshev polys, non-clip fit
    //
    // All Vectors non-NULL
    ok(genericTest(TS00_MASK_U8 | TS00_FERR_F64 | TS00_X_F64 | TS00_F_F64 | TS00_POLY_CHEB, POLY_ORDER, NUM_DATA, true), "F64 tests: Chebyshev polys, non-clip fit");
    // Some Vectors NULL
    ok(genericTest(TS00_MASK_NULL | TS00_FERR_F64 | TS00_X_F64 | TS00_F_F64 | TS00_POLY_CHEB, POLY_ORDER, NUM_DATA, true), "F64 tests: Chebyshev polys, non-clip fit.  Some Vectors NULL");
    ok(genericTest(TS00_MASK_U8 | TS00_FERR_NULL | TS00_X_F64 | TS00_F_F64 | TS00_POLY_CHEB, POLY_ORDER, NUM_DATA, true), "F64 tests: Chebyshev polys, non-clip fit.  Some Vectors NULL");
    ok(genericTest(TS00_MASK_U8 | TS00_FERR_F64 | TS00_X_NULL | TS00_F_F64 | TS00_POLY_CHEB, POLY_ORDER, NUM_DATA, true), "F64 tests: Chebyshev polys, non-clip fit.  Some Vectors NULL");
    ok(genericTest(TS00_MASK_NULL | TS00_FERR_NULL | TS00_X_NULL | TS00_F_F64 | TS00_POLY_CHEB, POLY_ORDER, NUM_DATA, true), "F64 tests: Chebyshev polys, non-clip fit.  Some Vectors NULL");
    // F-vector NULL
    ok(genericTest(TS00_MASK_U8 | TS00_FERR_F64 | TS00_X_F64 | TS00_F_NULL | TS00_POLY_CHEB, POLY_ORDER, NUM_DATA, false), "F64 tests: Chebyshev polys, non-clip fit.  F Vector NULL");
    // Mismatch vector types
    ok(genericTest(TS00_MASK_S32 | TS00_FERR_F64 | TS00_X_F64 | TS00_F_F64 | TS00_POLY_CHEB, POLY_ORDER, NUM_DATA, false), "F64 tests: Chebyshev polys, non-clip fit.  Mismatch vector types");
    ok(genericTest(TS00_MASK_U8 | TS00_FERR_F32 | TS00_X_F64 | TS00_F_F64 | TS00_POLY_CHEB, POLY_ORDER, NUM_DATA, true), "F64 tests: Chebyshev polys, non-clip fit.  Mismatch vector types");
    ok(genericTest(TS00_MASK_U8 | TS00_FERR_F64 | TS00_X_F32 | TS00_F_F64 | TS00_POLY_CHEB, POLY_ORDER, NUM_DATA, true), "F64 tests: Chebyshev polys, non-clip fit.  Mismatch vector types");

    //
    // F32 tests: Ordinary polys, clip fit
    //
    // All Vectors non-NULL
    ok(genericTest(TS00_MASK_U8 | TS00_FERR_F32 | TS00_X_F32 | TS00_F_F32 | TS00_POLY_ORD | TS00_CLIP_FIT, POLY_ORDER, NUM_DATA, true), "F32 tests: Ordinary polys, clip fit");
    // Some Vectors NULL
    //    ok(genericTest(TS00_MASK_U8 | TS00_FERR_NULL | TS00_X_F32 | TS00_F_F32 | TS00_POLY_ORD | TS00_CLIP_FIT, POLY_ORDER, NUM_DATA, true), "F32 tests: Ordinary polys, clip fit.  Some Vectors NULL");
    //    ok(genericTest(TS00_MASK_U8 | TS00_FERR_F32 | TS00_X_NULL | TS00_F_F32 | TS00_POLY_ORD | TS00_CLIP_FIT, POLY_ORDER, NUM_DATA, true), "F32 tests: Ordinary polys, clip fit.  Some Vectors NULL");
    //    ok(genericTest(TS00_MASK_U8 | TS00_FERR_NULL | TS00_X_NULL | TS00_F_F32 | TS00_POLY_ORD | TS00_CLIP_FIT, POLY_ORDER, NUM_DATA, true), "F32 tests: Ordinary polys, clip fit.  Some Vectors NULL");
    // F-vector NULL
    ok(genericTest(TS00_MASK_U8 | TS00_FERR_F32 | TS00_X_F32 | TS00_F_NULL | TS00_POLY_ORD | TS00_CLIP_FIT, POLY_ORDER, NUM_DATA, false), "F32 tests: Ordinary polys, clip fit.  F Vector NULL");
    ok(genericTest(TS00_MASK_NULL | TS00_FERR_F32 | TS00_X_F32 | TS00_F_F32 | TS00_POLY_ORD | TS00_CLIP_FIT, POLY_ORDER, NUM_DATA, false), "F32 tests: Ordinary polys, clip fit.  F Vector NULL");
    // Mismatch vector types
    ok(genericTest(TS00_MASK_S32 | TS00_FERR_F32 | TS00_X_F32 | TS00_F_F32 | TS00_POLY_ORD | TS00_CLIP_FIT, POLY_ORDER, NUM_DATA, false), "F32 tests: Ordinary polys, clip fit.  Mismatch vector types");
    ok(genericTest(TS00_MASK_U8 | TS00_FERR_F64 | TS00_X_F32 | TS00_F_F32 | TS00_POLY_ORD | TS00_CLIP_FIT, POLY_ORDER, NUM_DATA, false), "F32 tests: Ordinary polys, clip fit.  Mismatch vector types");
    ok(genericTest(TS00_MASK_U8 | TS00_FERR_F32 | TS00_X_F64 | TS00_F_F32 | TS00_POLY_ORD | TS00_CLIP_FIT, POLY_ORDER, NUM_DATA, false), "F32 tests: Ordinary polys, clip fit.  Mismatch vector types");

    //
    // F64 tests: Ordinary polys, clip fit
    //
    // All Vectors non-NULL
    ok(genericTest(TS00_MASK_U8 | TS00_FERR_F64 | TS00_X_F64 | TS00_F_F64 | TS00_POLY_ORD
                   | TS00_CLIP_FIT, POLY_ORDER, NUM_DATA, true),
       "F64 tests: Ordinary polys, clip fit");
    // Some Vectors NULL
    //    ok(genericTest(TS00_MASK_U8 | TS00_FERR_NULL | TS00_X_F64 | TS00_F_F64 | TS00_POLY_ORD | TS00_CLIP_FIT, POLY_ORDER, NUM_DATA, true), "F64 tests: Ordinary polys, clip fit.  Some Vectors NULL");
    //    ok(genericTest(TS00_MASK_U8 | TS00_FERR_F64 | TS00_X_NULL | TS00_F_F64 | TS00_POLY_ORD | TS00_CLIP_FIT, POLY_ORDER, NUM_DATA, true), "F64 tests: Ordinary polys, clip fit.  Some Vectors NULL");
    // F-vector NULL
    ok(genericTest(TS00_MASK_U8 | TS00_FERR_F64 | TS00_X_F64 | TS00_F_NULL | TS00_POLY_ORD
                   | TS00_CLIP_FIT, POLY_ORDER, NUM_DATA, false),
       "F64 tests: Ordinary polys, clip fit.  F Vector NULL");
    ok(genericTest(TS00_MASK_NULL | TS00_FERR_F64 | TS00_X_F64 | TS00_F_F32 | TS00_POLY_ORD | TS00_CLIP_FIT, POLY_ORDER, NUM_DATA, false), "F64 tests: Ordinary polys, clip fit.  F Vector NULL");
    // Mismatch vector types
    ok(genericTest(TS00_MASK_S32 | TS00_FERR_F64 | TS00_X_F64 | TS00_F_F64 | TS00_POLY_ORD | TS00_CLIP_FIT, POLY_ORDER, NUM_DATA, false), "F64 tests: Ordinary polys, clip fit.  Mismatch vector types");
    ok(genericTest(TS00_MASK_U8 | TS00_FERR_F32 | TS00_X_F64 | TS00_F_F64 | TS00_POLY_ORD | TS00_CLIP_FIT, POLY_ORDER, NUM_DATA, false), "F64 tests: Ordinary polys, clip fit.  Mismatch vector types");
    ok(genericTest(TS00_MASK_U8 | TS00_FERR_F64 | TS00_X_F32 | TS00_F_F64 | TS00_POLY_ORD | TS00_CLIP_FIT, POLY_ORDER, NUM_DATA, false), "F64 tests: Ordinary polys, clip fit.  Mismatch vector types");


    //
    // F32 tests: Chebyshev polys, clip fit
    //
    // All Vectors non-NULL
    ok(genericTest(TS00_MASK_U8 | TS00_FERR_F32 | TS00_X_F32 | TS00_F_F32 | TS00_POLY_CHEB | TS00_CLIP_FIT, POLY_ORDER, NUM_DATA, true), "F32 tests: Chebyshev polys, clip fit");
    // Some Vectors NULL
    //    ok(genericTest(TS00_MASK_U8 | TS00_FERR_NULL | TS00_X_F32 | TS00_F_F32 | TS00_POLY_CHEB | TS00_CLIP_FIT, POLY_ORDER, NUM_DATA, true), "F32 tests: Chebyshev polys, clip fit.  Some Vectors NULL");
    //    ok(genericTest(TS00_MASK_U8 | TS00_FERR_F32 | TS00_X_NULL | TS00_F_F32 | TS00_POLY_CHEB | TS00_CLIP_FIT, POLY_ORDER, NUM_DATA, true), "F32 tests: Chebyshev polys, clip fit.  Some Vectors NULL");
    //    ok(genericTest(TS00_MASK_U8 | TS00_FERR_NULL | TS00_X_NULL | TS00_F_F32 | TS00_POLY_CHEB | TS00_CLIP_FIT, POLY_ORDER, NUM_DATA, true), "F32 tests: Chebyshev polys, clip fit.  Some Vectors NULL");
    // F-vector NULL
    ok(genericTest(TS00_MASK_U8 | TS00_FERR_F32 | TS00_X_F32 | TS00_F_NULL | TS00_POLY_CHEB | TS00_CLIP_FIT, POLY_ORDER, NUM_DATA, false), "F32 tests: Chebyshev polys, clip fit.  F Vector NULL");
    ok(genericTest(TS00_MASK_NULL | TS00_FERR_F32 | TS00_X_F32 | TS00_F_F32 | TS00_POLY_CHEB | TS00_CLIP_FIT, POLY_ORDER, NUM_DATA, false), "F32 tests: Chebyshev polys, clip fit.  F Vector NULL");
    // Mismatch vector types
    ok(genericTest(TS00_MASK_S32 | TS00_FERR_F32 | TS00_X_F32 | TS00_F_F32 | TS00_POLY_CHEB | TS00_CLIP_FIT, POLY_ORDER, NUM_DATA, false), "F32 tests: Chebyshev polys, clip fit.  Mismatch vector types");
    ok(genericTest(TS00_MASK_U8 | TS00_FERR_F64 | TS00_X_F32 | TS00_F_F32 | TS00_POLY_CHEB | TS00_CLIP_FIT, POLY_ORDER, NUM_DATA, false), "F32 tests: Chebyshev polys, clip fit.  Mismatch vector types");
    ok(genericTest(TS00_MASK_U8 | TS00_FERR_F32 | TS00_X_F64 | TS00_F_F32 | TS00_POLY_CHEB | TS00_CLIP_FIT, POLY_ORDER, NUM_DATA, false), "F32 tests: Chebyshev polys, clip fit.  Mismatch vector types");


    //
    // F64 tests: Chebyshev polys, clip fit
    //
    // All Vectors non-NULL
    ok(genericTest(TS00_MASK_U8 | TS00_FERR_F64 | TS00_X_F64 | TS00_F_F64 | TS00_POLY_CHEB | TS00_CLIP_FIT, POLY_ORDER, NUM_DATA, true), "F64 tests: Chebyshev polys, clip fit");
    // Some Vectors NULL
    //    ok(genericTest(TS00_MASK_U8 | TS00_FERR_NULL | TS00_X_F64 | TS00_F_F64 | TS00_POLY_CHEB | TS00_CLIP_FIT, POLY_ORDER, NUM_DATA, true), "F64 tests: Chebyshev polys, clip fit.  Some Vectors NULL");
    //    ok(genericTest(TS00_MASK_U8 | TS00_FERR_F64 | TS00_X_NULL | TS00_F_F64 | TS00_POLY_CHEB | TS00_CLIP_FIT, POLY_ORDER, NUM_DATA, true), "F64 tests: Chebyshev polys, clip fit.  Some Vectors NULL");
    // F-vector NULL
    ok(genericTest(TS00_MASK_U8 | TS00_FERR_F64 | TS00_X_F64 | TS00_F_NULL | TS00_POLY_CHEB | TS00_CLIP_FIT, POLY_ORDER, NUM_DATA, false), "F64 tests: Chebyshev polys, clip fit.  F Vector NULL");
    ok(genericTest(TS00_MASK_NULL | TS00_FERR_F64 | TS00_X_F64 | TS00_F_F32 | TS00_POLY_CHEB | TS00_CLIP_FIT, POLY_ORDER, NUM_DATA, false), "F64 tests: Chebyshev polys, clip fit.  F Vector NULL");
    // Mismatch vector types
    ok(genericTest(TS00_MASK_S32 | TS00_FERR_F64 | TS00_X_F64 | TS00_F_F64 | TS00_POLY_CHEB | TS00_CLIP_FIT, POLY_ORDER, NUM_DATA, false), "F64 tests: Chebyshev polys, clip fit.  Mismatch vector types");
    ok(genericTest(TS00_MASK_U8 | TS00_FERR_F32 | TS00_X_F64 | TS00_F_F64 | TS00_POLY_CHEB | TS00_CLIP_FIT, POLY_ORDER, NUM_DATA, false), "F64 tests: Chebyshev polys, clip fit.  Mismatch vector types");
    ok(genericTest(TS00_MASK_U8 | TS00_FERR_F64 | TS00_X_F32 | TS00_F_F64 | TS00_POLY_CHEB | TS00_CLIP_FIT, POLY_ORDER, NUM_DATA, false), "F64 tests: Chebyshev polys, clip fit.  Mismatch vector types");

    // Note: memory leaks tests are performed in the genericTest() subroutine.
}

