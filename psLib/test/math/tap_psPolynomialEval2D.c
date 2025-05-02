/** tap_psPolynomialEval2D.c
*
*  This test driver will exercise the psPolynomialXDEval functions for both
*  ORD and CHEB type polynomials.
*
*  @version  $Revision: 1.8 $  $Name: not supported by cvs2svn $
*  @date  $Date: 2008-05-05 00:09:04 $
*
* Copyright 2004-2005 Maui High Performance Computing Center, Univ. of Hawaii
*
***************************************************************************/
#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include "tap.h"
#include "pstap.h"

#define  TERMS        4
#define  TESTPOINTS   5
#define  ERROR_TOL    0.001

psF32 poly2DCoeff[TERMS][TERMS]   = {  { -4.3,  2.3, -3.2,  1.9},
                                       {  1.2,  0.7, -0.3,  1.3},
                                       {  0.4, -2.9,  0.8, -3.1},
                                       { -1.1,  2.1,  1.9,  0.6}
                                    };
psF64 Dpoly2DCoeff[TERMS][TERMS]  = {  { -4.3,  2.3, -3.2,  1.9},
                                       {  1.2,  0.7, -0.3,  1.3},
                                       {  0.4, -2.9,  0.8, -3.1},
                                       { -1.1,  2.1,  1.9,  0.6}
                                    };
psF32 poly2DMask[TERMS][TERMS]    = {  {  0,    0,    0,    1},
                                       {  0,    0,    1,    0},
                                       {  1,    0,    0,    0},
                                       {  0,    0,    0,    1}
                                    };

psF32 poly2DXYValue[TESTPOINTS][2] = {  {  1.40,  0.55},
                                        { -0.55,  1.40},
                                        {  0.00,  2.34},
                                        { -0.88,  0.00},
                                        {  3.45, -0.78}
                                     };
psF32 Dpoly2DXYValue[TESTPOINTS][2] = {  {  1.40,  0.55},
                                      { -0.55,  1.40},
                                      {  0.00,  2.34},
                                      { -0.88,  0.00},
                                      {  3.45, -0.78}
                                      };
psF32 poly2DResult[TESTPOINTS] = {  -3.415938, -14.765687, -16.43992, -4.606381, -22.650702 };
psF64 Dpoly2DResult[TESTPOINTS] = {  -3.415938, -14.765687, -16.43992, -4.606381, -22.650702 };

psF32 poly2DXYChebValue[TESTPOINTS][2] = {  {  0.500,  0.500},
        {  0.000,  0.250},
        { -0.250,  0.000},
        {  0.990,  0.150},
        {  0.333, -0.666}
                                         };
psF32 Dpoly2DXYChebValue[TESTPOINTS][2] = {  {  0.500,  0.500},
        {  0.000,  0.250},
        { -0.250,  0.000},
        {  0.990,  0.150},
        {  0.333, -0.666}
                                          };
psF32 poly2DChebResult[TESTPOINTS] = {  0.750000, 1.687500, 0.625000, -0.113040, 0.386786 };
psF32 Dpoly2DChebResult[TESTPOINTS] = {  0.750000, 1.687500, 0.625000, -0.113040, 0.386786 };

