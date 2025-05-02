
/** @file  tst_psMatrixVectorArithmetic02.c
 *
 *  @brief Test driver for psMatrixVector arithmetic functions
 *
 *  This test driver tests combinations of matrix, vector, and scalar unary operations including:
 *     Matrix with all math operators with S32, F32, F64
 *     Vector with all math operators with S32, F32, F64
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
#include <math.h>
#include <pslib.h>
#include "tap.h"
#include "pstap.h"

#define CHECK_VECTOR(VECTOR,TYPE,TRUTH,ERRORFLAG)                                                                      \
{ \
    for(psS32 i=0; i<VECTOR->n; i++) {                                                                             \
        if(cabs(VECTOR->data.TYPE[i])-cabs(TRUTH) > FLT_EPSILON){                                                \
            ERRORFLAG = true; \
            diag("ERROR:Truth and calculated values don't match for vector operation:\n");                     \
            if(PS_IS_PSELEMTYPE_COMPLEX(VECTOR->type.type)) {                                                    \
                diag("Truth: %.2f%+.2fi\n", creal(VECTOR->data.TYPE[i]), cimag(VECTOR->data.TYPE[i]));         \
                diag("Calculated: %.2f%+.2fi\n", creal(TRUTH), cimag(TRUTH));                                  \
            } else if(PS_IS_PSELEMTYPE_INT(VECTOR->type.type)) {                                                 \
                diag("Truth: %d\n", (psS32)(VECTOR->data.TYPE[i]));                                              \
                diag("Calculated: %d\n", (psS32)(TRUTH));                                                        \
            } else {                                                                                             \
                diag("Truth: %.2f\n", (double)(VECTOR->data.TYPE[i]));                                         \
                diag("Calculated: %.2f\n", (double)(TRUTH));                                                   \
            }                                                                                                    \
            diag("\n"); \
        }                                                                                                        \
    }                                                                                                            \
}

#define CHECK_MATRIX(IMAGE,TYPE,TRUTH,ERRORFLAG)                                                                       \
{ \
    for(psS32 i=IMAGE->numRows-1; i>-1; i--) {                                                                     \
        for(psS32 j=0; j<IMAGE->numCols; j++) {                                                                    \
            if(cabs(IMAGE->data.TYPE[i][j])-cabs(TRUTH) > FLT_EPSILON){                                          \
                ERRORFLAG = true; \
                diag("ERROR:Truth and calculated values don't match for matrix operation:\n");                 \
                if(PS_IS_PSELEMTYPE_COMPLEX(IMAGE->type.type)) {                                                 \
                    diag("Truth: %.2f%+.2fi\n", creal(IMAGE->data.TYPE[i][j]), cimag(IMAGE->data.TYPE[i][j])); \
                    diag("Calculated: %.2f%+.2fi\n", creal(TRUTH), cimag(TRUTH));                              \
                } else if(PS_IS_PSELEMTYPE_INT(IMAGE->type.type)) {                                              \
                    diag("Truth: %d\n", (psS32)(IMAGE->data.TYPE[i][j]));                                        \
                    diag("Calculated: %d\n", (psS32)(TRUTH));                                                    \
                } else {                                                                                         \
                    diag("Truth: %.2f\n", (double)(IMAGE->data.TYPE[i][j]));                                   \
                    diag("Calculated: %.2f\n", (double)(TRUTH));                                               \
                }                                                                                                \
                diag("\n");                                                                                            \
                diag("\n");                                                                                            \
            }                                                                                                    \
        }                                                                                                        \
    }                                                                                                            \
}

#define CREATE_AND_SET_VECTOR(NAME,TYPE,VALUE,SIZE)                                                          \
psVector *NAME = (psVector*)psVectorAlloc(SIZE, PS_TYPE_##TYPE);                                             \
for(psS32 i=0; i<SIZE; i++) {                                                                                  \
    NAME->data.TYPE[i] = VALUE;                                                                              \
}                                                                                                            \
NAME->n = SIZE;


#define CREATE_AND_SET_IMAGE(NAME,TYPE,VALUE,NROWS,NCOLS)                                                    \
psImage *NAME = (psImage*)psImageAlloc(NCOLS,NROWS,PS_TYPE_##TYPE);                                          \
for(psS32 i=0; i<NAME->numRows; i++) {                                                                         \
    for(psS32 j=0; j<NAME->numCols; j++) {                                                                     \
        NAME->data.TYPE[i][j] = VALUE;                                                                       \
    }                                                                                                        \
}


#define CHECK_MEMORY \
if( psMemCheckLeaks(0, NULL, stdout, false) != 0 ) {  \
    psError(PS_ERR_UNKNOWN, true,"Memory leaks detected."); \
    return 50; \
} \
psS32 nBad = psMemCheckCorruption(0); \
if(nBad) { \
    psError(PS_ERR_UNKNOWN, true,"ERROR: Found %d bad memory blocks\n", nBad); \
    return 51; \
}

// Test matrix unary operations
#define testUnaryOpM(OP,TYPE,VALUE1,VALUE2,NROWS,NCOLS,TRUTH,ERRORFLAG,MEMORYFLAG) \
{                                                                                  \
    psMemId id = psMemGetId();                                                     \
    ERRORFLAG = false;                                                             \
    MEMORYFLAG = false;                                                            \
    CREATE_AND_SET_IMAGE(inImage,TYPE,VALUE1,NROWS,NCOLS);                         \
    CREATE_AND_SET_IMAGE(outImage,TYPE,VALUE2,NROWS,NCOLS);                        \
    outImage = (psImage*)psUnaryOp(outImage, inImage, #OP);                        \
    CHECK_MATRIX(outImage,TYPE,TRUTH,ERRORFLAG);                                   \
    psFree(inImage);                                                               \
    psFree(outImage);                                                              \
    MEMORYFLAG = psMemCheckLeaks(id, NULL, NULL, false);                           \
}

// Test vector unary operations
#define testUnaryOpV(OP,TYPE,VALUE1,VALUE2,SIZE,TRUTH,ERRORFLAG,MEMORYFLAG)        \
{                                                                                  \
    psMemId id = psMemGetId();                                                     \
    ERRORFLAG = false;                                                             \
    MEMORYFLAG = false;                                                            \
    CREATE_AND_SET_VECTOR(inVector,TYPE,VALUE1,SIZE);                              \
    CREATE_AND_SET_VECTOR(outVector,TYPE,VALUE2,SIZE);                             \
    outVector = (psVector*)psUnaryOp(outVector, inVector, #OP);                    \
    CHECK_VECTOR(outVector,TYPE,TRUTH,ERRORFLAG);                                  \
    psFree(inVector);                                                              \
    psFree(outVector);                                                             \
    MEMORYFLAG = psMemCheckLeaks (id, NULL, NULL, false);                          \
}


psS32 main( psS32 argc, char* argv[] )
{
    psLogSetFormat("HLNM");
    plan_tests(272);
    bool errorFlag = false;
    bool memoryFlag = false;

    testUnaryOpM( abs, S32, -10, 0, 3, 2, 10, errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): abs was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpM( abs, F32, -10.0, 0.0, 3, 2, 10.0, errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): abs was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpM( abs, F64, -10.0, 0.0, 3, 2, 10.0, errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): abs was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpM( exp, S32, 10, 0, 3, 2, exp(10), errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): exp was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpM( exp, F32, 10.0, 0.0, 3, 2, cexp(10.0), errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): exp was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpM( exp, F64, 10.0, 0.0, 3, 2, cexp(10.0), errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): exp was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpM( ln, S32, 10, 0, 3, 2, clog(10), errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): ln was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpM( ln, F32, 10.0, 0.0, 3, 2, clog(10.0), errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): ln was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpM( ln, F64, 10.0, 0.0, 3, 2, clog(10.0), errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): ln was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpM( ten, S32, 3, 0, 3, 2, 1000, errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): ten was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpM( ten, F32, 3.0, 0.0, 3, 2, 1000, errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): ten was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpM( ten, F64, 3.0, 0.0, 3, 2, 1000, errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): ten was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpM( log, S32, 1000, 0, 3, 2, 3, errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): log was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpM( log, F32, 1000.0, 0.0, 3, 2, 3, errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): log was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpM( log, F64, 1000.0, 0.0, 3, 2, 3, errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): log was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpM( sin, S32, M_PI_2, 0, 3, 2, 1, errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): sin was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpM( sin, F32, M_PI_2, 0.0, 3, 2, 1.0, errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): sin was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpM( sin, F64, M_PI_2, 0.0, 3, 2, 1.0, errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): sin was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpM( dsin, S32, 90, 0, 3, 2 , 1, errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): dsin was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpM( dsin, F32, 90.0, 0.0, 3, 2, 1.0, errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): dsin was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpM( dsin, F64, 90.0, 0.0, 3, 2, 1.0, errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): dsin was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpM( cos, S32, 0, 0, 3, 2, 1, errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): cos was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpM( cos, F32, 0.0, 0.0, 3, 2, 1.0, errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): cos was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpM( cos, F64, 0.0, 0.0, 3, 2, 1.0, errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): cos was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpM( dcos, S32, 0, 0, 3, 2, 1, errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): dcos was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpM( dcos, F32, 0.0, 0.0, 3, 2, 1.0, errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): dcos was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpM( dcos, F64, 0.0, 0.0, 3, 2, 1.0, errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): dcos was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpM( tan, S32, M_PI_4, 0, 3, 2, 1, errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): tan was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpM( tan, F32, M_PI_4, 0.0, 3, 2, 1.0, errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): tan was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpM( tan, F64, M_PI_4, 0.0, 3, 2, 1.0, errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): tan was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpM( dtan, S32, 45, 0, 3, 2, 1, errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): dtan was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpM( dtan, F32, 45.0, 0.0, 3, 2, 1.0, errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): dtan was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpM( dtan, F64, 45.0, 0.0, 3, 2, 1.0, errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): dtan was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpM( asin, S32, 1, 0, 3, 2, M_PI_2, errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): asin was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpM( asin, F32, 1.0, 0.0, 3, 2, M_PI_2 , errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): asin was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpM( asin, F64, 1.0, 0.0, 3, 2, M_PI_2, errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): asin was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpM( dasin, S32, 1.0, 0, 3, 2, 90, errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): dasin was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpM( dasin, F32, 1.0, 0.0, 3, 2, 90.0, errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): dasin was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpM( dasin, F64, 1.0, 0.0, 3, 2, 90.0, errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): dasin was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpM( acos, S32, 0, 0, 3, 2, M_PI_2, errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): acos was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpM( acos, F32, 0.0, 0.0, 3, 2, M_PI_2, errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): acos was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpM( acos, F64, 0.0, 0.0, 3, 2, M_PI_2, errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): acos was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpM( dacos, S32, 0, 0, 3, 2, 90, errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): dacos was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpM( dacos, F32, 0.0, 0.0, 3, 2, 90.0, errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): dacos was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpM( dacos, F64, 0.0, 0.0, 3, 2, 90.0, errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): dacos was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpM( atan, S32, 1, 0, 3, 2, M_PI_4, errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): atan was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpM( atan, F32, 1.0, 0.0, 3, 2, M_PI_4, errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): atan was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpM( atan, F64, 1.0, 0.0, 3, 2, M_PI_4, errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): atan was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpM( datan, S32, 1, 0, 3, 2, 45, errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): datan was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpM( datan, F32, 1.0, 0.0, 3, 2, 45.0, errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): datan was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpM( datan, F64, 1.0, 0.0, 3, 2, 45.0, errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): datan was successful");
    ok(memoryFlag== false, "no memory leaks");

    testUnaryOpV( abs, S32, -10, 0, 3, 10, errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): abs was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpV( abs, F32, -10.0, 0.0, 3, 10.0, errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): abs was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpV( abs, F64, -10.0, 0.0, 3, 10.0, errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): abs was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpV( exp, S32, 10, 0, 3, cexp(10), errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): exp was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpV( exp, F32, 10.0, 0.0, 3, cexp(10.0), errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): exp was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpV( exp, F64, 10.0, 0.0, 3, cexp(10.0), errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): exp was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpV( ln, S32, 10, 0, 3, clog(10), errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): ln was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpV( ln, F32, 10.0, 0.0, 3, clog(10.0), errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): ln was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpV( ln, F64, 10.0, 0.0, 3, clog(10.0), errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): ln was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpV( ten, S32, 3, 0, 3, 1000, errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): ten was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpV( ten, F32, 3.0, 0.0, 3, 1000, errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): ten was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpV( ten, F64, 3.0, 0.0, 3, 1000, errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): ten was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpV( log, S32, 1000, 0, 3,  3, errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): log was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpV( log, F32, 1000.0, 0.0, 3, 3, errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): log was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpV( log, F64, 1000.0, 0.0, 3, 3, errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): log was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpV( sin, S32, M_PI_2, 0, 3, 1, errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): sin was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpV( sin, F32, M_PI_2, 0.0, 3, 1.0, errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): sin was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpV( sin, F64, M_PI_2, 0.0, 3, 1.0, errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): sin was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpV( dsin, S32, 90, 0, 3, 1, errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): dsin was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpV( dsin, F32, 90.0, 0.0, 3, 1.0, errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): dsin was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpV( dsin, F64, 90.0, 0.0, 3, 1.0, errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): dsin was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpV( cos, S32, 0, 0, 3, 1, errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): cos was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpV( cos, F32, 0.0, 0.0, 3, 1.0, errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): cos was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpV( cos, F64, 0.0, 0.0, 3, 1.0, errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): cos was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpV( dcos, S32, 0, 0, 3, 1, errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): dcos was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpV( dcos, F32, 0.0, 0.0, 3, 1.0, errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): dcos was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpV( dcos, F64, 0.0, 0.0, 3, 1.0, errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): dcos was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpV( tan, S32, M_PI_4, 0, 3, 1, errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): tan was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpV( tan, F32, M_PI_4, 0.0, 3, 1.0, errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): tan was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpV( tan, F64, M_PI_4, 0.0, 3, 1.0, errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): tan was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpV( dtan, S32, 45, 0, 3, 1, errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): dtan was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpV( dtan, F32, 45.0, 0.0, 3, 1.0, errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): dtan was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpV( dtan, F64, 45.0, 0.0, 3, 1.0, errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): dtan was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpV( asin, S32, 1, 0, 3, M_PI_2, errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): asin was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpV( asin, F32, 1.0, 0.0, 3, M_PI_2 , errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): asin was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpV( asin, F64, 1.0, 0.0, 3, M_PI_2, errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): asin was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpV( dasin, S32, 1.0, 0, 3, 90, errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): dasin was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpV( dasin, F32, 1.0, 0.0, 3, 90.0, errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): dasin was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpV( dasin, F64, 1.0, 0.0, 3, 90.0, errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): dasin was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpV( acos, S32, 0, 0, 3, M_PI_2, errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): acos was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpV( acos, F32, 0.0, 0.0, 3, M_PI_2, errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): acos was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpV( acos, F64, 0.0, 0.0, 3, M_PI_2, errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): acos was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpV( dacos, S32, 0, 0, 3, 90, errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): dacos was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpV( dacos, F32, 0.0, 0.0, 3, 90.0, errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): dacos was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpV( dacos, F64, 0.0, 0.0, 3, 90.0, errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): dacos was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpV( atan, S32, 1, 0, 3, M_PI_4, errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): atan was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpV( atan, F32, 1.0, 0.0, 3, M_PI_4, errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): atan was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpV( atan, F64, 1.0, 0.0, 3, M_PI_4, errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): atan was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpV( datan, S32, 1, 0, 3, 45, errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): datan was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpV( datan, F32, 1.0, 0.0, 3, 45.0, errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): datan was successful");
    ok(memoryFlag== false, "no memory leaks");
    testUnaryOpV( datan, F64, 1.0, 0.0, 3, 45.0, errorFlag, memoryFlag);
    ok(errorFlag== false, "psUnaryOp(): datan was successful");
    ok(memoryFlag== false, "no memory leaks");

    return 0;
}

