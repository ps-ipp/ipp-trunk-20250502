/** @file tst_psImageInterpolate.c
 *
 * @brief Contains the tests for psImagePixelInterpolate
 *
 * @author Eric Van Alst, MHPCC
 *
 * @version $Revision: 1.1 $
 *          $Name: not supported by cvs2svn $
 * @date $Date: 2005-07-13 02:47:00 $
 *
 * Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 *
 */

#include "psTest.h"
#include "pslib_strict.h"

#define GENIMAGE(img,c,r,TYP,valueFcn) \
img = psImageAlloc(c,r,PS_TYPE_##TYP); \
for(psU32 row=0; row<r; row++) { \
    ps##TYP* imgRow = img->data.TYP[row]; \
    for(psU32 col=0; col<c; col++) { \
        imgRow[col] = (ps##TYP)(valueFcn); \
    } \
}

#define CHECK_INTERP_VALUE(img,x,y,mask,maskval,exposed,TYPE,expected) \
val = (psF32)psImagePixelInterpolate(img,x,y,mask,maskval,exposed,PS_INTERPOLATE_##TYPE); \
printf("returned = %.2f    expected = %.2f\n",val,expected); \
if(fabsf(val-expected)>FLT_EPSILON) { \
    psError(PS_ERR_UNKNOWN,true,"Return value is not as expected."); \
    return 1; \
}

#define CHECK_INTERP_BY_TYPE(TYPE) \
GENIMAGE(img1,10,10,TYPE,row+col) \
CHECK_INTERP_VALUE(img1,1.9,1.6,NULL,0,0,FLAT,2.0) \
CHECK_INTERP_VALUE(img1,4.0,2.0,NULL,0,0,BILINEAR,5.0) \
psFree(img1);

static psS32 testInterpolateFlatBilinear(void);
static psS32 testInterpolateError(void);
static psS32 testInterpolateMaskFlatBilinear(void);
static psS32 testInterpolate1D(void);

testDescription tests[] = {
                              {testInterpolateFlatBilinear,999,"psImagePixelInterpolate",0,false},
                              {testInterpolateError,999,"psImagePixelInterpolate",0,false},
                              {testInterpolateMaskFlatBilinear,999,"psImagePixelInterpolate",0,false},
                              {testInterpolate1D,999,"psImagePixelInterpolate",0,false},
                              {NULL}
                          };

psS32 main(psS32 argc, char* argv[])
{
    psLogSetLevel(PS_LOG_INFO);

    if ( !runTestSuite(stderr,"psImagePixelInterpolate",tests,argc,argv) ) {
        return 1;
    } else {
        return 0;
    }
}

psS32 testInterpolateFlatBilinear(void)
{
    psImage* img1;
    psF32 val = 0;

    // Perform simple(four neighbor) FLAT interpolation for all types
    // Perform bilinear interpolation for all types
    CHECK_INTERP_BY_TYPE(S8)
    CHECK_INTERP_BY_TYPE(S16)
    CHECK_INTERP_BY_TYPE(S32)
    CHECK_INTERP_BY_TYPE(S64)
    CHECK_INTERP_BY_TYPE(U8)
    CHECK_INTERP_BY_TYPE(U16)
    CHECK_INTERP_BY_TYPE(U32)
    CHECK_INTERP_BY_TYPE(U64)
    CHECK_INTERP_BY_TYPE(F32)
    CHECK_INTERP_BY_TYPE(F64)
    CHECK_INTERP_BY_TYPE(C32)
    CHECK_INTERP_BY_TYPE(C64)

    return 0;
}

psS32 testInterpolateError(void)
{
    psF32 val = 0;
    psImage* img1 = NULL;

    // Perform interpolation with NULL input image and verify return and error message
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate an error message");
    val = psImagePixelInterpolate(img1,1.2,1.2,NULL,0,5.0,PS_INTERPOLATE_FLAT);
    if(val != 5.0) {
        psError(PS_ERR_UNKNOWN,true,"Did not return the unexposed value");
        return 10;
    }

    // Perform interpolation with invalid input image type
    img1 = psImageAlloc(10,10,PS_TYPE_BOOL);
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate an error message");
    val = psImagePixelInterpolate(img1,1.2,1.2,NULL,0,10.0,PS_INTERPOLATE_FLAT);
    if(val != 10.0) {
        psError(PS_ERR_UNKNOWN,true,"Did not return the unexposed value");
        return 11;
    }

    // Perform interpolation with invalid method
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate an error message");
    val = psImagePixelInterpolate(img1,1.2,1.2,NULL,0,10.0,PS_INTERPOLATE_BILINEAR+1);
    if(val != 10.0) {
        psError(PS_ERR_UNKNOWN,true,"Did not return the unexposed value");
        return 12;
    }

    psFree(img1);

    return 0;
}

// The following assumptions about the test image are:
//
//                     col
//             0   1   2   3   4   5   6   7  ...
//
// row 0       0   1   2   3   4   5   6   7
// row 1       1   2   3   4   5   6   7   8
// row 2       2   3   4   5   6   7   8   9
// ...

psS32 testInterpolateMaskFlatBilinear(void)
{
    psImage* img1 = NULL;
    psImage* msk1 = NULL;
    psF32 val = 0;

    // Perform interpolate with mask and mask value set at nearest pixel to verify
    // the unexposed value is return
    GENIMAGE(img1,10,10,F32,row+col)
    GENIMAGE(msk1,10,10,U8,0)
    msk1->data.U8[1][1] = 1;
    CHECK_INTERP_VALUE(img1,1.9,1.6,msk1,1,10.0,FLAT,10.0)
    psFree(msk1);
    psFree(img1);

    // Perform interpolate with mask and one mask value for bilinear (upper left pixel)
    GENIMAGE(img1,10,10,F32,row+col)
    GENIMAGE(msk1,10,10,U8,0)
    msk1->data.U8[1][3] = 1;
    CHECK_INTERP_VALUE(img1,4.0,2.0,msk1,1,0,BILINEAR,5.25)
    psFree(msk1);
    psFree(img1);

    // Perform interpolate with mask and one mask value for bilinear (upper right pixel)
    GENIMAGE(img1,10,10,F32,row+col)
    GENIMAGE(msk1,10,10,U8,0)
    msk1->data.U8[1][4] = 1;
    CHECK_INTERP_VALUE(img1,4.0,2.0,msk1,1,0,BILINEAR,4.75)
    psFree(msk1);
    psFree(img1);

    // Perform interpolate with mask and one mask value for bilinear (lower left pixel)
    GENIMAGE(img1,10,10,F32,row+col)
    GENIMAGE(msk1,10,10,U8,0)
    msk1->data.U8[2][3] = 1;
    CHECK_INTERP_VALUE(img1,4.0,2.0,msk1,1,0,BILINEAR,5.25)
    psFree(msk1);
    psFree(img1);

    // Perform interpolate with mask and one mask value for bilinear (lower right pixel)
    GENIMAGE(img1,10,10,F32,row+col)
    GENIMAGE(msk1,10,10,U8,0)
    msk1->data.U8[2][4] = 1;
    CHECK_INTERP_VALUE(img1,4.0,2.0,msk1,1,0,BILINEAR,4.75)
    psFree(msk1);
    psFree(img1);

    // Perform interpolate with mask and one mask value for bilinear (upper pixels)
    GENIMAGE(img1,10,10,F32,row+col)
    GENIMAGE(msk1,10,10,U8,0)
    msk1->data.U8[1][3] = 1;
    msk1->data.U8[1][4] = 1;
    CHECK_INTERP_VALUE(img1,4.0,2.0,msk1,1,0,BILINEAR,5.5)
    psFree(msk1);
    psFree(img1);

    // Perform interpolate with mask and one mask value for bilinear (lower pixels)
    GENIMAGE(img1,10,10,F32,row+col)
    GENIMAGE(msk1,10,10,U8,0)
    msk1->data.U8[2][3] = 1;
    msk1->data.U8[2][4] = 1;
    CHECK_INTERP_VALUE(img1,4.0,2.0,msk1,1,0,BILINEAR,4.5)
    psFree(msk1);
    psFree(img1);

    // Perform interpolate with mask and one mask value for bilinear (left pixels)
    GENIMAGE(img1,10,10,F32,row+col)
    GENIMAGE(msk1,10,10,U8,0)
    msk1->data.U8[1][3] = 1;
    msk1->data.U8[2][3] = 1;
    CHECK_INTERP_VALUE(img1,4.0,2.0,msk1,1,0,BILINEAR,5.5)
    psFree(msk1);
    psFree(img1);

    // Perform interpolate with mask and one mask value for bilinear (right pixels)
    GENIMAGE(img1,10,10,F32,row+col)
    GENIMAGE(msk1,10,10,U8,0)
    msk1->data.U8[1][4] = 1;
    msk1->data.U8[2][4] = 1;
    CHECK_INTERP_VALUE(img1,4.0,2.0,msk1,1,0,BILINEAR,4.5)
    psFree(msk1);
    psFree(img1);

    // Perform interpolate with mask only one valid pixel (upper left)
    GENIMAGE(img1,10,10,F32,row+col)
    GENIMAGE(msk1,10,10,U8,0)
    msk1->data.U8[1][4] = 1;
    msk1->data.U8[2][3] = 1;
    msk1->data.U8[2][4] = 1;
    CHECK_INTERP_VALUE(img1,4.0,2.0,msk1,1,0,BILINEAR,4.0)
    psFree(msk1);
    psFree(img1);

    // Perform interpolate with mask only one valid pixel (upper right)
    GENIMAGE(img1,10,10,F32,row+col)
    GENIMAGE(msk1,10,10,U8,0)
    msk1->data.U8[1][3] = 1;
    msk1->data.U8[2][3] = 1;
    msk1->data.U8[2][4] = 1;
    CHECK_INTERP_VALUE(img1,4.0,2.0,msk1,1,0,BILINEAR,5.0)
    psFree(msk1);
    psFree(img1);

    // Perform interpolate with mask only one valid pixel (lower left)
    GENIMAGE(img1,10,10,F32,row+col)
    GENIMAGE(msk1,10,10,U8,0)
    msk1->data.U8[1][3] = 1;
    msk1->data.U8[1][4] = 1;
    msk1->data.U8[2][4] = 1;
    CHECK_INTERP_VALUE(img1,4.0,2.0,msk1,1,0,BILINEAR,5.0)
    psFree(msk1);
    psFree(img1);

    // Perform interpolate with mask only one valid pixel (lower right)
    GENIMAGE(img1,10,10,F32,row+col)
    GENIMAGE(msk1,10,10,U8,0)
    msk1->data.U8[1][3] = 1;
    msk1->data.U8[1][4] = 1;
    msk1->data.U8[2][3] = 1;
    CHECK_INTERP_VALUE(img1,4.0,2.0,msk1,1,0,BILINEAR,6.0)
    psFree(msk1);
    psFree(img1);

    // Perform interpolate with mask no valid pixels
    GENIMAGE(img1,10,10,F32,row+col)
    GENIMAGE(msk1,10,10,U8,0)
    msk1->data.U8[1][3] = 1;
    msk1->data.U8[1][4] = 1;
    msk1->data.U8[2][3] = 1;
    msk1->data.U8[2][4] = 1;
    CHECK_INTERP_VALUE(img1,4.0,2.0,msk1,1,0,BILINEAR,0.0)
    psFree(msk1);
    psFree(img1);
    return 0;
}

// Perform interpolation for a 1D image
psS32 testInterpolate1D(void)
{
    psImage* img1 = NULL;
    psF32 val = 0;

    GENIMAGE(img1,10,1,F32,row+col)
    CHECK_INTERP_VALUE(img1,4.0,0.0,NULL,0,100.0,BILINEAR,3.5)

    psFree(img1);

    return 0;
}

