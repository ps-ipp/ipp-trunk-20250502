/** @file  tst_psImageExtraction.c
*
*  @brief Contains the tests for psImageExtraction.[ch]
*
*
*  @author Robert DeSonia, MHPCC
*
*  @version $Revision: 1.11 $ $Name: not supported by cvs2svn $
*  @date $Date: 2006-05-16 23:12:20 $
*
*  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
*/
#include<stdlib.h>
#include<string.h>

#include "psTest.h"
#include "pslib_strict.h"
#include "psType.h"

static psS32 testImageSlice(void);
static psS32 testImageCut(void);
static psS32 testImageRadialCut(void);
static psS32 testImageRowColError(void);
static psS32 testImageRowColF32(void);
static psS32 testImageRowColF64(void);
static psS32 testImageRowColS8(void);
static psS32 testImageRowColS16(void);
static psS32 testImageRowColS32(void);
static psS32 testImageRowColS64(void);
static psS32 testImageRowColU8(void);
static psS32 testImageRowColU16(void);
static psS32 testImageRowColU32(void);
static psS32 testImageRowColU64(void);



testDescription tests[] = {
                              {testImageSlice, 552, "psImageSlice", 0, false},
                              {testImageCut, 555, "psImageCut", 0, false},
                              {testImageRadialCut, 556, "psImageRadialCut", 0, false},
                              {testImageRowColError, 557, "testImageRowColError", 0, false},
                              {testImageRowColF32, 558, "psImageRowColF32", 0, false},
                              {testImageRowColF64, 559, "psImageRowColF64", 0, false},
                              {testImageRowColU8, 560, "psImageRowColU8", 0, false},
                              {testImageRowColU16, 561, "psImageRowColU16", 0, false},
                              {testImageRowColU32, 562, "psImageRowColU32", 0, false},
                              {testImageRowColU64, 563, "psImageRowColU64", 0, false},
                              {testImageRowColS8, 564, "psImageRowColS8", 0, false},
                              {testImageRowColS16, 565, "psImageRowColS16", 0, false},
                              {testImageRowColS32, 566, "psImageRowColS32", 0, false},
                              {testImageRowColS64, 567, "psImageRowColS64", 0, false},
                              {NULL}
                          };

psS32 main( psS32 argc, char* argv[] )
{
    return ! runTestSuite( stderr, "psImage", tests, argc, argv );
}

