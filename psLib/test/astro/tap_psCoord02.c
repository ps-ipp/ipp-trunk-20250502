/** @file  tst_psCoord02.c
*
*  @brief This file contains test code for:
*
*  @author GLG, MHPCC
*
*  XXX: These tests should probably be split among several files
*
*  @version $Revision: 1.4 $ $Name: not supported by cvs2svn $
*  @date $Date: 2007-06-05 01:10:22 $
*
*  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
*/
#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include "tap.h"
#include "pstap.h"

#define PS_PERCENT_COMPARE(X, Y, PERCENT_FRACTION) (fabs((Y)-(X))/fabs(X) < (PERCENT_FRACTION))
#define PS_GEN_RAN_FLOAT (-0.5 + ((psF32)rand()) / ((psF32) RAND_MAX))
#define RANDOM_SEED 1995

// These macros determine the number of x/y test points which will be x-formed.
#define TST04_X_MIN 0.0
#define TST04_X_MAX 10.0
#define TST04_Y_MIN 0.0
#define TST04_Y_MAX 10.0
#define TST04_NUM 5.0
// These macros define the max sizes of the various input transforms.
#define TST04_T1_X_X 2
#define TST04_T1_X_Y 3
#define TST04_T1_Y_X 3
#define TST04_T1_Y_Y 3
#define TST04_T2_X_X 3
#define TST04_T2_X_Y 3
#define TST04_T2_Y_X 3
#define TST04_T2_Y_Y 3

/******************************************************************************
tstTransforms(t1, t2, t3): this function generates a set of arbitrary x/y
coordinates, then transforms them into output coordinates via the t3
transform, as well as the t1 followed by the t2 transform.  It verifies the
output coordinates are identical and returns TRUE if so.  Otherwise, it prints
a diag message and returns FALSE.
 *****************************************************************************/
bool tstTransforms(psPlaneTransform *t1, psPlaneTransform *t2, psPlaneTransform *t3)
{
    bool testStatus = true;
    psPlane *in = psPlaneAlloc();
    in->xErr = 0.0;
    in->yErr = 0.0;

    for (in->x = TST04_X_MIN ; in->x <= TST04_X_MAX ; in->x+= (TST04_X_MAX-TST04_X_MIN) / TST04_NUM) {
        for (in->y = TST04_Y_MIN ; in->y <= TST04_Y_MAX ; in->y+= (TST04_Y_MAX-TST04_Y_MIN) / TST04_NUM) {
            // Apply the t1/t2 transforms individually to the in coords.
            psPlane *mid = psPlaneTransformApply(NULL, t1, in);
            if (mid == NULL) {
                diag("TEST ERROR: intermediate psPlane coords are NULL.\n");
                psFree(in);
                return(false);
            }

            psPlane *outT1T2 = psPlaneTransformApply(NULL, t2, mid);
            if (outT1T2 == NULL) {
                diag("TEST ERROR: intermediate psPlane coords are NULL.\n");
                psFree(mid);
                return(false);
            }

            // Apply the t3 transforms individually to the in coords.
            psPlane *outT3 = psPlaneTransformApply(NULL, t3, in);
            if (outT3 == NULL) {
                diag("TEST ERROR: intermediate psPlane coords are NULL.\n");
                psFree(mid);
                psFree(outT1T2);
                return(false);
            }

            // Verify that the results are identical.
            if (!PS_PERCENT_COMPARE(outT3->x, outT1T2->x, 0.01)) {
                diag("TEST ERROR: x is %f, should be %f\n", outT3->x, outT1T2->x);
                testStatus = false;
            }
            // Verify that the results are identical.
            if (!PS_PERCENT_COMPARE(outT3->y, outT1T2->y, 0.01)) {
                diag("TEST ERROR: y is %f, should be %f\n", outT3->y, outT1T2->y);
                testStatus = false;
            }
            if (0) {
                if (fabs(outT3->x - outT1T2->x) > FLT_EPSILON) {
                    printf("TEST ERROR: x is %f, should be %f\n", outT3->x, outT1T2->x);
                    testStatus = false;
                    printf("(in->x, in->y) is (%f, %f)\n", in->x, in->y);
                    printf("(mid->x, mid->y) is (%f, %f)\n", mid->x, mid->y);
                    printf("(outT1T2->x, outT1T2->y) is (%f, %f)\n", outT1T2->x, outT1T2->y);
                    printf("(outT3->x, outT3->y) is (%f, %f)\n", outT3->x, outT3->y);
                    PS_POLY_PRINT_2D(t1->x);
                    PS_POLY_PRINT_2D(t1->y);
                    PS_POLY_PRINT_2D(t2->x);
                    PS_POLY_PRINT_2D(t2->y);
                    PS_POLY_PRINT_2D(t3->x);
                    PS_POLY_PRINT_2D(t3->y);
                }
                if (fabs(outT3->y - outT1T2->y) > FLT_EPSILON) {
                    printf("TEST ERROR: y is %f, should be %f\n", outT3->y, outT1T2->y);
                    testStatus = false;
                    printf("(in->x, in->y) is (%f, %f)\n", in->x, in->y);
                    printf("(mid->x, mid->y) is (%f, %f)\n", mid->x, mid->y);
                    printf("(outT1T2->x, outT1T2->y) is (%f, %f)\n", outT1T2->x, outT1T2->y);
                    printf("(outT3->x, outT3->y) is (%f, %f)\n", outT3->x, outT3->y);
                    PS_POLY_PRINT_2D(t1->x);
                    PS_POLY_PRINT_2D(t1->y);
                    PS_POLY_PRINT_2D(t2->x);
                    PS_POLY_PRINT_2D(t2->y);
                    PS_POLY_PRINT_2D(t3->x);
                    PS_POLY_PRINT_2D(t3->y);
                }
            }
            psFree(mid);
            psFree(outT1T2);
            psFree(outT3);
        }
    }

    psFree(in);
    return(testStatus);
}

