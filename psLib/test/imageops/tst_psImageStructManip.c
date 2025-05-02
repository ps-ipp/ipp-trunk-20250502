/** @file  tst_psImageExtraction.c
*
*  @brief Contains the tests for psImageExtraction.[ch]
*
*
*  @author Robert DeSonia, MHPCC
*
*  @version $Revision: 1.2 $ $Name: not supported by cvs2svn $
*  @date $Date: 2006-05-05 02:48:34 $
*
*  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
*/
#include<stdlib.h>
#include<string.h>

#include "psTest.h"
#include "pslib_strict.h"
#include "psType.h"

static psS32 testImageSubset(void);
static psS32 testImageCopy(void);
static psS32 testImageTrim(void);


testDescription tests[] = {
                              {testImageSubset,547,"psImageSubset",0,false},
                              {testImageSubset,550,"psImageSubset",0,true},
                              {testImageCopy,551,"psImageCopy",0,false},
                              {testImageTrim, 744, "psImageTrim", 0, false},
                              {NULL}
                          };

psS32 main( psS32 argc, char* argv[] )
{
    return ! runTestSuite( stderr, "psImage", tests, argc, argv );
}

// #547: psImageSubset shall create child image of a specified size from a parent psImage structure
psS32 testImageSubset(void)
{
    psImage preSubsetStruct;
    psImage* original;
    psImage* subset1 = NULL;
    psImage* subset2 = NULL;
    psImage* subset3 = NULL;
    psS32 c = 128;
    psS32 r = 256;
    psRegion region1 = psRegionSet(0,c/2,0,r/2);
    psRegion region2 = psRegionSet(c/4,c/4+c/2,r/4,r/4+r/2);

    original = psImageAlloc(c,r,PS_TYPE_U32);
    for (psS32 row=0;row<r;row++) {
        for (psS32 col=0;col<c;col++) {
            original->data.F32[row][col] = row*1000+col;
        }
    }

    memcpy(&preSubsetStruct,original,sizeof(psImage));

    subset2 = psImageSubset(original,region2);

    subset3 = psImageSubset(original,region1);

    psLogMsg(__func__,PS_LOG_INFO,
             "Verify the returned psImage structure members nrow and ncol are equal to "
             "the input parameter nrow and ncol respectively.");

    if (subset2->numCols != c/2 || subset2->numRows != r/2) {
        psError(PS_ERR_UNKNOWN,true,
                "psImageSubset output size was not proper(%dx%d, should be %dx%d).",
                subset2->numCols, subset2->numRows, c/2,r/2);
        return 1;
    }

    if (subset3->numCols != c/2 || subset3->numRows != r/2) {
        psError(PS_ERR_UNKNOWN,true,
                "psImageSubset output size was not proper(%dx%d, should be %dx%d).",
                subset3->numCols, subset3->numRows, c/2,r/2);
        return 2;
    }

    psLogMsg(__func__,PS_LOG_INFO,
             "Verify the returned psImage structure contains expected values in the "
             "row member, if the input psImage structure image contains known values.");

    for (psS32 row=0;row<r/2;row++) {
        for (psS32 col=0;col<c/2;col++) {
            if (subset2->data.U32[row][col] != original->data.U32[row+r/4][col+c/4]) {
                psError(PS_ERR_UNKNOWN,true,"psImageSubset output #1 was wrong at %dx%d (%d vs %d).",
                        row,col,subset2->data.U32[row][col], original->data.U32[row+r/4][col+c/4]);
                return 3;
            }
            if (subset3->data.U32[row][col] != original->data.U32[row][col]) {
                psError(PS_ERR_UNKNOWN,true,"psImageSubset output #1 was wrong at %dx%d (%d vs %d).",
                        row,col,subset2->data.U32[row][col], original->data.U32[row][col]);
                return 4;
            }
        }
    }

    psLogMsg(__func__,PS_LOG_INFO,
             "Verify the returned psImage structure member type is equal to the input "
             "psImage structure member type.");

    if (subset2->type.type != PS_TYPE_U32) {
        psError(PS_ERR_UNKNOWN,true,"psImageSubset output type was not proper(%d, should be %d).",
                subset2->type.type, PS_TYPE_U32);
        return 6;
    }
    if (subset3->type.type != PS_TYPE_U32) {
        psError(PS_ERR_UNKNOWN,true,"psImageSubset output type was not proper(%d, should be %d).",
                subset3->type.type, PS_TYPE_U32);
        return 7;
    }

    psLogMsg(__func__,PS_LOG_INFO,
             "Verify the returned psImage structure members row0 and col0 are equal to "
             "the input parameters row0 and col0 respectively.");

    if (subset2->col0 != c/4 || subset2->row0 != r/4) {
        psError(PS_ERR_UNKNOWN,true,
                "psImageSubset didn't set col0/row0 for subset2 (%d/%d, should be %d/%d).",
                subset2->col0,subset2->row0,c/4,r/4);
        return 8;
    }
    if (subset3->col0 != 0 || subset3->row0 != 0) {
        psError(PS_ERR_UNKNOWN,true,
                "psImageSubset didn't set col0/row0 for subset3 (%d/%d, should be %d/%d).",
                subset3->col0,subset3->row0,0,0);
        return 9;
    }

    psLogMsg(__func__,PS_LOG_INFO,
             "Verify the returned psImage structure member parent is equal to the "
             "input psImage structure pointer image.");

    if (subset2->parent != original || subset3->parent != original) {
        psError(PS_ERR_UNKNOWN,true,"psImageSubset didn't set parent.");
        return 10;
    }

    psLogMsg(__func__,PS_LOG_INFO,"Verify the returned psImage structure member children is null.");

    if (subset2->children != NULL || subset3->children != NULL) {
        psError(PS_ERR_UNKNOWN,true,"psImageSubset didn't set children to NULL.");
        return 11;
    }

    psLogMsg(__func__,PS_LOG_INFO,
             "Verify the input psImage structure image only has the following members "
             "changed: 1) Nchildren is increased by one. 2) parent contains pointer psImage structure "
             "out at parent[Nchildren-1].");

    if (original->children == NULL || original->children->n != 2) {
        psError(PS_ERR_UNKNOWN,true,
                "psImageSubset didn't increment number of children by one per subset.");
        return 12;
    }
    if (original->children->data[0] != subset2 || original->children->data[1] != subset3) {
        psError(PS_ERR_UNKNOWN,true,"psImageSubset didn't properly store the children pointers.");
        return 13;
    }

    psLogMsg(__func__,PS_LOG_INFO,"Verify the returned psImage structure pointer is null and program "
             "execution doesn't stop, if the input parameter image is null. Also verified the input "
             "psImage structure is not modified.");

    psLogMsg(__func__,PS_LOG_INFO,"An error should follow...");
    subset1 = psImageSubset(NULL,region1);
    if (subset1 != NULL) {
        psError(PS_ERR_UNKNOWN,true,"psImageSubset didn't return NULL when input image was NULL.");
        return 14;
    }

    psLogMsg(__func__,PS_LOG_INFO,"Verify the returned psImage structure pointer is null and program "
             " execution doesn't stop, if the input parameters nrow and/or ncol are zero. Also verify "
             "input psImage structure is not modified.");

    memcpy(&preSubsetStruct,original,sizeof(psImage));
    psLogMsg(__func__,PS_LOG_INFO,"An error should follow...");
    subset1 = psImageSubset(original, psRegionSet(0,c/2,r/2,r/2));
    if (subset1 != NULL) {
        psError(PS_ERR_UNKNOWN,true,"psImageSubset didn't return NULL when numRows=0.");
        return 15;
    }
    psLogMsg(__func__,PS_LOG_INFO,"An error should follow...");
    subset1 = psImageSubset(original,psRegionSet(c/2,c/2,0,r/2));
    if (subset1 != NULL) {
        psError(PS_ERR_UNKNOWN,true,"psImageSubset didn't return NULL when numCols=0.");
        return 16;
    }
    if (memcmp(original,&preSubsetStruct,sizeof(psImage)) != 0) {
        psError(PS_ERR_UNKNOWN,true,
                "psImageSubset changed the original struct though it failed to subset.");
        return 17;
    }


    psLogMsg(__func__,PS_LOG_INFO,
             "Verify the returned psImage structure pointer is null and program "
             "execution doesn't stop, if the input parameters row0 and col0 are not within "
             "the range of values of psImage structure image.");

    psLogMsg(__func__,PS_LOG_INFO,"An error should follow...");
    subset1 = psImageSubset(original, psRegionSet(0,c/2, 0,r*2));
    if (subset1 != NULL) {
        psError(PS_ERR_UNKNOWN,true,
                "psImageSubset didn't return NULL when subset origin was outside of "
                "image (via cols).");
        return 18;
    }
    psLogMsg(__func__,PS_LOG_INFO,"An error should follow...");
    subset1 = psImageSubset(original,psRegionSet(0,c*2,0,r/2));
    if (subset1 != NULL) {
        psError(PS_ERR_UNKNOWN,true,
                "psImageSubset didn't return NULL when subset origin was outside of "
                "image (via rows).");
        return 19;
    }
    psLogMsg(__func__,PS_LOG_INFO,"An error should follow...");
    subset1 = psImageSubset(original, psRegionSet(-1,c/2,0,r/2));
    if (subset1 != NULL) {
        psError(PS_ERR_UNKNOWN,true,
                "psImageSubset didn't return NULL when subset origin was outside of "
                "image (col0=-1).");
        return 20;
    }
    psLogMsg(__func__,PS_LOG_INFO,"An error should follow...");
    subset1 = psImageSubset(original, psRegionSet(0,c/2,-1,r/2));
    if (subset1 != NULL) {
        psError(PS_ERR_UNKNOWN,true,
                "psImageSubset didn't return NULL when subset origin was outside of "
                "image (row0=-1).");
        return 21;
    }

    psLogMsg(__func__,PS_LOG_INFO,
             "Verify the returned psImage structure pointer is null and program "
             "execution doesn't stop if the input parameters nrow, ncol, row0 and col0 "
             "specify a range of data not within the input psImage structure image.  Also "
             "verify the input psImage structure is not modified.");

    psLogMsg(__func__,PS_LOG_INFO,"An error should follow...");
    subset1 = psImageSubset(original,psRegionSet(0,c/2,0,r+1));
    if (subset1 != NULL) {
        psError(PS_ERR_UNKNOWN,true,
                "psImageSubset didn't return NULL when subset was outside of image (via rows).");
        return 22;
    }
    psLogMsg(__func__,PS_LOG_INFO,"An error should follow...");
    subset1 = psImageSubset(original, psRegionSet(0,c+1,0,r/2));
    if (subset1 != NULL) {
        psError(PS_ERR_UNKNOWN,true,
                "psImageSubset didn't return NULL when subset was outside of image (via cols).");
        return 23;
    }
    psLogMsg(__func__,PS_LOG_INFO,"An error should follow...");
    subset1 = psImageSubset(original,psRegionSet(0,c+1,0,r+1));
    if (subset1 != NULL) {
        psError(PS_ERR_UNKNOWN,true,
                "psImageSubset didn't return NULL when subset was outside of image (via row+cols).");
        return 24;
    }

    psLogMsg(__func__, PS_LOG_INFO,
             "psImageFreeChildren shall deallocate any children images of a "
             "psImage structure");

    memcpy(&preSubsetStruct,original,sizeof(psImage));

    psImageFreeChildren(original);

    // Verify the returned psImage structure member Nchildren is set to zero.
    if (original->children != NULL && original->children->n > 0) {
        psError(PS_ERR_UNKNOWN,true,
                "psImageFreeChildren didn't set number of children to zero.");
        return 25;
    }

    //Verify the returned psImage structure members type, nrow, ncol, row0, col0, rows
    // and parent are not modified.
    if (preSubsetStruct.numRows != original->numRows ||
            preSubsetStruct.numCols != original->numCols ||
            preSubsetStruct.row0 != original->row0 ||
            preSubsetStruct.col0 != original->col0) {

        psError(PS_ERR_UNKNOWN,true,
                "psImageFreeChildren modified parent's non-children elements.");
        return 27;
    }

    psFree(original);

    return 0;
}

