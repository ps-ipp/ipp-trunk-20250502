/** @file  tst_psImageConvolve.c
 *
 *  @brief Contains the tests for psImageConvolve.[ch]
 *
 *  @author Robert DeSonia, MHPCC
 *
 *  @version $Revision: 1.4 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2007-05-01 00:08:52 $
 *
 *  XXX: Must test the tRelative parameter to psKernelGenerate()
 *  XXX: Must test psImageConvolveFFT()
 *  XXX: Make sure psImageConvolveDirect() is correct.
 *
 *  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 */
#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include "tap.h"
#include "pstap.h"

bool testKernelAlloc(psS32 xMin, psS32 xMax, psS32 yMin, psS32 yMax)
{
    {
        psMemId id = psMemGetId();
        psKernel *k = psKernelAlloc(xMin, xMax, yMin, yMax);
        ok(k != NULL, "psKernelAlloc returned non-NULL for [%d:%d,%d:%d]",
           xMin, xMax, yMin, yMax);
        skip_start(k == NULL, 2, "Skipping tests because psKernelAlloc() returned NULL");
        ok(k->xMin == xMin && k->xMax == xMax &&
           k->yMin == yMin && k->yMax == yMax, "Min/max members, [%d:%d,%d:%d].  Should be [%d:%d,%d:%d]",
           k->xMin,k->xMax, k->yMin, k->yMax,
           xMin, xMax, yMin, yMax);

        ok(k->image->numCols == xMax-xMin+1 && k->image->numRows == yMax-yMin+1,
           "Size of the kernel image (%dx%d vs %dx%d)",
           xMax-xMin+1, yMax-yMin+1, k->image->numCols, k->image->numRows);

        bool errorFlag = false;
        for (psS32 j=yMin; j<yMax; j++) {
            if (k->kernel[j]+xMin != k->image->data.PS_TYPE_KERNEL_DATA[j-yMin]) {
                diag("ERROR: The kernel pointer was set wrong for row %d", j);
                errorFlag = true;
            }
        }
        ok(!errorFlag, "psKernelAlloc() produced the correct data values");
        skip_end();
        psFree(k);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Following should be a warning (xMin > xMax)
    {
        psMemId id = psMemGetId();
        psKernel *k = psKernelAlloc(5, -5, -2, 2);
        ok(k != NULL, "psKernelAlloc returned non-NULL with xMin > xMax");
        skip_start(k == NULL, 1, "Skipping tests because psKernelAlloc() returned NULL");
        ok(k->xMin == -5 && k->xMax == 5, "psKernelAlloc did swap xMin & xMax");
        skip_end();
        psFree(k);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Following should be a warning (yMin > yMax)
    {
        psMemId id = psMemGetId();
        psKernel *k = psKernelAlloc(-2, 2, 5, -5);
        ok(k != NULL, "psKernelAlloc returned non-NULL with yMin > yMax");
        skip_start(k == NULL, 1, "Skipping tests because psKernelAlloc() returned NULL");
        ok(k->yMin == -5 && k->yMax == 5, "psKernelAlloc did swap yMin & yMax");
        skip_end();
        psFree(k);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    return(true);
}

bool testKernelGenerate(void)
{
    psS32 size = 5;
    psS32 t[] = { 1, 2, 8, 9, 10 };
    psS32 x[] = { 0, 1, 0, -1, 0 };
    psS32 y[] = { 2, 1, -1, -2, 0 };
    float sum;

    {
        psMemId id = psMemGetId();
        psVector* xVec = psVectorAlloc(size,PS_TYPE_U32);
        psVector* yVec = psVectorAlloc(size,PS_TYPE_U32);
        psVector* tVec = psVectorAlloc(size,PS_TYPE_U32);

        for (psS32 i = 0; i < size; i++) {
            xVec->data.U32[i] = x[i];
            yVec->data.U32[i] = y[i];
            tVec->data.U32[i] = t[i];
        }

        psKernel* result = psKernelGenerate(tVec, xVec, yVec, false, false);
        ok(result != NULL, "psKernelGenerate() returned non-NULL");
        skip_start(result == NULL, 3, "Skipping tests because psKernelGenerate() returned NULL");
        ok(result->xMin == -1 && result->xMax == 1 &&
           result->yMin == -2 && result->yMax == 2,
           "psKernelGenerate result had a range of [%d:%d,%d:%d].  Suppose to be [-2:2,-1:1]",
           result->xMin, result->xMax, result->yMin, result->yMax);

        sum = 0.0;
        printf("Resulting kernel:\n");
        for (psS32 y = result->yMin; y <= result->yMax; y++) {
            for (psS32 x = result->xMin; x <= result->xMax; x++) {
                printf(" %6.2f ", result->kernel[y][x]);
                sum += result->kernel[y][x];
            }
            printf("\n");
        }
        ok(fabsf(1.0 - sum) <= FLT_EPSILON, "psKernelGenerate result is normalized (sum=%g)", sum);

        ok(fabsf(result->kernel[-2][0] - 0.1) <= FLT_EPSILON &&
           fabsf(result->kernel[ -1][ -1] - 0.1) <= FLT_EPSILON &&
           fabsf(result->kernel[ 1][ 0] - 0.6) <= FLT_EPSILON &&
           fabsf(result->kernel[2][1] - 0.1) <= FLT_EPSILON,
           "psKernelGenerate result values, %g,%g,%g,%g.  Suppose to be 0.1,0.1,0.6,0.1",
           result->kernel[-2][0], result->kernel[-1][-1],
           result->kernel[1][0], result->kernel[2][1]);
        skip_end();
        psFree(result);
        psFree(xVec);
        psFree(yVec);
        psFree(tVec);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    ok(false, "XXXX: Skipping this psKernelGenerate() because of bugs");
    if (0) {
        psMemId id = psMemGetId();
        psVector* xVec = psVectorAlloc(size,PS_TYPE_U32);
        psVector* yVec = psVectorAlloc(size,PS_TYPE_U32);
        psVector* tVec = psVectorAlloc(size,PS_TYPE_U32);

        for (psS32 i = 0; i < size; i++) {
            xVec->data.S16[i] = x[i];
            yVec->data.S16[i] = y[i];
            tVec->data.S16[i] = t[i];
        }

        psKernel *result = psKernelGenerate(tVec, xVec, yVec, false, true);
        ok(result != NULL, "psKernelGenerate() returned non-NULL");
        skip_start(result == NULL, 3, "Skipping tests because psKernelGenerate() returned NULL");
        ok (result->xMin == 0 && result->xMax == 1 &&
            result->yMin == 0 && result->yMax == 3,
            "psKernelGenerate result had a range of [%d:%d,%d:%d].  Suppose to be [0:1,1:2]",
            result->xMin, result->xMax, result->yMin, result->yMax);

        // XXXX: The bug surfaces here
        sum = 0.0;
        if (1) {
            printf("Resulting kernel (relative=true) (%u - %u) (%u - %u):\n",
                   result->yMin, result->yMax, result->xMin, result->xMax);
            for (psS32 y = result->yMin; y <= result->yMax; y++) {
                // printf("y is %d\n", y);
                for (psS32 x = result->xMin; x <= result->xMax; x++) {
                    printf(" %6.2f ", result->kernel[y][x]);
                    sum += result->kernel[y][x];
                }
                printf("\n");
            }
        }
        ok(fabsf(1.0 - sum) <= FLT_EPSILON, "psKernelGenerate result is normalized (sum=%g)", sum);

        ok(fabsf(result->kernel[0][0] - 19.0/30.0) <= FLT_EPSILON &&
           fabsf(result->kernel[2][0] - 1.0/30.0) <= FLT_EPSILON &&
           fabsf(result->kernel[2][1] - 8.0/30.0) <= FLT_EPSILON &&
           fabsf(result->kernel[3][1] - 2.0/30.0) <= FLT_EPSILON,
           "psKernelGenerate result values, %g,%g;%g,%g. Suppose to be 2,6;1,0",
           result->kernel[0][0], result->kernel[2][0],
           result->kernel[2][1], result->kernel[3][1]);
        skip_end();
        psFree(result);
        psFree(xVec);
        psFree(yVec);
        psFree(tVec);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Following should be an error
    if (1) {
        psMemId id = psMemGetId();
        psVector* xVec = psVectorAlloc(size,PS_TYPE_U32);
        psVector* yVec = psVectorAlloc(size,PS_TYPE_U32);
        psVector* tVec = psVectorAlloc(size,PS_TYPE_U32);

        for (psS32 i = 0; i < size; i++) {
            psVectorSet(xVec, i, x[i]+0.1);
            yVec->data.F32[i] = y[i]+0.2;
            tVec->data.F32[i] = t[i]+0.3;
        }

        tVec->n--; // decrease size by one to make vectors unequal in length.
        psKernel *result = psKernelGenerate(tVec, xVec, yVec, false, false);
        ok(result == NULL, "psKernelGenerate returned NULL given differing sized vectors");
        psFree(result);
        psFree(xVec);
        psFree(yVec);
        psFree(tVec);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    psVector* xVec = psVectorAlloc(size,PS_TYPE_U32);
    psVector* yVec = psVectorAlloc(size,PS_TYPE_U32);
    psVector* tVec = psVectorAlloc(size,PS_TYPE_U32);
    // Following should be a error (time vector NULL)
    {
        psMemId id = psMemGetId();
        psKernel *result = psKernelGenerate(NULL, xVec, yVec, false, true);
        ok(result == NULL, "psKernelGenerate returned NULL with no time vector");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Following should be a error (x vector NULL)
    {
        psMemId id = psMemGetId();
        psKernel *result = psKernelGenerate(tVec, NULL, yVec, false, true);
        ok(result == NULL, "psKernelGenerate returned NULL with no x vector");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Following should be a error (y vector NULL
    {
        psMemId id = psMemGetId();
        psKernel *result = psKernelGenerate(tVec, xVec, NULL, false, true);
        ok(result == NULL, "psKernelGenerate returned NULL with no y vector");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    psFree(xVec);
    psFree(yVec);
    psFree(tVec);
    return(true);
}


bool testImageConvolve(void)
{
    psMemId id = psMemGetId();
    const psS32 r = 200;
    const psS32 c = 300;
    psS32 sum;

    // approximate a normalized gaussian kernel.
    psKernel* g = psKernelAlloc(-1,1,-1,1);
    g->kernel[-1][-1] = g->kernel[-1][1] = g->kernel[1][-1] = g->kernel[1][1] = 0.0113;
    g->kernel[1][0] = g->kernel[-1][0] = g->kernel[0][-1] = g->kernel[0][1] = 0.0838;
    g->kernel[0][0] = 0.6193;

    // create a normalized non-symetric kernel.
    psKernel* nsk = psKernelAlloc(0,2,0,2);
    sum = 0.0;
    for (psS32 i=0;i<2;i++) {
        for (psS32 j=0;j<2;j++) {
            nsk->kernel[i][j] = i+j;
            sum = i+j;
        }
    }
    for (psS32 i=0;i<2;i++) {
        for (psS32 j=0;j<2;j++) {
            nsk->kernel[i][j] /= sum;
        }
    }


    psImage* img = psImageAlloc(c,r,PS_TYPE_F32);
    memset(img->data.F32[0],0,c*r*PSELEMTYPE_SIZEOF(PS_TYPE_F32));
    img->data.F32[0][0] = 1.0f;
    img->data.F32[r/2][c/2] = 1.0f;
    img->data.F32[r-1][c/2] = 1.0f;

    // test spacial convolution of gaussian
    ok(false, "XXXX: Skipping this psImageConvolve() because of bugs");
/*
    if (0) {
        psMemId id = psMemGetId();
        psImage* out = psImageConvolveDirect(NULL, img, g);
        ok(out != NULL, "psImageConvolveDirect() returned non-NULL");
        skip_start(out == NULL, 3, "Skipping tests because psImageConvolveDirect() returned NULL");
        ok(out->numCols == c && out->numRows == r,
           "psImageConvolveDirect result image is %dx%d, expected %dx%d",
           out->numCols, out->numRows, c,r);
        ok(out->type.type == PS_TYPE_F32, "psImageConvolveDirect() produced the correct type");

        // test values
        bool errorFlag = false;
        for (psS32 i=-1;i<1;i++) {
            for (psS32 j=-1;j<1;j++) {
                if (fabsf(out->data.F32[r/2+i][c/2+j] - g->kernel[i][j]) > 0.0001) {
                    diag("Convolved image wrong at %d,%d.  Value is %g, expected %g",
                         c/2+j,r/2+i, out->data.F32[r/2+i][c/2+j], g->kernel[i][j]);
                    errorFlag = true;
                }
                if (i >= 0 && j >= 0 && fabsf(out->data.F32[i][j] - g->kernel[i][j]) > 0.0001) {
                    diag("Convolved image wrong at %d,%d.  Value is %g, expected %g",
                         j, i, out->data.F32[i][j], g->kernel[i][j]);
                    errorFlag = true;
                }
                if (i <= 0 && fabsf(out->data.F32[r-1+i][c/2+j] - g->kernel[i][j]) > 0.0001) {
                    diag("Convolved image wrong at %d,%d.  Value is %g, expected %g",
                         c/2+j,r-1+i, out->data.F32[r-1+i][c/2+j], g->kernel[i][j]);
                    errorFlag = true;
                }
            }
        }
        ok(!errorFlag, "psKernelGenerate() produced the correct data values");
        skip_end();
        psFree(out);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // test fourier convolution of gaussian
    ok(false, "XXXX: Skipping this psImageConvolveDirect() because of bugs");
    if (0) {
        psMemId id = psMemGetId();
        psImage* out = psImageConvolveDirect(NULL, img, g);
        psImage* out2 = psImageConvolveDirect(out, img, g);

        ok(out != NULL, "psImageConvolveDirect() returned non-NULL");
        ok(out2 != NULL, "psImageConvolveDirect() returned non-NULL");
        skip_start(out == NULL || out2 == NULL, 3, "Skipping tests because psImageConvolveDirect() returned NULL");
        ok(out == out2, "psImageConvolveDirect did recycle the supplied out image struct");
        ok(out->numCols == c && out->numRows == r,
           "psImageConvolveDirect result image is %dx%d, expected %dx%d",
           out->numCols, out->numRows, c,r);
        ok(out->type.type == PS_TYPE_F32, "psImageConvolveDirect() produced the correct type");

        // test values
        bool errorFlag = false;
        for (psS32 i=-1;i<1;i++) {
            for (psS32 j=-1;j<1;j++) {
                if (fabsf(out->data.F32[r/2+i][c/2+j] - g->kernel[i][j]) > 0.01) {
                    diag("Convolved image wrong at %d,%d.  Value is %g, expected %g",
                         c/2+j,r/2+i, out->data.F32[r/2+i][c/2+j], g->kernel[i][j]);
                    errorFlag = true;
                }
                if (i >= 0 && j >= 0 && fabsf(out->data.F32[i][j] - g->kernel[i][j]) > 0.01) {
                    diag("Convolved image wrong at %d,%d.  Value is %g, expected %g",
                         j,i, out->data.F32[i][j], g->kernel[i][j]);
                    errorFlag = true;
                }
                if (i <= 0 && fabsf(out->data.F32[r-1+i][c/2+j] - g->kernel[i][j]) > 0.01) {
                    diag("Convolved image wrong at %d,%d.  Value is %g, expected %g",
                         c/2+j,r-1+i, out->data.F32[r-1+i][c/2+j], g->kernel[i][j]);
                    errorFlag = true;
                }
            }
        }
        ok(!errorFlag, "psKernelGenerate() produced the correct data values");
        skip_end();
        psFree(out);
        psFree(out2);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
*/

    psFree(g);
    psFree(img);
    psFree(nsk);
    ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    return(true);
}

psS32 main(psS32 argc, char* argv[])
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    plan_tests(62);

    testKernelAlloc(-5, 0, -4, 0);
    testKernelAlloc(0, 5, 0, 4);
    testKernelAlloc(-10, -5, -8, -4);
    testKernelAlloc(5, 10, 4, 8);
    testKernelGenerate();
    testImageConvolve();
}

