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


psS32 main( psS32 argc,
            char* argv[] )
{
    psLogSetFormat("HLNM");
    psImage * outImage = NULL;
    psImage *inImage1 = NULL;
    psImage *inImage2 = NULL;
    psImage * outImage32 = NULL;
    psImage *inImage132 = NULL;
    psImage *inImage232 = NULL;

    double truthMatrix[3][3] = {{  0.0, 52.0},
                                {-14.0, 51.0}};

    // Test A - Create input and output images
    printPositiveTestHeader(stdout, "psMatrix", "Create input and output images");
    outImage = (psImage*) psImageAlloc(2, 2, PS_TYPE_F64);
    inImage1 = (psImage*) psImageAlloc(3, 2, PS_TYPE_F64);
    inImage2 = (psImage*) psImageAlloc(2, 3, PS_TYPE_F64);
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
    printFooter( stdout, "psMatrix", "Create input and output images", true );


    // Test B - Multiply images
    printPositiveTestHeader(stdout, "psMatrix", "Multiply images");
    psMatrixMultiply(outImage, inImage1, inImage2);
    CHECK_MATRIX(outImage);
    psMatrixMultiply(outImage32, inImage132, inImage232);
    CHECK_MATRIX(outImage32);
    printFooter(stdout, "psMatrix", "Multiply images", true);


    // Test C - Free input and output images
    printPositiveTestHeader( stdout, "psMatrix", "Free input and output images" );
    psFree(outImage);
    psFree(inImage1);
    psFree(inImage2);
    psFree(outImage32);
    psFree(inImage132);
    psFree(inImage232);
    psS32 nLeaks = psMemCheckLeaks( 0, NULL, stdout, false );
    if ( nLeaks != 0 ) {
        printf( "ERROR: Found %d memory leaks\n", nLeaks );
    }
    psS32 nBad = psMemCheckCorruption( 0 );
    if ( nBad ) {
        printf( "ERROR: Found %d bad memory blocks\n", nBad );
    }
    printFooter( stdout, "psMatrix" , "Free input and output images", true );

    return 0;
}
