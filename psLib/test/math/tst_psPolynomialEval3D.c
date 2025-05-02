/** tst_psFunc10.c
*
*  This test driver will exercise the psPolynomialXDEval functions for both
*  ORD and CHEB type polynomials.
*
*  @version  $Revision: 1.2 $  $Name: not supported by cvs2svn $
*  @date  $Date: 2006-02-24 23:43:15 $
*
* Copyright 2004-2005 Maui High Performance Computing Center, Univ. of Hawaii
*
***************************************************************************/

#include "pslib_strict.h"
#include "psTest.h"

#define  TERMS        4
#define  TESTPOINTS   5
#define  ERROR_TOL    0.001

static psS32 testPoly3DEval(void);
static psS32 testPoly3DEvalVector(void);

testDescription tests[] = {
                              {testPoly3DEval,583,"psPolynomial3DEval",true,false},
                              {testPoly3DEvalVector,000,"psPolynomial3DEvalVector",true,false},
                              {NULL}
                          };

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

    return !runTestSuite(stderr,"psPolynomialXDEval", tests, argc, argv);
}

// This test will verify operation of 1D polynomial evaluation
psS32 testPoly3DEval(void)
{
    psF64  result;
    psF64  resultCheb;
    psBool testStatus = true;

    // Allocate polynomial structure
    psPolynomial3D*  polyOrd = psPolynomial3DAlloc(PS_POLYNOMIAL_ORD, TERMS-1, TERMS-1, TERMS-1);
    psPolynomial3D*  polyCheb = psPolynomial3DAlloc(PS_POLYNOMIAL_CHEB, TERMS-1, TERMS-1, TERMS-1);
    // Set polynomial members
    for(psS32 i = 0; i < TERMS; i++) {
        for(psS32 j = 0; j < TERMS; j++) {
            for(psS32 k = 0; k < TERMS; k++) {
                polyOrd->coeff[i][j][k] = Dpoly3DCoeff[i][j][k];
                polyOrd->mask[i][j][k]  = poly3DMask[i][j][k];
                polyCheb->coeff[i][j][k] = 1.0;
                polyCheb->mask[i][j][k]  = poly3DMask[i][j][k];
            }
        }
    }
    // Evaluate test points and verify results
    for(psS32 i = 0; i < TESTPOINTS; i++) {
        result = psPolynomial3DEval(polyOrd,Dpoly3DXYZValue[i][0],Dpoly3DXYZValue[i][1],
                                    Dpoly3DXYZValue[i][2]);
        if(fabs(Dpoly3DResult[i]-result) > ERROR_TOL ) {
            printf("TEST ERROR: Evaluated value %lg not as expected %lg.\n",
                   result, Dpoly3DResult[i]);
            testStatus = false;
        }
        resultCheb = psPolynomial3DEval(polyCheb,Dpoly3DXYZChebValue[i][0],Dpoly3DXYZChebValue[i][1],
                                        Dpoly3DXYZChebValue[i][2]);
        if(fabs(Dpoly3DChebResult[i]-resultCheb) > ERROR_TOL ) {
            printf("TEST ERROR: Evaluated Chebyshev value %lg not as expected %lg.\n",
                   resultCheb, Dpoly3DChebResult[i]);
            testStatus = false;
        }
    }
    psFree(polyOrd);
    psFree(polyCheb);

    // Allocate polynomial with invalid type
    polyOrd = psPolynomial3DAlloc(99, TERMS-1, TERMS-1, TERMS-1);
    // Attempt to evaluation invalid polynomial type
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate error message invalid type");
    result = psPolynomial3DEval(polyOrd,0.0, 0.0, 0.0);
    if ( !isnan(result) ) {
        printf("TEST ERROR: Did not return NAN for invalid polynomial type.\n");
        testStatus = false;
    }
    psFree(polyOrd);

    return(testStatus);
}

