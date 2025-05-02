/** @file  tst_psMatrix_03.c
 *
 *  @brief Test driver for psMatrix LU functions
 *
 *  This test driver contains the following tests for psMatrix test point 3:
 *     Create input and output images and vectors
 *     Calculate LU matrix
 *     Determine solution to matrix equation
 *     Free input and output images and vectors
 *     Attempt to use null image input argument
 *     Attempt to use null input vector argument
 *     Attempt to use null LU image argument
 *
 * XXX: Some tests should generate an error or warning, but we don't know how to test that.
 *
 *  @author  Ross Harman, MHPCC
 *
 *  @version $Revision: 1.2 $  $Name: not supported by cvs2svn $
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

#define TOLERANCE 0.000001
double truthVector[3] = {4.000000, -2.000000, 3.000000};
double truthMatrix[3][3] = {{4.000000,  5.000000,  6.000000},
                            {0.750000, -2.750000, -6.500000},
                            {0.500000, -0.545455, -0.545455}};

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
    plan_tests(16);

    // This test calculates the LU matrix and then solved the linear equations
    // in F64 mode.
    {
        psMemId id = psMemGetId();
        psImage *luImage = (psImage*) psImageAlloc(3, 3, PS_TYPE_F64);
        psImage *tempImage = luImage;
        psImage *inImage = (psImage*) psImageAlloc(3, 3, PS_TYPE_F64);
        inImage->data.F64[0][0] =  2;
        inImage->data.F64[0][1] =  4;
        inImage->data.F64[0][2] =  6;
        inImage->data.F64[1][0] =  4;
        inImage->data.F64[1][1] =  5;
        inImage->data.F64[1][2] =  6;
        inImage->data.F64[2][0] =  3;
        inImage->data.F64[2][1] =  1;
        inImage->data.F64[2][2] = -2;
        psVector *perm = (psVector*) psVectorAlloc(3, PS_TYPE_F64);
        psVector *outVector = (psVector*) psVectorAlloc(3, PS_TYPE_F32);
        psVector *tempVector = outVector;
        psVector *inVector = (psVector*) psVectorAlloc(3, PS_TYPE_F64);

        luImage = psMatrixLUDecomposition(luImage, &perm, inImage);
        ok(luImage != NULL, "psMatrixLUDecomposition() produced a non-NULL LU matrix");
        skip_start(luImage == NULL, 6, "Skipping tests because LU matrix was NULL");
        ok(!checkMatrix(luImage), "psMatrixLUDecomposition() produced the correct LU matrix");
        ok(luImage->type.dimen == PS_DIMEN_IMAGE, "The LU matrix has the correct ->dimen member");
        ok(luImage == tempImage, "The LU matrix was not created from scratch");

        // Determine solution to matrix equation
        inVector->data.F64[0] = 18.0;
        inVector->data.F64[1] = 24.0;
        inVector->data.F64[2] =  4.0;
        inVector->n = 3;

        outVector = psMatrixLUSolution(outVector, luImage, inVector, perm);
        ok(!checkVector(outVector), "psMatrixLUSolution() correctly solved the equations");
        ok(outVector->type.dimen == PS_DIMEN_VECTOR, "The output vector hasthe correct ->dimen member");
        ok(outVector == tempVector, "The output vector was not created from scratch");

        skip_end();
        psFree(inImage);
        psFree(luImage);
        psFree(perm);
        psFree(outVector);
        psFree(inVector);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // This test calculates the LU matrix and then solved the linear equations
    // in F32 mode.
    {
        psMemId id = psMemGetId();
        psImage *luImage32 = (psImage*)psImageAlloc(3, 3, PS_TYPE_F32);
        psImage *tempImage32 = luImage32;
        psImage *inImage32 = (psImage*)psImageAlloc(3, 3, PS_TYPE_F32);
        inImage32->data.F32[0][0] =  2;
        inImage32->data.F32[0][1] =  4;
        inImage32->data.F32[0][2] =  6;
        inImage32->data.F32[1][0] =  4;
        inImage32->data.F32[1][1] =  5;
        inImage32->data.F32[1][2] =  6;
        inImage32->data.F32[2][0] =  3;
        inImage32->data.F32[2][1] =  1;
        inImage32->data.F32[2][2] = -2;
        psVector *perm32 = NULL;
        psVector *outVector32 = (psVector*)psVectorAlloc(3, PS_TYPE_F32);
        psVector *tempVector32 = outVector32;
        psVector *inVector32 = (psVector*)psVectorAlloc(3, PS_TYPE_F32);
        inVector32->data.F32[0] = 18.0;
        inVector32->data.F32[1] = 24.0;
        inVector32->data.F32[2] =  4.0;
        inVector32->n = 3;

        luImage32 = psMatrixLUDecomposition(luImage32, &perm32, inImage32);
        ok(luImage32 != NULL, "psMatrixLUDecomposition() produced a non-NULL LU matrix");
        skip_start(luImage32 == NULL, 6, "Skipping tests because LU matrix was NULL");
        ok(!checkMatrix(luImage32), "psMatrixLUDecomposition() produced the correct LU matrix");
        ok(luImage32->type.dimen == PS_DIMEN_IMAGE, "The LU matrix has the correct ->dimen member");
        ok(luImage32 == tempImage32, "The LU matrix was not created from scratch");

        // Determine solution to matrix equation

        outVector32 = psMatrixLUSolution(outVector32, luImage32, inVector32, perm32);
        ok(!checkVector(outVector32), "psMatrixLUSolution() correctly solved the equations");
        ok(outVector32->type.dimen == PS_DIMEN_VECTOR, "The output vector hasthe correct ->dimen member");
        ok(outVector32 == tempVector32, "The output vector was not created from scratch");

        skip_end();
        psFree(inImage32);
        psFree(luImage32);
        psFree(perm32);
        psFree(outVector32);
        psFree(inVector32);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Attempt to use null image input argument
    // XXX: This test should generate an error or warning
    // XXX: This seg-faults
    { 
	psMemId id = psMemGetId();
	psImage *imageTest = (psImage*)psImageAlloc(3, 3, PS_TYPE_F64);
	psMatrixLUDecomposition(imageTest, NULL, NULL);
	psFree(imageTest);
	ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Attempt to use null input vector argument
    // XXX: This test should generate an error or warning
    // XXX: This seg-faulta
    { 
	psMemId id = psMemGetId();
	psVector *vectorBad = NULL;
	psVector *vectorBadOut = (psVector*)psVectorAlloc(3, PS_TYPE_F64);
	psVector *permBad = (psVector*)psVectorAlloc(3, PS_TYPE_F64);
	psImage *imageTest = (psImage*)psImageAlloc(3, 3, PS_TYPE_F64);
	psMatrixLUSolution(vectorBadOut, imageTest, vectorBad, permBad);
	psFree(vectorBadOut);
	psFree(permBad);
	psFree(imageTest);
	ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Attempt to use null LU image argument
    // XXX: This test should generate an error or warning, but we don't know how to test that.
    // XXX: This seg-faulta
    {
	psMemId id = psMemGetId();
	psVector *vectorBadOut = (psVector*)psVectorAlloc(3, PS_TYPE_F64);
	psVector *vectorBad = (psVector*)psVectorAlloc(3, PS_TYPE_F64);
	psVector *permBad = (psVector*)psVectorAlloc(3, PS_TYPE_F64);
	psMatrixLUSolution(vectorBadOut, NULL, vectorBad, permBad);
	psFree(vectorBadOut);
	psFree(vectorBad);
	psFree(permBad);
	ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
}
