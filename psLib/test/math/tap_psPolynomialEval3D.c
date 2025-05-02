/** tap_psPolynomialEval3D.c
*
*  This test driver will exercise the psPolynomialXDEval functions for both
*  ORD and CHEB type polynomials.
*
*  @version  $Revision: 1.9 $  $Name: not supported by cvs2svn $
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

psF32 poly3DCoeff[TERMS][TERMS][TERMS] = {  { { -1.1,  1.2, -1.3,  1.4},
        {  1.5, -1.6,  1.7, -1.8},
        {  0.1, -0.2,  0.3, -0.4},
        { -0.5,  0.6, -0.7,  0.8}
                                            },
        { { -2.1,  2.2, -2.3,  2.4},
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
                                         };
psF64 Dpoly3DCoeff[TERMS][TERMS][TERMS] = {  { { -1.1,  1.2, -1.3,  1.4},
        {  1.5, -1.6,  1.7, -1.8},
        {  0.1, -0.2,  0.3, -0.4},
        { -0.5,  0.6, -0.7,  0.8}
                                             },
        { { -2.1,  2.2, -2.3,  2.4},
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
                                          };


psF32 poly3DMask[TERMS][TERMS][TERMS]    = { {  {  0,    0,    0,    1},
        {  0,    0,    1,    0},
        {  1,    0,    0,    0},
        {  0,    0,    0,    1}
                                             },
        {  {  1,    0,    0,    0},
           {  1,    0,    0,    0},
           {  1,    0,    0,    0},
           {  1,    0,    0,    0}
        },
        {  {  0,    1,    0,    0},
           {  0,    0,    1,    0},
           {  0,    1,    0,    0},
           {  0,    0,    1,    0}
        },
        {  {  1,    0,    0,    0},
           {  0,    0,    0,    1},
           {  1,    0,    0,    0},
           {  0,    0,    0,    1}
        },
                                           };

psF32 poly3DXYZValue[TESTPOINTS][3] = {  {  0.450, -0.780,  0.500},
                                      {  0.297,  0.153, -0.354},
                                      {  0.000,  0.153, -0.354},
                                      {  0.297,  0.000, -0.354},
                                      {  0.297,  0.153,  0.000}
                                      };
psF64 Dpoly3DXYZValue[TESTPOINTS][3] = {  {  0.450, -0.780,  0.500},
                                       {  0.297,  0.153, -0.354},
                                       {  0.000,  0.153, -0.354},
                                       {  0.297,  0.000, -0.354},
                                       {  0.297,  0.153,  0.000}
                                       };
psF32 poly3DResult[TESTPOINTS]  = { -1.298691, -2.011591, -1.359247, -2.548266, -1.139072};
psF64 Dpoly3DResult[TESTPOINTS] = { -1.298691, -2.011591, -1.359247, -2.548266, -1.139072};


psF32 poly3DXYZChebValue[TESTPOINTS][3] = {  {  0.000,  0.250, -0.250},
        { -0.250,  0.000,  0.250},
        {  0.250, -0.250,  0.000},
        {  0.100, -0.300, -0.400},
        {  0.990, -0.010,  0.500}
                                          };
psF64 Dpoly3DXYZChebValue[TESTPOINTS][3] = {  {  0.000,  0.250, -0.250},
        { -0.250,  0.000,  0.250},
        {  0.250, -0.250,  0.000},
        {  0.100, -0.300, -0.400},
        {  0.990, -0.010,  0.500}
                                           };
psF32 poly3DChebResult[TESTPOINTS]  = {  1.230469, 1.687500, 0.187500, -1.452707, 2.032344 };
psF64 Dpoly3DChebResult[TESTPOINTS] = {  1.230469, 1.687500, 0.187500, -1.452707, 2.032344 };

psS32 main(psS32 argc, char* argv[])
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    plan_tests(46);


    // Evaluate NULL polynomial
    {
        psMemId id = psMemGetId();
        psF64 result = psPolynomial3DEval(NULL, 0.0, 0.0, 0.0);
        ok(isnan(result), "psPolynomial3DEval() returned NAN with NULL polynomial");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");

    }


    // Allocate and evaluate an ordinary polynomial structure
    {
        psMemId id = psMemGetId();

        // Allocate polynomial structure
        psPolynomial3D*  polyOrd = psPolynomial3DAlloc(PS_POLYNOMIAL_ORD, TERMS-1, TERMS-1, TERMS-1);
        ok(polyOrd != NULL, "Ordinary psPolynomial3D successfully allocated");
        skip_start(polyOrd == NULL, 1, "Skipping tests because psPolynomial3DAlloc() failed");

        // Set polynomial members
        for(psS32 i = 0; i < TERMS; i++)
        {
            for(psS32 j = 0; j < TERMS; j++) {
                for(psS32 k = 0; k < TERMS; k++) {
                    polyOrd->coeff[i][j][k] = Dpoly3DCoeff[i][j][k];
                    polyOrd->coeffMask[i][j][k]  = poly3DMask[i][j][k];
                }
            }
        }

        // Evaluate test points and verify results
        bool errorFlag = false;
        for(psS32 i = 0; i < TESTPOINTS; i++)
        {
            psF64 result = psPolynomial3DEval(polyOrd,Dpoly3DXYZValue[i][0],Dpoly3DXYZValue[i][1],
                                              Dpoly3DXYZValue[i][2]);
            if (fabs(Dpoly3DResult[i]-result) > ERROR_TOL ) {
                diag("TEST ERROR: Evaluated value %lg not as expected %lg.\n",
                     result, Dpoly3DResult[i]);
                errorFlag = true;
            }
        }
        ok(!errorFlag, "psPolynomial3DEval() successful (Ordinary)");

        psFree(polyOrd);
        skip_end();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Allocate and evaluate Cheby polynomial structure
    {
        psMemId id = psMemGetId();

        // Allocate polynomial structure
        psPolynomial3D*  polyCheb = psPolynomial3DAlloc(PS_POLYNOMIAL_CHEB, TERMS-1, TERMS-1, TERMS-1);
        ok(polyCheb != NULL, "Ordinary psPolynomial3D successfully allocated");
        skip_start(polyCheb == NULL, 1, "Skipping tests because psPolynomial3DAlloc() failed");

        // Set polynomial members
        for(psS32 i = 0; i < TERMS; i++)
        {
            for(psS32 j = 0; j < TERMS; j++) {
                for(psS32 k = 0; k < TERMS; k++) {
                    polyCheb->coeff[i][j][k] = 1.0;
                    polyCheb->coeffMask[i][j][k]  = poly3DMask[i][j][k];
                }
            }
        }

        // Evaluate test points and verify results
        bool errorFlag = false;
        for(psS32 i = 0; i < TESTPOINTS; i++)
        {
            psF64 resultCheb = psPolynomial3DEval(polyCheb,Dpoly3DXYZChebValue[i][0],Dpoly3DXYZChebValue[i][1],
                                                  Dpoly3DXYZChebValue[i][2]);
            if (fabs(Dpoly3DChebResult[i]-resultCheb) > ERROR_TOL ) {
                diag("TEST ERROR: Evaluated Chebyshev value %lg not as expected %lg.\n",
                     resultCheb, Dpoly3DChebResult[i]);
                errorFlag = true;
            }
        }
        ok(!errorFlag, "psPolynomial3DEval() successful (Cheby)");

        psFree(polyCheb);
        skip_end();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Allocate polynomial, test the psPolynomial3DEvalVector() routines
    {
        psMemId id = psMemGetId();
        // Create input vectors
        psVector* inputOrdX  = psVectorAlloc(TESTPOINTS, PS_TYPE_F64);
        psVector* inputOrdY  = psVectorAlloc(TESTPOINTS, PS_TYPE_F64);
        psVector* inputOrdZ  = psVectorAlloc(TESTPOINTS, PS_TYPE_F64);
        for(psS32 i = 0; i < TESTPOINTS; i++)
        {
            inputOrdX->data.F64[i]  = Dpoly3DXYZValue[i][0];
            inputOrdY->data.F64[i]  = Dpoly3DXYZValue[i][1];
            inputOrdZ->data.F64[i]  = Dpoly3DXYZValue[i][2];
            inputOrdX->n++;
            inputOrdY->n++;
            inputOrdZ->n++;
        }

        psPolynomial3D* polyOrd = psPolynomial3DAlloc(PS_POLYNOMIAL_ORD, TERMS-1, TERMS-1, TERMS-1);
        ok(polyOrd != NULL, "Ordinary polynomial allocation successful");
        skip_start(polyOrd == NULL, 10, "Skipping tests because psPolynomial3DAlloc() failed");
        // Set polynomial members
        for(psS32 i = 0; i < TERMS; i++)
        {
            for(psS32 j = 0; j < TERMS; j++) {
                for(psS32 k = 0; k < TERMS; k++) {
                    polyOrd->coeff[i][j][k] = Dpoly3DCoeff[i][j][k];
                    polyOrd->coeffMask[i][j][k]  = poly3DMask[i][j][k];
                }
            }
        }

        // Evaluate the vectors
        psVector* outputOrd = psPolynomial3DEvalVector(polyOrd,inputOrdX,inputOrdY,inputOrdZ);
        ok(outputOrd != NULL, "psPolynomial3DEvalVector() generated non-NULL psVector");
        ok(outputOrd->type.type == PS_TYPE_F64, "psPolynomial3DEvalVector() generated correct type");

        // Verify the results
        bool errorFlag = false;
        for(psS32 i = 0; i < TESTPOINTS; i++)
        {
            if (fabs(Dpoly3DResult[i]-outputOrd->data.F64[i]) > ERROR_TOL) {
                diag("TEST ERROR: Result[%d] %lg not equal to expected %lg.\n",
                     i, outputOrd->data.F64[i], Dpoly3DResult[i]);
                errorFlag = true;
            }
        }
        ok(!errorFlag, "psPolynomial3DEvalVector() produced the correct answers");


        // Attempt to invoke function with NULL polynomial
        {
            psMemId id = psMemGetId();
            ok(psPolynomial3DEvalVector(NULL,inputOrdX,inputOrdY,inputOrdZ) == NULL, "psPolynomial3DEvalVector() produced NULL when called with NULL polynomial");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }


        // Attempt to invoke function with NULL input vector
        {
            psMemId id = psMemGetId();
            ok(psPolynomial3DEvalVector(polyOrd,NULL,inputOrdY,inputOrdZ) == NULL, "psPolynomial3DEvalVector() produced NULL when called with NULL input vector");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }
    

        // Attempt to invoke function with NULL input vector
        {
            psMemId id = psMemGetId();
            ok(psPolynomial3DEvalVector(polyOrd,inputOrdX,NULL,inputOrdZ) == NULL, "psPolynomial3DEvalVector() produced NULL when called with NULL input vector");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }


        // Attempt to invoke function with NULL input vector
        {
            psMemId id = psMemGetId();
            ok(psPolynomial3DEvalVector(polyOrd,inputOrdX,inputOrdY,NULL) == NULL, "psPolynomial3DEvalVector() produced NULL when called with NULL input vector");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }


        // Attempt to invoke function with a non F64 type input vector
        {
            psMemId id = psMemGetId();
            inputOrdX->type.type = PS_TYPE_U8;
            ok(psPolynomial3DEvalVector(polyOrd,inputOrdX,inputOrdY,inputOrdZ) == NULL, "psPolynomial3DEvalVector() produced NULL when called with non F64 input vector");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }
        inputOrdX->type.type = PS_TYPE_F64;


        // Attempt to invoke function with a non F64 type input vector
        {
            psMemId id = psMemGetId();
            inputOrdY->type.type = PS_TYPE_U8;
            ok(psPolynomial3DEvalVector(polyOrd,inputOrdX,inputOrdY,inputOrdZ) == NULL, "psPolynomial3DEvalVector() produced NULL when called with non F64 input vector");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }
        inputOrdY->type.type = PS_TYPE_F64;

        // Attempt to invoke function with a non F64 type input vector
        {
            psMemId id = psMemGetId();
            inputOrdZ->type.type = PS_TYPE_U8;
            ok(psPolynomial3DEvalVector(polyOrd,inputOrdX,inputOrdY,inputOrdZ) == NULL, "psPolynomial3DEvalVector() produced NULL when called with non F64 input vector");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }
        psFree(outputOrd);
        skip_end();
        psFree(inputOrdX);
        psFree(inputOrdY);
        psFree(inputOrdZ);
        psFree(polyOrd);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Allocate polynomial, test the psPolynomial3DEvalVector() routines
    {
        psMemId id = psMemGetId();
        // Create input vectors
        psVector* inputChebX = psVectorAlloc(TESTPOINTS, PS_TYPE_F64);
        psVector* inputChebY = psVectorAlloc(TESTPOINTS, PS_TYPE_F64);
        psVector* inputChebZ = psVectorAlloc(TESTPOINTS, PS_TYPE_F64);
        for(psS32 i = 0; i < TESTPOINTS; i++)
        {
            inputChebX->data.F64[i] = Dpoly3DXYZChebValue[i][0];
            inputChebY->data.F64[i] = Dpoly3DXYZChebValue[i][1];
            inputChebZ->data.F64[i] = Dpoly3DXYZChebValue[i][2];
            inputChebX->n++;
            inputChebY->n++;
            inputChebZ->n++;
        }
        psPolynomial3D* polyCheb = psPolynomial3DAlloc(PS_POLYNOMIAL_CHEB, TERMS-1, TERMS-1, TERMS-1);
        ok(polyCheb != NULL, "Ordinary polynomial allocation successful");
        skip_start(polyCheb == NULL, 10, "Skipping tests because psPolynomial3DAlloc() failed");
        // Set polynomial members
        for(psS32 i = 0; i < TERMS; i++)
        {
            for(psS32 j = 0; j < TERMS; j++) {
                for(psS32 k = 0; k < TERMS; k++) {
                    polyCheb->coeff[i][j][k] = 1.0;
                    polyCheb->coeffMask[i][j][k]  = poly3DMask[i][j][k];
                }
            }
        }

        // Evaluate the vectors
        psVector* outputCheb = psPolynomial3DEvalVector(polyCheb,inputChebX,inputChebY,inputChebZ);
        ok(outputCheb != NULL, "psPolynomial3DEvalVector() generated non-NULL psVector");
        ok(outputCheb->type.type == PS_TYPE_F64, "psPolynomial3DEvalVector() generated correct type");

        // Verify the results
        bool errorFlag = false;
        for(psS32 i = 0; i < TESTPOINTS; i++)
        {
            if (fabs(Dpoly3DChebResult[i]-outputCheb->data.F64[i]) > ERROR_TOL) {
                diag("TEST ERROR: ResultCheb[%d] %lg not equal to expected %lg.\n",
                     i, outputCheb->data.F64[i], Dpoly3DChebResult[i]);
                errorFlag = true;
            }
        }
        ok(!errorFlag, "psPolynomial3DEvalVector() produced the correct answers");

        // Attempt to invoke function with NULL polynomial
        {
            psMemId id = psMemGetId();
            ok(psPolynomial3DEvalVector(NULL,inputChebX,inputChebY,inputChebZ) == NULL, "psPolynomial3DEvalVector() produced NULL when called with NULL polynomial");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }


        // Attempt to invoke function with NULL input vector
        {
            psMemId id = psMemGetId();
            ok(psPolynomial3DEvalVector(polyCheb,NULL,inputChebY,inputChebZ) == NULL, "psPolynomial3DEvalVector() produced NULL when called with NULL input vector");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }


        // Attempt to invoke function with NULL input vector
        {
            psMemId id = psMemGetId();
            ok(psPolynomial3DEvalVector(polyCheb,inputChebX,NULL,inputChebZ) == NULL, "psPolynomial3DEvalVector() produced NULL when called with NULL input vector");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }


        // Attempt to invoke function with NULL input vector
        {
            psMemId id = psMemGetId();
            ok(psPolynomial3DEvalVector(polyCheb,inputChebX,inputChebY,NULL) == NULL, "psPolynomial3DEvalVector() produced NULL when called with NULL input vector");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }


        // Attempt to invoke function with a non F64 type input vector
        {
            psMemId id = psMemGetId();
            inputChebX->type.type = PS_TYPE_U8;
            ok(psPolynomial3DEvalVector(polyCheb,inputChebX,inputChebY,inputChebZ) == NULL, "psPolynomial3DEvalVector() produced NULL when called with non F64 input vector");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }
        inputChebX->type.type = PS_TYPE_F64;


        // Attempt to invoke function with a non F64 type input vector
        {
            psMemId id = psMemGetId();
            inputChebY->type.type = PS_TYPE_U8;
            ok(psPolynomial3DEvalVector(polyCheb,inputChebX,inputChebY,inputChebZ) == NULL, "psPolynomial3DEvalVector() produced NULL when called with non F64 input vector");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }
        inputChebY->type.type = PS_TYPE_F64;


        // Attempt to invoke function with a non F64 type input vector
        {
            psMemId id = psMemGetId();
            inputChebZ->type.type = PS_TYPE_U8;
            ok(psPolynomial3DEvalVector(polyCheb,inputChebX,inputChebY,inputChebZ) == NULL, "psPolynomial3DEvalVector() produced NULL when called with non F64 input vector");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }
        psFree(outputCheb);
        skip_end();
        psFree(inputChebX);
        psFree(inputChebY);
        psFree(inputChebZ);
        psFree(polyCheb);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
}

