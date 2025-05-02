/** tst_Func00.c
*
*    This routine must ensure that the psPolynomial structures are
*    allocated and deallocated by the psPolynomialXXXlloc() procedures.
*    It also calls the various psPolynomialXXXEval() procedures.
*
*    The F32 and F64 polynomials are tested for all orders (1 - 4) and for
*    both ordinary and chebyshev polynomials.
*
*    NOTE: This test code requries the stdout file to verify that the results
*    are good.
*
*    XXX: Modify these tests so that polynomials with a variety of different
*    orders are created.
* 
*    XXX: Compare to FLT_EPSILON
* 
*    @version $Revision: 1.1 $  $Name: not supported by cvs2svn $
*    @date $Date: 2006-02-02 21:05:51 $
*
*  Copyright 2004-2005 Maui High Performance Computing Center, Univ. of Hawaii
*
*****************************************************************************/
#include <stdio.h>
#include "pslib_strict.h"
#include "psTest.h"

// Defines
#define ORDER    3

static psS32 testPolynomial1DAlloc(void);
static psS32 testPolynomial2DAlloc(void);
static psS32 testPolynomial3DAlloc(void);
static psS32 testPolynomial4DAlloc(void);

testDescription tests[] = {
                              {testPolynomial1DAlloc,578,"psPolynomial1DAlloc",0,false},
                              {testPolynomial2DAlloc,578,"psPolynomial2DAlloc",0,false},
                              {testPolynomial3DAlloc,578,"psPolynomial3DAlloc",0,false},
                              {testPolynomial4DAlloc,578,"psPolynomial4DAlloc",0,false},
                              {NULL}
                          };


psS32 main(psS32 argc, char* argv[])
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);

    return !runTestSuite(stderr,"psPolynomialXD", tests, argc, argv);
}


// This test will allocate a 1D polynomial and verify the structure allocated
psS32 testPolynomial1DAlloc(void)
{
    psPolynomial1D*  my1DPoly  = NULL;

    // Allocate polynomial
    my1DPoly = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, ORDER);
    // Verify structure allocated
    if(my1DPoly == NULL) {
        psError(PS_ERR_UNKNOWN,true,"Returned NULL not expected");
        return 1;
    }
    // Verify polynomial structure members set properly
    if(my1DPoly->nX != ORDER) {
        psError(PS_ERR_UNKNOWN,true,"Number of terms %d not as expected %d",
                my1DPoly->nX, ORDER);
        return 2;
    }
    if(my1DPoly->type != PS_POLYNOMIAL_ORD) {
        psError(PS_ERR_UNKNOWN,true,"Type %d not as expected %d",
                my1DPoly->type, PS_POLYNOMIAL_ORD);
        return 3;
    }
    for(psS32 i = 0; i < ORDER+1; i++) {
        if(my1DPoly->coeff[i] != 0.0) {
            psError(PS_ERR_UNKNOWN,true,"Coeff[%d] %lg not as expected %lg",
                    i, my1DPoly->coeff[i], 0.0);
            return 4;
        }
        if(my1DPoly->coeffErr[i] != 0.0) {
            psError(PS_ERR_UNKNOWN,true,"CoeffErr[%d] %lg not as expected %lg",
                    i, my1DPoly->coeffErr[i], 0.0);
            return 5;
        }
        if(my1DPoly->mask[i] != 0) {
            psError(PS_ERR_UNKNOWN,true,"Mask[%d] %d not as expected %d",
                    i, my1DPoly->mask[i], 0);
            return 6;
        }
    }
    psFree(my1DPoly);

    if (0) {
        // Attempt to allocate with negative order
        psLogMsg(__func__,PS_LOG_INFO,"Following should generate error msg for negative terms");
        if(psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, -1) != NULL) {
            psError(PS_ERR_UNKNOWN,true,"Returned structure but expected NULL");
            return 7;
        }
    }

    return 0;
}

