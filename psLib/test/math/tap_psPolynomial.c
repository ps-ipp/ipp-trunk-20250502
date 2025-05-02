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
*    @version $Revision: 1.5 $  $Name: not supported by cvs2svn $
*    @date $Date: 2008-05-05 00:09:04 $
*
*  Copyright 2004-2005 Maui High Performance Computing Center, Univ. of Hawaii
*
*****************************************************************************/
#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include "tap.h"
#include "pstap.h"

#define ORDER    3
#define VERBOSE  0

psS32 main(psS32 argc, char* argv[])
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    plan_tests(59);

    // This test will allocate a 1D polynomial and verify the structure allocated
    {
        psMemId id = psMemGetId();
        psPolynomial1D* my1DPoly  = NULL;

        // Allocate polynomial
        my1DPoly = psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, ORDER);
        ok(my1DPoly != NULL, "1D polynomial allocated successfully");
        skip_start(my1DPoly == NULL, 3, "Skipping tests because psPolynomial1DAlloc() failed");

        // Verify polynomial structure members set properly
        ok(my1DPoly->nX == ORDER, "psPolynomial1DAlloc(): Number of terms set correctly");
        ok(my1DPoly->type == PS_POLYNOMIAL_ORD, "psPolynomial1DAlloc():  type set correctly");
        bool errorFlag = false;
        for(psS32 i = 0; i < ORDER+1; i++)
        {
            if (my1DPoly->coeff[i] != 0.0) {
                diag("Coeff[%d] %lg not as expected %lg", i, my1DPoly->coeff[i], 0.0);
                errorFlag = true;
            }
            if (my1DPoly->coeffErr[i] != 0.0) {
                diag("CoeffErr[%d] %lg not as expected %lg", i, my1DPoly->coeffErr[i], 0.0);
                errorFlag = true;
            }
            if (my1DPoly->coeffMask[i] != 0) {
                diag("Mask[%d] %d not as expected %d", i, my1DPoly->coeffMask[i], 0);
                errorFlag = true;
            }
        }
        ok(!errorFlag, "psPolynomial1D coefficients set correctly");

        skip_end();
        psFree(my1DPoly);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // This test will allocate a Chebyshev 1D polynomial and verify the structure allocated
    {
        psMemId id = psMemGetId();
        psPolynomial1D* my1DPoly  = NULL;
        my1DPoly = psPolynomial1DAlloc(PS_POLYNOMIAL_CHEB, ORDER);
        ok(my1DPoly != NULL, "Chebyshev 1D polynomial allocated successfully");
        skip_start(my1DPoly == NULL, 1, "Skipping tests because psPolynomial1DAlloc() failed");
        ok(my1DPoly->type == PS_POLYNOMIAL_CHEB, "psPolynomial1DAlloc(): Chebyshev  type set correctly");
        skip_end();
        psFree(my1DPoly);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Attempt to allocate with negative order
    // Following should generate error msg for negative terms
    if (1) {
        psMemId id = psMemGetId();
        ok(psPolynomial1DAlloc(PS_POLYNOMIAL_ORD, -1) == NULL,
          "psPolynomial1DAlloc() returned NULL with negative polynomial order");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Allocate polynomial with unallowable type
    {
        psMemId id = psMemGetId();
        psPolynomial1D* polyOrd = psPolynomial1DAlloc(99, ORDER);
        ok(polyOrd==NULL, "psPolynomial1DAlloc() returned NULL with unallowed type");
        psFree(polyOrd);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // This test will allocate a 2D polynomial and verify the structure allocated
    {
        psMemId id = psMemGetId();
        psPolynomial2D* my2DPoly = NULL;

        // Allocate polynomial
        my2DPoly = psPolynomial2DAlloc(PS_POLYNOMIAL_ORD, ORDER,ORDER+1);
        ok(my2DPoly != NULL, "2D polynomial allocated successfully");
        skip_start(my2DPoly == NULL, 4, "Skipping tests because psPolynomial2DAlloc() failed");

        // Verify polynomial structure members set properly
        ok(my2DPoly->nX == ORDER, "psPolynomial2DAlloc(): Number of terms (nX) set correctly");
        ok(my2DPoly->nY == ORDER+1, "psPolynomial2DAlloc(): Number of terms (nY) set correctly");
        ok(my2DPoly->type == PS_POLYNOMIAL_ORD, "psPolynomial2DAlloc(): type set correctly");

        bool errorFlag = false;
        for(psS32 i = 0; i < ORDER+1; i++)
        {
            for(psS32 j = 0; j < ORDER+2; j++) {
                if (fabs(my2DPoly->coeff[i][j]) > FLT_EPSILON) {
                    diag("Coeff[%d][%d] %lg not as expected %lg", i, j, my2DPoly->coeff[i][j], 0.0);
                    errorFlag = true;
                }
                if (my2DPoly->coeffErr[i][j] != 0.0) {
                    diag("CoeffErr[%d][%d] %lg not as expected %lg", i, j, my2DPoly->coeffErr[i][j], 0.0);
                    errorFlag = true;
                }
                if (my2DPoly->coeffMask[i][j] != 0) {
                    diag("Mask[%d][%d] %d not as expected %d", i, j, my2DPoly->coeffMask[i][j], 0);
                    errorFlag = true;
                }
            }
        }
        ok(!errorFlag, "psPolynomial2D coefficients set correctly");
        skip_end();
        psFree(my2DPoly);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // This test will allocate a Chebyshev 2D polynomial and verify the structure allocated
    {
        psMemId id = psMemGetId();
        psPolynomial2D* my2DPoly = NULL;
        my2DPoly = psPolynomial2DAlloc(PS_POLYNOMIAL_CHEB, ORDER,ORDER+1);
        ok(my2DPoly != NULL, "Chebyshev 2D polynomial allocated successfully");
        skip_start(my2DPoly == NULL, 1, "Skipping tests because psPolynomial2DAlloc() failed");
        skip_end();
        psFree(my2DPoly);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Attempt to allocate with negative order
    // Following should generate error msg for negative terms
    if (1) {
        psMemId id = psMemGetId();
        ok(psPolynomial2DAlloc(PS_POLYNOMIAL_ORD, -1, 1) == NULL, 
          "psPolynomial2DAlloc() returned NULL with negative polynomial order");
        ok(psPolynomial2DAlloc(PS_POLYNOMIAL_ORD, 1, -1) == NULL,
          "psPolynomial2DAlloc() returned NULL with negative polynomial order");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Allocate polynomial with unallowed type
    {
        psMemId id = psMemGetId();
        psPolynomial2D* polyOrd = psPolynomial2DAlloc(99, ORDER, ORDER);
        ok(polyOrd == NULL, "psPolynomial2DAlloc() returned NULL with unallowed tpye");
        psFree(polyOrd);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // This test will allocate a 3D polynomial and verify the structure allocated
    {
        psMemId id = psMemGetId();
        psPolynomial3D* my3DPoly = NULL;

        // Allocate polynomial
        my3DPoly = psPolynomial3DAlloc(PS_POLYNOMIAL_ORD, ORDER, ORDER+1, ORDER+2);
        ok(my3DPoly != NULL, "3D polynomial allocated successfully");
        skip_start(my3DPoly == NULL, 5, "Skipping tests because psPolynomial3DAlloc() failed");

        // Verify polynomial structure members set properly
        ok(my3DPoly->nX == ORDER, "psPolynomial3DAlloc(): Number of terms (nX) set correctly");
        ok(my3DPoly->nY == ORDER+1, "psPolynomial3DAlloc(): Number of terms (nY) set correctly");
        ok(my3DPoly->nZ == ORDER+2, "psPolynomial3DAlloc(): Number of terms (nZ) set correctly");
        ok(my3DPoly->type == PS_POLYNOMIAL_ORD, "psPolynomial3DAlloc(): type set correctly");

        bool errorFlag = false;
        for(psS32 i = 0; i < ORDER+1; i++)
        {
            for(psS32 j = 0; j < ORDER+2; j++) {
                for(psS32 k = 0; k < ORDER+3; k++) {
                    if (my3DPoly->coeff[i][j][k] != 0.0) {
                        diag("Coeff[%d][%d][%d] %lg not as expected %lg",
                             i, j, k, my3DPoly->coeff[i][j][k], 0.0);
                        errorFlag = true;
                    }
                    if (my3DPoly->coeffErr[i][j][k] != 0.0) {
                        diag("CoeffErr[%d][%d][%d] %lg not as expected %lg",
                             i, j, k, my3DPoly->coeffErr[i][j][k], 0.0);
                        errorFlag = true;
                    }
                    if (my3DPoly->coeffMask[i][j][k] != 0) {
                        diag("Mask[%d][%d][%d] %d not as expected %d",
                             i, j, k, my3DPoly->coeffMask[i][j][k], 0);
                        errorFlag = true;
                    }
                }
            }
        }
        ok(!errorFlag, "psPolynomial3D coefficients set correctly");
        skip_end();
        psFree(my3DPoly);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // This test will allocate a Chebyshev 3D polynomial and verify the structure allocated
    {
        psMemId id = psMemGetId();
        psPolynomial3D* my3DPoly = NULL;
        my3DPoly = psPolynomial3DAlloc(PS_POLYNOMIAL_CHEB, ORDER, ORDER+1, ORDER+2);
        ok(my3DPoly != NULL, "Chebyshev 3D polynomial allocated successfully");
        skip_start(my3DPoly == NULL, 1, "Skipping tests because psPolynomial2DAlloc() failed");
        ok(my3DPoly->type == PS_POLYNOMIAL_CHEB, "psPolynomial3DAlloc(): Chebyshev type set correctly");
        skip_end();
        psFree(my3DPoly);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Attempt to allocate with negative order
    // Following should generate error msg for negative terms
    if (1) {
        psMemId id = psMemGetId();
        ok(psPolynomial3DAlloc(PS_POLYNOMIAL_ORD, -1, 1, 1) == NULL, 
          "psPolynomial3DAlloc() returned NULL with negative polynomial order");
        ok(psPolynomial3DAlloc(PS_POLYNOMIAL_ORD, 1, -1, 1) == NULL, 
          "psPolynomial3DAlloc() returned NULL with negative polynomial order");
        ok(psPolynomial3DAlloc(PS_POLYNOMIAL_ORD, 1, 1, -1) == NULL,
          "psPolynomial3DAlloc() returned NULL with negative polynomial order");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Allocate polynomial with unallowed type
    {
        psMemId id = psMemGetId();
        psPolynomial3D* polyOrd = psPolynomial3DAlloc(99, ORDER, ORDER, ORDER);
        ok(polyOrd == NULL, "psPolynomial3DAlloc() returned NULL with unallowed type");
        psFree(polyOrd);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");

    }


    // This test will allocate a 4D polynomial and verify the structure allocated
    {
        psMemId id = psMemGetId();
        psPolynomial4D* my4DPoly = NULL;

        // Allocate polynomial
        my4DPoly = psPolynomial4DAlloc(PS_POLYNOMIAL_ORD, ORDER,ORDER+1,ORDER+2,ORDER+3);
        ok(my4DPoly != NULL, "4D polynomial allocated successfully");
        skip_start(my4DPoly == NULL, 6, "Skipping tests because psPolynomial4DAlloc() failed");

        // Verify polynomial structure members set properly
        ok(my4DPoly->nX == ORDER, "psPolynomial4DAlloc(): Number of terms (nX) set correctly");
        ok(my4DPoly->nY == ORDER+1, "psPolynomial4DAlloc(): Number of terms (nY) set correctly");
        ok(my4DPoly->nZ == ORDER+2, "psPolynomial4DAlloc(): Number of terms (nZ) set correctly");
        ok(my4DPoly->nT == ORDER+3, "psPolynomial4DAlloc(): Number of terms (nT) set correctly");
        ok(my4DPoly->type == PS_POLYNOMIAL_ORD, "psPolynomial4D    Alloc(): type set correctly");

        bool errorFlag = false;
        for(psS32 i = 0; i < ORDER+1; i++)
        {
            for(psS32 j = 0; j < ORDER+2; j++) {
                for(psS32 k = 0; k < ORDER+3; k++) {
                    for(psS32 l = 0; l < ORDER+4; l++) {
                        if (my4DPoly->coeff[i][j][k][l] != 0.0) {
                            diag("Coeff[%d][%d][%d][%d] %lg not as expected %lg",
                                 i, j, k, l, my4DPoly->coeff[i][j][k][l], 0.0);
                            errorFlag = true;
                        }
                        if (my4DPoly->coeffErr[i][j][k][l] != 0.0) {
                            diag("CoeffErr[%d][%d][%d][%d] %lg not as expected %lg",
                                 i, j, k, l, my4DPoly->coeffErr[i][j][k][l], 0.0);
                            errorFlag = true;
                        }
                        if (my4DPoly->coeffMask[i][j][k][l] != 0) {
                            diag("Mask[%d][%d][%d][%d] %d not as expected %d",
                                 i, j, k, l, my4DPoly->coeffMask[i][j][k][l], 0);
                            errorFlag = true;
                        }
                    }
                }
            }
        }
        ok(!errorFlag, "psPolynomial4D coefficients set correctly");
        skip_end();
        psFree(my4DPoly);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // This test will allocate a Chebyshev 4D polynomial and verify the structure allocated
    {
        psMemId id = psMemGetId();
        psPolynomial4D* my4DPoly = NULL;
        my4DPoly = psPolynomial4DAlloc(PS_POLYNOMIAL_CHEB, ORDER,ORDER+1,ORDER+2,ORDER+3);
        ok(my4DPoly != NULL, "Chebyshev 4D polynomial allocated successfully");
        skip_start(my4DPoly == NULL, 1, "Skipping tests because psPolynomial4DAlloc() failed");
        ok(my4DPoly->type == PS_POLYNOMIAL_CHEB, "psPolynomial4DAlloc(): Chebyshev type set correctly");
        skip_end();
        psFree(my4DPoly);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Attempt to allocate with negative order
    // Following should generate error msg for negative terms
    if (1) {
        psMemId id = psMemGetId();
        ok(psPolynomial4DAlloc(PS_POLYNOMIAL_ORD, -1, 1, 1, 1) == NULL, 
          "psPolynomial4DAlloc() returned NULL with negative polynomial order");
        ok(psPolynomial4DAlloc(PS_POLYNOMIAL_ORD, 1, -1, 1, 1) == NULL, 
          "psPolynomial4DAlloc() returned NULL with negative polynomial order");
        ok(psPolynomial4DAlloc(PS_POLYNOMIAL_ORD, 1, 1, -1, 1) == NULL,
          "psPolynomial4DAlloc() returned NULL with negative polynomial order");
        ok(psPolynomial4DAlloc(PS_POLYNOMIAL_ORD, 1, 1, 1, -1) == NULL,
          "psPolynomial4DAlloc() returned NULL with negative polynomial order");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Allocate polynomial with unallowed type
    {
        psMemId id = psMemGetId();
        psPolynomial4D* polyOrd = psPolynomial4DAlloc(99, ORDER, ORDER, ORDER, ORDER);
        ok(polyOrd == NULL, "psPolynomial4DAlloc() returned NULL with unallowed type");
        psFree(polyOrd);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
}

