/**
*
*  This test driver will exercise the psPolynomialXDEval functions for both
*  ORD and CHEB type polynomials.
*
*  @version  $Revision: 1.10 $  $Name: not supported by cvs2svn $
*  @date  $Date: 2008-05-05 00:09:04 $
*
*  XXX: Probably should test single- and multi-dimensional polynomials in
*  which one diminsion is constant (n == 1).
*
*  XXX: define ORDERS, not TERMS
*
*
* Copyright 2004-2005 Maui High Performance Computing Center, Univ. of Hawaii
*
***************************************************************************/
#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include "tap.h"
#include "pstap.h"

#define TERMS        4
#define TESTPOINTS   5
#define ERROR_TOL    0.001

psF32 poly1DCoeff[TERMS]        = { -4.3, 2.3, -3.2, 1.9};
psF64 Dpoly1DCoeff[TERMS]        = { -4.3, 2.3, -3.2, 1.9};
psF32 poly1DMask[TERMS]         = {    0,   0,    1,   0  };

psF32 poly1DXValue[TESTPOINTS]   = { 2.550000,    -9.8000,   0.00134,   -12.2500,   0.000};
psF64 Dpoly1DXValue[TESTPOINTS]  = { 2.550000,    -9.8000,   0.00134,   -12.2500,   0.000};
psF32 poly1DXResult[TESTPOINTS]  = {33.069613, -1815.1048, -4.296918, -3525.1797, -4.3000};
psF64 Dpoly1DXResult[TESTPOINTS] = {33.069613, -1815.1048, -4.296918, -3525.1797, -4.3000};

psF32 poly1DXChebValue[TESTPOINTS]   = { -0.99,    -0.33,     0.125,    0.564,    0.875};
psF64 Dpoly1DXChebValue[TESTPOINTS]  = { -0.99,    -0.33,     0.125,    0.564,    0.875};
psF32 poly1DXChebResult[TESTPOINTS]  = { -1.401196, 1.016252, 0.257813, 0.089625, 1.429688};
psF64 Dpoly1DXChebResult[TESTPOINTS] = { -1.401196, 1.016252, 0.257813, 0.089625, 1.429688};