psS32 testImageSlice(void)
{
    const psS32 r = 200;
    const psS32 c = 300;
    const psS32 m = r / 2 -1;
    const psS32 n = r / 4 -1;
    psVector* out = NULL;
    psImage* image;
    psPixels* positions = psPixelsAlloc( r );
    psImage* mask = psImageAlloc( c, r, PS_TYPE_MASK );
    psStats* stat = psStatsAlloc( PS_STAT_SAMPLE_MEDIAN );

    /*
        This function shall extract pixels from a specified region of a psImage
        structure into a vector using a specified statistical method.

        Verify the returned psVector structure contains expected data, if the
        input psImage input has known data and the psStats structure specifies
        a known statistical method. At least two different statistical methods
        should be used within a valid range of psImage data. Allow for a delta
        when comparing results to allow for testing a different platforms. Two
        cases should be used for each possible direction. Data region cases
        should include 0x0, 1x1, Nx1, 1xN, NxN, MxN

     */

    for ( psS32 row = 0;row < r;row++ ) {
        psMaskType* maskRow = mask->data.PS_TYPE_MASK_DATA[row];
        for ( psS32 col = 0;col < c;col++ ) {
            maskRow[ col ] = 0;
        }
    }

    #define PSIMAGESLICE_TEST1(TYPE,M,N,DIRECTION,TRUTH_SIZE,TRUTHPIX_X,TRUTHPIX_Y,TESTNUM) \
    image = psImageAlloc( c, r, PS_TYPE_##TYPE ); \
    for ( psS32 row = 0;row < r;row++ ) { \
        ps##TYPE *imageRow = image->data.TYPE[ row ]; \
        ps##TYPE rowOffset = row * 2; \
        for ( psS32 col = 0;col < c;col++ ) { \
            imageRow[ col ] = col + rowOffset; \
        } \
    } \
    image->col0 = 1; \
    image->row0 = 1; \
    out = psImageSlice(out,positions,image,mask,1, \
                       psRegionSet(1+c/10,1+c/10+M,1+r/10,1+r/10+N),DIRECTION,stat); \
    \
    if (out->n != TRUTH_SIZE) { \
        psError(PS_ERR_UNKNOWN,true,"Number of results is wrong (%d, not %d)", \
                out->n,TRUTH_SIZE); \
        return TESTNUM*4+1; \
    } \
    \
    if (positions->n != TRUTH_SIZE) { \
        psError(PS_ERR_UNKNOWN,true,"Number of results for positions vector is wrong (%d, not %d)", \
                positions->n,TRUTH_SIZE); \
        return TESTNUM*4+2; \
    } \
    \
    for (psS32 i=0;i<out->n;i++) { \
        if (fabs(out->data.F64[i]-image->data.TYPE[r/10+TRUTHPIX_Y][c/10+TRUTHPIX_X]) > 1.0/(psF64)r) { \
            psError(PS_ERR_UNKNOWN,true,"Improper result at position %d.  Got %g, expected %g",i, \
                    out->data.F64[i],image->data.TYPE[r/10+TRUTHPIX_Y][c/10+TRUTHPIX_X]); \
            return TESTNUM*4+3; \
        } \
        if (DIRECTION == PS_CUT_X_POS || DIRECTION == PS_CUT_X_NEG) { \
            if (positions->data[i].x != c/10+TRUTHPIX_X) { \
                psError(PS_ERR_UNKNOWN,true,"Improper positions (%d vs %d) result @ %d.", \
                        positions->data[i].x,c/10+TRUTHPIX_X,i); \
                return TESTNUM*4+4; \
            } \
        } else { \
            if (positions->data[i].y != r/10+TRUTHPIX_Y) { \
                psError(PS_ERR_UNKNOWN,true,"Improper positions (%d vs %d) result @ %d.", \
                        positions->data[i].y,r/10+TRUTHPIX_Y,i); \
                return TESTNUM*4+4; \
            } \
        } \
    } \
    psFree(image);

    #define PSIMAGESLICE_TEST(TYPE) \
    /* test MxN case */ \
    PSIMAGESLICE_TEST1(TYPE, m, n, PS_CUT_X_POS, m, i, n / 2, 0 ); \
    PSIMAGESLICE_TEST1(TYPE, m, n, PS_CUT_X_NEG, m, m - 1 - i, n / 2, 1 ); \
    PSIMAGESLICE_TEST1(TYPE, m, n, PS_CUT_Y_POS, n, m / 2, i, 2 ); \
    PSIMAGESLICE_TEST1(TYPE, m, n, PS_CUT_Y_NEG, n, m / 2, n - 1 - i, 3 ); \
    \
    /* test Mx1 case */ \
    PSIMAGESLICE_TEST1(TYPE, m, 1, PS_CUT_X_POS, m, i, 0, 4 ); \
    PSIMAGESLICE_TEST1(TYPE, m, 1, PS_CUT_X_NEG, m, m - 1 - i, 0, 5 ); \
    PSIMAGESLICE_TEST1(TYPE, m, 1, PS_CUT_Y_POS, 1, m / 2, 0, 6 ); \
    PSIMAGESLICE_TEST1(TYPE, m, 1, PS_CUT_Y_NEG, 1, m / 2, 0, 7 ); \
    \
    /* test 1xN case */ \
    PSIMAGESLICE_TEST1(TYPE, 1, n, PS_CUT_X_POS, 1, 0, n / 2, 8 ); \
    PSIMAGESLICE_TEST1(TYPE, 1, n, PS_CUT_X_NEG, 1, 0, n / 2, 9 ); \
    PSIMAGESLICE_TEST1(TYPE, 1, n, PS_CUT_Y_POS, n, 0, i, 10 ); \
    PSIMAGESLICE_TEST1(TYPE, 1, n, PS_CUT_Y_NEG, n, 0, n - 1 - i, 11 ); \
    \
    /* test 1x1 case */ \
    PSIMAGESLICE_TEST1(TYPE, 1, 1, PS_CUT_X_POS, 1, 0, 0, 12 ); \
    PSIMAGESLICE_TEST1(TYPE, 1, 1, PS_CUT_X_NEG, 1, 0, 0, 13 ); \
    PSIMAGESLICE_TEST1(TYPE, 1, 1, PS_CUT_Y_POS, 1, 0, 0, 14 ); \
    PSIMAGESLICE_TEST1(TYPE, 1, 1, PS_CUT_Y_NEG, 1, 0, 0, 15 ); \

    PSIMAGESLICE_TEST(F32);
    PSIMAGESLICE_TEST(F64);
    PSIMAGESLICE_TEST(U16);

    image = psImageAlloc( c, r, PS_TYPE_F32 );

    /*
       Verify the returned psVector structure pointer is null and program
       execution doesn't stop, if input psImage input is null.

    */
    psLogMsg( __func__, PS_LOG_INFO, "Following should be an error." );
    out = psImageSlice( out,
                        NULL, NULL,
                        NULL, 0,
                        psRegionSet(c/10, c/10 + 1, r/10, r/10 + 1),
                        PS_CUT_X_POS,
                        stat );
    if ( out != NULL ) {
        psError( PS_ERR_UNKNOWN,true, "Giving a NULL image, psImageSlice didn't return NULL as expected" );
        return 101;
    }


    /*
       Verify the returned psVector structure pointer is null and program
       execution doesn't stop, if input psStats stats is null.
    */
    psLogMsg( __func__, PS_LOG_INFO, "Following should be an error." );
    out = psImageSlice( out,
                        NULL, image,
                        mask, 1,
                        psRegionSet(c/10, c/10 + 1, r/10, r/10 + 1),
                        PS_CUT_X_POS,
                        NULL );
    if ( out != NULL ) {
        psError( PS_ERR_UNKNOWN,true, "Giving a NULL stat struct, psImageSlice didn't return NULL as expected" );
        return 102;
    }
    /*

       Verify the returned psVector structure pointer is null and program
       executions doesn't stop, if the input direction is not set to one of
       the two valid values.
    */
    psLogMsg( __func__, PS_LOG_INFO, "Following should be an error." );
    out = psImageSlice( out, NULL,
                        image,
                        mask, 1,
                        psRegionSet(c/10, c/10 + 1, r/10, r/10 + 1),
                        5,
                        stat);
    if ( out != NULL ) {
        psError( PS_ERR_UNKNOWN,true, "Giving a bogus direction flag, psImageSlice didn't return NULL as expected" );
        return 103;
    }

    /*
       Verify the returned psVector structure pointer is null and program
       execution doesn't stop, if the input nrow and/or ncol are zero.
    */
    psLogMsg( __func__, PS_LOG_INFO, "Following should be an error." );
    out = psImageSlice( out,
                        NULL,
                        image,
                        mask, 1,
                        psRegionSet(c/10, c/10, r/10, r/10),
                        PS_CUT_X_POS,
                        stat );
    if ( out != NULL ) {
        psError( PS_ERR_UNKNOWN,true, "Giving a 0x0 region, psImageSlice didn't return NULL as expected" );
        return 104;
    }

    /*
       Verify the returned psVector structure pointer is null and program
       execution doesn't stop, if the inputs row, col, nrow, ncol specify a
       regions of data that is not within the input psImage structure.
    */
    psLogMsg( __func__, PS_LOG_INFO, "Following should be an error." );
    out = psImageSlice( out, NULL,
                        image,
                        mask, 1,
                        psRegionSet(c+1, c+2, r/10, r/10 + 10),
                        PS_CUT_X_POS,
                        stat );
    if ( out != NULL ) {
        psError( PS_ERR_UNKNOWN,true, "Giving an invalid x position, psImageSlice didn't return NULL as expected" );
        return 105;
    }

    psLogMsg( __func__, PS_LOG_INFO, "Following should be an error." );
    out = psImageSlice( out, NULL,
                        image,
                        mask, 1,
                        psRegionSet(c/10, c/10 + 1, r+1,r+5),
                        PS_CUT_X_POS,
                        stat );
    if ( out != NULL ) {
        psError( PS_ERR_UNKNOWN,true, "Giving an invalid y position, psImageSlice didn't return NULL as expected" );
        return 106;
    }

    psLogMsg( __func__, PS_LOG_INFO, "Following should be an error." );
    out = psImageSlice( out, NULL,
                        image,
                        mask, 1,
                        psRegionSet(c/10, c+1, r/10, r/10+1),
                        PS_CUT_X_POS,
                        stat);
    if ( out != NULL ) {
        psError( PS_ERR_UNKNOWN,true, "Giving an invalid numCols, psImageSlice didn't return NULL as expected" );
        return 107;
    }

    psLogMsg( __func__, PS_LOG_INFO, "Following should be an error." );
    out = psImageSlice( out, NULL,
                        image,
                        mask, 1,
                        psRegionSet(c/10, c/10 + 1, r/10, r + 1),
                        PS_CUT_X_POS,
                        stat);
    if ( out != NULL ) {
        psError( PS_ERR_UNKNOWN,true, "Giving an invalid numRows, psImageSlice didn't return NULL as expected" );
        return 108;
    }

    /*
       Verify the returned psVector structure pointer is null and program
       execution doesn't stop, if the input psStat structure member options is
       zero which indicates no statistic method specified.
    */
    psLogMsg( __func__, PS_LOG_INFO, "Following should be an error." );
    stat->options = 0;
    out = psImageSlice( out, NULL,
                        image,
                        mask, 1,
                        psRegionSet(c/10, c/10 + 1, r/10, r/10+1),
                        PS_CUT_X_POS,
                        stat);
    if ( out != NULL ) {
        psError( PS_ERR_UNKNOWN,true, "Giving an invalid numRows, psImageSlice didn't return NULL as expected" );
        return 109;
    }

    /* Verify that a mask of different size than the input image returns null and program
       execution doesn't stop.
    */
    psLogMsg( __func__, PS_LOG_INFO, "Following should be an error mask size != image size.");
    stat->options = PS_STAT_SAMPLE_MEDIAN;
    psImage* maskSz = psImageAlloc( r, c, PS_TYPE_MASK );
    out = psImageSlice( out, NULL,
                        image,
                        maskSz, 1,
                        psRegionSet(c/10, c/10 + 1, r/10, r/10 + 1),
                        PS_CUT_X_POS,
                        stat);
    if ( out != NULL ) {
        psError( PS_ERR_UNKNOWN,true, "Mask size different than image size didn't return NULL as expected" );
        return 110;
    }

    /* Verify the a invalid type mask returns null and program execution doesn't stop.
    */
    psLogMsg( __func__, PS_LOG_INFO, "Following should be an error invalid mask type.");
    psImage* maskS8 = psImageAlloc( c, r, PS_TYPE_S8 );
    out =  psImageSlice( out, NULL,
                         image,
                         maskS8, 1,
                         psRegionSet(c/10, c/10 + 1, r/10, r/10 + 1),
                         PS_CUT_X_POS,
                         stat);
    if ( out != NULL ) {
        psError( PS_ERR_UNKNOWN,true, "Mask invalid type didn't return NULL as expected.");
        return 111;
    }

    //Added tests after subimage changes.
    psFree(image);
    image = psImageAlloc( c, r, PS_TYPE_F64 );
    for ( psS32 row = 0;row < r;row++ ) {
        psF64 *imageRow = image->data.F64[ row ];
        psF64 rowOffset = row * 2;
        for ( psS32 col = 0;col < c;col++ ) {
            imageRow[ col ] = col + rowOffset;
        }
    }
    image->col0 = 1;
    image->row0 = 1;
    psFree(out);
    out = NULL;
    out = psImageSlice(out,positions,image,mask,1,
                       psRegionSet(1,c,1,r),PS_CUT_X_POS,stat);
    if (out == NULL) {
        psError(PS_ERR_BAD_PARAMETER_NULL, false,
                "psImageSlice failed to return the correct psVector.  Got NULL instead.\n");
        return 112;
    }
    psFree(out);
    out = NULL;
    //Return NULL for incorrect image inputs.
    image->row0 = -1;
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate error message");
    out = psImageSlice(out,positions,image,mask,1,
                       psRegionSet(1,c,1,r),PS_CUT_X_POS,stat);
    if (out != NULL) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, false,
                "psImageSlice failed to return NULL for invalid specified input.\n");
        return 113;
    }
    image->col0 = -1;
    image->row0 = 1;
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate error message");
    out = psImageSlice(out,positions,image,mask,1,
                       psRegionSet(1,c,1,r),PS_CUT_X_POS,stat);
    if (out != NULL) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, false,
                "psImageSlice failed to return NULL for invalid specified input.\n");
        return 114;
    }
    image->col0 = 1;
    //Return NULL for incorrect region inputs.
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate error message");
    out = psImageSlice(out,positions,image,mask,1,
                       psRegionSet(0,c,1,r),PS_CUT_X_POS,stat);
    if (out != NULL) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, false,
                "psImageSlice failed to return NULL for invalid specified input.\n");
        return 115;
    }
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate error message");
    out = psImageSlice(out,positions,image,mask,1,
                       psRegionSet(1,c,1,r+1),PS_CUT_X_POS,stat);
    if (out != NULL) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, false,
                "psImageSlice failed to return NULL for invalid specified input.\n");
        return 116;
    }
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate error message");
    out = psImageSlice(out,positions,image,mask,1,
                       psRegionSet(1,c,1,-r-2),PS_CUT_X_POS,stat);
    if (out != NULL) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, false,
                "psImageSlice failed to return NULL for invalid specified input.\n");
        return 117;
    }
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate error message");
    out = psImageSlice(out,positions,image,mask,1,
                       psRegionSet(c,1,1,r),PS_CUT_X_POS,stat);
    if (out != NULL) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, false,
                "psImageSlice failed to return NULL for invalid specified input.\n");
        return 118;
    }
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate error message");
    out = psImageSlice(out,positions,image,mask,1,
                       psRegionSet(1,1,1,1),PS_CUT_X_POS,stat);
    if (out != NULL) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, false,
                "psImageSlice failed to return NULL for invalid specified input.\n");
        return 119;
    }

    //Make sure that regions match appropriately...
    out = psImageSlice(out,positions,image,mask,1,
                       psRegionSet(1,-1,1,-1),PS_CUT_Y_NEG,stat);
    psVector *out2 = NULL;
    out2 = psImageSlice(out2,positions,image,mask,1,
                        psRegionSet(0,0,0,0),PS_CUT_Y_NEG,stat);
    if (out == NULL) {
        psError(PS_ERR_BAD_PARAMETER_NULL, false,
                "psImageSlice incorrectly returned NULL for valid inputs.\n");
        return 120;
    } else if (out2 == NULL) {
        psError(PS_ERR_BAD_PARAMETER_NULL, false,
                "psImageSlice incorrectly returned NULL for valid inputs.\n");
        return 121;
    } else if (out->n != out2->n) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, false,
                "psImageSlice failed to return matching vectors for equivalent inputs.\n");
        return 122;
    } else if (out->data.F64[out->n-1] != out2->data.F64[out2->n-1] ) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, false,
                "psImageSlice failed to return matching vectors for equivalent inputs.\n");
        return 123;
    }
    psFree(out2);

    psFree( image );
    psFree( positions );
    psFree( mask );
    psFree( out );
    psFree( stat );
    psFree( maskS8 );
    psFree( maskSz );

    return 0;

}

