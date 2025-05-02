/** @file  tst_psMatrix_07.c
 *
 *  @brief Test driver for psMatrix vector conversion functions
 *
 *  This test driver contains the following tests for psMatrix test point 7:
 *     Create input and output images and vectors
 *     Convert matrix to PS_DIMEN_VECTOR vector
 *     Attempt to use null image input argument
 *     Convert matrix to PS_DIMEN_TRANSV vector
 *     XXX: Improper image size (NOT TESTED)
 *     Convert PS_DIMEN_VECTOR vector to matrix
 *     XXX: Attempt to use null input vector argument (NOT TESTED)
 *     Convert PS_DIMEN_TRANSV vector to matrix
 *     Free input and output images and vectors
 *
 * XXX: This should produce an error; how can we test it?
 *  @author  Ross Harman, MHPCC
 *
 *  @version $Revision: 1.2 $  $Name: not supported by cvs2svn $
 *  @date  $Date: 2007-02-07 22:50:18 $
 *
 *  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 *
 */
#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include "tap.h"
#include "pstap.h"
#define TOLERANCE 0.000001
psF64 truthVector[3] = {0.0, 1.0, 2.0};
psF32 truthVector_32[3] = {0.0, 1.0, 2.0};
psF64 truthMatrix[3][1] = {{0.0}, {1.0}, {2.0}};
psF32 truthMatrix_32[3][1] = {{0.0}, {1.0}, {2.0}};

psS32 checkMatrix(psImage *img)
{
    bool errorFlag = false;
    for(psU32 i=0; i<img->numRows; i++) {
        for(psU32 j=0; j<img->numCols; j++) {
            if(img->type.type == PS_TYPE_F64) {
                if(fabs(img->data.F64[i][j]-truthMatrix[i][j]) > TOLERANCE) {
                    diag("Matrix values at element %d, %d don't agree %lf vs %lf\n", i, j,
                         img->data.F64[i][j], truthMatrix[i][j]);
                    errorFlag = true;
                }
            } else if(img->type.type == PS_TYPE_F32) {
                if(fabs(img->data.F32[i][j]-truthMatrix[i][j]) > TOLERANCE) {
                    diag("Matrix values at element %d, %d don't agree %f vs %lf\n", i, j,
                         img->data.F32[i][j], truthMatrix[i][j]);
                    errorFlag = true;
                }
            }
        }
    }
    return(errorFlag);
}

psS32 checkVector(psVector *vector)
{
    bool errorFlag = false;
    for(psU32 i=0; i<vector->n; i++) {
        if(vector->type.type == PS_TYPE_F64) {
            if(fabs(vector->data.F64[i]-truthVector[i]) > TOLERANCE) {
                diag("Vector values at element %d don't agree %lf vs %lf\n", i,
                     vector->data.F64[i], truthVector[i]);
                errorFlag = true;
            }
        } else if(vector->type.type == PS_TYPE_F32) {
            if(fabs(vector->data.F32[i]-truthVector[i]) > TOLERANCE) {
                diag("Vector values at element %d don't agree %f vs %lf\n", i,
                     vector->data.F32[i], truthVector[i]);
                errorFlag = true;
            }
        }
    }
    return(errorFlag);
}