psS32 testImageCopy(void)
{
    psImage* img = NULL;
    psImage* img2 = NULL;
    psImage* img3 = NULL;
    psImage* img4 = NULL;
    psU32 c = 128;
    psU32 r = 256;

    img = psImageAlloc(c,r,PS_TYPE_F32);
    for (unsigned row=0;row<r;row++) {
        psF32* imgRow = img->data.F32[row];
        for (unsigned col=0;col<c;col++) {
            imgRow[col] = (psF32)(row+col);
        }
    }
    img2 = psImageAlloc(c,r,PS_TYPE_F32);
    for (unsigned row=0;row<r;row++) {
        psF32* img2Row = img2->data.F32[row];
        for (unsigned col=0;col<c;col++) {
            img2Row[col] = 0.0f;
        }
    }

    img3 = psImageCopy(img2,img,PS_TYPE_F32);

    // Verify the returned psImage structure pointer is equal to the input parameter output.
    if (img2 != img3) {
        psError(PS_ERR_UNKNOWN, true,"the image given for recycled wasn't");
        return 1;
    }

    // Verify the returned psImage structure is the same type as the input image structure
    // if the specified output argument is NULL.
    img4 = psImageCopy(NULL,img,PS_TYPE_F32);
    if (img4 == NULL) {
        psError(PS_ERR_UNKNOWN, true,"output image doesn't exist");
        return 4;
    }
    if (img4->type.type != img->type.type) {
        psError(PS_ERR_UNKNOWN, true,"output image is not the same type as input image");
        return 4;
    }
    for (psU32 row=0;row<r;row++) {
        psF32* imgInRow = img->data.F32[row];
        psF32* imgOutRow = img4->data.F32[row];
        for (unsigned col=0;col<c;col++) {
            if( imgInRow[col] != imgOutRow[col] ) {
                psError(PS_ERR_UNKNOWN, true,"Input image not equal to output image at %d,%d!",
                        col, row);
                return 4;
            }
        }
    }

    // Verify the returned psImage structure member are equal to the values in
    // the input psImage structure input.

    #define testImageCopyType(IN,OUT) \
    img = psImageRecycle(img,c,r,PS_TYPE_##IN); \
    for (unsigned row=0;row<r;row++) { \
        ps##IN* imgRow = img->data.IN[row]; \
        for (unsigned col=0;col<c;col++) { \
            imgRow[col] = (ps##IN)(row+col); \
        } \
    } \
    img2 = psImageCopy(img2,img,PS_TYPE_##OUT); \
    if (img2 == NULL) { \
        psError(PS_ERR_UNKNOWN, true,"psImageCopy failed to copy U8."); \
        return 2; \
    } \
    for (psU32 row=0;row<r;row++) { \
        ps##IN* imgRow = img->data.IN[row]; \
        ps##OUT* img2Row = img2->data.OUT[row]; \
        for (psU32 col=0;col<c;col++) { \
            if (abs(imgRow[col] - (ps##IN)(row+col)) > 0.5) { \
                psError(PS_ERR_UNKNOWN, true,"Input image was changed at %d,%d!", \
                        col,row); \
                return 2; \
            } \
            if (abs(img2Row[col] - (ps##OUT)(imgRow[col])) > 0.5) { \
                psError(PS_ERR_UNKNOWN, true, \
                        "returned psImage values after copy don't match at %d,%d " \
                        "(%d vs %d)",\
                        col,row,img2Row[col], (ps##OUT)(imgRow[col])); \
                return 2; \
            } \
        } \
    }

    #define testImageCopyTypes(IN) \
    printf("to psF32\n"); \
    testImageCopyType(IN,F32);\
    printf("to psF64\n"); \
    testImageCopyType(IN,F64); \
    printf("to psU8\n"); \
    testImageCopyType(IN,U8); \
    printf("to psU16\n"); \
    testImageCopyType(IN,U16); \
    printf("to psU32\n"); \
    testImageCopyType(IN,U32); \
    printf("to psS8\n"); \
    testImageCopyType(IN,S8);\
    printf("to psS16\n"); \
    testImageCopyType(IN,S16);\
    printf("to psS32\n"); \
    testImageCopyType(IN,S32);

    psLogMsg(__func__,PS_LOG_INFO,"Image Copy Test for psU8");
    testImageCopyTypes(U8);
    psLogMsg(__func__,PS_LOG_INFO,"Image Copy Test for psU16");
    testImageCopyTypes(U16);
    psLogMsg(__func__,PS_LOG_INFO,"Image Copy Test for psU32");
    testImageCopyTypes(U32);
    psLogMsg(__func__,PS_LOG_INFO,"Image Copy Test for psS8");
    testImageCopyTypes(S8);
    psLogMsg(__func__,PS_LOG_INFO,"Image Copy Test for psS16");
    testImageCopyTypes(S16);
    psLogMsg(__func__,PS_LOG_INFO,"Image Copy Test for psS32");
    testImageCopyTypes(S32);
    psLogMsg(__func__,PS_LOG_INFO,"Image Copy Test for psF32");
    testImageCopyTypes(F32);
    psLogMsg(__func__,PS_LOG_INFO,"Image Copy Test for psF64");
    testImageCopyTypes(F64);

    // Verify the returned psImage structure pointer is null and program
    // execution doesn't stop, if the input parameter input is null.
    psLogMsg(__func__,PS_LOG_INFO,"An error should follow...");
    img3 = psImageCopy(NULL,NULL,PS_TYPE_F32);
    if (img3 != NULL) {
        psError(PS_ERR_UNKNOWN, true,"psImageCopy didn't return NULL when input image was NULL.");
        return 3;
    }

    psFree(img);
    psFree(img2);
    psFree(img3);
    psFree(img4);

    return 0;
}

static psS32 testImageTrim(void)
{
    psS32 r = 200;
    psS32 c = 300;
    psS32 qtrR = r/4;
    psS32 qtrC = c/4;
    psS32 halfR = r/2;
    psS32 halfC = c/2;
    psRegion centerHalf = {qtrC,qtrC+halfC,qtrR,qtrR+halfR};

    psImage* image = psImageAlloc(c,r,PS_TYPE_F32);
    for (psS32 row = 0; row < image->numRows; row++) {
        for (psS32 col = 0; col < image->numCols; col++) {
            image->data.F32[row][col] = (psF32)col + (psF32)row/1000.0f;
        }
    }

    /*
        1. invoke psImageTrim with non-NULL image, and a valid region
           x0,y0->x1,y1 (using only positive values). Verify that:
            a. the return psImage is the same as the input psImage.
            b. the size of the psImage is x1-x0 by y1-y0.
            c. the pixel values coorespond to the region [x0:x1-1,y0:y1-1].
    */
    psImage* image1 = psImageCopy(NULL,image,PS_TYPE_F32);
    psImage* image2 = psImageTrim(image1,centerHalf);

    if (image1 != image2) {
        psLogMsg(__func__,PS_LOG_ERROR,
                 "Return value not same as input value.  Not done in-place?");
        return 1;
    }

    if (image2->numCols != halfC ||
            image2->numRows != halfR) {
        psLogMsg(__func__,PS_LOG_ERROR,
                 "The resulting image size was %dx%d, but should be %dx%d.",
                 image2->numCols, image2->numRows,
                 halfC, halfR);
        return 2;
    }

    for (psS32 row = 0; row < image2->numRows; row++) {
        for (psS32 col = 0; col < image2->numCols; col++) {
            if (fabsf(image2->data.F32[row][col] - image->data.F32[row+qtrR][col+qtrC])
                    > FLT_EPSILON) {
                psLogMsg(__func__,PS_LOG_ERROR,
                         "The value at (%d,%d) was %g, but should be %g.",
                         col,row,
                         image2->data.F32[row][col],
                         image->data.F32[row+qtrR][col+qtrC]);
                return 3;
            }
        }
    }

    /*
        2. invoke psImageTrim with non-NULL image and valid region where x1=0,
           y1=0. Verify that:
            a. the return psImage size is numCols-x0 by numRows-y0
            b. the pixel values coorespond to the region
               [x0:numCols-1,y0:numRows-1].
    */
    image1 = psImageCopy(image1,image,PS_TYPE_F32);
    image2 = psImageTrim(image1,psRegionSet(qtrC,0,qtrR,0));

    if (image1 != image2) {
        psLogMsg(__func__,PS_LOG_ERROR,
                 "Return value not same as input value.  Not done in-place?");
        return 11;
    }

    if (image2->numCols != image->numCols-qtrC ||
            image2->numRows != image->numRows-qtrR) {
        psLogMsg(__func__,PS_LOG_ERROR,
                 "The resulting image size was %dx%d, but should be %dx%d.",
                 image2->numCols, image2->numRows,
                 image->numCols-qtrC, image->numRows-qtrR);
        return 12;
    }

    for (psS32 row = 0; row < image2->numRows; row++) {
        for (psS32 col = 0; col < image2->numCols; col++) {
            if (fabsf(image2->data.F32[row][col] -
                      image->data.F32[row+qtrR][col+qtrC]) > FLT_EPSILON) {
                psLogMsg(__func__,PS_LOG_ERROR,
                         "The value at (%d,%d) was %g, but should be %g.",
                         col,row,
                         image2->data.F32[row][col],
                         image->data.F32[row+qtrR][col+qtrC]);
                return 13;
            }
        }
    }

    /*
        4. invoke psImageTrim with x1<0, y1<0. Verify:
            a. the psImage size is (numCols+x1)-x0 by (numRows+y1)-y0.
            b. the pixel values coorespond to the region
               [x0:numCols+x1-1,y0:numRows+y1-1].

    */
    image1 = psImageCopy(image1,image,PS_TYPE_F32);
    image2 = psImageTrim(image1,psRegionSet(qtrC,-qtrC, qtrR,-qtrR));

    if (image1 != image2) {
        psLogMsg(__func__,PS_LOG_ERROR,
                 "Return value not same as input value.  Not done in-place?");
        return 21;
    }

    if (image2->numCols != image->numCols-qtrC-qtrC ||
            image2->numRows != image->numRows-qtrR-qtrR) {
        psLogMsg(__func__,PS_LOG_ERROR,
                 "The resulting image size was %dx%d, but should be %dx%d.",
                 image2->numCols, image2->numRows,
                 image->numCols-qtrC, image->numRows-qtrR);
        return 22;
    }

    for (psS32 row = 0; row < image2->numRows; row++) {
        for (psS32 col = 0; col < image2->numCols; col++) {
            if (fabsf(image2->data.F32[row][col] -
                      image->data.F32[row+qtrR][col+qtrC]) > FLT_EPSILON) {
                psLogMsg(__func__,PS_LOG_ERROR,
                         "The value at (%d,%d) was %g, but should be %g.",
                         col,row,
                         image2->data.F32[row][col],
                         image->data.F32[row+qtrR][col+qtrC]);
                return 23;
            }
        }
    }

    psFree(image2);

    /*
        6. invoke psImageTrim with image=NULL Verify:
            a. execution does not cease.
            b. return value is NULL
            c. appropriate error is generated.
    */

    image2 = psImageTrim(NULL, psRegionSet(qtrC,0,qtrR,0));

    if (image2 != NULL) {
        psLogMsg(__func__,PS_LOG_ERROR,
                 "psImageTrim did not return NULL given a NULL input image.");
        return 31;
    }
    psErr* err = psErrorLast();
    if (err == NULL || err->code != PS_ERR_BAD_PARAMETER_NULL) {
        psLogMsg(__func__,PS_LOG_ERROR,
                 "psImageTrim did not generate an appropriate error for NULL input image.");
        psErrorStackPrint(stderr,"Error Stack:");
        return 32;
    }
    psFree(err);

    image1 = psImageCopy(NULL,image,PS_TYPE_F32);
    image2 = psImageTrim(image1, psRegionSet(-1,0,0,0));

    if (image2 != NULL) {
        psLogMsg(__func__,PS_LOG_ERROR,
                 "psImageTrim did not return NULL given x0=-1.");
        return 33;
    }
    err = psErrorLast();
    if (err == NULL || err->code != PS_ERR_BAD_PARAMETER_VALUE) {
        psLogMsg(__func__,PS_LOG_ERROR,
                 "psImageTrim did not generate an appropriate error for x0=-1.");
        psErrorStackPrint(stderr,"Error Stack:");
        return 34;
    }
    psFree(err);

    image1 = psImageCopy(NULL,image,PS_TYPE_F32);
    image2 = psImageTrim(image1, psRegionSet(0,0,-1,0));

    if (image2 != NULL) {
        psLogMsg(__func__,PS_LOG_ERROR,
                 "psImageTrim did not return NULL given y0=-1.");
        return 35;
    }
    err = psErrorLast();
    if (err == NULL || err->code != PS_ERR_BAD_PARAMETER_VALUE) {
        psLogMsg(__func__,PS_LOG_ERROR,
                 "psImageTrim did not generate an appropriate error for y0=-1.");
        psErrorStackPrint(stderr,"Error Stack:");
        return 36;
    }
    psFree(err);

    image1 = psImageCopy(NULL,image,PS_TYPE_F32);
    image2 = psImageTrim(image1, psRegionSet(0,image->numCols+1,0,0));

    if (image2 != NULL) {
        psLogMsg(__func__,PS_LOG_ERROR,
                 "psImageTrim did not return NULL given x1=numCols+1.");
        return 37;
    }
    err = psErrorLast();
    if (err == NULL || err->code != PS_ERR_BAD_PARAMETER_VALUE) {
        psLogMsg(__func__,PS_LOG_ERROR,
                 "psImageTrim did not generate an appropriate error for x1=numCols+1.");
        psErrorStackPrint(stderr,"Error Stack:");
        return 38;
    }
    psFree(err);

    image1 = psImageCopy(NULL,image,PS_TYPE_F32);
    image2 = psImageTrim(image1, psRegionSet(0,0,0,image->numRows+1));

    if (image2 != NULL) {
        psLogMsg(__func__,PS_LOG_ERROR,
                 "psImageTrim did not return NULL given y1=numRows+1.");
        return 39;
    }
    err = psErrorLast();
    if (err == NULL || err->code != PS_ERR_BAD_PARAMETER_VALUE) {
        psLogMsg(__func__,PS_LOG_ERROR,
                 "psImageTrim did not generate an appropriate error for y1=numRows+1.");
        psErrorStackPrint(stderr,"Error Stack:");
        return 40;
    }
    psFree(err);

    image1 = psImageCopy(NULL,image,PS_TYPE_F32);
    image2 = psImageTrim(image1, psRegionSet(0,(((psF32)image->numCols)*-1.0),0,0));

    if (image2 != NULL) {
        psLogMsg(__func__,PS_LOG_ERROR,
                 "psImageTrim did not return NULL given x1=-numCols.");
        return 41;
    }
    err = psErrorLast();
    if (err == NULL || err->code != PS_ERR_BAD_PARAMETER_VALUE) {
        psLogMsg(__func__,PS_LOG_ERROR,
                 "psImageTrim did not generate an appropriate error for x1=-numCols.");
        psErrorStackPrint(stderr,"Error Stack:");
        return 42;
    }
    psFree(err);

    image1 = psImageCopy(NULL,image,PS_TYPE_F32);
    image2 = psImageTrim(image1, psRegionSet(0,0,0,(((psF32)image->numRows)*-1.0)));

    if (image2 != NULL) {
        psLogMsg(__func__,PS_LOG_ERROR,
                 "psImageTrim did not return NULL given y1=-numRows.");
        return 41;
    }
    err = psErrorLast();
    if (err == NULL || err->code != PS_ERR_BAD_PARAMETER_VALUE) {
        psLogMsg(__func__,PS_LOG_ERROR,
                 "psImageTrim did not generate an appropriate error for y1=-numRows.");
        psErrorStackPrint(stderr,"Error Stack:");
        return 42;
    }
    psFree(err);

    psFree(image);
    return 0;
}