static psS32 testImageCut(void)
{
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

    psVector* result = NULL;
    for (psS32 n = 0; n < numPoints; n++) {
        psVector* orig = result;
        if (! success[n]) {
            psLogMsg(__func__,PS_LOG_INFO,"The following should be an error.");
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

        if (success[n]) {
            if (result == NULL) {
                psLogMsg(__func__,PS_LOG_ERROR,
                         "psImageCut returned NULL instead of a valid result.");
                return n*10+1;
            }

            if (orig != NULL && orig != result) {
                psLogMsg(__func__,PS_LOG_ERROR,
                         "psImageCut didn't recycle the out parameter properly.");
                return n*10+2;
            }

            float deltaRow = (endRow[n]-startRow[n])/(length-1);
            float deltaCol = (endCol[n]-startCol[n])/(length-1);
            psF32 truth;
            for (psS32 i = 0; i < length; i++) {
                float x = (float)startCol[n]+(float)i*deltaCol;
                float y = (float)startRow[n]+(float)i*deltaRow;
                if (n == 1) {
                    truth = psImagePixelInterpolate( image, x, y,
                                                     NULL,0,0,PS_INTERPOLATE_FLAT);
                } else {
                    truth = psImagePixelInterpolate( image, x, y,
                                                     mask,1,0,PS_INTERPOLATE_FLAT);
                }
                if (fabs(result->data.F32[i]-truth) > FLT_EPSILON) {
                    psLogMsg(__func__,PS_LOG_ERROR,
                             "Bad result in position %d; Found %g but expected %g.",
                             i, result->data.F32[i], truth);
                    return n*10+5;
                }
                if (fabsf(x - cols->data.F32[i]) > FLT_EPSILON ||
                        fabsf(y - rows->data.F32[i]) > FLT_EPSILON) {
                    psLogMsg(__func__,PS_LOG_ERROR,
                             "Bad resulting col/row at index %d; Found (%g,%g) but expected (%g,%g).",
                             i, cols->data.F32[i], rows->data.F32[i], x, y);
                    return n*10+6;
                }
            }
        } else {
            if (result != NULL) {
                psLogMsg(__func__,PS_LOG_ERROR,
                         "psImageCut did not return NULL with a cut of (%g,%g)->(%g,%g).",
                         startCol[n],startRow[n],endCol[n],endRow[n]);
                return n*10+7;
            }
            psErr* err = psErrorLast();
            if (err->code != PS_ERR_BAD_PARAMETER_VALUE) {
                psLogMsg(__func__,PS_LOG_ERROR,
                         "psImageCut did not generate proper error message.");
                return 105;
            }
            psFree(err);
        }
    }

    psLogMsg(__func__,PS_LOG_INFO,"Following should be an error (NULL image).");
    result = psImageCut(result,
                        cols,rows,
                        NULL,
                        mask,1,
                        psRegionSet(startCol[0], endCol[0], startRow[0], endRow[0]),
                        length,
                        PS_INTERPOLATE_FLAT);
    if (result != NULL) {
        psLogMsg(__func__,PS_LOG_ERROR,
                 "psImageCut did not return NULL given NULL image.");
        return 100;
    }
    psErr* err = psErrorLast();
    if (err->code != PS_ERR_BAD_PARAMETER_NULL) {
        psLogMsg(__func__,PS_LOG_ERROR,
                 "psImageCut did not generate proper error message given NULL image.");
        return 101;
    }
    psFree(err);

    psLogMsg(__func__,PS_LOG_INFO,"Following should be an error (length=0).");
    result = psImageCut(result,
                        cols,rows,
                        image,
                        mask,1,
                        psRegionSet(startCol[0], endCol[0], startRow[0], endRow[0]),
                        0,
                        PS_INTERPOLATE_FLAT);
    if (result != NULL) {
        psLogMsg(__func__,PS_LOG_ERROR,
                 "psImageCut did not return NULL given length=0.");
        return 102;
    }
    err = psErrorLast();
    if (err->code != PS_ERR_BAD_PARAMETER_VALUE) {
        psLogMsg(__func__,PS_LOG_ERROR,
                 "psImageCut did not generate proper error message given length=0.");
        return 103;
    }
    psFree(err);

    psFree(result);
    psFree(image);
    psFree(mask);
    psFree(rows);
    psFree(cols);

    return 0;
}

static psS32 testImageRadialCut(void)
{
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
        radii->n++;
    }

    psVector* result = NULL;

    result = psImageRadialCut(result,image,mask,1,centerX,centerY,radii,stat);

    if (result == NULL) {
        psLogMsg(__func__,PS_LOG_ERROR,
                 "Return value of NULL unexpected.");
        return 1;
    }

    if (result->type.type != PS_TYPE_F64) {
        psLogMsg(__func__,PS_LOG_ERROR,
                 "Return type not psF64, as expected.");
        return 2;
    }

    for (psS32 i=0; i < 9; i++) {
        if (fabs(result->data.F64[i] - (15.0+i*10)) > 1) {
            psLogMsg(__func__,PS_LOG_ERROR,
                     "Result was not as expected for radii #%d (%g, expected %d +/- 1)",
                     result->data.F64[i], (15.0+i*10) );
            return 3+i;
        }
    }

    // again, but without mask
    psVector* orig = result;
    result = psImageRadialCut(result,image,NULL,1,centerX,centerY,radii,stat);

    if (result == NULL) {
        psLogMsg(__func__,PS_LOG_ERROR,
                 "Return value of NULL unexpected.");
        return 12;
    }

    if (result != orig) {
        psLogMsg(__func__,PS_LOG_ERROR,
                 "Return value of is not same as input parameter 'out'.");
        return 13;
    }

    for (psS32 i=0; i < 9; i++) {
        if (fabs(result->data.F64[i] - (15.0+i*10)) > 1) {
            psLogMsg(__func__,PS_LOG_ERROR,
                     "Result was not as expected for radii #%d (%g, expected %d +/- 1)",
                     result->data.F64[i], (15.0+i*10) );
            return 14+i;
        }
    }

    // NULL input image...
    psLogMsg(__func__,PS_LOG_INFO,"Following should be an error.");
    result = psImageRadialCut(result,NULL,NULL,1,centerX,centerY,radii,stat);

    if (result != NULL) {
        psLogMsg(__func__,PS_LOG_ERROR,
                 "Return value not NULL as expected.");
        return 23;
    }

    err = psErrorLast();
    if (err->code != PS_ERR_BAD_PARAMETER_NULL) {
        psLogMsg(__func__,PS_LOG_ERROR,
                 "psImageRadialCut did not generate proper error message.");
        return 24;
    }
    psFree(err);

    // NULL input radii...
    psLogMsg(__func__,PS_LOG_INFO,"Following should be an error.");
    result = psImageRadialCut(result,image,mask,1,centerX,centerY,NULL,stat);

    if (result != NULL) {
        psLogMsg(__func__,PS_LOG_ERROR,
                 "Return value not NULL as expected.");
        return 23;
    }

    err = psErrorLast();
    if (err->code != PS_ERR_BAD_PARAMETER_NULL) {
        psLogMsg(__func__,PS_LOG_ERROR,
                 "psImageRadialCut did not generate proper error message.");
        return 24;
    }
    psFree(err);

    // NULL input stat...
    psLogMsg(__func__,PS_LOG_INFO,"Following should be an error.");
    result = psImageRadialCut(result,image,mask,1,centerX,centerY,radii,NULL);

    if (result != NULL) {
        psLogMsg(__func__,PS_LOG_ERROR,
                 "Return value not NULL as expected.");
        return 23;
    }

    err = psErrorLast();
    if (err->code != PS_ERR_BAD_PARAMETER_NULL) {
        psLogMsg(__func__,PS_LOG_ERROR,
                 "psImageRadialCut did not generate proper error message.");
        return 24;
    }
    psFree(err);

    // Bad center X
    psLogMsg(__func__,PS_LOG_INFO,"Following should be an error.");
    result = psImageRadialCut(result,image,mask,1,
                              c+1,centerY,radii,stat);

    if (result != NULL) {
        psLogMsg(__func__,PS_LOG_ERROR,
                 "Return value not NULL as expected.");
        return 25;
    }

    err = psErrorLast();
    if (err->code != PS_ERR_BAD_PARAMETER_VALUE) {
        psLogMsg(__func__,PS_LOG_ERROR,
                 "psImageRadialCut did not generate proper error message.");
        return 26;
    }
    psFree(err);

    // Bad center Y
    psLogMsg(__func__,PS_LOG_INFO,"Following should be an error.");
    result = psImageRadialCut(result,image,mask,1,
                              centerX,r+1,radii,stat);

    if (result != NULL) {
        psLogMsg(__func__,PS_LOG_ERROR,
                 "Return value not NULL as expected.");
        return 27;
    }

    err = psErrorLast();
    if (err->code != PS_ERR_BAD_PARAMETER_VALUE) {
        psLogMsg(__func__,PS_LOG_ERROR,
                 "psImageRadialCut did not generate proper error message.");
        return 28;
    }
    psFree(err);

    // Bad mask type (N.B., swapped image/mask to do this)
    psLogMsg(__func__,PS_LOG_INFO,"Following should be an error.");
    result = psImageRadialCut(result,mask,image,1,
                              centerX,r+1,radii,stat);

    if (result != NULL) {
        psLogMsg(__func__,PS_LOG_ERROR,
                 "Return value not NULL as expected.");
        return 29;
    }

    err = psErrorLast();
    if (err->code != PS_ERR_BAD_PARAMETER_TYPE) {
        psLogMsg(__func__,PS_LOG_ERROR,
                 "psImageRadialCut did not generate proper error message.");
        return 30;
    }
    psFree(err);

    // Bad mask size
    psImage* mask2 = psImageAlloc(c/2,r/2,PS_TYPE_MASK);
    psLogMsg(__func__,PS_LOG_INFO,"Following should be an error.");
    result = psImageRadialCut(result,image, mask2, 1,
                              centerX,centerY,radii,stat);
    psFree(mask2);
    if (result != NULL) {
        psLogMsg(__func__,PS_LOG_ERROR,
                 "Return value not NULL as expected.");
        return 31;
    }

    err = psErrorLast();
    if (err->code != PS_ERR_BAD_PARAMETER_SIZE) {
        psLogMsg(__func__,PS_LOG_ERROR,
                 "psImageRadialCut did not generate proper error message.");
        return 32;
    }
    psFree(err);

    // Bad radii size
    psVector* radii2 = psVectorAlloc(1,PS_TYPE_MASK);
    psLogMsg(__func__,PS_LOG_INFO,"Following should be an error.");
    result = psImageRadialCut(result,image, mask, 1,
                              centerX,centerY,radii2,stat);
    psFree(radii2);
    if (result != NULL) {
        psLogMsg(__func__,PS_LOG_ERROR,
                 "Return value not NULL as expected.");
        return 33;
    }

    err = psErrorLast();
    if (err->code != PS_ERR_BAD_PARAMETER_SIZE) {
        psLogMsg(__func__,PS_LOG_ERROR,
                 "psImageRadialCut did not generate proper error message.");
        return 34;
    }
    psFree(err);

    // bad input stat option...
    stat->options = 0;
    psLogMsg(__func__,PS_LOG_INFO,"Following should be an error.");
    result = psImageRadialCut(result,image,mask,1,centerX,centerY,radii,stat);
    stat->options = PS_STAT_SAMPLE_MEAN;

    if (result != NULL) {
        psLogMsg(__func__,PS_LOG_ERROR,
                 "Return value not NULL as expected.");
        return 35;
    }

    err = psErrorLast();
    if (err->code != PS_ERR_BAD_PARAMETER_VALUE) {
        psLogMsg(__func__,PS_LOG_ERROR,
                 "psImageRadialCut did not generate proper error message.");
        return 36;
    }
    psFree(err);

    psFree(image);
    psFree(mask);
    psFree(radii);
    psFree(stat);
    psFree(result);

    return 0;
}

