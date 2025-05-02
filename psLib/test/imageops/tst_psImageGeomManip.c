/** @file  tst_psImageGeomManip.c
 *
 *  @brief Contains the tests for psImageManip.[ch]
 *
 *
 *  @author Robert DeSonia, MHPCC
 *
 *  @version $Revision: 1.8 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2006-04-20 01:13:11 $
 *
 *  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 */

#include <complex.h>
#include <math.h>
#include <float.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>                    // for memset
#include <sys/stat.h>
#include <sys/types.h>

#include "psTest.h"
#include "pslib_strict.h"
#include "psType.h"

static psS32 testImageRebin(void);
static psS32 testImageRoll(void);
static psS32 testImageRotate(void);
static psS32 testImageShift(void);
static psS32 testImageShiftCase(psS32 cols, psS32 rows, float colShift,float rowShift);
static psS32 testImageResample(void);
static psS32 testImageTransform(void);

testDescription tests[] = {
                              {testImageRebin,559,"psImageRebin",0,false},
                              {testImageRoll,562,"psImageRoll",0,false},
                              {testImageRotate,560,"psImageRotate",0,false},
                              {testImageShift,561,"psImageShift",0,false},
                              {testImageResample,743,"psImageResample",0,false},
                              {testImageTransform,-1,"psImageTransform",0,false},
                              {NULL}
                          };

psS32 main(psS32 argc, char* argv[])
{
    psLogSetLevel(PS_LOG_INFO);

    return ! runTestSuite(stderr,"psImage",tests,argc,argv);
}

