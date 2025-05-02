/** @file  tst_psMatrix_01.c
*
*  @brief Test driver for psMatrix transpose function
*
*  This test driver contains the following tests for psMatrix test point 1:
*     A)  Create input and output images
*     B)  Transpose input image into output image
*     C)  Transpose input image into auto allocated NULL output image
*     D)  Free images and check for leaks
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

#define TOLERANCE 0.000001

#define CHECK_MATRIX(IMAGE)                                                                                  \
for(psU32 i=0; i<IMAGE->numRows; i++) {                                                                  \
    for(psU32 j=0; j<IMAGE->numCols; j++) {                                                              \
        if(IMAGE->type.type == PS_TYPE_F64) {                                                            \
            if(fabs(IMAGE->data.F64[i][j]-truthMatrix[i][j]) > TOLERANCE) {                              \
                printf("Matrix values at element %d, %d don't agree %lf vs %lf\n", i, j,                 \
                       IMAGE->data.F64[i][j], truthMatrix[i][j]);                                        \
            }                                                                                            \
        } else if(IMAGE->type.type == PS_TYPE_F32){                                                      \
            if(fabs(IMAGE->data.F32[i][j]-truthMatrix[i][j]) > TOLERANCE) {                              \
                printf("Matrix values at element %d, %d don't agree %f vs %lf\n", i, j,                  \
                       IMAGE->data.F32[i][j], truthMatrix[i][j]);                                        \
            }                                                                                            \
        }                                                                                                \
    }                                                                                                    \
}


psS32 main( psS32 argc, char* argv[] )
{
    psLogSetFormat("HLNM");
    psImage * tempImage = NULL;

    double truthMatrix[3][3] = {{1, 4, 7},
                                {2, 5, 8},
                                {3, 6, 9}};

    // Test A - Create input and output images
    printPositiveTestHeader(stdout, "psMatrix", "Create input and output images");
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
    printFooter(stdout, "psMatrix", "Create input and output images", true);


    // Test B - Transpose input image into output image
    printPositiveTestHeader(stdout, "psMatrix", "Transpose input image into output image");
    tempImage = outImage;
    outImage = psMatrixTranspose(outImage, inImage);
    CHECK_MATRIX(outImage);
    if (outImage->type.dimen != PS_DIMEN_IMAGE) {
        printf( "Error: Resulting image is not PS_DIMEN_IMAGE\n");
    } else if (outImage != tempImage) {
        printf("Error: Return pointer not equal to output argument pointer\n");
    }

    tempImage = outImageF32;
    outImageF32 = psMatrixTranspose(outImageF32, inImageF32);
    CHECK_MATRIX(outImageF32);
    if (outImageF32->type.dimen != PS_DIMEN_IMAGE) {
        printf("Error: Resulting image is not PS_DIMEN_IMAGE\n");
    } else if (outImageF32 != tempImage) {
        printf("Error: Return pointer not equal to output argument pointer\n");
    }
    printFooter(stdout, "psMatrix", "Transpose input image into output image", true);


    // Test C - Transpose input image into auto allocated NULL output image
    printPositiveTestHeader(stdout, "psMatrix", "Transpose input image into auto allocated NULL output image");
    psImage *outImageNull = NULL;
    outImageNull = psMatrixTranspose(outImageNull, inImage);
    CHECK_MATRIX(outImageNull);

    psImage *outImageNullF32 = NULL;
    outImageNullF32 = psMatrixTranspose(outImageNullF32, inImageF32);
    CHECK_MATRIX(outImageNullF32);
    printFooter(stdout, "psMatrix", "Transpose input image into auto allocated NULL output image", true);


    // Test D - Free images and check for leaks
    printPositiveTestHeader(stdout, "psMatrix", "Free images and check for leaks");
    psFree(inImage);
    psFree(outImage);
    psFree(outImageNull);
    psFree(inImageF32);
    psFree(outImageF32);
    psFree(outImageNullF32);
    psS32 nLeaks = psMemCheckLeaks(0, NULL, stdout, false);
    if (nLeaks != 0) {
        printf("ERROR: Found %d memory leaks\n", nLeaks);
    }
    psS32 nBad = psMemCheckCorruption(0);
    if (nBad) {
        printf("ERROR: Found %d bad memory blocks\n", nBad);
    }
    printFooter(stdout, "psMatrix" , "Free images and check for leaks", true);

    return 0;
}