psS32 testImageRowColError(void)
{
    psImage *image = NULL;
    psVector *out = NULL;
    int num = 0;

    psLogMsg(__func__,PS_LOG_INFO,"Following should generate error message(for row)");
    out = psImageRow(NULL, image, num);
    if (out != NULL) {
        return 1;
    }
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate error message(for col)");
    out = psImageCol(NULL, image, num);
    if (out != NULL) {
        return 1;
    }

    image = psImageAlloc(3, 3, PS_TYPE_F64);

    //Test for invalid row0.
    *(psS32*)&(image->row0) = -1;
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate error message(for row)");
    out = psImageRow(NULL, image, num);
    if (out != NULL) {
        return 2;
    }
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate error message(for col)");
    out = psImageCol(NULL, image, num);
    if (out != NULL) {
        return 2;
    }

    //Test for invalid col0.
    *(psS32*)&(image->row0) = 5;
    *(psS32*)&(image->col0) = -1;
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate error message(for row)");
    out = psImageRow(NULL, image, num);
    if (out != NULL) {
        return 3;
    }
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate error message(for col)");
    out = psImageCol(NULL, image, num);
    if (out != NULL) {
        return 3;
    }

    //Test for invalid numRows
    *(psS32*)&(image->col0) = 10;
    *(int*)&(image->numRows) = -1;
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate error message(for row)");
    out = psImageRow(NULL, image, num);
    if (out != NULL) {
        return 4;
    }
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate error message(for col)");
    out = psImageCol(NULL, image, num);
    if (out != NULL) {
        return 4;
    }
    //Test for invalid numCols
    *(int*)&(image->numRows) = 3;
    *(int*)&(image->numCols) = -1;
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate error message(for row)");
    out = psImageRow(NULL, image, num);
    if (out != NULL) {
        return 5;
    }
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate error message(for col)");
    out = psImageCol(NULL, image, num);
    if (out != NULL) {
        return 5;
    }
    //Test for invalid row/col number specified.
    *(int*)&(image->numCols) = 3;
    num = 8;
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate error message(for row)");
    out = psImageRow(NULL, image, num);
    if (out != NULL) {
        return 6;
    }
    num = 13;
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate error message(for col)");
    out = psImageCol(NULL, image, num);
    if (out != NULL) {
        return 6;
    }
    num = 3;
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate error message(for row)");
    out = psImageRow(NULL, image, num);
    if (out != NULL) {
        return 7;
    }
    num = 8;
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate error message(for col)");
    out = psImageCol(NULL, image, num);
    if (out != NULL) {
        return 7;
    }
    num = -10;
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate error message(for row)");
    out = psImageRow(NULL, image, num);
    if (out != NULL) {
        return 8;
    }
    num = -14;
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate error message(for col)");
    out = psImageCol(NULL, image, num);
    if (out != NULL) {
        return 8;
    }

    //Test valid cases.
    image->col0 = 10;
    image->row0 = 5;
    *(int*)&(image->numRows) = 3;
    *(int*)&(image->numCols) = 3;
    image->data.F64[0][0] = 666.666;
    image->data.F64[1][0] = 66.6;
    image->data.F64[2][0] = 6.66;
    image->data.F64[0][1] = 6.6;
    image->data.F64[1][1] = 6.666;
    image->data.F64[2][1] = 66.666;
    image->data.F64[0][2] = 666.6;
    image->data.F64[1][2] = 666.66;
    image->data.F64[2][2] = 66.66;
    num = 7;
    out = psImageRow(out, image, num);
    if (out == NULL) {
        psError(PS_ERR_BAD_PARAMETER_NULL, false,
                "psImageRow failed to return correct psVector output.\n");
        return 10;
    } else {
        psFree(out);
        out = NULL;
    }
    num = 11;
    out = psImageCol(NULL, image, num);
    if (out == NULL) {
        psError(PS_ERR_BAD_PARAMETER_NULL, false,
                "psImageCol failed to return correct psVector output.\n");
        return 10;
    } else {
        psFree(out);
        out = NULL;
    }

    num = -3;
    out = psImageRow(out, image, num);
    if (out == NULL) {
        psError(PS_ERR_BAD_PARAMETER_NULL, false,
                "psImageRow failed to return correct psVector output.\n");
        return 10;
    } else {
        psFree(out);
        out = NULL;
    }
    num = -1;
    out = psImageCol(NULL, image, num);
    if (out == NULL) {
        psError(PS_ERR_BAD_PARAMETER_NULL, false,
                "psImageCol failed to return correct psVector output.\n");
        return 10;
    }
    psFree(out);
    psFree(image);
    return 0;
}

