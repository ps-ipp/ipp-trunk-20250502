/** @file  tst_psImageManip.c
 *
 *  @brief Contains the tests for psImageManip.[ch]
 *
 *
 *  @author Robert DeSonia, MHPCC
 *
 *  @version $Revision: 1.3 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2005-10-06 02:41:07 $
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

static psS32 testImageClip(void);
static psS32 testImageClipNAN(void);
static psS32 testImageClipComplexRegion(void);
static psS32 testImageOverlay(void);

testDescription tests[] = {
                              {testImageClip,571,"psImageClip",0,false},
                              {testImageClipNAN,572,"psImageClipNAN",0,false},
                              {testImageClipComplexRegion,673,"psImageClipComplexRegion",0,false},
                              {testImageOverlay,573,"psImageOverlay",0,false},
                              {NULL}
                          };

psS32 main(psS32 argc, char* argv[])
{
    psLogSetLevel(PS_LOG_INFO);

    return ! runTestSuite(stderr,"psImage",tests,argc,argv);
}


psS32 testImageClip(void)
{
    psImage* img = NULL;
    psU32 c = 128;
    psU32 r = 256;
    psF64 min;
    psF64 max;
    psS32 numClipped = 0;
    psS32 retVal;

    psLogMsg(__func__,PS_LOG_INFO,
             "psImageClip shall limit the minimum and maximum data value within a psImage structure");

    /*

        psImageClip shall limit the minimum and maximum data value within a
        psImage structure to a specified min and max value.

        Verify the returned integer is equal to the number of pixels clipped,
        if the input psImage structure contains known values and input parameters
        min and max have know values.

        Verify the psImage structure specified by the input parameter input is
        modified to contain the expected values, if the input psImage structure
        contains known values, min and max are specified and vmin and vmax
        parameters are known.

        Verify the retuned integer is zero, psImage structure input is unmodified
        and program executions doesn't stop, if input parameter psImage structure
        pointer is null.

        Verify the retuned integer is zero, psImage structure input is unmodified
        and program executions doesn't stop, if input parameter min is larger than max.
    */

    // create image
    #define testImageClipByType(datatype) \
    img = psImageAlloc(c,r,PS_TYPE_##datatype); \
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
                psError(PS_ERR_UNKNOWN, true,"Pixel value is not as expected (%g vs %g) at %u,%u", \
                        (psF64)imgRow[col],(psF64)value,col,row); \
                return 1; \
            } \
        } \
    } \
    if (retVal != numClipped) { \
        psError(PS_ERR_UNKNOWN, true,"Expected %d clips, but got %d", \
                numClipped,retVal); \
        return 2; \
    } \
    psFree(img);

    #define testImageClipByComplexType(datatype) \
    img = psImageAlloc(c,r,PS_TYPE_##datatype); \
    for (psU32 row=0;row<r;row++) { \
        ps##datatype* imgRow = img->data.datatype[row]; \
        for (psU32 col=0;col<c;col++) { \
            imgRow[col] = (ps##datatype)(row+I*col); \
        } \
    } \
    min = (float)r/2.0f; \
    max = (float)r; \
    \
    retVal = psImageClip(img,min,-1.0f,max,-2.0f); \
    \
    numClipped = 0; \
    for (psU32 row=0;row<r;row++) { \
        ps##datatype* imgRow = img->data.datatype[row]; \
        for (psU32 col=0;col<c;col++) { \
            ps##datatype value = row+I*col; \
            if (cabs(value) < min) { \
                numClipped++; \
                value = -1.0f; \
            } else if (cabs(value) > max) { \
                numClipped++; \
                value = -2.0f; \
            } \
            if (fabsf(creal(imgRow[col])-creal(value)) > FLT_EPSILON || \
                    fabsf(cimag(imgRow[col])-cimag(value)) > FLT_EPSILON) { \
                psError(PS_ERR_UNKNOWN, true,"Pixel value is not as expected (%.2f+%.2fi vs %.2f+%.2fi) at %u,%u", \
                        creal(imgRow[col]),cimag(imgRow[col]),creal(value),cimag(value),col,row); \
                return 1; \
            } \
        } \
    } \
    if (retVal != numClipped) { \
        psError(PS_ERR_UNKNOWN, true,"Expected %d clips, but got %d", \
                numClipped,retVal); \
        return 2; \
    } \
    psFree(img);

    psLogMsg(__func__,PS_LOG_INFO,"Testing clipping of F64 imagery");
    testImageClipByType(F64);
    psLogMsg(__func__,PS_LOG_INFO,"Testing clipping of F32 imagery");
    testImageClipByType(F32);
    psLogMsg(__func__,PS_LOG_INFO,"Testing clipping of S32 imagery");
    testImageClipByType(S32);
    psLogMsg(__func__,PS_LOG_INFO,"Testing clipping of S16 imagery");
    testImageClipByType(S16);
    psLogMsg(__func__,PS_LOG_INFO,"Testing clipping of S8 imagery");
    testImageClipByType(S8);
    //    psLogMsg(__func__,PS_LOG_INFO,"Testing clipping of U32 imagery");
    //    testImageClipByType(U32);
    psLogMsg(__func__,PS_LOG_INFO,"Testing clipping of U16 imagery");
    testImageClipByType(U16);
    psLogMsg(__func__,PS_LOG_INFO,"Testing clipping of U8 imagery");
    testImageClipByType(U8);
    psLogMsg(__func__,PS_LOG_INFO,"Testing clipping of C32 imagery");
    testImageClipByComplexType(C32);
    psLogMsg(__func__,PS_LOG_INFO,"Testing clipping of C64 imagery");
    testImageClipByComplexType(C64);

    // Verify the retuned integer is zero, psImage structure input is unmodified
    // and program executions doesn't stop, if input parameter psImage structure
    // pointer is null.
    retVal = psImageClip(NULL,min,-1.0f,max,-2.0f);
    if (retVal != 0) {
        psError(PS_ERR_UNKNOWN, true,"Expected zero return for clips of a NULL image.");
        return 3;
    }

    // Verify the retuned integer is zero, psImage structure input is unmodified
    // and program executions doesn't stop, if input parameter min is larger than max.
    psLogMsg(__func__,PS_LOG_INFO,"Following should be an error (max<min)");
    retVal = psImageClip(img,max,-1.0f,min,-2.0f);
    if (retVal != 0) {
        psError(PS_ERR_UNKNOWN, true,"Expected zero return for clips when max < min.");
        return 4;
    }

    return 0;
}

psS32 testImageClipNAN(void)
{
    psImage* img = NULL;
    psU32 c = 128;
    psU32 r = 256;
    psS32 numClipped = 0;
    psS32 retVal;

    psLogMsg(__func__,PS_LOG_INFO,
             "psImageClipNaN shall modified pixel values of NaN with a specified value");

    /*
        psImageClipNaN shall modify a psImage structure with pixel values set
        to NaN to a value specified as an input parameter.

        Verify the returned integer is equal to the number of pixels modified
        and the psImage is modified at locations where NaN pixels where
        located to the value specified in the input parameter value.

        Verify the returned integer is zero and program execution doesn't stop,
        if the input parameter psImage structure pointer is null.
    */

    // create image
    #define testImageClipNaNByType(datatype) \
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
    for (unsigned row=0;row<r;row++) { \
        ps##datatype* imgRow = img->data.datatype[row]; \
        for (unsigned col=0;col<c;col++) { \
            ps##datatype value = (ps##datatype)(row+col); \
            if ( (row == col) || (row+1 == col) ) { \
                numClipped++; \
                value = -1.0; \
            } \
            if (fabsf(imgRow[col]-value) > FLT_EPSILON) { \
                psError(PS_ERR_UNKNOWN, true,"Pixel value is not as expected (%f vs %f) at %d,%d", \
                        imgRow[col],value,col,row); \
                return 1; \
            } \
        } \
    } \
    if (retVal != numClipped) { \
        psError(PS_ERR_UNKNOWN, true,"Expected %d clips, but got %d", \
                numClipped,retVal); \
        return 2; \
    } \
    psFree(img);

    testImageClipNaNByType(F32);
    testImageClipNaNByType(F64);
    testImageClipNaNByType(C32);
    testImageClipNaNByType(C64);

    // Verify the retuned integer is zero, psImage structure input is unmodified
    // and program executions doesn't stop, if input parameter psImage structure
    // pointer is null.
    retVal = psImageClipNaN(NULL,-1.0f);
    if (retVal != 0) {
        psError(PS_ERR_UNKNOWN, true,"Expected zero return for clips of a NULL image.");
        return 3;
    }

    // Verify program execution doesn't stop if the input image type is something
    // other than F32, F64, C32, C64.
    img = psImageAlloc(c,r,PS_TYPE_S32);
    psLogMsg(__func__,PS_LOG_INFO,"Following should be an error (invalid type)");
    retVal = psImageClipNaN(img,2.0f);
    if (retVal != 0) {
        psError(PS_ERR_UNKNOWN, true,"Expected zero return for clip of invalid image type.");
        return 4;
    }
    psFree(img);

    return 0;
}

psS32 testImageClipComplexRegion(void)
{
    psImage* img = NULL;
    psU32 c = 1024;
    psU32 r = 2048;
    psS32 numClipped = 0;
    psS32 retVal;

    psLogMsg(__func__,PS_LOG_INFO,
             "psImageClipNaN shall modified pixel values of NaN with a specified value");

    /*
    1. Create a complex image with a wide range of complex values

    2. call psImageClipComplexRegion with min and max where there is at least
       2 pixels in the image above) that is:
        a) real(p) < real(min) && complex(p) < complex(min),
        b) real(p) < real(min) && complex(min) < complex(p) < complex(max)
        c) real(min) < real(p) < real(max) && complex(p) < complex(min)
        d) real(min) < real(p) < real(max) && complex(min) < complex(p) < complex(max)
        e) real(p) > real(max) && complex(min) < complex(p) < complex(max)
        f) real(pmin) < real(p) < real(max) && complex(p) > complex(max)
        g) real(p) > real(max) && complex(p) > complex(max)
        h) real(p) < real(min) && complex(p) > complex(max)
        i) real(p) > real(max) && complex(p) < complex(min)

    3. verify that All pixels in case (a), (b), and (c) have the value vmin

    4. verify that all pixels in case (d) are unchanged from input

    5. verify that all pixels in case (e), (f), (g), (h), and (i) have the
       value vmax

    */

    #define testImageClipComplexByType(datatype,MIN,MAX) /* datatype must be complex */ \
    /* create image */ \
    img = psImageAlloc(c,r,PS_TYPE_##datatype); \
    for (unsigned row=0;row<r;row++) { \
        ps##datatype* imgRow = img->data.datatype[row]; \
        for (unsigned col=0;col<c;col++) { \
            imgRow[col] = row+I*col; \
        } \
    } \
    \
    retVal = psImageClipComplexRegion(img,MIN,-1.0-1.0*I,MAX,-2.0-2.0*I); \
    \
    numClipped = 0; \
    for (unsigned row=0;row<r;row++) { \
        ps##datatype* imgRow = img->data.datatype[row]; \
        for (unsigned col=0;col<c;col++) { \
            ps##datatype value = (ps##datatype)(row+I*col); \
            if ( (row > creal(MAX)) || (col > cimag(MAX)) ) { \
                numClipped++; \
                value = -2.0-2.0*I; \
            } else if ((row < creal(MIN)) || (col < cimag(MIN)) ) { \
                numClipped++; \
                value = -1.0-1.0*I; \
            } \
            if (cabs(imgRow[col]-value) > FLT_EPSILON) { \
                psError(PS_ERR_UNKNOWN, true,"Pixel value is not as expected (%g%+gi vs %g%+gi) at %d,%d", \
                        creal(imgRow[col]),cimag(imgRow[col]),creal(value),cimag(value),col,row); \
                return 1; \
            } \
        } \
    } \
    if (retVal != numClipped) { \
        psError(PS_ERR_UNKNOWN, true,"Expected %d clips, but got %d", \
                numClipped,retVal); \
        return 2; \
    } \
    psFree(img);

    complex double min = ((double)r)/5.0+I*((double)c)/4.0;
    complex double max = ((double)r)/3.0+I*((double)c)/2.0;

    psLogMsg(__func__,PS_LOG_INFO,"Testing clipping at %g%+gi to %g%+gi for psC32",
             creal(min),cimag(min),creal(max),cimag(max));

    testImageClipComplexByType(C32,min,max);

    psLogMsg(__func__,PS_LOG_INFO,"Testing clipping at %g%+gi to %g%+gi for psC64",
             creal(min),cimag(min),creal(max),cimag(max));
    testImageClipComplexByType(C64,min,max);

    //  6. Call psImageClipComplexRegion with NULL input parameter; should error
    //     but not stop execution.

    psLogMsg(__func__,PS_LOG_INFO,"Following should be an error:");
    retVal = psImageClipComplexRegion(NULL,0,0,0,0);
    if (retVal != 0) {
        psError(PS_ERR_UNKNOWN, true,"Expected zero return for clips of a NULL image.");
        return 3;
    }


    img = psImageAlloc(c,r,PS_TYPE_C32);
    for (unsigned row=0;row<r;row++) {
        psC32* imgRow = img->data.C32[row];
        for (unsigned col=0;col<c;col++) {
            imgRow[col] = row+I*col;
        }
    }

    //  7. Call psImageClipComplexRegion with min > max; should error and return 0,
    //     but not stop execution

    psLogMsg(__func__,PS_LOG_INFO,"Following should be an error:");
    retVal = psImageClipComplexRegion(img,10.0+I*1.0,-1.0,5.0+5.0*I,-2.0);
    if (retVal != 0) {
        psError(PS_ERR_UNKNOWN, true,"Expected zero return for creal(min)>creal(max).");
        return 3;
    }
    psLogMsg(__func__,PS_LOG_INFO,"Following should be an error:");
    retVal = psImageClipComplexRegion(img,1.0+I*10.0,-1.0,5.0+5.0*I,-2.0);
    if (retVal != 0) {
        psError(PS_ERR_UNKNOWN, true,"Expected zero return for cimag(min)>cimag(max).");
        return 3;
    }
    psLogMsg(__func__,PS_LOG_INFO,"Following should be an error:");
    retVal = psImageClipComplexRegion(img,10.0+I*10.0,-1.0,5.0+5.0*I,-2.0);
    if (retVal != 0) {
        psError(PS_ERR_UNKNOWN, true,"Expected zero return for min>max.");
        return 3;
    }

    //  8. Call psImageClipComplexRegion with the follow vmin/vmax values; each
    //     should error and return 0, but not stop execution
    //      a) vmin < datatype region's minimum
    //      b) vmax < datatype region's minimum
    //      c) vmin > datatype region's maximum
    //      d) vmax > datatype region's maximum

    psLogMsg(__func__,PS_LOG_INFO,"Following should be an error:");
    retVal = psImageClipComplexRegion(img,
                                      1.0+I*1.0,
                                      -2.0*(double)FLT_MAX,
                                      5.0+5.0*I,
                                      0.0);
    if (retVal != 0) {
        psError(PS_ERR_UNKNOWN, true,"Expected zero return for vmin not in datatype range.");
        return 80;
    }

    psLogMsg(__func__,PS_LOG_INFO,"Following should be an error:");
    retVal = psImageClipComplexRegion(img,
                                      1.0+I*1.0,
                                      2.0*(double)FLT_MAX,
                                      5.0+5.0*I,
                                      0.0);
    if (retVal != 0) {
        psError(PS_ERR_UNKNOWN, true,"Expected zero return for vmin not in datatype range.");
        return 81;
    }

    psLogMsg(__func__,PS_LOG_INFO,"Following should be an error:");
    retVal = psImageClipComplexRegion(img,
                                      1.0+I*1.0,
                                      FLT_EPSILON-2.0*(double)FLT_MAX*I,
                                      5.0+5.0*I,
                                      0.0);
    if (retVal != 0) {
        psError(PS_ERR_UNKNOWN, true,"Expected zero return for vmin not in datatype range.");
        return 82;
    }

    psLogMsg(__func__,PS_LOG_INFO,"Following should be an error:");
    retVal = psImageClipComplexRegion(img,
                                      1.0+I*1.0,
                                      FLT_EPSILON+2.0*(double)FLT_MAX*I,
                                      5.0+5.0*I,
                                      0.0);
    if (retVal != 0) {
        psError(PS_ERR_UNKNOWN, true,"Expected zero return for vmin not in datatype range.");
        return 83;
    }

    psLogMsg(__func__,PS_LOG_INFO,"Following should be an error:");
    retVal = psImageClipComplexRegion(img,
                                      1.0+I*1.0,
                                      0.0,
                                      5.0+5.0*I,
                                      -2.0*(double)FLT_MAX);
    if (retVal != 0) {
        psError(PS_ERR_UNKNOWN, true,"Expected zero return for vmax not in datatype range.");
        return 84;
    }

    psLogMsg(__func__,PS_LOG_INFO,"Following should be an error:");
    retVal = psImageClipComplexRegion(img,
                                      1.0+I*1.0,
                                      0.0,
                                      5.0+5.0*I,
                                      2.0*(double)FLT_MAX);
    if (retVal != 0) {
        psError(PS_ERR_UNKNOWN, true,"Expected zero return for vmax not in datatype range.");
        return 85;
    }

    psLogMsg(__func__,PS_LOG_INFO,"Following should be an error:");
    retVal = psImageClipComplexRegion(img,
                                      1.0+I*1.0,
                                      0.0,
                                      5.0+5.0*I,
                                      FLT_EPSILON-2.0*(double)FLT_MAX*I);
    if (retVal != 0) {
        psError(PS_ERR_UNKNOWN, true,"Expected zero return for vmax not in datatype range.");
        return 87;
    }

    psLogMsg(__func__,PS_LOG_INFO,"Following should be an error:");
    retVal = psImageClipComplexRegion(img,
                                      1.0+I*1.0,
                                      0.0,
                                      5.0+5.0*I,
                                      FLT_EPSILON+2.0*(double)FLT_MAX*I);
    if (retVal != 0) {
        psError(PS_ERR_UNKNOWN, true,"Expected zero return for vmax not in datatype range.");
        return 88;
    }


    // now check if vmin > vmax is OK
    for (unsigned row=0;row<r;row++) {
        psC32* imgRow = img->data.C32[row];
        for (unsigned col=0;col<c;col++) {
            imgRow[col] = row+I*col;
        }
    }
    retVal = psImageClipComplexRegion(img,
                                      1.0+I*1.0,
                                      10.0,
                                      5.0+5.0*I,
                                      0.0);
    if (retVal == 0) {
        psError(PS_ERR_UNKNOWN, true,"Didn't expect zero return for vmin > vmax.");
        return 83;
    }


    psFree(img);
    img = NULL;

    //  9. Call psImageClipComplexRegion with the max value out of datatype's
    //     range; should clip as expected (see step 1-5). Repeat with min value
    //     out of datatype's range.

    testImageClipComplexByType(C32,-(double)FLT_MAX*2.0-I*(double)FLT_MAX*2.0,10.0+I*10.0);
    testImageClipComplexByType(C32,10.0+I*10.0,(double)FLT_MAX*2.0+I*(double)FLT_MAX*2.0);

    // Verify program execution doesn't stop if the input image type is something
    // other than C32, C64.
    img = psImageAlloc(c,r,PS_TYPE_S32);
    psLogMsg(__func__,PS_LOG_INFO,"Following should be an error (invalid type)");
    retVal = psImageClipComplexRegion(img,2.0,10.0,5.0,0.0);
    if (retVal != 0) {
        psError(PS_ERR_UNKNOWN, true,"Expected zero return for clip of invalid image type.");
        return 84;
    }
    psFree(img);

    return 0;
}

psS32 testImageOverlay(void)
{

    psImage* img = NULL;
    psImage* img2 = NULL;
    psImage* img3 = NULL;
    psImage* img4 = NULL;
    psU32 c = 128;
    psU32 r = 256;
    psS32 retVal;

    /*
    psImageSectionOverlay shall modified pixel values in a psImage structure to
    be equal to the value of the originaldata and an overlay image with a
    specified operation. Valid operations include =, +, -, *, /.

    Verify the returned integer is zero
    and the input parameter psImage structure is modified at the specified
    location and range with the given overlay image and the specified
    function. Cases should include all the valid operations. Comparison of
    expected values should include a delta to allow for testing on
    different platforms.

    */

    #define testOverlayTypeOP(DATATYPE,OP,OPSTRING) \
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
    for (unsigned row=0;row<r;row++) { \
        ps##DATATYPE* imgRow = img->data.DATATYPE[row]; \
        ps##DATATYPE* img2Row = img2->data.DATATYPE[row]; \
        for (unsigned col=0;col<c;col++) { \
            ps##DATATYPE val = 6.0; \
            if ( ! (row < r/4 || row >= r/2+r/4 || col < c/4 || col >= c/2+c/4)) { \
                val OP 2.0; \
            } \
            if (fabsf(imgRow[col] - val) > FLT_EPSILON) { \
                psError(PS_ERR_UNKNOWN, true,"Value incorrect at %d,%d (%.2f vs %.2f for %s)", \
                        col,row,imgRow[col],val,OPSTRING); \
                return 2; \
            } \
            if (row < r/2 && col < c/2 && fabsf(img2Row[col] - 2.0) > FLT_EPSILON) { \
                psError(PS_ERR_UNKNOWN, true,"Overlay modified at %d,%d (%.2f for %s)", \
                        col,row,img2Row[col],OPSTRING); \
                return 2; \
            } \
        } \
    } \
    psFree(img); \
    psFree(img2);

    #define testOverlayType(DATATYPE) \
    testOverlayTypeOP(DATATYPE,+=,"+"); \
    testOverlayTypeOP(DATATYPE,-=,"-"); \
    testOverlayTypeOP(DATATYPE,*=,"*");\
    testOverlayTypeOP(DATATYPE,/=,"/");\
    testOverlayTypeOP(DATATYPE,=,"=");

    testOverlayType(C64);
    testOverlayType(C32);
    testOverlayType(F64);
    testOverlayType(F32);
    testOverlayType(S16);
    testOverlayType(S8);
    testOverlayType(U16);
    testOverlayType(U8);

    /*
    Verify the returned integer is equal to non-zero and the input psImage structure
    is unmodified, if the overlay specified is not within the data range of the
    input psImage structure.
    */

    img = psImageAlloc(c,r,PS_TYPE_F32);
    for (unsigned row=0;row<r;row++) {
        psF32* imgRow = img->data.F32[row];
        for (unsigned col=0;col<c;col++) {
            imgRow[col] = 6.0f;
        }
    }
    img2 = psImageAlloc(c,r,PS_TYPE_F32);
    for (unsigned row=0;row<r;row++) {
        psF32* img2Row = img2->data.F32[row];
        for (unsigned col=0;col<c;col++) {
            img2Row[col] = 2.0f;
        }
    }
    img3 = psImageAlloc(c,r,PS_TYPE_S64);
    for (unsigned row=0;row<r;row++) {
        psS64* img3Row = img3->data.S64[row];
        for (unsigned col=0;col<c;col++) {
            img3Row[col] = 6.0f;
        }
    }
    img4 = psImageAlloc(c,r,PS_TYPE_S64);
    for (unsigned row=0;row<r;row++) {
        psS64* img4Row = img4->data.S64[row];
        for (unsigned col=0;col<c;col++) {
            img4Row[col] = 2.0f;
        }
    }

    psLogMsg(__func__,PS_LOG_INFO,"Following should error as overlay isn't "
             "within image boundaries");
    retVal = psImageOverlaySection(img,img2,c/4,r/4,"+");
    if (retVal != 0) {
        psError(PS_ERR_UNKNOWN, true,"psImageOverlaySection did not return "
                "zero even though overlay too big");
        return 3;
    }
    for (unsigned row=0;row<r;row++) {
        psF32* imgRow = img->data.F32[row];
        for (unsigned col=0;col<c;col++) {
            if (imgRow[col] != 6.0f) {
                psError(PS_ERR_UNKNOWN, true,"Input image modified when overlay size too big");
                return 4;
            }
        }
    }

    /*
    Verify the returned integer is equal to non-zero, the input psImage
    structure is unmodified and program execution doesn't stop, if the
    overlay specified is null.
    */

    psLogMsg(__func__,PS_LOG_INFO,"Following should error as overlay is NULL");
    retVal = psImageOverlaySection(img,NULL,c/4,r/4,"+");
    if (retVal != 0) {
        psError(PS_ERR_UNKNOWN, true,"psImageOverlaySection did not return "
                "zero even though overlay too big");
        return 5;
    }
    for (unsigned row=0;row<r;row++) {
        psF32* imgRow = img->data.F32[row];
        for (unsigned col=0;col<c;col++) {
            if (imgRow[col] != 6.0f) {
                psError(PS_ERR_UNKNOWN, true,"Input image modified when overlay NULL");
                return 6;
            }
        }
    }

    /*
    Verify the returned integer is equal to non-zero and program execution
    doesn't stop, if the input parameter image is null.
    */

    psLogMsg(__func__,PS_LOG_INFO,"Following should error as image input is NULL");
    retVal = psImageOverlaySection(NULL,img2,c/4,r/4,"+");
    if (retVal != 0) {
        psError(PS_ERR_UNKNOWN, true,"psImageOverlaySection returned non-zero even though "
                "overlay too big");
        return 7;
    }

    /*
    Verify the return integer is equal to non-zero and program execution
    doesn't stop, if the specified operator is not =,+,-,*,/
    */

    psLogMsg(__func__,PS_LOG_INFO,"Following should error as operator is invalid");
    retVal = psImageOverlaySection(img,img2,0,0,"$");
    if (retVal != 0) {
        psError(PS_ERR_UNKNOWN, true,"psImageOverlaySection returned non-zero even though "
                "overlay operator is invalid");
        return 8;
    }

    /*
    Verify the return integer is equal to non-zero and program execution
    doesn't stop, if the specified operator is NULL
    */

    psLogMsg(__func__,PS_LOG_INFO,"Following should error as operator is invalid");
    retVal = psImageOverlaySection(img,img2,0,0,NULL);
    if(retVal != 0) {
        psError(PS_ERR_UNKNOWN, true,"psImageOverlaySection returned non-zero even though "
                "overlay operator is NULL");
        return 9;
    }

    /*
    Verify the return integer is equal to non-zero and program execution
    doesn't stop, if overlay image is a different type than the input image
    */
    psLogMsg(__func__,PS_LOG_INFO,"Following should error as overlay is "
             "a different type");
    retVal = psImageOverlaySection(img,img3,0,0,"+");
    if(retVal != 0) {
        psError(PS_ERR_UNKNOWN, true,"psImageOverlaySection returned nonzero eventhough "
                " overlay image type is different than input image.");
        return 10;
    }

    /*
    Verify program execution doen't stop, if the overly image contains
    zero values with division operation is specified.
    */
    for (unsigned row=0;row<r;row++) {
        psF32* img2Row = img2->data.F32[row];
        for (unsigned col=0;col<c;col++) {
            img2Row[col] = 0.0f;
        }
    }
    retVal = psImageOverlaySection(img,img2,0,0,"/");
    if (retVal == 0) {
        psError(PS_ERR_UNKNOWN, true,"psImageOverlaySection returned zero when "
                "checking divide-by-zero.");
        return 12;
    }

    psFree(img);
    psFree(img2);
    psFree(img3);
    psFree(img4);

    return 0;
}
