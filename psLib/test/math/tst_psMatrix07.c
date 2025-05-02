/** @file  tst_psMatrix_07.c
 *
 *  @brief Test driver for psMatrix vector conversion functions
 *
 *  This test driver contains the following tests for psMatrix test point 7:
 *     A)  Create input and output images and vectors
 *     B)  Convert matrix to PS_DIMEN_VECTOR vector
 *     C)  Attempt to use null image input argument
 *     D)  Convert matrix to PS_DIMEN_TRANSV vector
 *     E)  Improper image size
 *     F)  Convert PS_DIMEN_VECTOR vector to matrix
 *     G)  Attempt to use null input vector argument
 *     H)  Convert PS_DIMEN_TRANSV vector to matrix
 *     I)  Free input and output images and vectors
 *
 *  @author  Ross Harman, MHPCC
 *
 *  @version $Revision: 1.2 $  $Name: not supported by cvs2svn $
 *  @date  $Date: 2005-08-24 01:24:24 $
 *
 *  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 *
 */

#include "pslib_strict.h"
#include "psTest.h"


#define TOLERANCE 0.000001

#define CHECK_MATRIX(IMAGE,TRUTH)                                                                        \
for(psU32 i=0; i<IMAGE->numRows; i++) {                                                                  \
    for(psU32 j=0; j<IMAGE->numCols; j++) {                                                              \
        if(IMAGE->type.type == PS_TYPE_F64) {                                                            \
            if(fabs(IMAGE->data.F64[i][j]-TRUTH[i][j]) > TOLERANCE) {                                    \
                printf("Matrix values at element %d, %d don't agree %lf vs %lf\n", i, j,                 \
                       IMAGE->data.F64[i][j], TRUTH[i][j]);                                              \
            }                                                                                            \
        } else if(IMAGE->type.type == PS_TYPE_F32){                                                      \
            if(fabs(IMAGE->data.F32[i][j]-TRUTH[i][j]) > TOLERANCE) {                                    \
                printf("Matrix values at element %d, %d don't agree %f vs %f\n", i, j,                  \
                       IMAGE->data.F32[i][j], TRUTH[i][j]);                                              \
            }                                                                                            \
        }                                                                                                \
    }                                                                                                    \
}

#define CHECK_VECTOR(VECTOR)                                                                                 \
for(psU32 i=0; i<VECTOR->n; i++) {                                                                       \
    if(VECTOR->type.type == PS_TYPE_F64) {                                                               \
        if(fabs(VECTOR->data.F64[i]-truthVector[i]) > TOLERANCE) {                                       \
            printf("Vector values at element %d don't agree %lf vs %lf\n", i,                            \
                   VECTOR->data.F64[i], truthVector[i]);                                                 \
        }                                                                                                \
    } else if(VECTOR->type.type == PS_TYPE_F32){                                                         \
        if(fabs(VECTOR->data.F32[i]-truthVector_32[i]) > TOLERANCE) {                                       \
            printf("Vector values at element %d don't agree %f vs %lf\n", i,                             \
                   VECTOR->data.F32[i], truthVector[i]);                                                 \
        }                                                                                                \
    }                                                                                                    \
}


