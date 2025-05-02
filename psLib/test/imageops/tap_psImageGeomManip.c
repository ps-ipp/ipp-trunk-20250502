/** @file  tst_psImageGeomManip.c
 *
 *  @brief Contains the tests for psImageManip.[ch]
 *
 *  @author Robert DeSonia, MHPCC
 *
 *  @version $Revision: 1.10 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-01-26 21:26:52 $
 *
 *  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 */
#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include "tap.h"
#include "pstap.h"
#define VERBOSE 0

void genericImageRollTest(int numRows, int numCols)
{
    psMemId id = psMemGetId();
    psImage *in;
    psImage *out;
    psImage *out2;

    // The function psImageRoll shall generate a new psImage structure by
    // rolling the input image the correponding number of pixels in the vertical
    // and/or horizontal direction. The image output image shall be the same size
    // as the input image. Values which roll off the image are wrapped to the
    // other side.
    //
    // Verify the returned psImage structure contains expected values, if the
    // input image contains known values and the roll performed is known.
    // Cases should include no roll, vertical roll, horizontal roll and
    // combination vertical/horizontal rolls. Positive and negative rolls
    // should be performed.

    in = psImageAlloc(numCols,numRows,PS_TYPE_F32);
    for (psS32 row=0;row<numRows;row++) {
        for (psS32 col=0;col<numCols;col++) {
            in->data.F32[row][col] = (psF32)row+(psF32)col/1000.0f;
        }
    }

    out = psImageRoll(NULL,in,0,0);
    bool errorFlag = false;
    for (psS32 row=0;row<numRows;row++) {
        psF32 *inRow = in->data.F32[row];
        psF32 *outRow = out->data.F32[row];
        for (psS32 col=0;col<numCols;col++) {
            if (inRow[col] != outRow[col]) {
                diag("psImageRoll didn't produce expected result "
                     "at %d,%d (%f vs %f) for dx=0, dy=0",
                     col,row,inRow[col],outRow[col]);
                errorFlag = true;
            }
        }
    }
    ok(!errorFlag, "psImageRoll() produced the correct data values (no roll)");

    errorFlag = false;
    out2 = psImageRoll(out,in,numCols/4,0);
    for (psS32 row=0;row<numRows;row++)
    {
        psF32 *inRow = in->data.F32[row];
        psF32 *outRow = out->data.F32[row];
        for (psS32 col=0;col<numCols;col++) {
            if (inRow[(col+numCols/4) % numCols] != outRow[col]) {
                diag("psImageRoll didn't produce expected result "
                     "at %d,%d (%f vs %f) for dx=numCols/4, dy=0",
                     col,row,inRow[(col+numCols/4) % numCols],outRow[col]);
                errorFlag = true;
            }
        }
    }
    ok(!errorFlag, "psImageRoll() produced the correct data values (column roll only");

    // Verify the returned psImage structure pointer is equal to the input
    // parameter out if provided.
    ok(out2 == out, "psImageRoll did recycle the out psImage");

    errorFlag = false;
    out = psImageRoll(out,in,0,numRows/4);
    for (psS32 row=0;row<numRows;row++)
    {
        psF32 *inRow = in->data.F32[(row+numRows/4)%numRows];
        psF32 *outRow = out->data.F32[row];
        for (psS32 col=0;col<numCols;col++) {
            if (inRow[col] != outRow[col]) {
                diag("psImageRoll didn't produce expected result "
                     "at %d,%d (%f vs %f) for dx=0, dy=numRows/4",
                     col,row,inRow[col],outRow[col]);
                errorFlag = true;
            }
        }
    }
    ok(!errorFlag, "psImageRoll() produced the correct data values (row roll onlt)");

    errorFlag = false;
    out = psImageRoll(out,in,numCols/4,numRows/4);
    for (psS32 row=0;row<numRows;row++)
    {
        psF32 *inRow = in->data.F32[(row+numRows/4)%numRows];
        psF32 *outRow = out->data.F32[row];
        for (psS32 col=0;col<numCols;col++) {
            if (inRow[(col+numCols/4) % numCols] != outRow[col]) {
                diag("psImageRoll didn't produce expected result "
                     "at %d,%d (%f vs %f) for dx=numCols/4, dy=numRows/4",
                     col,row,inRow[(col+numCols/4) % numCols],outRow[col]);
                errorFlag = true;
            }
        }
    }
    ok(!errorFlag, "psImageRoll() produced the correct data values (column and row roll)");

    errorFlag = false;
    out = psImageRoll(out,in,-numCols/4,0);
    for (psS32 row=0;row<numRows;row++)
    {
        psF32 *inRow = in->data.F32[row];
        psF32 *outRow = out->data.F32[row];
        for (psS32 col=0;col<numCols;col++) {
            if (inRow[(col+(numCols-numCols/4)) % numCols] != outRow[col]) {
                diag("psImageRoll didn't produce expected result "
                     "at %d,%d (%f vs %f) for dx=-numCols/4, dy=0",
                     col,row,inRow[(col+(numCols-numCols/4)) % numCols],outRow[col]);
                errorFlag = true;
            }
        }
    }
    ok(!errorFlag, "psImageRoll() produced the correct data values (negative column roll)");

    errorFlag = false;
    out = psImageRoll(out,in,0,-numRows/4);
    for (psS32 row=0;row<numRows;row++)
    {
        psF32 *inRow = in->data.F32[(row+numRows-numRows/4)%numRows];
        psF32 *outRow = out->data.F32[row];
        for (psS32 col=0;col<numCols;col++) {
            if (inRow[col] != outRow[col]) {
                diag("psImageRoll didn't produce expected result "
                     "at %d,%d (%f vs %f) for dx=0, dy=-numRows/4",
                     col,row,inRow[col],outRow[col]);
                errorFlag = true;
            }
        }
    }
    ok(!errorFlag, "psImageRoll() produced the correct data values (negative row roll)");

    errorFlag = false;
    out = psImageRoll(out,in,-numCols/4,-numRows/4);
    for (psS32 row=0;row<numRows;row++)
    {
        psF32 *inRow = in->data.F32[(row+numRows-numRows/4)%numRows];
        psF32 *outRow = out->data.F32[row];
        for (psS32 col=0;col<numCols;col++) {
            if (inRow[(col+numCols-numCols/4) % numCols] != outRow[col]) {
                diag("psImageRoll didn't produce expected result "
                     "at %d,%d (%f vs %f) for dx=numCols/4, dy=numRows/4",
                     col,row,inRow[(col+numCols-numCols/4) % numCols],outRow[col]);
                errorFlag = true;
            }
        }
    }
    ok(!errorFlag, "psImageRoll() produced the correct data values (negative column and row roll)");
    psFree(in);
    psFree(out);
    ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
}


