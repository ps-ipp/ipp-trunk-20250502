/** tst_psFunc09.c
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

static psS32 testPoly2DEval(void);
static psS32 testPoly2DEvalVector(void);

testDescription tests[] = {
                              {testPoly2DEval,583,"psPolynomial2DEval",true,false},
                              {testPoly2DEvalVector,000,"psPolynomial2DEvalVector",true,false},
                              {NULL}
                          };

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

    return !runTestSuite(stderr,"psPolynomialXDEval", tests, argc, argv);
}

// This test will verify operation of 1D polynomial evaluation
psS32 testPoly2DEval(void)
{
    psF64  result;
    psF64  resultCheb;
    psBool testStatus = true;

    // Allocate polynomial structure
    psPolynomial2D*  polyOrd = psPolynomial2DAlloc(PS_POLYNOMIAL_ORD, TERMS-1, TERMS-1);
    psPolynomial2D*  polyCheb = psPolynomial2DAlloc(PS_POLYNOMIAL_CHEB, TERMS-1, TERMS-1);
    // Set polynomial members
    for(psS32 i = 0; i < TERMS; i++) {
        for(psS32 j = 0; j < TERMS; j++) {
            polyOrd->coeff[i][j] = poly2DCoeff[i][j];
            polyOrd->mask[i][j]  = poly2DMask[i][j];
            polyCheb->coeff[i][j] = 1.0;
            polyCheb->mask[i][j]  = poly2DMask[i][j];
        }
    }
    // Evaluate test points and verify results
    for(psS32 i = 0; i < TESTPOINTS; i++) {
        result = psPolynomial2DEval(polyOrd,Dpoly2DXYValue[i][0],Dpoly2DXYValue[i][1]);
        if(fabs(Dpoly2DResult[i]-result) > ERROR_TOL ) {
            printf("TEST ERROR: Evaluated value %f, should be %f\n", result, Dpoly2DResult[i]);
            testStatus = false;
        }
        resultCheb = psPolynomial2DEval(polyCheb,Dpoly2DXYChebValue[i][0],Dpoly2DXYChebValue[i][1]);
        if(fabs(Dpoly2DChebResult[i]-resultCheb) > ERROR_TOL ) {
            printf("TEST ERROR: Evaluated value %f, should be %f\n", resultCheb, Dpoly2DChebResult[i]);
            testStatus = false;
        }
    }
    psFree(polyOrd);
    psFree(polyCheb);

    // Allocate polynomial with invalid type
    polyOrd = psPolynomial2DAlloc(99, TERMS-1, TERMS-1);
    // Attempt to evaluation invalid polynomial type
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate error message invalid type");
    result = psPolynomial2DEval(polyOrd,0.0, 0.0);
    if ( !isnan(result) ) {
        psError(PS_ERR_UNKNOWN,true,"Did not return NAN for invalid polynomial type");
        testStatus = false;
    }
    psFree(polyOrd);

    return(testStatus);
}

psS32 testPoly2DEvalVector(void)
{
    psBool testStatus = true;
    // Allocate polynomial
    psPolynomial2D* polyOrd = psPolynomial2DAlloc(PS_POLYNOMIAL_ORD, TERMS-1, TERMS-1);
    psPolynomial2D* polyCheb = psPolynomial2DAlloc(PS_POLYNOMIAL_CHEB, TERMS-1, TERMS-1);

    // Set polynomial members
    for(psS32 i = 0; i < TERMS; i++) {
        for(psS32 j = 0; j < TERMS; j++) {
            polyOrd->coeff[i][j] = poly2DCoeff[i][j];
            polyOrd->mask[i][j]  = poly2DMask[i][j];
            polyCheb->coeff[i][j] = 1.0;
            polyCheb->mask[i][j]  = poly2DMask[i][j];
        }
    }

    // Create input vectors
    psVector* inputOrdX  = psVectorAlloc(TESTPOINTS, PS_TYPE_F64);
    psVector* inputOrdY  = psVectorAlloc(TESTPOINTS, PS_TYPE_F64);
    psVector* inputChebX = psVectorAlloc(TESTPOINTS, PS_TYPE_F64);
    psVector* inputChebY = psVectorAlloc(TESTPOINTS, PS_TYPE_F64);
    for(psS32 i = 0; i < TESTPOINTS; i++) {
        inputOrdX->data.F64[i]  = poly2DXYValue[i][0];
        inputOrdY->data.F64[i]  = poly2DXYValue[i][1];
        inputChebX->data.F64[i] = poly2DXYChebValue[i][0];
        inputChebY->data.F64[i] = poly2DXYChebValue[i][1];
        inputOrdX->n++;
        inputOrdY->n++;
        inputChebX->n++;
        inputChebY->n++;
    }

    // Evaluate the vectors
    psVector* outputOrd = psPolynomial2DEvalVector(polyOrd, inputOrdX, inputOrdY);
    if(outputOrd == NULL) {
        printf("TEST ERROR: Unexpected return of NULL.\n");
        testStatus = false;
    }
    if(outputOrd->type.type != PS_TYPE_F64) {
        printf("TEST ERROR: Output vector of type %d expected %d\n", outputOrd->type.type, PS_TYPE_F64);
        testStatus = false;
    }
    psVector* outputCheb = psPolynomial2DEvalVector(polyCheb, inputChebX, inputChebY);
    if(outputCheb == NULL) {
        printf("TEST ERROR: Unexpected return of NULL.\n");
        testStatus = false;
    }
    if(outputCheb->type.type != PS_TYPE_F64) {
        printf("TEST ERROR: Output vector of type %d expected %d.\n", outputCheb->type.type, PS_TYPE_F64);
        testStatus = false;
    }

    // Verify the results
    for(psS32 i = 0; i < TESTPOINTS; i++) {
        if(fabs(poly2DResult[i]-outputOrd->data.F64[i]) > ERROR_TOL) {
            printf("TEST ERROR: Result[%d] %lg not equal to expected %lg.\n",
                   i, outputOrd->data.F64[i], poly2DResult[i]);
            testStatus = false;
        }
        if(fabs(poly2DChebResult[i]-outputCheb->data.F64[i]) > ERROR_TOL) {
            printf("TEST ERROR: ResultCheb[%d] %lg not equal to expected %lg.\n",
                   i, outputCheb->data.F64[i], poly2DChebResult[i]);
            testStatus = false;
        }
    }

    // Attempt to invoke function with null polynomial
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate an error message for NULL polynomial");
    if(psPolynomial2DEvalVector(NULL, inputOrdX, inputOrdY) != NULL) {
        printf("TEST ERROR: Return of NULL expected for NULL polynomial.\n");
        testStatus = false;
    }

    // Attempt to invoke function with null input vector
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate an error message for NULL input vector");
    if(psPolynomial2DEvalVector(polyOrd,NULL,inputOrdY) != NULL) {
        printf("TEST ERROR: Return of NULL expected for NULL input vector.\n");
        testStatus = false;
    }
    // Attempt to invoke function with null input vector
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate an error message for NULL input vector");
    if(psPolynomial2DEvalVector(polyOrd,inputOrdX,NULL) != NULL) {
        printf("TEST ERROR: Return of NULL expected for NULL input vector.\n");
        testStatus = false;
    }

    // Attempt to invoke function with a non F64 type input vector
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate an error message for invalid input type");
    inputOrdX->type.type = PS_TYPE_U8;
    if(psPolynomial2DEvalVector(polyOrd,inputOrdX, inputOrdY) != NULL) {
        printf("TEST ERROR: Return NULL expected for non-F64 input vector.\n");
        testStatus = false;
    }
    inputOrdX->type.type = PS_TYPE_F64;
    // Attempt to invoke function with a non F64 type input vector
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate an error message for invalid input type");
    inputOrdY->type.type = PS_TYPE_U8;
    if(psPolynomial2DEvalVector(polyOrd,inputOrdX, inputOrdY) != NULL) {
        printf("TEST ERROR: Return NULL expected for non-F64 input vector.\n");
        testStatus = false;
    }
    inputOrdY->type.type = PS_TYPE_F64;

    psFree(inputOrdX);
    psFree(inputOrdY);
    psFree(inputChebX);
    psFree(inputChebY);
    psFree(outputOrd);
    psFree(outputCheb);
    psFree(polyOrd);
    psFree(polyCheb);

    return(testStatus);
}

