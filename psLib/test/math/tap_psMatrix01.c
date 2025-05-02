/** @file  tst_psMatrix_01.c
*
*  @brief Test driver for psMatrix transpose function
*
*  This test driver contains the following tests:
*     Transpose input image into output image
*     Transpose input image into auto allocated NULL output image
*
*  @author  Ross Harman, MHPCC
*
*  @version $Revision: 1.2 $  $Name: not supported by cvs2svn $
*  @date  $Date: 2007-05-02 04:20:06 $
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
double truthMatrix[3][3] = {{1, 4, 7},
                            {2, 5, 8},
                            {3, 6, 9}};

bool check_matrix(psImage *img)
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

psS32 main( psS32 argc, char* argv[] )
{
    plan_tests(14);


    // Verify with NULL input params
    {
        psMemId id = psMemGetId();
        psImage *outImage = psMatrixTranspose(NULL, NULL);
        ok(outImage == NULL, "psMatrixTranspose() returned NULL with NULL input params");
        psFree(outImage);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Verify with incorrect input image type
    {
        psMemId id = psMemGetId();
        psImage *inImage = (psImage*)psImageAlloc(3, 3, PS_TYPE_S64);
        psImage *outImage = psMatrixTranspose(NULL, inImage);
        ok(outImage == NULL, "psMatrixTranspose() returned NULL with incorrect input image type");
        psFree(inImage);
        psFree(outImage);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    psImage *inImage = (psImage*)psImageAlloc(3, 3, PS_TYPE_F64);
    psImage *outImage = (psImage*)psImageAlloc(3, 3, PS_TYPE_F64);
    inImage->data.F64[0][0] = 1;
    inImage->data.F64[0][1] = 2;
    inImage->data.F64[0][2] = 3;
    inImage->data.F64[1][0] = 4;
    inImage->data.F64[1][1] = 5;
    inImage->data.F64[1][2] = 6;
    inImage->data.F64[2][0] = 7;
    inImage->data.F64[2][1] = 8;
    inImage->data.F64[2][2] = 9;
    psImage *inImageF32 = (psImage*)psImageAlloc(3, 3, PS_TYPE_F32);
    psImage *outImageF32 = (psImage*)psImageAlloc(3, 3, PS_TYPE_F32);
    inImageF32->data.F32[0][0] = 1;
    inImageF32->data.F32[0][1] = 2;
    inImageF32->data.F32[0][2] = 3;
    inImageF32->data.F32[1][0] = 4;
    inImageF32->data.F32[1][1] = 5;
    inImageF32->data.F32[1][2] = 6;
    inImageF32->data.F32[2][0] = 7;
    inImageF32->data.F32[2][1] = 8;
    inImageF32->data.F32[2][2] = 9;

    // Transpose input image into output image
    {
        psMemId id = psMemGetId();
        psImage *tempImage = outImage;
        outImage = psMatrixTranspose(outImage, inImage);
        ok(outImage->type.dimen == PS_DIMEN_IMAGE, "psMatrixTranspose(): outImage has correct number of dimensions");
        ok(outImage == tempImage, "psMatrixTranspose(): Return pointer equal to output argument pointer");
        ok(!check_matrix(outImage), "Output image data set correctly");

        tempImage = outImageF32;
        outImageF32 = psMatrixTranspose(outImageF32, inImageF32);
        ok(!check_matrix(outImageF32), "Output image data set correctly");
        ok(outImageF32->type.dimen == PS_DIMEN_IMAGE, "psMatrixTranspose(): outImage has correct number of dimensions");
        ok(outImageF32 == tempImage, "psMatrixTranspose(): Return pointer equal to output argument pointer");

        psFree(outImage);
        psFree(outImageF32);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Transpose input image into auto allocated NULL output image
    {
        psMemId id = psMemGetId();
        psImage *outImageNull = psMatrixTranspose(NULL, inImage);
        ok(!check_matrix(outImageNull), "Output image data set correctly");

        psImage *outImageNullF32 = psMatrixTranspose(NULL, inImageF32);
        check_matrix(outImageNullF32);
        ok(!check_matrix(outImageNullF32), "Output image data set correctly");

        psFree(outImageNull);
        psFree(outImageNullF32);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    psFree(inImage);
    psFree(inImageF32);
}
