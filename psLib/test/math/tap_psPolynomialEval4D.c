/** tst_psFunc11.c
*
*  This test driver will exercise the psPolynomialXDEval functions for both
*  ORD and CHEB type polynomials.
*
*  @version  $Revision: 1.7 $  $Name: not supported by cvs2svn $
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

psF32 poly4DCoeff[TERMS][TERMS][TERMS][TERMS] = {
            {
                {
                    { -1.1,  1.2, -1.3,  1.4},
                    {  1.5, -1.6,  1.7, -1.8},
                    {  0.1, -0.2,  0.3, -0.4},
                    { -0.5,  0.6, -0.7,  0.8}
                },
                {
                    { -2.1,  2.2, -2.3,  2.4},
                    {  2.5, -2.6,  2.7, -2.8},
                    {  3.1, -3.2,  3.3, -3.4},
                    { -3.5,  3.6, -3.7,  3.8}
                },
                { { -4.1,  4.2, -4.3,  4.4},
                  {  4.5, -4.6,  4.7, -4.8},
                  {  5.1, -5.2,  5.3, -5.4},
                  { -5.5,  5.6, -5.7,  5.8}
                },
                { { -6.1,  6.2, -6.3,  6.4},
                  {  6.5, -6.6,  6.7, -6.8},
                  {  7.1, -7.2,  7.3, -7.4},
                  { -7.5,  7.6, -7.7,  7.8}
                }
            },
            {
                {
                    { -1.1,  1.2, -1.3,  1.4},
                    {  1.5, -1.6,  1.7, -1.8},
                    {  0.1, -0.2,  0.3, -0.4},
                    { -0.5,  0.6, -0.7,  0.8}
                },
                {
                    { -2.1,  2.2, -2.3,  2.4},
                    {  2.5, -2.6,  2.7, -2.8},
                    {  3.1, -3.2,  3.3, -3.4},
                    { -3.5,  3.6, -3.7,  3.8}
                },
                { { -4.1,  4.2, -4.3,  4.4},
                  {  4.5, -4.6,  4.7, -4.8},
                  {  5.1, -5.2,  5.3, -5.4},
                  { -5.5,  5.6, -5.7,  5.8}
                },
                { { -6.1,  6.2, -6.3,  6.4},
                  {  6.5, -6.6,  6.7, -6.8},
                  {  7.1, -7.2,  7.3, -7.4},
                  { -7.5,  7.6, -7.7,  7.8}
                }
            },
            {
                {
                    { -1.1,  1.2, -1.3,  1.4},
                    {  1.5, -1.6,  1.7, -1.8},
                    {  0.1, -0.2,  0.3, -0.4},
                    { -0.5,  0.6, -0.7,  0.8}
                },
                {
                    { -2.1,  2.2, -2.3,  2.4},
                    {  2.5, -2.6,  2.7, -2.8},
                    {  3.1, -3.2,  3.3, -3.4},
                    { -3.5,  3.6, -3.7,  3.8}
                },
                { { -4.1,  4.2, -4.3,  4.4},
                  {  4.5, -4.6,  4.7, -4.8},
                  {  5.1, -5.2,  5.3, -5.4},
                  { -5.5,  5.6, -5.7,  5.8}
                },
                { { -6.1,  6.2, -6.3,  6.4},
                  {  6.5, -6.6,  6.7, -6.8},
                  {  7.1, -7.2,  7.3, -7.4},
                  { -7.5,  7.6, -7.7,  7.8}
                }
            },
            {
                {
                    { -1.1,  1.2, -1.3,  1.4},
                    {  1.5, -1.6,  1.7, -1.8},
                    {  0.1, -0.2,  0.3, -0.4},
                    { -0.5,  0.6, -0.7,  0.8}
                },
                {
                    { -2.1,  2.2, -2.3,  2.4},
                    {  2.5, -2.6,  2.7, -2.8},
                    {  3.1, -3.2,  3.3, -3.4},
                    { -3.5,  3.6, -3.7,  3.8}
                },
                { { -4.1,  4.2, -4.3,  4.4},
                  {  4.5, -4.6,  4.7, -4.8},
                  {  5.1, -5.2,  5.3, -5.4},
                  { -5.5,  5.6, -5.7,  5.8}
                },
                { { -6.1,  6.2, -6.3,  6.4},
                  {  6.5, -6.6,  6.7, -6.8},
                  {  7.1, -7.2,  7.3, -7.4},
                  { -7.5,  7.6, -7.7,  7.8}
                }
            }
        };
psF64 Dpoly4DCoeff[TERMS][TERMS][TERMS][TERMS] = {
            {
                {
                    { -1.1,  1.2, -1.3,  1.4},
                    {  1.5, -1.6,  1.7, -1.8},
                    {  0.1, -0.2,  0.3, -0.4},
                    { -0.5,  0.6, -0.7,  0.8}
                },
                {
                    { -2.1,  2.2, -2.3,  2.4},
                    {  2.5, -2.6,  2.7, -2.8},
                    {  3.1, -3.2,  3.3, -3.4},
                    { -3.5,  3.6, -3.7,  3.8}
                },
                { { -4.1,  4.2, -4.3,  4.4},
                  {  4.5, -4.6,  4.7, -4.8},
                  {  5.1, -5.2,  5.3, -5.4},
                  { -5.5,  5.6, -5.7,  5.8}
                },
                { { -6.1,  6.2, -6.3,  6.4},
                  {  6.5, -6.6,  6.7, -6.8},
                  {  7.1, -7.2,  7.3, -7.4},
                  { -7.5,  7.6, -7.7,  7.8}
                }
            },
            {
                {
                    { -1.1,  1.2, -1.3,  1.4},
                    {  1.5, -1.6,  1.7, -1.8},
                    {  0.1, -0.2,  0.3, -0.4},
                    { -0.5,  0.6, -0.7,  0.8}
                },
                {
                    { -2.1,  2.2, -2.3,  2.4},
                    {  2.5, -2.6,  2.7, -2.8},
                    {  3.1, -3.2,  3.3, -3.4},
                    { -3.5,  3.6, -3.7,  3.8}
                },
                { { -4.1,  4.2, -4.3,  4.4},
                  {  4.5, -4.6,  4.7, -4.8},
                  {  5.1, -5.2,  5.3, -5.4},
                  { -5.5,  5.6, -5.7,  5.8}
                },
                { { -6.1,  6.2, -6.3,  6.4},
                  {  6.5, -6.6,  6.7, -6.8},
                  {  7.1, -7.2,  7.3, -7.4},
                  { -7.5,  7.6, -7.7,  7.8}
                }
            },
            {
                {
                    { -1.1,  1.2, -1.3,  1.4},
                    {  1.5, -1.6,  1.7, -1.8},
                    {  0.1, -0.2,  0.3, -0.4},
                    { -0.5,  0.6, -0.7,  0.8}
                },
                {
                    { -2.1,  2.2, -2.3,  2.4},
                    {  2.5, -2.6,  2.7, -2.8},
                    {  3.1, -3.2,  3.3, -3.4},
                    { -3.5,  3.6, -3.7,  3.8}
                },
                { { -4.1,  4.2, -4.3,  4.4},
                  {  4.5, -4.6,  4.7, -4.8},
                  {  5.1, -5.2,  5.3, -5.4},
                  { -5.5,  5.6, -5.7,  5.8}
                },
                { { -6.1,  6.2, -6.3,  6.4},
                  {  6.5, -6.6,  6.7, -6.8},
                  {  7.1, -7.2,  7.3, -7.4},
                  { -7.5,  7.6, -7.7,  7.8}
                }
            },
            {
                {
                    { -1.1,  1.2, -1.3,  1.4},
                    {  1.5, -1.6,  1.7, -1.8},
                    {  0.1, -0.2,  0.3, -0.4},
                    { -0.5,  0.6, -0.7,  0.8}
                },
                {
                    { -2.1,  2.2, -2.3,  2.4},
                    {  2.5, -2.6,  2.7, -2.8},
                    {  3.1, -3.2,  3.3, -3.4},
                    { -3.5,  3.6, -3.7,  3.8}
                },
                { { -4.1,  4.2, -4.3,  4.4},
                  {  4.5, -4.6,  4.7, -4.8},
                  {  5.1, -5.2,  5.3, -5.4},
                  { -5.5,  5.6, -5.7,  5.8}
                },
                { { -6.1,  6.2, -6.3,  6.4},
                  {  6.5, -6.6,  6.7, -6.8},
                  {  7.1, -7.2,  7.3, -7.4},
                  { -7.5,  7.6, -7.7,  7.8}
                }
            }
        };

psF32 poly4DMask[TERMS][TERMS][TERMS][TERMS]    = {
            {
                {
                    {  0,    0,    0,    1},
                    {  0,    0,    1,    0},
                    {  1,    0,    0,    0},
                    {  0,    0,    0,    1}
                },
                {
                    {  1,    0,    0,    0},
                    {  1,    0,    0,    0},
                    {  1,    0,    0,    0},
                    {  1,    0,    0,    0}
                },
                {
                    {  0,    1,    0,    0},
                    {  0,    0,    1,    0},
                    {  0,    1,    0,    0},
                    {  0,    0,    1,    0}
                },
                {
                    {  1,    0,    0,    0},
                    {  0,    0,    0,    1},
                    {  1,    0,    0,    0},
                    {  0,    0,    0,    1}
                }
            },
            {
                {
                    {  0,    0,    0,    1},
                    {  0,    0,    1,    0},
                    {  1,    0,    0,    0},
                    {  0,    0,    0,    1}
                },
                {
                    {  1,    0,    0,    0},
                    {  1,    0,    0,    0},
                    {  1,    0,    0,    0},
                    {  1,    0,    0,    0}
                },
                {
                    {  0,    1,    0,    0},
                    {  0,    0,    1,    0},
                    {  0,    1,    0,    0},
                    {  0,    0,    1,    0}
                },
                {
                    {  1,    0,    0,    0},
                    {  0,    0,    0,    1},
                    {  1,    0,    0,    0},
                    {  0,    0,    0,    1}
                }
            },
            {
                {
                    {  0,    0,    0,    1},
                    {  0,    0,    1,    0},
                    {  1,    0,    0,    0},
                    {  0,    0,    0,    1}
                },
                {
                    {  1,    0,    0,    0},
                    {  1,    0,    0,    0},
                    {  1,    0,    0,    0},
                    {  1,    0,    0,    0}
                },
                {
                    {  0,    1,    0,    0},
                    {  0,    0,    1,    0},
                    {  0,    1,    0,    0},
                    {  0,    0,    1,    0}
                },
                {
                    {  1,    0,    0,    0},
                    {  0,    0,    0,    1},
                    {  1,    0,    0,    0},
                    {  0,    0,    0,    1}
                }
            },
            {
                {
                    {  0,    0,    0,    1},
                    {  0,    0,    1,    0},
                    {  1,    0,    0,    0},
                    {  0,    0,    0,    1}
                },
                {
                    {  1,    0,    0,    0},
                    {  1,    0,    0,    0},
                    {  1,    0,    0,    0},
                    {  1,    0,    0,    0}
                },
                {
                    {  0,    1,    0,    0},
                    {  0,    0,    1,    0},
                    {  0,    1,    0,    0},
                    {  0,    0,    1,    0}
                },
                {
                    {  1,    0,    0,    0},
                    {  0,    0,    0,    1},
                    {  1,    0,    0,    0},
                    {  0,    0,    0,    1}
                }
            }
        };

psF32 poly4DWXYZValue[TESTPOINTS][4] = {
                                           {  0.450, -0.780,  0.500, -0.123},
                                           {  0.297,  0.153, -0.354,  0.000},
                                           {  0.000,  0.153, -0.354,  0.321},
                                           {  0.297,  0.000, -0.354,  0.321},
                                           {  0.297,  0.153,  0.000,  0.321}
                                       };
psF64 Dpoly4DWXYZValue[TESTPOINTS][4] = {
                                            {  0.450, -0.780,  0.500, -0.123},
                                            {  0.297,  0.153, -0.354,  0.000},
                                            {  0.000,  0.153, -0.354,  0.321},
                                            {  0.297,  0.000, -0.354,  0.321},
                                            {  0.297,  0.153,  0.000,  0.321}
                                        };

psF32 poly4DResult[TESTPOINTS]  = { -3.588753, -2.439566, -1.175955, -1.645497, -1.216915};
psF64 Dpoly4DResult[TESTPOINTS]  = { -3.588753, -2.439566, -1.175955, -1.645497, -1.216915};

psF32 poly4DWXYZChebValue[TESTPOINTS][4] = {
            {  0.100,  0.000,  0.250, -0.250},
            {  0.100, -0.250,  0.000,  0.250},
            {  0.100,  0.250, -0.250,  0.000},
            {  0.300,  0.200, -0.300, -0.400},
            { -0.780,  0.990, -0.010,  0.500}
        };
psF64 Dpoly4DWXYZChebValue[TESTPOINTS][4] = {
            {  0.100,  0.000,  0.250, -0.250},
            {  0.100, -0.250,  0.000,  0.250},
            {  0.100,  0.250, -0.250,  0.000},
            {  0.300,  0.200, -0.300, -0.400},
            { -0.780,  0.990, -0.010,  0.500}
        };
psF32 poly4DChebResult[TESTPOINTS]   = { -0.216563, -0.297000, -0.033000, 0.432198, 1.785601 };
psF64 Dpoly4DChebResult[TESTPOINTS]  = { -0.216563, -0.297000, -0.033000, 0.432198, 1.785601 };

psS32 main(psS32 argc, char* argv[])
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    plan_tests(54);


    // Evaluate NULL polynomial
    {
        psMemId id = psMemGetId();
        psF64 result = psPolynomial4DEval(NULL, 0.0, 0.0, 0.0, 0.0);
        ok(isnan(result), "psPolynomial4DEval() returned NAN with unallowed type");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Allocate and evaluate an ordinary polynomial structure
    {
        psMemId id = psMemGetId();
        // Allocate polynomial structure
        psPolynomial4D*  polyOrd = psPolynomial4DAlloc(PS_POLYNOMIAL_ORD, TERMS-1, TERMS-1, TERMS-1, TERMS-1);
        ok(polyOrd != NULL, "Ordinary psPolynomial4D successfully allocated");
        skip_start(polyOrd == NULL, 1, "Skipping tests because psPolynomial4DAlloc() failed");
        // Set polynomial members
        for(psS32 i = 0; i < TERMS; i++)
        {
            for(psS32 j = 0; j < TERMS; j++) {
                for(psS32 k = 0; k < TERMS; k++) {
                    for(psS32 l = 0; l < TERMS; l++) {
                        polyOrd->coeff[i][j][k][l] = Dpoly4DCoeff[i][j][k][l];
                        polyOrd->coeffMask[i][j][k][l]  = poly4DMask[i][j][k][l];
                    }
                }
            }
        }

        // Evaluate test points and verify results
        bool errorFlag = false;
        for(psS32 i = 0; i < TESTPOINTS; i++)
        {
            psF64 result = psPolynomial4DEval(polyOrd, Dpoly4DWXYZValue[i][0], Dpoly4DWXYZValue[i][1],
                                              Dpoly4DWXYZValue[i][2], Dpoly4DWXYZValue[i][3]);
            if (fabs(Dpoly4DResult[i]-result) > ERROR_TOL ) {
                diag("TEST ERROR: Evaluated value %lg not as expected %lg.\n",
                     result, Dpoly4DResult[i]);
                errorFlag = true;
            }
        }
        ok(!errorFlag, "psPolynomial3DEval() successful (Ordinary)");

        psFree(polyOrd);
        skip_end();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Allocate and evaluate an ordinary polynomial structure
    {
        psMemId id = psMemGetId();
        // Allocate polynomial structure
        psPolynomial4D*  polyCheb = psPolynomial4DAlloc(PS_POLYNOMIAL_CHEB, TERMS-1, TERMS-1, TERMS-1, TERMS-1);
        ok(polyCheb != NULL, "Ordinary psPolynomial4D successfully allocated");
        skip_start(polyCheb == NULL, 1, "Skipping tests because psPolynomial4DAlloc() failed");
        // Set polynomial members
        for(psS32 i = 0; i < TERMS; i++)
        {
            for(psS32 j = 0; j < TERMS; j++) {
                for(psS32 k = 0; k < TERMS; k++) {
                    for(psS32 l = 0; l < TERMS; l++) {
                        polyCheb->coeff[i][j][k][l] = 1.0;
                        polyCheb->coeffMask[i][j][k][l]  = poly4DMask[i][j][k][l];
                    }
                }
            }
        }

        // Evaluate test points and verify results
        bool errorFlag = false;
        for(psS32 i = 0; i < TESTPOINTS; i++)
        {
            psF64 resultCheb = psPolynomial4DEval(polyCheb, Dpoly4DWXYZChebValue[i][0], Dpoly4DWXYZChebValue[i][1],
                                                  Dpoly4DWXYZChebValue[i][2], Dpoly4DWXYZChebValue[i][3]);
            if (fabs(Dpoly4DChebResult[i]-resultCheb) > ERROR_TOL ) {
                diag("TEST ERROR: Evaluated Chebyshev value %lg not as expected %lg.\n",
                     resultCheb, Dpoly4DChebResult[i]);
                errorFlag = true;
            }
        }
        ok(!errorFlag, "psPolynomial3DEval() successful (Ordinary)");

        psFree(polyCheb);
        skip_end();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Allocate polynomial, test the psPolynomial4DEvalVector() routines
    {
        psMemId id = psMemGetId();
        // Create input vectors
        psVector* inputOrdW  = psVectorAlloc(TESTPOINTS, PS_TYPE_F64);
        psVector* inputOrdX  = psVectorAlloc(TESTPOINTS, PS_TYPE_F64);
        psVector* inputOrdY  = psVectorAlloc(TESTPOINTS, PS_TYPE_F64);
        psVector* inputOrdZ  = psVectorAlloc(TESTPOINTS, PS_TYPE_F64);
        for(psS32 i = 0; i < TESTPOINTS; i++)
        {
            inputOrdW->data.F64[i]  = Dpoly4DWXYZValue[i][0];
            inputOrdX->data.F64[i]  = Dpoly4DWXYZValue[i][1];
            inputOrdY->data.F64[i]  = Dpoly4DWXYZValue[i][2];
            inputOrdZ->data.F64[i]  = Dpoly4DWXYZValue[i][3];
            inputOrdW->n++;
            inputOrdX->n++;
            inputOrdY->n++;
            inputOrdZ->n++;
        }

        // Allocate polynomial
        psPolynomial4D* polyOrd = psPolynomial4DAlloc(PS_POLYNOMIAL_ORD, TERMS-1, TERMS-1, TERMS-1, TERMS-1);
        ok(polyOrd != NULL, "Ordinary polynomial allocation successful");
        skip_start(polyOrd == NULL, 12, "Skipping tests because psPolynomial4DAlloc() failed");
        // Set polynomial members
        for(psS32 i = 0; i < TERMS; i++)
        {
            for(psS32 j = 0; j < TERMS; j++) {
                for(psS32 k = 0; k < TERMS; k++) {
                    for(psS32 l = 0; l < TERMS; l++) {
                        polyOrd->coeff[i][j][k][l] = Dpoly4DCoeff[i][j][k][l];
                        polyOrd->coeffMask[i][j][k][l]  = poly4DMask[i][j][k][l];
                    }
                }
            }
        }

        // Evaluate the vectors
        psVector* outputOrd = psPolynomial4DEvalVector(polyOrd,inputOrdW,inputOrdX,inputOrdY,inputOrdZ);
        ok(outputOrd != NULL, "psPolynomial4DEvalVector() generated non-NULL psVector");
        ok(outputOrd->type.type == PS_TYPE_F64, "psPolynomial4DEvalVector() generated correct type");

        // Verify the results
        bool errorFlag = false;
        for(psS32 i = 0; i < TESTPOINTS; i++)
        {
            if (fabs(Dpoly4DResult[i]-outputOrd->data.F64[i]) > ERROR_TOL) {
                diag("TEST ERROR: Result[%d] %lg not equal to expected %lg",
                     i, outputOrd->data.F64[i], Dpoly4DResult[i]);
                errorFlag = true;
            }
        }
        ok(!errorFlag, "psPolynomial4DEvalVector() produced the correct answers");


        // Attempt to invoke function with NULL polynomial
        {
            psMemId id = psMemGetId();
            ok(psPolynomial4DEvalVector(NULL,inputOrdW,inputOrdX,inputOrdY,inputOrdZ) == NULL, "psPolynomial4DEvalVector() produced NULL when called with NULL polynomial");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }


        // Attempt to invoke function with NULL input vector
        {
            psMemId id = psMemGetId();
            ok(psPolynomial4DEvalVector(polyOrd,NULL,inputOrdX,inputOrdY,inputOrdZ) == NULL, "psPolynomial4DEvalVector() produced NULL when called with NULL input vector");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }


        // Attempt to invoke function with NULL input vector
        {
            psMemId id = psMemGetId();
            ok(psPolynomial4DEvalVector(polyOrd,inputOrdW,NULL,inputOrdY,inputOrdZ) == NULL, "psPolynomial4DEvalVector() produced NULL when called with NULL input vector");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }


        // Attempt to invoke function with NULL input vector
        {
            psMemId id = psMemGetId();
            ok(psPolynomial4DEvalVector(polyOrd,inputOrdW,inputOrdX,NULL,inputOrdZ) == NULL, "psPolynomial4DEvalVector() produced NULL when called with NULL input vector");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }


        // Attempt to invoke function with NULL input vector
        {
            psMemId id = psMemGetId();
            ok(psPolynomial4DEvalVector(polyOrd,inputOrdW,inputOrdX,inputOrdY,NULL) == NULL, "psPolynomial4DEvalVector() produced NULL when called with NULL input vector");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }


        // Attempt to invoke function with a non F64 type input vector
        {
            psMemId id = psMemGetId();
            inputOrdX->type.type = PS_TYPE_U8;
            ok(psPolynomial4DEvalVector(polyOrd,inputOrdW,inputOrdX,inputOrdY,inputOrdZ) == NULL, "psPolynomial4DEvalVector() produced NULL when called with non F64 input vector");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }
        inputOrdX->type.type = PS_TYPE_F64;


        // Attempt to invoke function with a non F64 type input vector
        {
            psMemId id = psMemGetId();
            inputOrdY->type.type = PS_TYPE_U8;
            ok(psPolynomial4DEvalVector(polyOrd,inputOrdW,inputOrdX,inputOrdY,inputOrdZ) == NULL, "psPolynomial4DEvalVector() produced NULL when called with non F64 input vector");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }
        inputOrdY->type.type = PS_TYPE_F64;


        // Attempt to invoke function with a non F64 type input vector
        {
            psMemId id = psMemGetId();
            inputOrdZ->type.type = PS_TYPE_U8;
            ok(psPolynomial4DEvalVector(polyOrd,inputOrdW,inputOrdX,inputOrdY,inputOrdZ) == NULL, "psPolynomial4DEvalVector() produced NULL when called with non F64 input vector");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }
        inputOrdZ->type.type = PS_TYPE_F64;


        // Attempt to invoke function with a non F64 type input vector
        {
            psMemId id = psMemGetId();
            inputOrdW->type.type = PS_TYPE_U8;
            ok(psPolynomial4DEvalVector(polyOrd,inputOrdW,inputOrdX,inputOrdY,inputOrdZ) == NULL, "psPolynomial4DEvalVector() produced NULL when called with non F64 input vector");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }
        inputOrdW->type.type = PS_TYPE_F64;
        psFree(outputOrd);
        skip_end();
        psFree(inputOrdX);
        psFree(inputOrdY);
        psFree(inputOrdZ);
        psFree(inputOrdW);
        psFree(polyOrd);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Allocate polynomial, test the psPolynomial4DEvalVector() routines
    {
        psMemId id = psMemGetId();
        // Create input vectors
        psVector* inputChebW = psVectorAlloc(TESTPOINTS, PS_TYPE_F64);
        psVector* inputChebX = psVectorAlloc(TESTPOINTS, PS_TYPE_F64);
        psVector* inputChebY = psVectorAlloc(TESTPOINTS, PS_TYPE_F64);
        psVector* inputChebZ = psVectorAlloc(TESTPOINTS, PS_TYPE_F64);
        for(psS32 i = 0; i < TESTPOINTS; i++)
        {
            inputChebW->data.F64[i] = Dpoly4DWXYZChebValue[i][0];
            inputChebX->data.F64[i] = Dpoly4DWXYZChebValue[i][1];
            inputChebY->data.F64[i] = Dpoly4DWXYZChebValue[i][2];
            inputChebZ->data.F64[i] = Dpoly4DWXYZChebValue[i][3];
            inputChebW->n++;
            inputChebX->n++;
            inputChebY->n++;
            inputChebZ->n++;
        }

        // Allocate polynomial
        psPolynomial4D* polyCheb = psPolynomial4DAlloc(PS_POLYNOMIAL_CHEB, TERMS-1, TERMS-1, TERMS-1, TERMS-1);
        ok(polyCheb != NULL, "Ordinary polynomial allocation successful");
        skip_start(polyCheb == NULL, 12, "Skipping tests because psPolynomial4DAlloc() failed");

        // Set polynomial members
        for(psS32 i = 0; i < TERMS; i++)
        {
            for(psS32 j = 0; j < TERMS; j++) {
                for(psS32 k = 0; k < TERMS; k++) {
                    for(psS32 l = 0; l < TERMS; l++) {
                        polyCheb->coeff[i][j][k][l] = 1.0;
                        polyCheb->coeffMask[i][j][k][l]  = poly4DMask[i][j][k][l];
                    }
                }
            }
        }

        // Evaluate the vectors
        psVector* outputCheb = psPolynomial4DEvalVector(polyCheb,inputChebW,inputChebX,inputChebY,inputChebZ);
        ok(outputCheb != NULL, "psPolynomial4DEvalVector() generated non-NULL psVector");
        ok(outputCheb->type.type == PS_TYPE_F64, "psPolynomial4DEvalVector() generated correct type");

        // Verify the results
        bool errorFlag = false;
        for(psS32 i = 0; i < TESTPOINTS; i++)
        {
            if (fabs(Dpoly4DChebResult[i]-outputCheb->data.F64[i]) > ERROR_TOL) {
                diag("TEST ERROR: ResultCheb[%d] %lg not equal to expected %lg",
                     i, outputCheb->data.F64[i], Dpoly4DChebResult[i]);
                errorFlag = true;
            }
        }
        ok(!errorFlag, "psPolynomial4DEvalVector() produced the correct answers");


        // Attempt to invoke function with NULL polynomial
        {
            psMemId id = psMemGetId();
            ok(psPolynomial4DEvalVector(NULL,inputChebW,inputChebX,inputChebY,inputChebZ) == NULL, "psPolynomial4DEvalVector() produced NULL when called with NULL polynomial");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }


        // Attempt to invoke function with NULL input vector
        {
            psMemId id = psMemGetId();
            ok(psPolynomial4DEvalVector(polyCheb,NULL,inputChebX,inputChebY,inputChebZ) == NULL, "psPolynomial4DEvalVector() produced NULL when called with NULL input vector");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }


        // Attempt to invoke function with NULL input vector
        {
            psMemId id = psMemGetId();
            ok(psPolynomial4DEvalVector(polyCheb,inputChebW,NULL,inputChebY,inputChebZ) == NULL, "psPolynomial4DEvalVector() produced NULL when called with NULL input vector");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }


        // Attempt to invoke function with NULL input vector
        {
            psMemId id = psMemGetId();
            ok(psPolynomial4DEvalVector(polyCheb,inputChebW,inputChebX,NULL,inputChebZ) == NULL, "psPolynomial4DEvalVector() produced NULL when called with NULL input vector");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }


        // Attempt to invoke function with NULL input vector
        {
            psMemId id = psMemGetId();
            ok(psPolynomial4DEvalVector(polyCheb,inputChebW,inputChebX,inputChebY,NULL) == NULL, "psPolynomial4DEvalVector() produced NULL when called with NULL input vector");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }


        // Attempt to invoke function with a non F64 type input vector
        {
            psMemId id = psMemGetId();
            inputChebX->type.type = PS_TYPE_U8;
            ok(psPolynomial4DEvalVector(polyCheb,inputChebW,inputChebX,inputChebY,inputChebZ) == NULL, "psPolynomial4DEvalVector() produced NULL when called with non F64 input vector");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }
        inputChebX->type.type = PS_TYPE_F64;


        // Attempt to invoke function with a non F64 type input vector
        {
            psMemId id = psMemGetId();
            inputChebY->type.type = PS_TYPE_U8;
            ok(psPolynomial4DEvalVector(polyCheb,inputChebW,inputChebX,inputChebY,inputChebZ) == NULL, "psPolynomial4DEvalVector() produced NULL when called with non F64 input vector");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }
        inputChebY->type.type = PS_TYPE_F64;


        // Attempt to invoke function with a non F64 type input vector
        {
            psMemId id = psMemGetId();
            inputChebZ->type.type = PS_TYPE_U8;
            ok(psPolynomial4DEvalVector(polyCheb,inputChebW,inputChebX,inputChebY,inputChebZ) == NULL, "psPolynomial4DEvalVector() produced NULL when called with non F64 input vector");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }
        inputChebZ->type.type = PS_TYPE_F64;


        // Attempt to invoke function with a non F64 type input vector
        {
            psMemId id = psMemGetId();
            inputChebW->type.type = PS_TYPE_U8;
            ok(psPolynomial4DEvalVector(polyCheb,inputChebW,inputChebX,inputChebY,inputChebZ) == NULL, "psPolynomial4DEvalVector() produced NULL when called with non F64 input vector");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }
        inputChebW->type.type = PS_TYPE_F64;
        psFree(outputCheb);
        skip_end();
        psFree(inputChebW);
        psFree(inputChebX);
        psFree(inputChebY);
        psFree(inputChebZ);
        psFree(polyCheb);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
}
