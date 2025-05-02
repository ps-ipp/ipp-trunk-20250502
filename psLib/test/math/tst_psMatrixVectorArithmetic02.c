
/** @file  tst_psMatrixVectorArithmetic02.c
 *
 *  @brief Test driver for psMatrixVector arithmetic functions
 *
 *  This test driver tests combinations of matrix, vector, and scalar unary operations including:
 *     Matrix with all math operators with S32, F32, F64, C32
 *     Vector with all math operators with S32, F32, F64, C32
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

#include <math.h>


#define CHECK_VECTOR(VECTOR,TYPE,TRUTH)                                                                      \
for(psS32 i=0; i<VECTOR->n; i++) {                                                                             \
    if(cabs(VECTOR->data.TYPE[i])-cabs(TRUTH) > FLT_EPSILON){                                                \
        printf("ERROR:Truth and calculated values don't match for vector operation:\n");                     \
        if(PS_IS_PSELEMTYPE_COMPLEX(VECTOR->type.type)) {                                                    \
            printf("Truth: %.2f%+.2fi\n", creal(VECTOR->data.TYPE[i]), cimag(VECTOR->data.TYPE[i]));         \
            printf("Calculated: %.2f%+.2fi\n", creal(TRUTH), cimag(TRUTH));                                  \
        } else if(PS_IS_PSELEMTYPE_INT(VECTOR->type.type)) {                                                 \
            printf("Truth: %d\n", (psS32)(VECTOR->data.TYPE[i]));                                              \
            printf("Calculated: %d\n", (psS32)(TRUTH));                                                        \
        } else {                                                                                             \
            printf("Truth: %.2f\n", (double)(VECTOR->data.TYPE[i]));                                         \
            printf("Calculated: %.2f\n", (double)(TRUTH));                                                   \
        }                                                                                                    \
    }                                                                                                        \
}                                                                                                            \
printf("\n");


#define CHECK_MATRIX(IMAGE,TYPE,TRUTH)                                                                       \
for(psS32 i=IMAGE->numRows-1; i>-1; i--) {                                                                     \
    for(psS32 j=0; j<IMAGE->numCols; j++) {                                                                    \
        if(cabs(IMAGE->data.TYPE[i][j])-cabs(TRUTH) > FLT_EPSILON){                                          \
            printf("ERROR:Truth and calculated values don't match for matrix operation:\n");                 \
            if(PS_IS_PSELEMTYPE_COMPLEX(IMAGE->type.type)) {                                                 \
                printf("Truth: %.2f%+.2fi\n", creal(IMAGE->data.TYPE[i][j]), cimag(IMAGE->data.TYPE[i][j])); \
                printf("Calculated: %.2f%+.2fi\n", creal(TRUTH), cimag(TRUTH));                              \
            } else if(PS_IS_PSELEMTYPE_INT(IMAGE->type.type)) {                                              \
                printf("Truth: %d\n", (psS32)(IMAGE->data.TYPE[i][j]));                                        \
                printf("Calculated: %d\n", (psS32)(TRUTH));                                                    \
            } else {                                                                                         \
                printf("Truth: %.2f\n", (double)(IMAGE->data.TYPE[i][j]));                                   \
                printf("Calculated: %.2f\n", (double)(TRUTH));                                               \
            }                                                                                                \
        }                                                                                                    \
    }                                                                                                        \
    printf("\n");                                                                                            \
}                                                                                                            \
printf("\n");


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


psS32 main( psS32 argc, char* argv[] )
{
    psLogSetFormat("HLNM");
    // Test matrix unary operations
    #define testUnaryOpM(OP,TYPE,VALUE1,VALUE2,NROWS,NCOLS,TRUTH)                                            \
    {                                                                                                        \
        printPositiveTestHeader(stdout, "psMatrixVectorArithmetic", "Test matrix psUnaryOp");                \
        printf("Operation: %s\n", #OP);                                                                      \
        CREATE_AND_SET_IMAGE(inImage,TYPE,VALUE1,NROWS,NCOLS);                                               \
        CREATE_AND_SET_IMAGE(outImage,TYPE,VALUE2,NROWS,NCOLS);                                              \
        outImage = (psImage*)psUnaryOp(outImage, inImage, #OP);                                              \
        CHECK_MATRIX(outImage,TYPE,TRUTH);                                                                   \
        psFree(inImage);                                                                                     \
        psFree(outImage);                                                                                    \
        CHECK_MEMORY;                                                                                        \
        printFooter(stdout, "psMatrixVectorArithmetic", "Test matrix psUnaryOp", true);                      \
    }

    testUnaryOpM( abs, S32, -10, 0, 3, 2, 10 );
    testUnaryOpM( abs, F32, -10.0, 0.0, 3, 2, 10.0 );
    testUnaryOpM( abs, F64, -10.0, 0.0, 3, 2, 10.0 );
    testUnaryOpM( abs, C32, -10.0 - 10.0i, 0.0 + 0.0i, 3, 2,10+10i );
    testUnaryOpM( exp, S32, 10, 0, 3, 2, cexp(10));
    testUnaryOpM( exp, F32, 10.0, 0.0, 3, 2, cexp(10.0) );
    testUnaryOpM( exp, F64, 10.0, 0.0, 3, 2, cexp(10.0) );
    testUnaryOpM( exp, C32, 1.0 + 1.0i, 0.0 + 0.0i, 3, 2, cexp(1.0+1.0i) );
    testUnaryOpM( ln, S32, 10, 0, 3, 2, clog(10) );
    testUnaryOpM( ln, F32, 10.0, 0.0, 3, 2, clog(10.0) );
    testUnaryOpM( ln, F64, 10.0, 0.0, 3, 2, clog(10.0) );
    testUnaryOpM( ln, C32, 10.0 + 10.0i, 0.0 + 0.0i, 3, 2, clog(10.0+10.0i) );
    testUnaryOpM( ten, S32, 3, 0, 3, 2, 1000 );
    testUnaryOpM( ten, F32, 3.0, 0.0, 3, 2, 1000 );
    testUnaryOpM( ten, F64, 3.0, 0.0, 3, 2, 1000 );
    testUnaryOpM( ten, C32, 1.0 + 0.0i, 0.0 + 0.0i, 3, 2, 10.0 );
    testUnaryOpM( log, S32, 1000, 0, 3, 2, 3 );
    testUnaryOpM( log, F32, 1000.0, 0.0, 3, 2, 3 );
    testUnaryOpM( log, F64, 1000.0, 0.0, 3, 2, 3 );
    testUnaryOpM( log, C32, 1000.0 + 0.0i, 0.0 + 0.0i, 3, 2, 3 );
    testUnaryOpM( sin, S32, M_PI_2, 0, 3, 2, 1 );
    testUnaryOpM( sin, F32, M_PI_2, 0.0, 3, 2, 1.0 );
    testUnaryOpM( sin, F64, M_PI_2, 0.0, 3, 2, 1.0 );
    testUnaryOpM( sin, C32, M_PI_2 + 0.0i, 0.0 + 0.0i, 3, 2, 1.0 );
    testUnaryOpM( dsin, S32, 90, 0, 3, 2 , 1);
    testUnaryOpM( dsin, F32, 90.0, 0.0, 3, 2, 1.0 );
    testUnaryOpM( dsin, F64, 90.0, 0.0, 3, 2, 1.0 );
    testUnaryOpM( dsin, C32, 90.0 + 00.0i, 0.0 + 0.0i, 3, 2, 1.0 );
    testUnaryOpM( cos, S32, 0, 0, 3, 2, 1 );
    testUnaryOpM( cos, F32, 0.0, 0.0, 3, 2, 1.0 );
    testUnaryOpM( cos, F64, 0.0, 0.0, 3, 2, 1.0 );
    testUnaryOpM( cos, C32, 0.0 + 0.0i, 0.0 + 0.0i, 3, 2, 1.0 );
    testUnaryOpM( dcos, S32, 0, 0, 3, 2, 1 );
    testUnaryOpM( dcos, F32, 0.0, 0.0, 3, 2, 1.0 );
    testUnaryOpM( dcos, F64, 0.0, 0.0, 3, 2, 1.0 );
    testUnaryOpM( dcos, C32, 0.0 + 0.0i, 0.0 + 0.0i, 3, 2, 1.0 );
    testUnaryOpM( tan, S32, M_PI_4, 0, 3, 2, 1);
    testUnaryOpM( tan, F32, M_PI_4, 0.0, 3, 2, 1.0 );
    testUnaryOpM( tan, F64, M_PI_4, 0.0, 3, 2, 1.0 );
    testUnaryOpM( tan, C32, M_PI_4 + 0.0i, 0.0 + 0.0i, 3, 2, 1 );
    testUnaryOpM( dtan, S32, 45, 0, 3, 2, 1 );
    testUnaryOpM( dtan, F32, 45.0, 0.0, 3, 2, 1.0 );
    testUnaryOpM( dtan, F64, 45.0, 0.0, 3, 2, 1.0 );
    testUnaryOpM( dtan, C32, 45.0 + 45.0i, 0.0 + 0.0i, 3, 2, 1.0 );
    testUnaryOpM( asin, S32, 1, 0, 3, 2, M_PI_2);
    testUnaryOpM( asin, F32, 1.0, 0.0, 3, 2, M_PI_2  );
    testUnaryOpM( asin, F64, 1.0, 0.0, 3, 2, M_PI_2);
    testUnaryOpM( asin, C32, 1.0 + 1.0i, 0.0 + 0.0i, 3, 2, M_PI_2);
    testUnaryOpM( dasin, S32, 1.0, 0, 3, 2, 90 );
    testUnaryOpM( dasin, F32, 1.0, 0.0, 3, 2, 90.0 );
    testUnaryOpM( dasin, F64, 1.0, 0.0, 3, 2, 90.0 );
    testUnaryOpM( dasin, C32, 1.0 + 1.0i, 0.0 + 0.0i, 3, 2, 90.0 );
    testUnaryOpM( acos, S32, 0, 0, 3, 2, M_PI_2);
    testUnaryOpM( acos, F32, 0.0, 0.0, 3, 2, M_PI_2 );
    testUnaryOpM( acos, F64, 0.0, 0.0, 3, 2, M_PI_2 );
    testUnaryOpM( acos, C32, 0.0 + 0.0i, 0.0 + 0.0i, 3, 2, M_PI_2 );
    testUnaryOpM( dacos, S32, 0, 0, 3, 2, 90 );
    testUnaryOpM( dacos, F32, 0.0, 0.0, 3, 2, 90.0 );
    testUnaryOpM( dacos, F64, 0.0, 0.0, 3, 2, 90.0 );
    testUnaryOpM( dacos, C32, 0.0 + 0.0i, 0.0 + 0.0i, 3, 2, 90.0 );
    testUnaryOpM( atan, S32, 1, 0, 3, 2, M_PI_4);
    testUnaryOpM( atan, F32, 1.0, 0.0, 3, 2, M_PI_4 );
    testUnaryOpM( atan, F64, 1.0, 0.0, 3, 2, M_PI_4);
    testUnaryOpM( atan, C32, 1.0 + 0.0i, 0.0 + 0.0i, 3, 2, M_PI_4);
    testUnaryOpM( datan, S32, 1, 0, 3, 2, 45 );
    testUnaryOpM( datan, F32, 1.0, 0.0, 3, 2, 45.0 );
    testUnaryOpM( datan, F64, 1.0, 0.0, 3, 2, 45.0 );
    testUnaryOpM( datan, C32, 1.0 + 0.0i, 0.0 + 0.0i, 3, 2, 45.0 );


    // Test vector unary operations
    #define testUnaryOpV(OP,TYPE,VALUE1,VALUE2,SIZE,TRUTH)                                                   \
    {                                                                                                        \
        printPositiveTestHeader(stdout, "psMatrixVectorArithmetic", "Test vector psUnaryOp");                \
        printf("Operation: %s\n", #OP);                                                                      \
        CREATE_AND_SET_VECTOR(inVector,TYPE,VALUE1,SIZE);                                                    \
        CREATE_AND_SET_VECTOR(outVector,TYPE,VALUE2,SIZE);                                                   \
        outVector = (psVector*)psUnaryOp(outVector, inVector, #OP);                                          \
        CHECK_VECTOR(outVector,TYPE,TRUTH);                                                                  \
        psFree(inVector);                                                                                    \
        psFree(outVector);                                                                                   \
        CHECK_MEMORY;                                                                                        \
        printFooter(stdout, "psMatrixVectorArithmetic", "Test vector psUnaryOp", true);                      \
    }

    testUnaryOpV( abs, S32, -10, 0, 3, 10 );
    testUnaryOpV( abs, F32, -10.0, 0.0, 3, 10.0 );
    testUnaryOpV( abs, F64, -10.0, 0.0, 3, 10.0 );
    testUnaryOpV( abs, C32, -10.0 - 10.0i, 0.0 + 0.0i, 3, 10+10i );
    testUnaryOpV( exp, S32, 10, 0, 3, cexp(10));
    testUnaryOpV( exp, F32, 10.0, 0.0, 3, cexp(10.0) );
    testUnaryOpV( exp, F64, 10.0, 0.0, 3, cexp(10.0) );
    testUnaryOpV( exp, C32, 1.0 + 1.0i, 0.0 + 0.0i, 3, cexp(1.0+1.0i) );
    testUnaryOpV( ln, S32, 10, 0, 3, clog(10) );
    testUnaryOpV( ln, F32, 10.0, 0.0, 3, clog(10.0) );
    testUnaryOpV( ln, F64, 10.0, 0.0, 3, clog(10.0) );
    testUnaryOpV( ln, C32, 10.0 + 10.0i, 0.0 + 0.0i, 3, clog(10.0+10.0i) );
    testUnaryOpV( ten, S32, 3, 0, 3, 1000 );
    testUnaryOpV( ten, F32, 3.0, 0.0, 3, 1000 );
    testUnaryOpV( ten, F64, 3.0, 0.0, 3, 1000 );
    testUnaryOpV( ten, C32, 1.0 + 0.0i, 0.0 + 0.0i, 3, 10.0 );
    testUnaryOpV( log, S32, 1000, 0, 3,  3 );
    testUnaryOpV( log, F32, 1000.0, 0.0, 3, 3 );
    testUnaryOpV( log, F64, 1000.0, 0.0, 3, 3 );
    testUnaryOpV( log, C32, 1000.0 + 0.0i, 0.0 + 0.0i, 3, 3 );
    testUnaryOpV( sin, S32, M_PI_2, 0, 3, 1 );
    testUnaryOpV( sin, F32, M_PI_2, 0.0, 3, 1.0 );
    testUnaryOpV( sin, F64, M_PI_2, 0.0, 3, 1.0 );
    testUnaryOpV( sin, C32, M_PI_2 + 0.0i, 0.0 + 0.0i, 3, 1.0 );
    testUnaryOpV( dsin, S32, 90, 0, 3, 1);
    testUnaryOpV( dsin, F32, 90.0, 0.0, 3, 1.0 );
    testUnaryOpV( dsin, F64, 90.0, 0.0, 3, 1.0 );
    testUnaryOpV( dsin, C32, 90.0 + 00.0i, 0.0 + 0.0i, 3, 1.0 );
    testUnaryOpV( cos, S32, 0, 0, 3, 1 );
    testUnaryOpV( cos, F32, 0.0, 0.0, 3, 1.0 );
    testUnaryOpV( cos, F64, 0.0, 0.0, 3, 1.0 );
    testUnaryOpV( cos, C32, 0.0 + 0.0i, 0.0 + 0.0i, 3, 1.0 );
    testUnaryOpV( dcos, S32, 0, 0, 3, 1 );
    testUnaryOpV( dcos, F32, 0.0, 0.0, 3, 1.0 );
    testUnaryOpV( dcos, F64, 0.0, 0.0, 3, 1.0 );
    testUnaryOpV( dcos, C32, 0.0 + 0.0i, 0.0 + 0.0i, 3, 1.0 );
    testUnaryOpV( tan, S32, M_PI_4, 0, 3, 1);
    testUnaryOpV( tan, F32, M_PI_4, 0.0, 3, 1.0 );
    testUnaryOpV( tan, F64, M_PI_4, 0.0, 3, 1.0 );
    testUnaryOpV( tan, C32, M_PI_4 + 0.0i, 0.0 + 0.0i, 3, 1 );
    testUnaryOpV( dtan, S32, 45, 0, 3, 1 );
    testUnaryOpV( dtan, F32, 45.0, 0.0, 3, 1.0 );
    testUnaryOpV( dtan, F64, 45.0, 0.0, 3, 1.0 );
    testUnaryOpV( dtan, C32, 45.0 + 45.0i, 0.0 + 0.0i, 3, 1.0 );
    testUnaryOpV( asin, S32, 1, 0, 3, M_PI_2);
    testUnaryOpV( asin, F32, 1.0, 0.0, 3, M_PI_2  );
    testUnaryOpV( asin, F64, 1.0, 0.0, 3, M_PI_2);
    testUnaryOpV( asin, C32, 1.0 + 1.0i, 0.0 + 0.0i, 3, M_PI_2);
    testUnaryOpV( dasin, S32, 1.0, 0, 3, 90 );
    testUnaryOpV( dasin, F32, 1.0, 0.0, 3, 90.0 );
    testUnaryOpV( dasin, F64, 1.0, 0.0, 3, 90.0 );
    testUnaryOpV( dasin, C32, 1.0 + 1.0i, 0.0 + 0.0i, 3, 90.0 );
    testUnaryOpV( acos, S32, 0, 0, 3, M_PI_2);
    testUnaryOpV( acos, F32, 0.0, 0.0, 3, M_PI_2 );
    testUnaryOpV( acos, F64, 0.0, 0.0, 3, M_PI_2 );
    testUnaryOpV( acos, C32, 0.0 + 0.0i, 0.0 + 0.0i, 3, M_PI_2 );
    testUnaryOpV( dacos, S32, 0, 0, 3, 90 );
    testUnaryOpV( dacos, F32, 0.0, 0.0, 3, 90.0 );
    testUnaryOpV( dacos, F64, 0.0, 0.0, 3, 90.0 );
    testUnaryOpV( dacos, C32, 0.0 + 0.0i, 0.0 + 0.0i, 3, 90.0 );
    testUnaryOpV( atan, S32, 1, 0, 3, M_PI_4);
    testUnaryOpV( atan, F32, 1.0, 0.0, 3, M_PI_4 );
    testUnaryOpV( atan, F64, 1.0, 0.0, 3, M_PI_4);
    testUnaryOpV( atan, C32, 1.0 + 0.0i, 0.0 + 0.0i, 3, M_PI_4);
    testUnaryOpV( datan, S32, 1, 0, 3, 45 );
    testUnaryOpV( datan, F32, 1.0, 0.0, 3, 45.0 );
    testUnaryOpV( datan, F64, 1.0, 0.0, 3, 45.0 );
    testUnaryOpV( datan, C32, 1.0 + 0.0i, 0.0 + 0.0i, 3, 45.0 );

    return 0;
}

