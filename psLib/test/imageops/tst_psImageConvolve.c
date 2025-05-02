/** @file  tst_psImageConvolve.c
 *
 *  @brief Contains the tests for psImageConvolve.[ch]
 *
 *  @author Robert DeSonia, MHPCC
 *
 *  @version $Revision: 1.2 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2006-02-24 23:43:15 $
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

static psS32 testKernelAlloc(void);
static psS32 testKernelGenerate(void);
static psS32 testImageConvolve(void);

testDescription tests[] = {
                              {testKernelAlloc,731,"psKernelAlloc",0,false},
                              {testKernelGenerate,732,"psKernelGenerate",0,false},
                              {testImageConvolve,733,"psImageConvolve",0,false},
                              {NULL}
                          };

psS32 main(psS32 argc, char* argv[])
{
    psLogSetLevel(PS_LOG_INFO);

    return ! runTestSuite(stderr,"psImage",tests,argc,argv);
}

static psS32 testKernelAlloc(void)
{
    psS32 numCases = 4;
    psS32 xMin[] = { -5,  0,-10,  5};
    psS32 xMax[] = {  0,  5, -5, 10};
    psS32 yMin[] = { -4,  0, -8,  4};
    psS32 yMax[] = {  0,  4, -4,  8};
    psS32 i;
    psKernel* k;

    for (i=0;i<numCases;i++) {
        k = psKernelAlloc(xMin[i],xMax[i],yMin[i],yMax[i]);

        if (k == NULL) {
            psError(PS_ERR_UNKNOWN, true,"psKernelAlloc returned NULL for [%d:%d,%d:%d].",
                    xMin[i], xMax[i], yMin[i], yMax[i]);
            return i*10+1;
        }

        if (k->xMin != xMin[i] || k->xMax != xMax[i] ||
                k->yMin != yMin[i] || k->yMax != yMax[i]) {
            psError(PS_ERR_UNKNOWN, true,"Min/max members, [%d:%d,%d:%d], of psKernel wrong. Should be [%d:%d,%d:%d].",
                    k->xMin,k->xMax, k->yMin, k->yMax,
                    xMin[i], xMax[i], yMin[i], yMax[i]);
            return i*10+2;
        }

        if (k->image->numCols != xMax[i]-xMin[i]+1 ||
                k->image->numRows != yMax[i]-yMin[i]+1) {
            psError(PS_ERR_UNKNOWN, true,"Size of the kernel image is wrong (%dx%d vs %dx%d).",
                    xMax[i]-xMin[i]+1, yMax[i]-yMin[i]+1,
                    k->image->numCols, k->image->numRows);
            return i*10+2;
        }

        for (psS32 j=yMin[i]; j<yMax[i]; j++) {
            if (k->kernel[j]+xMin[i] != k->image->data.PS_TYPE_KERNEL_DATA[j-yMin[i]]) {
                psError(PS_ERR_UNKNOWN, true,"The kernel pointer was set wrong for row %d.",
                        j);
                return i*10+4;
            }
        }

        psFree(k);
    }

    psLogMsg(__func__,PS_LOG_INFO,"Following should be a warning (xMin > xMax)");
    k = psKernelAlloc(5, -5, -2, 2);
    if (k == NULL) {
        psError(PS_ERR_UNKNOWN, true,"psKernelAlloc returned NULL for xMin > xMax.");
        return i*10+5;
    }

    if (k->xMin != -5 || k->xMax != 5) {
        psError(PS_ERR_UNKNOWN, true,"psKernelAlloc didn't swap xMin & xMax.");
        return i*10+6;
    }

    psFree(k);

    psLogMsg(__func__,PS_LOG_INFO,"Following should be a warning (yMin > yMax)");
    k = psKernelAlloc(-2, 2, 5, -5);
    if (k == NULL) {
        psError(PS_ERR_UNKNOWN, true,"psKernelAlloc returned NULL for yMin > yMax.");
        return i*10+7;
    }

    if (k->yMin != -5 || k->yMax != 5) {
        psError(PS_ERR_UNKNOWN, true,"psKernelAlloc didn't swap yMin & yMax.");
        return i*10+8;
    }

    psFree(k);

    return 0;
}

static psS32 testKernelGenerate(void)
{
    psS32 size = 5;
    psS32 t[] = { 1, 2, 8, 9, 10 };
    psS32 x[] = { 0, 1, 0, -1, 0 };
    psS32 y[] = { 2, 1, -1, -2, 0 };
    float sum;

    psVector* xVec = psVectorAlloc(size,PS_TYPE_U32);
    psVector* yVec = psVectorAlloc(size,PS_TYPE_U32);
    psVector* tVec = psVectorAlloc(size,PS_TYPE_U32);

    for (psS32 i = 0; i < size; i++) {
        xVec->data.U32[i] = x[i];
        xVec->n++;
        yVec->data.U32[i] = y[i];
        yVec->n++;
        tVec->data.U32[i] = t[i];
        tVec->n++;
    }

    psKernel* result = psKernelGenerate(tVec, xVec, yVec, false);

    if (result == NULL) {
        psError(PS_ERR_UNKNOWN, true,"psKernelGenerate returned NULL.");
        return 1;
    }

    if (result->xMin != -1 || result->xMax != 1 ||
            result->yMin != -2 || result->yMax != 2) {
        psError(PS_ERR_UNKNOWN, true,"psKernelGenerate result had a range of [%d:%d,%d:%d].  Suppose to be [-2:2,-1:1].",
                result->xMin, result->xMax, result->yMin, result->yMax);
        return 2;
    }

    sum = 0.0;
    printf("Resulting kernel:\n");
    for (psS32 y = result->yMin; y <= result->yMax; y++) {
        for (psS32 x = result->xMin; x <= result->xMax; x++) {
            printf(" %6.2f ", result->kernel[y][x]);
            sum += result->kernel[y][x];
        }
        printf("\n");
    }
    if (fabsf(1.0 - sum) > FLT_EPSILON) {
        psError(PS_ERR_UNKNOWN, true,"psKernelGenerate result is not normalized (sum=%g).",
                sum);

        return 3;

    }

    if (fabsf(result->kernel[-2][0] - 0.1) > FLT_EPSILON ||
            fabsf(result->kernel[ -1][ -1] - 0.1) > FLT_EPSILON ||
            fabsf(result->kernel[ 1][ 0] - 0.6) > FLT_EPSILON ||
            fabsf(result->kernel[2][1] - 0.1) > FLT_EPSILON) {
        psError(PS_ERR_UNKNOWN, true,"psKernelGenerate result values, %g,%g,%g,%g, are wrong. Suppose to be 0.1,0.1,0.6,0.1",
                result->kernel[-2][0], result->kernel[-1][-1],
                result->kernel[1][0], result->kernel[2][1]);

        return 4;
    }

    psFree(result);
    psFree(xVec);
    psFree(yVec);
    psFree(tVec);

    xVec = psVectorAlloc(size,PS_TYPE_S16);
    yVec = psVectorAlloc(size,PS_TYPE_S16);
    tVec = psVectorAlloc(size,PS_TYPE_S16);

    for (psS32 i = 0; i < size; i++) {
        xVec->data.S16[i] = x[i];
        xVec->n++;
        yVec->data.S16[i] = y[i];
        yVec->n++;
        tVec->data.S16[i] = t[i];
        tVec->n++;
    }

    result = psKernelGenerate(tVec, xVec, yVec, true);

    if (result == NULL) {
        psError(PS_ERR_UNKNOWN, true,"psKernelGenerate returned NULL.");
        return 5;
    }

    if (result->xMin != 0 || result->xMax != 1 ||
            result->yMin != 0 || result->yMax != 3) {
        psError(PS_ERR_UNKNOWN, true,"psKernelGenerate result had a range of [%d:%d,%d:%d].  Suppose to be [0:1,1:2].",
                result->xMin, result->xMax, result->yMin, result->yMax);
        return 6;
    }

    sum = 0.0;
    printf("Resulting kernel (relative=true):\n");
    for (psS32 y = result->yMin; y <= result->yMax; y++) {
        for (psS32 x = result->xMin; x <= result->xMax; x++) {
            printf(" %6.2f ", result->kernel[y][x]);
            sum += result->kernel[y][x];
        }
        printf("\n");
    }
    if (fabsf(1.0 - sum) > FLT_EPSILON) {
        psError(PS_ERR_UNKNOWN, true,"psKernelGenerate result is not normalized (sum=%g).",
                sum);

        return 7;

    }

    if (fabsf(result->kernel[0][0] - 19.0/30.0) > FLT_EPSILON ||
            fabsf(result->kernel[2][0] - 1.0/30.0) > FLT_EPSILON ||
            fabsf(result->kernel[2][1] - 8.0/30.0) > FLT_EPSILON ||
            fabsf(result->kernel[3][1] - 2.0/30.0) > FLT_EPSILON) {
        psError(PS_ERR_UNKNOWN, true,"psKernelGenerate result values, %g,%g;%g,%g, are wrong. Suppose to be 2,6;1,0.",
                result->kernel[0][0], result->kernel[2][0],
                result->kernel[2][1], result->kernel[3][1]);

        return 8;
    }

    psFree(result);
    psFree(xVec);
    psFree(yVec);
    psFree(tVec);

    xVec = psVectorAlloc(size,PS_TYPE_F32);
    yVec = psVectorAlloc(size,PS_TYPE_F32);
    tVec = psVectorAlloc(size,PS_TYPE_F32);

    for (psS32 i = 0; i < size; i++) {
        //        xVec->data.F32[i] = x[i]+0.1;
        //        xVec->n++;
        psVectorSet(xVec, i, x[i]+0.1);
        yVec->data.F32[i] = y[i]+0.2;
        yVec->n++;
        tVec->data.F32[i] = t[i]+0.3;
        tVec->n++;
    }

    tVec->n--; // decrease size by one to make vectors unequal in length.
    psLogMsg(__func__,PS_LOG_INFO, "Following should be an error.");
    result = psKernelGenerate(tVec, xVec, yVec, false);
    if (result != NULL) {
        psError(PS_ERR_UNKNOWN, true,"psKernelGenerate returned non-NULL given differing sized vectors.");
        return 9;
    }

    psFree(result);

    psLogMsg(__func__,PS_LOG_INFO, "Following should be a error (time vector NULL).");
    result = psKernelGenerate(NULL, xVec, yVec, true);
    if (result != NULL) {
        psError(PS_ERR_UNKNOWN, true,"psKernelGenerate returned a kernel with no time vector.");
        return 11;
    }

    psLogMsg(__func__,PS_LOG_INFO, "Following should be a error (x vector NULL).");
    result = psKernelGenerate(tVec, NULL, yVec, true);
    if (result != NULL) {
        psError(PS_ERR_UNKNOWN, true,"psKernelGenerate returned a kernel with no x vector.");
        return 11;
    }

    psLogMsg(__func__,PS_LOG_INFO, "Following should be a error (y vector NULL).");
    result = psKernelGenerate(tVec, xVec, NULL, true);
    if (result != NULL) {
        psError(PS_ERR_UNKNOWN, true,"psKernelGenerate returned a kernel with no y vector.");
        return 11;
    }

    psFree(xVec);
    psFree(yVec);
    psFree(tVec);
    psFree(result);

    return 0;
}

static psS32 testImageConvolve(void)
{
    const psS32 r = 200;
    const psS32 c = 300;
    psS32 sum;

    // approximate a normalized gaussian kernel.
    psKernel* g = psKernelAlloc(-1,1,-1,1);
    g->kernel[-1][-1] =
        g->kernel[-1][1] =
            g->kernel[1][-1] =
                g->kernel[1][1] = 0.0113;
    g->kernel[1][0] =
        g->kernel[-1][0] =
            g->kernel[0][-1] =
                g->kernel[0][1] = 0.0838;
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
    psLogMsg(__func__,PS_LOG_INFO,"Testing direct gaussian convolution");
    psImage* out = psImageConvolve(NULL, img, g, true);

    if (out == NULL) {
        psError(PS_ERR_UNKNOWN, true, "psImageConvolve returned a NULL for direct gaussian case.");
        return 1;
    }

    if (out->numCols != c || out->numRows != r) {
        psError(PS_ERR_UNKNOWN, true, "psImageConvolve result image is %dx%d, but expected %dx%d.",
                out->numCols, out->numRows,
                c,r);
        return 2;
    }

    if (out->type.type != PS_TYPE_F32) {
        char* typeStr;
        PS_TYPE_NAME(typeStr,out->type.type);
        psError(PS_ERR_UNKNOWN, true, "psImageConvolve result image is of type %s, not psF32.",
                typeStr);
        return 3;
    }

    // test values
    for (psS32 i=-1;i<1;i++) {
        for (psS32 j=-1;j<1;j++) {
            if (fabsf(out->data.F32[r/2+i][c/2+j] - g->kernel[i][j]) > 0.0001) {
                psError(PS_ERR_UNKNOWN, true,"Convolved image wrong at %d,%d.  Value is %g, expected %g.",
                        c/2+j,r/2+i,
                        out->data.F32[r/2+i][c/2+j], g->kernel[i][j]);
                return 4;
            }
            if (i >= 0 && j >= 0 && fabsf(out->data.F32[i][j] - g->kernel[i][j]) > 0.0001) {
                psError(PS_ERR_UNKNOWN, true,"Convolved image wrong at %d,%d.  Value is %g, expected %g.",
                        j,i,
                        out->data.F32[i][j], g->kernel[i][j]);
                return 5;
            }
            if (i <= 0 && fabsf(out->data.F32[r-1+i][c/2+j] - g->kernel[i][j]) > 0.0001) {
                psError(PS_ERR_UNKNOWN, true,"Convolved image wrong at %d,%d.  Value is %g, expected %g.",
                        c/2+j,r-1+i,
                        out->data.F32[r-1+i][c/2+j], g->kernel[i][j]);
                return 6;
            }
        }
    }

    // test fourier convolution of gaussian
    psLogMsg(__func__,PS_LOG_INFO,"Testing fourier gaussian convolution");
    psImage* out2 = psImageConvolve(out, img, g, false);

    if (out == NULL) {
        psError(PS_ERR_UNKNOWN, true, "psImageConvolve returned a NULL for gaussian case.");
        return 10;
    }

    if (out != out2) {
        psError(PS_ERR_UNKNOWN, true, "psImageConvolve didn't recycle the supplied out image struct.");
        return 11;
    }

    if (out->numCols != c || out->numRows != r) {
        psError(PS_ERR_UNKNOWN, true, "psImageConvolve result image is %dx%d, but expected %dx%d.",
                out->numCols, out->numRows,
                c,r);
        return 12;
    }

    if (out->type.type != PS_TYPE_F32) {
        char* typeStr;
        PS_TYPE_NAME(typeStr,out->type.type);
        psError(PS_ERR_UNKNOWN, true, "psImageConvolve result image is of type %s, not psF32.",
                typeStr);
        return 13;
    }

    // test values
    for (psS32 i=-1;i<1;i++) {
        for (psS32 j=-1;j<1;j++) {
            if (fabsf(out->data.F32[r/2+i][c/2+j] - g->kernel[i][j]) > 0.01) {
                psError(PS_ERR_UNKNOWN, true,"Convolved image wrong at %d,%d.  Value is %g, expected %g.",
                        c/2+j,r/2+i,
                        out->data.F32[r/2+i][c/2+j], g->kernel[i][j]);
                return 14;
            }
            if (i >= 0 && j >= 0 && fabsf(out->data.F32[i][j] - g->kernel[i][j]) > 0.01) {
                psError(PS_ERR_UNKNOWN, true,"Convolved image wrong at %d,%d.  Value is %g, expected %g.",
                        j,i,
                        out->data.F32[i][j], g->kernel[i][j]);
                return 15;
            }
            if (i <= 0 && fabsf(out->data.F32[r-1+i][c/2+j] - g->kernel[i][j]) > 0.01) {
                psError(PS_ERR_UNKNOWN, true,"Convolved image wrong at %d,%d.  Value is %g, expected %g.",
                        c/2+j,r-1+i,
                        out->data.F32[r-1+i][c/2+j], g->kernel[i][j]);
                return 16;
            }
        }
    }

    psFree(g);
    psFree(img);
    psFree(nsk);
    psFree(out);
    return 0;
}