psS32 main(psS32 argc, char* argv[])
{
    psLogSetFormat("HLNM");
    plan_tests(44);

    // Convert matrix to PS_DIMEN_VECTOR vector: F64 version
    {
        psMemId id = psMemGetId();
        psVector *v1 = (psVector*)psVectorAlloc(3, PS_TYPE_F64);
        psImage *m1 = (psImage*)psImageAlloc(1, 3, PS_TYPE_F64);
        m1->data.F64[0][0] = 0.0;
        m1->data.F64[1][0] = 1.0;
        m1->data.F64[2][0] = 2.0;
        psVector *tempVector = v1;

        v1 = psMatrixToVector(v1, m1);
        ok(v1 != NULL, "psMatrixToVector() produced a non-NULL vector");
        skip_start(v1 == NULL, 3, "Skipping tests because vector was NULL");
        ok(!checkVector(v1), "psMatrixToVector() produced the correct vector");
        ok(v1->type.dimen == PS_DIMEN_VECTOR, "psMatrixToVector() produced the correct ->dimen member");
        ok(v1 == tempVector, "psMatrixToVector() did not allocate a new output vector");

        skip_end();
        psFree(m1);
        psFree(v1);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Convert matrix to PS_DIMEN_VECTOR vector: F32 version
    {
        psMemId id = psMemGetId();
        psVector *v1_32 = (psVector*)psVectorAlloc(3, PS_TYPE_F32);
        psImage *m1_32 = (psImage*)psImageAlloc(1,3,PS_TYPE_F32);
        m1_32->data.F32[0][0] = 0.0;
        m1_32->data.F32[1][0] = 1.0;
        m1_32->data.F32[2][0] = 2.0;
        psVector *tempVector_32 = v1_32;

        v1_32 = psMatrixToVector(v1_32, m1_32);
        ok(v1_32 != NULL, "psMatrixToVector() produced a non-NULL vector");
        skip_start(v1_32 == NULL, 3, "Skipping tests because vector was NULL");
        ok(!checkVector(v1_32), "psMatrixToVector() produced the correct vector");
        ok(v1_32->type.dimen == PS_DIMEN_VECTOR, "psMatrixToVector() produced the correct ->dimen member");
        ok(v1_32 == tempVector_32, "psMatrixToVector() did not allocate a new output vector");

        skip_end();
        psFree(m1_32);
        psFree(v1_32);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Attempt to use null image input argument: F64 version
    {
        psMemId id = psMemGetId();
        psVector *v1 = (psVector*)psVectorAlloc(3, PS_TYPE_F64);
        v1 = psMatrixToVector(v1, NULL);
        ok(v1 == NULL, "psMatrixToVector() returned NULL with NULL matrix argument");
        psFree(v1);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Attempt to use null image input argument: F32 version
    {
        psMemId id = psMemGetId();
        psVector *v1_32 = (psVector*)psVectorAlloc(3, PS_TYPE_F32);
        v1_32 = psMatrixToVector(v1_32, NULL);
        ok(v1_32 == NULL, "psMatrixToVector() returned NULL with NULL matrix argument");
        psFree(v1_32);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Convert matrix to PS_DIMEN_TRANSV vector: F64 version
    {
        psMemId id = psMemGetId();
        psImage *m4 = (psImage*)psImageAlloc(3, 1, PS_TYPE_F64);
        m4->data.F64[0][0] = 0.0;
        m4->data.F64[0][1] = 1.0;
        m4->data.F64[0][2] = 2.0;
        psVector *v3 = psVectorAlloc(3, PS_TYPE_F64);
        psVector *tempVector = v3;

        v3->type.dimen = PS_DIMEN_TRANSV;
        psMatrixToVector(v3, m4);
        ok(v3 != NULL, "psMatrixToVector() produced non-NULL output VECTOR");
        skip_start(v3 == NULL, 3, "Skipping tests because of non-NULL output VECTOR");
        ok(!checkVector(v3), "psMatrixToVector() produced the correct vector");
        ok(v3->type.dimen == PS_DIMEN_TRANSV, "psMatrixToVector() produced the correct ->dimen member");
        ok(v3 == tempVector, "psMatrixToVector() did not allocate a new output vector");

        skip_end();
        psFree(m4);
        psFree(v3);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Convert matrix to PS_DIMEN_TRANSV vector: F32 version
    {
        psMemId id = psMemGetId();
        psImage *m4_32 = m4_32 = (psImage*)psImageAlloc(3,1,PS_TYPE_F32);
        m4_32->data.F32[0][0] = 0.0;
        m4_32->data.F32[0][1] = 1.0;
        m4_32->data.F32[0][2] = 2.0;
        psVector *v3_32 = psVectorAlloc(3, PS_TYPE_F32);
        psVector *tempVector_32 = v3_32;

        v3_32->type.dimen = PS_DIMEN_TRANSV;
        psMatrixToVector(v3_32, m4_32);
        ok(v3_32 != NULL, "psMatrixToVector() produced non-NULL output VECTOR");
        skip_start(v3_32 == NULL, 3, "Skipping tests because of non-NULL output VECTOR");
        ok(!checkVector(v3_32), "psMatrixToVector() produced the correct vector");
        ok(v3_32->type.dimen == PS_DIMEN_TRANSV, "psMatrixToVector() produced the correct ->dimen member");
        ok(v3_32 == tempVector_32, "psMatrixToVector() did not allocate a new output vector");

        skip_end();
        psFree(m4_32);
        psFree(v3_32);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Improper image size: F64 version
    // XXX: This should produce an error; how can we test it?
    if (0) {
        psMemId id = psMemGetId();
        psImage *badImage = (psImage*)psImageAlloc(2, 2, PS_TYPE_F64);
        psVector *v1 = (psVector*)psVectorAlloc(3, PS_TYPE_F64);
        ok(psMatrixToVector(v1, badImage) == NULL, "psMatrixToVector() returned NULL with improper sizes");
        psFree(badImage);
        psFree(v1);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Improper image size: F32 version
    // XXX: This should produce an error; how can we test it?
    if (0) {
        psMemId id = psMemGetId();
        psImage *badImage_32 = (psImage*)psImageAlloc(2,2,PS_TYPE_F32);
        psVector *v1_32 = (psVector*)psVectorAlloc(3, PS_TYPE_F32);
        ok(psMatrixToVector(v1_32, badImage_32) == NULL, "psMatrixToVector() returned NULL with improper sizes");
        psFree(badImage_32);
        psFree(v1_32);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Convert PS_DIMEN_VECTOR vector to matrix: F64 version
    {
        psMemId id = psMemGetId();
        psImage *m2 = (psImage*)psImageAlloc(1, 3, PS_TYPE_F64);
        psVector *v2 = (psVector*)psVectorAlloc(3, PS_TYPE_F64);
        v2->data.F64[0] = 0.0;
        v2->data.F64[1] = 1.0;
        v2->data.F64[2] = 2.0;
        v2->n = 3;

        psImage *tempImage = m2;
        m2 = psVectorToMatrix(m2, v2);
        ok(m2 != NULL, "psVectorToMatrix() produced non-NULL output matrix");
        skip_start(m2 == NULL, 3, "Skipping tests because of non-NULL output matrix");
        ok(!checkMatrix(m2), "psVectorToMatrix() produced the correct vector");
        ok(m2->type.dimen == PS_DIMEN_IMAGE, "psVectorToMatrix() produced the correct ->dimen member");
        ok(m2 == tempImage, "psVectorToMatrix() did not allocate a new output matrix");

        skip_end();
        psFree(m2);
        psFree(v2);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Convert PS_DIMEN_VECTOR vector to matrix: F32 version
    {
        psMemId id = psMemGetId();
        psImage *m2_32 = (psImage*)psImageAlloc(1,3,PS_TYPE_F32);
        psVector *v2_32 = (psVector*)psVectorAlloc(3,PS_TYPE_F32);
        v2_32->data.F32[0] = 0.0;
        v2_32->data.F32[1] = 1.0;
        v2_32->data.F32[2] = 2.0;
        v2_32->n = 3;

        psImage *tempImage_32 = m2_32;
        m2_32 = psVectorToMatrix(m2_32, v2_32);
        ok(m2_32 != NULL, "psVectorToMatrix() produced non-NULL output matrix");
        skip_start(m2_32 == NULL, 3, "Skipping tests because of non-NULL output matrix");
        ok(!checkMatrix(m2_32), "psVectorToMatrix() produced the correct vector");
        ok(m2_32->type.dimen == PS_DIMEN_IMAGE, "psVectorToMatrix() produced the correct ->dimen member");
        ok(m2_32 == tempImage_32, "psVectorToMatrix() did not allocate a new output matrix");

        skip_end();
        psFree(m2_32);
        psFree(v2_32);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Attempt to use null input vector argument: F64 version
    // XXX: This should produce an error; how can we test it?
    if (0) {
        psMemId id = psMemGetId();
        psImage *m2 = (psImage*)psImageAlloc(1, 3, PS_TYPE_F64);
        ok(psVectorToMatrix(m2, NULL) == NULL, "psVectorToMatrix() returned NULL with NULL input vector");
        psFree(m2);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Attempt to use null input vector argument: F32 version
    // XXX: This should produce an error; how can we test it?
    if (0) {
        psMemId id = psMemGetId();
        psImage *m2_32 = (psImage*)psImageAlloc(1,3,PS_TYPE_F32);
        ok(psVectorToMatrix(m2_32, NULL) == NULL, "psVectorToMatrix() returned NULL with NULL input vector");
        psFree(m2_32);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Convert PS_DIMEN_TRANSV vector to matrix: F64 version
    {
        psMemId id = psMemGetId();
        psVector *v2 = (psVector*)psVectorAlloc(3, PS_TYPE_F64);
        v2->data.F64[0] = 0.0;
        v2->data.F64[1] = 1.0;
        v2->data.F64[2] = 2.0;
        v2->n = 3;
        v2->type.dimen = PS_DIMEN_TRANSV;
        psImage *m3 = (psImage*)psImageAlloc(3, 1, PS_TYPE_F64);
        psImage *tempImage = m3;

        psVectorToMatrix(m3, v2);
        ok(m3 != NULL, "psVectorToMatrix() produced non-NULL output matrix");
        skip_start(m3 == NULL, 3, "Skipping tests because of non-NULL output matrix");
        ok(!checkMatrix(m3), "psVectorToMatrix() produced the correct vector");
        ok(m3->type.dimen == PS_DIMEN_IMAGE, "psVectorToMatrix() produced the correct ->dimen member");
        ok(m3 == tempImage, "psVectorToMatrix() did not allocate a new output matrix");

        skip_end();
        psFree(v2);
        psFree(m3);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Convert PS_DIMEN_TRANSV vector to matrix: F32 version
    {
        psMemId id = psMemGetId();
        psVector *v2_32 = (psVector*)psVectorAlloc(3,PS_TYPE_F32);
        v2_32->data.F32[0] = 0.0;
        v2_32->data.F32[1] = 1.0;
        v2_32->data.F32[2] = 2.0;
        v2_32->n = 3;
        v2_32->type.dimen = PS_DIMEN_TRANSV;
        psImage *m3_32 = (psImage*)psImageAlloc(3,1,PS_TYPE_F32);
        psImage *tempImage_32 = m3_32;

        psVectorToMatrix(m3_32, v2_32);
        ok(m3_32 != NULL, "psVectorToMatrix() produced non-NULL output matrix");
        skip_start(m3_32 == NULL, 3, "Skipping tests because of non-NULL output matrix");
        ok(!checkMatrix(m3_32), "psVectorToMatrix() produced the correct vector");
        ok(m3_32->type.dimen == PS_DIMEN_IMAGE, "psVectorToMatrix() produced the correct ->dimen member");
        ok(m3_32 == tempImage_32, "psVectorToMatrix() did not allocate a new output matrix");

        skip_end();
        psFree(v2_32);
        psFree(m3_32);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
}