psS32 main(psS32 argc, char* argv[])
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    plan_tests(38);


    // Evaluate a NULL polynomial
    {
        psMemId id = psMemGetId();
        psF64 result = psPolynomial2DEval(NULL, 0.0, 0.0);
        ok(isnan(result), "psPolynomial2DEval() returned NAN with NULL psPolynomial");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Allocate and evaluate an ordinary polynomial structure
    {
        psMemId id = psMemGetId();

        // Allocate polynomial structure
        psPolynomial2D*  polyOrd = psPolynomial2DAlloc(PS_POLYNOMIAL_ORD, TERMS-1, TERMS-1);
        ok(polyOrd != NULL, "Ordinary psPolynomial2D successfully allocated");
        skip_start(polyOrd == NULL, 1, "Skipping tests because psPolynomial2DAlloc() failed");

        // Set polynomial members
        for(psS32 i = 0; i < TERMS; i++)
        {
            for(psS32 j = 0; j < TERMS; j++) {
                polyOrd->coeff[i][j] = poly2DCoeff[i][j];
                polyOrd->coeffMask[i][j]  = poly2DMask[i][j];
            }
        }

        // Evaluate test points and verify results
        bool errorFlag = false;
        for(psS32 i = 0; i < TESTPOINTS; i++)
        {
            psF64 result = psPolynomial2DEval(polyOrd,Dpoly2DXYValue[i][0],Dpoly2DXYValue[i][1]);
            if (fabs(Dpoly2DResult[i]-result) > ERROR_TOL ) {
                diag("TEST ERROR: Evaluated value %f, should be %f\n", result, Dpoly2DResult[i]);
                errorFlag = true;
            }
        }
        ok(!errorFlag, "psPolynomial2DEval() successful (Ordinary)");
        skip_end();
        psFree(polyOrd);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Allocate and evaluate Cheby polynomial structure
    {
        psMemId id = psMemGetId();

        // Allocate polynomial structure
        psPolynomial2D*  polyCheb = psPolynomial2DAlloc(PS_POLYNOMIAL_CHEB, TERMS-1, TERMS-1);
        ok(polyCheb != NULL, "Cheby psPolynomial2D successfully allocated");
        skip_start(polyCheb == NULL, 1, "Skipping tests because psPolynomial2DAlloc() failed");

        // Set polynomial members
        for(psS32 i = 0; i < TERMS; i++)
        {
            for(psS32 j = 0; j < TERMS; j++) {
                polyCheb->coeff[i][j] = 1.0;
                polyCheb->coeffMask[i][j]  = poly2DMask[i][j];
            }
        }

        // Evaluate test points and verify results
        bool errorFlag = false;
        for(psS32 i = 0; i < TESTPOINTS; i++)
        {
            psF64 resultCheb = psPolynomial2DEval(polyCheb,Dpoly2DXYChebValue[i][0],Dpoly2DXYChebValue[i][1]);
            if (fabs(Dpoly2DChebResult[i]-resultCheb) > ERROR_TOL ) {
                diag("TEST ERROR: Evaluated value %f, should be %f\n", resultCheb, Dpoly2DChebResult[i]);
                errorFlag = true;
            }
        }
        ok(!errorFlag, "psPolynomial2DEval() successful (Chebyshev)");
        skip_end();
        psFree(polyCheb);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Allocate polynomial, test the psPolynomial2DEvalVector() routines
    {
        psMemId id = psMemGetId();
        // Create input vectors
        psVector* inputOrdX  = psVectorAlloc(TESTPOINTS, PS_TYPE_F64);
        psVector* inputOrdY  = psVectorAlloc(TESTPOINTS, PS_TYPE_F64);
        for(psS32 i = 0; i < TESTPOINTS; i++)
        {
            inputOrdX->data.F64[i]  = poly2DXYValue[i][0];
            inputOrdY->data.F64[i]  = poly2DXYValue[i][1];
            inputOrdX->n++;
            inputOrdY->n++;
        }

        psPolynomial2D* polyOrd = psPolynomial2DAlloc(PS_POLYNOMIAL_ORD, TERMS-1, TERMS-1);
        ok(polyOrd != NULL, "Ordinary polynomial allocation successful");
        skip_start(polyOrd == NULL, 8, "Skipping tests because psPolynomial2DAlloc() failed");
        // Set polynomial members
        for(psS32 i = 0; i < TERMS; i++)
        {
            for(psS32 j = 0; j < TERMS; j++) {
                polyOrd->coeff[i][j] = poly2DCoeff[i][j];
                polyOrd->coeffMask[i][j]  = poly2DMask[i][j];
            }
        }

        // Evaluate the vectors
        psVector* outputOrd = psPolynomial2DEvalVector(polyOrd, inputOrdX, inputOrdY);
        ok(outputOrd != NULL, "psPolynomial2DEvalVector() generated non-NULL psVector");
        ok(outputOrd->type.type == PS_TYPE_F64, "psPolynomial2DEvalVector() generated correct type");

        // Verify the results
        bool errorFlag = false;
        for(psS32 i = 0; i < TESTPOINTS; i++)
        {
            if (fabs(poly2DResult[i]-outputOrd->data.F64[i]) > ERROR_TOL) {
                diag("TEST ERROR: Result[%d] %lg not equal to expected %lg.\n",
                     i, outputOrd->data.F64[i], poly2DResult[i]);
                errorFlag = true;
            }
        }
        ok(!errorFlag, "psPolynomial2DEvalVector() produced the correct answers");


        // Attempt to invoke function with NULL polynomial
        {
            psMemId id = psMemGetId();
            ok(psPolynomial2DEvalVector(NULL, inputOrdX, inputOrdY) == NULL, "psPolynomial2DEvalVector() produced NULL when called with NULL polynomial");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }


        // Attempt to invoke function with NULL input vector
        {
            psMemId id = psMemGetId();
            ok(psPolynomial2DEvalVector(polyOrd,NULL,inputOrdY) == NULL, "psPolynomial2DEvalVector() produced NULL when called with NULL input vector");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }

    
        // Attempt to invoke function with NULL input vector
        {
            psMemId id = psMemGetId();
            ok(psPolynomial2DEvalVector(polyOrd,inputOrdX,NULL) == NULL, "psPolynomial2DEvalVector() produced NULL when called with NULL input vector");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }

    
        // Attempt to invoke function with a non F64 type input vector
        {
            psMemId id = psMemGetId();
            inputOrdX->type.type = PS_TYPE_U8;
            ok(psPolynomial2DEvalVector(polyOrd,inputOrdX,inputOrdY) == NULL, "psPolynomial2DEvalVector() produced NULL when called with non F64 input vector");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }

        inputOrdX->type.type = PS_TYPE_F64;
        // Attempt to invoke function with a non F64 type input vector
        {
            psMemId id = psMemGetId();
            inputOrdY->type.type = PS_TYPE_U8;
            ok(psPolynomial2DEvalVector(polyOrd,inputOrdX, inputOrdY) == NULL, "psPolynomial2DEvalVector() produced NULL when called with non F64 input vector");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }
        psFree(outputOrd);
        skip_end();
        psFree(inputOrdX);
        psFree(inputOrdY);
        psFree(polyOrd);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");

    }


    // Allocate polynomial, test the psPolynomial2DEvalVector() routines
    {
        psMemId id = psMemGetId();
        // Create input vectors
        psVector* inputChebX = psVectorAlloc(TESTPOINTS, PS_TYPE_F64);
        psVector* inputChebY = psVectorAlloc(TESTPOINTS, PS_TYPE_F64);
        for(psS32 i = 0; i < TESTPOINTS; i++)
        {
            inputChebX->data.F64[i] = poly2DXYChebValue[i][0];
            inputChebY->data.F64[i] = poly2DXYChebValue[i][1];
            inputChebX->n++;
            inputChebY->n++;
        }

        psPolynomial2D* polyCheb = psPolynomial2DAlloc(PS_POLYNOMIAL_CHEB, TERMS-1, TERMS-1);
        ok(polyCheb != NULL, "Cheby polynomial allocation successful");
        skip_start(polyCheb == NULL, 8, "Skipping tests because psPolynomial2DAlloc() failed");
        // Set polynomial members
        for(psS32 i = 0; i < TERMS; i++)
        {
            for(psS32 j = 0; j < TERMS; j++) {
                polyCheb->coeff[i][j] = 1.0;
                polyCheb->coeffMask[i][j]  = poly2DMask[i][j];
            }
        }

        // Evaluate the vectors
        psVector* outputCheb = psPolynomial2DEvalVector(polyCheb, inputChebX, inputChebY);
        ok(outputCheb != NULL, "psPolynomial2DEvalVector() generated non-NULL psVector");
        ok(outputCheb->type.type == PS_TYPE_F64, "psPolynomial2DEvalVector() generated correct type");


        // Verify the results
        bool errorFlag = false;
        for(psS32 i = 0; i < TESTPOINTS; i++)
        {
            if (fabs(poly2DChebResult[i]-outputCheb->data.F64[i]) > ERROR_TOL) {
                diag("TEST ERROR: ResultCheb[%d] %lg not equal to expected %lg.\n",
                     i, outputCheb->data.F64[i], poly2DChebResult[i]);
                errorFlag = true;
            }
        }
        ok(!errorFlag, "psPolynomial2DEvalVector() produced the correct answers");

        // Attempt to invoke function with NULL polynomial
        {
            psMemId id = psMemGetId();
            ok(psPolynomial2DEvalVector(NULL, inputChebX, inputChebY) == NULL, "psPolynomial2DEvalVector() produced NULL when called with NULL polynomial");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }


        // Attempt to invoke function with NULL input vector
        {
            psMemId id = psMemGetId();
            ok(psPolynomial2DEvalVector(polyCheb,NULL,inputChebY) == NULL, "psPolynomial2DEvalVector() produced NULL when called with NULL input vector");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }


        // Attempt to invoke function with NULL input vector
        {
            psMemId id = psMemGetId();
            ok(psPolynomial2DEvalVector(polyCheb,inputChebX,NULL) == NULL, "psPolynomial2DEvalVector() produced NULL when called with NULL input vector");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }


        // Attempt to invoke function with a non F64 type input vector
        {
            psMemId id = psMemGetId();
            inputChebX->type.type = PS_TYPE_U8;
            ok(psPolynomial2DEvalVector(polyCheb,inputChebX,inputChebY) == NULL, "psPolynomial2DEvalVector() produced NULL when called with non F64 input vector");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }


        inputChebX->type.type = PS_TYPE_F64;

        // Attempt to invoke function with a non F64 type input vector
        {
            psMemId id = psMemGetId();
            inputChebY->type.type = PS_TYPE_U8;
            ok(psPolynomial2DEvalVector(polyCheb,inputChebX, inputChebY) == NULL, "psPolynomial2DEvalVector() produced NULL when called with non F64 input vector");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }
        psFree(outputCheb);
        skip_end();
        psFree(inputChebX);
        psFree(inputChebY);
        psFree(polyCheb);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
}
