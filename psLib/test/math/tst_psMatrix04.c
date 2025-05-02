/** @file  tst_psMatrix_04.c
 *
 *  @brief Test driver for psMatrix invert function
 *
 *  This test driver contains the following tests for psMatrix test point 4:
 *     A)  Create input and output images
 *     B)  Invert matrix and calculate determinant
 *     C)  Calculate determinant only
 *     D)  Free input and output images
 *     E)  Attempt to use null input image argument
 *     F)  Attempt to use null input float argument
 *
 *  @author  Ross Harman, MHPCC
 *
 *  @version $Revision: 1.3 $  $Name: not supported by cvs2svn $
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

#define CHECK_VALUE(VALUE)                                                                                   \
if(fabs(VALUE-truthValue) > TOLERANCE) {                                                                     \
    printf("Values don't agree %lf vs %lf\n", VALUE, truthValue);                                            \
}

psS32 main(psS32 argc, char* argv[])
{
    psLogSetFormat("HLNM");
    float det = 0.0f;
    float det2 = 0;
    psImage *outImage = NULL;
    psImage *inImage = NULL;
    psImage *tempImage = NULL;
    psImage *outImage32 = NULL;
    psImage *inImage32 = NULL;
    psImage *tempImage32 = NULL;

    double truthMatrix[3][3] = {{4.0000000, -4.333333, -2.333333},
                                {-1.000000,  1.666667,  0.666667},
                                {-1.000000,  0.666667,  0.666667}};
    double truthValue = 3.0;


    // Test A - Create input and output images
    printPositiveTestHeader(stdout, "psMatrix", "Create input and output images");
    outImage = (psImage*)psImageAlloc(3, 3, PS_TYPE_F64);
    inImage = (psImage*)psImageAlloc(3, 3, PS_TYPE_F64);
    inImage->data.F64[0][0] =  2;
    inImage->data.F64[0][1] =  4;
    inImage->data.F64[0][2] =  3;
    inImage->data.F64[1][0] =  0;
    inImage->data.F64[1][1] =  1;
    inImage->data.F64[1][2] = -1;
    inImage->data.F64[2][0] =  3;
    inImage->data.F64[2][1] =  5;
    inImage->data.F64[2][2] =  7;

    outImage32 = (psImage*)psImageAlloc(3, 3, PS_TYPE_F32);
    inImage32 = (psImage*)psImageAlloc(3, 3, PS_TYPE_F32);
    inImage32->data.F32[0][0] =  2;
    inImage32->data.F32[0][1] =  4;
    inImage32->data.F32[0][2] =  3;
    inImage32->data.F32[1][0] =  0;
    inImage32->data.F32[1][1] =  1;
    inImage32->data.F32[1][2] = -1;
    inImage32->data.F32[2][0] =  3;
    inImage32->data.F32[2][1] =  5;
    inImage32->data.F32[2][2] =  7;
    printFooter(stdout, "psMatrix", "Create input and output images", true);


    // Test B - Invert matrix and calculate determinant
    printPositiveTestHeader(stdout, "psMatrix", "Invert matrix and calculate determinant");
    tempImage = outImage;
    outImage = psMatrixInvert(outImage, inImage, &det);
    CHECK_MATRIX(outImage);
    CHECK_VALUE(det);
    if(outImage->type.dimen != PS_DIMEN_IMAGE) {
        printf("Error: Resulting image is not PS_DIMEN_IMAGE\n");
    } else if(outImage != tempImage) {
        printf("Error: Return pointer not equal to output argument pointer\n");
    }
    det = 0.0f;
    tempImage32 = outImage32;
    outImage32 = psMatrixInvert(outImage32, inImage32, &det);
    CHECK_MATRIX(outImage32);
    CHECK_VALUE(det);
    if(outImage32->type.dimen != PS_DIMEN_IMAGE) {
        printf("Error: Resulting image is not PS_DIMEN_IMAGE\n");
    } else if(outImage32 != tempImage32) {
        printf("Error: Return pointer not equal to output argument pointer\n");
    }
    printFooter(stdout, "psMatrix", "Invert matrix and calculate determinant", true);


    // Test C - Calculate determinant only
    printPositiveTestHeader(stdout, "psMatrix", "Calculate determinant only");
    det2 = psMatrixDeterminant(inImage);
    CHECK_VALUE(det2);
    det2 = psMatrixDeterminant(inImage32);
    CHECK_VALUE(det2);
    printFooter(stdout, "psMatrix", "Calculate determinant only", true);


    // Test D - Free input and output images
    printPositiveTestHeader(stdout, "psMatrix", "Free input and output images");
    psFree(outImage);
    psFree(inImage);
    psFree(outImage32);
    psFree(inImage32);
    if(psMemCheckLeaks(0, NULL, stdout, false) != 0 ) {
        psError(PS_ERR_UNKNOWN,true,"Memory leaks detected.");
        return 10;
    }
    psS32 nBad = psMemCheckCorruption(0);
    if(nBad) {
        printf("ERROR: Found %d bad memory blocks\n", nBad);
    }
    printFooter(stdout, "psMatrix" ,"Free input and output images", true);


    // Test E - Attempt to use null input image argument
    printNegativeTestHeader(stdout,"psMatrix", "Attempt to use null input image argument",
                            "Invalid operation: inImage or its data is NULL.", 0);
    psImage *badOutImage = (psImage*)psImageAlloc(3, 3, PS_TYPE_F64);
    psMatrixInvert(badOutImage, NULL, &det);
    printFooter(stdout, "psMatrix", "Attempt to use null input image argument", true);


    // Test F - Attempt to use null input float argument
    printNegativeTestHeader(stdout,"psMatrix", "Attempt to use null input float argument",
                            "Invalid operation: determinant argument is NULL.", 0);
    psImage *badInImage = (psImage*)psImageAlloc(3, 3, PS_TYPE_F64);
    psMatrixInvert(badOutImage, badInImage, NULL);
    printFooter(stdout, "psMatrix", "Attempt to use null input float argument", true);

    return 0;
}
