/** @file  tst_psMatrixVectorArithmetic01.c
 *
 *  @brief Test driver for psMatrixVector arithmetic functions
 *
 *  This test driver tests combinations of matrix, vector, and scalar binary operations including:
 *     Matrix-matrix with +,-,*,/ with S32, F32, F64
 *     Matrix-vector with +,-,*,/ with S32, F32, F64
 *     Matrix-scalar with +,-,*,/ with S32, F32, F64
 *
 *  @author  Ross Harman, MHPCC
 *
 *  @version $Revision: 1.3 $  $Name: not supported by cvs2svn $
 *  @date  $Date: 2007-05-02 04:14:33 $
 *
 *  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 *
 */
#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include "tap.h"
#include "pstap.h"
#define VERBOSE 1

#define CREATE_AND_SET_VECTOR(NAME,TYPE,VALUE,SIZE)                \
psVector *NAME = (psVector*)psVectorAlloc(SIZE, PS_TYPE_##TYPE);   \
{                                                                  \
    ps##TYPE tmpScalar = VALUE;                                    \
    for(psS32 i=0; i<SIZE; i++) {                                  \
        NAME->data.TYPE[i] = tmpScalar;                            \
        tmpScalar+= (ps##TYPE) 1;                                  \
    }                                                              \
}                                                                  \
NAME->n = SIZE;


#define CREATE_AND_SET_IMAGE(NAME,TYPE,VALUE,NROWS,NCOLS)          \
psImage *NAME = (psImage*)psImageAlloc(NCOLS,NROWS,PS_TYPE_##TYPE);\
{                                                                  \
    ps##TYPE tmpScalar = VALUE;                                    \
    for (psS32 i=0; i<NAME->numRows; i++) {                        \
        for(psS32 j=0; j<NAME->numCols; j++) {                     \
            NAME->data.TYPE[i][j] = tmpScalar;                     \
            tmpScalar+= (ps##TYPE) 1;                              \
        }                                                          \
    }                                                              \
}

#define testBinaryOpMM(OP,TYPE,VALUE1,VALUE2,NROWS,NCOLS,ERRORFLAG,MEMORYFLAG) \
{                                                                              \
    psMemId id = psMemGetId();                                                 \
    ERRORFLAG = false;                                                         \
    MEMORYFLAG = false;                                                        \
    CREATE_AND_SET_IMAGE(in1,TYPE,VALUE1,NROWS,NCOLS);                         \
    CREATE_AND_SET_IMAGE(in2,TYPE,VALUE2,NROWS,NCOLS);                         \
    psImage *out = (psImage*)psBinaryOp(NULL, in1, #OP, in2);                  \
    for (psS32 i = 0 ; i < NROWS ; i++) {                                      \
        for (psS32 j = 0 ; j < NCOLS ; j++) {                                  \
            ps##TYPE expect = in1->data.TYPE[i][j] OP in2->data.TYPE[i][j];    \
            if (fabs((psF32) (out->data.TYPE[i][j] - expect)) > 0.1) {         \
                errorFlag = true;                                              \
                if (VERBOSE) {                                                 \
                    printf("TEST ERROR: outImage[%d][%d] is %f, should be %f\n", i, j, (psF32) out->data.TYPE[i][j], (psF32) expect); \
                }                                                              \
            }                                                                  \
        }                                                                      \
    }                                                                          \
    psFree(in1);                                                               \
    psFree(in2);                                                               \
    psFree(out);                                                               \
    MEMORYFLAG = psMemCheckLeaks(id, NULL, NULL, false);                       \
}

#define testBinaryOpMV(OP,TYPE,VALUE1,VALUE2,NROWS,NCOLS,ERRORFLAG,MEMORYFLAG) \
{                                                                              \
    psMemId id = psMemGetId();                                                 \
    ERRORFLAG = false;                                                         \
    MEMORYFLAG = false;                                                        \
    CREATE_AND_SET_IMAGE(in,TYPE,VALUE1,NROWS,NCOLS);                          \
    CREATE_AND_SET_VECTOR(inVector,TYPE,VALUE2,NROWS);                         \
    psImage *out = (psImage*)psBinaryOp(NULL, in, #OP, inVector);              \
    for (psS32 i = 0 ; i < NROWS ; i++) {                                      \
        for (psS32 j = 0 ; j < NCOLS ; j++) {                                  \
            ps##TYPE expect = in->data.TYPE[i][j] OP inVector->data.TYPE[i];   \
            if (fabs((psF32) (out->data.TYPE[i][j] - expect)) > 0.1) {         \
                errorFlag = true;                                              \
                if (VERBOSE) {                                                 \
                    printf("TEST ERROR: outImage[%d][%d] is %f, should be %f\n", i, j, (psF32) out->data.TYPE[i][j], (psF32) expect); \
                }                                                              \
            }                                                                  \
        }                                                                      \
    }                                                                          \
    psFree(in);                                                                \
    psFree(inVector);                                                          \
    psFree(out);                                                               \
    MEMORYFLAG = psMemCheckLeaks(id, NULL, NULL, false);                       \
}

#define testBinaryOpMS(OP,TYPE,VALUE1,VALUE2,NROWS,NCOLS,ERRORFLAG,MEMORYFLAG) \
{                                                                              \
    psMemId id = psMemGetId();                                                 \
    ERRORFLAG = false;                                                         \
    MEMORYFLAG = false;                                                        \
    CREATE_AND_SET_IMAGE(in,TYPE,VALUE1,NROWS,NCOLS);                          \
    psScalar *inScalar = (psScalar*)psScalarAlloc(VALUE2,PS_TYPE_##TYPE);      \
    psImage *out = (psImage*)psBinaryOp(NULL, in, #OP, psScalarCopy(inScalar));\
    for (psS32 i = 0 ; i < NROWS ; i++) {                                      \
        for (psS32 j = 0 ; j < NCOLS ; j++) {                                  \
            ps##TYPE expect = in->data.TYPE[i][j] OP inScalar->data.TYPE;      \
            if (fabs((psF32) (out->data.TYPE[i][j] - expect)) > 0.1) {         \
                errorFlag = true;                                              \
                if (VERBOSE) {                                                 \
                    printf("TEST ERROR: outImage[%d][%d] is %f, should be %f\n", i, j, (psF32) out->data.TYPE[i][j], (psF32) expect); \
                }                                                              \
            }                                                                  \
        }                                                                      \
    }                                                                          \
    psFree(inScalar);                                                          \
    psFree(in);                                                                \
    psFree(out);                                                               \
    MEMORYFLAG = psMemCheckLeaks(id, NULL, NULL, false);                       \
}


psS32 main(psS32 argc, char* argv[])
{
    psLogSetFormat("HLNM");
    bool errorFlag = false;
    bool memoryFlag = false;
    plan_tests(72);

    //Test matrix-matrix binary operations
    testBinaryOpMM(+,S32,10,20,5,4,errorFlag,memoryFlag);
    ok(errorFlag== false, "psBinaryOp(): was successful: (matrix-matrix, +, S32)");
    ok(memoryFlag== false, "no memory leaks");
    testBinaryOpMM(+,F32,10.0,10.0,3,2,errorFlag,memoryFlag);
    ok(errorFlag== false, "psBinaryOp(): was successful: (matrix-matrix, +, F32)");
    ok(memoryFlag== false, "no memory leaks");
    testBinaryOpMM(+,F64,10.0,10.0,3,2,errorFlag,memoryFlag);
    ok(errorFlag== false, "psBinaryOp(): was successful: (matrix-matrix, +, F64)");
    ok(memoryFlag== false, "no memory leaks");
    testBinaryOpMM(-,S32,20,10,3,2,errorFlag,memoryFlag);
    ok(errorFlag== false, "psBinaryOp(): was successful: (matrix-matrix, -, S32)");
    ok(memoryFlag== false, "no memory leaks");
    testBinaryOpMM(-,F32,20.0,10.0,3,2,errorFlag,memoryFlag);
    ok(errorFlag== false, "psBinaryOp(): was successful: (matrix-matrix, -, F32)");
    ok(memoryFlag== false, "no memory leaks");
    testBinaryOpMM(-,F64,20.0,10.0,3,2,errorFlag,memoryFlag);
    ok(errorFlag== false, "psBinaryOp(): was successful: (matrix-matrix, -, F64)");
    ok(memoryFlag== false, "no memory leaks");
    testBinaryOpMM(*,S32,20,10,3,2,errorFlag,memoryFlag);
    ok(errorFlag== false, "psBinaryOp(): was successful: (matrix-matrix, *, S32)");
    ok(memoryFlag== false, "no memory leaks");
    testBinaryOpMM(*,F32,20.0,10.0,3,2,errorFlag,memoryFlag);
    ok(errorFlag== false, "psBinaryOp(): was successful: (matrix-matrix, *, F32)");
    ok(memoryFlag== false, "no memory leaks");
    testBinaryOpMM(*,F64,20.0,10.0,3,2,errorFlag,memoryFlag);
    ok(errorFlag== false, "psBinaryOp(): was successful: (matrix-matrix, *, F64)");
    ok(memoryFlag== false, "no memory leaks");
    testBinaryOpMM(/,S32,20,10,3,2,errorFlag,memoryFlag);
    ok(errorFlag== false, "psBinaryOp(): was successful: (matrix-matrix, /, S32)");
    ok(memoryFlag== false, "no memory leaks");
    testBinaryOpMM(/,F32,20.0,10.0,3,2,errorFlag,memoryFlag);
    ok(errorFlag== false, "psBinaryOp(): was successful: (matrix-matrix, /, F32)");
    ok(memoryFlag== false, "no memory leaks");
    testBinaryOpMM(/,F64,20.0,10.0,3,2,errorFlag,memoryFlag);
    ok(errorFlag== false, "psBinaryOp(): was successful: (matrix-matrix, /, F64)");
    ok(memoryFlag== false, "no memory leaks");

    // Test Matrix-Vector binary operations
    testBinaryOpMV(+,S32,10,5,3,2,errorFlag,memoryFlag);
    ok(errorFlag== false, "psBinaryOp(): was successful: (matrix-vector, +, S32)");
    ok(memoryFlag== false, "no memory leaks");
    testBinaryOpMV(+,F32,10.0,5.0,3,2,errorFlag,memoryFlag);
    ok(errorFlag== false, "psBinaryOp(): was successful: (matrix-vector, +, F32)");
    ok(memoryFlag== false, "no memory leaks");
    testBinaryOpMV(+,F64,10.0,5.0,3,2,errorFlag,memoryFlag);
    ok(errorFlag== false, "psBinaryOp(): was successful: (matrix-vector, +, F64)");
    ok(memoryFlag== false, "no memory leaks");
    testBinaryOpMV(-,S32,20,5,3,2,errorFlag,memoryFlag);
    ok(errorFlag== false, "psBinaryOp(): was successful: (matrix-vector, -, S32)");
    ok(memoryFlag== false, "no memory leaks");
    testBinaryOpMV(-,F32,20.0,5.0,3,2,errorFlag,memoryFlag);
    ok(errorFlag== false, "psBinaryOp(): was successful: (matrix-vector, -, F32)");
    ok(memoryFlag== false, "no memory leaks");
    testBinaryOpMV(-,F64,20.0,5.0,3,2,errorFlag,memoryFlag);
    ok(errorFlag== false, "psBinaryOp(): was successful: (matrix-vector, -, F32)");
    ok(memoryFlag== false, "no memory leaks");
    testBinaryOpMV(*,S32,20,5,3,2,errorFlag,memoryFlag);
    ok(errorFlag== false, "psBinaryOp(): was successful: (matrix-vector, *, S32)");
    ok(memoryFlag== false, "no memory leaks");
    testBinaryOpMV(*,F32,20.0,5.0,3,2,errorFlag,memoryFlag);
    ok(errorFlag== false, "psBinaryOp(): was successful: (matrix-vector, *, F32)");
    ok(memoryFlag== false, "no memory leaks");
    testBinaryOpMV(*,F64,20.0,5.0,3,2,errorFlag,memoryFlag);
    ok(errorFlag== false, "psBinaryOp(): was successful: (matrix-vector, *, F64)");
    ok(memoryFlag== false, "no memory leaks");
    testBinaryOpMV(/,S32,20,5,3,2,errorFlag,memoryFlag);
    ok(errorFlag== false, "psBinaryOp(): was successful: (matrix-vector, /, S32)");
    ok(memoryFlag== false, "no memory leaks");
    testBinaryOpMV(/,F32,20.0,5.0,3,2,errorFlag,memoryFlag);
    ok(errorFlag== false, "psBinaryOp(): was successful: (matrix-vector, /, F32)");
    ok(memoryFlag== false, "no memory leaks");
    testBinaryOpMV(/,F64,20.0,5.0,3,2,errorFlag,memoryFlag);
    ok(errorFlag== false, "psBinaryOp(): was successful: (matrix-vector, /, F64)");
    ok(memoryFlag== false, "no memory leaks");

    // Test Matrix-Scalar binary operations
    testBinaryOpMS(+,S32,10,5,3,2,errorFlag,memoryFlag);
    ok(errorFlag== false, "psBinaryOp(): was successful: (matrix-scalar, +, S32)");
    ok(memoryFlag== false, "no memory leaks");
    testBinaryOpMS(+,F32,10.0,5.0,3,2,errorFlag,memoryFlag);
    ok(errorFlag== false, "psBinaryOp(): was successful: (matrix-scalar, +, F32)");
    ok(memoryFlag== false, "no memory leaks");
    testBinaryOpMS(+,F64,10.0,5.0,3,2,errorFlag,memoryFlag);
    ok(errorFlag== false, "psBinaryOp(): was successful: (matrix-scalar, +, F64)");
    ok(memoryFlag== false, "no memory leaks");
    testBinaryOpMS(-,S32,20,5,3,2,errorFlag,memoryFlag);
    ok(errorFlag== false, "psBinaryOp(): was successful: (matrix-scalar, -, S32)");
    ok(memoryFlag== false, "no memory leaks");
    testBinaryOpMS(-,F32,20.0,5.0,3,2,errorFlag,memoryFlag);
    ok(errorFlag== false, "psBinaryOp(): was successful: (matrix-scalar, -, F32)");
    ok(memoryFlag== false, "no memory leaks");
    testBinaryOpMS(-,F64,20.0,5.0,3,2,errorFlag,memoryFlag);
    ok(errorFlag== false, "psBinaryOp(): was successful: (matrix-scalar, -, F64)");
    ok(memoryFlag== false, "no memory leaks");
    testBinaryOpMS(*,S32,20,5,3,2,errorFlag,memoryFlag);
    ok(errorFlag== false, "psBinaryOp(): was successful: (matrix-scalar, *, S32)");
    ok(memoryFlag== false, "no memory leaks");
    testBinaryOpMS(*,F32,20.0,5.0,3,2,errorFlag,memoryFlag);
    ok(errorFlag== false, "psBinaryOp(): was successful: (matrix-scalar, *, F32)");
    ok(memoryFlag== false, "no memory leaks");
    testBinaryOpMS(*,F64,20.0,5.0,3,2,errorFlag,memoryFlag);
    ok(errorFlag== false, "psBinaryOp(): was successful: (matrix-scalar, *, F64)");
    ok(memoryFlag== false, "no memory leaks");
    testBinaryOpMS(/,S32,20,5,3,2,errorFlag,memoryFlag);
    ok(errorFlag== false, "psBinaryOp(): was successful: (matrix-scalar, /, S32)");
    ok(memoryFlag== false, "no memory leaks");
    testBinaryOpMS(/,F32,20.0,5.0,3,2,errorFlag,memoryFlag);
    ok(errorFlag== false, "psBinaryOp(): was successful: (matrix-scalar, /, F32)");
    ok(memoryFlag== false, "no memory leaks");
    testBinaryOpMS(/,F64,20.0,5.0,3,2,errorFlag,memoryFlag);
    ok(errorFlag== false, "psBinaryOp(): was successful: (matrix-scalar, /, F64)");
    ok(memoryFlag== false, "no memory leaks");
    return 0;
}

