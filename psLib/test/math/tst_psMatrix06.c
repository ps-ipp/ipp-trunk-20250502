/** @file  tst_psMatrix_06.c
*
*  @brief Test driver for psMatrix Eigenvectors function
*
*  This test driver contains the following tests for psMatrix test point 6:
*     A)  Create input and output images
*     B)  Calculate Eigenvectors
*     C)  Free input and output images
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


psS32 main(psS32 argc, char* argv[])
{
    psLogSetFormat("HLNM");
    psImage * outImage = NULL;
    psImage *inImage = NULL;
    psImage * outImage32 = NULL;
    psImage *inImage32 = NULL;

    double truthMatrix[4][4] = {{0.792608,  0.582076, -0.179186, -0.029193},
                                {0.451923, -0.370502,  0.741918,  0.328712},
                                {0.322416, -0.509579, -0.100228, -0.791411},
                                {0.252161, -0.514048, -0.638283,  0.514553}};


    // Test A - Create input and output images
    printPositiveTestHeader( stdout, "psMatrix", "Create input and output images" );
    outImage = (psImage*) psImageAlloc(4, 4, PS_TYPE_F64);
    inImage = (psImage*)  psImageAlloc(4, 4, PS_TYPE_F64);

    inImage->data.F64[0][0] = 1./1.;
    inImage->data.F64[0][1] = 1./2.;
    inImage->data.F64[0][2] = 1./3.;
    inImage->data.F64[0][3] = 1./4.;
    inImage->data.F64[1][0] = 1./2.;
    inImage->data.F64[1][1] = 1./3.;
    inImage->data.F64[1][2] = 1./4.;
    inImage->data.F64[1][3] = 1./5.;
    inImage->data.F64[2][0] = 1./3.;
    inImage->data.F64[2][1] = 1./4.;
    inImage->data.F64[2][2] = 1./5.;
    inImage->data.F64[2][3] = 1./6.;
    inImage->data.F64[3][0] = 1./4.;
    inImage->data.F64[3][1] = 1./5.;
    inImage->data.F64[3][2] = 1./6.;
    inImage->data.F64[3][3] = 1./7.;

    outImage32 = (psImage*) psImageAlloc(4, 4, PS_TYPE_F32);
    inImage32 = (psImage*)  psImageAlloc(4, 4, PS_TYPE_F32);
    inImage32->data.F32[0][0] = 1./1.;
    inImage32->data.F32[0][1] = 1./2.;
    inImage32->data.F32[0][2] = 1./3.;
    inImage32->data.F32[0][3] = 1./4.;
    inImage32->data.F32[1][0] = 1./2.;
    inImage32->data.F32[1][1] = 1./3.;
    inImage32->data.F32[1][2] = 1./4.;
    inImage32->data.F32[1][3] = 1./5.;
    inImage32->data.F32[2][0] = 1./3.;
    inImage32->data.F32[2][1] = 1./4.;
    inImage32->data.F32[2][2] = 1./5.;
    inImage32->data.F32[2][3] = 1./6.;
    inImage32->data.F32[3][0] = 1./4.;
    inImage32->data.F32[3][1] = 1./5.;
    inImage32->data.F32[3][2] = 1./6.;
    inImage32->data.F32[3][3] = 1./7.;
    printFooter(stdout, "psMatrix", "Create input and output images", true);


    // Test B - Calculate Eigenvectors
    printPositiveTestHeader(stdout, "psMatrix", "Calculate Eigenvectors");
    psMatrixEigenvectors(outImage, inImage);
    CHECK_MATRIX(outImage);
    psMatrixEigenvectors(outImage32, inImage32);
    CHECK_MATRIX(outImage32);
    printFooter(stdout, "psMatrix", "Calculate Eigenvectors", true);


    // Test C - Free input and output images
    printPositiveTestHeader(stdout, "psMatrix", "Free input and output images");
    psFree(outImage);
    psFree(inImage);
    psFree(outImage32);
    psFree(inImage32);
    psS32 nLeaks = psMemCheckLeaks(0, NULL, stdout, false);
    if (nLeaks != 0) {
        printf("ERROR: Found %d memory leaks\n", nLeaks);
    }
    psS32 nBad = psMemCheckCorruption(0);
    if (nBad) {
        printf("ERROR: Found %d bad memory blocks\n", nBad);
    }
    printFooter(stdout, "psMatrix" , "Free input and output images", true);

    return 0;
}
