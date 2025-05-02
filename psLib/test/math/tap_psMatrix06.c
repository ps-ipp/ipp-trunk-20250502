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
double truthMatrix[4][4] = {{0.792608,  0.582076, -0.179186, -0.029193},
                            {0.451923, -0.370502,  0.741918,  0.328712},
                            {0.322416, -0.509579, -0.100228, -0.791411},
                            {0.252161, -0.514048, -0.638283,  0.514553}};


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


psS32 main(psS32 argc, char* argv[])
{
    psLogSetFormat("HLNM");
    plan_tests(6);

    // Create input and output images, then produce Eigen vectors: F64 version
    {
        psMemId id = psMemGetId();
        psImage * outImage = (psImage*) psImageAlloc(4, 4, PS_TYPE_F64);
        psImage *inImage = (psImage*)  psImageAlloc(4, 4, PS_TYPE_F64);
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

        psMatrixEigenvectors(outImage, inImage);
        ok(outImage != NULL, "psMatrixEigenvectors() produced a NULL output Matrix");
        skip_start(outImage == NULL, 1, "Skipping tests because output matrix was NULL");
        ok(!checkMatrix(outImage), "psMatrixEigenvectors() produced the correct Matrix");
        psFree(outImage);
        psFree(inImage);

        skip_end();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Create input and output images, then produce Eigen vectors: F32 version
    {
        psMemId id = psMemGetId();
        psImage * outImage32 = (psImage*) psImageAlloc(4, 4, PS_TYPE_F32);
        psImage *inImage32 = (psImage*)  psImageAlloc(4, 4, PS_TYPE_F32);
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

        psMatrixEigenvectors(outImage32, inImage32);
        ok(outImage32 != NULL, "psMatrixEigenvectors() produced a NULL output Matrix");
        skip_start(outImage32 == NULL, 1, "Skipping tests because output matrix was NULL");
        ok(!checkMatrix(outImage32), "psMatrixEigenvectors() produced the correct Matrix");
        psFree(outImage32);
        psFree(inImage32);

        skip_end();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
}
