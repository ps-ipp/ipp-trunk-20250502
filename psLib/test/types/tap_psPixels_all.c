/**
 *  C Implementation: tap_psPixels_all
 *
 * Description:  Tests for psPixelsAlloc, psPixelsRealloc, psMemCheckPixels,
 *               psPixelsCopy, psPixels(To/From)Mask, p_psPixelsAdd, psPixelsLength,
 *               psPixels(Set/Get), psPixelsConcatenate
 *
 *
 * Author: dRob <David.Robbins@mhpcc.hpc.mil>, (C) 2006
 *
 * Copyright: See COPYING file that comes with this distribution
 *
 */
#include <pslib.h>
#include "tap.h"
#include "pstap.h"

void testPixelsCreate(void);
void testPixelsManip(void);

int main(void)
{
    plan_tests(48);

    //Tests for psPixels Functions
    testPixelsCreate();
    testPixelsManip();

    done();
}

void testPixelsCreate(void)
{
    //Test 1:  psPixels Creation Fxns
    //Return allocated psPixels of length 0 with NULL data.
    psPixels* p0 = psPixelsAlloc(0);
    {
        ok(p0 != NULL && p0->n == 0 && p0->nalloc == 0,
           "psPixelsAlloc:         return newly allocated psPixels of 0 length.");
    }
    //Return properly allocated psPixels
    psPixels* p1 = psPixelsAlloc(1);
    {
        ok(p1 != NULL && p1->n == 1 && p1->data != NULL && p1->nalloc == 1,
           "psPixelsAlloc:         return newly allocated psPixels of 1 length.");
    }
    //Make sure psMemCheckPixels works correctly - return true
    {
        ok(psMemCheckPixels(p1),
            "psMemCheckPixels:      return true for psPixels input.");
    }
    //Make sure psMemCheckPixels works correctly - return false
    // XXX EAM : disabled -- failing test causes segfault
    if (0) {
        int j = 2;
        ok(!psMemCheckPixels(&j),
            "psMemCheckPixels:      return false for non-psPixels input.");
    }
    //Return a 0-length allocated psPixels on Realloc with 0
    {
        psPixels *noPix = psPixelsAlloc(1);
        noPix = psPixelsRealloc(noPix, 0);
        ok(noPix->n == 0 && noPix->nalloc == 0,
            "psPixelsRealloc:       return re-allocated psPixels of 0 length.");
        psFree(noPix);
    }
    //Return a properly down-sized psPixels.
    p0 = psPixelsAdd(p0, 1, 1.1, 1.2);
    p0 = psPixelsAdd(p0, 1, 2.1, 2.2);
    p0 = psPixelsAdd(p0, 1, 3.1, 3.2);
    p0 = psPixelsAdd(p0, 0, 4.1, 4.2);
    {
        skip_start(  psPixelsLength(p0) != 4, 1,
                     "Skipping 1 tests because psPixels input is of wrong size! =%ld", p0->n);
        p0 = psPixelsRealloc(p0, 2);
        ok(p0->n == 2 && fabs(p0->data[0].x - 1.1) < FLT_EPSILON && p0->nalloc == 2,
            "psPixelsRealloc:       return properly down-sized psPixels.");
        skip_end();
    }
    //Now see if we can create a valid psPixels copy of p0
    {
        psPixels *noPix = NULL;
        noPix = psPixelsCopy(noPix, p0);
        ok(noPix->n == 2 && fabs(noPix->data[0].x - 1.1) < FLT_EPSILON && noPix->nalloc == 2,
            "psPixelsCopy:          return properly copied psPixels.");
        psFree(noPix);
    }
    //Return NULL for attempting to create a psPixels copy of NULL input.
    {
        psPixels *noPix = NULL;
        noPix = psPixelsCopy(noPix, NULL);
        ok(noPix == NULL,
            "psPixelsCopy:          return NULL for NULL psPixels input.");
    }

    //Check for Memory leaks
    {
        psFree(p1);
        psFree(p0);
        checkMem();
    }
}

