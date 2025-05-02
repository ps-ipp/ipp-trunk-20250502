/** @file  tap_psImageStructManip.c
*
*  @brief Contains the tests for psImageExtraction.[ch]
*
*  @author Robert DeSonia, MHPCC
*
*  psLib functions tested:
*     psImageSubset()
*     psImageCopy()
*     psImageTrim()
*
*  @version $Revision: 1.5 $ $Name: not supported by cvs2svn $
*  @date $Date: 2007-09-20 23:56:10 $
*
*  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
*/
#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include "tap.h"
#include "pstap.h"


void genericImageSubsetTest(int numRows, int numCols)
{
    // psImageSubset shall create child image of a specified size from a
    // parent psImage structure
    // psImageSubset()
    {
        psMemId id = psMemGetId();
        psImage preSubsetStruct;
        psRegion region1 = psRegionSet(0, numCols/2, 0, numRows/2);
        psRegion region2 = psRegionSet(numCols/4, numCols/4+numCols/2, numRows/4, numRows/4+numRows/2);

	// test basic subset creation 
	{ 
	    psImage* original = psImageAlloc(numCols,numRows,PS_TYPE_U32);
	    for (psS32 row=0;row<numRows;row++) {
		for (psS32 col=0;col<numCols;col++) {
		    original->data.F32[row][col] = row*1000+col;
		}
	    }

	    // XXX this is not being used in this section
	    memcpy(&preSubsetStruct, original, sizeof(psImage));
	    psImage* subset1 = psImageSubset(original, region2);
	    ok(subset1, "psImageSubset() returned non-NULL (subset1)");
	    skip_start(subset1 == NULL, 25, "Skipping tests because psImageSubset() returned NULL");
	    psImage* subset2 = psImageSubset(original, region1);
	    ok(subset2, "psImageSubset() returned non-NULL (subset2)");
	    skip_start(subset2 == NULL, 24, "Skipping tests because psImageSubset() returned NULL");

	    // Verify the returned psImage structure members nrow and ncol are equal to
	    // the input parameter nrow and ncol respectively
	    ok(subset1->numCols == numCols/2 && subset1->numRows == numRows/2,
	       "psImageSubset output size set properly");
	    ok(subset2->numCols == numCols/2 && subset2->numRows == numRows/2,
	       "psImageSubset output size set properly");

	    // Verify the returned psImage structure contains expected values in the
	    // row member, if the input psImage structure image contains known values
	    bool errorFlag = false;
	    for (psS32 row=0;row<numRows/2;row++) {
		for (psS32 col=0;col<numCols/2;col++) {
		    if (subset1->data.U32[row][col] != original->data.U32[row+numRows/4][col+numCols/4]) {
			diag("psImageSubset output #1 was wrong at %dx%d (%d vs %d).",
			     row,col,subset1->data.U32[row][col], original->data.U32[row+numRows/4][col+numCols/4]);
			errorFlag = true;
		    }
		    if (subset2->data.U32[row][col] != original->data.U32[row][col]) {
			diag("psImageSubset output #1 was wrong at %dx%d (%d vs %d).",
			     row,col,subset1->data.U32[row][col], original->data.U32[row][col]);
			errorFlag = true;
		    }
		}
	    }
	    ok(!errorFlag, "psImageSubset() produced the expected values");

	    // Verify the returned psImage structure member type is equal to the input
	    // psImage structure member type
	    ok(subset1->type.type == PS_TYPE_U32, "psImageSubset() type was correct (subset1)");
	    ok(subset2->type.type == PS_TYPE_U32, "psImageSubset() type was correct (subset2)");

	    // Verify the returned psImage structure members row0 and col0 are equal to
	    // the input parameters row0 and col0 respectively
	    ok(subset1->col0 == numCols/4 && subset1->row0 == numRows/4,
	       "psImageSubset() set col0/row0 correctly (subset1)");
	    ok(subset2->col0 == 0 && subset2->row0 == 0,
	       "psImageSubset() set col0/row0 correctly (subset2)");

	    // Verify the returned psImage structure member parent is equal to the
	    // input psImage structure pointer image
	    ok(subset1->parent == original && subset2->parent == original, "psImageSubset() set ->parent correctly");

	    // Verify the returned psImage structure member children is null
	    ok(subset1->children == NULL && subset2->children == NULL, "psImageSubset() set ->children correctly");

	    // Verify the input psImage structure image only has the following members
	    // changed: 1) Nchildren is increased by one. 2) parent contains pointer psImage structure
	    // out at parent[Nchildren-1]
	    ok(original->children != NULL && original->children->n == 2, "psImageSubset did increment number of children by one per subset.");
	    ok(original->children->data[0] == subset1 && original->children->data[1] == subset2,
	       "psImageSubset did properly store the children pointers.");

	    psFree (subset1);
	    psFree (subset2);
	    skip_end();
	    skip_end();
	    psFree (original);
	}

	// test free in reverse order from above
	{ 
	    psImage* original = psImageAlloc(numCols,numRows,PS_TYPE_U32);
	    for (psS32 row=0;row<numRows;row++) {
		for (psS32 col=0;col<numCols;col++) {
		    original->data.F32[row][col] = row*1000+col;
		}
	    }

	    // XXX this is not being used in this section
	    memcpy(&preSubsetStruct, original, sizeof(psImage));
	    psImage* subset1 = psImageSubset(original, region2);
	    ok(subset1, "psImageSubset() returned non-NULL (subset1)");
	    psImage* subset2 = psImageSubset(original, region1);
	    ok(subset2, "psImageSubset() returned non-NULL (subset2)");

	    psFree (original);
	    psFree (subset1);
	    psFree (subset2);
	}

	{
	    psImage* original = psImageAlloc(numCols,numRows,PS_TYPE_U32);
	    for (psS32 row=0;row<numRows;row++) {
		for (psS32 col=0;col<numCols;col++) {
		    original->data.F32[row][col] = row*1000+col;
		}
	    }

	    // XXX this is not being used in this section
	    memcpy(&preSubsetStruct, original, sizeof(psImage));

	    // Verify the returned psImage structure pointer is null and program
	    // execution doesn't stop, if the input parameter image is null.
	    // Also verified the input psImage structure is not modified
	    // An error should follow...
	    // XXX: Verify error
	    psImage* subset1 = psImageSubset(NULL,region1);
	    ok(subset1 == NULL, "psImageSubset returned NULL when input image was NULL.");

	    // Verify the returned psImage structure pointer is null and program
	    // execution doesn't stop, if the input parameters nrow and/or ncol are zero.
	    // Also verify input psImage structure is not modified
	    // An error should follow...
	    // XXX: Verify error
	    {
		memcpy(&preSubsetStruct,original,sizeof(psImage));
		subset1 = psImageSubset(original, psRegionSet(0,numCols/2,numRows/2,numRows/2));
		ok(subset1 == NULL, "psImageSubset returned NULL when numRows=0.");
		// An error should follow...
		// XXX: Verify error
		subset1 = psImageSubset(original,psRegionSet(numCols/2,numCols/2,0,numRows/2));
		ok(subset1 == NULL, "psImageSubset returned NULL when numCols=0.");
		ok(memcmp(original,&preSubsetStruct,sizeof(psImage)) == 0,
		   "psImageSubset didn't change the original struct though it failed to subset.");
	    }

	    // Verify the returned psImage structure pointer is null and program
	    // execution doesn't stop, if the input parameters row0 and col0 are not within
	    // the range of values of psImage structure image
	    // An error should follow...
	    // XXX: Verify error
	    {
		subset1 = psImageSubset(original, psRegionSet(0,numCols/2, 0,numRows*2));
		ok(subset1 == NULL,
		   "psImageSubset returned NULL when subset origin was outside of image (via cols)");
		// An error should follow...
		// XXX: Verify error
		subset1 = psImageSubset(original,psRegionSet(0,numCols*2,0,numRows/2));
		ok(subset1 == NULL,
		   "psImageSubset returned NULL when subset origin was outside of image (via rows)");
		// An error should follow...
		// XXX: Verify error
		subset1 = psImageSubset(original, psRegionSet(-1,numCols/2,0,numRows/2));
		ok(subset1 == NULL,
		   "psImageSubset returned NULL when subset origin was outside of image (col0=-1)");
		// An error should follow...
		// XXX: Verify error
		subset1 = psImageSubset(original, psRegionSet(0,numCols/2,-1,numRows/2));
		ok(subset1 == NULL,
		   "psImageSubset returned NULL when subset origin was outside of image (row0=-1)");
	    }    

	    // Verify the returned psImage structure pointer is null and program
	    // execution doesn't stop if the input parameters nrow, ncol, row0 and col0
	    // specify a range of data not within the input psImage structure image.  Also
	    // verify the input psImage structure is not modified
	    // An error should follow...
	    // XXX: Verify error
	    {
		subset1 = psImageSubset(original,psRegionSet(0,numCols/2,0,numRows+1));
		ok(subset1 == NULL,
		   "psImageSubset returned NULL when subset was outside of image (via rows)");
		// An error should follow...
		// XXX: Verify error
		subset1 = psImageSubset(original, psRegionSet(0,numCols+1,0,numRows/2));
		ok(subset1 == NULL,
		   "psImageSubset returned NULL when subset was outside of image (via cols)");
		// An error should follow...
		// XXX: Verify error
		subset1 = psImageSubset(original,psRegionSet(0,numCols+1,0,numRows+1));
		ok(subset1 == NULL,
		   "psImageSubset returned NULL when subset was outside of image (via row+cols)");
	    }
	    psFree(original);
	}
        ok(!psMemCheckLeaks (id, NULL, stderr, false), "no memory leaks");
    }
}

