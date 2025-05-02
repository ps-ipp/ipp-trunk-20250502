/** @file  tst_psMatrixVectorArithmetic01.c
 *
 *  @brief Test driver for psMatrixVector arithmetic functions
 *
 *  This test driver tests combinations of matrix, vector, and scalar binary operations including:
 *     Matrix-matrix with +,-,*,/ with S32, F32, F64, C32
 *     Matrix-vector with +,-,*,/ with S32, F32, F64, C32
 *     Matrix-scalar with +,-,*,/ with S32, F32, F64, C32
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

#define PRINT_SCALAR(SCALAR,TYPE)                                                                            \
if(PS_IS_PSELEMTYPE_COMPLEX(SCALAR->type.type)) {                                                        \
    printf("%f+%fi ", creal(SCALAR->data.TYPE), cimag(SCALAR->data.TYPE));                               \
} else if(PS_IS_PSELEMTYPE_INT(SCALAR->type.type)) {                                                     \
    printf("%d ", (psS32)SCALAR->data.TYPE);                                                               \
} else {                                                                                                 \
    printf("%f ", (double)SCALAR->data.TYPE);                                                            \
}                                                                                                        \
printf("\n\n");

#define PRINT_VECTOR(VECTOR,TYPE)                                                                            \
for(psS32 i=0; i<VECTOR->n; i++) {                                                                         \
    if(PS_IS_PSELEMTYPE_COMPLEX(VECTOR->type.type)) {                                                    \
        printf("%f+%fi ", creal(VECTOR->data.TYPE[i]), cimag(VECTOR->data.TYPE[i]));                     \
    } else if(PS_IS_PSELEMTYPE_INT(VECTOR->type.type)) {                                                 \
        printf("%d ", (psS32)VECTOR->data.TYPE[i]);                                                        \
    } else {                                                                                             \
        printf("%f ", (double)VECTOR->data.TYPE[i]);                                                     \
    }                                                                                                    \
}                                                                                                        \
printf("\n\n");


#define PRINT_MATRIX(IMAGE,TYPE)                                                                             \
for(psS32 i=IMAGE->numRows-1; i>-1; i--) {                                                                 \
    for(psS32 j=0; j<IMAGE->numCols; j++) {                                                                \
        if(PS_IS_PSELEMTYPE_COMPLEX(IMAGE->type.type)) {                                                 \
            printf("%f+%fi ", creal(IMAGE->data.TYPE[i][j]), cimag(IMAGE->data.TYPE[i][j]));             \
        } else if(PS_IS_PSELEMTYPE_INT(IMAGE->type.type)) {                                              \
            printf("%d ", (psS32)IMAGE->data.TYPE[i][j]);                                                  \
        } else {                                                                                         \
            printf("%f ", (double)IMAGE->data.TYPE[i][j]);                                               \
        }                                                                                                \
    }                                                                                                    \
    printf("\n");                                                                                        \
}                                                                                                        \
printf("\n");


#define CREATE_AND_SET_VECTOR(NAME,TYPE,VALUE,SIZE)                                                          \
psVector *NAME = (psVector*)psVectorAlloc(SIZE, PS_TYPE_##TYPE);                                         \
for(psS32 i=0; i<SIZE; i++) {                                                                              \
    NAME->data.TYPE[i] = VALUE;                                                                          \
}                                                                                                        \
NAME->n = SIZE;


#define CREATE_AND_SET_IMAGE(NAME,TYPE,VALUE,NROWS,NCOLS)                                                    \
psImage *NAME = (psImage*)psImageAlloc(NCOLS,NROWS,PS_TYPE_##TYPE);                                      \
for(psS32 i=0; i<NAME->numRows; i++) {                                                                     \
    for(psS32 j=0; j<NAME->numCols; j++) {                                                                 \
        NAME->data.TYPE[i][j] = VALUE;                                                                   \
    }                                                                                                    \
}


#define CHECK_MEMORY                                                                                     \
if(psMemCheckLeaks(0, NULL, stdout, false) != 0 ) {                                                             \
    psError(PS_ERR_UNKNOWN,true,"Memory leaks detected.");                                                \
    return 10;                                                                                            \
}                                                                                                        \
psS32 nBad = psMemCheckCorruption(0);                                                                    \
if(nBad) {                                                                                               \
    printf("ERROR: Found %d bad memory blocks\n", nBad);                                                 \
}


psS32 main(psS32 argc, char* argv[])
{
    psLogSetFormat("HLNM");

    // Test matrix-matrix binary operations
    #define testBinaryOpMM(OP,TYPE,VALUE1,VALUE2,NROWS,NCOLS)                                                \
    {                                                                                                        \
        printPositiveTestHeader(stdout, "psMatrixVectorArithmetic", "Test matrix-matrix psBinaryOp");        \
        printf("Operation: %s\n", #OP);                                                                      \
        CREATE_AND_SET_IMAGE(inImage,TYPE,VALUE1,NROWS,NCOLS);                                               \
        CREATE_AND_SET_IMAGE(outImage,TYPE,VALUE2,NROWS,NCOLS);                                              \
        printf("Input:\n");                                                                                  \
        PRINT_MATRIX(inImage,TYPE);                                                                          \
        PRINT_MATRIX(outImage,TYPE);                                                                         \
        outImage = (psImage*)psBinaryOp(outImage, inImage, #OP, outImage);                                   \
        printf("Output:\n");                                                                                 \
        PRINT_MATRIX(outImage,TYPE);                                                                         \
        psFree(inImage);                                                                                     \
        psFree(outImage);                                                                                    \
        CHECK_MEMORY;                                                                                        \
        printFooter(stdout, "psMatrixVectorArithmetic", "Test matrix-matrix psBinaryOp", true);              \
    }

    testBinaryOpMM(+,S32,10,10,3,2);
    testBinaryOpMM(+,F32,10.0,10.0,3,2);
    testBinaryOpMM(+,F64,10.0,10.0,3,2);
    testBinaryOpMM(+,C32,10.0+10.0i,10.0+10.0i,3,2);
    testBinaryOpMM(-,S32,20,10,3,2);
    testBinaryOpMM(-,F32,20.0,10.0,3,2);
    testBinaryOpMM(-,F64,20.0,10.0,3,2);
    testBinaryOpMM(*,C32,20.0+20.0i,10.0+10.0i,3,2);
    testBinaryOpMM(*,S32,20,10,3,2);
    testBinaryOpMM(*,F32,20.0,10.0,3,2);
    testBinaryOpMM(*,F64,20.0,10.0,3,2);
    testBinaryOpMM(*,C32,20.0+20.0i,10.0+10.0i,3,2);
    testBinaryOpMM(/,C32,20.0+20.0i,10.0+10.0i,3,2);
    testBinaryOpMM(/,S32,20,10,3,2);
    testBinaryOpMM(/,F32,20.0,10.0,3,2);
    testBinaryOpMM(/,F64,20.0,10.0,3,2);
    testBinaryOpMM(/,C32,20.0+20.0i,10.0+10.0i,3,2);

    // Test Matrix-Vector binary operations
    #define testBinaryOpMV(OP,TYPE,VALUE1,VALUE2,NROWS,NCOLS)                                                \
    {                                                                                                        \
        printPositiveTestHeader(stdout, "psMatrixVectorArithmetic", "Test matrix-vector psBinaryOp");        \
        printf("Operation: %s\n", #OP);                                                                      \
        CREATE_AND_SET_IMAGE(outImage,TYPE,VALUE1,NROWS,NCOLS);                                              \
        CREATE_AND_SET_VECTOR(inVector,TYPE,VALUE2,NROWS);                                                   \
        printf("Input:\n");                                                                                  \
        PRINT_MATRIX(outImage,TYPE);                                                                         \
        PRINT_VECTOR(inVector,TYPE);                                                                         \
        outImage = (psImage*)psBinaryOp(outImage, outImage, #OP, inVector);                                  \
        printf("Output:\n");                                                                                 \
        PRINT_MATRIX(outImage,TYPE);                                                                         \
        psFree(inVector);                                                                                    \
        psFree(outImage);                                                                                    \
        CHECK_MEMORY;                                                                                        \
        printFooter(stdout, "psMatrixVectorArithmetic", "Test matrix-vector psBinaryOp", true);              \
    }

    testBinaryOpMV(+,S32,10,5,3,2);
    testBinaryOpMV(+,F32,10.0,5.0,3,2);
    testBinaryOpMV(+,F64,10.0,5.0,3,2);
    testBinaryOpMV(+,C32,10.0+10.0i,5.0+5.0i,3,2);
    testBinaryOpMV(-,S32,20,5,3,2);
    testBinaryOpMV(-,F32,20.0,5.0,3,2);
    testBinaryOpMV(-,F64,20.0,5.0,3,2);
    testBinaryOpMV(*,C32,20.0+20.0i,5.0+5.0i,3,2);
    testBinaryOpMV(*,S32,20,5,3,2);
    testBinaryOpMV(*,F32,20.0,5.0,3,2);
    testBinaryOpMV(*,F64,20.0,5.0,3,2);
    testBinaryOpMV(*,C32,20.0+20.0i,5.0+5.0i,3,2);
    testBinaryOpMV(/,C32,20.0+20.0i,5.0+5.0i,3,2);
    testBinaryOpMV(/,S32,20,5,3,2);
    testBinaryOpMV(/,F32,20.0,5.0,3,2);
    testBinaryOpMV(/,F64,20.0,5.0,3,2);
    testBinaryOpMV(/,C32,20.0+20.0i,5.0+5.0i,3,2);

    // Test Matrix-Scalar binary operations
    #define testBinaryOpMS(OP,TYPE,VALUE1,VALUE2,NROWS,NCOLS)                                                \
    {                                                                                                        \
        printPositiveTestHeader(stdout, "psMatrixVectorArithmetic", "Test matrix-scalar psBinaryOp");        \
        printf("Operation: %s\n", #OP);                                                                      \
        CREATE_AND_SET_IMAGE(outImage,TYPE,VALUE1,NROWS,NCOLS);                                              \
        psScalar *inScalar = (psScalar*)psScalarAlloc(VALUE2,PS_TYPE_##TYPE);                                \
        printf("Input:\n");                                                                                  \
        PRINT_MATRIX(outImage,TYPE);                                                                         \
        PRINT_SCALAR(inScalar,TYPE);                                                                         \
        outImage = (psImage*)psBinaryOp(outImage, outImage, #OP, psScalarCopy(inScalar));                    \
        printf("Output:\n");                                                                                 \
        PRINT_MATRIX(outImage,TYPE);                                                                         \
        psFree(inScalar);                                                                                    \
        psFree(outImage);                                                                                    \
        CHECK_MEMORY;                                                                                        \
        printFooter(stdout, "psMatrixVectorArithmetic", "Test matrix-scalar psBinaryOp", true);              \
    }

    testBinaryOpMS(+,S32,10,5,3,2);
    testBinaryOpMS(+,F32,10.0,5.0,3,2);
    testBinaryOpMS(+,F64,10.0,5.0,3,2);
    testBinaryOpMS(+,C32,10.0+10.0i,5.0+5.0i,3,2);
    testBinaryOpMS(-,S32,20,5,3,2);
    testBinaryOpMS(-,F32,20.0,5.0,3,2);
    testBinaryOpMS(-,F64,20.0,5.0,3,2);
    testBinaryOpMS(*,C32,20.0+20.0i,5.0+5.0i,3,2);
    testBinaryOpMS(*,S32,20,5,3,2);
    testBinaryOpMS(*,F32,20.0,5.0,3,2);
    testBinaryOpMS(*,F64,20.0,5.0,3,2);
    testBinaryOpMS(*,C32,20.0+20.0i,5.0+5.0i,3,2);
    testBinaryOpMS(/,C32,20.0+20.0i,5.0+5.0i,3,2);
    testBinaryOpMS(/,S32,20,5,3,2);
    testBinaryOpMS(/,F32,20.0,5.0,3,2);
    testBinaryOpMS(/,F64,20.0,5.0,3,2);
    testBinaryOpMS(/,C32,20.0+20.0i,5.0+5.0i,3,2);

    return 0;
}