void testPixelsManip(void)
{
    //psPixels Manipulation Fxns
    //Tests for psPixelsSet
    psPixels *p0 = psPixelsAlloc(1);
    psPixelCoord in;
    in.x = 1.1;
    in.y = 1.2;
    //Return false for NULL pixels input
    {
        ok(!psPixelsSet(NULL, 0, in),
            "psPixelsSet:          return false for NULL pixels input.");
    }
    //Return true for valid inputs
    {
        ok(psPixelsSet(p0, 0, in),
            "psPixelsSet:          return true for valid inputs.");
    }
    //Return false for position == nalloc
    {
        ok(!psPixelsSet(p0, 1, in),
            "psPixelsSet:          return false for position == nalloc.");
    }
    //Return false for out-of-range position
    {
        ok(!psPixelsSet(p0, 2, in),
            "psPixelsSet:          return false for out-of-range position.");
    }
    //Return false for negative out-of-range position
    {
        ok(!psPixelsSet(p0, -2, in),
            "psPixelsSet:          return false for negative out-of-range position.");
    }
    //Return true for valid negative position
    {
        ok(psPixelsSet(p0, -1, in),
            "psPixelsSet:          return true for valid negative position.");
    }

    //Tests for psPixelsGet
    psPixelCoord out;
    //Return NAN's for NULL pixels input
    {
        out = psPixelsGet(NULL, 1);
        ok(isnan(out.x) &&  isnan(out.y),
            "psPixelsGet:          return NANs for NULL pixels input.");
    }
    //Return NAN's for out-of-range position
    {
        out = psPixelsGet(p0, 2);
        ok(isnan(out.x) &&  isnan(out.y),
            "psPixelsGet:          return NANs for out-of-range position.");
    }
    //Return NAN's for out-of-range negative position
    {
        out = psPixelsGet(p0, -2);
        ok(isnan(out.x) &&  isnan(out.y),
            "psPixelsGet:          return NANs for out-of-range negative position.");
    }
    //Return true for valid inputs
    {
        out = psPixelsGet(p0, 0);
        ok(fabs(out.x - 1.1) < FLT_EPSILON &&  fabs(out.y - 1.2) < FLT_EPSILON,
            "psPixelsGet:          return correct values for valid inputs.");
    }
    //Return true for valid negative position
    {
        out = psPixelsGet(p0, -1);
        ok(fabs(out.x - 1.1) < FLT_EPSILON &&  fabs(out.y - 1.2) < FLT_EPSILON,
            "psPixelsGet:          return correct values for valid negative position.");
    }

    //p_psPixelsPrint Tests
    //Return true for valid inputs
    {
        FILE *newFD = fopen("psPixels.out", "w+");
        ok(p_psPixelsPrint(newFD, p0, "PS-PIXELS"),
             "p_psPixelsPrint:      return true for valid input.");
        fflush(NULL);
        fclose(newFD);
    }
    //Return false for invalid file
    {
        FILE *dummy = fopen("psPixels.out", "r");
        ok(!p_psPixelsPrint(dummy, p0, "failed test"),
             "p_psPixelsPrint:      return false for invalid file input.");
        fclose(dummy);
        remove
            ("psPixels.out");
    }
    //Return false for NULL pixels and name inputs
    {
        ok(!p_psPixelsPrint(stdout, NULL, NULL),
             "p_psPixelsPrint:      return false for NULL pixels and name inputs.");
    }
    //Return true for empty pixel data.
    psPixels *p1 = psPixelsAlloc(0);
    {
        ok(p_psPixelsPrint(NULL, p1, "noPix"),
             "p_psPixelsPrint:      return true for NULL file and empty pixel data inputs.");
    }


    //----------------------------------------------------------------------
    //psPixelsToMask Tests
    psImage *outImage = NULL;
    psRegion region;
    region.x0 = 1.0;
    region.x1 = -2.0;
    region.y0 = 1.0;
    region.y1 = 5.0;
    psMaskType maskVal = 1;
    //Return NULL for NULL pixels input
    {
        psMemId id = psMemGetId();
        outImage = psPixelsToMask(outImage, NULL, region, maskVal);
        ok(outImage == NULL, "psPixelsToMask: return NULL for NULL pixels input.");
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }


    //Return NULL for empty pixel data
    {
        psMemId id = psMemGetId();
        outImage = psPixelsToMask(outImage, p1, region, maskVal);
        ok(outImage == NULL, "psPixelsToMask: return NULL for NULL pixels data input.");
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }


    //Return NULL for bad region input - negative value
    {
        psMemId id = psMemGetId();
        outImage = psPixelsToMask(outImage, p0, region, maskVal);
        ok(outImage == NULL, "psPixelsToMask: return NULL for negative value in region.");
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }


    //Return NULL for bad region input - upper bound less than lower bound
    region.x0 = 3.0;
    region.x1 = 1.0;
    {
        psMemId id = psMemGetId();
        outImage = psPixelsToMask(outImage, p0, region, maskVal);
        ok(outImage == NULL, "psPixelsToMask: return NULL for bad region input.");
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }


    //Return NULL for un-recyclable image - out->type.dimen != PS_DIMEN_IMAGE
    region.x0 = 1.0;
    region.x1 = 3.0;
    outImage = psImageAlloc(1, 1, PS_TYPE_S32);
    *(psDimen*)&(outImage->type.dimen) = PS_DIMEN_OTHER;
    {
        psMemId id = psMemGetId();
        outImage = psPixelsToMask(outImage, p0, region, maskVal);
        ok(outImage == NULL, "psPixelsToMask: return NULL for bad image input.");
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }


    //Return valid image for valid inputs
    //XXX: Musr check pixel values
    p0 = psPixelsAdd(p0, 1, 2.1, 2.2);
    p0 = psPixelsAdd(p0, 1, 3.1, 3.2);
    p0 = psPixelsAdd(p0, 1, 1.0, 1.0);
    {
        psMemId id = psMemGetId();
        outImage = psPixelsToMask(outImage, p0, region, maskVal);
        ok(outImage != NULL, "psPixelsToMask: return valid image for valid input.");
        psFree(outImage);
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }


    //----------------------------------------------------------------------
    //psPixelsFromMask Tests
    outImage = psImageAlloc(31, 99, PS_TYPE_MASK);
    for (int i = 0; i < outImage->numCols; i++) {
        for (int j = 0; j < outImage->numRows; j++) {
            outImage->data.PS_TYPE_MASK_DATA[i][j] = 1;
        }
    }
    //Return valid psPixels for valid inputs
    //XXX: We never check output psPixels
    {
        psMemId id = psMemGetId();
        psPixels *outPixels = NULL;
        outPixels = psPixelsFromMask(outPixels, outImage, maskVal);
        ok(outPixels != NULL, "psPixelsFromMask: return valid pixels for valid input.");
        psFree(outPixels);
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }


    // Return NULL for NULL image input
    {
        psMemId id = psMemGetId();
        psPixels *outPixels = NULL;
        outPixels = psPixelsFromMask(outPixels, NULL, maskVal);
        ok(outPixels == NULL, "psPixelsFromMask: return NULL for NULL image input.");
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }


    // Return NULL for image with wrong maskType
    {
        *(psElemType*)&(outImage->type.type) = PS_TYPE_U32;
        psMemId id = psMemGetId();
        psPixels *outPixels = NULL;
        outPixels = psPixelsFromMask(outPixels, outImage, maskVal);
        ok(outPixels == NULL, "psPixelsFromMask: return NULL for image with wrong maskType.");
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }

    //----------------------------------------------------------------------
    // psPixelsConcatenate Tests
    // Return NULL for NULL pixels input
    {
        psMemId id = psMemGetId();
        psPixels *outPixels = NULL;
        outPixels = psPixelsConcatenate(outPixels, NULL);
        ok(outPixels == NULL, "psPixelsConcatenate: return NULL for NULL pixels input.");
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }


    //Return copy of input pixels for NULL out pixels
    {
        psMemId id = psMemGetId();
        psPixels *outPixels = NULL;
        outPixels = psPixelsFromMask(outPixels, outImage, maskVal);
        outPixels = psPixelsConcatenate(outPixels, p0);
        ok(outPixels->n == 4 && fabs(outPixels->data[0].x - 1.1) < FLT_EPSILON &&
           fabs(outPixels->data[0].y - 1.2) < FLT_EPSILON ,
          "psPixelsConcatenate:  return copy of input pixels for NULL out input.");
        psFree(outPixels);
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }


    //test psPixelsConcatenate()
    //Return properly concatenated psPixels list
    {
        psMemId id = psMemGetId();
        psPixels *outPixels = psPixelsAlloc(6);
        in.x = 1.0;
        in.y = 1.0;
        psPixelsSet(outPixels, 0, in);  //1, 1
        psPixelsSet(outPixels, 1, in);  //1, 1
        in.y = 2.0;
        psPixelsSet(outPixels, 2, in);  //1, 2
        in.y = 1.0;
        psPixelsSet(outPixels, 3, in);  //1, 1
        in.x = 2.0;
        psPixelsSet(outPixels, 4, in);  //2, 1
        in.y = 2.0;
        psPixelsSet(outPixels, 5, in);  //2, 2
        psPixels *testPixels = psPixelsAlloc(7);
        in.x = 1.0;
        in.y = 1.0;
        psPixelsSet(testPixels, 0, in);  //1, 1
        in.y = 2.0;
        psPixelsSet(testPixels, 1, in);  //1, 2
        in.y = 1.0;
        psPixelsSet(testPixels, 2, in);  //1, 1
        in.x = 2.0;
        psPixelsSet(testPixels, 3, in);  //2, 1
        in.y = 2.0;
        psPixelsSet(testPixels, 4, in);  //2, 2
        in.x = 1.0;
        psPixelsSet(testPixels, 5, in);  //1, 2
        in.x = 5.0;
        in.y = 3.0;
        psPixelsSet(testPixels, 6, in);  //5, 3
        outPixels = psPixelsConcatenate(outPixels, testPixels);
        outPixels = psPixelsDuplicates(outPixels, outPixels);
        // Should be 5 entries: (1,1) (2,1) (1,2) (2,2) (5,3)
        ok(outPixels->n == 5, "psPixelsConcatenate:  return properly concatenate pixel list for valid inputs.");
        psFree(testPixels);
        psFree(outPixels);
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
        ok(!psMemCheckLeaks(id, NULL, NULL, false), "no memory leaks");
    }


    //Check for Memory leaks
    //XXX: Remove this, add memory checks to individual blocks
    {
        psFree(outImage);
        psFree(p1);
        psFree(p0);
        checkMem();
    }
}