/* removed 1/09 WJG
bool testImageShiftCase(psS32 cols,
                        psS32 rows,
                        float colShift,
                        float rowShift)
{
    psImage *fOut = NULL;
    psImage *sOut = NULL;
    psImage *fImg = psImageAlloc(cols,rows,PS_TYPE_F32);
    psImage *sImg = psImageAlloc(cols,rows,PS_TYPE_S16);
    psImage *fBiOut = psImageAlloc(cols,rows,PS_TYPE_F32);
    psImage *sBiOut = psImageAlloc(cols,rows,PS_TYPE_S16);
    bool errorFlag = false;

    for(psS32 row=0;row<rows;row++) {
        psF32 *fRow = fImg->data.F32[row];
        psS16 *sRow = sImg->data.S16[row];
        for (psS32 col=0;col<cols;col++) {
            fRow[col] = (psF32)(row)+(psF32)(col)/100.0f;
            sRow[col] = row-2*col;
        }
    }

    fOut = psImageShift(fOut, fImg, colShift, rowShift, NAN, PS_INTERPOLATE_FLAT);
    sOut = psImageShift(sOut, sImg, colShift, rowShift, -1, PS_INTERPOLATE_FLAT);
    fBiOut = psImageShift(fBiOut, fImg, colShift, rowShift, NAN, PS_INTERPOLATE_BILINEAR);
    sBiOut = psImageShift(sBiOut, sImg, colShift, rowShift, -1, PS_INTERPOLATE_BILINEAR);

    {
        psImageInterpolateOptions *tmpIntOpts = psImageInterpolateOptionsAlloc(
            PS_INTERPOLATE_FLAT, fImg, NULL, NULL, 0, NAN, NAN, 0, 0, 0.0);
        double imgVal;
        double varVal;
        psMaskType maskVal;
        for(psS32 row=0;row<rows;row++) {
            psF32 *fRow = fOut->data.F32[row];
            for (psS32 col=0;col<cols;col++) {
                psImageInterpolate(&imgVal, &varVal, &maskVal, col+0.5-colShift,
                                   row+0.5-rowShift, tmpIntOpts);
                if (fabsf(fRow[col] - imgVal) > FLT_EPSILON) {
                    if (VERBOSE) diag("Float image not shifted correctly at %d,%d (%g vs %g) (flat interpolation)",
                         row,col,fRow[col],imgVal);
                    errorFlag = true;
                }
            }
        }
        psFree(tmpIntOpts);
    }

    {
        psImageInterpolateOptions *tmpIntOpts = psImageInterpolateOptionsAlloc(
            PS_INTERPOLATE_FLAT, sImg, NULL, NULL, 0, -1, NAN, 0, 0, 0.0);
        double imgVal;
        double varVal;
        psMaskType maskVal;

        for(psS32 row=0;row<rows;row++) {
            psS16 *sRow = sOut->data.S16[row];
            for (psS32 col=0;col<cols;col++) {
                psImageInterpolate(&imgVal, &varVal, &maskVal, col+0.5-colShift,
                                   row+0.5-rowShift, tmpIntOpts);
                psS16 sValue = (psS16) imgVal;
                if (sRow[col] != sValue) {
                    if (VERBOSE) diag("Short image not shifted correctly at %d,%d (%d vs %d) (flat interpolation)",
                        row,col,sRow[col],sValue);
                    errorFlag = true;
                }
            }
        }
        psFree(tmpIntOpts);
    }


    {
        psImageInterpolateOptions *tmpIntOpts = psImageInterpolateOptionsAlloc(
            PS_INTERPOLATE_BILINEAR, fImg, NULL, NULL, 0, NAN, NAN, 0, 0, 0.0);
        double imgVal;
        double varVal;
        psMaskType maskVal;
        for(psS32 row=0;row<rows;row++) {
            psF32 *fBiRow = fBiOut->data.F32[row];
            for (psS32 col=0;col<cols;col++) {
                psImageInterpolate(&imgVal, &varVal, &maskVal, col+0.5-colShift,
                                   row+0.5-rowShift, tmpIntOpts);
                psF32 fBiValue = imgVal;
                if (fabsf(fBiRow[col] - fBiValue) > FLT_EPSILON) {
                    if (VERBOSE) diag("Float image not shifted correctly at %d,%d (%g vs %g) (bilinear interpolation)",
                         row,col,fBiRow[col],fBiValue);
                    errorFlag = true;
                }
            }
        }
        psFree(tmpIntOpts);
    }


    {
        psImageInterpolateOptions *tmpIntOpts = psImageInterpolateOptionsAlloc(
            PS_INTERPOLATE_BILINEAR, sImg, NULL, NULL, 0, -1, NAN, 0, 0, 0.0);
        double imgVal;
        double varVal;
        psMaskType maskVal;
        for(psS32 row=0;row<rows;row++) {
            psS16 *sBiRow = sBiOut->data.S16[row];
            for (psS32 col=0;col<cols;col++) {
                psImageInterpolate(&imgVal, &varVal, &maskVal, col+0.5-colShift,
                                   row+0.5-rowShift, tmpIntOpts);
                psS16 sBiValue = (psS16) imgVal;
                if (sBiRow[col] != sBiValue) {
                    if (VERBOSE) diag("Short image not shifted correctly at %d,%d (%d vs %d) (bilinear interpolation)",
                         row,col,sBiRow[col],sBiValue);
                    errorFlag = true;
                }
            }
        }
        psFree(tmpIntOpts);
    }

    if (errorFlag) {
        diag("Short or Float image not shifted correctly");
    }
    psFree(fImg);
    psFree(sImg);
    psFree(fOut);
    psFree(sOut);
    psFree(fBiOut);
    psFree(sBiOut);

    return !errorFlag;
}
*/

