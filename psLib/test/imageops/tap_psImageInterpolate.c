/** @file tst_psImageInterpolate.c
 *
 * @brief Contains the tests for psImagePixelInterpolate
 *
 * @author Eric Van Alst, MHPCC
 *
 * @version $Revision: 1.3 $
 *          $Name: not supported by cvs2svn $
 * @date $Date: 2007-03-14 21:20:28 $
 *
 * Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 *
 */
#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include "tap.h"
#include "pstap.h"

#define GENIMAGE(img,c,r,TYP,valueFcn) \
img = psImageAlloc(c,r,PS_TYPE_##TYP); \
for(psU32 row=0; row<r; row++) { \
    ps##TYP* imgRow = img->data.TYP[row]; \
    for(psU32 col=0; col<c; col++) { \
        imgRow[col] = (ps##TYP)(valueFcn); \
    } \
}

#define CHECK_INTERP_VALUE(img,x,y,mask,maskval,exposed,TYPE,expected) \
{ \
    psF32 val = (psF32)psImagePixelInterpolate(img,x,y,mask,maskval,exposed,PS_INTERPOLATE_##TYPE); \
    ok(fabsf(val-expected)<FLT_EPSILON, "psImagePixelInterpolate() correct"); \
}

#define CHECK_INTERP_BY_TYPE(TYPE) \
{ \
    psImage* img1; \
    GENIMAGE(img1,10,10,TYPE,row+col) \
    CHECK_INTERP_VALUE(img1,1.9,1.6,NULL,0,0,FLAT,2.0) \
    CHECK_INTERP_VALUE(img1,4.0,2.0,NULL,0,0,BILINEAR,5.0) \
    psFree(img1); \
}

#define CHECK_INTERP_BY_TYPE_ORIG(TYPE) \
{ \
    psImage* img1; \
    GENIMAGE(img1,10,10,TYPE,row+col) \
    CHECK_INTERP_VALUE(img1,1.9,1.6,NULL,0,0,FLAT,2.0) \
}

psS32 main(psS32 argc, char* argv[])
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    plan_tests(53);

    // testInterpolateFlatBilinear(void)
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

    // testInterpolateError(void)
    // Perform interpolation with NULL input image
    // Following should generate an error message
    // XXX: we do not test the error generation
    {
        psMemId id = psMemGetId();
        psF32 val = psImagePixelInterpolate(NULL,1.2,1.2,NULL,0,5.0,PS_INTERPOLATE_FLAT);
        ok(val == 5.0, "psImagePixelInterpolate() returned the unexposed value (NULL image)");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Perform interpolation with unallowed input image type
    // Following should generate an error message
    // XXX: we do not test the error generation
    {
        psMemId id = psMemGetId();
        psImage *img1 = psImageAlloc(10,10,PS_TYPE_BOOL);
        psF32 val = psImagePixelInterpolate(img1,1.2,1.2,NULL,0,10.0,PS_INTERPOLATE_FLAT);
        ok(val == 10.0, "psImagePixelInterpolate() returned the unexposed value (unallowed image type)");
        psFree(img1);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Perform interpolation with unallowed method
    // Following should generate an error message
    // XXX: we do not test the error generation
    {
        psMemId id = psMemGetId();
        psImage *img1 = psImageAlloc(10,10,PS_TYPE_BOOL);
        psF32 val = psImagePixelInterpolate(img1,1.2,1.2,NULL,0,10.0,PS_INTERPOLATE_BILINEAR+1);
        ok(val == 10.0, "psImagePixelInterpolate() returned the unexposed value (unallowed interpolation method)");
        psFree(img1);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
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
    // testInterpolateMaskFlatBilinear(void)
    psImage* img1 = NULL;
    psImage* msk1 = NULL;

    // Perform interpolate with mask and mask value set at nearest pixel to verify
    // the unexposed value is return
    {
        psMemId id = psMemGetId();
        GENIMAGE(img1,10,10,F32,row+col)
        GENIMAGE(msk1,10,10,U8,0)
        msk1->data.U8[1][1] = 1;
        CHECK_INTERP_VALUE(img1,1.9,1.6,msk1,1,10.0,FLAT,10.0)
        psFree(msk1);
        psFree(img1);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Perform interpolate with mask and one mask value for bilinear (upper left pixel)
    {
        psMemId id = psMemGetId();
        GENIMAGE(img1,10,10,F32,row+col)
        GENIMAGE(msk1,10,10,U8,0)
        msk1->data.U8[1][3] = 1;
        CHECK_INTERP_VALUE(img1,4.0,2.0,msk1,1,0,BILINEAR,5.25)
        psFree(msk1);
        psFree(img1);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Perform interpolate with mask and one mask value for bilinear (upper right pixel)
    {
        psMemId id = psMemGetId();
        GENIMAGE(img1,10,10,F32,row+col)
        GENIMAGE(msk1,10,10,U8,0)
        msk1->data.U8[1][4] = 1;
        CHECK_INTERP_VALUE(img1,4.0,2.0,msk1,1,0,BILINEAR,4.75)
        psFree(msk1);
        psFree(img1);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Perform interpolate with mask and one mask value for bilinear (lower left pixel)
    {
        psMemId id = psMemGetId();
        GENIMAGE(img1,10,10,F32,row+col)
        GENIMAGE(msk1,10,10,U8,0)
        msk1->data.U8[2][3] = 1;
        CHECK_INTERP_VALUE(img1,4.0,2.0,msk1,1,0,BILINEAR,5.25)
        psFree(msk1);
        psFree(img1);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Perform interpolate with mask and one mask value for bilinear (lower right pixel)
    {
        psMemId id = psMemGetId();
        GENIMAGE(img1,10,10,F32,row+col)
        GENIMAGE(msk1,10,10,U8,0)
        msk1->data.U8[2][4] = 1;
        CHECK_INTERP_VALUE(img1,4.0,2.0,msk1,1,0,BILINEAR,4.75)
        psFree(msk1);
        psFree(img1);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Perform interpolate with mask and one mask value for bilinear (upper pixels)
    {
        psMemId id = psMemGetId();
        GENIMAGE(img1,10,10,F32,row+col)
        GENIMAGE(msk1,10,10,U8,0)
        msk1->data.U8[1][3] = 1;
        msk1->data.U8[1][4] = 1;
        CHECK_INTERP_VALUE(img1,4.0,2.0,msk1,1,0,BILINEAR,5.5)
        psFree(msk1);
        psFree(img1);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Perform interpolate with mask and one mask value for bilinear (lower pixels)
    {
        psMemId id = psMemGetId();
        GENIMAGE(img1,10,10,F32,row+col)
        GENIMAGE(msk1,10,10,U8,0)
        msk1->data.U8[2][3] = 1;
        msk1->data.U8[2][4] = 1;
        CHECK_INTERP_VALUE(img1,4.0,2.0,msk1,1,0,BILINEAR,4.5)
        psFree(msk1);
        psFree(img1);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Perform interpolate with mask and one mask value for bilinear (left pixels)
    {
        psMemId id = psMemGetId();
        GENIMAGE(img1,10,10,F32,row+col)
        GENIMAGE(msk1,10,10,U8,0)
        msk1->data.U8[1][3] = 1;
        msk1->data.U8[2][3] = 1;
        CHECK_INTERP_VALUE(img1,4.0,2.0,msk1,1,0,BILINEAR,5.5)
        psFree(msk1);
        psFree(img1);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Perform interpolate with mask and one mask value for bilinear (right pixels)
    {
        psMemId id = psMemGetId();
        GENIMAGE(img1,10,10,F32,row+col)
        GENIMAGE(msk1,10,10,U8,0)
        msk1->data.U8[1][4] = 1;
        msk1->data.U8[2][4] = 1;
        CHECK_INTERP_VALUE(img1,4.0,2.0,msk1,1,0,BILINEAR,4.5)
        psFree(msk1);
        psFree(img1);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Perform interpolate with mask only one valid pixel (upper left)
    {
        psMemId id = psMemGetId();
        GENIMAGE(img1,10,10,F32,row+col)
        GENIMAGE(msk1,10,10,U8,0)
        msk1->data.U8[1][4] = 1;
        msk1->data.U8[2][3] = 1;
        msk1->data.U8[2][4] = 1;
        CHECK_INTERP_VALUE(img1,4.0,2.0,msk1,1,0,BILINEAR,4.0)
        psFree(msk1);
        psFree(img1);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Perform interpolate with mask only one valid pixel (upper right)
    {
        psMemId id = psMemGetId();
        GENIMAGE(img1,10,10,F32,row+col)
        GENIMAGE(msk1,10,10,U8,0)
        msk1->data.U8[1][3] = 1;
        msk1->data.U8[2][3] = 1;
        msk1->data.U8[2][4] = 1;
        CHECK_INTERP_VALUE(img1,4.0,2.0,msk1,1,0,BILINEAR,5.0)
        psFree(msk1);
        psFree(img1);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Perform interpolate with mask only one valid pixel (lower left)
    {
        psMemId id = psMemGetId();
        GENIMAGE(img1,10,10,F32,row+col)
        GENIMAGE(msk1,10,10,U8,0)
        msk1->data.U8[1][3] = 1;
        msk1->data.U8[1][4] = 1;
        msk1->data.U8[2][4] = 1;
        CHECK_INTERP_VALUE(img1,4.0,2.0,msk1,1,0,BILINEAR,5.0)
        psFree(msk1);
        psFree(img1);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Perform interpolate with mask only one valid pixel (lower right)
    {
        psMemId id = psMemGetId();
        GENIMAGE(img1,10,10,F32,row+col)
        GENIMAGE(msk1,10,10,U8,0)
        msk1->data.U8[1][3] = 1;
        msk1->data.U8[1][4] = 1;
        msk1->data.U8[2][3] = 1;
        CHECK_INTERP_VALUE(img1,4.0,2.0,msk1,1,0,BILINEAR,6.0)
        psFree(msk1);
        psFree(img1);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Perform interpolate with mask no valid pixels
    {
        psMemId id = psMemGetId();
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
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Perform interpolation for a 1D image
    {
        psMemId id = psMemGetId();
        psImage* img1 = NULL;
        GENIMAGE(img1,10,1,F32,row+col)
        CHECK_INTERP_VALUE(img1,4.0,0.0,NULL,0,100.0,BILINEAR,3.5)
        psFree(img1);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
}