psS32 testImageRowColF64(void)
{
    psVector *rowcol = NULL;
    psVector *empty = NULL;
    psImage *image = NULL;
    psImage *emptyImage = NULL;

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
    empty = psImageCol(empty, emptyImage, 0);
    if (empty != NULL) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, false,
                "psImageCol failed to return NULL for NULL image input.\n");
        return 1;
    }
    //Test for error with Out of Range Row
    empty = psImageRow(empty, image, 5);
    if (empty != NULL) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, false,
                "psImageRow failed to return NULL for out of range row input.\n");
        return 2;
    }
    rowcol->data.F64[0] = 1.1;
    rowcol->data.F64[2] = 2.2;
    //Test recycling of non-NULL vector & correct output
    rowcol = psImageCol(rowcol, image, 1);
    double test1, test2;
    double TOLTST = .001;
    test1 = fabs(rowcol->data.F64[0]-6.6);
    test2 = fabs(rowcol->data.F64[2]-66.666);
    if ( (test1>TOLTST) || (test2>TOLTST) ) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, false,
                "psImageCol failed to return correct values.\n");
        return 3;
    }
    rowcol = psImageRow(rowcol, image, 1);
    test1 = fabs(rowcol->data.F64[0]-66.6);
    test2 = fabs(rowcol->data.F64[2]-666.66);
    if ( (test1>TOLTST) || (test2>TOLTST) ) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, false,
                "psImageRow failed to return correct values.\n");
        return 4;
    }

    psFree(rowcol);
    psFree(image);
    return 0;
}