psS32 main(psS32 argc, char* argv[])
{
    psLogSetFormat("HLNM");
    psVector *v1 = NULL;
    psVector *v1_32 = NULL;
    psVector *tempVector = NULL;
    psVector *tempVector_32 = NULL;
    psImage *tempImage = NULL;
    psImage *tempImage_32 = NULL;
    psImage *m1 = NULL;
    psImage *m1_32 = NULL;
    psVector *v2 = NULL;
    psVector *v2_32 = NULL;
    psVector *v3 = NULL;
    psVector *v3_32 = NULL;
    psImage *m2 = NULL;
    psImage *m2_32 = NULL;
    psImage *m3 = NULL;
    psImage *m3_32 = NULL;
    psImage *m4 = NULL;
    psImage *m4_32 = NULL;
    psImage *badImage = NULL;
    psImage *badImage_32 = NULL;

    psF64 truthVector[3] = {0.0, 1.0, 2.0};
    psF32 truthVector_32[3] = {0.0, 1.0, 2.0};
    psF64 truthMatrix[3][1] = {{0.0}, {1.0}, {2.0}};
    psF32 truthMatrix_32[3][1] = {{0.0}, {1.0}, {2.0}};

    // Test A - Create input and output images
    printPositiveTestHeader(stdout, "psMatrix", "Create input and output images and vectors");
    v1 = (psVector*)psVectorAlloc(3, PS_TYPE_F64);
    v1_32 = (psVector*)psVectorAlloc(3, PS_TYPE_F32);
    m1 = (psImage*)psImageAlloc(1, 3, PS_TYPE_F64);
    m1_32 = (psImage*)psImageAlloc(1,3,PS_TYPE_F32);
    v2 = (psVector*)psVectorAlloc(3, PS_TYPE_F64);
    v2_32 = (psVector*)psVectorAlloc(3,PS_TYPE_F32);
    m2 = (psImage*)psImageAlloc(1, 3, PS_TYPE_F64);
    m2_32 = (psImage*)psImageAlloc(1,3,PS_TYPE_F32);
    m3 = (psImage*)psImageAlloc(3, 1, PS_TYPE_F64);
    m3_32 = (psImage*)psImageAlloc(3,1,PS_TYPE_F32);
    m4 = (psImage*)psImageAlloc(3, 1, PS_TYPE_F64);
    m4_32 = (psImage*)psImageAlloc(3,1,PS_TYPE_F32);
    badImage = (psImage*)psImageAlloc(2, 2, PS_TYPE_F64);
    badImage_32 = (psImage*)psImageAlloc(2,2,PS_TYPE_F32);
    m1->data.F64[0][0] = 0.0;
    m1->data.F64[1][0] = 1.0;
    m1->data.F64[2][0] = 2.0;
    m1_32->data.F32[0][0] = 0.0;
    m1_32->data.F32[1][0] = 1.0;
    m1_32->data.F32[2][0] = 2.0;
    v2->data.F64[0] = 0.0;
    v2->data.F64[1] = 1.0;
    v2->data.F64[2] = 2.0;
    v2->n = 3;
    v2_32->data.F32[0] = 0.0;
    v2_32->data.F32[1] = 1.0;
    v2_32->data.F32[2] = 2.0;
    v2_32->n = 3;
    m4->data.F64[0][0] = 0.0;
    m4->data.F64[0][1] = 1.0;
    m4->data.F64[0][2] = 2.0;
    m4_32->data.F32[0][0] = 0.0;
    m4_32->data.F32[0][1] = 1.0;
    m4_32->data.F32[0][2] = 2.0;
    printFooter(stdout, "psMatrix", "Create input and output images and vectors", true);

    // Test B - Convert matrix to PS_DIMEN_VECTOR vector
    printPositiveTestHeader(stdout, "psMatrix", "Convert matrix to PS_DIMEN_VECTOR vector");
    tempVector = v1;
    v1 = psMatrixToVector(v1, m1);
    CHECK_VECTOR(v1);
    if(v1->type.dimen != PS_DIMEN_VECTOR) {
        psError(PS_ERR_UNKNOWN,true,"Resulting image is not PS_DIMEN_VECTOR");
        return 1;
    } else if(v1 != tempVector) {
        psError(PS_ERR_UNKNOWN,true,"Return pointer not equal to output argument pointer");
        return 2;
    }
    tempVector_32 = v1_32;
    v1_32 = psMatrixToVector(v1_32, m1_32);
    CHECK_VECTOR(v1_32);
    if(v1_32->type.dimen != PS_DIMEN_VECTOR) {
        psError(PS_ERR_UNKNOWN,true,"Resulting image is not PS_DIMEN_VECTOR");
        return 1;
    } else if(v1_32 != tempVector_32) {
        psError(PS_ERR_UNKNOWN,true,"Return pointer not equal to output argument pointer");
        return 2;
    }
    printFooter(stdout, "psMatrix", "Convert matrix to PS_DIMEN_VECTOR vector", true);


    // Test C - Attempt to use null image input argument
    printNegativeTestHeader(stdout,"psMatrix", "Attempt to use null image input argument",
                            "Invalid operation: inImage or its data is NULL.", 0);
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate an error.");
    v1 = psMatrixToVector(v1, NULL);
    if(v1 != NULL) {
        psError(PS_ERR_UNKNOWN,true,"Did not return NULL with NULL input");
        return 3;
    }
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate an error.");
    v1_32 = psMatrixToVector(v1_32, NULL);
    if(v1_32 != NULL) {
        psError(PS_ERR_UNKNOWN,true,"Did not return NULL with NULL input");
        return 3;
    }
    printFooter(stdout, "psMatrix", "Attempt to use null image input argument", true);


    // Test D - Convert matrix to PS_DIMEN_TRANSV vector
    printPositiveTestHeader(stdout, "psMatrix", "Convert matrix to PS_DIMEN_TRANSV vector");
    v3 = psVectorAlloc(3, PS_TYPE_F64);
    tempVector = v3;
    v3->type.dimen = PS_DIMEN_TRANSV;
    psMatrixToVector(v3, m4);
    CHECK_VECTOR(v3);
    if(v3->type.dimen != PS_DIMEN_TRANSV) {
        psError(PS_ERR_UNKNOWN,true,"Resulting image is not PS_DIMEN_TRANSV");
        return 4;
    } else if(v3 != tempVector) {
        psError(PS_ERR_UNKNOWN,true,"Return pointer not equal to output argument pointer");
        return 5;
    }
    v3_32 = psVectorAlloc(3, PS_TYPE_F32);
    tempVector_32 = v3_32;
    v3_32->type.dimen = PS_DIMEN_TRANSV;
    psMatrixToVector(v3_32, m4_32);
    CHECK_VECTOR(v3_32);
    if(v3_32->type.dimen != PS_DIMEN_TRANSV) {
        psError(PS_ERR_UNKNOWN,true,"Resulting image is not PS_DIMEN_TRANSV");
        return 6;
    } else if(v3_32 != tempVector_32) {
        psError(PS_ERR_UNKNOWN,true,"Return pointer not equal to output argument pointer");
        return 7;
    }
    printFooter(stdout, "psMatrix", "Convert matrix to PS_DIMEN_TRANSV vector", true);


    // Test E - Improper image size
    printNegativeTestHeader(stdout,"psMatrix", "Improper image size",
                            "Image does not have dim with 1 col or 1 row: (2 x 2).", 0);
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate error message");
    if(psMatrixToVector(v1, badImage) != NULL ) {
        psError(PS_ERR_UNKNOWN,true,"Did not return NULL with improper sizes");
        return 8;
    }
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate error message");
    if(psMatrixToVector(v1_32, badImage_32) != NULL ) {
        psError(PS_ERR_UNKNOWN,true,"Did not return NULL with improper sizes");
        return 9;
    }
    printFooter(stdout, "psMatrix", "Improper image size", true);


    // Test F - Convert PS_DIMEN_VECTOR vector to matrix
    printPositiveTestHeader(stdout, "psMatrix", "Convert PS_DIMEN_VECTOR vector to matrix");
    tempImage = m2;
    m2 = psVectorToMatrix(m2, v2);
    CHECK_MATRIX(m2,truthMatrix);
    if(m2->type.dimen != PS_DIMEN_IMAGE) {
        psError(PS_ERR_UNKNOWN,true,"Resulting image is not PS_DIMEN_IMAGE");
        return 10;
    } else if(m2 != tempImage) {
        psError(PS_ERR_UNKNOWN,true,"Return pointer not equal to output argument pointer");
        return 11;
    }
    tempImage_32 = m2_32;
    m2_32 = psVectorToMatrix(m2_32, v2_32);
    CHECK_MATRIX(m2_32,truthMatrix_32);
    if(m2_32->type.dimen != PS_DIMEN_IMAGE) {
        psError(PS_ERR_UNKNOWN,true,"Resulting image is not PS_DIMEN_IMAGE");
        return 10;
    } else if(m2_32 != tempImage_32) {
        psError(PS_ERR_UNKNOWN,true,"Return pointer not equal to output argument pointer");
        return 11;
    }
    printFooter(stdout, "psMatrix", "Convert PS_DIMEN_VECTOR vector to matrix", true);


    // Test G - Attempt to use null input vector argument
    printNegativeTestHeader(stdout,"psMatrix", "Attempt to use null input vector argument",
                            "Invalid operation: inVector or its data is NULL.", 0);
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate an error message");
    if(psVectorToMatrix(m2, NULL) != NULL) {
        psError(PS_ERR_UNKNOWN,true,"Did not return output image");
        return 12;
    }
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate an error message");
    if(psVectorToMatrix(m2_32, NULL) != NULL) {
        psError(PS_ERR_UNKNOWN,true,"Did not return output image");
        return 13;
    }
    printFooter(stdout, "psMatrix", "Attempt to use null input vector argument", true);


    // Test H - Convert PS_DIMEN_TRANSV vector to matrix
    printPositiveTestHeader(stdout, "psMatrix", "Convert PS_DIMEN_TRANSV vector to matrix");
    v2->type.dimen = PS_DIMEN_TRANSV;
    tempImage = m3;
    psVectorToMatrix(m3, v2);
    CHECK_MATRIX(m3, truthMatrix);
    if(m3->type.dimen != PS_DIMEN_IMAGE) {
        psError(PS_ERR_UNKNOWN,true,"Resulting image is not PS_DIMEN_IMAGE");
        return 14;
    } else if(m3 != tempImage) {
        psError(PS_ERR_UNKNOWN,true,"Return pointer not equal to output argument pointer");
        return 15;
    }
    v2_32->type.dimen = PS_DIMEN_TRANSV;
    tempImage_32 = m3_32;
    psVectorToMatrix(m3_32, v2_32);
    CHECK_MATRIX(m3_32, truthMatrix_32);
    if(m3_32->type.dimen != PS_DIMEN_IMAGE) {
        psError(PS_ERR_UNKNOWN,true,"Resulting image is not PS_DIMEN_IMAGE");
        return 16;
    } else if(m3_32 != tempImage_32) {
        psError(PS_ERR_UNKNOWN,true,"Return pointer not equal to output argument pointer");
        return 17;
    }
    printFooter(stdout, "psMatrix", "Convert PS_DIMEN_TRANSV vector to matrix", true);


    // Test I - Free input and output images
    printPositiveTestHeader(stdout, "psMatrix", "Free input and output images and vectors");
    psFree(m1);
    psFree(v1);
    psFree(v2);
    psFree(v3);
    psFree(m3);
    psFree(m4);
    psFree(m1_32);
    psFree(v1_32);
    psFree(v2_32);
    psFree(v3_32);
    psFree(m3_32);
    psFree(m4_32);
    psFree(badImage);
    psFree(badImage_32);
    if( psMemCheckLeaks(0, NULL, stdout, false) != 0) {
        psError(PS_ERR_UNKNOWN,true,"Memory leaks detected.");
        return 10;
    }
    psS32 nBad = psMemCheckCorruption(0);
    if(nBad) {
        printf("ERROR: Found %d bad memory blocks\n", nBad);
    }
    printFooter(stdout, "psMatrix" ,"Free input and output images and vectors", true);

    return 0;
}
