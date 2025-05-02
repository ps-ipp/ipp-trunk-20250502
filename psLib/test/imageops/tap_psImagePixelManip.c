/** @file  tst_psImageManip.c
 *
 *  @brief Contains the tests for psImageManip.[ch]
 *
 *
 *  @author Robert DeSonia, MHPCC
 *
 *  @version $Revision: 1.8 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2007-06-04 20:25:32 $
 *
 *  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 */
#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include "tap.h"
#include "pstap.h"
#define VERBOSE false

void genericImageClipTest(int numRows, int numCols) {
    psMemId id = psMemGetId();

    psImage *image = psImageAlloc(numCols, numRows, PS_TYPE_F32);
    for (int row = 0 ; row < numRows ; row++) {
        for (int col = 0 ; col < numCols ; col++) {
            image->data.F32[row][col] = (float) (row + col);
        }
    }
    psF32 min = (psF64)numRows/2.0;
    psF32 max = (psF64)numRows;

    psS32 retVal = psImageClip(image, min, (double)PS_MIN_F32, max, (double)PS_MAX_F32);
    int numClipped = 0;
    bool errorFlag = false;
    for (int row=0;row<numRows;row++) {
        for (int col=0;col<numCols;col++) {
            psF32 value = (psF32)(row+col);
            if (value < min) {
                numClipped++;
                value = PS_MIN_F32;
            } else if (value > max) {
                numClipped++;
                value = PS_MAX_F32;
            }
            if (fabsf(image->data.F32[row][col]-value) > FLT_EPSILON) {
                diag("Pixel value is not as expected (%g vs %g) at %u,%u",
                     (psF64)image->data.F32[row][col], (psF64)value, col, row);
                errorFlag = true;
            }
        }
    }
    ok(!errorFlag, "psImageClip() produced the correct data values");
    ok(retVal == numClipped, "Got the expected number of clips");
    psFree(image);

    ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
}