psS32 testImageRowColF32(void)
{
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
    if (empty != NULL) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, false,
                "psImageCol failed to return NULL for NULL image input.\n");
        return 1;
    }
    //Test for error with Out of Range Row
    empty = psImageRow(empty, image, 5);
    if (empty != NULL) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, false,
                "psImageRow failed to return NULL for out of range row input.\n");
        return 2;
    }
    rowcol->data.F32[0] = 1.1;
    rowcol->data.F32[2] = 2.2;
    //Test recycling of non-NULL vector & correct output
    rowcol = psImageCol(rowcol, image, 1);
    test1 = fabs(rowcol->data.F32[0]-6.6);
    test2 = fabs(rowcol->data.F32[2]-66.666);
    if ( (test1>TOLTST) || (test2>TOLTST) ) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, false,
                "psImageCol failed to return correct values.\n");
        return 3;
    }

    psFree(rowcol);
    psFree(image);
    return 0;
}

psS32 testImageRowColU64(void)
{
    psVector *rowcol = NULL;
    psVector *empty = NULL;
    psImage *image = NULL;
    psImage *emptyImage = NULL;

    image = psImageAlloc(3, 3, PS_TYPE_U64);
    rowcol = psVectorAlloc(3, PS_TYPE_U64);

    image->data.U64[0][0] = 666666;
    image->data.U64[1][0] = 666;
    image->data.U64[2][0] = 666;
    image->data.U64[0][1] = 66;
    image->data.U64[1][1] = 6666;
    image->data.U64[2][1] = 66666;
    image->data.U64[0][2] = 6666;
    image->data.U64[1][2] = 66666;
    image->data.U64[2][2] = 6666;

    //Test for error with NULL image
    empty = psImageCol(empty, emptyImage, 0);
    if (empty != NULL) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, false,
                "psImageCol failed to return NULL for NULL image input.\n");
        return 1;
    }
    //Test for error with Out of Range Row
    empty = psImageRow(empty, image, 5);
    if (empty != NULL) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, false,
                "psImageRow failed to return NULL for out of range row input.\n");
        return 2;
    }
    rowcol->data.U64[0] = 11;
    rowcol->data.U64[2] = 22;
    //Test recycling of non-NULL vector & correct output
    rowcol = psImageCol(rowcol, image, 1);
    if (rowcol->data.U64[0] != 666 && rowcol->data.U64[2] != 66666) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, false,
                "psImageCol failed to return correct values.\n");
        return 3;
    }

    psFree(rowcol);
    psFree(image);
    return 0;
}