/******************************************************************************
setCoeffs(t1, t2): this function sets the coefficients of the t1 and t2
transforms to somewhat arbitrary values.
 *****************************************************************************/
int setCoeffs(psPlaneTransform *t1, psPlaneTransform *t2)
{
    for (psS32 t1xx = 0 ; t1xx < t1->x->nX+1 ; t1xx++) {
        for (psS32 t1xy = 0 ; t1xy < t1->x->nY+1 ; t1xy++) {
            t1->x->coeff[t1xx][t1xy] = PS_GEN_RAN_FLOAT;
        }
    }

    for (psS32 t1yx = 0 ; t1yx < t1->y->nX+1 ; t1yx++) {
        for (psS32 t1yy = 0 ; t1yy < t1->y->nY+1 ; t1yy++) {
            t1->y->coeff[t1yx][t1yy] = PS_GEN_RAN_FLOAT;
        }
    }
    for (psS32 t2xx = 0 ; t2xx < t2->x->nX+1 ; t2xx++) {
        for (psS32 t2xy = 0 ; t2xy < t2->x->nY+1 ; t2xy++) {
            t2->x->coeff[t2xx][t2xy] = PS_GEN_RAN_FLOAT;
        }
    }
    for (psS32 t2yx = 0 ; t2yx < t2->y->nX+1 ; t2yx++) {
        for (psS32 t2yy = 0 ; t2yy < t2->y->nY+1 ; t2yy++) {
            t2->y->coeff[t2yx][t2yy] = PS_GEN_RAN_FLOAT;
        }
    }
    return(0);
}

