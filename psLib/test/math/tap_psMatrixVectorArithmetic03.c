/** @file  tst_psMatrixVectorArithmetic03.c
 *
 *  @brief Test driver for psMatrixVector arithmetic functions
 *
 *  This test driver contains negative tests for psBinaryOp and psUanryOp:
 *     Check for NULL arguments
 *     Inconsistent element types
 *     Inconsistent element count
 *     Inconsistent dimensionality
 *     Division by zero
 *     Invalid operation
 *
 * XXX: Many of these tests are desinged to produce an error.  They are commented out.
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

#define CREATE_AND_SET_VECTOR(NAME,TYPE,VALUE,SIZE) \
psVector *NAME = (psVector*)psVectorAlloc(SIZE, PS_TYPE_##TYPE); \
for(psS32 i=0; i<SIZE; i++) { \
    NAME->data.TYPE[i] = VALUE; \
} \
NAME->n = SIZE;


#define CREATE_AND_SET_IMAGE(NAME,TYPE,VALUE,NROWS,NCOLS) \
psImage *NAME = (psImage*)psImageAlloc(NCOLS,NROWS,PS_TYPE_##TYPE); \
for(psS32 i=0; i<NAME->numRows; i++) { \
    for(psS32 j=0; j<NAME->numCols; j++) { \
        NAME->data.TYPE[i][j] = VALUE; \
    } \
}


psS32 main(psS32 argc, char* argv[])
{
    psLogSetFormat("HLNM");
    plan_tests(5);
    psMemId idGlobal = psMemGetId();

    CREATE_AND_SET_IMAGE(image1,F64,0,3,3);
    CREATE_AND_SET_IMAGE(image2,F64,0,3,3);
    CREATE_AND_SET_IMAGE(image3,F32,0,3,3);
    CREATE_AND_SET_IMAGE(image4,F64,0,2,2);
    CREATE_AND_SET_VECTOR(vector1,F64,0,2);
    CREATE_AND_SET_VECTOR(vector2,F64,0,3);

    // Check for NULL output argument
    {
        psMemId id = psMemGetId();
        psImage* image6 = (psImage*)psBinaryOp(NULL, image1, "+", image2);
        ok(image6 != NULL, "psBinaryOp() produced a non-NULL image given no output to recycle.");
        psFree(image6);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Check for NULL input argument #1

    if (1) {
        psMemId id = psMemGetId();
        psImage* image6 = (psImage*)psBinaryOp(image6, NULL, "+", image2);
        ok(image6 == NULL, "psBinaryOp() returned a NULL result given a NULL first operand.");
        psFree(image6);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Check for NULL input argument #2
    // Following should generate an error message
    if (0) {
        psMemId id = psMemGetId();
        psImage* image6 = (psImage*)psBinaryOp(image6, image1, "+", NULL);
        ok(image6 == NULL, "psBinaryOp() returned a NULL given a NULL second operand.");
        psFree(image6);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Check for NULL operand
    // Following should generate an error message
    if (0) {
        psMemId id = psMemGetId();
        psImage* image6 = (psImage*)psBinaryOp(image6, image1, NULL, image2);
        ok(image6 == NULL, "psBinaryOp returned a NULL given a NULL operator.");
        psFree(image6);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Check for null output
    {
        psMemId id = psMemGetId();
        psImage* image6 = (psImage*)psUnaryOp(NULL, image1, "sin");
        ok(image6 != NULL, "psUnaryOp produced an image given no output to recycle.");
        psFree(image6);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Check for NULL input arg
    // Following should generate an error message
    if (0) {
        psMemId id = psMemGetId();
        psImage* image6 = (psImage*)psUnaryOp(image6, NULL, "sin");
        ok(image6 == NULL, "psUnaryOp returned a NULL given a NULL operand.");
        psFree(image6);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Check for NULL operand
    // Following should generate an error message
    if (0) {
        psMemId id = psMemGetId();
        psImage* image6 = (psImage*)psUnaryOp(image6, image1, NULL);
        ok(image6 == NULL, "psUnaryOp returned a NULL operand.");
        psFree(image6);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Inconsistent element types
    // Following should generate an error message
    if (0) {
        psMemId id = psMemGetId();
        psImage* image6 = (psImage*)psBinaryOp(image6, image3, "+", image2);
        ok(image6 == NULL, "psBinaryOp returned a NULL given operands of different types");
        psFree(image6);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Check unary op to convert to correct type
    if (0) {
        psMemId id = psMemGetId();
        psImage* image6 = psImageCopy(image6,image2,PS_TYPE_F64);
        image6 = (psImage*)psUnaryOp(image6, image3, "sin");
        ok(!(image6 == NULL || image6->type.type != PS_TYPE_F32),
           "psUnaryOp converted the type of the output.");
        psFree(image6);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Inconsistent element count
    // Following should generate an error message
    if (0) {
        psMemId id = psMemGetId();
        psImage* image6 = (psImage*)psBinaryOp(image6, image4, "+", image2);
        ok(image6 == NULL, "psBinaryOp returned a NULL given operands of different sizes.");
        psFree(image6);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Inconsistent element in input and output
    // XXX: This fails, but should work.
    if (0) {
        psMemId id = psMemGetId();
        psImage* image6 = psImageCopy(image6,image2,PS_TYPE_F64);
        image6 = (psImage*)psUnaryOp(image6, image4, "sin");
        ok(!(image6 == NULL ||
             image6->numCols != image6->numCols ||
             image6->numRows != image6->numRows),"psUnaryOp resized the output.");
        psFree(image6);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Inconsistent size of input 1 and input 2
    // Following should generate an error message
    if (0) {
        psMemId id = psMemGetId();
        psImage* image6 = (psImage*)psBinaryOp(image6, vector1, "+", image2);
        ok(image6 == NULL, "psBinaryOp returned a NULL given operands of different sizes.");
        psFree(image6);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Inconsistent size of input 1 and input 2
    // Following should generate an error message
    if (0) {
        psMemId id = psMemGetId();
        vector1->type.dimen = PS_DIMEN_TRANSV;
        psImage* image6 = (psImage*)psBinaryOp(image6, vector1, "+", image2);
        ok(image6 == NULL, "psBinaryOp returned a NULL given operands of different sizes.");
        psFree(image6);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Inconsistent dimensionality
    // Following should generate an two error messages
    if (0) {
        psMemId id = psMemGetId();
        psImage* image6 = psImageCopy(image6,image2,PS_TYPE_F64);
        image6 = (psImage*)psUnaryOp(image6, vector2, "sin");
        ok(image6 == NULL, "psUnaryOp returned NULL given wrong type out parameter.");
        psFree(image6);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Invalid operation
    // Following should generate an error messgae
    if (0) {
        psMemId id = psMemGetId();
        psImage* image6 = (psImage*)psBinaryOp(image6, image1, "yarg", image2);
        ok(image6 == NULL, "psBinaryOp returned NULL with invalid operator");
        psFree(image6);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Following should generate an error message
    if (0) {
        psMemId id = psMemGetId();
        psImage* image6 = (psImage*)psUnaryOp(image6, image1, "yarg");
        ok(image6 == NULL, "psUnaryOp returned NULL with invalid operator");
        psFree(image6);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    CREATE_AND_SET_VECTOR(vector4,F64,0,3);
    CREATE_AND_SET_VECTOR(vector5,F64,0,3);

    // Input parameter with dimension of PS_DIMEN_OTHER
    // Following should generate an error message
    if (0) {
        psMemId id = psMemGetId();
        vector4->type.dimen = PS_DIMEN_OTHER;
        ok(psBinaryOp(NULL,vector4,"+",vector5) == NULL, "psBinaryOp should return null when input dimen PS_DIMEN_OTHER.");
        vector4->type.dimen = PS_DIMEN_VECTOR;
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Input parameter with dimension of PS_DIMEN_OTHER
    // Following should generate an error message
    if (0) {
        psMemId id = psMemGetId();
        vector4->type.dimen = PS_DIMEN_OTHER;
        ok(psUnaryOp(NULL,vector4,"sin") == NULL, "psUnaryOp should return null when input dimen PS_DIMEN_OTHER");
        vector4->type.dimen = PS_DIMEN_VECTOR;
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    psFree(vector4);
    psFree(vector5);
    psFree(image1);
    psFree(image2);
    psFree(image3);
    psFree(image4);
    psFree(vector1);
    psFree(vector2);
    ok(!psMemCheckLeaks (idGlobal, NULL, NULL, false), "no memory leaks");
}