psS32 main(psS32 argc, char* argv[])
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    plan_tests(122);

    // test psImageClip()
    {
        psMemId id = psMemGetId();

        //psImageClip shall limit the minimum and maximum data value within a psImage structure

        // psImageClip shall limit the minimum and maximum data value within a
        // psImage structure to a specified min and max value.
        //
        // Verify the returned integer is equal to the number of pixels clipped,
        // if the input psImage structure contains known values and input parameters
        // min and max have know values.
        //
        // Verify the psImage structure specified by the input parameter input is
        // modified to contain the expected values, if the input psImage structure
        // contains known values, min and max are specified and vmin and vmax
        // parameters are known.
        //
        // Verify the retuned integer is zero, psImage structure input is unmodified
        // and program executions doesn't stop, if input parameter psImage structure
        // pointer is NULL.
        //
        // Verify the retuned integer is zero, psImage structure input is unmodified
        // and program executions doesn't stop, if input parameter min is larger than max.
        // create image

        genericImageClipTest(8, 1);
        genericImageClipTest(1, 8);
        genericImageClipTest(8, 8);
        genericImageClipTest(8, 16);
        genericImageClipTest(16, 8);

        #define testImageClipByType(datatype) \
        { \
            psU32 c = 128; \
            psU32 r = 256; \
            psF64 min; \
            psF64 max; \
            psS32 numClipped = 0; \
            psS32 retVal; \
            psMemId id = psMemGetId(); \
            psImage *img = psImageAlloc(c,r,PS_TYPE_##datatype); \
            for (psU32 row=0;row<r;row++) { \
                ps##datatype* imgRow = img->data.datatype[row]; \
                for (psU32 col=0;col<c;col++) { \
                    imgRow[col] = (ps##datatype)(row+col); \
                } \
            } \
            min = (psF64)r/2.0; \
            max = (psF64)r; \
            \
            retVal = psImageClip(img,min,(double)PS_MIN_##datatype,max,(double)PS_MAX_##datatype); \
            \
            numClipped = 0; \
            bool errorFlag = false; \
            for (psU32 row=0;row<r;row++) { \
                ps##datatype* imgRow = img->data.datatype[row]; \
                for (psU32 col=0;col<c;col++) { \
                    ps##datatype value = (ps##datatype)(row+col); \
                    if (value < min) { \
                        numClipped++; \
                        value = PS_MIN_##datatype; \
                    } else if (value > max) { \
                        numClipped++; \
                        value = PS_MAX_##datatype; \
                    } \
                    if (fabsf(imgRow[col]-value) > FLT_EPSILON) { \
                        diag("Pixel value is not as expected (%g vs %g) at %u,%u", \
                             (psF64)imgRow[col],(psF64)value,col,row); \
                        errorFlag = true; \
                    } \
                } \
            } \
            ok(!errorFlag, "psImageClip() produced the correct data values"); \
            ok(retVal == numClipped, "Got the expected number of clips"); \
            psFree(img); \
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks"); \
        }

        testImageClipByType(F64);
        testImageClipByType(F32);
        testImageClipByType(S32);
        testImageClipByType(S16);
        testImageClipByType(S8);
        testImageClipByType(U32);
        testImageClipByType(U16);
        testImageClipByType(U8);

        psF64 min=0.0;
        psF64 max=0.0;
        psS32 retVal;
        psImage *img = NULL;
        // Verify the retuned integer is zero, psImage structure input is unmodified
        // and program executions doesn't stop, if input parameter psImage structure
        // pointer is NULL.
        retVal = psImageClip(NULL,min,-1.0f,max,-2.0f);
        ok(retVal == 0, "Expected zero return for clips of a NULL image");

        // Verify the retuned integer is zero, psImage structure input is unmodified
        // and program executions doesn't stop, if input parameter min is larger than max.
        // Following should be an error (max<min)
        // XXX: Verify error
        retVal = psImageClip(img,max,-1.0f,min,-2.0f);
        ok(retVal == 0, "Expected zero return for clips when max < min");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // testImageClipNAN()
    {
        psMemId id = psMemGetId();
        psImage* img = NULL;
        psU32 c = 128;
        psU32 r = 256;
        psS32 numClipped = 0;
        psS32 retVal;

        // psImageClipNaN shall modified pixel values of NaN with a specified value");

        // psImageClipNaN shall modify a psImage structure with pixel values set
        // to NaN to a value specified as an input parameter.
        //
        // Verify the returned integer is equal to the number of pixels modified
        // and the psImage is modified at locations where NaN pixels where
        // located to the value specified in the input parameter value.
        //
        // Verify the returned integer is zero and program execution doesn't stop,
        // if the input parameter psImage structure pointer is NULL.
        // create image
        #define testImageClipNaNByType(datatype) \
        { \
            psMemId id = psMemGetId(); \
            img = psImageAlloc(c,r,PS_TYPE_##datatype); \
            for (unsigned row=0;row<r;row++) { \
                ps##datatype* imgRow = img->data.datatype[row]; \
                for (unsigned col=0;col<c;col++) { \
                    if (row == col) { \
                        imgRow[col] = NAN; \
                    } else if (row+1 == col) { \
                        imgRow[col] = INFINITY; \
                    } else { \
                        imgRow[col] = (ps##datatype)(row+col); \
                    } \
                } \
            } \
            \
            retVal = psImageClipNaN(img,-1.0f); \
            \
            numClipped = 0; \
            bool errorFlag = false; \
            for (unsigned row=0;row<r;row++) { \
                ps##datatype* imgRow = img->data.datatype[row]; \
                for (unsigned col=0;col<c;col++) { \
                    ps##datatype value = (ps##datatype)(row+col); \
                    if ( (row == col) || (row+1 == col) ) { \
                        numClipped++; \
                        value = -1.0; \
                    } \
                    if (fabsf(imgRow[col]-value) > FLT_EPSILON) { \
                        diag("Pixel value is not as expected (%f vs %f) at %d,%d", \
                             imgRow[col],value,col,row); \
                        errorFlag = true; \
                    } \
                } \
            } \
            ok(!errorFlag, "psImageClip() produced the correct data values"); \
            ok(retVal == numClipped, "Got the expected number of clips"); \
            psFree(img); \
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks"); \
        }

        testImageClipNaNByType(F32);
        testImageClipNaNByType(F64);

        // Verify the retuned integer is zero, psImage structure input is unmodified
        // and program executions doesn't stop, if input parameter psImage structure
        // pointer is NULL.
        retVal = psImageClipNaN(NULL,-1.0f);
        ok(retVal == 0, "Expected zero return for clips of a NULL image");

        // Verify program execution doesn't stop if the input image type is something
        // other than F32, F64.
        img = psImageAlloc(c,r,PS_TYPE_S32);
        // Following should be an error (incorrect type)
        // XXX: Verify error
        retVal = psImageClipNaN(img,2.0f);
        ok(retVal == 0, "Expected zero return for clip of incorrect image type");
        psFree(img);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // testImageOverlay()
    {
        psMemId id = psMemGetId();
        psImage* img = NULL;
        psImage* img2 = NULL;
        psImage* img3 = NULL;
        psImage* img4 = NULL;
        psU32 c = 128;
        psU32 r = 256;
        psS32 retVal;

        // psImageSectionOverlay shall modified pixel values in a psImage structure to
        // be equal to the value of the originaldata and an overlay image with a
        // specified operation. Valid operations include =, +, -, *, /.
        //
        // Verify the returned integer is zero
        // and the input parameter psImage structure is modified at the specified
        // location and range with the given overlay image and the specified
        // function. Cases should include all the valid operations. Comparison of
        // expected values should include a delta to allow for testing on
        // different platforms.
        #define testOverlayTypeOP(DATATYPE,OP,OPSTRING) \
        { \
            psMemId id = psMemGetId(); \
            img = psImageAlloc(c,r,PS_TYPE_##DATATYPE); \
            for (unsigned row=0;row<r;row++) { \
                ps##DATATYPE* imgRow = img->data.DATATYPE[row]; \
                for (unsigned col=0;col<c;col++) { \
                    imgRow[col] = 6.0; \
                } \
            } \
            img2 = psImageAlloc(c/2,r/2,PS_TYPE_##DATATYPE); \
            for (unsigned row=0;row<r/2;row++) { \
                ps##DATATYPE* img2Row = img2->data.DATATYPE[row]; \
                for (unsigned col=0;col<c/2;col++) { \
                    img2Row[col] = 2.0; \
                } \
            } \
            retVal = psImageOverlaySection(img,img2,c/4,r/4,OPSTRING); \
            if (retVal == 0) { \
                psError(PS_ERR_UNKNOWN, true,"psImageOverlaySection returned zero with %s op", \
                        OPSTRING); \
                return 1; \
            } \
            bool errorFlag = false; \
            for (unsigned row=0;row<r;row++) { \
                ps##DATATYPE* imgRow = img->data.DATATYPE[row]; \
                ps##DATATYPE* img2Row = img2->data.DATATYPE[row]; \
                for (unsigned col=0;col<c;col++) { \
                    ps##DATATYPE val = 6.0; \
                    if ( ! (row < r/4 || row >= r/2+r/4 || col < c/4 || col >= c/2+c/4)) { \
                        val OP 2.0; \
                    } \
                    if (fabsf(imgRow[col] - val) > FLT_EPSILON) { \
                        diag("Value incorrect at %d,%d (%.2f vs %.2f for %s)", \
                             col,row,imgRow[col],val,OPSTRING); \
                        errorFlag = true; \
                    } \
                    if (row < r/2 && col < c/2 && fabsf(img2Row[col] - 2.0) > FLT_EPSILON) { \
                        diag("Overlay modified at %d,%d (%.2f for %s)", \
                             col,row,img2Row[col],OPSTRING); \
                        errorFlag = true; \
                    } \
                } \
            } \
            ok(!errorFlag, "psImageOverlaySection() produced the correct data values"); \
            psFree(img); \
            psFree(img2); \
            ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks"); \
        }

        #define testOverlayType(DATATYPE) \
        testOverlayTypeOP(DATATYPE,+=,"+"); \
        testOverlayTypeOP(DATATYPE,-=,"-"); \
        testOverlayTypeOP(DATATYPE,*=,"*");\
        testOverlayTypeOP(DATATYPE,/=,"/");\
        testOverlayTypeOP(DATATYPE,=,"=");

        testOverlayType(F64);
        testOverlayType(F32);
        testOverlayType(S16);
        testOverlayType(S8);
        testOverlayType(U16);
        testOverlayType(U8);

        // Verify the returned integer is equal to non-zero and the input psImage structure
        // is unmodified, if the overlay specified is not within the data range of the
        // input psImage structure.
        img = psImageAlloc(c,r,PS_TYPE_F32);
        for (unsigned row=0;row<r;row++)
        {
            psF32* imgRow = img->data.F32[row];
            for (unsigned col=0;col<c;col++) {
                imgRow[col] = 6.0f;
            }
        }
        img2 = psImageAlloc(c,r,PS_TYPE_F32);
        for (unsigned row=0;row<r;row++)
        {
            psF32* img2Row = img2->data.F32[row];
            for (unsigned col=0;col<c;col++) {
                img2Row[col] = 2.0f;
            }
        }
        img3 = psImageAlloc(c,r,PS_TYPE_S64);
        for (unsigned row=0;row<r;row++)
        {
            psS64* img3Row = img3->data.S64[row];
            for (unsigned col=0;col<c;col++) {
                img3Row[col] = 6.0f;
            }
        }
        img4 = psImageAlloc(c,r,PS_TYPE_S64);
        for (unsigned row=0;row<r;row++)
        {
            psS64* img4Row = img4->data.S64[row];
            for (unsigned col=0;col<c;col++) {
                img4Row[col] = 2.0f;
            }
        }

        // Following should error as overlay isn't within image boundaries
        // XXX: Verify error
        retVal = psImageOverlaySection(img,img2,c/4,r/4,"+");
        ok(retVal == 0, "psImageOverlaySection did return zero when overlay too big");

        bool errorFlag = false;
        for (unsigned row=0;row<r;row++)
        {
            psF32* imgRow = img->data.F32[row];
            for (unsigned col=0;col<c;col++) {
                if (imgRow[col] != 6.0f) {
                    diag("Input image modified when overlay size too big");
                    errorFlag = true;
                }
            }
        }
        ok(!errorFlag, "psImageOverlaySection() produced the correct data values");


        // Verify the returned integer is equal to non-zero, the input psImage
        // structure is unmodified and program execution doesn't stop, if the
        // overlay specified is NULL.
        // Following should error as overlay is NULL
        // XXX: Verify error
        retVal = psImageOverlaySection(img,NULL,c/4,r/4,"+");
        ok(retVal == 0, "psImageOverlaySection did return zero when overlay too big");
        errorFlag = false;
        for (unsigned row=0;row<r;row++)
        {
            psF32* imgRow = img->data.F32[row];
            for (unsigned col=0;col<c;col++) {
                if (imgRow[col] != 6.0f) {
                    diag("Input image modified when overlay NULL");
                    errorFlag = true;
                }
            }
        }
        ok(!errorFlag, "psImageOverlaySection() produced the correct data values");


        // Verify the returned integer is equal to non-zero and program execution
        // doesn't stop, if the input parameter image is NULL.
        // Following should error as image input is NULL
        // XXX: Verify error
        retVal = psImageOverlaySection(NULL,img2,c/4,r/4,"+");
        ok(retVal == 0, "psImageOverlaySection returned zero when overlay too big");


        // Verify the return integer is equal to non-zero and program execution
        // doesn't stop, if the specified operator is not =,+,-,*,/
        // Following should error as operator is incorrect
        // XXX: Verify error
        retVal = psImageOverlaySection(img,img2,0,0,"$");
        ok(retVal == 0, "psImageOverlaySection returned zero when overlay too big");


        // Verify the return integer is equal to non-zero and program execution
        // doesn't stop, if the specified operator is NULL
        // Following should error as operator is incorrect
        // XXX: Verify error
        retVal = psImageOverlaySection(img,img2,0,0,NULL);
        ok(retVal == 0, "psImageOverlaySection returned zero when overlay operator is NULL");


        // Verify the return integer is equal to non-zero and program execution
        // doesn't stop, if overlay image is a different type than the input image
        // Following should error as overlay is a different type
        // XXX: Verify error
        retVal = psImageOverlaySection(img,img3,0,0,"+");
        ok(retVal == 0, "psImageOverlaySection returned zero when overlay image type is different than input image");


        // Verify program execution doen't stop, if the overly image contains
        // zero values with division operation is specified.
        // XXX: This currently doesn't work.  Apparently, the psImageOverlaySection()
        // will happily divide by 0.0.
        for (unsigned row=0;row<r;row++)
        {
            psF32* img2Row = img2->data.F32[row];
            for (unsigned col=0;col<c;col++) {
                img2Row[col] = 0.0f;
            }
        }
        retVal = psImageOverlaySection(img,img2,0,0,"/");
        ok(retVal != 0, "psImageOverlaySection returned non-zero when checking divide-by-zero");
        errorFlag = false;

        for (unsigned row=0;row<r;row++)
        {
            for (unsigned col=0;col<c;col++) {
                if (!isnan(img->data.F32[row][col])) {
                    if (VERBOSE) diag("img[%d][%d] is %f, should be NAN\n", row, col, img->data.F32[row][col]);
                    errorFlag = true;
                }
            }
        }
        ok(!errorFlag, "psImageOverlaySection() properly set data to NANs");

        psFree(img);
        psFree(img2);
        psFree(img3);
        psFree(img4);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
}
