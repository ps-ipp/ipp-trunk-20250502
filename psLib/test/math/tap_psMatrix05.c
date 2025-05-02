/** @file  tst_psMatrix_05.c
*
*  @brief Test driver for psMatrix multiplication function
*
*  This test driver contains the following tests for psMatrix test point 5:
*     A)  Create input and output images
*     B)  Multiply images
*     C)  Free input and output images
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
double truthMatrix[3][3] = {{  0.0, 52.0},
                            {-14.0, 51.0}};

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

psS32 main(psS32 argc,
           char* argv[])
{
    psLogSetFormat("HLNM");
    plan_tests(6);

    // Create input and output images, then multiple: F64 version
    {
        psMemId id = psMemGetId();
        psImage * outImage = (psImage*) psImageAlloc(2, 2, PS_TYPE_F64);
        psImage *inImage1 = (psImage*) psImageAlloc(3, 2, PS_TYPE_F64);
        psImage *inImage2 = (psImage*) psImageAlloc(2, 3, PS_TYPE_F64);
        inImage1->data.F64[0][0] = 2;
        inImage1->data.F64[0][1] = 3;
        inImage1->data.F64[0][2] = 4;
        inImage1->data.F64[1][0] = -1;
        inImage1->data.F64[1][1] = 2;
        inImage1->data.F64[1][2] = 5;
        inImage2->data.F64[0][0] = 4;
        inImage2->data.F64[0][1] = 1;
        inImage2->data.F64[1][0] = 0;
        inImage2->data.F64[1][1] = 6;
        inImage2->data.F64[2][0] = -2;
        inImage2->data.F64[2][1] = 8;

        // Test B - Multiply images
        psMatrixMultiply(outImage, inImage1, inImage2);
        ok(outImage != NULL, "psMatrixMultiply() produced a non-NULL output matrix");
        ok(!checkMatrix(outImage), "psMatrixMultiply() produced the correct output matrix");

        // Test C - Free input and output images
        psFree(outImage);
        psFree(inImage1);
        psFree(inImage2);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Create input and output images, then multiple: F32 version
    {
        psMemId id = psMemGetId();
        psImage * outImage32 = NULL;
        psImage *inImage132 = NULL;
        psImage *inImage232 = NULL;
        outImage32 = (psImage*)psImageAlloc(2, 2, PS_TYPE_F32);
        inImage132 = (psImage*)psImageAlloc(3, 2, PS_TYPE_F32);
        inImage232 = (psImage*)psImageAlloc(2, 3, PS_TYPE_F32);
        inImage132->data.F32[0][0] = 2;
        inImage132->data.F32[0][1] = 3;
        inImage132->data.F32[0][2] = 4;
        inImage132->data.F32[1][0] = -1;
        inImage132->data.F32[1][1] = 2;
        inImage132->data.F32[1][2] = 5;
        inImage232->data.F32[0][0] = 4;
        inImage232->data.F32[0][1] = 1;
        inImage232->data.F32[1][0] = 0;
        inImage232->data.F32[1][1] = 6;
        inImage232->data.F32[2][0] = -2;
        inImage232->data.F32[2][1] = 8;

        // Test B - Multiply images
        psMatrixMultiply(outImage32, inImage132, inImage232);
        ok(outImage32 != NULL, "psMatrixMultiply() produced a non-NULL output matrix");
        ok(!checkMatrix(outImage32), "psMatrixMultiply() produced the correct output matrix");

        // Test C - Free input and output images
        psFree(outImage32);
        psFree(inImage132);
        psFree(inImage232);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
}