psS32 main(psS32 argc, char* argv[])
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    plan_tests(239);

    genericImageSubsetTest(128, 256);
    genericImageSubsetTest(256, 128);
    genericImageSubsetTest(128, 128);

    // psImageCopy()
    {
        psMemId id = psMemGetId();
        psU32 c = 128;
        psU32 r = 256;

        psImage *img = psImageAlloc(c,r,PS_TYPE_F32);
        for (unsigned row=0;row<r;row++) {
            for (unsigned col=0;col<c;col++) {
                img->data.F32[row][col] = (psF32)(row+col);
            }
        }
        psImage *img2 = psImageAlloc(c,r,PS_TYPE_F32);
        for (unsigned row=0;row<r;row++) {
            for (unsigned col=0;col<c;col++) {
                img2->data.F32[row][col] = 0.0f;
            }
        }

        psImage *img3 = psImageCopy(img2,img,PS_TYPE_F32);
        // Verify the returned psImage structure pointer is equal to the input
        // parameter output.
        ok(img2 == img3, "psImageCopy(): recycled input image");

        // Verify the returned psImage structure is the same type as the input image
        // structure if the specified output argument is NULL.
        psImage *img4 = psImageCopy(NULL,img,PS_TYPE_F32);
        ok(img4 != NULL, "psImageCopy() returned non-NULL with NULL output argument");
        ok(img4->type.type == img->type.type, "psImageCopy() set the correct image type");
        bool errorFlag = false;
        for (psU32 row=0;row<r;row++) {
            psF32* imgInRow = img->data.F32[row];
            psF32* imgOutRow = img4->data.F32[row];
            for (unsigned col=0;col<c;col++) {
                if(imgInRow[col] != imgOutRow[col]) {
                    diag("Input image not equal to output image at %d,%d", col, row);
                    errorFlag = true;
                }
            }
        }
        ok(!errorFlag, "psImageCopy() set data correctly");


        // Verify the returned psImage structure member are equal to the values in
        // the input psImage structure input.
        #define testImageCopyType(IN,OUT) \
        { \
            bool errorFlag = false; \
            img = psImageRecycle(img,c,r,PS_TYPE_##IN); \
            for (unsigned row=0;row<r;row++) { \
                ps##IN* imgRow = img->data.IN[row]; \
                for (unsigned col=0;col<c;col++) { \
                    imgRow[col] = (ps##IN)(row+col); \
                } \
            } \
            img2 = psImageCopy(img2,img,PS_TYPE_##OUT); \
            if(img2 != NULL) { \
                ok(img2, "psImageCopy() returned non-NULL"); \
                for (psU32 row=0;row<r;row++) { \
                    ps##IN* imgRow = img->data.IN[row]; \
                    ps##OUT* img2Row = img2->data.OUT[row]; \
                    for (psU32 col=0;col<c;col++) { \
                        if (abs(imgRow[col] - (ps##IN)(row+col)) > 0.5) { \
                            diag("Input image was changed at %d,%d", col,row); \
                            errorFlag = true; \
                        } \
                        if (abs(img2Row[col] - (ps##OUT)(imgRow[col])) > 0.5) { \
                            diag("returned psImage values after copy don't match at %d,%d " \
                                 "(%d vs %d)",\
                                 col,row,img2Row[col], (ps##OUT)(imgRow[col])); \
                            errorFlag = true; \
                        } \
                    } \
                } \
                ok(!errorFlag, "psImageCopy() set image data correctly"); \
            } else {\
                ok(img2, "psImageCopy() returned non-NULL"); \
            } \
        }

        #define testImageCopyTypes(IN) \
        testImageCopyType(IN,F32);\
        testImageCopyType(IN,F64); \
        testImageCopyType(IN,U8); \
        testImageCopyType(IN,U16); \
        testImageCopyType(IN,U32); \
        testImageCopyType(IN,S8);\
        testImageCopyType(IN,S16);\
        testImageCopyType(IN,S32);

        testImageCopyTypes(U8);
        testImageCopyTypes(U16);
        testImageCopyTypes(U32);
        testImageCopyTypes(S8);
        testImageCopyTypes(S16);
        testImageCopyTypes(S32);
        testImageCopyTypes(F32);
        testImageCopyTypes(F64);

        // Verify the returned psImage structure pointer is null and program
        // execution doesn't stop, if the input parameter input is null.
        // An error should follow...
        // XXX: Verify error
        img3 = psImageCopy(NULL,NULL,PS_TYPE_F32);
        ok(img3 == NULL, "psImageCopy() returned NULL when input image was NULL");

        psFree(img);
        psFree(img2);
        psFree(img3);
        psFree(img4);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // psImageTrim()
    // XXX: This should probably be broken into separate blocks
    {
        psMemId id = psMemGetId();
        psS32 r = 200;
        psS32 c = 300;
        psS32 qtrR = r/4;
        psS32 qtrC = c/4;
        psS32 halfR = r/2;
        psS32 halfC = c/2;
        psRegion centerHalf = {qtrC, qtrC+halfC, qtrR, qtrR+halfR};

        psImage* image = psImageAlloc(c,r,PS_TYPE_F32);
        for (psS32 row = 0; row < image->numRows; row++) {
            for (psS32 col = 0; col < image->numCols; col++) {
                image->data.F32[row][col] = (psF32)col + (psF32)row/1000.0f;
            }
        }
        // invoke psImageTrim with non-NULL image, and a valid region
        // x0,y0->x1,y1 (using only positive values). Verify that:
        // a. the return psImage is the same as the input psImage.
        // b. the size of the psImage is x1-x0 by y1-y0.
        // c. the pixel values coorespond to the region [x0:x1-1,y0:y1-1].
        psImage* image1 = psImageCopy(NULL,image,PS_TYPE_F32);
        psImage* image2 = psImageTrim(image1,centerHalf);
        ok(image1 == image2, "psImageTrim() Return value same as input value");
        ok(image2->numCols == halfC && image2->numRows == halfR,
           "The resulting image size was %dx%d, should be %dx%d.",
           image2->numCols, image2->numRows,
           halfC, halfR);

        bool errorFlag = false;
        for (psS32 row = 0; row < image2->numRows; row++) {
            for (psS32 col = 0; col < image2->numCols; col++) {
                if (fabsf(image2->data.F32[row][col] - image->data.F32[row+qtrR][col+qtrC])
                        > FLT_EPSILON) {
                    diag("The value at (%d,%d) was %g, but should be %g.",
                         col,row,
                         image2->data.F32[row][col],
                         image->data.F32[row+qtrR][col+qtrC]);
                    errorFlag = true;
                }
            }
        }
        ok(!errorFlag, "psImageTrim() set image data correctly");


        // 2. invoke psImageTrim with non-NULL image and valid region where x1=0,
        //    y1=0. Verify that:
        //    a. the return psImage size is numCols-x0 by numRows-y0
        //    b. the pixel values coorespond to the region
        //       [x0:numCols-1,y0:numRows-1].
        image1 = psImageCopy(image1,image,PS_TYPE_F32);
        image2 = psImageTrim(image1,psRegionSet(qtrC,0,qtrR,0));
        ok(image1 == image2, "psImageTrim() Return value same as input value");
        ok(image2->numCols == image->numCols-qtrC && image2->numRows == image->numRows-qtrR,
           "The resulting image size was %dx%d, it should be %dx%d.",
           image2->numCols, image2->numRows,
           image->numCols-qtrC, image->numRows-qtrR);

        for (psS32 row = 0; row < image2->numRows; row++) {
            for (psS32 col = 0; col < image2->numCols; col++) {
                if (fabsf(image2->data.F32[row][col] -
                          image->data.F32[row+qtrR][col+qtrC]) > FLT_EPSILON) {
                    diag("The value at (%d,%d) was %g, but should be %g.",
                         col,row,
                         image2->data.F32[row][col],
                         image->data.F32[row+qtrR][col+qtrC]);
                    errorFlag = true;
                }
            }
        }
        ok(!errorFlag, "psImageTrim() set image data correctly");


        //  Invoke psImageTrim with x1<0, y1<0. Verify:
        //      a. the psImage size is (numCols+x1)-x0 by (numRows+y1)-y0.
        //      b. the pixel values coorespond to the region
        //         [x0:numCols+x1-1,y0:numRows+y1-1].
        image1 = psImageCopy(image1,image,PS_TYPE_F32);
        image2 = psImageTrim(image1,psRegionSet(qtrC,-qtrC, qtrR,-qtrR));
        ok(image1 == image2, "psImageTrim() Return value same as input value");
        ok(image2->numCols == image->numCols-qtrC-qtrC && image2->numRows == image->numRows-qtrR-qtrR,
           "The resulting image size was %dx%d, but should be %dx%d.",
           image2->numCols, image2->numRows,
           image->numCols-qtrC, image->numRows-qtrR);
        for (psS32 row = 0; row < image2->numRows; row++) {
            for (psS32 col = 0; col < image2->numCols; col++) {
                if (fabsf(image2->data.F32[row][col] -
                          image->data.F32[row+qtrR][col+qtrC]) > FLT_EPSILON) {
                    diag("The value at (%d,%d) was %g, but should be %g.",
                         col,row,
                         image2->data.F32[row][col],
                         image->data.F32[row+qtrR][col+qtrC]);
                    errorFlag = true;
                }
            }
        }
        ok(!errorFlag, "psImageTrim() set image data correctly");
        psFree(image1);


        //  Invoke psImageTrim with image=NULL Verify:
        //      a. execution does not cease.
        //      b. return value is NULL
        //      c. appropriate error is generated.
        // An error should follow...
        // XXX: Verify errors
        image2 = psImageTrim(NULL, psRegionSet(qtrC,0,qtrR,0));
        ok(image2 == NULL, "psImageTrim returned NULL given a NULL input image");
        psErr* err = psErrorLast();
        ok(err != NULL && err->code == PS_ERR_BAD_PARAMETER_NULL,
           "psImageTrim did generate an appropriate error for NULL input image.");
        psFree(err);


        // Verify when psRegion has a coord at -1
        image1 = psImageCopy(NULL,image,PS_TYPE_F32);
        image2 = psImageTrim(image1, psRegionSet(-1,0,0,0));
        ok(image2 == NULL, "psImageTrim returned NULL given x0=-1.");
        err = psErrorLast();
        ok(err != NULL && err->code == PS_ERR_BAD_PARAMETER_VALUE,
           "psImageTrim did generate an appropriate error for x0=-1.");
        psFree(err);


        // Verify when psRegion has a coord at -1
        image1 = psImageCopy(NULL,image,PS_TYPE_F32);
        image2 = psImageTrim(image1, psRegionSet(0,0,-1,0));
        ok(image2 == NULL, "psImageTrim returned NULL given y0=-1.");
        err = psErrorLast();
        ok(err != NULL && err->code == PS_ERR_BAD_PARAMETER_VALUE,
           "psImageTrim did generate an appropriate error for y0=-1");
        psFree(err);


        // Verify when psRegion has a coord at outside the image range
        image1 = psImageCopy(NULL,image,PS_TYPE_F32);
        image2 = psImageTrim(image1, psRegionSet(0,image->numCols+1,0,0));
        ok(image2 == NULL, "psImageTrim returned NULL given x1=numCols+1");
        err = psErrorLast();
        ok(err != NULL && err->code == PS_ERR_BAD_PARAMETER_VALUE,
           "psImageTrim did generate an appropriate error for x1=numCols+1");
        psFree(err);


        // Verify when psRegion has a coord at outside the image range
        image1 = psImageCopy(NULL,image,PS_TYPE_F32);
        image2 = psImageTrim(image1, psRegionSet(0,0,0,image->numRows+1));
        ok(image2 == NULL, "psImageTrim returned NULL given y1=numRows+1");
        err = psErrorLast();
        ok(err != NULL && err->code == PS_ERR_BAD_PARAMETER_VALUE,
           "psImageTrim did generate an appropriate error for y1=numRows+1");
        psFree(err);


        // Verify when psRegion has a coord at outside the image range
        image1 = psImageCopy(NULL,image,PS_TYPE_F32);
        image2 = psImageTrim(image1, psRegionSet(0,0,0,(((psF32)image->numRows)*-1.0)));
        ok(image2 == NULL, "psImageTrim returned NULL given y1=-numRows");
        err = psErrorLast();
        ok(err != NULL && err->code == PS_ERR_BAD_PARAMETER_VALUE,
           "psImageTrim did generate an appropriate error for y1=-numRows");
        psFree(err);


        // Verify when psRegion has a coord at outside the image range
        image1 = psImageCopy(NULL,image,PS_TYPE_F32);
        image2 = psImageTrim(image1, psRegionSet(0,(((psF32)image->numCols)*-1.0),0,0));
        ok(image2 == NULL, "psImageTrim returned NULL given x1=-numCols");
        err = psErrorLast();
        ok(err != NULL && err->code == PS_ERR_BAD_PARAMETER_VALUE,
           "psImageTrim did generate an appropriate error for x1=-numCols");


        psFree(image);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
}
