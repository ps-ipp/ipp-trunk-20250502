/** tst_psFunc08.c
*
*  This test driver will exercise the psPolynomialXDEval functions for both
*  ORD and CHEB type polynomials.
*
*  @version  $Revision: 1.2 $  $Name: not supported by cvs2svn $
*  @date  $Date: 2006-02-24 23:43:15 $
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

#include "pslib_strict.h"
#include "psTest.h"

#define  TERMS        4
#define  TESTPOINTS   5
#define  ERROR_TOL    0.001

static psS32 testPoly1DEval(void);
static psS32 testPoly1DEvalVector(void);

testDescription tests[] = {
                              {testPoly1DEval,000,"psPolynomial1DEval",0,false},
                              {testPoly1DEvalVector,000,"psPolynomial1DEvalVector",0,false},
                              {NULL}
                          };

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

    return !runTestSuite(stderr,"psPolynomialXDEval", tests, argc, argv);
}

// This test will verify operation of 1D polynomial evaluation
psS32 testPoly1DEval(void)
{
    psF64  result;
    psF64  resultCheb;

    // Allocate polynomial structure
    psPolynomial1D*  polyOrd = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, TERMS-1);
    psPolynomial1D*  polyCheb = psPolynomial1DAlloc(PS_POLYNOMIAL_CHEB, TERMS-1);
    // Set polynomial members
    for(psS32 i = 0; i < TERMS; i++) {
        polyOrd->coeff[i] = poly1DCoeff[i];
        polyOrd->mask[i]  = poly1DMask[i];
        polyCheb->coeff[i] = 1.0;
        polyCheb->mask[i]  = poly1DMask[i];
    }
    // Evaluate test points and verify results
    for(psS32 i = 0; i < TESTPOINTS; i++) {
        result = psPolynomial1DEval(polyOrd,poly1DXValue[i]);
        if(fabs(poly1DXResult[i]-result) > ERROR_TOL ) {
            psError(PS_ERR_UNKNOWN,true,"Evaluated value %g not as expected %g",
                    result, poly1DXResult[i]);
            return i;
        }
        resultCheb = psPolynomial1DEval(polyCheb,poly1DXChebValue[i]);
        if(fabs(poly1DXChebResult[i]-resultCheb) > ERROR_TOL ) {
            psError(PS_ERR_UNKNOWN,true,"Evaluated Chebyshev value %lg not as expected %lg",
                    resultCheb, poly1DXChebResult[i]);
            return 5*i;
        }
    }
    psFree(polyOrd);
    psFree(polyCheb);

    // Allocate polynomial with invalid type
    polyOrd = psPolynomial1DAlloc(99, TERMS-1);
    // Attempt to evaluation invalid polynomial type
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate error message invalid type");
    result = psPolynomial1DEval(polyOrd,0.0);
    if ( !isnan(result) ) {
        psError(PS_ERR_UNKNOWN,true,"Did not return NAN for invalid polynomial type");
        return 20;
    }
    psFree(polyOrd);

    return 0;
}


psS32 testPoly1DEvalVector(void)
{
    // Allocate polynomial
    psPolynomial1D* polyOrd = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, TERMS-1);
    psPolynomial1D* polyCheb = psPolynomial1DAlloc(PS_POLYNOMIAL_CHEB, TERMS-1);

    // Set polynomial members
    for(psS32 i = 0; i < TERMS; i++) {
        polyOrd->coeff[i] = poly1DCoeff[i];
        polyOrd->mask[i]  = poly1DMask[i];
        polyCheb->coeff[i] = 1.0;
        polyCheb->mask[i]  = poly1DMask[i];
    }

    // Create input vectors
    psVector* inputOrd = psVectorAlloc(TESTPOINTS, PS_TYPE_F64);
    psVector* inputCheb = psVectorAlloc(TESTPOINTS, PS_TYPE_F64);
    for(psS32 i = 0; i < TESTPOINTS; i++) {
        inputOrd->data.F64[i] = poly1DXValue[i];
        inputCheb->data.F64[i] = poly1DXChebValue[i];
        inputOrd->n++;
        inputCheb->n++;
    }

    // Evaluate the vectors
    psVector* outputOrd = psPolynomial1DEvalVector(polyOrd, inputOrd);
    if(outputOrd == NULL) {
        psError(PS_ERR_UNKNOWN,true,"Unexpected return of NULL.");
        return 1;
    }
    if(outputOrd->type.type != PS_TYPE_F64) {
        psError(PS_ERR_UNKNOWN,true,"Output vector of type %d expected %d",
                outputOrd->type.type, PS_TYPE_F64);
        return 2;
    }
    psVector* outputCheb = psPolynomial1DEvalVector(polyCheb, inputCheb);
    if(outputCheb == NULL) {
        psError(PS_ERR_UNKNOWN,true,"Unexpected return of NULL.");
        return 1;
    }
    if(outputCheb->type.type != PS_TYPE_F64) {
        psError(PS_ERR_UNKNOWN,true,"Output vector of type %d expected %d",
                outputCheb->type.type, PS_TYPE_F64);
        return 2;
    }

    // Verify the results
    for(psS32 i = 0; i < TESTPOINTS; i++) {
        if(fabs(poly1DXResult[i]-outputOrd->data.F64[i]) > ERROR_TOL) {
            psError(PS_ERR_UNKNOWN,true,"Result[%d] %lg not equal to expected %lg",
                    i, outputOrd->data.F64[i], poly1DXResult[i]);
            return i*5;
        }
        if(fabs(poly1DXChebResult[i]-outputCheb->data.F64[i]) > ERROR_TOL) {
            psError(PS_ERR_UNKNOWN,true,"ResultCheb[%d] %lg not equal to expected %lg",
                    i, outputCheb->data.F64[i], poly1DXChebResult[i]);
            return i*10;
        }
    }

    // Attempt to invoke function with null polynomial
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate an error message for NULL polynomial");
    if(psPolynomial1DEvalVector(NULL, inputOrd) != NULL) {
        psError(PS_ERR_UNKNOWN,true,"Return of NULL expected for NULL polynomial");
        return 60;
    }

    // Attempt to invoke function with null input vector
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate an error message for NULL input vector");
    if(psPolynomial1DEvalVector(polyOrd,NULL) != NULL) {
        psError(PS_ERR_UNKNOWN,true,"Return of NULL expected for NULL input vector");
        return 61;
    }

    // Attempt to invoke function with a non F64 type input vector
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate an error message for invalid input type");
    inputOrd->type.type = PS_TYPE_U8;
    if(psPolynomial1DEvalVector(polyOrd,inputOrd) != NULL) {
        psError(PS_ERR_UNKNOWN,true,"Return NULL expected for non-F64 input vector");
        return 62;
    }
    inputOrd->type.type = PS_TYPE_F64;

    psFree(inputOrd);
    psFree(inputCheb);
    psFree(outputOrd);
    psFree(outputCheb);
    psFree(polyOrd);
    psFree(polyCheb);

    return 0;
}
