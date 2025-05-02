/** @file  tst_psImage.c
 *
 *  @brief Contains the tests for psImage.[ch]
 *
 *
 *  @author Robert DeSonia, MHPCC
 *
 *  @version $Revision: 1.8 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2008-05-05 00:09:04 $
 *
 *  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 */

#include <math.h>
#include <float.h>
#include <string.h>
#include <stdlib.h>
#include "pslib.h"
#include "tap.h"
#include "pstap.h"


#define OK(exp) \
exp ? setOkay(1) : setOkay(0)

#define RET_OK          \
if ( ! Okay ) {     \
    psFree(image);  \
    return;         \
}

int main(int argc, char* argv[])
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    plan_tests(402);

    // testImageAlloc()
    {
        psMemId id = psMemGetId();
        psImage* image = NULL;
        psU32 sizes = 6;
        psU32 numCols[] = {0,1,1,100,100,150};
        psU32 numRows[] = {0,1,100,1,150,100};
        psU32 types = 12;
        psElemType type[] = { PS_TYPE_S8, PS_TYPE_S16, PS_TYPE_S32, PS_TYPE_S64,
                              PS_TYPE_U8, PS_TYPE_U16, PS_TYPE_U32, PS_TYPE_U64,
                              PS_TYPE_F32, PS_TYPE_F64 };
        for (psU32 t=0;t<types;t++) {
            for (psU32 i=0;i<sizes;i++) {
                // Following should be an error if numRows[i] == 0 || numCols[i]==0
                image = psImageAlloc(numCols[i],numRows[i],type[t]);
                if (image == NULL) {
                    if (numRows[i] == 0 || numCols[i] == 0) {
                        continue;
                    }
                    ok(0, "psImageAlloc returned NULL for type %x, size %dx%d.",
                       type[t], numCols[i], numRows[i]);
                    psFree(image);
                }

                ok(image->type.dimen==PS_DIMEN_IMAGE && image->type.type==type[t],
                    "psImageAlloc allocated dimen/type (%d/%d), expecting "
                    "PS_IMAGE_DIMEN/%d", image->type.dimen,
                    image->type.type, type[t]);
                ok(image->numCols == numCols[i] && image->numRows == numRows[i],
                    "psImageAlloc allocated size %dx%d, expecting be %dx%d "
                    "(type = %d)", image->numCols, image->numRows, numCols[i],
                    numRows[i],type[t]);
                ok(image->col0 == 0 && image->row0 == 0,
                    "psImageAlloc returned row0/col0 of %d/%d.  Expected 0/0.",
                    image->row0, image->row0);
                ok(image->parent == NULL, "psImageAlloc returned NULL parent");
                ok(image->children == NULL,
                    "psImageAlloc returned NULL children array");


                if (image->children != NULL) {
                    psFree(image);
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
                        bool errorFlag = false;
                        for (psS32 r=0;r<rows;r++) {
                            for (psS32 c=0;c<cols;c++) {
                                if (image->data.U16[r][c] != 2*c+r) {
                                    errorFlag = true;
                                }
                            }
                        }
                        ok(!errorFlag, "psImageAlloc() set data correctly (U16)");
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
                        bool errorFlag = false;
                        for (psS32 r=0;r<rows;r++) {
                            for (psS32 c=0;c<cols;c++) {
                                if (fabsf(image->data.F32[r][c] - (2.0f*c+r)) > FLT_EPSILON) {
                                    errorFlag = true;
                                }
                            }
                        }
                        ok(!errorFlag, "psImageAlloc() set data correctly (F32)");
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
                        bool errorFlag = false;
                        for (psS32 r=0;r<rows;r++) {
                            for (psS32 c=0;c<cols;c++) {
                                if (fabs(image->data.F64[r][c] - (2.0f*c+r)) > DBL_EPSILON) {
                                    errorFlag = true;
                                }
                            }
                        }
                        ok(!errorFlag, "psImageAlloc() set data correctly (F64)");
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
                        bool errorFlag = false;
                        for (psS32 r=0;r<rows;r++) {
                            for (psS32 c=0;c<cols;c++) {
                                if (image->data.U8[r][c] != (uint8_t)(r + c)) {
                                    errorFlag = true;
                                }
                            }
                        }
                        ok(!errorFlag, "psImageAlloc() set data correctly (default)");
                    }
                }
                psFree(image);
            }
        }

        // #548: Verify no memory leaks or corruption are detected after a valid psImage structure with multiple
        // children is freed.
        image = psImageAlloc(100,100,PS_TYPE_F32);
        psImageSubset(image,((psRegion) {50,0,70,20}));
        psImageSubset(image,((psRegion) {70,20,90,40}));
        psFree(image);
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }


    // testRegion()
    {
        psMemId id = psMemGetId();
        psRegion region = psRegionSet(1,2,3,4);
        char *tmpStr = psRegionToString(region);
        ok(!(region.x0 != 1 || region.x1 != 2 || region.y0 != 3 || region.y1 != 4),
            "The region attributes are set to (%s)", tmpStr);
        psFree(tmpStr);
        region = psRegionFromString("[1:2,3:4]");
        tmpStr = psRegionToString(region);
        ok(!(region.x0 != 0 || region.x1 != 2 || region.y0 != 2 || region.y1 != 4),
            "The region attributes are set to (%s)", tmpStr);
        psFree(tmpStr);
        region = psRegionFromString("[1:2,3:]");
        ok(isnan(region.x0),
            "psRegionFromString returned a NULL pointer given a malformed string.");
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }


    //psRegionForImage
    {
        psMemId id = psMemGetId();
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
    
        ok(!( out.x0 != 1 || out.x1 != 1 || out.y0 != 1 || out.y1 != 1 ),
           "Region For Image returned correct values");
    
        ok(!( out2.x0 != 0 || out2.x1 != 1 || out2.y0 != 0 || out2.y1 != 0 ),
           "Region For Image returned correct values");
        psFree(in);
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }


    //psRegionForSquare//
    {
        psMemId id = psMemGetId();
        float X = 1;
        float Y = 1;
        float RAD = 1;
        psRegion out;
        out = psRegionForSquare(X, Y, RAD);
        ok(!(out.x0 != 0 || out.x1 != 3 || out.y0 != 0 || out.y1!= 3),
            "Region For Square returned correct values");
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }


    //Initialize an image with a given value.//
    {
        psMemId id = psMemGetId();
        psImage *in1 = NULL;
        psImage *in2 = NULL;
        psImage *in3 = NULL;
        psImage *in4 = NULL;
        int nRow = 1;
        int nCol = 1;
        in1 = psImageAlloc(nRow, nCol, PS_TYPE_U8);
        ok(psImageInit(in1, 5 ),
             "ImageAlloc.  U8 Case - 1x1");
        nRow = 5;
        in2 = psImageAlloc(nRow, nCol, PS_TYPE_F32);
        ok(psImageInit(in2, 3),
             "ImageAlloc.  F32 Case - 5x1");
    
        nCol = 5;
        in3 = psImageAlloc(nRow, nCol, PS_TYPE_F64);
        ok(psImageInit(in3, 3.14),
             "ImageAlloc.  F64 Case - 5x5");
    
        in4 = psImageAlloc(nRow, nCol, PS_TYPE_S32);
        ok(psImageInit(in4, 3),
             "ImageAlloc.  S32 Case - 5x5");
        psFree(in1);
        psFree(in2);
        psFree(in3);
        psFree(in4);
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }


    // testImageSet()
    {
        psMemId id = psMemGetId();
        psImage *image = NULL;
        image = psImageAlloc(5, 4, PS_TYPE_S32);

        //Set the position of the subimage relative to the parent.
        P_PSIMAGE_SET_COL0(image, 10);
        P_PSIMAGE_SET_ROW0(image, 10);
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 5; j++) {
                image->data.S32[i][j] = i+j;
            }
        }

        //Attempt to set a position in a NULL psImage*
        psImage *none = NULL;
        ok(! psImageSet(none, 1, 1, 1),
            "psImageSet returned non-null");

        //Attempt to set a position in a psImage* with negative numCols, numRows
        none = psImageAlloc(2, 2, PS_TYPE_S32);
        *(int*)&none->numCols = -1;
        ok(! psImageSet(none, 1, 1, 1),
            "psImageSet return false when passed an image with negative numCols");

        *(int*)&none->numCols = 2;
        *(int*)&none->numRows = -1;
        ok(!psImageSet(none, 1, 1, 1),
             "psImageSet failed to return false when passed an image with negative numRows");

        //Attempt to set a position in a psImage* with negative col0, row0
        *(int*)&none->numRows = 2;
        P_PSIMAGE_SET_ROW0(none, -1);
        ok(!psImageSet(none, 1, 1, 1),
             "psImageSet failed to return false when passed an image with negative col0");

        P_PSIMAGE_SET_COL0(none, -1);
        P_PSIMAGE_SET_ROW0(none, 0);
        ok(!psImageSet(none, 1, 1, 1),
             "psImageSet failed to return false when passed an image with negative col0");
        psFree(none);

        //Try to set a position inside of the subimage.
        ok(psImageSet(image, 14, 12, 666),
             "psImageSet failed to return true when passed valid parameters");

        ok(image->data.S32[2][4] == 666,
            "psImageSet set the data value. value=%d", image->data.S32[2][4]);

        //Try to set a position inside of the subimage from the tail.
        ok(psImageSet(image, -1, -1, 666),
             "psImageSet return true when passed valid parameters");

        ok(image->data.S32[3][4] == 666,
            "psImageSet set (from the tail) the data values correctly");

        //Try to set a position outside of the subimage but inside of the parent image.
        ok(!psImageSet(image, 0, 0, 666),
             "psImageSet return false when passed an invalid location");

        ok(!psImageSet(image, 9, 10, 666),
             "psImageSet return false when passed an invalid location");

        //Try to set a position outside of the subimage.
        ok(!psImageSet(image, 15, 14, 666),
             "psImageSet return false when passed an invalid location");

        //Try to set a position outside of the subimage, indexing from the tail.
        ok(!psImageSet(image, -6, -1, 666),
             "psImageSet return false when passed an invalid location");
        psFree(image);
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }


    // testImageGet()
    {
        psMemId id = psMemGetId();
        psImage *image = NULL;
        image = psImageAlloc(5, 3, PS_TYPE_S32);

        //Set the position of the subimage relative to the parent.
        P_PSIMAGE_SET_COL0(image, 10);
        P_PSIMAGE_SET_ROW0(image, 10);
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 5; j++) {
                image->data.S32[i][j] = i+j;
            }
        }
    
        //Attempt to get a position in a NULL psImage*
        psImage *none = NULL;
        ok(isnan( psImageGet(none, 1, 1) ),
             "psImageGet return false when passed a NULL image");
    
        //Attempt to get a position in a psImage* with negative numCols, numRows
        none = psImageAlloc(2, 2, PS_TYPE_S32);
        *(int*)&none->numCols = -1;
        ok(isnan( psImageGet(none, 1, 1) ),
             "psImageGet return false when passed an image with negative numCols");
    
        *(int*)&none->numCols = 2;
        *(int*)&none->numRows = -1;
        ok(isnan( psImageGet(none, 1, 1) ),
             "psImageGet return false when passed an image with negative numRows");
    
        //Attempt to get a position in a psImage* with negative col0, row0
        *(int*)&none->numRows = 2;
        P_PSIMAGE_SET_ROW0(none, -1);
        ok(isnan( psImageGet(none, 1, 1) ),
             "psImageGet return false when passed an image with negative col0");
    
        P_PSIMAGE_SET_COL0(none, -1);
        P_PSIMAGE_SET_ROW0(none, 0);
        ok(isnan( psImageGet(none, 1, 1) ),
             "psImageGet return false when passed an image with negative col0");
    
        psFree(none);
    
    
        //Try to get a position inside of the subimage.
        ok(psImageGet(image, 14, 12) == 6,
             "psImageGet return the correct value");
    
        //Try to get a position inside of the subimage from the tail.
        ok(psImageGet(image, -1, -1) == 6,
             "psImageGet return the correct value");
    
        //Try to get a position outside of the subimage but inside of the parent image.
        ok(isnan( psImageGet(image, 1, 1) ),
             "psImageGet return NAN when passed an invalid location");
    
        ok(isnan( psImageGet(image, 9, 10) ),
             "psImageGet return NAN when passed an invalid location");
    
        //Try to set a position outside of the subimage.
        ok(isnan( psImageGet(image, 15, 14) ),
             "psImageGet return NAN when passed an invalid location");
    
        //Try to set a position outside of the subimage, indexing from the tail.
        ok(isnan( psImageGet(image, -6, -1) ),
             "psImageGet failed to return NAN when passed an invalid location");
        psFree(image);
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }
}