psS32 main( psS32 argc, char* argv[] )
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    plan_tests(25);
    srand(RANDOM_SEED);

    // We test psPlaneTransformCombine() with a variety of input transforms
    // as well as input coordinates
    //
    //    The strategy here is to generate a pair of transforms t1 and t2 with a wide
    //    variety of sizes for the x/y components of the x/y transforms, then set the
    //    coefficients of t1/t2 to arbitray values, then generate a new transform t3 via
    //    psPlaneTransformCombine() function.  Then, for several arbitrary input plane
    //    coordinates, we tarnsform them via the new transform t3, as well as
    //    individually via t1 then t2, and verify that they produce identical output
    //    coordinates.
    //
    {
        psMemId id = psMemGetId();
        bool errorFlag = false;
        for (psS32 t1xx = 0 ; t1xx < TST04_T1_X_X ; t1xx++)
        {
            for (psS32 t1xy = 0 ; t1xy < TST04_T1_X_Y ; t1xy++) {
                for (psS32 t1yx = 0 ; t1yx < TST04_T1_Y_X ; t1yx++) {
                    for (psS32 t1yy = 0 ; t1yy < TST04_T1_Y_Y ; t1yy++) {
                        for (psS32 t2xx = 0 ; t2xx < TST04_T2_X_X ; t2xx++) {
                            for (psS32 t2xy = 0 ; t2xy < TST04_T2_X_Y ; t2xy++) {
                                for (psS32 t2yx = 0 ; t2yx < TST04_T2_Y_X ; t2yx++) {
                                    for (psS32 t2yy = 0 ; t2yy < TST04_T2_Y_Y ; t2yy++) {
                                        //printf("(%d %d %d %d %d %d %d %d)\n", t1xx, t1xy, t1yx, t1yy, t2xx, t2xy, t2yx, t2yy);
                                        psPlaneTransform *trans1 = psPlaneTransformAlloc(PS_MAX(t1xx, t1yx), PS_MAX(t1xy, t1yy));
                                        psPlaneTransform *trans2 = psPlaneTransformAlloc(PS_MAX(t2xx, t2yx), PS_MAX(t2xy, t2yy));
                                        psPlaneTransform *trans3 = NULL;
                                        setCoeffs(trans1, trans2);
                                        trans3 = psPlaneTransformCombine(NULL, trans1,
                                                                         trans2, psRegionSet(NAN,NAN,NAN,NAN), 0);
                                        //XXX: the last two parameters are bogus.  Needs to change.  -rdd

                                        if (trans3 == NULL) {
                                            diag("TEST ERROR: psPlaneTransformCombine() returned NULL");
                                            errorFlag = true;
                                        } else {
                                            bool testStatus = tstTransforms(trans1, trans2, trans3);
                                            if (!testStatus) {
                                                diag("TEST ERROR: psPlaneTransformCombine() did not produce correct results");
                                                errorFlag = true;
                                            }
                                        }
                                        psFree(trans1);
                                        psFree(trans2);
                                        psFree(trans3);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        ok(!errorFlag, "psPlaneTransformCombine() successful on a variety of transforms and data points");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Calling psPlaneTransformCombine with NULL trans1
    // Should generate error and return NULL
    // XXX: We do not test error generation
    {
        psMemId id = psMemGetId();
        psPlaneTransform *trans2 = psPlaneTransformAlloc(2, 2);
        psPlaneTransform *trans3 = psPlaneTransformCombine(NULL, NULL, trans2, psRegionSet(0,0,0,0), 10);
        ok(trans3 == NULL, "psPlaneTransformCombine() returned NULL with NULL trans1 input");
        psFree(trans2);
        psFree(trans3);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Calling psPlaneTransformCombine with NULL trans2
    // Should generate error and return NULL.\n");
    // XXX: We do not test error generation
    {
        psMemId id = psMemGetId();
        psPlaneTransform *trans1 = psPlaneTransformAlloc(2, 2);
        psPlaneTransform *trans3 = psPlaneTransformCombine(NULL, trans1, NULL, psRegionSet(0,0,0,0), 10);
        ok(trans3 == NULL, "psPlaneTransformCombine() returned NULL with NULL trans2 input");
        psFree(trans1);
        psFree(trans3);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // XXX: Clean this, put it elsewhere
    #define TST05_X_MIN 0.0
    #define TST05_X_MAX 10.0
    #define TST05_Y_MIN 0.0
    #define TST05_Y_MAX 10.0
    #define TST05_NUM_X 5.0
    #define TST05_NUM_Y 5.0
    #define TST05_X_POLY_ORDER 3
    #define TST05_Y_POLY_ORDER 3
    #define TST05_NUM_DATA (TST05_NUM_X * TST05_NUM_Y)
    #define TST05_IGNORE 1
    #define TST05_VERBOSE 0

    // Calling psPlaneTransformFit with NULL trans
    // Should generate error and return NULL
    // XXX: We do not test error generation
    {
        psArray *src = psArrayAlloc((int) TST05_NUM_DATA);
        psArray *dst = psArrayAlloc((int) TST05_NUM_DATA);
        psMemId id = psMemGetId();
        ok(!psPlaneTransformFit(NULL, src, dst, 100, 100.0), "psPlaneTransformFit() returned FALSE with NULL trans");
        psFree(src);
        psFree(dst);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Calling psPlaneTransformFit with NULL src psArray
    // Should generate error and return NULL
    // XXX: We do not test error generation
    {
        psMemId id = psMemGetId();
        psPlaneTransform *trans = psPlaneTransformAlloc(TST05_X_POLY_ORDER, TST05_Y_POLY_ORDER);
        psArray *dst = psArrayAlloc((int) TST05_NUM_DATA);
        ok(!psPlaneTransformFit(trans, NULL, dst, 100, 100.0), "psPlaneTransformFit() returned FALSE with NULL src psArray");
        psFree(trans);
        psFree(dst);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Calling psPlaneTransformFit with NULL dst psArray
    // Should generate error and return NULL
    // XXX: We do not test error generation
    {
        psMemId id = psMemGetId();
        psPlaneTransform *trans = psPlaneTransformAlloc(TST05_X_POLY_ORDER, TST05_Y_POLY_ORDER);
        psArray *src = psArrayAlloc((int) TST05_NUM_DATA);
        ok(!psPlaneTransformFit(trans, src, NULL, 100, 100.0), "psPlaneTransformFit() returned FALSE with NULL dst psArray");
        psFree(trans);
        psFree(src);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // psPlaneTransformFit()
    // XXX: This is only a rudimentary test of the psPlaneTransformFit() function.
    //      It tests some simple linear transformations.
    {
        psMemId id = psMemGetId();
        psPlaneTransform *trans = psPlaneTransformAlloc(TST05_X_POLY_ORDER, TST05_Y_POLY_ORDER);
        psPlaneTransform *transInit = psPlaneTransformAlloc(TST05_X_POLY_ORDER, TST05_Y_POLY_ORDER);
        psArray *src = psArrayAlloc((int) TST05_NUM_DATA);
        psArray *dst = psArrayAlloc((int) TST05_NUM_DATA);
        bool rc;

        // We set an arbitrary non-linear transformation.
        for (psS32 x = 0 ; x < TST05_X_POLY_ORDER+1 ; x++)
        {
            for (psS32 y = 0 ; y < TST05_Y_POLY_ORDER+1 ; y++) {
                transInit->x->coeff[x][y] = (psF32)rand() / (psF32)RAND_MAX * 10.0f;
                transInit->y->coeff[x][y] = (psF32)rand() / (psF32)RAND_MAX * 10.0f;
            }
        }
        if (TST05_VERBOSE)
        {
            PS_POLY_PRINT_2D(transInit->x);
            PS_POLY_PRINT_2D(transInit->y);
        }

        // We generate a grid of input data points in the x1,y1 plane, calculate the
        // corresponding values in the transformed plane, and set these to
        // the src and dst psVectors.
        psS32 i = 0;
        for (psF32 x = TST05_X_MIN ; x < TST05_X_MAX ; x+= (TST05_X_MAX-TST05_X_MIN)/TST05_NUM_X)
        {
            for (psF32 y = TST05_Y_MIN ; y < TST05_Y_MAX ; y+= (TST05_Y_MAX-TST05_Y_MIN)/TST05_NUM_Y) {
                if (i == src->n) {
                    if (src->n == src->nalloc) {
                        src = psArrayRealloc(src, src->n+1);
                    }
                    src->n++;
                }
                if (i == dst->n) {
                    if (dst->n == dst->nalloc) {
                        dst = psArrayRealloc(dst, dst->n+1);
                    }
                    dst->n++;
                }
                src->data[i] = (psPtr *) psPlaneAlloc();
                ((psPlane *) src->data[i])->x = x;
                ((psPlane *) src->data[i])->y = y;

                dst->data[i] = psPlaneTransformApply(NULL, transInit, ((psPlane *) src->data[i]));
                i++;
            }
        }

        // Call psPlaneTransformFit with above data arrays
        rc = psPlaneTransformFit(trans, src, dst, 100, 100.0);
        ok(rc, "psPlaneTransformFit() returned TRUE");
        skip_start(!rc, 1, "Skipping tests because psPlaneTransformFit() returned FALSE");
        if (TST05_VERBOSE)
        {
            PS_POLY_PRINT_2D(trans->x);
            PS_POLY_PRINT_2D(trans->y);
        }

        // For the initial grid of input points, we transform them to output points with
        // the derived transformation, and verify that they are within 10%.
        bool errorFlag = false;
        for (psS32 i = TST05_IGNORE ; i < src->n-TST05_IGNORE ; i++)
        {
            psPlane *inData = (psPlane *) src->data[i];
            psPlane *outData = (psPlane *) dst->data[i];
            psPlane *outDataDeriv = psPlaneTransformApply(NULL, trans, inData);
            if (!PS_PERCENT_COMPARE(outDataDeriv->x, outData->x, 0.20) ||
                    !PS_PERCENT_COMPARE(outDataDeriv->y, outData->y, 0.20)) {
                diag("TEST ERROR: the derived output coords (%d) were (%.2f, %.2f) should have been (%.2f, %.2f).\n",
                     i, outDataDeriv->x, outDataDeriv->y, outData->x, outData->y);
                errorFlag = true;
            } else if (TST05_VERBOSE) {
                diag("GOOD: the derived output coords (%d) were (%.2f, %.2f) should have been (%.2f, %.2f).\n",
                     i, outDataDeriv->x, outDataDeriv->y, outData->x, outData->y);
            }
            psFree(outDataDeriv);
        }
        ok(!errorFlag, "The derived transformation was correct");
        skip_end();
        psFree(transInit);
        psFree(trans);
        psFree(src);
        psFree(dst);
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }


    // psPlaneTransformInvert()
    // Calling psPlaneTransformInvert with NULL trans
    // Should generate error and return NULL
    // XXX: We do not test error generation
    {
        psMemId id = psMemGetId();
        psRegion myRegion = psRegionSet(1.0, 5.0, 1.0, 5.0);
        psPlaneTransform *transInverse = psPlaneTransformInvert(NULL, NULL, myRegion, 10);
        ok(transInverse == NULL, "psPlaneTransformInvert() returned NULL with NULL trans");
        psFree(transInverse);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // psPlaneTransformInvert()
    // Calling psPlaneTransformInvert with zero nSamples
    // Should generate error and return NULL
    // XXX: We do not test error generation
    {
        psMemId id = psMemGetId();
        psPlaneTransform *trans = psPlaneTransformAlloc(1, 1);
        psRegion myRegion = psRegionSet(1.0, 5.0, 1.0, 5.0);
        psPlaneTransform *transInverse = psPlaneTransformInvert(NULL, trans, myRegion, 0);
        ok(transInverse == NULL, "psPlaneTransformInvert() returned NULL with zero nSamples");
        psFree(trans);
        psFree(transInverse);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    #define NUM_TRANSFORMS 100
    // psPlaneTransformInvert()
    // This is only a rudimentary test of the psPlaneTransformInvert() function.
    // It tests: Several random linear transformations.
    // XXX: Must extensively test non-linear transformations.
    {
        psMemId id = psMemGetId();
        bool errorFlag = false;
        psPlaneTransform *trans = psPlaneTransformAlloc(1, 1);
        psPlaneTransform *transInverse = NULL;
        psRegion myRegion = psRegionSet(1.0, 5.0, 1.0, 5.0);

        trans->x->coeff[0][0] = 0.0;
        trans->x->coeff[0][1] = 1.0;
        trans->x->coeff[1][0] = 2.0;
        trans->x->coeff[1][1] = 3.0;
        trans->y->coeff[0][0] = 4.0;
        trans->y->coeff[0][1] = 5.0;
        trans->y->coeff[1][0] = 6.0;
        trans->y->coeff[1][1] = 7.0;

        // We calling psPlaneTransformInvert with acceptable linear transformations
        for (psS32 n = 0 ; n < NUM_TRANSFORMS ; n++)
        {
            if (n == 0) {
                // I ensure that we test the identity transformation since this was
                // giving us probs before.
                trans->x->coeff[0][0] = 0.0;
                trans->x->coeff[0][1] = 0.0;
                trans->x->coeff[1][0] = 1.0;
                trans->x->coeff[1][1] = 0.0;
                trans->y->coeff[0][0] = 0.0;
                trans->y->coeff[0][1] = 1.0;
                trans->y->coeff[1][0] = 0.0;
                trans->y->coeff[1][1] = 0.0;
            } else {
                // We create a random linear transformation and hope it is invertible.
                trans->x->coeff[0][0] = (psF32)rand() / (psF32)RAND_MAX * 10.0f;
                trans->x->coeff[0][1] = (psF32)rand() / (psF32)RAND_MAX * 10.0f;
                trans->x->coeff[1][0] = (psF32)rand() / (psF32)RAND_MAX * 10.0f;
                trans->x->coeff[1][1] = 0.0;
                trans->y->coeff[0][0] = (psF32)rand() / (psF32)RAND_MAX * 10.0f;
                trans->y->coeff[0][1] = (psF32)rand() / (psF32)RAND_MAX * 10.0f;
                trans->y->coeff[1][0] = (psF32)rand() / (psF32)RAND_MAX * 10.0f;
                trans->y->coeff[1][1] = 0.0;
            }

            transInverse = psPlaneTransformInvert(NULL, trans, myRegion, 10);
            if (transInverse == NULL) {
                diag("TEST ERROR: psPlaneTransformInvert() returned a NULL psPlaneTransform.\n");
                errorFlag = true;
            } else {
                // Transform the "in" coords to "mid" with psPlaneTransform "trans", then
                // transform "mid" to "out" with psPlaneTransform "transInverse".
                // XXX: Must loop on a few data points.
                psPlane *in = psPlaneAlloc();
                in->x = 2.0;
                in->y = 3.0;
                psPlane *mid = psPlaneTransformApply(NULL, trans, in);
                psPlane *out = psPlaneTransformApply(NULL, transInverse, mid);

                if (((fabs(in->x - out->x) > FLT_EPSILON)) ||
                        ((fabs(in->y - out->y) > FLT_EPSILON)) ||
                        isnan(out->x) ||
                        isnan(out->y)) {
                    diag("TEST ERROR: in coords were (%f, %f), out coords were (%f, %f).\n",
                         in->x, in->y, out->x, out->y);
                    errorFlag = true;
                }
                psFree(in);
                psFree(mid);
                psFree(out);
            }
            psFree(transInverse);
        }
        psFree(trans);
        ok(!errorFlag, "psPlaneTransformInvert() successful on several transforms");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // psPlaneTransformDeriv()
    // Ensure psPlaneTransformDeriv() returns NULL for NULL input plane
    // Following should generate error message
    // XXX: un-NULL the first parameter since we test that above
    // XXX: We do not test error generation
    {
        psMemId id = psMemGetId();
        psPlaneTransform *trans = psPlaneTransformAlloc(1, 3);
        psPlane *deriv = psPlaneTransformDeriv(NULL, trans, NULL);
        ok(deriv == NULL, "psPlaneTransformDeriv(NULL, trans, NULL) returned NULL");
        psFree(trans);
        psFree(deriv);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // psPlaneTransformDeriv()
    // We test on a very simple transformation.
    {
        psMemId id = psMemGetId();
        psPlane *coord = psPlaneAlloc();
        psPlane *deriv = NULL;
        psPlaneTransform *trans = NULL;
        coord->x = 3.0;
        coord->y = 1.0;
        trans = psPlaneTransformAlloc(1, 3);
        //Set Polynomials.  f(x) = x, f(y) = 0.5*y^2  -->  f'(x) = 1, f'(y) = y
        //So for 1,1  -> f'(1) = 1, f'(1) = 1
        trans->x->coeff[0][0] = 0.0;
        trans->x->coeff[1][0] = 1.0;
        trans->y->coeff[0][0] = 0.0;
        trans->y->coeff[0][1] = 0.0;
        trans->y->coeff[0][2] = 0.5;

        deriv = psPlaneTransformDeriv(NULL, trans, coord);
        ok(deriv->x == 1.0 && deriv->y == 1.0, "psPlaneTransformDeriv() returned the correct values");
        psFree(trans);
        psFree(deriv);
        psFree(coord);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
}

