/** @file  tst_psImageConvolve.c
 *
 *  This code will test the psImageSmooth() routine.
 *
 *  @author GLG, MHPCC
 *
 *  @version $Revision: 1.3 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2006-05-09 03:20:00 $
 *
 *  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 */

#include <math.h>
#include <float.h>
#include <string.h>
#include <stdlib.h>

#include "psTest.h"
#include "pslib_strict.h"
#include "psType.h"

static psS32 testImageSmooth();

testDescription tests[] = {
                              {testImageSmooth, 0, "psImageSmooth", 0, false},
                              {NULL}
                          };

psS32 main(psS32 argc, char* argv[])
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);

    return ! runTestSuite(stderr, "psImage", tests, argc, argv);
}

#define NUM_ROWS 20
#define NUM_COLS 20
#define SIGMA 1.0
#define NSIGMA 5.0
#define TS00_IM_NULL            0x00000001
#define TS00_IM_F32             0x00000002
#define TS00_IM_F64             0x00000004
#define TS00_IM_S32             0x00000008
#define VERBOSE 0

static psBool testImageSmoothGeneric(
    psU32 flags,
    psS32 numCols,
    psS32 numRows,
    psF64 sigma,
    psF64 Nsigma,
    psBool expectedRC)
{
    psBool testStatus = true;
    psImage *img = NULL;

    printPositiveTestHeader(stdout, "psImageConvolve.c", "Image Smoothing Routine");

    if (expectedRC == false) {
        printf("This test should generate FALSE.\n");
    } else {
        printf("This test should generate TRUE.\n");
    }

    if (flags & TS00_IM_NULL) {
        printf("        using a NULL image\n");
    }

    if (flags & TS00_IM_F32) {
        printf("        using a psF32 image\n");
        img = psImageAlloc(numCols, numRows, PS_TYPE_F32);
        // Set a checkboard pattern
        for (psS32 i = 0 ; i < numRows ; i++) {
            for (psS32 j = 0 ; j < numCols ; j++) {
                if ((i%2) != (j%2)) {
                    img->data.F32[i][j] = 1.0;
                } else {
                    img->data.F32[i][j] = 0.0;
                }
            }
        }
    }
    if (VERBOSE) {
        p_psImagePrint(1, img, "The Smoothed Image");
    }

    if (flags & TS00_IM_F64) {
        printf("        using a psF64 image\n");
        img = psImageAlloc(numCols, numRows, PS_TYPE_F64);
        // Set a checkboard pattern
        for (psS32 i = 0 ; i < numRows ; i++) {
            for (psS32 j = 0 ; j < numCols ; j++) {
                if ((i%2) != (j%2)) {
                    img->data.F64[i][j] = 1.0;
                } else {
                    img->data.F64[i][j] = 0.0;
                }
            }
        }
    }

    if (flags & TS00_IM_S32) {
        printf("        using a psS32 image\n");
        img = psImageAlloc(numCols, numRows, PS_TYPE_S32);
        // Set a checkboard pattern
        for (psS32 i = 0 ; i < numRows ; i++) {
            for (psS32 j = 0 ; j < numCols ; j++) {
                if ((i%2) != (j%2)) {
                    img->data.S32[i][j] = 1;
                } else {
                    img->data.S32[i][j] = 0;
                }
            }
        }
    }
    printf(" %d columns\n", numCols);
    printf(" %d rows\n", numRows);
    printf(" sigma is %.2f\n", sigma);
    printf(" Nsigma is %.2f\n", Nsigma);

    psBool rc = psImageSmooth(img, sigma, Nsigma);
    if (rc == false) {
        if (expectedRC == true) {
            printf("TEST ERROR: psImageSmooth returned FALSE\n");
            testStatus = false;
        }
    } else {
        if (expectedRC == false) {
            printf("TEST ERROR: psImageSmooth returned TRUE\n");
            testStatus = false;
        }
        if (VERBOSE) {
            p_psImagePrint(1, img, "The Smoothed Image");
        }

        if (flags & TS00_IM_F32) {
            for (psS32 i = 1 ; i < numRows-1 ; i++) {
                for (psS32 j = 1 ; j < numCols-1 ; j++) {
                    if ((fabs(img->data.F32[i][j] - 0.5) > 0.1)) {
                        printf("TEST ERROR: img[%d][%d] was %f, expected 0.5\n",
                               i, j, img->data.F32[i][j]);
                        testStatus = false;
                    }
                }
            }
        }

        if (flags & TS00_IM_F64) {
            for (psS32 i = 1 ; i < numRows-1 ; i++) {
                for (psS32 j = 1 ; j < numCols-1 ; j++) {
                    if ((fabs(img->data.F64[i][j] - 0.5) > 0.1)) {
                        printf("TEST ERROR: img[%d][%d] was %f, expected 0.5\n",
                               i, j, img->data.F64[i][j]);
                        testStatus = false;
                    }
                }
            }
        }
    }
    psFree(img);

    return(testStatus);
}

static psS32 testImageSmooth()
{
    int bad = 0x00;            // What's bad?

    bad |= (testImageSmoothGeneric(TS00_IM_F32, NUM_COLS, NUM_ROWS, SIGMA, NSIGMA, true)    ? 0x00 : 0x01);
    bad |= (testImageSmoothGeneric(TS00_IM_F32, 1, NUM_ROWS, SIGMA, NSIGMA, true)           ? 0x00 : 0x02);
    bad |= (testImageSmoothGeneric(TS00_IM_F32, NUM_COLS, 1, SIGMA, NSIGMA, true)           ? 0x00 : 0x04);
    bad |= (testImageSmoothGeneric(TS00_IM_F32, 1, 1, SIGMA, NSIGMA, true)                  ? 0x00 : 0x08);
    bad |= (testImageSmoothGeneric(TS00_IM_F64, NUM_COLS, NUM_ROWS, SIGMA, NSIGMA, true)    ? 0x00 : 0x10);
    bad |= (testImageSmoothGeneric(TS00_IM_S32, NUM_COLS, NUM_ROWS, SIGMA, NSIGMA, false)   ? 0x00 : 0x20);
    bad |= (testImageSmoothGeneric(TS00_IM_NULL, NUM_COLS, NUM_ROWS, SIGMA, NSIGMA, false)  ? 0x00 : 0x40);

    return bad;
}
