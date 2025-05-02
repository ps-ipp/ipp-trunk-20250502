/** @file  tst_psImageExtraction.c
*
*  @brief Contains the tests for psImageExtraction.[ch]
*
*
*  @author Robert DeSonia, MHPCC
*
*  @version $Revision: 1.7 $ $Name: not supported by cvs2svn $
*  @date $Date: 2009-01-26 21:27:12 $
*
*  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
*/
#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include "tap.h"
#include "pstap.h"

void genericImageRowColTests(int numRows, int numCols)
{
    psMemId id = psMemGetId();
    psImage *image = psImageAlloc(numCols, numRows, PS_TYPE_F32);
    for (int i = 0 ; i < numRows; i++) {
        for (int j = 0 ; j < numCols; j++) {
            image->data.F32[0][0] = (float) i+j;
        }
    }            

    bool errorFlag = false;
    for (int i = 0 ; i < numRows; i++) {
        psVector *out = psImageRow(NULL, image, i);
        ok(out != NULL, "psImageRow returned non-NULL");
        if (out != NULL) {
            for (int j = 0 ; j < numCols; j++) {
                if (out->data.F32[j] != image->data.F32[i][j]) {
                    diag("TEST ERROR: image->data.F32[%d][%d] is %f, should be %f", i, j, 
                         image->data.F32[i][j], out->data.F32[j]);
                    errorFlag = true;
                }
            }
            psFree(out);
        } else {
            errorFlag = true;
        }
    }
    ok(!errorFlag, "psImageRow() passed tests with correct data inputs");
    psFree(image);
    ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
}



