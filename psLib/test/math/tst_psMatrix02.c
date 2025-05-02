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
 *  @version $Revision: 1.2 $  $Name: not supported by cvs2svn $
 *  @date  $Date: 2005-08-24 01:24:24 $
 *
 *  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 *
 */

#include "pslib_strict.h"
#include "psTest.h"

psS32 main(psS32 argc,
           char* argv[])
{
    psLogSetFormat("HLNM");
    psImage *nullImage = NULL;
    psImage *inImage = (psImage*)psImageAlloc(3, 3, PS_TYPE_F64);
    psImage *outImage = (psImage*)psImageAlloc(3, 3, PS_TYPE_F64);
    psImage *badImage1 = (psImage*)psImageAlloc(3, 3, PS_TYPE_C32);
    psImage *badImage2 = (psImage*)psImageAlloc(3, 2, PS_TYPE_F64);

    // Test A - Input pointer same as output pointer
    printPositiveTestHeader(stdout,"psMatrix", "Input pointer same as output pointer");
    psMemIncrRefCounter(inImage);
    if (psMatrixTranspose(inImage, inImage) != NULL) {
        psError(PS_ERR_UNKNOWN, true,
                "inImage = outImage didn't results in NULL return");
        return 1;
    }
    if (psMemGetRefCounter(inImage) != 1) {
        psError(PS_ERR_UNKNOWN, true,
                "the output image was not freed on an error.");
        return 2;
    }
    printFooter(stdout, "psMatrix", "Input pointer same as output pointer", true);

    // Test B - Null input psImage
    printPositiveTestHeader(stdout,"psMatrix", "Null input psImage");
    psMemIncrRefCounter(outImage);
    if (psMatrixTranspose(outImage, nullImage) != NULL) {
        psError(PS_ERR_UNKNOWN, true,
                "inImage = outImage didn't results in NULL return");
        return 3;
    }
    if (psMemGetRefCounter(outImage) != 1) {
        psError(PS_ERR_UNKNOWN, true,
                "the output image was not freed on an error.");
        return 4;
    }
    printFooter(stdout, "psMatrix", "Null input psImage", true);

    // Test C - Incorrect type for input pointer
    printPositiveTestHeader(stdout,"psMatrix", "Incorrect type for input pointer");
    psMemIncrRefCounter(outImage);
    if (psMatrixTranspose(outImage, badImage1) != NULL) {
        psError(PS_ERR_UNKNOWN, true,
                "inImage = outImage didn't results in NULL return");
        return 5;
    }
    if (psMemGetRefCounter(outImage) != 1) {
        psError(PS_ERR_UNKNOWN, true,
                "the output image was not freed on an error.");
        return 6;
    }
    printFooter(stdout, "psMatrix", "Incorrect type for input pointer", true);

    // Test D - Incorrect type for output pointer
    printPositiveTestHeader(stdout,"psMatrix", "Incorrect type for output pointer");
    badImage1 = psMatrixTranspose(badImage1, inImage);
    if (badImage1 == NULL) {
        psError(PS_ERR_UNKNOWN, true,
                "inImage = outImage didn't results in NULL return");
        return 7;
    }
    // check that the type was changed.
    if (badImage1->type.type != PS_TYPE_F64) {
        psError(PS_ERR_UNKNOWN, true,
                "the output image was not freed on an error.");
        return 8;
    }
    printFooter(stdout, "psMatrix", "Incorrect type for output pointer", true);

    // Test E - Matrix not square for output pointer
    printPositiveTestHeader(stdout,"psMatrix", "Matrix not square for output pointer");
    psMatrixTranspose(badImage2, inImage);
    printFooter(stdout, "psMatrix", "Matrix not square for output pointer", true);

    return 0;
}
