/** @file  tst_psImage.c
 *
 *  @brief Contains the tests for psImage.[ch]
 *
 *
 *  @author Robert DeSonia, MHPCC
 *
 *  @version $Revision: 1.11 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2006-06-15 02:29:12 $
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

static psS32 testImageAlloc(void);
static psS32 testRegion(void);
static psS32 testRegion2(void);
static psS32 testRegion3(void);
static psS32 testImageInit(void);
static psS32 testImageSet(void);
static psS32 testImageGet(void);

testDescription tests[] = {
                              {testImageAlloc,546,"psImageAlloc",0,false},
                              {testImageAlloc,548,"psImageFree",0,true},
                              {testRegion,790,"psRegionSet",0,false},
                              {testRegion,791,"psRegionFromString",0,true},
                              {testRegion2,792,"psRegionForImage",0,false},
                              {testRegion3,793,"psRegionForSquare",0,false},
                              {testImageInit,794,"psImageInit",0,false},
                              {testImageSet,795,"psImageSet",0,false},
                              {testImageGet,666,"psImageGet",0,false},
                              {NULL}
                          };

psS32 main(psS32 argc, char* argv[])
{
    psLogSetLevel(PS_LOG_INFO);

    return ! runTestSuite(stderr,"psImage",tests,argc,argv);
}

psS32 testImageAlloc(void)
{
    psImage* image = NULL;
    psU32 sizes = 6;
    psU32 numCols[] = {
                          0,1,1,100,100,150
                      };
    psU32 numRows[] = {
                          0,1,100,1,150,100
                      };
    psU32 types = 12;
    psElemType type[] = { PS_TYPE_S8, PS_TYPE_S16, PS_TYPE_S32, PS_TYPE_S64,
                          PS_TYPE_U8, PS_TYPE_U16, PS_TYPE_U32, PS_TYPE_U64,
                          PS_TYPE_F32, PS_TYPE_F64, PS_TYPE_C32, PS_TYPE_C64};

    psLogMsg(__func__,PS_LOG_INFO,"#546 - psImageAlloc shall allocate memory for a psImage structure");

    for (psU32 t=0;t<types;t++) {
        psLogMsg(__func__,PS_LOG_INFO,"Testing psImage with type %xh",type[t]);

        for (psU32 i=0;i<sizes;i++) {

            if (numRows[i] == 0 || numCols[i] == 0) {
                psLogMsg(__func__,PS_LOG_INFO,"Following should be an error.");
            }

            image = psImageAlloc(numCols[i],numRows[i],type[t]);

            if (image == NULL) {
                if (numRows[i] == 0 || numCols[i] == 0) {
                    continue;
                }
                psError(PS_ERR_UNKNOWN, true,"psImageAlloc returned NULL for type %x, size %dx%d.",
                        type[t], numCols[i], numRows[i]);
                psFree(image);
                return 1;
            }

            if (image->type.dimen != PS_DIMEN_IMAGE || image->type.type != type[t]) {
                psError(PS_ERR_UNKNOWN, true,"psImageAlloc allocated wrong dimen/type (%d/%d), should be "
                        "PS_IMAGE_DIMEN/%d", image->type.dimen, image->type.type, type[t]);
                psFree(image);
                return 2;
            }

            if (image->numCols != numCols[i] || image->numRows != numRows[i]) {
                psError(PS_ERR_UNKNOWN, true,"psImageAlloc allocated wrong size %dx%d, should be %dx%d (type = %d)",
                        image->numCols, image->numRows, numCols[i], numRows[i],type[t]);
                psFree(image);
                return 3;
            }

            if (image->col0 != 0 || image->row0 != 0) {
                psError(PS_ERR_UNKNOWN, true,"psImageAlloc returned row0/col0 of %d/%d.  Should be 0/0.",
                        image->row0, image->row0);
                psFree(image);
                return 4;
            }

            if (image->parent != NULL) {
                psError(PS_ERR_UNKNOWN, true,"psImageAlloc returned non-NULL parent");
                psFree(image);
                return 5;
            }

            if (image->children != NULL) {
                psError(PS_ERR_UNKNOWN, true,"psImageAlloc returned non-NULL children array");
                psFree(image);
                return 7;
            }

            switch (type[t]) {
            case PS_TYPE_U16: {
                    psU32 rows = numRows[i];
                    psU32 cols = numCols[i];

                    for (psS32 r=0;r<rows;r++) {
                        for (psS32 c=0;c<cols;c++) {
                            image->data.U16[r][c] = 2*c+r;
                        }
                    }
                    for (psS32 r=0;r<rows;r++) {
                        for (psS32 c=0;c<cols;c++) {
                            if (image->data.U16[r][c] != 2*c+r) {
                                psError(PS_ERR_UNKNOWN, true,"Could not set all pixels in uint16 image at (%d,%d)",c,r);
                                psFree(image);
                                return 8;
                            }
                        }
                    }
                }
                break;
            case PS_TYPE_F32: {
                    psU32 rows = numRows[i];
                    psU32 cols = numCols[i];

                    for (psS32 r=0;r<rows;r++) {
                        for (psS32 c=0;c<cols;c++) {
                            image->data.F32[r][c] = 2.0f*c+r;
                        }
                    }
                    for (psS32 r=0;r<rows;r++) {
                        for (psS32 c=0;c<cols;c++) {
                            if (fabsf(image->data.F32[r][c] - (2.0f*c+r)) > FLT_EPSILON) {
                                psError(PS_ERR_UNKNOWN, true,"Could not set all pixels in float image at (%d,%d)",c,r);
                                psFree(image);
                                return 8;
                            }
                        }
                    }
                }
                break;
            case PS_TYPE_F64: {
                    psU32 rows = numRows[i];
                    psU32 cols = numCols[i];

                    for (psS32 r=0;r<rows;r++) {
                        for (psS32 c=0;c<cols;c++) {
                            image->data.F64[r][c] = 2.0f*c+r;
                        }
                    }
                    for (psS32 r=0;r<rows;r++) {
                        for (psS32 c=0;c<cols;c++) {
                            if (fabs(image->data.F64[r][c] - (2.0f*c+r)) > DBL_EPSILON) {
                                psError(PS_ERR_UNKNOWN, true,"Could not set all pixels in double image at (%d,%d)",c,r);
                                psFree(image);
                                return 8;
                            }
                        }
                    }
                }
                break;
            case PS_TYPE_C32: {
                    psU32 rows = numRows[i];
                    psU32 cols = numCols[i];

                    for (psS32 r=0;r<rows;r++) {
                        for (psS32 c=0;c<cols;c++) {
                            image->data.C32[r][c] = r + I * c;
                        }
                    }
                    for (psS32 r=0;r<rows;r++) {
                        for (psS32 c=0;c<cols;c++) {
                            if (fabsf(crealf(image->data.C32[r][c]) - r) > FLT_EPSILON ||
                                    fabsf(cimagf(image->data.C32[r][c]) - c) > FLT_EPSILON ) {
                                psError(PS_ERR_UNKNOWN, true,"Could not set all pixels in complex image at (%d,%d)",c,r);
                                psFree(image);
                                return 8;
                            }
                        }
                    }
                }
                break;
            default: {
                    // ignore type and just use as byte bucket.
                    psU32 rows = numRows[i];
                    psU32 cols = numCols[i]*PSELEMTYPE_SIZEOF(type[t]);

                    for (psS32 r=0;r<rows;r++) {
                        for (psS32 c=0;c<cols;c++) {
                            image->data.U8[r][c] = (uint8_t)(r + c);
                        }
                    }
                    for (psS32 r=0;r<rows;r++) {
                        for (psS32 c=0;c<cols;c++) {
                            if (image->data.U8[r][c] != (uint8_t)(r + c)) {
                                psError(PS_ERR_UNKNOWN, true,"Could not set all pixels in image (type=%d) at (%d,%d)",
                                        type[t],c,r);
                                psFree(image);
                                return 8;
                            }
                        }
                    }
                }
            }
            psFree(image);
        }
    }

    // #548: Verify no memory leaks or corruption are detected after a valid psImage structure with multiple
    // children is freed.
    image = psImageAlloc(100,100,PS_TYPE_F32);
    psImageSubset(image,(psRegion) {
                      50,0,70,20
                  }
                 );
    psImageSubset(image,(psRegion) {
                      70,20,90,40
                  }
                 );

    psFree(image);

    return 0;
}

static psS32 testRegion(void)
{
    int testNum = 0;


    // Testpoint #790

    psRegion region = psRegionSet(1,2,3,4);

    testNum++;
    if (region.x0 != 1 || region.x1 != 2 || region.y0 != 3 || region.y1 != 4) {
        psError(PS_ERR_UNKNOWN, false,
                "The region attributes are not set properly (%s)",
                psRegionToString(region));
        return testNum;
    }

    // Testpoint #791

    region = psRegionFromString("[1:2,3:4]");

    testNum++;
    if (region.x0 != 0 || region.x1 != 2 || region.y0 != 2 || region.y1 != 4) {
        psError(PS_ERR_UNKNOWN, false,
                "The region attributes are not set properly (%s)",
                psRegionToString(region));
        return testNum;
    }

    region = psRegionFromString("[1:2,3:]");

    testNum++;
    if (! isnan(region.x0)) {
        psError(PS_ERR_UNKNOWN, false,
                "psRegionFromString returned a non-NULL pointer given a malformed string.");
        return testNum;
    }

    return 0;
}

//psRegionForImage//
static psS32 testRegion2(void)
{
    psImage *in;
    psRegion inReg;
    psRegion out;
    in = psImageAlloc(1, 1, PS_TYPE_S32);
    inReg = psRegionSet(1, 2, 1, 2);
    out = psRegionForImage(in,inReg);
    psRegion inReg2;
    psRegion out2;
    inReg2 = psRegionSet(-1, 0, -2, -1);
    out2 = psRegionForImage(in, inReg2);

    if( out.x0 != 1 || out.x1 != 1 || out.y0 != 1 || out.y1 != 1 ) {
        psError(PS_ERR_UNKNOWN, true, "Error:  Region For Image returned incorrect values.\n");
    }
    if( out2.x0 != 0 || out2.x1 != 1 || out2.y0 != 0 || out2.y1 != 0 ) {
        psError(PS_ERR_UNKNOWN, true, "Error:  Region For Image returned incorrect values.\n");
    }

    psFree(in);
    return 0;
}

//psRegionForSquare//
static psS32 testRegion3(void)
{
    float X = 1;
    float Y = 1;
    float RAD = 1;
    psRegion out;
    out = psRegionForSquare(X, Y, RAD);
    if (out.x0 != 0 || out.x1 != 3 || out.y0 != 0 || out.y1!= 3)
        psError(PS_ERR_UNKNOWN, true, "Error:  Region For Square returned incorrect values.\n");
    return 0;
}

//Initialize an image with a given value.//
static psS32 testImageInit(void)
{
    psImage *in1 = NULL;
    psImage *in2 = NULL;
    psImage *in3 = NULL;
    psImage *in4 = NULL;
    int nRow = 1;
    int nCol = 1;
    in1 = psImageAlloc(nRow, nCol, PS_TYPE_U8);
    if ( !psImageInit(in1, -1 ) ) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "ImageInit failed.  U8 Case - 1x1\n");
    }
    nRow = 5;
    in2 = psImageAlloc(nRow, nCol, PS_TYPE_F32);
    if ( !psImageInit(in2, 3) ) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "ImageInit failed.  F32 Case - 5x1\n");
    }
    nCol = 5;
    in3 = psImageAlloc(nRow, nCol, PS_TYPE_F64);
    if ( !psImageInit(in3, 3.141) ) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "ImageInit failed.  F64 Case - 5x5\n");
    }
    in4 = psImageAlloc(nRow, nCol, PS_TYPE_S32);
    if ( !psImageInit(in4, 3) ) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "ImageInit failed.  S32 Case - 5x5\n");
    }
    psFree(in1);
    psFree(in2);
    psFree(in3);
    psFree(in4);
    return 0;
}

static psS32 testImageSet(void)
{
    psImage *image = NULL;
    image = psImageAlloc(5, 4, PS_TYPE_S32);

    //Set the position of the subimage relative to the parent.
    image->col0 = 10;
    image->row0 = 10;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 5; j++) {
            image->data.S32[i][j] = i+j;
        }
    }

    //Attempt to set a position in a NULL psImage*
    psImage *none = NULL;
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate error message");
    if ( psImageSet(none, 1, 1, 1) ) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                "psImageSet failed to return false when passed a NULL image.\n");
        return 1;
    }

    //Attempt to set a position in a psImage* with negative numCols, numRows
    none = psImageAlloc(2, 2, PS_TYPE_S32);
    *(int*)&none->numCols = -1;
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate error message");
    if ( psImageSet(none, 1, 1, 1) ) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                "psImageSet failed to return false when passed an image with negative numCols.\n");
        return 2;
    }
    *(int*)&none->numCols = 2;
    *(int*)&none->numRows = -1;
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate error message");
    if ( psImageSet(none, 1, 1, 1) ) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                "psImageSet failed to return false when passed an image with negative numRows.\n");
        return 3;
    }

    //Attempt to set a position in a psImage* with negative col0, row0
    *(int*)&none->numRows = 2;
    none->row0 = -1;
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate error message");
    if ( psImageSet(none, 1, 1, 1) ) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                "psImageSet failed to return false when passed an image with negative col0.\n");
        return 4;
    }
    none->row0 = 0;
    none->col0 = -1;
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate error message");
    if ( psImageSet(none, 1, 1, 1) ) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                "psImageSet failed to return false when passed an image with negative col0.\n");
        return 5;
    }
    psFree(none);

    //Try to set a position inside of the subimage.
    if ( !psImageSet(image, 14, 12, 666) ) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                "psImageSet failed to return true when passed valid parameters.\n");
        return 6;
    }
    if (image->data.S32[2][4] != 666) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                "psImageSet failed to set the data values correctly. value=%d\n", image->data.S32[2][4]);
        return 7;
    }
    //Try to set a position inside of the subimage from the tail.
    if ( !psImageSet(image, -1, -1, 666) ) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                "psImageSet failed to return true when passed valid parameters.\n");
        return 8;
    } else if  (image->data.S32[3][4] != 666) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                "psImageSet failed to set (from the tail) the data values correctly.\n");
        printf("\n value at 14,14 is %d\n", image->data.S32[3][4]);
        return 9;
    }

    //Try to set a position outside of the subimage but inside of the parent image.
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate error message");
    if ( psImageSet(image, 0, 0, 666) ) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, false,
                "psImageSet failed to return false when passed an invalid location.\n");
        return 10;
    }
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate error message");
    if ( psImageSet(image, 9, 10, 666) ) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, false,
                "psImageSet failed to return false when passed an invalid location.\n");
        return 11;
    }

    //Try to set a position outside of the subimage.
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate error message");
    if ( psImageSet(image, 15, 14, 666) ) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, false,
                "psImageSet failed to return false when passed an invalid location.\n");
        return 12;
    }
    //Try to set a position outside of the subimage, indexing from the tail.
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate error message");
    if ( psImageSet(image, -6, -1, 666) ) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, false,
                "psImageSet failed to return false when passed an invalid location.\n");
        return 13;
    }


    psFree(image);
    return 0;
}

static psS32 testImageGet(void)
{
    psImage *image = NULL;
    image = psImageAlloc(5, 3, PS_TYPE_S32);

    //Set the position of the subimage relative to the parent.
    image->col0 = 10;
    image->row0 = 10;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 5; j++) {
            image->data.S32[i][j] = i+j;
        }
    }

    //Attempt to get a position in a NULL psImage*
    psImage *none = NULL;
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate error message");
    if ( !isnan( psImageGet(none, 1, 1) ) ) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                "psImageGet failed to return false when passed a NULL image.\n");
        return 1;
    }

    //Attempt to get a position in a psImage* with negative numCols, numRows
    none = psImageAlloc(2, 2, PS_TYPE_S32);
    *(int*)&none->numCols = -1;
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate error message");
    if ( !isnan( psImageGet(none, 1, 1) ) ) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                "psImageGet failed to return false when passed an image with negative numCols.\n");
        return 2;
    }
    *(int*)&none->numCols = 2;
    *(int*)&none->numRows = -1;
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate error message");
    if ( !isnan( psImageGet(none, 1, 1) ) ) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                "psImageGet failed to return false when passed an image with negative numRows.\n");
        return 3;
    }

    //Attempt to get a position in a psImage* with negative col0, row0
    *(int*)&none->numRows = 2;
    none->row0 = -1;
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate error message");
    if ( !isnan( psImageGet(none, 1, 1) ) ) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                "psImageGet failed to return false when passed an image with negative col0.\n");
        return 4;
    }
    none->row0 = 0;
    none->col0 = -1;
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate error message");
    if ( !isnan( psImageGet(none, 1, 1) ) ) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                "psImageGet failed to return false when passed an image with negative col0.\n");
        return 5;
    }
    psFree(none);

    //Try to get a position inside of the subimage.
    if ( psImageGet(image, 14, 12) != 6 ) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                "psImageGet failed to return the correct value.\n");
        return 6;
    }
    //Try to get a position inside of the subimage from the tail.
    if ( psImageGet(image, -1, -1) != 6 ) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                "psImageGet failed to return the correct value.\n");
        return 8;
    }

    //Try to get a position outside of the subimage but inside of the parent image.
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate error message");
    if ( !isnan( psImageGet(image, 1, 1) ) ) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, false,
                "psImageGet failed to return NAN when passed an invalid location.\n");
        return 10;
    }
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate error message");
    if ( !isnan( psImageGet(image, 9, 10) ) ) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, false,
                "psImageGet failed to return NAN when passed an invalid location.\n");
        return 11;
    }

    //Try to set a position outside of the subimage.
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate error message");
    if ( !isnan( psImageGet(image, 15, 14) ) ) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, false,
                "psImageGet failed to return NAN when passed an invalid location.\n");
        return 12;
    }
    //Try to set a position outside of the subimage, indexing from the tail.
    psLogMsg(__func__,PS_LOG_INFO,"Following should generate error message");
    if ( !isnan( psImageGet(image, -6, -1) ) ) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, false,
                "psImageGet failed to return NAN when passed an invalid location.\n");
        return 13;
    }


    psFree(image);
    return 0;
}