psS32 testImageRowColU32(void)
{
    psVector *rowcol = NULL;
    psVector *empty = NULL;
    psImage *image = NULL;
    psImage *emptyImage = NULL;

    image = psImageAlloc(3, 3, PS_TYPE_U32);
    rowcol = psVectorAlloc(3, PS_TYPE_U32);

    image->data.U32[0][0] = 666666;
    image->data.U32[1][0] = 666;
    image->data.U32[2][0] = 666;
    image->data.U32[0][1] = 66;
    image->data.U32[1][1] = 6666;
    image->data.U32[2][1] = 66666;
    image->data.U32[0][2] = 6666;
    image->data.U32[1][2] = 66666;
    image->data.U32[2][2] = 6666;

    //Test for error with NULL image
    empty = psImageCol(empty, emptyImage, 0);
    if (empty != NULL) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, false,
                "psImageCol failed to return NULL for NULL image input.\n");
        return 1;
    }
    //Test for error with Out of Range Row
    empty = psImageRow(empty, image, 5);
    if (empty != NULL) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, false,
                "psImageRow failed to return NULL for out of range row input.\n");
        return 2;
    }
    rowcol->data.U32[0] = 11;
    rowcol->data.U32[2] = 22;
    //Test recycling of non-NULL vector & correct output
    rowcol = psImageCol(rowcol, image, 1);
    if (rowcol->data.U32[0] != 666 && rowcol->data.U32[2] != 66666) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, false,
                "psImageCol failed to return correct values.\n");
        return 3;
    }

    psFree(rowcol);
    psFree(image);
    return 0;
}

psS32 testImageRowColS32(void)
{
    psVector *rowcol = NULL;
    psVector *empty = NULL;
    psImage *image = NULL;
    psImage *emptyImage = NULL;

    image = psImageAlloc(3, 3, PS_TYPE_S32);
    rowcol = psVectorAlloc(3, PS_TYPE_S32);

    image->data.S32[0][0] = 666666;
    image->data.S32[1][0] = 666;
    image->data.S32[2][0] = 666;
    image->data.S32[0][1] = 66;
    image->data.S32[1][1] = 6666;
    image->data.S32[2][1] = 66666;
    image->data.S32[0][2] = 6666;
    image->data.S32[1][2] = 66666;
    image->data.S32[2][2] = 6666;

    //Test for error with NULL image
    empty = psImageCol(empty, emptyImage, 0);
    if (empty != NULL) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, false,
                "psImageCol failed to return NULL for NULL image input.\n");
        return 1;
    }
    //Test for error with Out of Range Row
    empty = psImageRow(empty, image, 5);
    if (empty != NULL) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, false,
                "psImageRow failed to return NULL for out of range row input.\n");
        return 2;
    }
    rowcol->data.S32[0] = 11;
    rowcol->data.S32[2] = 22;
    //Test recycling of non-NULL vector & correct output
    rowcol = psImageCol(rowcol, image, 1);
    if (rowcol->data.S32[0] != 666 && rowcol->data.S32[2] != 66666) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, false,
                "psImageCol failed to return correct values.\n");
        return 3;
    }

    psFree(rowcol);
    psFree(image);
    return 0;
}

psS32 testImageRowColS64(void)
{
    psVector *rowcol = NULL;
    psVector *empty = NULL;
    psImage *image = NULL;
    psImage *emptyImage = NULL;

    image = psImageAlloc(3, 3, PS_TYPE_S64);
    rowcol = psVectorAlloc(3, PS_TYPE_S64);

    image->data.S64[0][0] = 666666;
    image->data.S64[1][0] = 666;
    image->data.S64[2][0] = 666;
    image->data.S64[0][1] = 66;
    image->data.S64[1][1] = 6666;
    image->data.S64[2][1] = 66666;
    image->data.S64[0][2] = 6666;
    image->data.S64[1][2] = 66666;
    image->data.S64[2][2] = 6666;

    //Test for error with NULL image
    empty = psImageCol(empty, emptyImage, 0);
    if (empty != NULL) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, false,
                "psImageCol failed to return NULL for NULL image input.\n");
        return 1;
    }
    //Test for error with Out of Range Row
    empty = psImageRow(empty, image, 5);
    if (empty != NULL) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, false,
                "psImageRow failed to return NULL for out of range row input.\n");
        return 2;
    }
    rowcol->data.S64[0] = 11;
    rowcol->data.S64[2] = 22;
    //Test recycling of non-NULL vector & correct output
    rowcol = psImageCol(rowcol, image, 1);
    if (rowcol->data.S64[0] != 666 && rowcol->data.S64[2] != 66666) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, false,
                "psImageCol failed to return correct values.\n");
        return 3;
    }

    psFree(rowcol);
    psFree(image);
    return 0;
}

