/** @file  tst_psImageConvolve.c
 *
 *  This code will test the psImageSmooth() routine.
 *
 *  @author GLG, MHPCC
 *
 *  @version $Revision: 1.3 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2007-02-27 23:56:12 $
 *
 *  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 */
#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include "tap.h"
#include "pstap.h"

#define NUM_ROWS 20
#define NUM_COLS 20
#define SIGMA 1.0
#define NSIGMA 5.0
#define TS00_IM_NULL            0x00000001
#define TS00_IM_F32             0x00000002
#define TS00_IM_F64             0x00000004
#define TS00_IM_S32             0x00000008
#define VERBOSE 1

static psBool testImageSmoothGeneric(
    psU32 flags,
    psS32 numCols,
    psS32 numRows,
    psF64 sigma,
    psF64 Nsigma,
    psBool expectedRC)
{
    psMemId id = psMemGetId();
    psBool testStatus = true;
    psImage *img = NULL;

    if (VERBOSE) {
        if (expectedRC == false) {
            printf("This test should generate FALSE.\n");
        } else {
            printf("This test should generate TRUE.\n");
        }
    }

    if (flags & TS00_IM_NULL) {
        if (VERBOSE) printf("        using a NULL image\n");
    }

    if (flags & TS00_IM_F32) {
        if (VERBOSE) printf("        using a psF32 image\n");
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
    if (flags & TS00_IM_F64) {
        if (VERBOSE)
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
        if (VERBOSE)
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

    if (VERBOSE) {
        if (img) p_psImagePrint(1, img, "The Raw Image");
        printf(" %d columns\n", numCols);
        printf(" %d rows\n", numRows);
        printf(" sigma is %.2f\n", sigma);
        printf(" Nsigma is %.2f\n", Nsigma);
    }

    psImageSmooth_PreAlloc_Data *smdata = psImageSmooth_PreAlloc_DataAlloc(img, sigma, Nsigma);
    psBool rc = psImageSmooth_PreAlloc_F32(img, smdata);
    if (rc == false) {
        if (expectedRC == true) {
            diag("TEST ERROR: psImageSmooth returned FALSE\n");
            testStatus = false;
        }
    } else {
        if (expectedRC == false) {
            diag("TEST ERROR: psImageSmooth returned TRUE\n");
            testStatus = false;
        }
        if (VERBOSE) {
            p_psImagePrint(1, img, "The Smoothed Image");
        }

        if (flags & TS00_IM_F32) {
            for (psS32 i = 1 ; i < numRows-1 ; i++) {
                for (psS32 j = 1 ; j < numCols-1 ; j++) {
                    if ((fabs(img->data.F32[i][j] - 0.5) > 0.1)) {
                        diag("TEST ERROR: img[%d][%d] was %f, expected 0.5\n",
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
                        diag("TEST ERROR: img[%d][%d] was %f, expected 0.5\n",
                             i, j, img->data.F64[i][j]);
                        testStatus = false;
                    }
                }
            }
        }
    }
    psFree(img);
    psFree(smdata);
    ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    return(testStatus);
}

psS32 main(psS32 argc, char* argv[])
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    plan_tests(14);

    ok(testImageSmoothGeneric(TS00_IM_F32, NUM_COLS, NUM_ROWS, SIGMA, NSIGMA, true), "testImageSmoothGeneric() successful");
    ok(testImageSmoothGeneric(TS00_IM_F32, 1, NUM_ROWS, SIGMA, NSIGMA, true), "testImageSmoothGeneric() successful");
    ok(testImageSmoothGeneric(TS00_IM_F32, NUM_COLS, 1, SIGMA, NSIGMA, true), "testImageSmoothGeneric() successful");
    ok(testImageSmoothGeneric(TS00_IM_F32, 1, 1, SIGMA, NSIGMA, true), "testImageSmoothGeneric() successful");
    // ok(testImageSmoothGeneric(TS00_IM_F64, NUM_COLS, NUM_ROWS, SIGMA, NSIGMA, true), "testImageSmoothGeneric() successful");
    // ok(testImageSmoothGeneric(TS00_IM_S32, NUM_COLS, NUM_ROWS, SIGMA, NSIGMA, false), "testImageSmoothGeneric() successful");
    ok(testImageSmoothGeneric(TS00_IM_NULL, NUM_COLS, NUM_ROWS, SIGMA, NSIGMA, false), "testImageSmoothGeneric() successful");
}
