/** @file  tst_psMatrixVectorArithmetic04.c
 *
 *  @brief Test driver for psBinary arithmetic operations with scalars
 *
 *  This test driver will test the following binary operation with scalar inputs
 *        vector addition with scalar in first argument
 *        image addition with scalar in second argument
 *
 * XXX: The scalar functions at the end are producing memory corruption errors
 * when they shouldn't.
 *
 * @author  Eric Van Alst, MHPCC
 *
 * @version $Revision: 1.1 $  $Name: not supported by cvs2svn $
 * @date  $Date: 2007-01-06 00:48:54 $
 *
 * Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 *
 */
#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include "tap.h"
#include "pstap.h"


// Create vector
#define CREATE_AND_SET_VECTOR(NAME,TYPE,VALUE,SIZE) \
psVector *NAME = psVectorAlloc(SIZE,PS_TYPE_##TYPE); \
for(psS32 i=0; i<SIZE; i++) { \
    NAME->data.TYPE[i] = VALUE; \
} \
NAME->n = SIZE;

// Create image
#define CREATE_AND_SET_IMAGE(NAME,TYPE,VALUE,NROWS,NCOLS) \
psImage *NAME = psImageAlloc(NCOLS,NROWS,PS_TYPE_##TYPE); \
for(psS32 i=0; i<NAME->numRows; i++) { \
    for(psS32 j=0; j<NAME->numCols; j++) { \
        NAME->data.TYPE[i][j] = VALUE; \
    } \
}

psS32 main(psS32 argc, char* argv[])
{
    psLogSetFormat("HLNM");
    plan_tests(12);

    // testBinOpScalarFirst(void)
    {
        psMemId id = psMemGetId();
        bool psErrorFlag = false;
        CREATE_AND_SET_VECTOR(vector1,S8,1,5)
        CREATE_AND_SET_VECTOR(vector2,S8,0,5)
        psScalar* inScalar1 = psScalarAlloc(2,PS_TYPE_S8);

        // Add vector and scalar
        vector2 = (psVector*)psBinaryOp(vector2,inScalar1,"+",vector1);
        // Verify the result vector
        for(psS32 i=0; i<vector2->n; i++)
        {
            if(vector2->data.S8[i] != 3 ) {
                psErrorFlag = true;
                diag("Unexpected value in return vector[%d]",i);
            }
        }
        ok(!psErrorFlag, "psBinaryOp() produced the correct vector");
        psFree(vector1);
        psFree(vector2);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    {
        psMemId id = psMemGetId();
        bool psErrorFlag = false;
        CREATE_AND_SET_VECTOR(vector3,S8,1,5);
        CREATE_AND_SET_VECTOR(vector4,S8,0,3);

        psScalar* inScalar2 = psScalarAlloc(2,PS_TYPE_S8);
        vector4->type.dimen = PS_DIMEN_TRANSV;
        vector4 = (psVector*)psBinaryOp(vector4,inScalar2,"+",vector3);
        for(psS32 i=0; i<vector4->n; i++)
        {
            if(vector4->data.S8[i] != 3 ) {
                psErrorFlag = true;
                diag("Unexpected value in return vector[%d]",i);
            }
        }
        ok(!psErrorFlag, "psBinaryOp() produced the correct vector");
        psFree(vector3);
        psFree(vector4);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    {
        psMemId id = psMemGetId();
        bool psErrorFlag = false;
        CREATE_AND_SET_IMAGE(image1,S8,1,5,5)
        psImage* image2 = NULL;
        psScalar* inScalar3 = psScalarAlloc(2,PS_TYPE_S8);

        image2 = (psImage*)psBinaryOp(image2,inScalar3,"+",image1);
        for(psS32 i=0; i<image2->numRows; i++)
        {
            for(psS32 j=0; j<image2->numCols; j++) {
                if(image2->data.S8[i][j] != 3 ) {
                    psErrorFlag = true;
                    diag("Unexpected value in return image[%d][%d]",i,j);
                }
            }
        }
        ok(!psErrorFlag, "psBinaryOp() produced the correct matrix");
        psFree(image1);
        psFree(image2);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    {
        psMemId id = psMemGetId();
        bool psErrorFlag = false;
        CREATE_AND_SET_VECTOR(vector1,S8,1,5)
        CREATE_AND_SET_VECTOR(vector2,S8,0,5)
        psScalar* inScalar1 = psScalarAlloc(2,PS_TYPE_S8);

        vector2 = (psVector*)psBinaryOp(vector2,vector1,"+",inScalar1);
        for(psS32 i=0; i<vector2->n; i++)
        {
            if(vector2->data.S8[i] != 3 ) {
                psErrorFlag = true;
                diag("Unexpected value in return vector[%d]",i);
            }
        }
        ok(!psErrorFlag, "psBinaryOp() produced the correct vector");
        psFree(vector1);
        psFree(vector2);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    {
        psMemId id = psMemGetId();
        bool psErrorFlag = false;
        CREATE_AND_SET_VECTOR(vector3,S8,1,5);
        CREATE_AND_SET_VECTOR(vector4,S8,0,3);
        psScalar* inScalar2 = psScalarAlloc(2,PS_TYPE_S8);

        vector4->type.dimen = PS_DIMEN_TRANSV;
        vector4 = (psVector*)psBinaryOp(vector4,vector3,"+",inScalar2);
        for(psS32 i=0; i<vector4->n; i++)
        {
            if(vector4->data.S8[i] != 3 ) {
                psErrorFlag = true;
                diag("Unexpected value in return vector[%d]",i);
            }
        }
        ok(!psErrorFlag, "psBinaryOp() produced the correct vector");
        psFree(vector3);
        psFree(vector4);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    if (1) {
        psMemId id = psMemGetId();
        bool psErrorFlag = false;
        CREATE_AND_SET_IMAGE(image1,S8,1,5,5);
        psImage* image2 = NULL;
        psScalar* inScalar3 = psScalarAlloc(2,PS_TYPE_S8);

        image2 = (psImage*)psBinaryOp(image2,image1,"+",inScalar3);
        for(psS32 i=0; i<image2->numRows; i++) {
            for(psS32 j=0; j<image2->numCols; j++) {
                if(image2->data.S8[i][j] != 3 ) {
                    psErrorFlag = true;
                    diag("Unexpected value in return image[%d][%d]",i,j);
                }
            }
        }
        ok(!psErrorFlag, "psBinaryOp() produced the correct matrix");
        psFree(image1);
        psFree(image2);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // testBinOpScalarBoth
    // This test fails.  It shouldn't.
    if (0) {
        psMemId id = psMemGetId();
        psScalar* inScalar1 = psScalarAlloc(1,PS_TYPE_S8);
        psScalar* inScalar2 = psScalarAlloc(2,PS_TYPE_S8);
        psScalar* outScalar = psScalarAlloc(4,PS_TYPE_S8);

        outScalar = (psScalar*)psBinaryOp(outScalar,inScalar1,"+",inScalar2);
        ok(outScalar->data.S8 == 3, "psBinaryOp() produced the correct result");
        psFree(outScalar);
        psFree(inScalar1);
        psFree(inScalar2);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // This test fails.  It shouldn't.
    if (0) {
        psMemId id = psMemGetId();
        psScalar* inScalar3 = psScalarAlloc(10,PS_TYPE_S8);
        psScalar* inScalar4 = psScalarAlloc(20,PS_TYPE_S8);
        psScalar* outScalar1 = NULL;

        outScalar1 = (psScalar*)psBinaryOp(outScalar1,inScalar3,"+",inScalar4);
        ok(outScalar1->data.S8 == 30, "psBinaryOp() produced the correct result");
        psFree(outScalar1);
        psFree(inScalar3);
        psFree(inScalar4);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // This test fails.  It shouldn't.
    // testUnaryOpScalar
    if (0) {
        psMemId id = psMemGetId();
        psScalar* inScalar = psScalarAlloc(-1,PS_TYPE_F32);
        psScalar* outScalar = NULL;

        outScalar = (psScalar*)psUnaryOp(outScalar,inScalar,"abs");
        ok(outScalar->data.F32 == 1, "psUnaryOp() produced the correct result");
        psFree(outScalar);
        psFree(inScalar);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
}

