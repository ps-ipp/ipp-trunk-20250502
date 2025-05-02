/** @file  tst_psMatrix_04.c
 *
 *  @brief Test driver for psMatrix invert function
 *
 *  This test driver contains the following tests for psMatrix test point 4:
 *     Create input and output images
 *     Invert matrix and calculate determinant
 *     Calculate determinant only
 *     Free input and output images
 *     Attempt to use null input image argument
 *     Attempt to use null input float argument
 *
 * Sme tests should generate an error/warning, but we don't know how to test that.
 *
 *  @author  Ross Harman, MHPCC
 *
 *  @version $Revision: 1.1 $  $Name: not supported by cvs2svn $
 *  @date  $Date: 2006-12-20 20:02:29 $
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
double truthMatrix[3][3] = {{4.0000000, -4.333333, -2.333333},
                            {-1.000000,  1.666667,  0.666667},
                            {-1.000000,  0.666667,  0.666667}};
double truthValue = 3.0;

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

psS32 checkValue(psF64 value)
{
    if(fabs(value-truthValue) > TOLERANCE) {
        diag("Values don't agree %lf vs %lf\n", value, truthValue);
        return(true);
    } else {
        return(false);
    }
}

psS32 main(psS32 argc, char* argv[])
{
    psLogSetFormat("HLNM");
    plan_tests(14);

    // Invert matrix and calculate determinant: F64
    {
        psMemId id = psMemGetId();
        float det = 0.0f;
        float det2 = 0;
        psImage *tempImage = NULL;
        psImage *outImage = (psImage*)psImageAlloc(3, 3, PS_TYPE_F64);
        psImage *inImage = (psImage*)psImageAlloc(3, 3, PS_TYPE_F64);
        inImage->data.F64[0][0] =  2;
        inImage->data.F64[0][1] =  4;
        inImage->data.F64[0][2] =  3;
        inImage->data.F64[1][0] =  0;
        inImage->data.F64[1][1] =  1;
        inImage->data.F64[1][2] = -1;
        inImage->data.F64[2][0] =  3;
        inImage->data.F64[2][1] =  5;
        inImage->data.F64[2][2] =  7;

        tempImage = outImage;
        outImage = psMatrixInvert(outImage, inImage, &det);
        ok(outImage != NULL, "psMatrixInvert() produced a non-NULL matrix");
        skip_start(outImage == NULL, 4, "Skipping tests because the output matrix is NULL");
        ok(!checkMatrix(outImage), "psMatrixInvert() produced the correct output matrix");
        ok(!checkValue(det), "psMatrixInvert() produced the correct determinant");
        ok(outImage->type.dimen == PS_DIMEN_IMAGE, "psMatrixInvert() produced the correct ->dimen member");
        ok(outImage == tempImage, "psMatrixInvert() did not allocate a new output matrix");
        det = 0.0f;

        det2 = psMatrixDeterminant(inImage);
        ok(!checkValue(det2), "psMatrixDeterminant() produced the correct determinant");

        skip_end()
        psFree(outImage);
        psFree(inImage);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Invert matrix and calculate determinant: F32
    {
        psMemId id = psMemGetId();
        float det = 0.0f;
        float det2 = 0;
        psImage *tempImage32 = NULL;
        psImage *outImage32 = (psImage*)psImageAlloc(3, 3, PS_TYPE_F32);
        psImage *inImage32 = (psImage*)psImageAlloc(3, 3, PS_TYPE_F32);
        inImage32->data.F32[0][0] =  2;
        inImage32->data.F32[0][1] =  4;
        inImage32->data.F32[0][2] =  3;
        inImage32->data.F32[1][0] =  0;
        inImage32->data.F32[1][1] =  1;
        inImage32->data.F32[1][2] = -1;
        inImage32->data.F32[2][0] =  3;
        inImage32->data.F32[2][1] =  5;
        inImage32->data.F32[2][2] =  7;

        tempImage32 = outImage32;
        outImage32 = psMatrixInvert(outImage32, inImage32, &det);
        ok(outImage32 != NULL, "psMatrixInvert() produced a non-NULL matrix");
        skip_start(outImage32 == NULL, 4, "Skipping tests because the output matrix is NULL");
        ok(!checkMatrix(outImage32), "psMatrixInvert() produced the correct output matrix");
        ok(!checkValue(det), "psMatrixInvert() produced the correct determinant");
        ok(outImage32->type.dimen == PS_DIMEN_IMAGE, "psMatrixInvert() produced the correct ->dimen member");
        ok(outImage32 == tempImage32, "psMatrixInvert() did not allocate a new output matrix");

        det2 = psMatrixDeterminant(inImage32);
        ok(!checkValue(det2), "psMatrixDeterminant() produced the correct determinant");

        skip_end()
        psFree(outImage32);
        psFree(inImage32);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Attempt to use null input image argument
    // XXX: This should generate an error/warning, but we don't know how to test that.
    if (0) {
        psMemId id = psMemGetId();
        float det = 0.0f;
        psImage *badOutImage = (psImage*)psImageAlloc(3, 3, PS_TYPE_F64);
        psMatrixInvert(badOutImage, NULL, &det);
        psFree(badOutImage);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Attempt to use null input float argument
    // XXX: This should generate an error/warning, but we don't know how to test that.
    if (0) {
        psMemId id = psMemGetId();
        psImage *badInImage = (psImage*)psImageAlloc(3, 3, PS_TYPE_F64);
        psImage *badOutImage = (psImage*)psImageAlloc(3, 3, PS_TYPE_F64);
        psMatrixInvert(badOutImage, badInImage, NULL);
        psFree(badInImage);
        psFree(badOutImage);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
}
