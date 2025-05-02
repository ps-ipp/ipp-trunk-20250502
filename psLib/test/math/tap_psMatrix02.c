/** @file  tst_psMatrix_02.c
 *
 *  @brief Test driver for negative tests for psMatrix transpose function
 *
 *  This test driver contains the following tests for psMatrix test point 2:
 *     A)  Input pointer same as output pointer
 *     B)  Null input psImage
 *     C)  Incorrect type for input pointer
 *     D)  Incorrect type for output pointer
 *     E)  Matrix not square for output pointer
 *
 *  @author  Ross Harman, MHPCC
 *
 *  @version $Revision: 1.4 $  $Name: not supported by cvs2svn $
 *  @date  $Date: 2008-05-07 23:12:13 $
 *
 *  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 *
 */
#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include "tap.h"
#include "pstap.h"

int main(psS32 argc,
         char* argv[])
{
    psLogSetFormat("HLNM");
    plan_tests(23);

    // Input pointer same as output pointer
    // XXX: This results in a seg fault.  It's not clear that passing this test is
    // a requirement.  However, we should probably fix the case where the input
    // image equals the output image.
     {
        psMemId id = psMemGetId();
        psImage *inImage = (psImage*)psImageAlloc(3, 3, PS_TYPE_F64);
        ok(psMatrixTranspose(inImage, inImage) == NULL, "psMatrixTranspose(): inImage == outImage results in NULL");
        psFree(inImage);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Null input psImage
    // Merge with tap_psMatrix01.c, get rid of this test (redundant)
    {
        psMemId id = psMemGetId();
        psImage *nullImage = NULL;
        psImage *outImage = (psImage*)psImageAlloc(3, 3, PS_TYPE_F64);
        ok(psMatrixTranspose(outImage, nullImage) == NULL, "psMatrixTranspose(): inImage = NULL results in NULL return");
        psFree(outImage);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Incorrect type for input pointer
    {
        psMemId id = psMemGetId();
        psImage *inImage = (psImage*)psImageAlloc(3, 3, PS_TYPE_U8);
        psImage *outImage = (psImage*)psImageAlloc(3, 3, PS_TYPE_F64);
        ok(psMatrixTranspose(outImage, inImage) == NULL, "psMatrixTranspose(): inImage wrong type (U8) results in NULL return");
        psFree(outImage);
        psFree(inImage);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Incorrect type for input pointer
    {
        psMemId id = psMemGetId();
        psImage *inImage = (psImage*)psImageAlloc(3, 3, PS_TYPE_S32);
        psImage *outImage = (psImage*)psImageAlloc(3, 3, PS_TYPE_F64);
        ok(psMatrixTranspose(outImage, inImage) == NULL, "psMatrixTranspose(): inImage wrong type (S32) results in NULL return");
        psFree(outImage);
        psFree(inImage);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Incorrect type for output pointer
    {
        psMemId id = psMemGetId();
        psImage *inImage = (psImage*)psImageAlloc(3, 3, PS_TYPE_F64);
        psImage *outImage = (psImage*)psImageAlloc(3, 3, PS_TYPE_U8);
        outImage = psMatrixTranspose(outImage, inImage);
        ok(outImage != NULL, "psMatrixTranspose() results in non-NULL return");

        // check that the type was changed.
        ok(outImage->type.type == PS_TYPE_F64, "the output type was changed to F64");
        psFree(inImage);
        psFree(outImage);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // output target matrix not square (Nx > Ny) for output pointer
    {
        psMemId id = psMemGetId();
        psImage *inImage = (psImage*)psImageAlloc(3, 3, PS_TYPE_F64);
        psImage *outImage = (psImage*)psImageAlloc(3, 2, PS_TYPE_F64);
        ok(psMatrixTranspose(outImage, inImage) != NULL, "psMatrixTranspose(): non-square matrix results in non-NULL");
        ok(outImage->numCols == 3, "psMatrixTranspose(): output matrix dimensions match input");
        ok(outImage->numRows == 3, "psMatrixTranspose(): output matrix dimensions match input");
        psFree(inImage);
        psFree(outImage);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // input matrix not square (Nx < Ny) 
    {
        psMemId id = psMemGetId();
        psImage *inImage = (psImage*)psImageAlloc(2, 3, PS_TYPE_F64);
        psImage *outImage = (psImage*)psImageAlloc(3, 3, PS_TYPE_F64);
        ok(psMatrixTranspose(outImage, inImage) != NULL, "psMatrixTranspose(): non-square matrix results in non-NULL");
        ok(outImage->numCols == 3, "psMatrixTranspose(): output matrix dimensions match input");
        ok(outImage->numRows == 2, "psMatrixTranspose(): output matrix dimensions match input");
        psFree(inImage);
        psFree(outImage);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // input matrix not square (Nx > Ny) 
    {
        psMemId id = psMemGetId();
        psImage *inImage = (psImage*)psImageAlloc(3, 2, PS_TYPE_F64);
        psImage *outImage = (psImage*)psImageAlloc(3, 3, PS_TYPE_F64);
        ok(psMatrixTranspose(outImage, inImage) != NULL, "psMatrixTranspose(): non-square matrix results in non-NULL");
        ok(outImage->numCols == 2, "psMatrixTranspose(): output matrix dimensions match input");
        ok(outImage->numRows == 3, "psMatrixTranspose(): output matrix dimensions match input");
        psFree(inImage);
        psFree(outImage);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
}