psS32 testPoly3DEvalVector(void)
{
    psBool testStatus = true;
    // Allocate polynomial
    psPolynomial3D* polyOrd = psPolynomial3DAlloc(PS_POLYNOMIAL_ORD, TERMS-1, TERMS-1, TERMS-1);
    psPolynomial3D* polyCheb = psPolynomial3DAlloc(PS_POLYNOMIAL_CHEB, TERMS-1, TERMS-1, TERMS-1);

    // Set polynomial members
    for(psS32 i = 0; i < TERMS; i++) {
        for(psS32 j = 0; j < TERMS; j++) {
            for(psS32 k = 0; k < TERMS; k++) {
                polyOrd->coeff[i][j][k] = Dpoly3DCoeff[i][j][k];
                polyOrd->mask[i][j][k]  = poly3DMask[i][j][k];
                polyCheb->coeff[i][j][k] = 1.0;
                polyCheb->mask[i][j][k]  = poly3DMask[i][j][k];
            }
        }
    }

    // Create input vectors
    psVector* inputOrdX  = psVectorAlloc(TESTPOINTS, PS_TYPE_F64);
    psVector* inputOrdY  = psVectorAlloc(TESTPOINTS, PS_TYPE_F64);
    psVector* inputOrdZ  = psVectorAlloc(TESTPOINTS, PS_TYPE_F64);
    psVector* inputChebX = psVectorAlloc(TESTPOINTS, PS_TYPE_F64);
    psVector* inputChebY = psVectorAlloc(TESTPOINTS, PS_TYPE_F64);
    psVector* inputChebZ = psVectorAlloc(TESTPOINTS, PS_TYPE_F64);
    for(psS32 i = 0; i < TESTPOINTS; i++) {
        inputOrdX->data.F64[i]  = Dpoly3DXYZValue[i][0];
        inputOrdY->data.F64[i]  = Dpoly3DXYZValue[i][1];
        inputOrdZ->data.F64[i]  = Dpoly3DXYZValue[i][2];
        inputChebX->data.F64[i] = Dpoly3DXYZChebValue[i][0];
        inputChebY->data.F64[i] = Dpoly3DXYZChebValue[i][1];
        inputChebZ->data.F64[i] = Dpoly3DXYZChebValue[i][2];
        inputOrdX->n++;
        inputOrdY->n++;
        inputOrdZ->n++;
        inputChebX->n++;
        inputChebY->n++;
        inputChebZ->n++;
    }

    // Evaluate the vectors
    psVector* outputOrd = psPolynomial3DEvalVector(polyOrd,inputOrdX,inputOrdY,inputOrdZ);
    if(outputOrd == NULL) {
        printf("TEST ERROR: Unexpected return of NULL.\n");
        testStatus = false;
    }
    if(outputOrd->type.type != PS_TYPE_F64) {
        printf("TEST ERROR: Output vector of type %d expected %d.\n",
               outputOrd->type.type, PS_TYPE_F64);
        testStatus = false;
    }
    psVector* outputCheb = psPolynomial3DEvalVector(polyCheb,inputChebX,inputChebY,inputChebZ);
    if(outputCheb == NULL) {
        printf("TEST ERROR: Unexpected return of NULL.\n");
        testStatus = false;
    }
    if(outputCheb->type.type != PS_TYPE_F64) {
        printf("TEST ERROR: Output vector of type %d expected %d.\n",
               outputCheb->type.type, PS_TYPE_F64);
        testStatus = false;
    }

    // Verify the results
    for(psS32 i = 0; i < TESTPOINTS; i++) {
        if(fabs(Dpoly3DResult[i]-outputOrd->data.F64[i]) > ERROR_TOL) {
            printf("TEST ERROR: Result[%d] %lg not equal to expected %lg.\n",
                   i, outputOrd->data.F64[i], Dpoly3DResult[i]);
            testStatus = false;
        }
        if(fabs(Dpoly3DChebResult[i]-outputCheb->data.F64[i]) > ERROR_TOL) {
            printf("TEST ERROR: ResultCheb[%d] %lg not equal to expected %lg.\n",
                   i, outputCheb->data.F64[i], Dpoly3DChebResult[i]);
            testStatus = false;
        }
    }

    // Attempt to invoke function with null polynomial
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate an error message for NULL polynomial");
    if(psPolynomial3DEvalVector(NULL,inputOrdX,inputOrdY,inputOrdZ) != NULL) {
        printf("TEST ERROR: Return of NULL expected for NULL polynomial.\n");
        testStatus = false;
    }

    // Attempt to invoke function with null input vector
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate an error message for NULL input vector");
    if(psPolynomial3DEvalVector(polyOrd,NULL,inputOrdY,inputOrdZ) != NULL) {
        printf("TEST ERROR: Return of NULL expected for NULL input vector.\n");
        testStatus = false;
    }
    // Attempt to invoke function with null input vector
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate an error message for NULL input vector");
    if(psPolynomial3DEvalVector(polyOrd,inputOrdX,NULL,inputOrdZ) != NULL) {
        printf("TEST ERROR: Return of NULL expected for NULL input vector.\n");
        testStatus = false;
    }
    // Attempt to invoke function with null input vector
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate an error message for NULL input vector");
    if(psPolynomial3DEvalVector(polyOrd,inputOrdX,inputOrdY,NULL) != NULL) {
        printf("TEST ERROR: Return of NULL expected for NULL input vector.\n");
        testStatus = false;
    }

    // Attempt to invoke function with a non F64 type input vector
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate an error message for invalid input type");
    inputOrdX->type.type = PS_TYPE_U8;
    if(psPolynomial3DEvalVector(polyOrd,inputOrdX,inputOrdY,inputOrdZ) != NULL) {
        printf("TEST ERROR: Return NULL expected for non-F64 input vector.\n");
        testStatus = false;
    }
    inputOrdX->type.type = PS_TYPE_F64;
    // Attempt to invoke function with a non F64 type input vector
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate an error message for invalid input type");
    inputOrdY->type.type = PS_TYPE_U8;
    if(psPolynomial3DEvalVector(polyOrd,inputOrdX,inputOrdY,inputOrdZ) != NULL) {
        printf("TEST ERROR: Return NULL expected for non-F64 input vector.\n");
        testStatus = false;
    }
    inputOrdY->type.type = PS_TYPE_F64;
    // Attempt to invoke function with a non F64 type input vector
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate an error message for invalid input type");
    inputOrdZ->type.type = PS_TYPE_U8;
    if(psPolynomial3DEvalVector(polyOrd,inputOrdX,inputOrdY,inputOrdZ) != NULL) {
        printf("TEST ERROR: Return NULL expected for non-F64 input vector.\n");
        testStatus = false;
    }
    inputOrdZ->type.type = PS_TYPE_F64;

    psFree(inputOrdX);
    psFree(inputOrdY);
    psFree(inputOrdZ);
    psFree(inputChebX);
    psFree(inputChebY);
    psFree(inputChebZ);
    psFree(outputOrd);
    psFree(outputCheb);
    psFree(polyOrd);
    psFree(polyCheb);

    return(testStatus);
}