static psS32 testImageRebin(void)
{

    /*
    This function shall generate a rescaled version of a psImage structure
    derived from a specified statistics method.
    */

    psImage* in = NULL;
    psImage* out = NULL;
    psImage* out2 = NULL;
    psImage* out3 = NULL;
    psImage* mask = NULL;
    psImage* meanTruth = NULL;
    psImage* meanTruthWMask = NULL;
    psImage* maxTruth = NULL;
    psStats stats;

    /*
    Verify the returned psImage structure contains expected values, if the
    input parameter input contains known data, the input scale is a known
    value with a known statistical method specified in stats. Cases should
    include at least two different scales and statistical methods. Comparison
    of expected values should include a delta to allow testing on different
    platforms.
    */

    #define testRebinType(DATATYPE)  \
    in = psImageAlloc(16,16,PS_TYPE_##DATATYPE); \
    mask = psImageAlloc(16,16,PS_TYPE_U8); \
    meanTruth = psImageAlloc(4,4,PS_TYPE_F32); \
    meanTruthWMask = psImageAlloc(4,4,PS_TYPE_F32); \
    maxTruth = psImageAlloc(6,6,PS_TYPE_F32); \
    memset(meanTruth->data.F32[0],0,sizeof(psF32)*4*4); \
    memset(meanTruthWMask->data.F32[0],0,sizeof(psF32)*4*4); \
    memset(maxTruth->data.F32[0],0,sizeof(psF32)*6*6); \
    for (psS32 row = 0; row<16; row++) { \
        ps##DATATYPE* inRow = in->data.DATATYPE[row]; \
        psF32* meanTruthRow = meanTruth->data.F32[row/4]; \
        psF32* meanTruthWMaskRow = meanTruthWMask->data.F32[row/4]; \
        psF32* maxTruthRow = maxTruth->data.F32[row/3]; \
        psU8* maskRow = mask->data.U8[row]; \
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
        psF32* meanTruthRow = meanTruth->data.F32[row]; \
        psF32* meanTruthWMaskRow = meanTruthWMask->data.F32[row]; \
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
    out = psImageRebin(NULL,in,NULL,0,4,&stats); \
    if (out == NULL) { \
        psError(PS_ERR_UNKNOWN, true,"psImageRebin returned a NULL pointer!?"); \
        return 1; \
    } \
    if (out->numRows != 4 || out->numCols != 4) { \
        psError(PS_ERR_UNKNOWN, true,"psImageRebin didn't produce the proper size image " \
                "(%d x %d).", \
                out->numCols, out->numRows); \
        return 2; \
    } \
    for (psS32 row = 0; row<4; row++) { \
        ps##DATATYPE* outRow = out->data.DATATYPE[row]; \
        psF32* truthRow = meanTruth->data.F32[row]; \
        for (psS32 col = 0; col<4; col++) { \
            if (fabsf((float)outRow[col]-(float)truthRow[col]) > FLT_EPSILON) { \
                psError(PS_ERR_UNKNOWN, true,"psImageRebin didn't produce the proper mean " \
                        "result at (%d,%d) [%f vs %f].", \
                        col,row,outRow[col],truthRow[col]); \
                return 3; \
            } \
        } \
    } \
    stats.options = PS_STAT_SAMPLE_MEAN; \
    out3 = psImageRebin(NULL,in,mask,1,4,&stats); \
    for (psS32 row = 0; row<4; row++) { \
        ps##DATATYPE* outRow = out3->data.DATATYPE[row]; \
        psF32* truthRow = meanTruthWMask->data.F32[row]; \
        for ( psS32 col = 0; col<4; col++) { \
            if(abs((psS32)outRow[col]-(psS32)truthRow[col]) > FLT_EPSILON) { \
                psError(PS_ERR_UNKNOWN, true,"psImageRebin with mask didn't produce the proper mean " \
                        "result at (%d,%d) [%f vs %f].", \
                        col,row,outRow[col],truthRow[col]); \
                return 3; \
            } \
        } \
    } \
    stats.options = PS_STAT_MAX; \
    out2 = psImageRebin(out,in,NULL,0,3,&stats); \
    if (out != out2) { \
        psError(PS_ERR_UNKNOWN, true,"psImageRebin didn't recycle a psImage properly!?"); \
        return 7; \
    } \
    if (out == NULL) { \
        psError(PS_ERR_UNKNOWN, true,"psImageRebin returned a NULL pointer!?"); \
        return 4; \
    } \
    if (out->numRows != 6 || out->numCols != 6) { \
        psError(PS_ERR_UNKNOWN, true,"psImageRebin didn't produce the proper size image " \
                "(%d x %d).", \
                out->numCols, out->numRows); \
        return 5; \
    } \
    for (psS32 row = 0; row<6; row++) { \
        ps##DATATYPE* outRow = out->data.DATATYPE[row]; \
        psF32* truthRow = maxTruth->data.F32[row]; \
        for (psS32 col = 0; col<6; col++) { \
            if (fabsf((float)outRow[col]-(float)truthRow[col]) > FLT_EPSILON) { \
                psError(PS_ERR_UNKNOWN, true,"psImageRebin didn't produce the proper " \
                        "max result at (%d,%d) [%f vs %f].", \
                        col,row,outRow[col],truthRow[col]); \
                return 6; \
            } \
        } \
    } \
    psFree(in); \
    psFree(out); \
    psFree(out3); \
    psFree(mask); \
    psFree(meanTruth); \
    psFree(meanTruthWMask); \
    psFree(maxTruth);

    testRebinType(F32);
    testRebinType(F64);
    testRebinType(U16);
    testRebinType(S8);

    // Verify the returned psImage structure is null and program execution
    // doesn't stop, if the input image type is not supported.
    psLogMsg(__func__,PS_LOG_INFO,"Following should be an error for unsupported type.");
    in = psImageAlloc(16,16,PS_TYPE_U8);
    mask = psImageAlloc(16,16,PS_TYPE_F32);
    stats.options = PS_STAT_SAMPLE_MEAN;
    out = psImageRebin(NULL,in,NULL,0,4,&stats);
    if(out != NULL) {
        psError(PS_ERR_UNKNOWN, true,"psImageRebin return an image eventhough the "
                "type is not handled.");
        return 14;
    }
    // Verify the returned psImage structure is null and program execution
    // doesn't stop, if the mask type is not U8
    psLogMsg(__func__,PS_LOG_INFO,"Following should be an error for invallid mask type.");
    out = psImageRebin(NULL,in,mask,1,4,&stats);
    if(out != NULL) {
        psError(PS_ERR_UNKNOWN, true,"psImageRebin return an image eventhough the "
                "mask is not the correct type.");
        return 17;
    }
    psFree(mask);
    psFree(in);

    // Verify the returned psImage structure is null and program execution
    // doesn't stop, if the input parameter input is null.

    out2 = psImageRebin(NULL,NULL,NULL,0,1,&stats);

    if (out2 != NULL) {
        psError(PS_ERR_UNKNOWN, true,"psImageRebin returned an image though the input was "
                "NULL!?");
        return 8;
    }

    // Verify the returned psImage structure is null and program execution
    // doesn't stop, if the input parameter scale is less than or equal to zero.
    in = psImageAlloc(16, 16, PS_TYPE_F32);
    psLogMsg(__func__,PS_LOG_INFO,"Following should be an error for scale < 0.");
    out2 = psImageRebin(NULL,in,NULL,0,0,&stats);

    if (out2 != NULL) {
        psError(PS_ERR_UNKNOWN, true,"psImageRebin returned an image though the scale was "
                "zero!?");
        return 9;
    }

    // Verify the returned psImage structure is null and program execution
    // doesn't stop, if the input parameter stats is null.
    psLogMsg(__func__,PS_LOG_INFO,"Following should be an error for stats null.");
    out2 = psImageRebin(NULL,in,NULL,0,1,NULL);

    if (out2 != NULL) {
        psError(PS_ERR_UNKNOWN, true,"psImageRebin returned an image though the stats was "
                "NULL!?");
        return 10;
    }

    // Verify the returned psImage structure is null and program execution
    // doesn't stop, if the input parameter psStats structure member options
    // is zero or any value which doesn't correspond to a valid statistical
    // method.
    psLogMsg(__func__,PS_LOG_INFO,"Following should be an error for stats options 0.");
    stats.options = 0;
    out2 = psImageRebin(NULL,in,NULL,0,1,&stats);

    if (out2 != NULL) {
        psError(PS_ERR_UNKNOWN, true,"psImageRebin returned an image though the stats "
                "options was zero!?");
        return 11;
    }

    psLogMsg(__func__,PS_LOG_INFO,"Following should be an error for stat options use range.");
    stats.options = PS_STAT_USE_RANGE;
    out2 = psImageRebin(NULL,in,NULL,0,1,&stats);

    if (out2 != NULL) {
        psError(PS_ERR_UNKNOWN, true,"psImageRebin returned an image though the stats "
                "options was PS_STAT_USE_RANGE!?");
        return 12;
    }

    // Verify the returned psImage structure is null and program execution
    // doesn't stop, if the input parameter psStats structure member options
    // specifies more than one valid statistical method.
    psLogMsg(__func__,PS_LOG_INFO,"Following should be an error for stats with multiple options.");
    stats.options = PS_STAT_SAMPLE_MEAN + PS_STAT_MAX;
    out2 = psImageRebin(NULL,in,NULL,0,1,&stats);

    if (out2 != NULL) {
        psError(PS_ERR_UNKNOWN, true,"psImageRebin returned an image though the stats "
                "options was PS_STAT_SAMPLE_MEAN+PS_STAT_MAX!?");
        return 13;
    }

    psFree(in);

    return 0;
}

static psS32 testImageRoll(void)
{

    psImage* in;
    psImage* out;
    psImage* out2;
    psS32 rows = 64;
    psS32 cols = 64;
    psS32 rows1 = 8;
    psS32 cols1 = 8;

    /*
     The function psImageRoll shall generate a new psImage structure by
     rolling the input image the correponding number of pixels in the vertical
     and/or horizontal direction. The image output image shall be the same size
     as the input image. Values which roll off the image are wrapped to the
     other side.

     Verify the returned psImage structure contains expected values, if the
     input image contains known values and the roll performed is known.
     Cases should include no roll, vertical roll, horizontal roll and
     combination vertical/horizontal rolls. Positive and negative rolls
     should be performed.
    */

    in = psImageAlloc(cols,rows,PS_TYPE_F32);
    for (psS32 row=0;row<rows;row++) {
        psF32* inRow = in->data.F32[row];
        for (psS32 col=0;col<cols;col++) {
            inRow[col] = (psF32)row+(psF32)col/1000.0f;
        }
    }

    out = psImageRoll(NULL,in,0,0);
    for (psS32 row=0;row<rows;row++) {
        psF32* inRow = in->data.F32[row];
        psF32* outRow = out->data.F32[row];
        for (psS32 col=0;col<cols;col++) {
            if (inRow[col] != outRow[col]) {
                psError(PS_ERR_UNKNOWN, true,"psImageRoll didn't produce expected result "
                        "at %d,%d (%f vs %f) for dx=0, dy=0.",
                        col,row,inRow[col],outRow[col]);
                return 3;
            }
        }
    }

    out2 = psImageRoll(out,in,cols/4,0);
    for (psS32 row=0;row<rows;row++) {
        psF32* inRow = in->data.F32[row];
        psF32* outRow = out->data.F32[row];
        for (psS32 col=0;col<cols;col++) {
            if (inRow[(col+cols/4) % cols] != outRow[col]) {
                psError(PS_ERR_UNKNOWN, true,"psImageRoll didn't produce expected result "
                        "at %d,%d (%f vs %f) for dx=cols/4, dy=0.",
                        col,row,inRow[(col+cols/4) % cols],outRow[col]);
                return 4;
            }
        }
    }

    // Verify the returned psImage structure pointer is equal to the input
    // parameter out if provided.
    if (out2 != out) {
        psError(PS_ERR_UNKNOWN, true,"psImageRoll didn't recycle my out psImage!?");
        return 1;
    }

    out = psImageRoll(out,in,0,rows/4);
    for (psS32 row=0;row<rows;row++) {
        psF32* inRow = in->data.F32[(row+rows/4)%rows];
        psF32* outRow = out->data.F32[row];
        for (psS32 col=0;col<cols;col++) {
            if (inRow[col] != outRow[col]) {
                psError(PS_ERR_UNKNOWN, true,"psImageRoll didn't produce expected result "
                        "at %d,%d (%f vs %f) for dx=0, dy=rows/4.",
                        col,row,inRow[col],outRow[col]);
                return 5;
            }
        }
    }

    out = psImageRoll(out,in,cols/4,rows/4);
    for (psS32 row=0;row<rows;row++) {
        psF32* inRow = in->data.F32[(row+rows/4)%rows];
        psF32* outRow = out->data.F32[row];
        for (psS32 col=0;col<cols;col++) {
            if (inRow[(col+cols/4) % cols] != outRow[col]) {
                psError(PS_ERR_UNKNOWN, true,"psImageRoll didn't produce expected result "
                        "at %d,%d (%f vs %f) for dx=cols/4, dy=rows/4.",
                        col,row,inRow[(col+cols/4) % cols],outRow[col]);
                return 6;
            }
        }
    }

    out = psImageRoll(out,in,-cols/4,0);
    for (psS32 row=0;row<rows;row++) {
        psF32* inRow = in->data.F32[row];
        psF32* outRow = out->data.F32[row];
        for (psS32 col=0;col<cols;col++) {
            if (inRow[(col+(cols-cols/4)) % cols] != outRow[col]) {
                psError(PS_ERR_UNKNOWN, true,"psImageRoll didn't produce expected result "
                        "at %d,%d (%f vs %f) for dx=-cols/4, dy=0.",
                        col,row,inRow[(col+(cols-cols/4)) % cols],outRow[col]);
                return 7;
            }
        }
    }

    out = psImageRoll(out,in,0,-rows/4);
    for (psS32 row=0;row<rows;row++) {
        psF32* inRow = in->data.F32[(row+rows-rows/4)%rows];
        psF32* outRow = out->data.F32[row];
        for (psS32 col=0;col<cols;col++) {
            if (inRow[col] != outRow[col]) {
                psError(PS_ERR_UNKNOWN, true,"psImageRoll didn't produce expected result "
                        "at %d,%d (%f vs %f) for dx=0, dy=-rows/4.",
                        col,row,inRow[col],outRow[col]);
                return 8;
            }
        }
    }

    out = psImageRoll(out,in,-cols/4,-rows/4);
    for (psS32 row=0;row<rows;row++) {
        psF32* inRow = in->data.F32[(row+rows-rows/4)%rows];
        psF32* outRow = out->data.F32[row];
        for (psS32 col=0;col<cols;col++) {
            if (inRow[(col+cols-cols/4) % cols] != outRow[col]) {
                psError(PS_ERR_UNKNOWN, true,"psImageRoll didn't produce expected result "
                        "at %d,%d (%f vs %f) for dx=cols/4, dy=rows/4.",
                        col,row,inRow[(col+cols-cols/4) % cols],outRow[col]);
                return 9;
            }
        }
    }


    // Verify the returned psImage structure pointer is null and program
    // execution doesn't stop, if input parameter input is null.
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate an error.");
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
        ps##DATATYPE* inRow = in->data.DATATYPE[row]; \
        for (psS32 col=0;col<cols1;col++) { \
            inRow[col] = (ps##DATATYPE)row+(ps##DATATYPE)col; \
        } \
    } \
    \
    out = psImageRoll(NULL,in,rows1/4,cols1/4); \
    for (psS32 row=0;row<rows1;row++) { \
        ps##DATATYPE* inRow = in->data.DATATYPE[(row+rows1/4)%rows1]; \
        ps##DATATYPE* outRow = out->data.DATATYPE[row]; \
        for (psS32 col=0;col<cols1;col++) { \
            if (inRow[(col+cols1/4)%cols1] != outRow[col]) { \
                psError(PS_ERR_UNKNOWN, true,"psImageRoll didn't produce expected result " \
                        "at %d,%d (%f vs %f) for dx=0, dy=0.", \
                        col,row,(float)inRow[col],(float)outRow[col]); \
                return 3; \
            } \
        } \
    } \
    psFree(in); \
    psFree(out);

    testRollType(U8);
    testRollType(U16);
    testRollType(S8);
    testRollType(S16);
    testRollType(F64);
    testRollType(C32);
    testRollType(C64);

    return 0;
}

psS32 testImageRotate(void)
{
    /*

    This function shall calculate a new psImage structure based upon the
    rotation of a given psImage structure. The center of rotation shall be the
    center pixel of the input image.

    The following steps of the testpoint are done manually via inspection of
    fOut.fits & sOut.fits.

        * Verify the returned psImage structure contains expected values, if
          the input parameter psImage contains known values. Cases should
          include rotations of 0, 45, 90, 135, 180, 225, 270, 315, 360 and at leat one
          other arbitrary angle. Cases of the input image should include image
          with a center pixel and an image without a center pixel.
        * Verify the returned psImage structure contains pixels set to exposed value, if
          the rotation and input psImage to not correspond to the output image.

    */

    psImage* fOut = NULL;
    psImage* sOut = NULL;
    psImage* fBiOut = NULL;
    psImage* sBiOut = NULL;
    psImage* fTruth = NULL;
    psImage* sTruth = NULL;
    psImage* fBiTruth = NULL;
    psImage* sBiTruth = NULL;
    psS32 rows = 64;
    psS32 cols = 64;
    psImage* fImg = psImageAlloc(cols,rows,PS_TYPE_F32);
    psImage* sImg = psImageAlloc(cols,rows,PS_TYPE_S16);

    for(psS32 row=0;row<rows;row++) {
        psF32* fRow = fImg->data.F32[row];
        psS16* sRow = sImg->data.S16[row];
        for (psS32 col=0;col<cols;col++) {
            fRow[col] = (psF32)(row)+(psF32)(col)/100.0f;
            sRow[col] = row-2*col;
        }
    }

    // since interpolation is involved, etc., the simplist way to verify things
    // is to verify the results manually and bless it for automated comparison
    // thereafter


    // write results of various rotates to a file and verify with truth images
    mkdir("temp",0777);
    psS32 index = 0;
    psBool fail = false;
    psF32 radianRot;

    psFits* fOutFile = psFitsOpen("fOut.fits","w");
    psFits* sOutFile = psFitsOpen("sOut.fits","w");
    psFits* fBiOutFile = psFitsOpen("fBiOut.fits","w");
    psFits* sBiOutFile = psFitsOpen("sBiOut.fits","w");
    if (fOutFile == NULL ||sOutFile == NULL || fBiOutFile == NULL || sBiOutFile == NULL) {
        psError(PS_ERR_UNKNOWN, true, "Can not create output files, so why continue!?");
        return 1;
    }

    psFits* fTruthFile = psFitsOpen(VERIFIED_DIR "/fOut.fits","r");
    psFits* sTruthFile = psFitsOpen(VERIFIED_DIR "/sOut.fits","r");
    psFits* fBiTruthFile = psFitsOpen(VERIFIED_DIR "/fBiOut.fits","r");
    psFits* sBiTruthFile = psFitsOpen(VERIFIED_DIR "/sBiOut.fits","r");
    if (fTruthFile == NULL ||sTruthFile == NULL || fBiTruthFile == NULL || sBiTruthFile == NULL) {
        psError(PS_ERR_UNKNOWN, true, "Can not open truth files, so why continue!?");
        return 1;
    }

    psRegion regionAll = psRegionSet(0,0,0,0);
    for (psS32 rot=-180;rot<=180;rot+=45) {
        psImage* oldOut = fOut;
        psImage* oldBiOut = fBiOut;
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
        if (oldOut != NULL && oldOut != fOut) {
            psError(PS_ERR_UNKNOWN, true,"the output recycle functionality failed");
            return 2;
        }
        if (oldBiOut != NULL && oldBiOut != fBiOut) {
            psError(PS_ERR_UNKNOWN, true,"the output recycle functionality failed");
            return 4;
        }
        sOut = psImageRotate(sOut,sImg,radianRot,-1.0,PS_INTERPOLATE_FLAT);
        sBiOut = psImageRotate(sBiOut,sImg,radianRot,-1.0,PS_INTERPOLATE_BILINEAR);

        if (! psFitsWriteImage(fOutFile, NULL, fOut, 1, NULL) ) {
            psError(PS_ERR_UNKNOWN, true,"Can not write fOut.");
            return 20;
        }
        if (! psFitsWriteImage(sOutFile, NULL, sOut, 1, NULL) ) {
            psError(PS_ERR_UNKNOWN, true,"Can not write sOut.");
            return 21;
        }
        if (! psFitsWriteImage(fBiOutFile, NULL, fBiOut, 1, NULL) ) {
            psError(PS_ERR_UNKNOWN, true,"Can not write fBiOut.fits.");
            return 40;
        }
        if (! psFitsWriteImage(sBiOutFile, NULL, sBiOut, 1, NULL) ) {
            psError(PS_ERR_UNKNOWN, true,"Can not write sBiOut.fits.");
            return 41;
        }

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
        if (fTruth == NULL) {
            psError(PS_ERR_UNKNOWN, true,"verified psF32 image failed to be read (%d deg. rotation)",
                    rot);
            fail = true;
        } else {
            if (fTruth->numRows != fOut->numRows || fTruth->numCols != fOut->numCols) {
                psError(PS_ERR_UNKNOWN, true,"Rotated float image size did not match truth "
                        "image for %d deg rotation (%dx%d vs %dx%d).",
                        rot,fOut->numCols,fOut->numRows,fTruth->numCols,fTruth->numRows);
                fail = true;
            } else {
                for (psS32 row=0;row<fTruth->numRows;row++) {
                    psF32* truthRow = fTruth->data.F32[row];
                    psF32* outRow = fOut->data.F32[row];
                    for (psS32 col=0;col<fTruth->numCols;col++) {
                        if (fabsf(truthRow[col]-outRow[col]) > 1) {
                            psError(PS_ERR_UNKNOWN, true,"Float Image mismatch (%f vs %f) at %d,%d.",
                                    outRow[col], truthRow[col],col,row);
                            fail = true;
                        }
                    }
                }
            }
        }

        if (sTruth == NULL) {
            psError(PS_ERR_UNKNOWN, true,"verified psS16 image failed to be read "
                    "(%d deg. rotation)",rot);
            fail = true;
        } else {
            if (sTruth->numRows != sOut->numRows ||
                    sTruth->numCols != sOut->numCols) {
                psError(PS_ERR_UNKNOWN, true,"Rotated psS16 image size did not match truth "
                        "image for %d deg rotation.",rot);
                fail = true;
            } else {
                for (psS32 row=0;row<sTruth->numRows;row++) {
                    psS16* truthRow = sTruth->data.S16[row];
                    psS16* outRow = sOut->data.S16[row];
                    for (psS32 col=0;col<sTruth->numCols;col++) {
                        if (fabsf(truthRow[col]-outRow[col]) > 1) {
                            psError(PS_ERR_UNKNOWN, true,"Short Image mismatch (%d vs %d) "
                                    "at %d,%d.",
                                    outRow[col], truthRow[col],col,row);
                            fail = true;
                        }
                    }
                }
            }
        }


        if (fBiTruth == NULL) {
            psError(PS_ERR_UNKNOWN, true,"verified psF32 Bi image failed to be read (%d deg. rotation)",
                    rot);
            fail = true;
        } else {
            if (fBiTruth->numRows != fBiOut->numRows || fBiTruth->numCols != fBiOut->numCols) {
                psError(PS_ERR_UNKNOWN, true,"Rotated float image size did not match truth "
                        "image for %d deg rotation (%dx%d vs %dx%d). BILINEAR",
                        rot,fBiOut->numCols,fBiOut->numRows,fBiTruth->numCols,fBiTruth->numRows);
                fail = true;
            } else {
                for (psS32 row=0;row<fBiTruth->numRows;row++) {
                    psF32* truthRow = fBiTruth->data.F32[row];
                    psF32* outRow = fBiOut->data.F32[row];
                    for (psS32 col=0;col<fBiTruth->numCols;col++) {
                        if (fabsf(truthRow[col]-outRow[col]) > 1) {
                            psError(PS_ERR_UNKNOWN, true,"Float Image mismatch (%f vs %f) at %d,%d. BILINEAR",
                                    outRow[col], truthRow[col],col,row);
                            fail = true;
                        }
                    }
                }
            }
        }

        if (sBiTruth == NULL) {
            psError(PS_ERR_UNKNOWN, true,"verified psS16 image failed to be read "
                    "(%d deg. rotation) BILINEAR",rot);
            fail = true;
        } else {
            if (sBiTruth->numRows != sBiOut->numRows ||
                    sBiTruth->numCols != sBiOut->numCols) {
                psError(PS_ERR_UNKNOWN, true,"Rotated psS16 image size did not match truth "
                        "image for %d deg rotation. BILINEAR",rot);
                fail = true;
            } else {
                for (psS32 row=0;row<sBiTruth->numRows;row++) {
                    psS16* truthRow = sBiTruth->data.S16[row];
                    psS16* outRow = sBiOut->data.S16[row];
                    for (psS32 col=0;col<sBiTruth->numCols;col++) {
                        if (fabsf(truthRow[col]-outRow[col]) > 1) {
                            psError(PS_ERR_UNKNOWN, true,"Short Image mismatch (%d vs %d) "
                                    "at %d,%d. BILINEAR",
                                    outRow[col], truthRow[col],col,row);
                            fail = true;
                        }
                    }
                }
            }
        }

        index++;
    }

    if (fail) {
        psError(PS_ERR_UNKNOWN, true,"One or more images didn't match truth or truth did "
                "not exist.");
        return 10;
    }


    // Verify the returned psImage structure pointer is null and program
    // execution doesn't stop, if the input parameter input is null.
    psLogMsg(__func__,PS_LOG_INFO,"Following should be an error");
    fOut = psImageRotate(fOut,NULL,0,0,PS_INTERPOLATE_FLAT);
    if (fOut != NULL) {
        psError(PS_ERR_UNKNOWN, true,"NULL wasn't returned though the input image was NULL.");
        return 3;
    }

    // Verify the returned psImage structure pointer is null and program
    // execution doesn't stop, if the specified interpolation mode is invalid
    psLogMsg(__func__,PS_LOG_INFO,"Following should be an error for invalid "
             "interpolation type.");
    fOut = psImageRotate(fOut, fImg, 33, 0, -1);
    if (fOut != NULL) {
        psError(PS_ERR_UNKNOWN, true,"NULL wasn't returned though the interpolation mode "
                "is invalid.");
        return 4;
    }

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

    return 0;
}

static psS32 testImageShift(void)
{
    /* psImageShift:

       This functions shall generate a new psImage structure by shifting the
       input psImage structure a specified number of pixels in the horizontal
       and/or vertical directions.

       Verify the returned psImage structure contains expected values, if the
       input psImage structure contains known values and a know shift in the
       vertical and/or horizontal directions. Cases should include no shift,
       vertical only(up,down), horizontal only(right, left), and combination
       shift. Cases should include fractional shifts. Comparison of expected
       values should include a delta to allow for testing on different
       platforms.

       Verify the returned psImage structure contains values for pixels not in
       the original image set to the input parameter exposed.

    */

    psS32 retVal=0;

    // integer shift
    retVal |= testImageShiftCase(64,128,0.0f,0.0f);
    retVal |= testImageShiftCase(64,128,0.0f,16.0f);
    retVal |= testImageShiftCase(64,128,0.0f,-16.0f);
    retVal |= testImageShiftCase(64,128,32.0f,0.0f);
    retVal |= testImageShiftCase(64,128,-32.0f,0.0f);
    retVal |= testImageShiftCase(64,128,32.0f,16.0f);
    retVal |= testImageShiftCase(64,128,32.0f,-16.0f);
    retVal |= testImageShiftCase(64,128,-32.0f,16.0f);
    retVal |= testImageShiftCase(64,128,-32.0f,-16.0f);

    if (retVal != 0) {
        return retVal;
    }

    // fractional shift
    retVal |= testImageShiftCase(64,128,0.0f,16.4f);
    retVal |= testImageShiftCase(64,128,0.0f,-16.4f);
    retVal |= testImageShiftCase(64,128,32.7f,0.0f);
    retVal |= testImageShiftCase(64,128,-32.7f,0.0f);
    retVal |= testImageShiftCase(64,128,32.6f,16.2f);
    retVal |= testImageShiftCase(64,128,32.6f,-16.2f);
    retVal |= testImageShiftCase(64,128,-32.6f,16.2f);
    retVal |= testImageShiftCase(64,128,-32.6f,-16.2f);

    if (retVal != 0) {
        return retVal;
    }

    /*
       Verify the returned psImage structure pointer is equal to the input
       parameter out if provided.
    */
    psImage* fImg = psImageAlloc(32,32,PS_TYPE_F32);
    psImage* fRecycle = psImageAlloc(32,32,PS_TYPE_F32);
    psImage* fOut = psImageShift(fRecycle, fImg, 8,8, NAN, PS_INTERPOLATE_FLAT);

    if (fRecycle != fOut) {
        psError(PS_ERR_UNKNOWN, true,"psImageShift didn't recycle my image?");
        return 10;
    }

    /*
       Verify the returned psImage structure pointer is null and program
       execution doesn't stop, if the input psImage structure pointer is null.
    */
    psLogMsg(__func__,PS_LOG_INFO,"Following should be an error...");
    fOut = psImageShift(fOut,NULL,8,8,NAN,PS_INTERPOLATE_FLAT);
    if (fOut != NULL) {
        psError(PS_ERR_UNKNOWN, true,"psImageShift didn't return NULL given a NULL input image.");
        return 11;
    }

    // Verify the returned psImage structure is null and program execution
    // doesn't stop, if the specified interpolation mode is invalid.
    psLogMsg(__func__,PS_LOG_INFO,"Following should be an error for invalid interpolation mode.");
    fOut = psImageShift(fOut,fImg,8,8,NAN,-1);
    if (fOut != NULL ) {
        psError(PS_ERR_UNKNOWN, true,"psImageShift didn't return NULL given an invalid interpolation mode.");
        return 12;
    }

    psFree(fImg);

    return 0;
}

static psS32 testImageShiftCase(psS32 cols,
                                psS32 rows,
                                float colShift,
                                float rowShift)
{
    psImage* fOut = NULL;
    psImage* sOut = NULL;

    psLogMsg(__func__,PS_LOG_INFO,"Testing psImageShift with a %dx%d image for "
             "a shift of %g,%g.",cols,rows,colShift,rowShift);

    psImage* fImg = psImageAlloc(cols,rows,PS_TYPE_F32);
    psImage* sImg = psImageAlloc(cols,rows,PS_TYPE_S16);
    psImage* fBiOut = psImageAlloc(cols,rows,PS_TYPE_F32);
    psImage* sBiOut = psImageAlloc(cols,rows,PS_TYPE_S16);

    for(psS32 row=0;row<rows;row++) {
        psF32* fRow = fImg->data.F32[row];
        psS16* sRow = sImg->data.S16[row];
        for (psS32 col=0;col<cols;col++) {
            fRow[col] = (psF32)(row)+(psF32)(col)/100.0f;
            sRow[col] = row-2*col;
        }
    }

    fOut = psImageShift(fOut, fImg, colShift, rowShift, NAN, PS_INTERPOLATE_FLAT);
    sOut = psImageShift(sOut, sImg, colShift, rowShift, -1, PS_INTERPOLATE_FLAT);
    fBiOut = psImageShift(fBiOut, fImg, colShift, rowShift, NAN, PS_INTERPOLATE_BILINEAR);
    sBiOut = psImageShift(sBiOut, sImg, colShift, rowShift, -1, PS_INTERPOLATE_BILINEAR);

    for(psS32 row=0;row<rows;row++) {
        psF32* fRow = fOut->data.F32[row];
        psS16* sRow = sOut->data.S16[row];
        psF32* fBiRow = fBiOut->data.F32[row];
        psS16* sBiRow = sBiOut->data.S16[row];

        for (psS32 col=0;col<cols;col++) {
            psF32 fValue = psImagePixelInterpolate(fImg,col+colShift,
                                                   row+rowShift,NULL,0,NAN,PS_INTERPOLATE_FLAT);
            psS16 sValue = (psS16)psImagePixelInterpolate(sImg,col+colShift,
                           row+rowShift,NULL,0,-1,PS_INTERPOLATE_FLAT);

            psF32 fBiValue = psImagePixelInterpolate(fImg,col+colShift,
                             row+rowShift,NULL,0,NAN,PS_INTERPOLATE_BILINEAR);
            psS16 sBiValue = (psS16)psImagePixelInterpolate(sImg,col+colShift,
                             row+rowShift,NULL,0,-1,PS_INTERPOLATE_BILINEAR);

            if (fabsf(fRow[col] - fValue) > FLT_EPSILON) {
                psError(PS_ERR_UNKNOWN, true,"Float image not shifted correctly at %d,%d (%g vs %g)",
                        col,row,fRow[col],fValue);
                return 1;
            }
            if (sRow[col] != sValue) {
                psError(PS_ERR_UNKNOWN, true,"Short image not shifted correctly at %d,%d (%d vs %d)",
                        col,row,sRow[col],sValue);
                return 2;
            }
            if (fabsf(fBiRow[col] - fBiValue) > FLT_EPSILON) {
                psError(PS_ERR_UNKNOWN, true,"Float image not shifted correctly at %d,%d (%g vs %g)",
                        col,row,fBiRow[col],fBiValue);
                return 1;
            }
            if (sBiRow[col] != sBiValue) {
                psError(PS_ERR_UNKNOWN, true,"Short image not shifted correctly at %d,%d (%d vs %d)",
                        col,row,sBiRow[col],sBiValue);
                return 2;
            }
        }
    }

    psFree(fImg);
    psFree(sImg);
    psFree(fOut);
    psFree(sOut);
    psFree(fBiOut);
    psFree(sBiOut);

    return 0;
}

static psS32 testImageResample(void)
{

    psS32 rows = 60;
    psS32 cols = 80;
    psImage* result = NULL;
    psS32 scale = 4;
    psErr* err;

    psImage* image = psImageAlloc(cols,rows,PS_TYPE_F32);
    for(psS32 row=0;row<rows;row++) {
        psF32* imageRow = image->data.F32[row];
        for (psS32 col=0;col<cols;col++) {
            imageRow[col] = row+2*col;
        }
    }
    result = psImageCopy(NULL,image,PS_TYPE_F64);
    psImage* orig = result;
    result = psImageResample(result,image,scale,PS_INTERPOLATE_FLAT);

    if (result == NULL) {
        psLogMsg(__func__,PS_LOG_ERROR,
                 "NULL return unexpected");
        return 1;
    }

    if (result != orig) {
        psLogMsg(__func__,PS_LOG_ERROR,"failure to recycle image.");
        return 2;
    }

    if (result->type.type != PS_TYPE_F32) {
        psLogMsg(__func__,PS_LOG_ERROR,"unexpected type");
        return 3;
    }


    if (result->numCols != image->numCols*scale ||
            result->numRows != image->numRows*scale) {
        psLogMsg(__func__,PS_LOG_ERROR,"The size of the result is %dx%d, but %dx%d was expected.",
                 result->numCols,result->numRows,
                 image->numCols*scale, image->numRows*scale);
        return 4;
    }

    psF32 truthValue;
    for(psS32 row=0;row<result->numRows;row++) {
        for (psS32 col=0;col<result->numCols;col++) {
            truthValue = psImagePixelInterpolate(image,
                                                 (float)col/(float)scale,(float)row/(float)scale,
                                                 NULL,0,-1,PS_INTERPOLATE_FLAT);
            if (fabs(truthValue - result->data.F32[row][col]) > FLT_EPSILON) {
                psLogMsg(__func__,PS_LOG_ERROR,"value bad at (%d,%d).  Got %g, expected %g.",
                         col,row,result->data.F32[row][col], truthValue);
                return 5;
            }
        }
    }

    // verify that image=null is handled properly.
    psErrorClear();
    result = psImageResample(result,NULL,scale,PS_INTERPOLATE_FLAT);

    if (result != NULL) {
        psLogMsg(__func__,PS_LOG_ERROR,
                 "return was not NULL, as expected.");
        return 6;
    }
    err = psErrorLast();
    if (err->code != PS_ERR_BAD_PARAMETER_NULL) {
        psLogMsg(__func__,PS_LOG_ERROR,
                 "error message was not appropriate type.");
        return 7;
    }
    psFree(err);

    // verify that scale < 1 is handled properly
    psErrorClear();
    result = psImageResample(result,image,0,PS_INTERPOLATE_FLAT);

    if (result != NULL) {
        psLogMsg(__func__,PS_LOG_ERROR,
                 "return was not NULL, as expected.");
        return 8;
    }
    err = psErrorLast();
    if (err->code != PS_ERR_BAD_PARAMETER_VALUE) {
        psLogMsg(__func__,PS_LOG_ERROR,
                 "error message was not appropriate type.");
        return 9;
    }
    psFree(err);

    // verify that invalid interpolation mode is handled properly
    psErrorClear();
    result = psImageResample(result,image,2,-1);

    if (result != NULL) {
        psLogMsg(__func__,PS_LOG_ERROR,
                 "return was not NULL, as expected.");
        return 10;
    }
    err = psErrorLast();
    if (err->code != PS_ERR_BAD_PARAMETER_VALUE) {
        psLogMsg(__func__,PS_LOG_ERROR,
                 "error message was not appropriate type.");
        return 11;
    }
    psFree(err);

    // Verify that that an invalid image type is handled properly
    psErrorClear();
    psImage* invImage = psImageAlloc(cols,rows,PS_TYPE_BOOL);
    memset(invImage->p_rawDataBuffer,0,cols*rows*PSELEMTYPE_SIZEOF(PS_TYPE_BOOL)); // make sure the image is of all NULLs
    result = psImageResample(result,invImage,2,PS_INTERPOLATE_FLAT);
    if (result != NULL) {
        psLogMsg(__func__,PS_LOG_ERROR,"return was not NULL, as expected.");
        return 20;
    }
    err = psErrorLast();
    if (err->code != PS_ERR_BAD_PARAMETER_TYPE) {
        psLogMsg(__func__,PS_LOG_ERROR,"error message was not appropriate type.");
        return 21;
    }
    psFree(err);

    psFree(image);
    psFree(result);
    psFree(invImage);

    return 0;
}

static psS32 testImageTransform(void)
{
    int cols = 16;
    int rows = 32;

    psPlaneTransform* trans = psPlaneTransformAlloc(2,2);
    trans->x->coeff[1][0] = 0.5;
    trans->y->coeff[0][1] = 1.5;

    psImage* in = psImageAlloc(cols,rows,PS_TYPE_F32);
    for (psS32 row=0;row<rows;row++) {
        psF32* inRow = in->data.F32[row];
        for (psS32 col=0;col<cols;col++) {
            inRow[col] = (psF32)row+(psF32)col/1000.0f;
        }
    }
    in->col0 = 1;
    psImage* out = psImageTransform(NULL,
                                    NULL,
                                    in,
                                    NULL,
                                    0,
                                    trans,
                                    psRegionSet(1,1+cols*2,0,rows*2),
                                    NULL,
                                    PS_INTERPOLATE_FLAT,
                                    -1);

    if (out == NULL) {
        psError(PS_ERR_UNKNOWN, false,
                "out == NULL");
        return 1;
    }
    if (out->type.type != PS_TYPE_F32) {
        psError(PS_ERR_UNKNOWN, false,
                "out->type.type != PS_TYPE_F32, out->type.type == %d",
                out->type.type);
        return 2;
    }
    if (out->numRows != rows*2 || out->numCols != cols*2) {
        psError(PS_ERR_UNKNOWN, false,
                "out size is %dx%d, not %dx%d",
                out->numCols, out->numRows, cols*2, rows);
        return 3;
    }

    for (psS32 row=0;row<out->numRows;row++) {
        psF32* outRow = out->data.F32[row];
        for (psS32 col=0;col<cols;col++) {
            float inValue = p_psImagePixelInterpolateFLAT_F32(in,
                            col*trans->x->coeff[1][0]+trans->x->coeff[0][0]+1,
                            row*trans->y->coeff[0][1]+trans->y->coeff[0][0],
                            NULL, 0,
                            -1);
            if (fabsf(outRow[col] - inValue) > 0.01) {
                psError(PS_ERR_UNKNOWN, false,
                        "out at %d,%d was %g, expected %g",
                        col,row,outRow[col], inValue);
                return 4;
            }
        }
    }

    psFree(out);
    psFree(in);
    psFree(trans);
    return 0;
}