psS32 main(psS32 argc, char* argv[])
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    plan_tests(28);


    // Evaluate a NULL polynomial
    {
        psMemId id = psMemGetId();
        psF64 result = psPolynomial1DEval(NULL, 0.0);
        ok(isnan(result), "psPolynomial1DEval() returned NAN with NULL psPolynomial");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Allocate and evaluate an ordinary polynomial structure
    {
        psMemId id = psMemGetId();

        // Allocate polynomial structure
        psPolynomial1D* polyOrd = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, TERMS-1);
        ok(polyOrd != NULL, "Ordinary psPolynomial1D successfully allocated");
        skip_start(polyOrd == NULL, 1, "Skipping tests because psPolynomial1DAlloc() failed");

        // Set polynomial members
        for(psS32 i = 0; i < TERMS; i++) {
            polyOrd->coeff[i] = poly1DCoeff[i];
            polyOrd->coeffMask[i]  = poly1DMask[i];
        }

        // Evaluate test points and verify results
        bool errorFlag = false;
        for(psS32 i = 0; i < TESTPOINTS; i++) {
            psF64 result = psPolynomial1DEval(polyOrd,poly1DXValue[i]);
            if (fabs(poly1DXResult[i]-result) > ERROR_TOL ) {
                diag("Evaluated value %g not as expected %g", result, poly1DXResult[i]);
                errorFlag = true;
            }
        }
        ok(!errorFlag, "psPolynomial1DEval() successful (Ordinary)");
        skip_end();
        psFree(polyOrd);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Allocate Cheby polynomial structure
    {
        bool errorFlag = false;
        psMemId id = psMemGetId();
        psPolynomial1D*  polyCheb = psPolynomial1DAlloc(PS_POLYNOMIAL_CHEB, TERMS-1);
        ok(polyCheb != NULL, "Chebyshev psPolynomial1D successfully allocated");
        skip_start(polyCheb == NULL, 1, "Skipping tests because psPolynomial1DAlloc() failed");
        // Set polynomial members
        for(psS32 i = 0; i < TERMS; i++)
        {
            polyCheb->coeff[i] = 1.0;
            polyCheb->coeffMask[i]  = poly1DMask[i];
        }
        // Evaluate test points and verify results
        for(psS32 i = 0; i < TESTPOINTS; i++)
        {
            psF64 resultCheb = psPolynomial1DEval(polyCheb,poly1DXChebValue[i]);
            if (fabs(poly1DXChebResult[i]-resultCheb) > ERROR_TOL ) {
                diag("Evaluated value %g not as expected %g", resultCheb, poly1DXChebResult[i]);
                errorFlag = true;
            }
        }
        ok(!errorFlag, "psPolynomial1DEval() successful (Chebyshev)");
        skip_end();
        psFree(polyCheb);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Allocate polynomial, test the psPolynomial1DEvalVector() routines
    {
        psMemId id = psMemGetId();
        // Create input vectors
        psVector* inputOrd = psVectorAlloc(TESTPOINTS, PS_TYPE_F64);
        for(psS32 i = 0; i < TESTPOINTS; i++)
        {
            inputOrd->data.F64[i] = poly1DXValue[i];
            inputOrd->n++;
        }

        psPolynomial1D* polyOrd = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, TERMS-1);
        ok(polyOrd != NULL, "Ordinary polynomial allocation successful");
        skip_start(polyOrd == NULL, 6, "Skipping tests because psPolynomial1DAlloc() failed");
        // Set polynomial members
        for(psS32 i = 0; i < TERMS; i++)
        {
            polyOrd->coeff[i] = poly1DCoeff[i];
            polyOrd->coeffMask[i]  = poly1DMask[i];
        }

        // Evaluate the vectors
        psVector* outputOrd = psPolynomial1DEvalVector(polyOrd, inputOrd);
        ok(outputOrd != NULL, "psPolynomial1DEvalVector() generated non-NULL psVector");
        ok(outputOrd->type.type == PS_TYPE_F64, "psPolynomial1DEvalVector() generated correct output type");

        // Verify the results
        bool errorFlag = false;
        for(psS32 i = 0; i < TESTPOINTS; i++)
        {
            if (fabs(poly1DXResult[i]-outputOrd->data.F64[i]) > ERROR_TOL) {
                diag("Result[%d] %lg not equal to expected %lg",
                     i, outputOrd->data.F64[i], poly1DXResult[i]);
                errorFlag = TRUE;
            }
        }
        ok(!errorFlag, "psPolynomial1DEvalVector() produced the correct answers");

        // Attempt to invoke function with NULL polynomial
        {
            psMemId id = psMemGetId();
            ok(psPolynomial1DEvalVector(NULL, inputOrd) == NULL, "psPolynomial1DEvalVector() produced NULL when called with NULL polynomial");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }

        // Attempt to invoke function with NULL input vector
        {
            psMemId id = psMemGetId();
            ok(psPolynomial1DEvalVector(polyOrd, NULL) == NULL, "psPolynomial1DEvalVector() produced NULL when called with NULL input vector");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }

        // Attempt to invoke function with a non F64 type input vector
        {
            psMemId id = psMemGetId();
            inputOrd->type.type = PS_TYPE_U8;
            ok(psPolynomial1DEvalVector(polyOrd,inputOrd) == NULL, "psPolynomial1DEvalVector() produced NULL when called with non F64 input vector");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }
        psFree(outputOrd);
        skip_end();
        psFree(inputOrd);
        psFree(polyOrd);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Allocate polynomial, test the psPolynomial1DEvalVector() routines (Chebyshev)
    {
        psMemId id = psMemGetId();
        // Create input vectors
        psVector* inputCheb = psVectorAlloc(TESTPOINTS, PS_TYPE_F64);
        for(psS32 i = 0; i < TESTPOINTS; i++)
        {
            inputCheb->data.F64[i] = poly1DXChebValue[i];
            inputCheb->n++;
        }

        psPolynomial1D* polyCheb = psPolynomial1DAlloc(PS_POLYNOMIAL_CHEB, TERMS-1);
        ok(polyCheb != NULL, "Ordinary polynomial allocation successful");
        skip_start(polyCheb == NULL, 5, "Skipping tests because psPolynomial1DAlloc() failed");
        // Set polynomial members
        for(psS32 i = 0; i < TERMS; i++)
        {
            polyCheb->coeff[i] = 1.0;
            polyCheb->coeffMask[i]  = poly1DMask[i];
        }

        // Evaluate the vectors
        psVector* outputCheb = psPolynomial1DEvalVector(polyCheb, inputCheb);
        ok(outputCheb != NULL, "psPolynomial1DEvalVector() generated non-NULL psVector");
        ok(outputCheb->type.type == PS_TYPE_F64, "psPolynomial1DEvalVector() generated correct output type");

        // Verify the results
        bool errorFlag = false;
        for(psS32 i = 0; i < TESTPOINTS; i++)
        {
            if (fabs(poly1DXChebResult[i]-outputCheb->data.F64[i]) > ERROR_TOL) {
                diag("ResultCheb[%d] %lg not equal to expected %lg",
                     i, outputCheb->data.F64[i], poly1DXChebResult[i]);
                errorFlag = TRUE;
            }
        }
        ok(!errorFlag, "psPolynomial1DEvalVector() produced the correct answers");


        // Attempt to invoke function with NULL input vector
        {
            psMemId id = psMemGetId();
            ok(psPolynomial1DEvalVector(polyCheb,NULL) == NULL, "psPolynomial1DEvalVector() produced NULL when called with NULL input vector");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }


        // Attempt to invoke function with a non F64 type input vector
        {
            psMemId id = psMemGetId();
            inputCheb->type.type = PS_TYPE_U8;
            ok(psPolynomial1DEvalVector(polyCheb,inputCheb) == NULL, "psPolynomial1DEvalVector() produced NULL when called with non F64 input vector");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }


        psFree(outputCheb);
        skip_end();
        psFree(inputCheb);
        psFree(polyCheb);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
    return exit_status();
}
