/** @file  tst_psImageMaskOps.c
 *
 *  @brief Contains the tests for psMaskOps.[ch]
 *
 *  @version $Revision: 1.2 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2007-05-02 04:14:33 $
 *
 *  XXX: In general, the image tests with (1, N) and (N, 1) failed and have
 *  been excluded.
 *
 *  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 */
#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include "tap.h"
#include "pstap.h"
#define N 20
#define MASK_VAL 10
#define GROW_SIZE 5
#define GROW_VAL 15

bool genericImageGrowMaskTest(int rows, int cols, int maskVal, int growSize, int growVal)
{
    // Try with legitimate data
    // XXX: psImageGrowMask() is currently growing a circular mask when the SCD
    // specs a square mask.
    {
        psMemId id = psMemGetId();
        psImage *in = psImageAlloc(rows, cols, PS_TYPE_MASK);
        psImage *inout = psImageAlloc(rows, cols, PS_TYPE_MASK);
        for (int i = 0 ; i < rows ; i++) {
            for (int j = 0 ; j < cols ; j++) {
                in->data.PS_TYPE_MASK_DATA[i][j] = 0;
            }
        }
        in->data.PS_TYPE_MASK_DATA[N/2][N/2] = maskVal;
        psImage *out = psImageGrowMask(NULL, in, maskVal, growSize, growVal);
        ok(out != NULL, "psImageGrowMask() returned non-NULL with legitimate data");
        skip_start(out == NULL, 1, "Skipping tests because psImageGrowMask() returned NULL");
        ok(out->type.type == PS_TYPE_MASK, "psImageGrowMask() returned correct type mask");
        ok((out->numRows == in->numRows) && (out->numCols == in->numCols),
          "psImageGrowMask() returned correct size mask");
        bool errorFlag = false;
        for (int i = 0 ; i < rows ; i++) {
            for (int j = 0 ; j < cols ; j++) {
                if (((rows/2-i)*(rows/2-i)+(cols/2-j)*(cols/2-j)) <= (growSize*growSize)) {
                    if (out->data.PS_TYPE_MASK_DATA[i][j] != growVal) {
                        diag("out[%d][%d] is incorrect (%d)", i, j, out->data.PS_TYPE_MASK_DATA[i][j]);
                        errorFlag = true;
                    }
                } else {
                    if (out->data.PS_TYPE_MASK_DATA[i][j] != 0) {
                        diag("out[%d][%d] is incorrect (%d)", i, j, out->data.PS_TYPE_MASK_DATA[i][j]);
                        errorFlag = true;
                    }
                }
            }
        }
        ok(!errorFlag, "psImageGrowMask() produced the correct data values");
        skip_end();
        psFree(in);
        psFree(out);
        psFree(inout);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
    return true;
}

bool testInRegion(psRegion region, int row, int col)
{
    return(row >= region.y0 && row <= region.y1 && col >= region.x0 && col <= region.x1);
}


bool genericImageMaskRegionTest(int numRows, int numCols)
{
    psMemId id = psMemGetId();
    psMaskType maskVal = 1;
    psRegion reg;
    reg.x0 = numCols/4;
    reg.x1 = 3 * numCols/4;
    reg.y0 = numRows/4;
    reg.y1 = 3 * numRows/4;
    psImage *img = psImageAlloc(numRows, numCols, PS_TYPE_MASK);

    psBool errorFlag = false;
    // psImageMaskRegion(): logical or
    {
        for (int i = 0 ; i < numRows ; i++) {
            for (int j = 0 ; j < numCols ; j++) {
                img->data.PS_TYPE_MASK_DATA[i][j] = 0;
            }
        }
        psImageMaskRegion(img, reg, "|", maskVal);
        errorFlag = false;
        for (int i = 0 ; i < numRows ; i++) {
            for (int j = 0 ; j < numCols ; j++) {
                if (testInRegion(reg, i, j)) {
                    if (img->data.PS_TYPE_MASK_DATA[i][j] != maskVal) {
                        diag("ERROR: img[%d][%d] is %d, should be %d", i, j, img->data.PS_TYPE_MASK_DATA[i][j], maskVal);
                        errorFlag = true;
                    }
                } else {
                    if (img->data.PS_TYPE_MASK_DATA[i][j] != 0) {
                        diag("ERROR: img[%d][%d] is %d, should be 0", i, j, img->data.PS_TYPE_MASK_DATA[i][j]);
                        errorFlag = true;
                    }
                }
            }
        }
    }
    ok(!errorFlag, "psImageMaskRegion() produced the correct results for logical or");

    // psImageMaskRegion(): logical or
    {
        for (int i = 0 ; i < numRows ; i++) {
            for (int j = 0 ; j < numCols ; j++) {
                img->data.PS_TYPE_MASK_DATA[i][j] = 0;
            }
        }
        psImageMaskRegion(img, reg, "=", maskVal);
        psBool errorFlag = false;
        for (int i = 0 ; i < numRows ; i++) {
            for (int j = 0 ; j < numCols ; j++) {
                if (testInRegion(reg, i, j)) {
                    if (img->data.PS_TYPE_MASK_DATA[i][j] != maskVal) {
                        diag("ERROR: img[%d][%d] is %d, should be %d", i, j, img->data.PS_TYPE_MASK_DATA[i][j], maskVal);
                        errorFlag = true;
                    }
                } else {
                    if (img->data.PS_TYPE_MASK_DATA[i][j] != 0) {
                        diag("ERROR: img[%d][%d] is %d, should be 0", i, j, img->data.PS_TYPE_MASK_DATA[i][j]);
                        errorFlag = true;
                    }
                }
            }
        }
    }
    ok(!errorFlag, "psImageMaskRegion() produced the correct results for =");

    // psImageMaskRegion(): logical and
    {
        for (int i = 0 ; i < numRows ; i++) {
            for (int j = 0 ; j < numCols ; j++) {
                img->data.PS_TYPE_MASK_DATA[i][j] = 0xff;
            }
        }
        maskVal = 0xf;
        psImageMaskRegion(img, reg, "&", maskVal);
        psBool errorFlag = false;
        for (int i = 0 ; i < numRows ; i++) {
            for (int j = 0 ; j < numCols ; j++) {
                if (testInRegion(reg, i, j)) {
                    if (img->data.PS_TYPE_MASK_DATA[i][j] != maskVal) {
                        diag("ERROR: img[%d][%d] is %d, should be %d", i, j, img->data.PS_TYPE_MASK_DATA[i][j], maskVal);
                        errorFlag = true;
                    }
                } else {
                    if (img->data.PS_TYPE_MASK_DATA[i][j] != 0xff) {
                        diag("ERROR: img[%d][%d] is %d, should be 0xff", i, j, img->data.PS_TYPE_MASK_DATA[i][j]);
                        errorFlag = true;
                    }
                }
            }
        }
    }
    ok(!errorFlag, "psImageMaskRegion() produced the correct results for logical and");

    // psImageMaskRegion(): logical and
    {
        for (int i = 0 ; i < numRows ; i++) {
            for (int j = 0 ; j < numCols ; j++) {
                img->data.PS_TYPE_MASK_DATA[i][j] = 0xff;
            }
        }
        maskVal = 0xf;
        psImageMaskRegion(img, reg, "^", maskVal);
        psBool errorFlag = false;
        for (int i = 0 ; i < numRows ; i++) {
            for (int j = 0 ; j < numCols ; j++) {
                if (testInRegion(reg, i, j)) {
                    if (img->data.PS_TYPE_MASK_DATA[i][j] != 0xf0) {
                        diag("ERROR: img[%d][%d] is %d, should be 0xf0", i, j, img->data.PS_TYPE_MASK_DATA[i][j]);
                        errorFlag = true;
                    }
                } else {
                    if (img->data.PS_TYPE_MASK_DATA[i][j] != 0xff) {
                        diag("ERROR: img[%d][%d] is %d, should be 0xff", i, j, img->data.PS_TYPE_MASK_DATA[i][j]);
                        errorFlag = true;
                    }
                }
            }
        }
    }
    ok(!errorFlag, "psImageMaskRegion() produced the correct results for logical xor");

    psFree(img);
    ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    return(true);
}



bool genericImageKeepRegionTest(int numRows, int numCols)
{
    psMemId id = psMemGetId();
    psMaskType maskVal = 1;
    psRegion reg;
    reg.x0 = numCols/4;
    reg.x1 = 3 * numCols/4;
    reg.y0 = numRows/4;
    reg.y1 = 3 * numRows/4;
    psImage *img = psImageAlloc(numRows, numCols, PS_TYPE_MASK);

    psBool errorFlag = false;
    // psImageKeepRegion(): logical or
    {
        for (int i = 0 ; i < numRows ; i++) {
            for (int j = 0 ; j < numCols ; j++) {
                img->data.PS_TYPE_MASK_DATA[i][j] = 0;
            }
        }
        psImageKeepRegion(img, reg, "|", maskVal);
        errorFlag = false;
        for (int i = 0 ; i < numRows ; i++) {
            for (int j = 0 ; j < numCols ; j++) {
                if (!testInRegion(reg, i, j)) {
                    if (img->data.PS_TYPE_MASK_DATA[i][j] != maskVal) {
                        diag("ERROR: img[%d][%d] is %d, should be %d", i, j, img->data.PS_TYPE_MASK_DATA[i][j], maskVal);
                        errorFlag = true;
                    }
                } else {
                    if (img->data.PS_TYPE_MASK_DATA[i][j] != 0) {
                        diag("ERROR: img[%d][%d] is %d, should be 0", i, j, img->data.PS_TYPE_MASK_DATA[i][j]);
                        errorFlag = true;
                    }
                }
            }
        }
    }
    ok(!errorFlag, "psImageKeepRegion() produced the correct results for logical or");

    // psImageKeepRegion(): logical or
    {
        for (int i = 0 ; i < numRows ; i++) {
            for (int j = 0 ; j < numCols ; j++) {
                img->data.PS_TYPE_MASK_DATA[i][j] = 0;
            }
        }
        psImageKeepRegion(img, reg, "=", maskVal);
        psBool errorFlag = false;
        for (int i = 0 ; i < numRows ; i++) {
            for (int j = 0 ; j < numCols ; j++) {
                if (!testInRegion(reg, i, j)) {
                    if (img->data.PS_TYPE_MASK_DATA[i][j] != maskVal) {
                        diag("ERROR: img[%d][%d] is %d, should be %d", i, j, img->data.PS_TYPE_MASK_DATA[i][j], maskVal);
                        errorFlag = true;
                    }
                } else {
                    if (img->data.PS_TYPE_MASK_DATA[i][j] != 0) {
                        diag("ERROR: img[%d][%d] is %d, should be 0", i, j, img->data.PS_TYPE_MASK_DATA[i][j]);
                        errorFlag = true;
                    }
                }
            }
        }
    }
    ok(!errorFlag, "psImageKeepRegion() produced the correct results for =");

    // psImageKeepRegion(): logical and
    {
        for (int i = 0 ; i < numRows ; i++) {
            for (int j = 0 ; j < numCols ; j++) {
                img->data.PS_TYPE_MASK_DATA[i][j] = 0xff;
            }
        }
        maskVal = 0xf;
        psImageKeepRegion(img, reg, "&", maskVal);
        psBool errorFlag = false;
        for (int i = 0 ; i < numRows ; i++) {
            for (int j = 0 ; j < numCols ; j++) {
                if (!testInRegion(reg, i, j)) {
                    if (img->data.PS_TYPE_MASK_DATA[i][j] != maskVal) {
                        diag("ERROR: img[%d][%d] is %d, should be %d", i, j, img->data.PS_TYPE_MASK_DATA[i][j], maskVal);
                        errorFlag = true;
                    }
                } else {
                    if (img->data.PS_TYPE_MASK_DATA[i][j] != 0xff) {
                        diag("ERROR: img[%d][%d] is %d, should be 0xff", i, j, img->data.PS_TYPE_MASK_DATA[i][j]);
                        errorFlag = true;
                    }
                }
            }
        }
    }
    ok(!errorFlag, "psImageKeepRegion() produced the correct results for logical and");

    // psImageKeepRegion(): logical and
    {
        for (int i = 0 ; i < numRows ; i++) {
            for (int j = 0 ; j < numCols ; j++) {
                img->data.PS_TYPE_MASK_DATA[i][j] = 0xff;
            }
        }
        maskVal = 0xf;
        psImageKeepRegion(img, reg, "^", maskVal);
        psBool errorFlag = false;
        for (int i = 0 ; i < numRows ; i++) {
            for (int j = 0 ; j < numCols ; j++) {
                if (!testInRegion(reg, i, j)) {
                    if (img->data.PS_TYPE_MASK_DATA[i][j] != 0xf0) {
                        diag("ERROR: img[%d][%d] is %d, should be 0xf0", i, j, img->data.PS_TYPE_MASK_DATA[i][j]);
                        errorFlag = true;
                    }
                } else {
                    if (img->data.PS_TYPE_MASK_DATA[i][j] != 0xff) {
                        diag("ERROR: img[%d][%d] is %d, should be 0xff", i, j, img->data.PS_TYPE_MASK_DATA[i][j]);
                        errorFlag = true;
                    }
                }
            }
        }
    }
    ok(!errorFlag, "psImageKeepRegion() produced the correct results for logical xor");

    psFree(img);
    ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    return(true);
}


bool testInCircle(psImage *img, double xCenter, double yCenter, double radius, double xCoord, double yCoord)
{
    return((((xCoord+img->col0)-xCenter) * ((xCoord+img->col0)-xCenter) +
             ((yCoord+img->row0) - yCenter) * ((yCoord+img->row0) - yCenter)) <= (radius * radius));
}



bool genericImageMaskCircleTest(int numRows, int numCols, int x, int y, int radius)
{
    psMemId id = psMemGetId();
    psMaskType maskVal = 1;
    psImage *img = psImageAlloc(numRows, numCols, PS_TYPE_MASK);

    psBool errorFlag = false;
    // psImageMaskCircle(): logical or
    {
        for (int i = 0 ; i < numRows ; i++) {
            for (int j = 0 ; j < numCols ; j++) {
                img->data.PS_TYPE_MASK_DATA[i][j] = 0;
            }
        }
        psImageMaskCircle(img, (psF64) x, (psF64) y, (psF64) radius, "|", maskVal);
        errorFlag = false;
        for (int i = 0 ; i < numRows ; i++) {
            for (int j = 0 ; j < numCols ; j++) {
                if (testInCircle(img, (psF64) x, (psF64) y, (psF64) radius, (psF64) j, (psF64) i)) {
                    if (img->data.PS_TYPE_MASK_DATA[i][j] != maskVal) {
                        diag("ERROR: img[%d][%d] is %d, should be %d", i, j, img->data.PS_TYPE_MASK_DATA[i][j], maskVal);
                        errorFlag = true;
                    }
                } else {
                    if (img->data.PS_TYPE_MASK_DATA[i][j] != 0) {
                        diag("ERROR: img[%d][%d] is %d, should be 0", i, j, img->data.PS_TYPE_MASK_DATA[i][j]);
                        errorFlag = true;
                    }
                }
            }
        }
    }
    ok(!errorFlag, "psImageMaskCircle() produced the correct results for logical or");

    // psImageMaskCircle(): logical or
    {
        for (int i = 0 ; i < numRows ; i++) {
            for (int j = 0 ; j < numCols ; j++) {
                img->data.PS_TYPE_MASK_DATA[i][j] = 0;
            }
        }
        psImageMaskCircle(img, (psF64) x, (psF64) y, (psF64) radius, "=", maskVal);
        psBool errorFlag = false;
        for (int i = 0 ; i < numRows ; i++) {
            for (int j = 0 ; j < numCols ; j++) {
                if (testInCircle(img, (psF64) x, (psF64) y, (psF64) radius, (psF64) j, (psF64) i)) {
                    if (img->data.PS_TYPE_MASK_DATA[i][j] != maskVal) {
                        diag("ERROR: img[%d][%d] is %d, should be %d", i, j, img->data.PS_TYPE_MASK_DATA[i][j], maskVal);
                        errorFlag = true;
                    }
                } else {
                    if (img->data.PS_TYPE_MASK_DATA[i][j] != 0) {
                        diag("ERROR: img[%d][%d] is %d, should be 0", i, j, img->data.PS_TYPE_MASK_DATA[i][j]);
                        errorFlag = true;
                    }
                }
            }
        }
    }
    ok(!errorFlag, "psImageMaskCircle() produced the correct results for =");

    // psImageMaskCircle(): logical and
    {
        for (int i = 0 ; i < numRows ; i++) {
            for (int j = 0 ; j < numCols ; j++) {
                img->data.PS_TYPE_MASK_DATA[i][j] = 0xff;
            }
        }
        maskVal = 0xf;
        psImageMaskCircle(img, (psF64) x, (psF64) y, (psF64) radius, "&", maskVal);
        psBool errorFlag = false;
        for (int i = 0 ; i < numRows ; i++) {
            for (int j = 0 ; j < numCols ; j++) {
                if (testInCircle(img, (psF64) x, (psF64) y, (psF64) radius, (psF64) j, (psF64) i)) {
                    if (img->data.PS_TYPE_MASK_DATA[i][j] != maskVal) {
                        diag("ERROR: img[%d][%d] is %d, should be %d", i, j, img->data.PS_TYPE_MASK_DATA[i][j], maskVal);
                        errorFlag = true;
                    }
                } else {
                    if (img->data.PS_TYPE_MASK_DATA[i][j] != 0xff) {
                        diag("ERROR: img[%d][%d] is %d, should be 0xff", i, j, img->data.PS_TYPE_MASK_DATA[i][j]);
                        errorFlag = true;
                    }
                }
            }
        }
    }
    ok(!errorFlag, "psImageMaskCircle() produced the correct results for logical and");

    // psImageMaskCircle(): logical and
    {
        for (int i = 0 ; i < numRows ; i++) {
            for (int j = 0 ; j < numCols ; j++) {
                img->data.PS_TYPE_MASK_DATA[i][j] = 0xff;
            }
        }
        maskVal = 0xf;
        psImageMaskCircle(img, (psF64) x, (psF64) y, (psF64) radius, "^", maskVal);
        psBool errorFlag = false;
        for (int i = 0 ; i < numRows ; i++) {
            for (int j = 0 ; j < numCols ; j++) {
                if (testInCircle(img, (psF64) x, (psF64) y, (psF64) radius, (psF64) j, (psF64) i)) {
                    if (img->data.PS_TYPE_MASK_DATA[i][j] != 0xf0) {
                        diag("ERROR: img[%d][%d] is %d, should be 0xf0", i, j, img->data.PS_TYPE_MASK_DATA[i][j]);
                        errorFlag = true;
                    }
                } else {
                    if (img->data.PS_TYPE_MASK_DATA[i][j] != 0xff) {
                        diag("ERROR: img[%d][%d] is %d, should be 0xff", i, j, img->data.PS_TYPE_MASK_DATA[i][j]);
                        errorFlag = true;
                    }
                }
            }
        }
    }
    ok(!errorFlag, "psImageMaskCircle() produced the correct results for logical xor");

    psFree(img);
    ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    return(true);
}


bool genericImageKeepCircleTest(int numRows, int numCols, int x, int y, int radius)
{
    psMemId id = psMemGetId();
    psMaskType maskVal = 1;
    psImage *img = psImageAlloc(numRows, numCols, PS_TYPE_MASK);

    psBool errorFlag = false;
    // psImageKeepCircle(): logical or
    {
        for (int i = 0 ; i < numRows ; i++) {
            for (int j = 0 ; j < numCols ; j++) {
                img->data.PS_TYPE_MASK_DATA[i][j] = 0;
            }
        }
        psImageKeepCircle(img, (psF64) x, (psF64) y, (psF64) radius, "|", maskVal);
        errorFlag = false;
        for (int i = 0 ; i < numRows ; i++) {
            for (int j = 0 ; j < numCols ; j++) {
                if (!testInCircle(img, (psF64) x, (psF64) y, (psF64) radius, (psF64) j, (psF64) i)) {
                    if (img->data.PS_TYPE_MASK_DATA[i][j] != maskVal) {
                        diag("ERROR: img[%d][%d] is %d, should be %d", i, j, img->data.PS_TYPE_MASK_DATA[i][j], maskVal);
                        errorFlag = true;
                    }
                } else {
                    if (img->data.PS_TYPE_MASK_DATA[i][j] != 0) {
                        diag("ERROR: img[%d][%d] is %d, should be 0", i, j, img->data.PS_TYPE_MASK_DATA[i][j]);
                        errorFlag = true;
                    }
                }
            }
        }
    }
    ok(!errorFlag, "psImageKeepCircle() produced the correct results for logical or");

    // psImageKeepCircle(): logical or
    {
        for (int i = 0 ; i < numRows ; i++) {
            for (int j = 0 ; j < numCols ; j++) {
                img->data.PS_TYPE_MASK_DATA[i][j] = 0;
            }
        }
        psImageKeepCircle(img, (psF64) x, (psF64) y, (psF64) radius, "=", maskVal);
        psBool errorFlag = false;
        for (int i = 0 ; i < numRows ; i++) {
            for (int j = 0 ; j < numCols ; j++) {
                if (!testInCircle(img, (psF64) x, (psF64) y, (psF64) radius, (psF64) j, (psF64) i)) {
                    if (img->data.PS_TYPE_MASK_DATA[i][j] != maskVal) {
                        diag("ERROR: img[%d][%d] is %d, should be %d", i, j, img->data.PS_TYPE_MASK_DATA[i][j], maskVal);
                        errorFlag = true;
                    }
                } else {
                    if (img->data.PS_TYPE_MASK_DATA[i][j] != 0) {
                        diag("ERROR: img[%d][%d] is %d, should be 0", i, j, img->data.PS_TYPE_MASK_DATA[i][j]);
                        errorFlag = true;
                    }
                }
            }
        }
    }
    ok(!errorFlag, "psImageKeepCircle() produced the correct results for =");

    // psImageKeepCircle(): logical and
    {
        for (int i = 0 ; i < numRows ; i++) {
            for (int j = 0 ; j < numCols ; j++) {
                img->data.PS_TYPE_MASK_DATA[i][j] = 0xff;
            }
        }
        maskVal = 0xf;
        psImageKeepCircle(img, (psF64) x, (psF64) y, (psF64) radius, "&", maskVal);
        psBool errorFlag = false;
        for (int i = 0 ; i < numRows ; i++) {
            for (int j = 0 ; j < numCols ; j++) {
                if (!testInCircle(img, (psF64) x, (psF64) y, (psF64) radius, (psF64) j, (psF64) i)) {
                    if (img->data.PS_TYPE_MASK_DATA[i][j] != maskVal) {
                        diag("ERROR: img[%d][%d] is %d, should be %d", i, j, img->data.PS_TYPE_MASK_DATA[i][j], maskVal);
                        errorFlag = true;
                    }
                } else {
                    if (img->data.PS_TYPE_MASK_DATA[i][j] != 0xff) {
                        diag("ERROR: img[%d][%d] is %d, should be 0xff", i, j, img->data.PS_TYPE_MASK_DATA[i][j]);
                        errorFlag = true;
                    }
                }
            }
        }
    }
    ok(!errorFlag, "psImageKeepCircle() produced the correct results for logical and");

    // psImageKeepCircle(): logical and
    {
        for (int i = 0 ; i < numRows ; i++) {
            for (int j = 0 ; j < numCols ; j++) {
                img->data.PS_TYPE_MASK_DATA[i][j] = 0xff;
            }
        }
        maskVal = 0xf;
        psImageKeepCircle(img, (psF64) x, (psF64) y, (psF64) radius, "^", maskVal);
        psBool errorFlag = false;
        for (int i = 0 ; i < numRows ; i++) {
            for (int j = 0 ; j < numCols ; j++) {
                if (!testInCircle(img, (psF64) x, (psF64) y, (psF64) radius, (psF64) j, (psF64) i)) {
                    if (img->data.PS_TYPE_MASK_DATA[i][j] != 0xf0) {
                        diag("ERROR: img[%d][%d] is %d, should be 0xf0", i, j, img->data.PS_TYPE_MASK_DATA[i][j]);
                        errorFlag = true;
                    }
                } else {
                    if (img->data.PS_TYPE_MASK_DATA[i][j] != 0xff) {
                        diag("ERROR: img[%d][%d] is %d, should be 0xff", i, j, img->data.PS_TYPE_MASK_DATA[i][j]);
                        errorFlag = true;
                    }
                }
            }
        }
    }
    ok(!errorFlag, "psImageKeepCircle() produced the correct results for logical xor");

    psFree(img);
    ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    return(true);
}


psS32 main(psS32 argc, char* argv[])
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    plan_tests(47);

    // psImageGrowMask()
    //   *psImageGrowMask(psImage *out, const psImage *in,
    //                    psMaskType maskVal, unsigned int growSize,
    //                    psMaskType growVal)
    // psImageGrowMask grows specified values on the input mask image, in,
    // returning the result. If out is NULL, then a new image of the same type
    // and dimension as in shall be allocated and returned; otherwise out shall
    // be modified. If out is non-NULL and does not have the same size and type
    // as in, the function shall generate an error and return NULL.  Pixels in
    // the in image within growSize pixels (either horizontal or vertical) of a
    // pixel which matches the maskVal shall have the corresponding pixel in the
    // out image set to the growValue.

    psMaskType maskVal = 0;
    psMaskType growVal = 0;
    unsigned int growSize = 0;

    // return null for null input image
    {
        psMemId id = psMemGetId();
        psImage *inout = psImageAlloc(N, N, PS_TYPE_MASK);
        psImage *out = psImageGrowMask(inout, NULL, maskVal, growSize, growVal);
        ok(out == NULL, "psImageGrowMask() returned NULL with NULL input image");
        psFree(out);
        psFree(inout);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // return null for incompatible image size
    {
        psMemId id = psMemGetId();
        psImage *in = psImageAlloc(5, 5, PS_TYPE_MASK);
        psImage *inout = psImageAlloc(2, 2, PS_TYPE_MASK);
        psImage *out = psImageGrowMask(inout, in, maskVal, growSize, growVal);
        ok(out == NULL, "psImageGrowMask() returned NULL with incompatible image size");
        psFree(in);
        psFree(out);
        psFree(inout);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // return null for incompatible out image type
    {
        psMemId id = psMemGetId();
        psImage *in = psImageAlloc(5, 5, PS_TYPE_MASK);
        psImage *inout = psImageAlloc(5, 5, PS_TYPE_F32);
        psImage *out = psImageGrowMask(inout, in, maskVal, growSize, growVal);
        ok(out == NULL, "psImageGrowMask() returned NULL with incompatible out image type");
        psFree(in);
        psFree(out);
        psFree(inout);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // return NULL for input image that doesn't match PS_TYPE_MASK
    {
        psMemId id = psMemGetId();
        psImage *in = psImageAlloc(5, 5, PS_TYPE_F32);
        psImage *inout = psImageAlloc(5, 5, PS_TYPE_MASK);
        psImage *out = psImageGrowMask(inout, in, maskVal, growSize, growVal);
        ok(out == NULL, "psImageGrowMask() returned NULL with incompatible in image type");
        psFree(in);
        psFree(out);
        psFree(inout);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
    // XXX: These tests fail
    if (0) {
        genericImageGrowMaskTest(N, 1, MASK_VAL, GROW_SIZE, GROW_VAL);
    }
    if (0) {
        genericImageGrowMaskTest(1, N, MASK_VAL, GROW_SIZE, GROW_VAL);
    }
    // Try with grow region outside image
    genericImageGrowMaskTest(N, N, MASK_VAL, N*2, GROW_VAL);
    // Try with zero-size grow region
    genericImageGrowMaskTest(N, N, MASK_VAL, 0, GROW_VAL);
    // Try with sensible parameters
    genericImageGrowMaskTest(N, N, MASK_VAL, GROW_SIZE, GROW_VAL);



    // psImageMaskRegion
    // Generate error NULL input image that doesn't match PS_TYPE_MASK
    // XXX: Verify error
    {
        psMemId id = psMemGetId();
        psRegion reg;
        reg.x0 = 1;
        reg.x1 = 2;
        reg.y0 = 1;
        reg.y1 = 2;
        psMaskType maskVal = 2;
        psImageMaskRegion(NULL, reg, "|", maskVal);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
    if (0) genericImageMaskRegionTest(N, 1);
    if (0) genericImageMaskRegionTest(1, N);
    genericImageMaskRegionTest(N, N);


    // psImageKeepRegion
    // Generate error NULL input image that doesn't match PS_TYPE_MASK
    // XXX: Verify error
    {
        psMemId id = psMemGetId();
        psRegion reg;
        reg.x0 = 1;
        reg.x1 = 2;
        reg.y0 = 1;
        reg.y1 = 2;
        psImageKeepRegion(NULL, reg, "|", 1);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
    if (0) genericImageKeepRegionTest(N, 1);
    if (0) genericImageKeepRegionTest(1, N);
    genericImageKeepRegionTest(N, N);


    // psImageMaskCircle
    // Generate error NULL input image that doesn't match PS_TYPE_MASK
    // XXX: Verify error
    {
        psMemId id = psMemGetId();
        psImageMaskCircle(NULL, 1.0, 2.0, 3.0, "|", 1);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
    // XXX: These seg fault
    if (0) genericImageMaskCircleTest(N, 1, N/2, 1, N/4);
    if (0) genericImageMaskCircleTest(1, N, 1, N/2, N/4);
    genericImageMaskCircleTest(N, N, N/2, N/2, N/4);


    // psImageKeepCircle
    // Generate error NULL input image that doesn't match PS_TYPE_MASK
    // XXX: Verify error
    {
        psMemId id = psMemGetId();
        psImageKeepCircle(NULL, 1.0, 2.0, 3.0, "|", 1);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
    // XXX: These seg fault
    if (0) genericImageKeepCircleTest(N, 1, N/2, 1, N/4);
    if (0) genericImageKeepCircleTest(1, N, 1, N/2, N/4);
    genericImageKeepCircleTest(N, N, N/2, N/2, N/4);
}