// This test will allocate a 2D polynomial and verify the structure allocated
psS32 testPolynomial2DAlloc(void)
{
    psPolynomial2D* my2DPoly = NULL;

    // Allocate polynomial
    my2DPoly = psPolynomial2DAlloc(PS_POLYNOMIAL_ORD, ORDER,ORDER+1);
    // Verify structure allocated
    if(my2DPoly == NULL) {
        psError(PS_ERR_UNKNOWN,true,"Returned NULL not expected");
        return 1;
    }
    // Verify polynomial structure members set properly
    if(my2DPoly->nX != ORDER) {
        psError(PS_ERR_UNKNOWN,true,"Number of terms %d not as expected %d",
                my2DPoly->nX, ORDER);
        return 2;
    }
    // Verify polynomial structure members set properly
    if(my2DPoly->nY != ORDER+1) {
        psError(PS_ERR_UNKNOWN,true,"Number of terms %d not as expected %d",
                my2DPoly->nY, ORDER+1);
        return 3;
    }
    if(my2DPoly->type != PS_POLYNOMIAL_ORD) {
        psError(PS_ERR_UNKNOWN,true,"Type %d not as expected %d",
                my2DPoly->type, PS_POLYNOMIAL_ORD);
        return 4;
    }

    for(psS32 i = 0; i < ORDER+1; i++) {
        for(psS32 j = 0; j < ORDER+2; j++) {
            if(fabs(my2DPoly->coeff[i][j]) > FLT_EPSILON) {
                psError(PS_ERR_UNKNOWN,true,"Coeff[%d][%d] %lg not as expected %lg",
                        i, j, my2DPoly->coeff[i][j], 0.0);
                return 5;
            }
            if(my2DPoly->coeffErr[i][j] != 0.0) {
                psError(PS_ERR_UNKNOWN,true,"CoeffErr[%d][%d] %lg not as expected %lg",
                        i, j, my2DPoly->coeffErr[i][j], 0.0);
                return 6;
            }
            if(my2DPoly->mask[i][j] != 0) {
                psError(PS_ERR_UNKNOWN,true,"Mask[%d][%d] %d not as expected %d",
                        i, j, my2DPoly->mask[i][j], 0);
                return 7;
            }
        }
    }
    psFree(my2DPoly);

    if (0) {
        // Attempt to allocate with negative order
        psLogMsg(__func__,PS_LOG_INFO,"Following should generate error msg for negative terms");
        if(psPolynomial2DAlloc(PS_POLYNOMIAL_ORD, -1, 1) != NULL) {
            psError(PS_ERR_UNKNOWN,true,"Returned structure but expected NULL");
            return 8;
        }
        // Attempt to allocate with negative order
        psLogMsg(__func__,PS_LOG_INFO,"Following should generate error msg for negative terms");
        if(psPolynomial2DAlloc(PS_POLYNOMIAL_ORD, 1, -1) != NULL) {
            psError(PS_ERR_UNKNOWN,true,"Returned structure but expected NULL");
            return 9;
        }
    }
    return 0;
}

// This test will allocate a 3D polynomial and verify the structure allocated
psS32 testPolynomial3DAlloc(void)
{
    psPolynomial3D* my3DPoly = NULL;

    // Allocate polynomial
    my3DPoly = psPolynomial3DAlloc(PS_POLYNOMIAL_ORD, ORDER, ORDER+1, ORDER+2);
    // Verify structure allocated
    if(my3DPoly == NULL) {
        psError(PS_ERR_UNKNOWN,true,"Returned NULL not expected");
        return 1;
    }
    // Verify polynomial structure members set properly
    if(my3DPoly->nX != ORDER) {
        psError(PS_ERR_UNKNOWN,true,"Number of terms %d not as expected %d",
                my3DPoly->nX, ORDER);
        return 2;
    }
    // Verify polynomial structure members set properly
    if(my3DPoly->nY != ORDER+1) {
        psError(PS_ERR_UNKNOWN,true,"Number of terms %d not as expected %d",
                my3DPoly->nY, ORDER+1);
        return 3;
    }
    // Verify polynomial structure members set properly
    if(my3DPoly->nZ != ORDER+2) {
        psError(PS_ERR_UNKNOWN,true,"Number of terms %d not as expected %d",
                my3DPoly->nZ, ORDER+2);
        return 4;
    }
    if(my3DPoly->type != PS_POLYNOMIAL_ORD) {
        psError(PS_ERR_UNKNOWN,true,"Type %d not as expected %d",
                my3DPoly->type, PS_POLYNOMIAL_ORD);
        return 5;
    }
    for(psS32 i = 0; i < ORDER+1; i++) {
        for(psS32 j = 0; j < ORDER+2; j++) {
            for(psS32 k = 0; k < ORDER+3; k++) {
                if(my3DPoly->coeff[i][j][k] != 0.0) {
                    psError(PS_ERR_UNKNOWN,true,"Coeff[%d][%d][%d] %lg not as expected %lg",
                            i, j, k, my3DPoly->coeff[i][j][k], 0.0);
                    return 6;
                }
                if(my3DPoly->coeffErr[i][j][k] != 0.0) {
                    psError(PS_ERR_UNKNOWN,true,"CoeffErr[%d][%d][%d] %lg not as expected %lg",
                            i, j, k, my3DPoly->coeffErr[i][j][k], 0.0);
                    return 7;
                }
                if(my3DPoly->mask[i][j][k] != 0) {
                    psError(PS_ERR_UNKNOWN,true,"Mask[%d][%d] %d not as expected %d",
                            i, j, k, my3DPoly->mask[i][j][k], 0);
                    return 8;
                }
            }
        }
    }
    psFree(my3DPoly);

    if (0) {
        // Attempt to allocate with negative order
        psLogMsg(__func__,PS_LOG_INFO,"Following should generate error msg for negative terms");
        if(psPolynomial3DAlloc(PS_POLYNOMIAL_ORD, -1, 1, 1) != NULL) {
            psError(PS_ERR_UNKNOWN,true,"Returned structure but expected NULL");
            return 9;
        }
        // Attempt to allocate with negative order
        psLogMsg(__func__,PS_LOG_INFO,"Following should generate error msg for negative terms");
        if(psPolynomial3DAlloc(PS_POLYNOMIAL_ORD, 1, -1, 1) != NULL) {
            psError(PS_ERR_UNKNOWN,true,"Returned structure but expected NULL");
            return 10;
        }
        // Attempt to allocate with negative order
        psLogMsg(__func__,PS_LOG_INFO,"Following should generate error msg for negative terms");
        if(psPolynomial3DAlloc(PS_POLYNOMIAL_ORD, 1, 1, -1) != NULL) {
            psError(PS_ERR_UNKNOWN,true,"Returned structure but expected NULL");
            return 11;
        }
    }

    return 0;
}