psS32 testImageRowColS16(void)
{
    psVector *rowcol = NULL;
    psVector *empty = NULL;
    psImage *image = NULL;
    psImage *emptyImage = NULL;

    image = psImageAlloc(3, 3, PS_TYPE_S16);
    rowcol = psVectorAlloc(3, PS_TYPE_S16);

    image->data.S16[0][0] = 3333;
    image->data.S16[1][0] = 666;
    image->data.S16[2][0] = 666;
    image->data.S16[0][1] = 66;
    image->data.S16[1][1] = 6666;
    image->data.S16[2][1] = 4444;
    image->data.S16[0][2] = 6666;
    image->data.S16[1][2] = 4444;
    image->data.S16[2][2] = 6666;

    //Test for error with NULL image
    empty = psImageCol(empty, emptyImage, 0);
    if (empty != NULL) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, false,
                "psImageCol failed to return NULL for NULL image input.\n");
        return 1;
    }
    //Test for error with Out of Range Row
    empty = psImageRow(empty, image, 5);
    if (empty != NULL) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, false,
                "psImageRow failed to return NULL for out of range row input.\n");
        return 2;
    }
    rowcol->data.S16[0] = 11;
    rowcol->data.S16[2] = 22;
    //Test recycling of non-NULL vector & correct output
    rowcol = psImageCol(rowcol, image, 1);
    if (rowcol->data.S16[0] != 666 && rowcol->data.S16[2] != 4444) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, false,
                "psImageCol failed to return correct values.\n");
        return 3;
    }

    psFree(rowcol);
    psFree(image);
    return 0;
}

psS32 testImageRowColU16(void)
{
    psVector *rowcol = NULL;
    psVector *empty = NULL;
    psImage *image = NULL;
    psImage *emptyImage = NULL;

    image = psImageAlloc(3, 3, PS_TYPE_U16);
    rowcol = psVectorAlloc(3, PS_TYPE_U16);

    image->data.S16[0][0] = 3333;
    image->data.S16[1][0] = 666;
    image->data.S16[2][0] = 666;
    image->data.S16[0][1] = 66;
    image->data.S16[1][1] = 6666;
    image->data.S16[2][1] = 4444;
    image->data.S16[0][2] = 6666;
    image->data.S16[1][2] = 4444;
    image->data.S16[2][2] = 6666;

    //Test for error with NULL image
    empty = psImageCol(empty, emptyImage, 0);
    if (empty != NULL) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, false,
                "psImageCol failed to return NULL for NULL image input.\n");
        return 1;
    }
    //Test for error with Out of Range Row
    empty = psImageRow(empty, image, 5);
    if (empty != NULL) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, false,
                "psImageRow failed to return NULL for out of range row input.\n");
        return 2;
    }
    rowcol->data.U16[0] = 11;
    rowcol->data.U16[2] = 22;
    //Test recycling of non-NULL vector & correct output
    rowcol = psImageCol(rowcol, image, 1);
    if (rowcol->data.U16[0] != 666 && rowcol->data.U16[2] != 4444) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, false,
                "psImageCol failed to return correct values.\n");
        return 3;
    }

    psFree(rowcol);
    psFree(image);
    return 0;
}

psS32 testImageRowColU8(void)
{
    psVector *rowcol = NULL;
    psVector *empty = NULL;
    psImage *image = NULL;
    psImage *emptyImage = NULL;

    image = psImageAlloc(3, 3, PS_TYPE_U8);
    rowcol = psVectorAlloc(3, PS_TYPE_U8);

    image->data.U8[0][0] = 244;
    image->data.U8[1][0] = 123;
    image->data.U8[2][0] = 123;
    image->data.U8[0][1] = 66;
    image->data.U8[1][1] = 199;
    image->data.U8[2][1] = 249;
    image->data.U8[0][2] = 199;
    image->data.U8[1][2] = 249;
    image->data.U8[2][2] = 199;

    //Test for error with NULL image
    empty = psImageCol(empty, emptyImage, 0);
    if (empty != NULL) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, false,
                "psImageCol failed to return NULL for NULL image input.\n");
        return 1;
    }
    //Test for error with Out of Range Row
    empty = psImageRow(empty, image, 5);
    if (empty != NULL) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, false,
                "psImageRow failed to return NULL for out of range row input.\n");
        return 2;
    }
    rowcol->data.U8[0] = 11;
    rowcol->data.U8[2] = 22;
    //Test recycling of non-NULL vector & correct output
    rowcol = psImageCol(rowcol, image, 1);
    if (rowcol->data.U8[0] != 123 && rowcol->data.U8[2] != 249) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, false,
                "psImageCol failed to return correct values.\n");
        return 3;
    }

    psFree(rowcol);
    psFree(image);
    return 0;
}
psS32 testImageRowColS8(void)
{
    psVector *rowcol = NULL;
    psVector *empty = NULL;
    psImage *image = NULL;
    psImage *emptyImage = NULL;

    image = psImageAlloc(3, 3, PS_TYPE_S8);
    rowcol = psVectorAlloc(3, PS_TYPE_S8);

    image->data.S8[0][0] = 44;
    image->data.S8[1][0] = 23;
    image->data.S8[2][0] = 23;
    image->data.S8[0][1] = 66;
    image->data.S8[1][1] = 99;
    image->data.S8[2][1] = 49;
    image->data.S8[0][2] = 99;
    image->data.S8[1][2] = 49;
    image->data.S8[2][2] = 99;

    //Test for error with NULL image
    empty = psImageCol(empty, emptyImage, 0);
    if (empty != NULL) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, false,
                "psImageCol failed to return NULL for NULL image input.\n");
        return 1;
    }
    //Test for error with Out of Range Row
    empty = psImageRow(empty, image, 5);
    if (empty != NULL) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, false,
                "psImageRow failed to return NULL for out of range row input.\n");
        return 2;
    }
    rowcol->data.S8[0] = 11;
    rowcol->data.S8[2] = 22;
    //Test recycling of non-NULL vector & correct output
    rowcol = psImageCol(rowcol, image, 1);
    if (rowcol->data.S8[0] != 23 && rowcol->data.S8[2] != 49) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, false,
                "psImageCol failed to return correct values.\n");
        return 3;
    }

    psFree(rowcol);
    psFree(image);
    return 0;
}