psS32 main(psS32 argc, char *argv[])
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    plan_tests(280);

    // test psImageRebin()
    // This function shall generate a rescaled version of a psImage
    // structure derived from a specified statistics method.
    if (1) {
        psMemId id = psMemGetId();
        psImage *in = NULL;
        psImage *out = NULL;
        psImage *out2 = NULL;
        psImage *out3 = NULL;
        psImage *mask = NULL;
        psImage *meanTruth = NULL;
        psImage *meanTruthWMask = NULL;
        psImage *maxTruth = NULL;
        psStats stats;

        //        Verify the returned psImage structure contains expected values, if the
        //        input parameter input contains known data, the input scale is a known
        //        value with a known statistical method specified in stats. Cases should
        //        include at least two different scales and statistical methods. Comparison
        //        of expected values should include a delta to allow testing on different
        //        platforms.
        #define testRebinType(DATATYPE)  \
        { \
            psMemId id = psMemGetId();\
            in = psImageAlloc(16,16,PS_TYPE_##DATATYPE); \
            mask = psImageAlloc(16,16,PS_TYPE_U8); \
            meanTruth = psImageAlloc(4,4,PS_TYPE_F32); \
            meanTruthWMask = psImageAlloc(4,4,PS_TYPE_F32); \
            maxTruth = psImageAlloc(6,6,PS_TYPE_F32); \
            memset(meanTruth->data.F32[0],0,sizeof(psF32)*4*4); \
            memset(meanTruthWMask->data.F32[0],0,sizeof(psF32)*4*4); \
            memset(maxTruth->data.F32[0],0,sizeof(psF32)*6*6); \
            for (psS32 row = 0; row<16; row++) { \
                ps##DATATYPE *inRow = in->data.DATATYPE[row]; \
                psF32 *meanTruthRow = meanTruth->data.F32[row/4]; \
                psF32 *meanTruthWMaskRow = meanTruthWMask->data.F32[row/4]; \
                psF32 *maxTruthRow = maxTruth->data.F32[row/3]; \
                psU8 *maskRow = mask->data.U8[row]; \
                for (psS32 col = 0; col<16; col++) { \
                    if(col != 15) { \
                        maskRow[col] = 0; \
                    } else { \
                        maskRow[col] = 1; \
                    } \
                    inRow[col] = row + col; \
                    meanTruthRow[col/4] += row + col; \
                    if (maxTruthRow[col/3] < row + col) { \
                        maxTruthRow[col/3] = row+col; \
                    } \
                    if(maskRow[col] == 0 ) { \
                        meanTruthWMaskRow[col/4] += row + col; \
                    } \
                } \
            } \
            for (psS32 row = 0; row<4; row++) { \
                psF32 *meanTruthRow = meanTruth->data.F32[row]; \
                psF32 *meanTruthWMaskRow = meanTruthWMask->data.F32[row]; \
                for (psS32 col = 0; col<4; col++) { \
                    meanTruthRow[col] /= 16; \
                    if ( col == 3 ) { \
                        meanTruthWMaskRow[col] /= 12; \
                    } else { \
                        meanTruthWMaskRow[col] /= 16; \
                    } \
                } \
            } \
            stats.options = PS_STAT_SAMPLE_MEAN; \
            { \
                out = psImageRebin(NULL,in,NULL,0,4,&stats); \
                ok(out != NULL, "psImageRebin returned non-NULL"); \
                ok(out->numRows == 4 && out->numCols == 4, \
                   "psImageRebin did produce the proper size image"); \
                bool errorFlag = false; \
                for (psS32 row = 0; row<4; row++) { \
                    ps##DATATYPE *outRow = out->data.DATATYPE[row]; \
                    psF32 *truthRow = meanTruth->data.F32[row]; \
                    for (psS32 col = 0; col<4; col++) { \
                        if (fabsf((float)outRow[col]-(float)truthRow[col]) > FLT_EPSILON) { \
                            diag("psImageRebin didn't produce the proper mean " \
                                 "result at (%d,%d) [%f vs %f]", \
                                 col,row,outRow[col],truthRow[col]); \
                            errorFlag = true;\
                        } \
                    } \
                } \
                ok(!errorFlag, "psImageRebin() produced the correct data"); \
            } \
            { \
                stats.options = PS_STAT_SAMPLE_MEAN; \
                out3 = psImageRebin(NULL,in,mask,1,4,&stats); \
                bool errorFlag = false; \
                for (psS32 row = 0; row<4; row++) { \
                    ps##DATATYPE *outRow = out3->data.DATATYPE[row]; \
                    psF32 *truthRow = meanTruthWMask->data.F32[row]; \
                    for ( psS32 col = 0; col<4; col++) { \
                        if(abs((psS32)outRow[col]-(psS32)truthRow[col]) > FLT_EPSILON) { \
                            diag("psImageRebin with mask didn't produce the proper mean " \
                                 "result at (%d,%d) [%f vs %f]", \
                                 col,row,outRow[col],truthRow[col]); \
                            errorFlag = true;\
                        } \
                    } \
                    ok(!errorFlag, "psImageRebin() produced the correct data"); \
                } \
            } \
            stats.options = PS_STAT_MAX; \
            { \
                out2 = psImageRebin(out,in,NULL,0,3,&stats); \
                ok(out == out2, "psImageRebin didt recycle a psImage properly"); \
                ok(out != NULL, "psImageRebin returned non-NULL"); \
                ok(out->numRows == 6 && out->numCols == 6, \
                   "psImageRebin did produce the proper size image"); \
                bool errorFlag = false; \
                for (psS32 row = 0; row<6; row++) { \
                    ps##DATATYPE *outRow = out->data.DATATYPE[row]; \
                    psF32 *truthRow = maxTruth->data.F32[row]; \
                    for (psS32 col = 0; col<6; col++) { \
                        if (fabsf((float)outRow[col]-(float)truthRow[col]) > FLT_EPSILON) { \
                            diag("psImageRebin didn't produce the proper " \
                                 "max result at (%d,%d) [%f vs %f]", \
                                 col,row,outRow[col],truthRow[col]); \
                            errorFlag = true;\
                        } \
                    } \
                } \
                ok(!errorFlag, "psImageRebin() produced the correct data"); \
            } \
            psFree(in); \
            psFree(out); \
            psFree(out3); \
            psFree(mask); \
            psFree(meanTruth); \
            psFree(meanTruthWMask); \
            psFree(maxTruth); \
            ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks"); \
        }

        testRebinType(F32);
        testRebinType(F64);
        testRebinType(U16);
        testRebinType(S8);
        // Verify the returned psImage structure is NULL and program execution
        // doesn't stop, if the input image type is not supported.
        // Following should be an error for unsupported type
        // XXX: Verify error
        in = psImageAlloc(16,16,PS_TYPE_U8);
        mask = psImageAlloc(16,16,PS_TYPE_F32);
        stats.options = PS_STAT_SAMPLE_MEAN;
        out = psImageRebin(NULL,in,NULL,0,4,&stats);
        ok(out == NULL, "psImageRebin returned NULL for unsupported type");

        // Verify the returned psImage structure is NULL and program execution
        // doesn't stop, if the mask type is not U8
        // Following should be an error for invallid mask type
        // XXX: Verify error
        out = psImageRebin(NULL,in,mask,1,4,&stats);
        ok(out == NULL, "psImageRebin returned NULL for incorrect mask");
        psFree(mask);
        psFree(in);

        // Verify the returned psImage structure is NULL and program execution
        // doesn't stop, if the input parameter input is NULL.
        out2 = psImageRebin(NULL,NULL,NULL,0,1,&stats);
        ok(out2 == NULL, "psImageRebin returned NULL with NULL input");

        // Verify the returned psImage structure is NULL and program execution
        // doesn't stop, if the input parameter scale is less than or equal to zero.
        in = psImageAlloc(16, 16, PS_TYPE_F32);
        // Following should be an error for scale < 0
        // XXX: Verify error
        out2 = psImageRebin(NULL,in,NULL,0,0,&stats);
        ok(out2 == NULL, "psImageRebin returned NULL when the scale was zero");

        // Verify the returned psImage structure is NULL and program execution
        // doesn't stop, if the input parameter stats is NULL.
        // Following should be an error for stats NULL
        // XXX: Verify error
        out2 = psImageRebin(NULL,in,NULL,0,1,NULL);
        ok(out2 == NULL, "psImageRebin returned an NULL when the stats was NULL");

        // Verify the returned psImage structure is NULL and program execution
        // doesn't stop, if the input parameter psStats structure member options
        // is zero or any value which doesn't correspond to a valid statistical
        // method.
        // Following should be an error for stats options 0
        // XXX: Verify error
        stats.options = 0;
        out2 = psImageRebin(NULL,in,NULL,0,1,&stats);
        ok(out2 == NULL, "psImageRebin returned an NULL when the stats options was zero");

        //Following should be an error for stat options use range");
        // XXX: Verify error
        stats.options = PS_STAT_USE_RANGE;
        out2 = psImageRebin(NULL,in,NULL,0,1,&stats);
        ok(out2 == NULL, "psImageRebin returned an image though the stats options was PS_STAT_USE_RANGE");

        // Verify the returned psImage structure is NULL and program execution
        // doesn't stop, if the input parameter psStats structure member options
        // specifies more than one valid statistical method.
        // Following should be an error for stats with multiple options
        // XXX: Verify error
        stats.options = PS_STAT_SAMPLE_MEAN + PS_STAT_MAX;
        out2 = psImageRebin(NULL,in,NULL,0,1,&stats);
        ok(out2 == NULL, "psImageRebin returned an NULL when the stats options was PS_STAT_SAMPLE_MEAN+PS_STAT_MAX");

        psFree(in);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // test psImageRoll()
    if (1) {
        psMemId id = psMemGetId();
        // Perform generic tests with various image sizes
        genericImageRollTest(8, 1);
        genericImageRollTest(1, 8);
        genericImageRollTest(8, 8);
        genericImageRollTest(8, 16);
        genericImageRollTest(16, 8);

        psImage *in;
        psImage *out;
        psImage *out2;
        psS32 rows = 64;
        psS32 cols = 64;
        psS32 rows1 = 8;
        psS32 cols1 = 8;
        in = psImageAlloc(cols,rows,PS_TYPE_F32);
        for (psS32 row=0;row<rows;row++) {
            for (psS32 col=0;col<cols;col++) {
                in->data.F32[row][col] = (psF32)row+(psF32)col/1000.0f;
            }
        }

        bool errorFlag = false;
        // Verify the returned psImage structure pointer is NULL and program
        // execution doesn't stop, if input parameter input is NULL.
        out2 = psImageRoll(NULL,NULL,0,0);
        if (out2 != NULL) {
            psError(PS_ERR_UNKNOWN, true,"psImageRoll did not return NULL though input image was NULL!?");
            return 2;
        }

        psFree(in);
        psFree(out);

        #define testRollType(DATATYPE) \
        in = psImageAlloc(rows1,cols1,PS_TYPE_##DATATYPE); \
        \
        for (psS32 row=0;row<rows1;row++) { \
            ps##DATATYPE *inRow = in->data.DATATYPE[row]; \
            for (psS32 col=0;col<cols1;col++) { \
                inRow[col] = (ps##DATATYPE)row+(ps##DATATYPE)col; \
            } \
        } \
        \
        errorFlag = false; \
        out = psImageRoll(NULL,in,rows1/4,cols1/4); \
        for (psS32 row=0;row<rows1;row++) { \
            ps##DATATYPE *inRow = in->data.DATATYPE[(row+rows1/4)%rows1]; \
            ps##DATATYPE *outRow = out->data.DATATYPE[row]; \
            for (psS32 col=0;col<cols1;col++) { \
                if (inRow[(col+cols1/4)%cols1] != outRow[col]) { \
                    diag("psImageRoll didn't produce expected result " \
                         "at %d,%d (%f vs %f) for dx=0, dy=0", \
                         col,row,(float)inRow[col],(float)outRow[col]); \
                    errorFlag = true; \
                } \
            } \
        } \
        psFree(in); \
        psFree(out); \
        ok(!errorFlag, "psImageRoll() produced the correct data values");

        testRollType(U8);
        testRollType(U16);
        testRollType(S8);
        testRollType(S16);
        testRollType(F64);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // test psImageRotate()
    if (1) {
        psMemId id = psMemGetId();
        // This function shall calculate a new psImage structure based upon the
        // rotation of a given psImage structure. The center of rotation shall be the
        // center pixel of the input image.
        //
        // The following steps of the testpoint are done manually via inspection of
        // fOut.fits & sOut.fits.
        //
        //  * Verify the returned psImage structure contains expected values, if
        //    the input parameter psImage contains known values. Cases should
        //    include rotations of 0, 45, 90, 135, 180, 225, 270, 315, 360 and at leat one
        //    other arbitrary angle. Cases of the input image should include image
        //    with a center pixel and an image without a center pixel.
        //  * Verify the returned psImage structure contains pixels set to exposed value, if
        //    the rotation and input psImage to not correspond to the output image.

        psS32 rows = 64;
        psS32 cols = 64;
        psImage *fOut = psImageAlloc(cols,rows,PS_TYPE_F32);
        psImage *sOut = NULL;
        psImage *fBiOut = psImageAlloc(cols,rows,PS_TYPE_F32);
        psImage *sBiOut = NULL;
        psImage *fTruth = NULL;
        psImage *sTruth = NULL;
        psImage *fBiTruth = NULL;
        psImage *sBiTruth = NULL;
        psImage *fImg = psImageAlloc(cols,rows,PS_TYPE_F32);
        psImage *sImg = psImageAlloc(cols,rows,PS_TYPE_S16);

        for(psS32 row=0;row<rows;row++) {
            psF32 *fRow = fImg->data.F32[row];
            psS16 *sRow = sImg->data.S16[row];
            for (psS32 col=0;col<cols;col++) {
                fRow[col] = (psF32)(row)+(psF32)(col)/100.0f;
                sRow[col] = row-2*col;
            }
        }

        // since interpolation is involved, etc., the simplist way to verify things
        // is to verify the results manually and bless it for automated comparison
        // thereafter

        // write results of various rotates to a file and verify with truth images
        system("mkdir temp");
        psS32 index = 0;
        psBool fail = false;
        psF32 radianRot;

        psFits *fOutFile = psFitsOpen("fOut.fits","w");
        psFits *sOutFile = psFitsOpen("sOut.fits","w");
        psFits *fBiOutFile = psFitsOpen("fBiOut.fits","w");
        psFits *sBiOutFile = psFitsOpen("sBiOut.fits","w");
        ok(fOutFile != NULL && sOutFile != NULL && fBiOutFile != NULL && sBiOutFile != NULL,
           "psFitsOpen() created the output files");

//            psFits *fTruthFile = psFitsOpen("imageops/verified/fOut.fits","r");
//            psFits *sTruthFile = psFitsOpen("imageops/verified/sOut.fits","r");
//            psFits *fBiTruthFile = psFitsOpen("imageops/verified/fBiOut.fits","r");
//            psFits *sBiTruthFile = psFitsOpen("imageops/verified/sBiOut.fits","r");
        psFits *fTruthFile = psFitsOpen("verified/fOut.fits","r");
        psFits *sTruthFile = psFitsOpen("verified/sOut.fits","r");
        psFits *fBiTruthFile = psFitsOpen("verified/fBiOut.fits","r");
        psFits *sBiTruthFile = psFitsOpen("verified/sBiOut.fits","r");
        ok(fTruthFile != NULL && sTruthFile != NULL && fBiTruthFile != NULL && sBiTruthFile != NULL,

           "psFitsOpen() opened the truth files");

        psRegion regionAll = psRegionSet(0,0,0,0);
        for (psS32 rot=-180;rot<=180;rot+=45)
        {
            psImage *oldOut = fOut;
            psImage *oldBiOut = fBiOut;
            if (rot == 90) {
                radianRot = M_PI_2;
            } else if (rot == -90) {
                radianRot = M_PI+M_PI_2;
            } else if (rot == 180 || rot == -180) {
                radianRot = M_PI;
            } else {
                radianRot = ((float)rot)*M_PI/180.0;
            }

            fOut = psImageRotate(fOut,fImg,radianRot,-1.0,PS_INTERPOLATE_FLAT);
            fBiOut = psImageRotate(fBiOut,fImg,radianRot,-1.0,PS_INTERPOLATE_BILINEAR);
            // Verify the returned psImage structure is equal to the input
            // parameter out if provided.
            ok(fOut != NULL, "psImageRotate() returned non-NULL (psImageRotate(), float image, with FLAT interpolation)");
            ok(oldOut == fOut, "psImageRotate(): the output recycle functionality was successful");
            ok(fBiOut != NULL, "psImageRotate() returned non-NULL (psImageRotate(), float image, with BILINEAR interpolation)");
            ok(oldBiOut == fBiOut, "psImageRotate(): the output recycle functionality was successful");
            sOut = psImageRotate(sOut,sImg,radianRot,-1.0,PS_INTERPOLATE_FLAT);
            ok(sOut != NULL, "psImageRotate() returned non-NULL (psImageRotate(), short image, with FLAT interpolation)");
            sBiOut = psImageRotate(sBiOut,sImg,radianRot,-1.0,PS_INTERPOLATE_BILINEAR);
            ok(sBiOut != NULL, "psImageRotate() returned non-NULL (psImageRotate(), short image, with BILINEAR interpolation)");
            ok(psFitsWriteImage(fOutFile, NULL, fOut, 1, NULL), "psFitsWriteImage() successful");
            ok(psFitsWriteImage(sOutFile, NULL, sOut, 1, NULL), "psFitsWriteImage() successful");
            ok(psFitsWriteImage(fBiOutFile, NULL, fBiOut, 1, NULL), "psFitsWriteImage() successful");
            ok(psFitsWriteImage(sBiOutFile, NULL, sBiOut, 1, NULL), "psFitsWriteImage() successful");

            // now, let's compare this with the verified file
            psFitsMoveExtNum(fTruthFile, index, false);
            psFitsMoveExtNum(sTruthFile, index, false);
            psFitsMoveExtNum(fBiTruthFile, index, false);
            psFitsMoveExtNum(sBiTruthFile, index, false);
            psFree(fTruth);
            psFree(sTruth);
            psFree(fBiTruth);
            psFree(sBiTruth);
            fTruth = NULL;
            sTruth = NULL;
            fBiTruth = NULL;
            sBiTruth = NULL;
            fTruth = psFitsReadImage(fTruthFile, regionAll, 0);
            sTruth = psFitsReadImage(sTruthFile, regionAll, 0);
            fBiTruth = psFitsReadImage(fBiTruthFile, regionAll, 0);
            sBiTruth = psFitsReadImage(sBiTruthFile, regionAll, 0);
            bool errorFlag = false;
            if (fTruth == NULL) {
                diag("verified psF32 image failed to be read (%d deg. rotation)", rot);
                errorFlag = true;
                fail = true;
            } else {
                if(fTruth->numRows != fOut->numRows || fTruth->numCols != fOut->numCols) {
                    diag("Rotated float image size did not equal truth image for %d deg rotation (%dx%d vs %dx%d)",
                         rot,fOut->numCols,fOut->numRows,fTruth->numCols,fTruth->numRows);
                    errorFlag = true;
                } else {
                    for (psS32 row=0;row<fTruth->numRows;row++) {
                        psF32 *truthRow = fTruth->data.F32[row];
                        psF32 *outRow = fOut->data.F32[row];
                        for (psS32 col=0;col<fTruth->numCols;col++) {
                            if (fabsf(truthRow[col]-outRow[col]) > 1) {
                                if (VERBOSE) diag("Float Image mismatch (%f vs %f) at %d,%d",
                                     outRow[col], truthRow[col],col,row);
                                errorFlag = true;
                            }
                        }
                    }
                }
            }
            ok(!errorFlag, "psImageRotate() produced the correct data values (%d degree rotation), float images, FLAT interpolation", rot);


            errorFlag = false;
            if (sTruth == NULL) {
                diag("verified psS16 image failed to be read (%d deg. rotation)",rot);
                errorFlag = true;
            } else {
                if (sTruth->numRows != sOut->numRows ||
                        sTruth->numCols != sOut->numCols) {
                    diag("Rotated psS16 image size did not match truth image for %d deg rotation",rot);
                    errorFlag = true;
                } else {
                    for (psS32 row=0;row<sTruth->numRows;row++) {
                        psS16 *truthRow = sTruth->data.S16[row];
                        psS16 *outRow = sOut->data.S16[row];
                        for (psS32 col=0;col<sTruth->numCols;col++) {
                            if (fabsf(truthRow[col]-outRow[col]) > 1) {
                                if (VERBOSE) diag("Short Image mismatch (%d vs %d) at %d,%d",
                                     outRow[col], truthRow[col],col,row);
                                errorFlag = true;
                            }
                        }
                    }
                }
            }
            ok(!errorFlag, "psImageRotate() produced the correct data values (%d degree rotation), short images, FLAT interpolation", rot);
            errorFlag = false;
            if (fBiTruth == NULL) {
                diag("verified psF32 Bi image failed to be read (%d deg. rotation)",
                     rot);
                errorFlag = true;
            } else {
                if (fBiTruth->numRows != fBiOut->numRows || fBiTruth->numCols != fBiOut->numCols) {
                    diag("Rotated float image size did not match truth "
                         "image for %d deg rotation (%dx%d vs %dx%d). BILINEAR",
                         rot,fBiOut->numCols,fBiOut->numRows,fBiTruth->numCols,fBiTruth->numRows);
                    errorFlag = true;
                } else {
                    for (psS32 row=0;row<fBiTruth->numRows;row++) {
                        psF32 *truthRow = fBiTruth->data.F32[row];
                        psF32 *outRow = fBiOut->data.F32[row];
                        for (psS32 col=0;col<fBiTruth->numCols;col++) {
                            if (fabsf(truthRow[col]-outRow[col]) > 1) {
                                if (VERBOSE) diag("Float Image mismatch (%f vs %f) at %d,%d. BILINEAR",
                                     outRow[col], truthRow[col],col,row);
                                errorFlag = true;
                            }
                        }
                    }
                }
            }
            ok(!errorFlag, "psImageRotate() produced the correct data values (%d degree rotation), float images, BILINEAR interpolation", rot);
            if (sBiTruth == NULL) {
                diag("verified psS16 image failed to be read "
                     "(%d deg. rotation) BILINEAR",rot);
                errorFlag = true;
                fail = true;
            } else {
                if (sBiTruth->numRows != sBiOut->numRows ||
                        sBiTruth->numCols != sBiOut->numCols) {
                    diag("Rotated psS16 image size did not match truth "
                         "image for %d deg rotation. BILINEAR",rot);
                    errorFlag = true;
                } else {
                    for (psS32 row=0;row<sBiTruth->numRows;row++) {
                        psS16 *truthRow = sBiTruth->data.S16[row];
                        psS16 *outRow = sBiOut->data.S16[row];
                        for (psS32 col=0;col<sBiTruth->numCols;col++) {
                            if (fabsf(truthRow[col]-outRow[col]) > 1) {
                                if (VERBOSE) diag("Short Image mismatch (%d vs %d) "
                                     "at %d,%d. BILINEAR",
                                     outRow[col], truthRow[col],col,row);
                                errorFlag = true;
                            }
                        }
                    }
                }
            }
            ok(!errorFlag, "psImageRotate() produced the correct data values (%d degree rotation), short images, BILINEAR interpolation", rot);

            index++;
        }
//HERE
        // Verify the returned psImage structure pointer is NULL and program
        // execution doesn't stop, if the input parameter input is NULL.
        // Following should be an error
        // XXX: Verify error
        fOut = psImageRotate(fOut,NULL,0,0,PS_INTERPOLATE_FLAT);
        ok(fOut == NULL, "NULL was returned when the input image was NULL");

        // Verify the returned psImage structure pointer is NULL and program
        // execution doesn't stop, if the specified interpolation mode is unallowed
        // Following should be an error for unallowed interpolation type
        // XXX: Verify error
        fOut = psImageRotate(fOut, fImg, 33, 0, -1);
        ok(fOut == NULL, "NULL was returned when the interpolation mode is unallowed");

        psFree(sOut);
        psFree(fImg);
        psFree(sImg);
        psFree(fTruth);
        psFree(sTruth);
        psFree(sBiOut);
        psFree(fBiTruth);
        psFree(sBiTruth);
        psFree(fBiOut);

        psFree(fOutFile);
        psFree(sOutFile);
        psFree(fBiOutFile);
        psFree(sBiOutFile);

        psFree(fTruthFile);
        psFree(sTruthFile);
        psFree(fBiTruthFile);
        psFree(sBiTruthFile);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

/* removed 1/09 WJG

    // testImageShift()
    if (1) {
        psMemId id = psMemGetId();
        // This functions shall generate a new psImage structure by shifting the
        // input psImage structure a specified number of pixels in the horizontal
        // and/or vertical directions.
        //
        // Verify the returned psImage structure contains expected values, if the
        // input psImage structure contains known values and a know shift in the
        // vertical and/or horizontal directions. Cases should include no shift,
        // vertical only(up,down), horizontal only(right, left), and combination
        // shift. Cases should include fractional shifts. Comparison of expected
        // values should include a delta to allow for testing on different
        // platforms.
        //
        // Verify the returned psImage structure contains values for pixels not in
        // the original image set to the input parameter exposed.

        // integer shift
        if (1)
        {
            ok(testImageShiftCase(64,128,0.0f,0.0f), "psImageShift (0, 0)");
            ok(testImageShiftCase(64,128,0.0f,2.0f), "psImageShift (0, 16)");
            ok(testImageShiftCase(64,128,0.0f,-16.0f), "psImageShift (0, -16)");
            ok(testImageShiftCase(64,128,32.0f,0.0f), "psImageShift (32, 0)");
            ok(testImageShiftCase(64,128,-32.0f,0.0f), "psImageShift (-32, 0)");
            ok(testImageShiftCase(64,128,32.0f,16.0f), "psImageShift (32, 16)");
            ok(testImageShiftCase(64,128,32.0f,-16.0f), "psImageShift (32, -16)");
            ok(testImageShiftCase(64,128,-32.0f,16.0f), "psImageShift (-32, 16)");
            ok(testImageShiftCase(64,128,-32.0f,-16.0f), "psImageShift (-32, -16)");

            // fractional shift
            ok(testImageShiftCase(64,128,0.0f,16.4f), "psImageShift (0, 16.4)");
            ok(testImageShiftCase(64,128,0.0f,-16.4f), "psImageShift (0, -16.4)");
            ok(testImageShiftCase(64,128,32.7f,0.0f), "psImageShift (32.7, 0)");
            ok(testImageShiftCase(64,128,-32.7f,0.0f), "psImageShift (-32.7, 0)");
            ok(testImageShiftCase(64,128,32.6f,16.2f), "psImageShift (32.6, 16.2)");
            ok(testImageShiftCase(64,128,32.6f,-16.2f), "psImageShift (32.6, -16.2)");
            ok(testImageShiftCase(64,128,-32.6f,16.2f), "psImageShift (-32.6, 16.2)");
            ok(testImageShiftCase(64,128,-32.6f,-16.2f), "psImageShift (-32.6, -16.2)");
        }

        // Verify the returned psImage structure pointer is equal to the input
        // parameter out if provided.
        psImage *fImg = psImageAlloc(32,32,PS_TYPE_F32);
        psImage *fRecycle = psImageAlloc(32,32,PS_TYPE_F32);
        psImage *fOut = psImageShift(fRecycle, fImg, 8,8, NAN, PS_INTERPOLATE_FLAT);
        ok(fRecycle == fOut, "psImageShift did recycle my image");

        // Verify the returned psImage structure pointer is NULL and program
        // execution doesn't stop, if the input psImage structure pointer is NULL.
        // Following should be an error
        // XXX: Verify error
        fOut = psImageShift(fOut,NULL,8,8,NAN,PS_INTERPOLATE_FLAT);
        ok(fOut == NULL, "psImageShift did return NULL given a NULL input image");

        // Verify the returned psImage structure is NULL and program execution
        // doesn't stop, if the specified interpolation mode is unallowed.
        // Following should be an error for unallowed interpolation mode
        // XXX: Verify error
        fOut = psImageShift(fOut,fImg,8,8,NAN,-1);
        ok(fOut == NULL, "psImageShift did return NULL given an unallowed interpolation mode");

        psFree(fImg);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // test psImageResample()
    if (1) {
        psMemId id = psMemGetId();
        psS32 rows = 60;
        psS32 cols = 80;
        psImage *result = NULL;
        psS32 scale = 4;
        psErr *err;

        psImage *image = psImageAlloc(cols, rows, PS_TYPE_F32);
        for(psS32 row=0;row<rows;row++) {
            for (psS32 col=0;col<cols;col++) {
                image->data.F32[row][col] = row+2*col;
            }
        }
        result = psImageCopy(NULL,image,PS_TYPE_F64);
        psImage *orig = result;
        result = psImageResample(result,image,scale,PS_INTERPOLATE_FLAT);
        ok(result != NULL, "psImageResample() returned non-NULL");
        ok(result == orig, "psImageResample() recycled image");
        ok(result->type.type == PS_TYPE_F32, "psImageResample() produced the correct type");
        ok(result->numCols == image->numCols*scale && result->numRows == image->numRows*scale,
           "psImageResample() produced the correct size");

        bool errorFlag = false;
        {
            psImageInterpolateOptions *tmpIntOpts = psImageInterpolateOptionsAlloc(
                PS_INTERPOLATE_FLAT, image, NULL, NULL, 0, -1.0, NAN, 0, 0, 0.0);
            double imgVal;
            double varVal;
            psMaskType maskVal;
            psF32 truthValue;
            for(psS32 row=0;row<result->numRows;row++) {
                for (psS32 col=0;col<result->numCols;col++) {
//                    truthValue = psImagePixelInterpolate(image,
//                                                         (float)col/(float)scale,(float)row/(float)scale,
//                                                         NULL,0,-1,PS_INTERPOLATE_FLAT);
                    psImageInterpolate(&imgVal, &varVal, &maskVal, 
                                       (float)col/(float)scale,(float)row/(float)scale,
                                       tmpIntOpts);

                    if (fabs(truthValue - result->data.F32[row][col]) > FLT_EPSILON) {
                        if (VERBOSE) diag("value bad at (%d,%d).  Got %g, expected %g",
                             col,row,result->data.F32[row][col], truthValue);
                        errorFlag = true;
                    }
                }
            }
            psFree(tmpIntOpts);
        }

        ok(!errorFlag, "psImageResample() produced the correct data values");

        // verify that image=NULL is handled properly.
        psErrorClear();
        result = psImageResample(result,NULL,scale,PS_INTERPOLATE_FLAT);
        ok(result == NULL, "psImageResample() returned NULL with NULL input");
        err = psErrorLast();
        ok(err->code == PS_ERR_BAD_PARAMETER_NULL, "psImageResample() produced the correct error message");
        psFree(err);

        // verify that scale < 1 is handled properly
        psErrorClear();
        result = psImageResample(result,image,0,PS_INTERPOLATE_FLAT);
        ok(result == NULL, "psImageResample() returned NULL with scale < 1");
        err = psErrorLast();
        ok(err->code == PS_ERR_BAD_PARAMETER_VALUE, "psImageResample() produced the correct error message");
        psFree(err);

        // verify that unallowed interpolation mode is handled properly
        psErrorClear();
        result = psImageResample(result,image,2,-1);
        ok(result == NULL, "psImageResample() returned NULL with unallowed interpolation mode");
        err = psErrorLast();
        ok(err->code == PS_ERR_BAD_PARAMETER_VALUE, "psImageResample() produced the correct error message");
        psFree(err);

        // Verify that that an unallowed image type is handled properly
        psErrorClear();
        psImage *invImage = psImageAlloc(cols,rows,PS_TYPE_BOOL);
        memset(invImage->p_rawDataBuffer,0,cols*rows*PSELEMTYPE_SIZEOF(PS_TYPE_BOOL)); // make sure the image is of all NULLs
        result = psImageResample(result,invImage,2,PS_INTERPOLATE_FLAT);
        ok(result == NULL, "psImageResample() returned NULL with unallowed image type");
        err = psErrorLast();
        ok(err->code == PS_ERR_BAD_PARAMETER_TYPE, "psImageResample() produced the correct error message");
        psFree(err);

        psFree(image);
        psFree(result);
        psFree(invImage);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // test psImageTransform()
    if (1) {
        psMemId id = psMemGetId();
        int cols = 16;
        int rows = 32;

        psPlaneTransform *trans = psPlaneTransformAlloc(2,2);
        trans->x->coeff[1][0] = 0.5;
        trans->y->coeff[0][1] = 1.5;

        psImage *in = psImageAlloc(cols,rows,PS_TYPE_F32);
        for (psS32 row=0;row<rows;row++) {
            for (psS32 col=0;col<cols;col++) {
                in->data.F32[row][col] = (psF32)row+(psF32)col/1000.0f;
            }
        }
        P_PSIMAGE_SET_COL0(in, 1);
        psImage *out = psImageTransform(NULL, NULL, in, NULL, 0, trans,
                                        psRegionSet(1,1+cols*2,0,rows*2), NULL,
                                        PS_INTERPOLATE_FLAT, -1);

        ok(out != NULL, "psImageTransform() returned non-NULL");
        ok(out->type.type == PS_TYPE_F32, "psImageTransform() produced the correct type");
        ok(out->numRows == rows*2 && out->numCols == cols*2, "psImageTransform() produced the correct size");

        if (1) {
            psMemId id = psMemGetId();
            psImageInterpolateOptions *tmpIntOpts = psImageInterpolateOptionsAlloc(
                PS_INTERPOLATE_FLAT, in, NULL, NULL, 0, -1.0, NAN, 0, 0, 0.0);

            double imgVal;
            double varVal;
            psMaskType maskVal;
            bool errorFlag = false;
            for (psS32 row=0;row<out->numRows;row++) {
                for (psS32 col=0;col<cols;col++) {
                    psImageInterpolate(&imgVal, &varVal, &maskVal, 
                                       col*trans->x->coeff[1][0]+trans->x->coeff[0][0]+1,
                                       row*trans->y->coeff[0][1]+trans->y->coeff[0][0],
                                       tmpIntOpts);
                    float inValue = imgVal;
                    if (fabsf(out->data.F32[row][col] - inValue) > 0.01) {
                        diag("out at %d,%d was %g, expected %g", col,row,
                              out->data.F32[row][col], inValue);
                        errorFlag = true;
                    }
                }
            }
            psFree(tmpIntOpts);
            ok(!errorFlag, "psImageTransform() produced the correct data values");
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        }

        psFree(out);
        psFree(in);
        psFree(trans);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
*/
}