// This test will allocate a 4D polynomial and verify the structure allocated
psS32 testPolynomial4DAlloc(void)
{
    psPolynomial4D* my4DPoly = NULL;

    // Allocate polynomial
    my4DPoly = psPolynomial4DAlloc(PS_POLYNOMIAL_ORD, ORDER,ORDER+1,ORDER+2,ORDER+3);
    // Verify structure allocated
    if(my4DPoly == NULL) {
        psError(PS_ERR_UNKNOWN,true,"Returned NULL not expected");
        return 1;
    }
    // Verify polynomial structure members set properly
    if(my4DPoly->nX != ORDER) {
        psError(PS_ERR_UNKNOWN,true,"Number of terms %d not as expected %d",
                my4DPoly->nX, ORDER);
        return 2;
    }
    // Verify polynomial structure members set properly
    if(my4DPoly->nY != ORDER+1) {
        psError(PS_ERR_UNKNOWN,true,"Number of terms %d not as expected %d",
                my4DPoly->nX, ORDER+1);
        return 3;
    }
    // Verify polynomial structure members set properly
    if(my4DPoly->nZ != ORDER+2) {
        psError(PS_ERR_UNKNOWN,true,"Number of terms %d not as expected %d",
                my4DPoly->nZ, ORDER+2);
        return 4;
    }
    // Verify polynomial structure members set properly
    if(my4DPoly->nT != ORDER+3) {
        psError(PS_ERR_UNKNOWN,true,"Number of terms %d not as expected %d",
                my4DPoly->nT, ORDER+3);
        return 5;
    }
    if(my4DPoly->type != PS_POLYNOMIAL_ORD) {
        psError(PS_ERR_UNKNOWN,true,"Type %d not as expected %d",
                my4DPoly->type, PS_POLYNOMIAL_ORD);
        return 6;
    }
    for(psS32 i = 0; i < ORDER+1; i++) {
        for(psS32 j = 0; j < ORDER+2; j++) {
            for(psS32 k = 0; k < ORDER+3; k++) {
                for(psS32 l = 0; l < ORDER+4; l++) {
                    if(my4DPoly->coeff[i][j][k][l] != 0.0) {
                        psError(PS_ERR_UNKNOWN,true,"Coeff[%d][%d][%d][%d] %lg not as expected %lg",
                                i, j, k, l, my4DPoly->coeff[i][j][k][l], 0.0);
                        return 7;
                    }
                    if(my4DPoly->coeffErr[i][j][k][l] != 0.0) {
                        psError(PS_ERR_UNKNOWN,true,"CoeffErr[%d][%d][%d][l] %lg not as expected %lg",
                                i, j, k, l, my4DPoly->coeffErr[i][j][k][l], 0.0);
                        return 8;
                    }
                    if(my4DPoly->mask[i][j][k][l] != 0) {
                        psError(PS_ERR_UNKNOWN,true,"Mask[%d][%d][%d][%d] %d not as expected %d",
                                i, j, k, l, my4DPoly->mask[i][j][k][l], 0);
                        return 9;
                    }
                }
            }
        }
    }
    psFree(my4DPoly);

    if (0) {
        // Attempt to allocate with negative order
        psLogMsg(__func__,PS_LOG_INFO,"Following should generate error msg for negative terms");
        if(psPolynomial4DAlloc(PS_POLYNOMIAL_ORD, -1, 1, 1, 1) != NULL) {
            psError(PS_ERR_UNKNOWN,true,"Returned structure but expected NULL");
            return 10;
        }
        // Attempt to allocate with negative order
        psLogMsg(__func__,PS_LOG_INFO,"Following should generate error msg for negative terms");
        if(psPolynomial4DAlloc(PS_POLYNOMIAL_ORD, 1, -1, 1, 1) != NULL) {
            psError(PS_ERR_UNKNOWN,true,"Returned structure but expected NULL");
            return 11;
        }
        // Attempt to allocate with negative order
        psLogMsg(__func__,PS_LOG_INFO,"Following should generate error msg for negative terms");
        if(psPolynomial4DAlloc(PS_POLYNOMIAL_ORD, 1, 1, -1, 1) != NULL) {
            psError(PS_ERR_UNKNOWN,true,"Returned structure but expected NULL");
            return 12;
        }
        // Attempt to allocate with negative order
        psLogMsg(__func__,PS_LOG_INFO,"Following should generate error msg for negative terms");
        if(psPolynomial4DAlloc(PS_POLYNOMIAL_ORD, 1, 1, 1, -1) != NULL) {
            psError(PS_ERR_UNKNOWN,true,"Returned structure but expected NULL");
            return 13;
        }
    }

    return 0;
}