psS32 main(psS32 argc, char* argv[])
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    plan_tests(295);

    // test psImageSlice()
    {
        psMemId id = psMemGetId();
        const psS32 r = 200;
        const psS32 c = 300;
        const psS32 m = r / 2 -1;
        const psS32 n = r / 4 -1;
        psImage* image;
        psPixels* positions = psPixelsAlloc(r);
        psImage* mask = psImageAlloc(c, r, PS_TYPE_MASK);
        psStats* stat = psStatsAlloc(PS_STAT_SAMPLE_MEDIAN);
    

        // This function shall extract pixels from a specified region of a psImage
        // structure into a vector using a specified statistical method.
        //    
        // Verify the returned psVector structure contains expected data, if the
        // input psImage input has known data and the psStats structure specifies
        // a known statistical method. At least two different statistical methods
        // should be used within a valid range of psImage data. Allow for a delta
        // when comparing results to allow for testing a different platforms. Two
        // cases should be used for each possible direction. Data region cases
        // should include 0x0, 1x1, Nx1, 1xN, NxN, MxN
        for (psS32 row = 0;row < r;row++) {
            psMaskType* maskRow = mask->data.PS_TYPE_MASK_DATA[row];
            for (psS32 col = 0;col < c;col++) {
                maskRow[col] = 0;
            }
        }
    
        #define PSIMAGESLICE_TEST1(TYPE,M,N,DIRECTION,TRUTH_SIZE,TRUTHPIX_X,TRUTHPIX_Y,TESTNUM) \
        { \
            psMemId id = psMemGetId(); \
            psVector* out = NULL; \
            bool errorFlag = false; \
            image = psImageAlloc(c, r, PS_TYPE_##TYPE); \
            for (psS32 row = 0;row < r;row++) { \
                ps##TYPE *imageRow = image->data.TYPE[row]; \
                ps##TYPE rowOffset = row * 2; \
                for (psS32 col = 0;col < c;col++) { \
                    imageRow[col] = col + rowOffset; \
                } \
            } \
            P_PSIMAGE_SET_COL0(image, 1); \
            P_PSIMAGE_SET_ROW0(image, 1); \
            out = psImageSlice(out,positions,image,mask,1, \
                               psRegionSet(1+c/10,1+c/10+M,1+r/10,1+r/10+N),DIRECTION,stat); \
            \
            if (out->n != TRUTH_SIZE) { \
                diag("Number of results is wrong (%d, not %d)", \
                      out->n,TRUTH_SIZE); \
                errorFlag = true; \
            } \
            \
            if (positions->n != TRUTH_SIZE) { \
                diag("Number of results for positions vector is wrong (%d, not %d)", \
                      positions->n,TRUTH_SIZE); \
                errorFlag = true; \
            } \
            \
            for (psS32 i=0;i<out->n;i++) { \
                if (fabs(out->data.F64[i]-image->data.TYPE[r/10+TRUTHPIX_Y][c/10+TRUTHPIX_X]) > 1.0/(psF64)r) { \
                    diag("Improper result at position %d.  Got %g, expected %g",i, \
                          out->data.F64[i],image->data.TYPE[r/10+TRUTHPIX_Y][c/10+TRUTHPIX_X]); \
                    errorFlag = true; \
                } \
                if (DIRECTION == PS_CUT_X_POS || DIRECTION == PS_CUT_X_NEG) { \
                    if (positions->data[i].x != c/10+TRUTHPIX_X) { \
                        diag("Improper positions (%d vs %d) result @ %d.", \
                              positions->data[i].x,c/10+TRUTHPIX_X,i); \
                        errorFlag = true; \
                    } \
                } else { \
                    if (positions->data[i].y != r/10+TRUTHPIX_Y) { \
                        diag("Improper positions (%d vs %d) result @ %d.", \
                              positions->data[i].y,r/10+TRUTHPIX_Y,i); \
                        errorFlag = true; \
                    } \
                } \
            } \
            ok(!errorFlag, "psImageSlice() produced correct data values (test number %d", TESTNUM); \
            psFree(out); \
            psFree(image); \
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks"); \
        }
    
        //
        #define PSIMAGESLICE_TEST(TYPE) \
        PSIMAGESLICE_TEST1(TYPE, m, n, PS_CUT_X_POS, m, i, n / 2, 0); \
        PSIMAGESLICE_TEST1(TYPE, m, n, PS_CUT_X_NEG, m, m - 1 - i, n / 2, 1); \
        PSIMAGESLICE_TEST1(TYPE, m, n, PS_CUT_Y_POS, n, m / 2, i, 2); \
        PSIMAGESLICE_TEST1(TYPE, m, n, PS_CUT_Y_NEG, n, m / 2, n - 1 - i, 3); \
        \
        PSIMAGESLICE_TEST1(TYPE, m, 1, PS_CUT_X_POS, m, i, 0, 4); \
        PSIMAGESLICE_TEST1(TYPE, m, 1, PS_CUT_X_NEG, m, m - 1 - i, 0, 5); \
        PSIMAGESLICE_TEST1(TYPE, m, 1, PS_CUT_Y_POS, 1, m / 2, 0, 6); \
        PSIMAGESLICE_TEST1(TYPE, m, 1, PS_CUT_Y_NEG, 1, m / 2, 0, 7); \
        \
        PSIMAGESLICE_TEST1(TYPE, 1, n, PS_CUT_X_POS, 1, 0, n / 2, 8); \
        PSIMAGESLICE_TEST1(TYPE, 1, n, PS_CUT_X_NEG, 1, 0, n / 2, 9); \
        PSIMAGESLICE_TEST1(TYPE, 1, n, PS_CUT_Y_POS, n, 0, i, 10); \
        PSIMAGESLICE_TEST1(TYPE, 1, n, PS_CUT_Y_NEG, n, 0, n - 1 - i, 11); \
        \
        PSIMAGESLICE_TEST1(TYPE, 1, 1, PS_CUT_X_POS, 1, 0, 0, 12); \
        PSIMAGESLICE_TEST1(TYPE, 1, 1, PS_CUT_X_NEG, 1, 0, 0, 13); \
        PSIMAGESLICE_TEST1(TYPE, 1, 1, PS_CUT_Y_POS, 1, 0, 0, 14); \
        PSIMAGESLICE_TEST1(TYPE, 1, 1, PS_CUT_Y_NEG, 1, 0, 0, 15); \
    
        PSIMAGESLICE_TEST(F32);

        PSIMAGESLICE_TEST(F64);
        PSIMAGESLICE_TEST(U16);
    
        image = psImageAlloc(c, r, PS_TYPE_F32);
        // Verify the returned psVector structure pointer is null and program
        // execution doesn't stop, if input psImage input is null.
        // Following should be an error
        // XXX: Verify error


        psVector *out = NULL;
        out = psImageSlice(out,
                           NULL, NULL,
                           NULL, 0,
                           psRegionSet(c/10, c/10 + 1, r/10, r/10 + 1),
                           PS_CUT_X_POS,
                           stat);
        ok(out == NULL, "psImageSlice() returned NULL with NULL input");
    
        // Verify the returned psVector structure pointer is null and program
        // execution doesn't stop, if input psStats stats is null.
        // Following should be an error
        // XXX: Verify error
        out = psImageSlice(out,
                            NULL, image,
                            mask, 1,
                            psRegionSet(c/10, c/10 + 1, r/10, r/10 + 1),
                            PS_CUT_X_POS,
                            NULL);
        ok(out == NULL, "psImageSlice() returned NULL with NULL psStats");
            psError(PS_ERR_UNKNOWN,true, "Giving a NULL stat struct, psImageSlice didn't return NULL as expected");

        // Verify the returned psVector structure pointer is null and program
        // executions doesn't stop, if the input direction is not set to one of
        // the two valid values.
        // Following should be an error
        // XXX: Verify error
        out = psImageSlice(out, NULL,
                           image,
                           mask, 1,
                           psRegionSet(c/10, c/10 + 1, r/10, r/10 + 1),
                           5,
                           stat);
        ok(out == NULL, "psImageSlice() returned NULL with bogus direction flag");

        // Verify the returned psVector structure pointer is null and program
        // execution doesn't stop, if the input nrow and/or ncol are zero.
        // Following should be an error
        // XXX: Verify error
        out = psImageSlice(out,
                            NULL,
                            image,
                            mask, 1,
                            psRegionSet(c/10, c/10, r/10, r/10),
                            PS_CUT_X_POS,
                            stat);
        ok(out == NULL, "psImageSlice() returned NULL with 0x0 region");
    
        // Verify the returned psVector structure pointer is null and program
        // execution doesn't stop, if the inputs row, col, nrow, ncol specify a
        // regions of data that is not within the input psImage structure.
        // Following should be an error
        // XXX: Verify error
        out = psImageSlice(out, NULL,
                            image,
                            mask, 1,
                            psRegionSet(c+1, c+2, r/10, r/10 + 10),
                            PS_CUT_X_POS,
                            stat);
        ok(out == NULL, "psImageSlice() returned NULL with unallowed x position");
    
        // Following should be an error
        // XXX: Verify error
        out = psImageSlice(out, NULL,
                            image,
                            mask, 1,
                            psRegionSet(c/10, c/10 + 1, r+1,r+5),
                            PS_CUT_X_POS,
                            stat);
        ok(out == NULL, "psImageSlice() returned NULL with unallowed y position");
    
        // Following should be an error
        // XXX: Verify error
        out = psImageSlice(out, NULL,
                            image,
                            mask, 1,
                            psRegionSet(c/10, c+1, r/10, r/10+1),
                            PS_CUT_X_POS,
                            stat);
        ok(out == NULL, "psImageSlice() returned NULL with unallowed numCols");
    
        // Following should be an error
        // XXX: Verify error
        out = psImageSlice(out, NULL,
                            image,
                            mask, 1,
                            psRegionSet(c/10, c/10 + 1, r/10, r + 1),
                            PS_CUT_X_POS,
                            stat);
        ok(out == NULL, "psImageSlice() returned NULL with unallowed numRows");
    
        // Verify the returned psVector structure pointer is null and program
        // execution doesn't stop, if the input psStat structure member options is
        // zero which indicates no statistic method specified.
        // Following should be an error
        // XXX: Verify error
        stat->options = 0;
        out = psImageSlice(out, NULL,
                            image,
                            mask, 1,
                            psRegionSet(c/10, c/10 + 1, r/10, r/10+1),
                            PS_CUT_X_POS,
                            stat);
        ok(out == NULL, "psImageSlice() returned NULL with unallowed numRows");
    
        // Verify that a mask of different size than the input image returns null and program
        // execution doesn't stop.
        // Following should be an error mask size != image size
        // XXX: Verify error
        stat->options = PS_STAT_SAMPLE_MEDIAN;
        psImage* maskSz = psImageAlloc(r, c, PS_TYPE_MASK);
        out = psImageSlice(out, NULL,
                            image,
                            maskSz, 1,
                            psRegionSet(c/10, c/10 + 1, r/10, r/10 + 1),
                            PS_CUT_X_POS,
                            stat);
        ok(out == NULL, "psImageSlice() returned NULL with differing mask size and image size");
    
        // Verify the a unallowed type mask returns null and program execution doesn't stop.
        // Following should be an error unallowed mask type
        // XXX: Verify error
        psImage* maskS8 = psImageAlloc(c, r, PS_TYPE_S8);
        out =  psImageSlice(out, NULL,
                             image,
                             maskS8, 1,
                             psRegionSet(c/10, c/10 + 1, r/10, r/10 + 1),
                             PS_CUT_X_POS,
                             stat);
        ok(out == NULL, "psImageSlice() returned NULL with unallowed mask type");
    
        //Added tests after subimage changes.
        psFree(image);
        image = psImageAlloc(c, r, PS_TYPE_F64);
        for (psS32 row = 0;row < r;row++) {
            psF64 *imageRow = image->data.F64[row];
            psF64 rowOffset = row * 2;
            for (psS32 col = 0;col < c;col++) {
                imageRow[col] = col + rowOffset;
            }
        }
        P_PSIMAGE_SET_COL0(image, 1);
        P_PSIMAGE_SET_ROW0(image, 1);
        psFree(out);
        out = NULL;
        out = psImageSlice(out,positions,image,mask,1,
                           psRegionSet(1,c,1,r),PS_CUT_X_POS,stat);
        ok(out != NULL, "psImageSlice() with correct inputs");
        psFree(out);
        out = NULL;

        //Return NULL for incorrect image inputs.
        P_PSIMAGE_SET_ROW0(image, -1);
        // Following should generate error message
        // XXX: Verify error
        out = psImageSlice(out,positions,image,mask,1,
                           psRegionSet(1,c,1,r),PS_CUT_X_POS,stat);
        ok(out == NULL, "psImageSlice returned NULL for unallowed specified input");

        P_PSIMAGE_SET_COL0(image, -1);
        P_PSIMAGE_SET_ROW0(image, 1);
        // Following should generate error message
        // XXX: Verify error
        out = psImageSlice(out,positions,image,mask,1,
                           psRegionSet(1,c,1,r),PS_CUT_X_POS,stat);
        ok(out == NULL, "psImageSlice returned NULL for unallowed specified input");

        P_PSIMAGE_SET_COL0(image, -1);
        // Return NULL for incorrect region inputs.
        // Following should generate error message
        // XXX: Verify error
        out = psImageSlice(out,positions,image,mask,1,
                           psRegionSet(0,c,1,r),PS_CUT_X_POS,stat);
        ok(out == NULL, "psImageSlice returned NULL for unallowed specified input");

        // Following should generate error message
        // XXX: Verify error
        out = psImageSlice(out,positions,image,mask,1,
                           psRegionSet(1,c,1,r+1),PS_CUT_X_POS,stat);
        ok(out == NULL, "psImageSlice returned NULL for unallowed specified input");

        // Following should generate error message
        // XXX: Verify error
        out = psImageSlice(out,positions,image,mask,1,
                           psRegionSet(1,c,1,-r-2),PS_CUT_X_POS,stat);
        ok(out == NULL, "psImageSlice returned NULL for unallowed specified input");

        // Following should generate error message
        // XXX: Verify error
        out = psImageSlice(out,positions,image,mask,1,
                           psRegionSet(c,1,1,r),PS_CUT_X_POS,stat);
        ok(out == NULL, "psImageSlice returned NULL for unallowed specified input");


        // Following should generate error message
        // XXX: Verify error
        out = psImageSlice(out,positions,image,mask,1,
                           psRegionSet(1,1,1,1),PS_CUT_X_POS,stat);
        ok(out == NULL, "psImageSlice returned NULL for unallowed specified input");

    
        //Make sure that regions match appropriately...
        out = psImageSlice(out,positions,image,mask,1,
                           psRegionSet(1,-1,1,-1),PS_CUT_Y_NEG,stat);
        psVector *out2 = NULL;
        out2 = psImageSlice(out2,positions,image,mask,1,
                            psRegionSet(0,0,0,0),PS_CUT_Y_NEG,stat);
        ok(out != NULL, "psImageSlice returned non-NULL for valid inputs");
        ok(out2 != NULL, "psImageSlice returned non-NULL for valid inputs");
        skip_start(out == NULL || out2 == NULL, 2, "Skipping tests because psImageSlice() returned NULL");
        ok(out->n == out2->n, "psImageSlice returned matching vectors for equivalent inputs");
        ok(out->data.F64[out->n-1] == out2->data.F64[out2->n-1], "psImageSlice returned matching vectors for equivalent inputs");
        skip_end();
        psFree(out2);
        psFree(image);
        psFree(positions);
        psFree(mask);
        psFree(out);
        psFree(stat);
        psFree(maskS8);
        psFree(maskSz);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

/* removed 1/09 WJG
    // testImageCut()
    {
        psMemId id = psMemGetId();
        psS32 c = 300;
        psS32 r = 200;
        psS32 numPoints = 15;
        float startCol[] = { 40,150, 40,  0,280, 40,280, -1,300, 20, 20, 20, 20, 20, 20};
        float endCol[] =   {240,150,240,299, 40,240, 40,240,240, -1,300,240,240,240,240};
        float startRow[] = { 20, 10,100,  0, 20,180,180, 10, 10, 10, 10, -1,200, 10, 10};
        float endRow[] =   {160,180,100,199,160, 10, 10,180,180,180,180,180,180, -1,200};
        psBool success[] = {true,true,true,true,true,true,true,false,false,false,false,false,false,false,false};
        psU32 length = 100;
    
        psImage* image = psImageAlloc(c,r,PS_TYPE_F32);
        psImage* mask = psImageAlloc(c,r,PS_TYPE_MASK);
        for (psS32 row = 0; row < image->numRows; row++) {
            for (psS32 col = 0; col < image->numCols; col++) {
                image->data.F32[row][col] = (psF32)col + (psF32)row/1000.0f;
                if ((row & 0x0F) == 0) {
                    mask->data.PS_TYPE_MASK_DATA[row][col] = 1;
                } else {
                    mask->data.PS_TYPE_MASK_DATA[row][col] = 0;
                }
            }
        }
        psVector* rows = psVectorAlloc(length,PS_TYPE_F32);
        psVector* cols = psVectorAlloc(length,PS_TYPE_F32);
    
        psVector* result = psVectorAlloc(length,PS_TYPE_F32);
        for (psS32 n = 0; n < numPoints; n++) {
            psVector* orig = result;
            if (! success[n]) {
                // The following should be an error
                // XXX: Verify error
            }
            if (n == 1) {
                result = psImageCut(result,
                                    cols,rows,
                                    image,
                                    NULL,0,
                                    psRegionSet(startCol[n], endCol[n], startRow[n],endRow[n]),
                                    length,
                                    PS_INTERPOLATE_FLAT);
            } else {
                result = psImageCut(result,
                                    cols,rows,
                                    image,
                                    mask,1,
                                    psRegionSet(startCol[n], endCol[n], startRow[n], endRow[n]),
                                    length,
                                    PS_INTERPOLATE_FLAT);
            }

            bool errorFlag = false;    
            if (success[n]) {
                ok(result != NULL, "psImageCut returned non-NULL");
                ok(orig != NULL, "orig image is non-NULL");
                ok(orig == result, "psImageCut did recycle the out parameter properly");
                psImageInterpolateOptions *tmpIntOptsNoMask = psImageInterpolateOptionsAlloc(
                    PS_INTERPOLATE_FLAT, image, NULL, NULL, 0, 0, NAN, 0, 0, 0.0);
                psImageInterpolateOptions *tmpIntOptsMask = psImageInterpolateOptionsAlloc(
                    PS_INTERPOLATE_FLAT, image, NULL, NULL, 1, 0, NAN, 0, 0, 0.0);
                double imgVal;
                double varVal;
                psMaskType maskVal;
    
                float deltaRow = (endRow[n]-startRow[n])/(length-1);
                float deltaCol = (endCol[n]-startCol[n])/(length-1);
                psF32 truth;
                for (psS32 i = 0; i < length; i++) {
                    float x = (float)startCol[n]+(float)i*deltaCol;
                    float y = (float)startRow[n]+(float)i*deltaRow;
                    if (n == 1) {
//                        truth = psImagePixelInterpolate(image, x, y,
//                                                         NULL,0,0,PS_INTERPOLATE_FLAT);
                          psImageInterpolate(&imgVal, &varVal, &maskVal, x, y, tmpIntOptsNoMask);
                          truth = imgVal;
                    } else {
//                        truth = psImagePixelInterpolate(image, x, y,
//                                                         mask,1,0,PS_INTERPOLATE_FLAT);
                          psImageInterpolate(&imgVal, &varVal, &maskVal, x, y, tmpIntOptsMask);
                          truth = imgVal;
                    }
                    if (fabs(result->data.F32[i]-truth) > FLT_EPSILON) {
                        diag("Bad result in position %d; Found %g but expected %g.",
                              i, result->data.F32[i], truth);
                        errorFlag = true;
                    }
                    if (fabsf(x - cols->data.F32[i]) > FLT_EPSILON ||
                            fabsf(y - rows->data.F32[i]) > FLT_EPSILON) {
                        diag("Bad resulting col/row at index %d; Found (%g,%g) but expected (%g,%g).",
                              i, cols->data.F32[i], rows->data.F32[i], x, y);
                        errorFlag = true;
                    }
                }
                psFree(tmpIntOptsNoMask);
                psFree(tmpIntOptsMask);
            } else {
                if (result != NULL) {
                    diag("psImageCut did not return NULL with a cut of (%g,%g)->(%g,%g).",
                          startCol[n],startRow[n],endCol[n],endRow[n]);
                    errorFlag = true;
                }
                psErr* err = psErrorLast();
                if (err->code != PS_ERR_BAD_PARAMETER_VALUE) {
                    diag("psImageCut did not generate proper error message.");
                    errorFlag = true;
                }
                psFree(err);
            }
            ok(!errorFlag, "psImageCut() produced the correct data values");
        }

        // Following should be an error (NULL image)
        // XXX: Verify error
        result = psImageCut(result,
                            cols,rows,
                            NULL,
                            mask,1,
                            psRegionSet(startCol[0], endCol[0], startRow[0], endRow[0]),
                            length,
                            PS_INTERPOLATE_FLAT);
        ok(result == NULL, "psImageCut did return NULL given NULL image");
        psErr* err = psErrorLast();
        ok(err->code == PS_ERR_BAD_PARAMETER_NULL, 
          "psImageCut did generate proper error message given NULL image");
        psFree(err);
    
        // Following should be an error (length=0)
        // XXX: Verify error
        result = psImageCut(result,
                            cols,rows,
                            image,
                            mask,1,
                            psRegionSet(startCol[0], endCol[0], startRow[0], endRow[0]),
                            0,
                            PS_INTERPOLATE_FLAT);
        ok(result == NULL, "psImageCut did return NULL given length=0");
        err = psErrorLast();
        ok(err->code == PS_ERR_BAD_PARAMETER_VALUE,
          "psImageCut did generate proper error message given length=0");
        psFree(err);
    
        psFree(result);
        psFree(image);
        psFree(mask);
        psFree(rows);
        psFree(cols);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
*/

    // test psImageRadialCut()
    {
        psMemId id = psMemGetId();
        psS32 c = 300;
        psS32 r = 200;
        psS32 centerX = c/2;
        psS32 centerY = r/2;
        psErr* err = NULL;
    
        psImage* image = psImageAlloc(c,r,PS_TYPE_F32);
        psImage* mask = psImageAlloc(c,r,PS_TYPE_MASK);
        for (psS32 row = 0; row < image->numRows; row++) {
            for (psS32 col = 0; col < image->numCols; col++) {
                image->data.F32[row][col] = sqrtf((col-centerX)*(col-centerX)+(row-centerY)*(row-centerY));
                if ((row & 0x0F) == 0) {
                    mask->data.PS_TYPE_MASK_DATA[row][col] = 1;
                } else {
                    mask->data.PS_TYPE_MASK_DATA[row][col] = 0;
                }
            }
        }
    
        psStats* stat = psStatsAlloc(PS_STAT_SAMPLE_MEAN);
        psVector* radii = psVectorAlloc(10,PS_TYPE_F32);
        for (psS32 i=0; i < 10; i++) {
            radii->data.F32[i] = 10+i*10;
        }
    
        bool errorFlag = false;    
        psVector* result = psVectorAlloc(10,PS_TYPE_F32);
        if (1) {
            result = psImageRadialCut(result,image,mask,1,centerX,centerY,radii,stat);
            ok(result != NULL, "psImageRadialCut returned non-NULL");
            ok(result->type.type == PS_TYPE_F64, "psImageRadialCut produced the correct type");
            for (psS32 i=0; i < 9; i++) {
                if (fabs(result->data.F64[i] - (15.0+i*10)) > 1) {
                    diag("Result was not as expected for radii #%d (%g, expected %d +/- 1)",
                          result->data.F64[i], (15.0+i*10));
                    errorFlag = true;
                }
            }
            ok(!errorFlag, "psImageRadialCut() produced the correct data values");
        }    
        // again, but without mask
        psVector* orig = result;
        if (1) {
            result = psImageRadialCut(result,image,NULL,1,centerX,centerY,radii,stat);
            ok(result != NULL, "psImageRadialCut returned non-NULL");
            ok(result == orig, "psImageRadialCut recycle output parameter");
    
            errorFlag = false;    
            for (psS32 i=0; i < 9; i++) {
                if (fabs(result->data.F64[i] - (15.0+i*10)) > 1) {
                    diag("Result was not as expected for radii #%d (%g, expected %d +/- 1)",
                          result->data.F64[i], (15.0+i*10));
                    errorFlag = true;
                }
            }
            ok(!errorFlag, "psImageRadialCut() produced the correct data values");
        }    

    
        // NULL input image...
        // Following should be an error
        // XXX: Verify error
        result = psImageRadialCut(result,NULL,NULL,1,centerX,centerY,radii,stat);
        ok(result == NULL, "psImageRadialCut returned NULL with NULL input");
        err = psErrorLast();
        ok(err->code == PS_ERR_BAD_PARAMETER_NULL, 
          "psImageRadialCut did generate proper error message");
        psFree(err);
    

        // NULL input radii...
        // Following should be an error
        // XXX: Verify error
        result = psImageRadialCut(result,image,mask,1,centerX,centerY,NULL,stat);
        ok(result == NULL, "psImageRadialCut returned NULL with NULL input radii");
        err = psErrorLast();
        ok(err->code == PS_ERR_BAD_PARAMETER_NULL,
          "psImageRadialCut did generate proper error message");
        psFree(err);

    
        // NULL input stat...
        // Following should be an error
        // XXX: Verify error
        result = psImageRadialCut(result,image,mask,1,centerX,centerY,radii,NULL);
        ok(result == NULL, "psImageRadialCut returned NULL with NULL input stats");
        err = psErrorLast();
        ok(err->code == PS_ERR_BAD_PARAMETER_NULL,
          "psImageRadialCut did generate proper error message");
        psFree(err);
    

        // Bad center X
        // Following should be an error
        // XXX: Verify error
        result = psImageRadialCut(result,image,mask,1,
                                  c+1,centerY,radii,stat);
        ok(result == NULL, "psImageRadialCut returned NULL with bad center X");
        err = psErrorLast();
        ok(err->code == PS_ERR_BAD_PARAMETER_VALUE,
          "psImageRadialCut did generate proper error message");
        psFree(err);

    
        // Bad center Y
        // Following should be an error
        // XXX: Verify error
        result = psImageRadialCut(result,image,mask,1,
                                  centerX,r+1,radii,stat);
        ok(result == NULL, "psImageRadialCut returned NULL with bad center Y");
        err = psErrorLast();
        ok(err->code == PS_ERR_BAD_PARAMETER_VALUE,
          "psImageRadialCut did generate proper error message");
        psFree(err);

    
        // Bad mask type (N.B., swapped image/mask to do this)
        // Following should be an error
        // XXX: Verify error
        result = psImageRadialCut(result,mask,image,1,
                                  centerX,r+1,radii,stat);
        ok(result == NULL, "psImageRadialCut returned NULL with bad mask type");
        err = psErrorLast();
        ok(err->code == PS_ERR_BAD_PARAMETER_TYPE,
          "psImageRadialCut did generate proper error message");
        psFree(err);

    
        // Bad mask size
        psImage* mask2 = psImageAlloc(c/2,r/2,PS_TYPE_MASK);
        // Following should be an error
        // XXX: Verify error
        result = psImageRadialCut(result,image, mask2, 1,
                                  centerX,centerY,radii,stat);
        psFree(mask2);
        ok(result == NULL, "psImageRadialCut returned NULL with bad mask size");
        err = psErrorLast();
        ok(err->code == PS_ERR_BAD_PARAMETER_SIZE,
          "psImageRadialCut did generate proper error message");
        psFree(err);

    
        // Bad radii size
        psVector* radii2 = psVectorAlloc(1,PS_TYPE_MASK);
        // Following should be an error
        // XXX: Verify error
        result = psImageRadialCut(result,image, mask, 1,
                                  centerX,centerY,radii2,stat);
        psFree(radii2);
        ok(result == NULL, "psImageRadialCut returned NULL with bad radii size");
        err = psErrorLast();
        ok(err->code == PS_ERR_BAD_PARAMETER_SIZE,
          "psImageRadialCut did generate proper error message");
        psFree(err);

    
        // bad input stat option...
        stat->options = 0;
        // Following should be an error
        // XXX: Verify error
        result = psImageRadialCut(result,image,mask,1,centerX,centerY,radii,stat);
        stat->options = PS_STAT_SAMPLE_MEAN;
        ok(result == NULL, "psImageRadialCut returned NULL with bad input stat option");
        err = psErrorLast();
        ok(err->code == PS_ERR_BAD_PARAMETER_VALUE,
          "psImageRadialCut did generate proper error message");
        psFree(err);
    
        psFree(image);
        psFree(mask);
        psFree(radii);
        psFree(stat);
        psFree(result);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");    
    }


    // testImageRowColError()
    {
        psMemId id = psMemGetId();
        psImage *image = NULL;
        psVector *out = NULL;
        int num = 0;
    
        // Following should generate error message(for row)
        // XXX: Verify error
        out = psImageRow(NULL, NULL, num);
        ok(out == NULL, "psImageRow() returned NULL with NULL input image");

        // Following should generate error message(for col)
        // XXX: Verify error
        out = psImageCol(NULL, NULL, num);
        ok(out == NULL, "psImagecol() returned NULL with NULL input image");
        image = psImageAlloc(3, 3, PS_TYPE_F64);


        //Test for unallowed row0.
        P_PSIMAGE_SET_ROW0(image, -2);
        // Following should generate error message(for row)
        // XXX: Verify error
        out = psImageRow(NULL, image, num);
        ok(out == NULL, "psImageRow() returned NULL with unallowed row0");
        // Following should generate error message(for col)
        // XXX: Verify error
        out = psImageCol(NULL, image, num);
        ok(out == NULL, "psImageCol() returned NULL with unallowed row0");

    
        //Test for unallowed col0.
        P_PSIMAGE_SET_COL0(image, -1);
        P_PSIMAGE_SET_ROW0(image, 5);
        // Following should generate error message(for row)
        // XXX: Verify error
        out = psImageRow(NULL, image, num);
        ok(out == NULL, "psImageRow() returned NULL with unallowed col0");
        // Following should generate error message(for col)
        // XXX: Verify error
        out = psImageCol(NULL, image, num);
        ok(out == NULL, "psImageCol() returned NULL with unallowed col0");

    
        //Test for unallowed numRows
        P_PSIMAGE_SET_COL0(image, 10);
        *(int*)&(image->numRows) = -1;
        // Following should generate error message(for row)
        // XXX: Verify error
        out = psImageRow(NULL, image, num);
        ok(out == NULL, "psImageRow() returned NULL with unallowed numRows");
        // Following should generate error message(for col)
        // XXX: Verify error
        out = psImageCol(NULL, image, num);
        ok(out == NULL, "psImageCol() returned NULL with unallowed numRows");


        //Test for unallowed numCols
        *(int*)&(image->numRows) = 3;
        *(int*)&(image->numCols) = -1;
        // Following should generate error message(for row)
        // XXX: Verify error
        out = psImageRow(NULL, image, num);
        ok(out == NULL, "psImageRow() returned NULL with unallowed numCols");
        // Following should generate error message(for col)
        // XXX: Verify error
        out = psImageCol(NULL, image, num);
        ok(out == NULL, "psImageCol() returned NULL with unallowed numCols");


        //Test for unallowed row/col number specified.
        *(int*)&(image->numCols) = 3;
        num = 8;
        // Following should generate error message(for row)
        // XXX: Verify error
        out = psImageRow(NULL, image, num);
        ok(out == NULL, "psImageRow() returned NULL with unallowed row/col number");
        num = 13;
        // Following should generate error message(for col)
        // XXX: Verify error
        out = psImageCol(NULL, image, num);
        ok(out == NULL, "psImageCol() returned NULL with unallowed row/col number");


        num = 3;
        // Following should generate error message(for row)
        // XXX: Verify error
        out = psImageRow(NULL, image, num);
        ok(out == NULL, "psImageRow() returned NULL");
        num = 8;
        // Following should generate error message(for col)
        // XXX: Verify error
        out = psImageCol(NULL, image, num);
        ok(out == NULL, "psImageCol() returned NULL");


        num = -10;
        // Following should generate error message(for row)
        // XXX: Verify error
        out = psImageRow(NULL, image, num);
        ok(out == NULL, "psImageRow() returned NULL");
        num = -14;
        // Following should generate error message(for col)
        // XXX: Verify error
        out = psImageCol(NULL, image, num);
        ok(out == NULL, "psImageCol() returned NULL");

        // Test on correct input data for several sizes    
        genericImageRowColTests(1, 8);
        genericImageRowColTests(8, 1);
        genericImageRowColTests(8, 8);
        genericImageRowColTests(8, 16);
        genericImageRowColTests(16, 8);
        psFree(image);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");    
    }

    // testImageRowColF64()
    {
        psMemId id = psMemGetId();
        psVector *rowcol = NULL;
        psVector *empty = NULL;
        psImage *image = NULL;
    
        image = psImageAlloc(3, 3, PS_TYPE_F64);
        rowcol = psVectorAlloc(3, PS_TYPE_F64);

        image->data.F64[0][0] = 666.666;
        image->data.F64[1][0] = 66.6;
        image->data.F64[2][0] = 6.66;
        image->data.F64[0][1] = 6.6;
        image->data.F64[1][1] = 6.666;
        image->data.F64[2][1] = 66.666;
        image->data.F64[0][2] = 666.6;
        image->data.F64[1][2] = 666.66;
        image->data.F64[2][2] = 66.66;
    
        //Test for error with NULL image
        empty = psImageCol(empty, NULL, 0);
        ok(empty == NULL, "psImageCol returned NULL for NULL image input");
        //Test for error with Out of Range Row
        empty = psImageRow(empty, image, 5);
        ok(empty == NULL, "psImageRow returned NULL for out of range row input");

        rowcol->data.F64[0] = 1.1;
        rowcol->data.F64[2] = 2.2;
        //Test recycling of non-NULL vector & correct output
        rowcol = psImageCol(rowcol, image, 1);
        double test1, test2;
        double TOLTST = .001;
        test1 = fabs(rowcol->data.F64[0]-6.6);
        test2 = fabs(rowcol->data.F64[2]-66.666);
        ok((test1<=TOLTST) && (test2<=TOLTST), "psImageCol returned correct values");

        rowcol = psImageRow(rowcol, image, 1);
        test1 = fabs(rowcol->data.F64[0]-66.6);
        test2 = fabs(rowcol->data.F64[2]-666.66);
        ok(!((test1>TOLTST) || (test2>TOLTST)),
           "psImageRow returned correct values");
        psFree(rowcol);
        psFree(image);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // testImageRowColF32()
    {
        psMemId id = psMemGetId();
        psVector *rowcol = NULL;
        psVector *empty = NULL;
        psImage *image = NULL;
        psImage *emptyImage = NULL;
    
        float test1;
        float test2;
        float TOLTST = .01;
    
        image = psImageAlloc(3, 3, PS_TYPE_F32);
        rowcol = psVectorAlloc(3, PS_TYPE_F32);
        rowcol->n = rowcol->nalloc;
    
        image->data.F32[0][0] = 666.666;
        image->data.F32[1][0] = 66.6;
        image->data.F32[2][0] = 6.66;
        image->data.F32[0][1] = 6.6;
        image->data.F32[1][1] = 6.666;
        image->data.F32[2][1] = 66.666;
        image->data.F32[0][2] = 666.6;
        image->data.F32[1][2] = 666.66;
        image->data.F32[2][2] = 66.66;
    
        //Test for error with NULL image
        empty = psImageCol(empty, emptyImage, 0);
        ok(empty == NULL, "psImageCol returned NULL for NULL image input");
        //Test for error with Out of Range Row
        empty = psImageRow(empty, image, 5);
        ok(empty == NULL, "psImageRow returned NULL for out of range row input");

        rowcol->data.F32[0] = 1.1;
        rowcol->data.F32[2] = 2.2;
        //Test recycling of non-NULL vector & correct output
        rowcol = psImageCol(rowcol, image, 1);
        test1 = fabs(rowcol->data.F32[0]-6.6);
        test2 = fabs(rowcol->data.F32[2]-66.666);
        ok((test1<=TOLTST) && (test2<=TOLTST), "psImageCol returned correct values");
    
        psFree(rowcol);
        psFree(image);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // testImageRowColU64()
    {
        psMemId id = psMemGetId();
        psVector *rowcol = NULL;
        psVector *empty = NULL;
        psImage *image = NULL;
        psImage *emptyImage = NULL;
    
        image = psImageAlloc(3, 3, PS_TYPE_U64);
        rowcol = psVectorAlloc(3, PS_TYPE_U64);
    
        image->data.U64[0][0] = 1;
        image->data.U64[1][0] = 2;
        image->data.U64[2][0] = 3;
        image->data.U64[0][1] = 4;
        image->data.U64[1][1] = 5;
        image->data.U64[2][1] = 6;
        image->data.U64[0][2] = 7;
        image->data.U64[1][2] = 8;
        image->data.U64[2][2] = 9;
    
        //Test for error with NULL image
        empty = psImageCol(empty, emptyImage, 0);
        ok(empty == NULL, "psImageCol returned NULL for NULL image input");
        //Test for error with Out of Range Row
        empty = psImageRow(empty, image, 5);
        ok(empty == NULL, "psImageRow returned NULL for out of range row input");

        //Test recycling of non-NULL vector & correct output
        rowcol = psImageCol(rowcol, image, 1);
        ok((rowcol->data.U64[0] == 4) && (rowcol->data.U64[2] == 6),
          "psImageCol returned correct values (%u %u)",
           rowcol->data.U64[0], rowcol->data.U64[2]);
        psFree(rowcol);
        psFree(image);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // testImageRowColU32()
    {
        psMemId id = psMemGetId();
        psVector *rowcol = NULL;
        psVector *empty = NULL;
        psImage *image = NULL;
        psImage *emptyImage = NULL;
    
        image = psImageAlloc(3, 3, PS_TYPE_U32);
        rowcol = psVectorAlloc(3, PS_TYPE_U32);
    
        image->data.U32[0][0] = 1;
        image->data.U32[1][0] = 2;
        image->data.U32[2][0] = 3;
        image->data.U32[0][1] = 4;
        image->data.U32[1][1] = 5;
        image->data.U32[2][1] = 6;
        image->data.U32[0][2] = 7;
        image->data.U32[1][2] = 8;
        image->data.U32[2][2] = 9;
    
        //Test for error with NULL image
        empty = psImageCol(empty, emptyImage, 0);
        ok(empty == NULL, "psImageCol returned NULL for NULL image input");
        //Test for error with Out of Range Row
        empty = psImageRow(empty, image, 5);
        ok(empty == NULL, "psImageRow returned NULL for out of range row input");

        //Test recycling of non-NULL vector & correct output
        rowcol = psImageCol(rowcol, image, 1);
        ok(rowcol->data.U32[0] == 4 && rowcol->data.U32[2] == 6,
          "psImageCol returned correct values (%d %d)",
          rowcol->data.U32[0] == 4, rowcol->data.U32[2]);
    
        psFree(rowcol);
        psFree(image);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // testImageRowColS32()
    {
        psMemId id = psMemGetId();
        psVector *rowcol = NULL;
        psVector *empty = NULL;
        psImage *image = NULL;
        psImage *emptyImage = NULL;
    
        image = psImageAlloc(3, 3, PS_TYPE_S32);
        rowcol = psVectorAlloc(3, PS_TYPE_S32);
    
        image->data.S32[0][0] = 1;
        image->data.S32[1][0] = 2;
        image->data.S32[2][0] = 3;
        image->data.S32[0][1] = 4;
        image->data.S32[1][1] = 5;
        image->data.S32[2][1] = 6;
        image->data.S32[0][2] = 7;
        image->data.S32[1][2] = 8;
        image->data.S32[2][2] = 9;

        //Test for error with NULL image
        empty = psImageCol(empty, emptyImage, 0);
        ok(empty == NULL, "psImageCol returned NULL for NULL image input");
        //Test for error with Out of Range Row
        empty = psImageRow(empty, image, 5);
        ok(empty == NULL, "psImageRow returned NULL for out of range row input");

        //Test recycling of non-NULL vector & correct output
        rowcol = psImageCol(rowcol, image, 1);
        ok(rowcol->data.S32[0] == 4 && rowcol->data.S32[2] == 6,
          "psImageCol returned correct values (%d %d)",
           rowcol->data.S32[0], rowcol->data.S32[2]);
        psFree(rowcol);
        psFree(image);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // testImageRowColS64()
    {
        psMemId id = psMemGetId();
        psVector *rowcol = NULL;
        psVector *empty = NULL;
        psImage *image = NULL;
        psImage *emptyImage = NULL;
    
        image = psImageAlloc(3, 3, PS_TYPE_S64);
        rowcol = psVectorAlloc(3, PS_TYPE_S64);
    
        image->data.S64[0][0] = 1;
        image->data.S64[1][0] = 2;
        image->data.S64[2][0] = 3;
        image->data.S64[0][1] = 4;
        image->data.S64[1][1] = 5;
        image->data.S64[2][1] = 6;
        image->data.S64[0][2] = 7;
        image->data.S64[1][2] = 8;
        image->data.S64[2][2] = 9;
    
        //Test for error with NULL image
        empty = psImageCol(empty, emptyImage, 0);
        ok(empty == NULL, "psImageCol returned NULL for NULL image input");
        //Test for error with Out of Range Row
        empty = psImageRow(empty, image, 5);
        ok(empty == NULL, "psImageRow returned NULL for out of range row input");

        //Test recycling of non-NULL vector & correct output
        rowcol = psImageCol(rowcol, image, 1);
        ok(rowcol->data.S64[0] == 4 && rowcol->data.S64[2] == 6,
          "psImageCol returned correct values");
        psFree(rowcol);
        psFree(image);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // testImageRowColS16()
    {
        psMemId id = psMemGetId();
        psVector *rowcol = NULL;
        psVector *empty = NULL;
        psImage *image = NULL;
        psImage *emptyImage = NULL;
    
        image = psImageAlloc(3, 3, PS_TYPE_S16);
        rowcol = psVectorAlloc(3, PS_TYPE_S16);
    
        image->data.S16[0][0] = 1;
        image->data.S16[1][0] = 2;
        image->data.S16[2][0] = 3;
        image->data.S16[0][1] = 4;
        image->data.S16[1][1] = 5;
        image->data.S16[2][1] = 6;
        image->data.S16[0][2] = 7;
        image->data.S16[1][2] = 8;
        image->data.S16[2][2] = 9;
    
        //Test for error with NULL image
        empty = psImageCol(empty, emptyImage, 0);
        ok(empty == NULL, "psImageCol returned NULL for NULL image input");
        //Test for error with Out of Range Row
        empty = psImageRow(empty, image, 5);
        ok(empty == NULL, "psImageRow returned NULL for out of range row input");

        //Test recycling of non-NULL vector & correct output
        rowcol = psImageCol(rowcol, image, 1);
        ok(rowcol->data.S16[0] == 4 && rowcol->data.S16[2] == 6,
           "psImageRow returned correct values");
        psFree(rowcol);
        psFree(image);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // testImageRowColU16()
    {
        psMemId id = psMemGetId();
        psVector *rowcol = NULL;
        psVector *empty = NULL;
        psImage *image = NULL;
        psImage *emptyImage = NULL;
    
        image = psImageAlloc(3, 3, PS_TYPE_U16);
        rowcol = psVectorAlloc(3, PS_TYPE_U16);
    
        image->data.U16[0][0] = 1;
        image->data.U16[1][0] = 2;
        image->data.U16[2][0] = 3;
        image->data.U16[0][1] = 4;
        image->data.U16[1][1] = 5;
        image->data.U16[2][1] = 6;
        image->data.U16[0][2] = 7;
        image->data.U16[1][2] = 8;
        image->data.U16[2][2] = 9;
    
        //Test for error with NULL image
        empty = psImageCol(empty, emptyImage, 0);
        ok(empty == NULL, "psImageCol returned NULL for NULL image input");
        //Test for error with Out of Range Row
        empty = psImageRow(empty, image, 5);
        ok(empty == NULL, "psImageRow returned NULL for out of range row input");

        //Test recycling of non-NULL vector & correct output
        rowcol = psImageCol(rowcol, image, 1);
        ok(rowcol->data.U16[0] == 4 && rowcol->data.U16[2] == 6,
           "psImageRow returned correct values");
        psFree(rowcol);
        psFree(image);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // testImageRowColU8()
    {
        psMemId id = psMemGetId();
        psVector *rowcol = NULL;
        psVector *empty = NULL;
        psImage *image = NULL;
        psImage *emptyImage = NULL;
    
        image = psImageAlloc(3, 3, PS_TYPE_U8);
        rowcol = psVectorAlloc(3, PS_TYPE_U8);
    
        image->data.U8[0][0] = 1;
        image->data.U8[1][0] = 2;
        image->data.U8[2][0] = 3;
        image->data.U8[0][1] = 4;
        image->data.U8[1][1] = 5;
        image->data.U8[2][1] = 6;
        image->data.U8[0][2] = 7;
        image->data.U8[1][2] = 8;
        image->data.U8[2][2] = 9;
    
        //Test for error with NULL image
        empty = psImageCol(empty, emptyImage, 0);
        ok(empty == NULL, "psImageCol returned NULL for NULL image input");
        //Test for error with Out of Range Row
        empty = psImageRow(empty, image, 5);
        ok(empty == NULL, "psImageRow returned NULL for out of range row input");

        //Test recycling of non-NULL vector & correct output
        rowcol = psImageCol(rowcol, image, 1);
        ok(rowcol->data.U8[0] == 4 && rowcol->data.U8[2] == 6,
           "psImageRow returned correct values");
        psFree(rowcol);
        psFree(image);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // testImageRowColS8()
    {
        psMemId id = psMemGetId();
        psVector *rowcol = NULL;
        psVector *empty = NULL;
        psImage *image = NULL;
        psImage *emptyImage = NULL;
    
        image = psImageAlloc(3, 3, PS_TYPE_S8);
        rowcol = psVectorAlloc(3, PS_TYPE_S8);
    
        image->data.S8[0][0] = 1;
        image->data.S8[1][0] = 2;
        image->data.S8[2][0] = 3;
        image->data.S8[0][1] = 4;
        image->data.S8[1][1] = 5;
        image->data.S8[2][1] = 6;
        image->data.S8[0][2] = 7;
        image->data.S8[1][2] = 8;
        image->data.S8[2][2] = 9;

        //Test for error with NULL image
        empty = psImageCol(empty, emptyImage, 0);
        ok(empty == NULL, "psImageCol returned NULL for NULL image input");
        //Test for error with Out of Range Row
        empty = psImageRow(empty, image, 5);
        ok(empty == NULL, "psImageRow returned NULL for out of range row input");

        //Test recycling of non-NULL vector & correct output
        rowcol = psImageCol(rowcol, image, 1);
        ok(rowcol->data.S8[0] == 4 && rowcol->data.S8[2] == 6,
           "psImageRow returned correct values");
        psFree(rowcol);
        psFree(image);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
}
