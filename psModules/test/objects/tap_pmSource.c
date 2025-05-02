#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include <psmodules.h>
#include "tap.h"
#include "pstap.h"
/*
Tested:
    Tested
        pmSourceAlloc()
        pmSourceCopy()
        pmSourceDefinePixels()
        pmSourceRedefinePixels()
        pmSourcePSFClump()
        pmSourceGetModel()
        pmSourceAdd()
        pmSourceSub()
        pmSourceAddWithOffset()
        pmSourceSubWithOffset()
        pmSourceOp()
        pmSourceCacheModel()
        pmSourceCachePSF()
        pmSourceSortByFlux()	(COMPILER ERRORS)
        pmSourceSortByY()	(COMPILER ERRORS)
    Must test
        pmSourceMoments()
        pmSourceRoughClass()

*/


#define MISC_NUM                32
#define MISC_NAME              "META00"
#define NUM_BIAS_DATA           10
#define TEST_NUM_ROWS           (4+1)
#define TEST_NUM_COLS           (4+1)
#define VERBOSE                 0
#define ERR_TRACE_LEVEL         0
#define TEST_FLOATS_EQUAL(X, Y) (abs(X - Y) < 0.01)

/******************************************************************************
generateSimpleReadout(): This function generates a pmReadout data structure and then
populates its members with real data.
 *****************************************************************************/
pmReadout *generateSimpleReadout(pmCell *cell)
{
    pmReadout *readout = pmReadoutAlloc(cell);
    readout->image = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_F32);
    readout->mask = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_U8);
    readout->variance = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_F32);
    for (psS32 i = 0 ; i < NUM_BIAS_DATA ; i++) {
        psImage *tmpImage = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_F32);
        psImageInit(tmpImage, (double) i);
        psListAdd(readout->bias, PS_LIST_HEAD, tmpImage);
        psFree(tmpImage);
    }
    psMetadataAddS32(readout->analysis, PS_LIST_HEAD, MISC_NAME, 0, NULL, MISC_NUM);
    return(readout);
}


int main(int argc, char* argv[])
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    psTraceSetLevel("err", ERR_TRACE_LEVEL);
    plan_tests(128);

    // ----------------------------------------------------------------------
    // Test pmSourceAlloc()
    // pmSource *pmSourceAlloc();
    {
        psMemId id = psMemGetId();
        pmSource *src = pmSourceAlloc();
        ok(src != NULL && psMemCheckSource(src), "pmSourceAlloc() returned a non-NULL pmSource");
        skip_start(src == NULL, 24, "Skipping tests because pmSourceAlloc() returned NULL");
        ok(src->peak == NULL, "pmSourceAlloc() pmSource->peak correctly");
        ok(src->pixels == NULL, "pmSourceAlloc() pmSource->pixels correctly");
        ok(src->variance == NULL, "pmSourceAlloc() pmSource->variance correctly");
        ok(src->maskObj == NULL, "pmSourceAlloc() pmSource->maskObj correctly");
        ok(src->maskView == NULL, "pmSourceAlloc() pmSource->maskView correctly");
        ok(src->modelFlux == NULL, "pmSourceAlloc() pmSource->modelFlux correctly");
        ok(src->psfFlux == NULL, "pmSourceAlloc() pmSource->psfFlux correctly");
        ok(src->moments == NULL, "pmSourceAlloc() pmSource->moments correctly");
        ok(src->blends == NULL, "pmSourceAlloc() pmSource->blends correctly");
        ok(src->modelPSF == NULL, "pmSourceAlloc() pmSource->modelPSF correctly");
        ok(src->modelEXT == NULL, "pmSourceAlloc() pmSource->modelEXT correctly");
        ok(src->type == PM_SOURCE_TYPE_UNKNOWN, "pmSourceAlloc() pmSource->type correctly");
        ok(src->mode == PM_SOURCE_MODE_DEFAULT, "pmSourceAlloc() pmSource->mode correctly");

        ok(isnan(src->psfMag), "pmSourceAlloc() pmSource->psfMag correctly");
        ok(isnan(src->extMag), "pmSourceAlloc() pmSource->extMag correctly");
        ok(isnan(src->psfMagErr), "pmSourceAlloc() pmSource->psfMagErr correctly");
        ok(isnan(src->apMag), "pmSourceAlloc() pmSource->apMag correctly");
        ok(isnan(src->sky), "pmSourceAlloc() pmSource->sky correctly");
        ok(isnan(src->skyErr), "pmSourceAlloc() pmSource->skyErr correctly");
        ok(isnan(src->pixWeight), "pmSourceAlloc() pmSource->pixWeight correctly");
        int srcID = src->id;
        psFree(src);

        // Allocate another pmSource to ensure that pmSource->id is incremented
        pmSource *src = pmSourceAlloc();
        ok(src != NULL, "pmSourceAlloc() returned a non-NULL pmSource");
        ok(src->id == srcID+1, "pmSourceAlloc() incremented the pmSource->id correctly");
        psFree(src);

        skip_end();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // pmSourceCopy() tests
    // Call pmSourceCopy() with NULL input
    {
        psMemId id = psMemGetId();
        pmSource *src = pmSourceCopy(NULL);
        ok(src == NULL, "pmSourceCopy(NULL) returned NULL");
        psFree(src);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmSourceCopy() with non-NULL input, but NULL members
    {
        psMemId id = psMemGetId();
        pmSource *src = pmSourceAlloc();
        ok(src != NULL, "pmSourceAlloc() returned a non-NULL pmSource");
        pmSource *dst = pmSourceCopy(src);
        ok(dst != NULL, "pmSourceCopy() returned a non-NULL pmSource");
        ok(dst != src, "pmSourceCopy() allocated a new pmSource");
        ok(dst->type == src->type, "pmSourceCopy() set the pmSource->type");
        ok(dst->mode == src->mode, "pmSourceCopy() set the pmSource->mode");
        psFree(src);
        psFree(dst);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmSourceCopy() with non-NULL input, non-NULL members
    {
        psMemId id = psMemGetId();
        pmSource *src = pmSourceAlloc();
        ok(src != NULL, "pmSourceAlloc() returned a non-NULL pmSource");
        // Set pmPeak values
        src->peak = pmPeakAlloc (1, 2, 3.0, PM_PEAK_LONE);
        src->peak->xf = 4.0;
        src->peak->yf = 5.0;
        src->peak->flux = 6.0;
        src->peak->SN = 10.0;
        src->moments = pmMomentsAlloc();
        src->moments->Mx = 11.0;
        src->moments->My = 11.0;
        src->moments->Mxx = 11.0;
        src->moments->Myy = 11.0;
        src->moments->Mxxx = 12.0;
        src->moments->Mxxy = 12.0;
        src->moments->Mxyy = 12.0;
        src->moments->Myyy = 12.0;
        src->moments->Mxxxx = 13.0;
        src->moments->Mxxxy = 13.0;
        src->moments->Mxyyy = 13.0;
        src->moments->Myyyy = 13.0;
        src->moments->Sum = 14.0;
        src->moments->Peak = 14.0;
        src->moments->Sky = 14.0;
        src->moments->dSky = 14.0;
        src->moments->nPixels = 14.0;

        src->pixels = psImageAlloc(2, 4, PS_TYPE_F32);
        src->variance = psImageAlloc(6, 8, PS_TYPE_F32);
        src->maskView  = psImageAlloc(10, 12, PS_TYPE_U8);
        pmSource *dst = pmSourceCopy(src);
        ok(dst != NULL, "pmSourceCopy() returned a non-NULL pmSource");
        ok(dst != src, "pmSourceCopy() allocated a new pmSource");
        ok(dst->type == src->type, "pmSourceCopy() set the pmSource->type");
        ok(dst->mode == src->mode, "pmSourceCopy() set the pmSource->mode");
        ok(dst->peak != NULL, "pmSourceCopy() allocated a new pmSource->peak");
        ok(dst->peak->xf == src->peak->xf, "pmSourceCopy() pmSource->peak->xf");
        ok(dst->peak->yf == src->peak->yf, "pmSourceCopy() pmSource->peak->yf");
        ok(dst->peak->flux == src->peak->flux, "pmSourceCopy() pmSource->peak->flux");
        ok(dst->peak->SN == src->peak->SN, "pmSourceCopy() pmSource->peak->SN");
        ok(dst->moments != NULL, "pmSourceCopy() allocated a new pmSource->moments");
        ok(dst->moments->Mx == src->moments->Mx, "pmSourceCopy() pmSource->moments->Mx");
        ok(dst->moments->My == src->moments->My, "pmSourceCopy() pmSource->moments->My");
        ok(dst->moments->Mxx == src->moments->Mxx, "pmSourceCopy() pmSource->moments->Mxx");
        ok(dst->moments->Mxy == src->moments->Mxy, "pmSourceCopy() pmSource->moments->Mxy");
        ok(dst->moments->Myy == src->moments->Myy, "pmSourceCopy() pmSource->moments->Myy");
        ok(dst->moments->Mxxx == src->moments->Mxxx, "pmSourceCopy() pmSource->moments->Mxxx");
        ok(dst->moments->Mxxy == src->moments->Mxxy, "pmSourceCopy() pmSource->moments->Mxxy");
        ok(dst->moments->Mxyy == src->moments->Mxyy, "pmSourceCopy() pmSource->moments->Mxyy");
        ok(dst->moments->Myyy == src->moments->Myyy, "pmSourceCopy() pmSource->moments->Myyy");
        ok(dst->moments->Mxxxx == src->moments->Mxxxx, "pmSourceCopy() pmSource->moments->Mxxxx");
        ok(dst->moments->Mxxxy == src->moments->Mxxxy, "pmSourceCopy() pmSource->moments->Mxxxy");
        ok(dst->moments->Mxxyy == src->moments->Mxxyy, "pmSourceCopy() pmSource->moments->Mxxyy");
        ok(dst->moments->Mxxxy == src->moments->Mxxxy, "pmSourceCopy() pmSource->moments->Mxxxy");
        ok(dst->moments->Myyyy == src->moments->Myyyy, "pmSourceCopy() pmSource->moments->Myyyy");
        ok(dst->moments->Sum == src->moments->Sum, "pmSourceCopy() pmSource->moments->Sum");
        ok(dst->moments->Peak == src->moments->Peak, "pmSourceCopy() pmSource->moments->Peak");
        ok(dst->moments->Sky == src->moments->Sky, "pmSourceCopy() pmSource->moments->Sky");
        ok(dst->moments->dSky == src->moments->dSky, "pmSourceCopy() pmSource->moments->dSky");
        ok(dst->moments->SN == src->moments->SN, "pmSourceCopy() pmSource->moments->SN");
        ok(dst->moments->nPixels == src->moments->nPixels, "pmSourceCopy() pmSource->moments->nPixels");

        // XXX: We should possibly do a better job testing that these images are copied correctly.
        ok(dst->pixels != NULL, "pmSourceCopy() allocated a new pmSource->pixels");
        ok(dst->pixels->numCols == src->pixels->numCols && dst->pixels->numRows == src->pixels->numRows, 
           "pmSourceCopy() generated correct size pmSource->pixels");
        ok(dst->variance != NULL, "pmSourceCopy() allocated a new pmSource->variance");
        ok(dst->variance->numCols == src->variance->numCols && dst->variance->numRows == src->variance->numRows, 
           "pmSourceCopy() generated correct size pmSource->variance");
        ok(dst->maskView != NULL, "pmSourceCopy() allocated a new pmSource->maskView");
        ok(dst->maskView->numCols == src->maskView->numCols && dst->maskView->numRows == src->maskView->numRows, 
           "pmSourceCopy() generated correct size pmSource->maskView");

        psFree(src);
        psFree(dst);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // pmSourceDefinePixels() tests
    // Call pmSourceDefinePixels() with NULL pmSource input parameter
    {
        psMemId id = psMemGetId();
        pmSource *src = pmSourceAlloc();
        pmReadout *readout = generateSimpleReadout(NULL);
        ok(!pmSourceDefinePixels(NULL, readout, 1.0, 2.0, 3.0),
           "pmSourceDefinePixels() returned false with NULL pmSource input parameter");
        psFree(src);
        psFree(readout);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmSourceDefinePixels() with NULL pmReadout input parameter
    {
        psMemId id = psMemGetId();
        pmSource *src = pmSourceAlloc();
        pmReadout *readout = generateSimpleReadout(NULL);
        ok(!pmSourceDefinePixels(src, NULL, 1.0, 2.0, 3.0),
           "pmSourceDefinePixels() returned false with NULL pmReadout input parameter");
        psFree(src);
        psFree(readout);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmSourceDefinePixels() with NULL pmReadout->image input parameter
    {
        psMemId id = psMemGetId();
        pmSource *src = pmSourceAlloc();
        pmReadout *readout = generateSimpleReadout(NULL);
        psFree(readout->image);
        readout->image = NULL;
        ok(!pmSourceDefinePixels(src, NULL, 1.0, 2.0, 3.0),
           "pmSourceDefinePixels() returned false with NULL pmReadout input parameter");
        psFree(src);
        psFree(readout);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmSourceDefinePixels() with negative radius input parameter
    {
        psMemId id = psMemGetId();
        pmSource *src = pmSourceAlloc();
        pmReadout *readout = generateSimpleReadout(NULL);
        ok(!pmSourceDefinePixels(src, readout, 1.0, 2.0, -3.0),
           "pmSourceDefinePixels() returned false with negative radius input parameter");
        psFree(src);
        psFree(readout);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmSourceDefinePixels() with acceptable input parameters
    {
        psMemId id = psMemGetId();
        pmSource *src = pmSourceAlloc();
        pmReadout *readout = generateSimpleReadout(NULL);
        for (int i = 0 ; i < readout->image->numRows ; i++) {
            for (int j = 0 ; j < readout->image->numCols ; j++) {
                readout->image->data.F32[i][j] = (float) (i + j);
	    }
	}
        bool rc = pmSourceDefinePixels(src, readout, (float) TEST_NUM_COLS/2, (float) TEST_NUM_ROWS/2, 2.0);
        ok(rc, "pmSourceDefinePixels() returned TRUE with acceptable input parameters");
        int expectedNumCols = 1 + (TEST_NUM_COLS/2);
        int expectedNumRows = 1 + (TEST_NUM_COLS/2);
        // XX: We only verify the size of the pixels, variance, maskView, and maskObj
        // images.  A better test would verify that the actual data is set correctly.
        // We don't do that here since it basically duplicated psLib function tests.
        ok(src->pixels->numCols == expectedNumCols && src->pixels->numRows == expectedNumRows, 
               "pmSourceDefinePixels() set the size of pmSource->pixels correctly");
        ok(src->variance->numCols == expectedNumCols && src->variance->numRows == expectedNumRows, 
               "pmSourceDefinePixels() set the size of pmSource->variance correctly");
        ok(src->maskView->numCols == expectedNumCols && src->maskView->numRows == expectedNumRows, 
               "pmSourceDefinePixels() set the size of pmSource->maskView correctly");
        ok(src->maskObj->numCols == expectedNumCols && src->maskObj->numRows == expectedNumRows, 
               "pmSourceDefinePixels() set the size of pmSource->maskObj correctly");

        psRegion region = psRegionForSquare((float) TEST_NUM_COLS/2, (float) TEST_NUM_ROWS/2, 2.0);
        region = psRegionForImage(readout->image, region);
        ok(src->region.x0 == region.x0 && src->region.x1 == region.x1 &&
           src->region.y0 == region.y0 && src->region.y1 == region.y1,
          "pmSourceDefinePixels() set pmSource->region correctly");

        psFree(src);
        psFree(readout);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }





    // ----------------------------------------------------------------------
    // pmSourceRedefinePixels() tests
    // Call pmSourceRedefinePixels() with NULL pmSource input parameter
    {
        psMemId id = psMemGetId();
        pmSource *src = pmSourceAlloc();
        pmReadout *readout = generateSimpleReadout(NULL);
        ok(!pmSourceRedefinePixels(NULL, readout, 1.0, 2.0, 3.0),
           "pmSourceRedefinePixels() returned false with NULL pmSource input parameter");
        psFree(src);
        psFree(readout);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmSourceRedefinePixels() with NULL pmReadout input parameter
    {
        psMemId id = psMemGetId();
        pmSource *src = pmSourceAlloc();
        pmReadout *readout = generateSimpleReadout(NULL);
        ok(!pmSourceRedefinePixels(src, NULL, 1.0, 2.0, 3.0),
           "pmSourceRedefinePixels() returned false with NULL pmReadout input parameter");
        psFree(src);
        psFree(readout);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmSourceRedefinePixels() with NULL pmReadout->image input parameter
    {
        psMemId id = psMemGetId();
        pmSource *src = pmSourceAlloc();
        pmReadout *readout = generateSimpleReadout(NULL);
        psFree(readout->image);
        readout->image = NULL;
        ok(!pmSourceRedefinePixels(src, NULL, 1.0, 2.0, 3.0),
           "pmSourceRedefinePixels() returned false with NULL pmReadout input parameter");
        psFree(src);
        psFree(readout);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmSourceRedefinePixels() with negative radius input parameter
    {
        psMemId id = psMemGetId();
        pmSource *src = pmSourceAlloc();
        pmReadout *readout = generateSimpleReadout(NULL);
        ok(!pmSourceRedefinePixels(src, readout, 1.0, 2.0, -3.0),
           "pmSourceRedefinePixels() returned false with negative radius input parameter");
        psFree(src);
        psFree(readout);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmSourceRedefinePixels() with acceptable input parameters
    if (1) {
        psMemId id = psMemGetId();
        pmSource *src = pmSourceAlloc();
        pmReadout *readout = generateSimpleReadout(NULL);
        for (int i = 0 ; i < readout->image->numRows ; i++) {
            for (int j = 0 ; j < readout->image->numCols ; j++) {
                readout->image->data.F32[i][j] = (float) (i + j);
	    }
	}
        // First set the pixels region to a square of radius 1.
        bool rc = pmSourceDefinePixels(src, readout, (float) TEST_NUM_COLS/2, (float) TEST_NUM_ROWS/2, 1.0);
        ok(rc, "pmSourceDefinePixels() returned TRUE with radius 1");

        // Set these flux images so we can verify they are later psFree'ed and set to NULL
        src->modelFlux = psImageAlloc(2, 2, PS_TYPE_F32);
        src->psfFlux = psImageAlloc(2, 2, PS_TYPE_F32);

        // Now set the radius to 2, and call pmSourceRedefinePixels()
        rc = pmSourceRedefinePixels(src, readout, (float) TEST_NUM_COLS/2, (float) TEST_NUM_ROWS/2, 2.0);
        ok(rc, "pmSourceRedefinePixels() returned TRUE with acceptable input parameters");
        int expectedNumCols = 1 + (TEST_NUM_COLS/2);
        int expectedNumRows = 1 + (TEST_NUM_COLS/2);
        // XX: We only verify the size of the pixels, variance, maskView, and maskObj
        // images.  A better test would verify that the actual data is set correctly.
        // We don't do that here since it basically duplicated psLib function tests.
        ok(src->pixels->numCols == expectedNumCols && src->pixels->numRows == expectedNumRows, 
               "pmSourceRedefinePixels() set the size of pmSource->pixels correctly");
        ok(src->variance->numCols == expectedNumCols && src->variance->numRows == expectedNumRows, 
               "pmSourceRedefinePixels() set the size of pmSource->variance correctly");
        ok(src->maskView->numCols == expectedNumCols && src->maskView->numRows == expectedNumRows, 
               "pmSourceRedefinePixels() set the size of pmSource->maskView correctly");
        ok(src->maskObj->numCols == expectedNumCols && src->maskObj->numRows == expectedNumRows, 
               "pmSourceRedefinePixels() set the size of pmSource->maskObj correctly");

        psRegion region = psRegionForSquare((float) TEST_NUM_COLS/2, (float) TEST_NUM_ROWS/2, 2.0);
        region = psRegionForImage(readout->image, region);
        ok(src->region.x0 == region.x0 && src->region.x1 == region.x1 &&
           src->region.y0 == region.y0 && src->region.y1 == region.y1,
          "pmSourceRedefinePixels() set pmSource->region correctly");

        ok(src->modelFlux == NULL, "pmSourceRedefinePixels() set the pmSource->modelFlux to NULL");
        ok(src->psfFlux == NULL, "pmSourceRedefinePixels() set the pmSource->psfFlux to NULL");

        psFree(src);
        psFree(readout);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // pmSourcePSFClump() tests
    // pmPSFClump pmSourcePSFClump(psArray *sources, psMetadata *recipe)
    // Call pmSourcePSFClump() with NULL pmSource input parameter
    #define NUM_SOURCES 10
    {
        psMemId id = psMemGetId();
        psArray *sources = psArrayAlloc(NUM_SOURCES);
        psMetadata *recipe = psMetadataAlloc();
        bool rc = psMetadataAddF32(recipe, PS_LIST_HEAD, "PSF_SN_LIM", 0, NULL, 0.0);
        rc = psMetadataAddF32(recipe, PS_LIST_HEAD, "MOMENTS_SX_MAX", 0, NULL, 10.0);
        rc = psMetadataAddF32(recipe, PS_LIST_HEAD, "MOMENTS_SY_MAX", 0, NULL, 10.0);
        rc = psMetadataAddF32(recipe, PS_LIST_HEAD, "MOMENTS_AR_MAX", 0, NULL, 3.0);

        pmPSFClump clump = pmSourcePSFClump(NULL, NULL, recipe);
        ok(clump.X == -1.0 && clump.dX == -1.0 && 
           clump.Y == 0.0 && clump.dY == 0.0, "pmSourcePSFClump(NULL, recipe) returned the error clump");

        psFree(sources);
        psFree(recipe);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmSourcePSFClump() with NULL recipe input parameter
    {
        psMemId id = psMemGetId();
        psArray *sources = psArrayAlloc(NUM_SOURCES);
        psMetadata *recipe = psMetadataAlloc();
        bool rc = psMetadataAddF32(recipe, PS_LIST_HEAD, "PSF_SN_LIM", 0, NULL, 0.0);
        rc = psMetadataAddF32(recipe, PS_LIST_HEAD, "MOMENTS_SX_MAX", 0, NULL, 10.0);
        rc = psMetadataAddF32(recipe, PS_LIST_HEAD, "MOMENTS_SY_MAX", 0, NULL, 10.0);
        rc = psMetadataAddF32(recipe, PS_LIST_HEAD, "MOMENTS_AR_MAX", 0, NULL, 3.0);

        pmPSFClump clump = pmSourcePSFClump(NULL, sources, NULL);
        ok(clump.X == -1.0 && clump.dX == -1.0 && 
           clump.Y == 0.0 && clump.dY == 0.0, "pmSourcePSFClump(sources, NULL) returned the error clump");

        psFree(sources);
        psFree(recipe);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmSourcePSFClump() with acceptable input parameters
    // XX: This is a fairly simplistic test.  We set the moments of all test pmSources
    // to (5.0, 5.0), so the clump should be fairly easy to detect.  For further testing
    // add:
    //	  Add outliers to the Mx and My data, make sure they aren't used in the calculation
    //    Force it to get the various metadata from the recipes arg.
    //    Add a few non pmSource types to the sources input pmArray
    //    Add a few NULL pmSources, or NULL pmSource->moments to the sources input pmArray
    //    Ensure the data is saved with the KEEP_PSF_CLUMP metadata item
    //    Add cases where you fail to find a peak.
    //    Modify data so that the inputs have different moments
    //
    {
        psMemId id = psMemGetId();
        psArray *sources = psArrayAlloc(NUM_SOURCES);
        for (int i = 0 ; i < sources->n ; i++) {
            pmSource *src = pmSourceAlloc();
            src->moments = pmMomentsAlloc();
            src->moments->Mx = 5.0;
            src->moments->My = 5.0;
            sources->data[i] = src;
	}
        psMetadata *recipe = psMetadataAlloc();
        bool rc = psMetadataAddF32(recipe, PS_LIST_HEAD, "PSF_SN_LIM", 0, NULL, 0.0);
        rc = psMetadataAddF32(recipe, PS_LIST_HEAD, "MOMENTS_SX_MAX", 0, NULL, 10.0);
        rc = psMetadataAddF32(recipe, PS_LIST_HEAD, "MOMENTS_SY_MAX", 0, NULL, 10.0);
        rc = psMetadataAddF32(recipe, PS_LIST_HEAD, "MOMENTS_AR_MAX", 0, NULL, 3.0);

        pmPSFClump clump = pmSourcePSFClump(NULL, sources, recipe);
        ok(clump.X == 5.0 && clump.dX == 0.0 && 
           clump.Y == 5.0 && clump.dY == 0.0, "pmSourcePSFClump(sources, NULL) returned the correct clump");

        if (VERBOSE) {
            printf("clump: (%.2f %.2f %.2f %.2f)\n", clump.X, clump.dX, clump.Y, clump.dY);
	}
        psFree(sources);
        psFree(recipe);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // pmSourceGetModel() tests
    // pmModel *pmSourceGetModel (bool *isPSF, const pmSource *source)
    // Call pmSourceGetModel() with NULL pmSource input parameter
    #define NUM_SOURCES 10
    {
        psMemId id = psMemGetId();
        pmSource *src = pmSourceAlloc();
        bool isPDF;
        pmModel *model = pmSourceGetModel(&isPDF, NULL);
        ok(model == NULL, "pmSourceGetModel() returned NULL with NULL pmSource input parameter");

        psFree(src);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Call pmSourceGetModel() with acceptable input parameters
    #define NUM_SOURCES 10
    {
        psMemId id = psMemGetId();
        pmSource *src = pmSourceAlloc();
        // For testing only:
        src->modelPSF = (pmModel *) 1;
        src->modelEXT = (pmModel *) 3;
        bool isPDF;

        src->type = PM_SOURCE_TYPE_UNKNOWN;
        pmModel *model = pmSourceGetModel(&isPDF, src);
        ok(model == NULL, "pmSourceGetModel() returned a NULL pmModel with acceptable input parameters and src->type = PM_SOURCE_TYPE_UNKNOWN");
        ok(false == isPDF, "pmSourceGetModel() set isPDF to FALSE");

        src->type = PM_SOURCE_TYPE_STAR;
        model = pmSourceGetModel(&isPDF, src);
        ok(model != NULL, "pmSourceGetModel() returned a non-NULL pmModel with acceptable input parameters");
        //ok(1 == (int) model, "pmSourceGetModel() returned the correct model with pmSource->type == PM_SOURCE_TYPE_STAR");
        ok(true == isPDF, "pmSourceGetModel() set isPDF to TRUE");

        src->type = PM_SOURCE_TYPE_EXTENDED;
        model = pmSourceGetModel(&isPDF, src);
        ok(model != NULL, "pmSourceGetModel() returned a non-NULL pmModel with acceptable input parameters");
        //ok(2 == (int) model, "pmSourceGetModel() returned the correct model with pmSource->type == PM_SOURCE_TYPE_EXTENDED (%d)", (int) model);
        ok(false == isPDF, "pmSourceGetModel() set isPDF to FALSE");

        src->type = PM_SOURCE_TYPE_EXTENDED;
        model = pmSourceGetModel(&isPDF, src);
        ok(model != NULL, "pmSourceGetModel() returned a non-NULL pmModel with acceptable input parameters");
        //ok(3 == (int) model, "pmSourceGetModel() returned the correct model with pmSource->type == PM_SOURCE_TYPE_EXTENDED");
        ok(false == isPDF, "pmSourceGetModel() set isPDF to FALSE");

        src->modelPSF = NULL;
        src->modelEXT = NULL;
        psFree(src);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // pmSourceOp() tests
    // bool pmSourceOp (pmSource *source, pmModelOpMode mode, bool add,
    //                  psMaskType maskVal, int dx, int dy)
    // Call pmSourceOp() with NULL pmSource input parameter
    #define NUM_SOURCES 10
    {
        psMemId id = psMemGetId();
        pmSource *src = pmSourceAlloc();
        bool rc = pmSourceOp(NULL, PM_MODEL_OP_NONE, true, 1, 0, 0);
        ok(!rc, "pmSourceOpl() returned FALSE with NULL pmSource input parameter");
        psFree(src);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // call pmSourceOp() with acceptable parameters
    // We only test with a single Gaussian model, with no residuals or masks.
    // For completeness, additional tests should be added.
        // We should also set mode &= PM_MODEL_OP_NOISE to test that the src->variances are added.
    {
        psMemId id = psMemGetId();
        pmSource *src = pmSourceAlloc();
        src->pixels = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_F32);
        for (int i = 0 ; i < TEST_NUM_ROWS ; i++) {
            for (int j = 0 ; j < TEST_NUM_COLS ; j++) {
                src->pixels->data.F32[i][j] = 0.0;
            }
        }
        src->peak = pmPeakAlloc(TEST_NUM_COLS/2, TEST_NUM_ROWS/2, 5.0, PM_PEAK_LONE);
        src->type = PM_SOURCE_TYPE_STAR;
        src->modelPSF = pmModelAlloc(pmModelClassGetType("PS_MODEL_GAUSS"));
        psF32 *PAR = src->modelPSF->params->data.F32;
        PAR[PM_PAR_I0] = 5.0;
        PAR[PM_PAR_XPOS] = 0.0;
        PAR[PM_PAR_YPOS] = 0.0;
        PAR[PM_PAR_XPOS] = (float) (TEST_NUM_COLS/2);
        PAR[PM_PAR_YPOS] = (float) (TEST_NUM_ROWS/2);
        PAR[PM_PAR_SXX] = 10.0;
        PAR[PM_PAR_SYY] = 10.0;

        bool rc = pmSourceOp(src, PM_SOURCE_MODE_PSFMODEL, true, 0, 0, 0);
        ok(rc == true, "pmSourceOp() returned TRUE with acceptable input parameters");
        psVector *x = psVectorAlloc(2, PS_TYPE_F32);
        bool errorFlag = false;
        for (int i = 0 ; i < TEST_NUM_ROWS ; i++) {
            for (int j = 0 ; j < TEST_NUM_COLS ; j++) {
                x->data.F32[0] = (float) j;
                x->data.F32[1] = (float) i;
                psF32 modF = src->modelPSF->modelFunc (NULL, src->modelPSF->params, x);
                psF32 imgF = src->pixels->data.F32[i][j];
                if (!TEST_FLOATS_EQUAL(modF, imgF)) {
                    diag("ERROR: src->pixels[%d][%d] is %.2f, should be %.2f", i, j, src->pixels->data.F32[i][j], modF);
                    errorFlag = true;
                }
            }
        }
        ok(!errorFlag, "pmSourceOp() set the image pixels correctly (PSF function)");
        psFree(x);
        psFree(src);
        pmModelClassCleanup();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // call pmSourceOp() with acceptable parameters
    // Test source->modelFlux
        // We should also set mode &= PM_MODEL_OP_NOISE to test that the src->variances are added.
    {
        psMemId id = psMemGetId();
        pmSource *src = pmSourceAlloc();
        src->pixels = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_F32);
        src->modelFlux = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_F32);
        for (int i = 0 ; i < TEST_NUM_ROWS ; i++) {
            for (int j = 0 ; j < TEST_NUM_COLS ; j++) {
                src->pixels->data.F32[i][j] = 0.0;
                src->modelFlux->data.F32[i][j] = (float) (i + j);
            }
        }
        src->peak = pmPeakAlloc(TEST_NUM_COLS/2, TEST_NUM_ROWS/2, 5.0, PM_PEAK_LONE);
        src->type = PM_SOURCE_TYPE_STAR;
        src->modelPSF = pmModelAlloc(pmModelClassGetType("PS_MODEL_GAUSS"));
        psF32 *PAR = src->modelPSF->params->data.F32;
        PAR[PM_PAR_I0] = 1.0;
        PAR[PM_PAR_XPOS] = 0.0;
        PAR[PM_PAR_YPOS] = 0.0;
        PAR[PM_PAR_XPOS] = (float) (TEST_NUM_COLS/2);
        PAR[PM_PAR_YPOS] = (float) (TEST_NUM_ROWS/2);
        PAR[PM_PAR_SXX] = 10.0;
        PAR[PM_PAR_SYY] = 10.0;

        bool rc = pmSourceOp(src, PM_SOURCE_MODE_PSFMODEL, true, 0, 0, 0);
        ok(rc == true, "pmSourceOp() returned TRUE with acceptable input parameters");
        psVector *x = psVectorAlloc(2, PS_TYPE_F32);
        bool errorFlag = false;
        for (int i = 0 ; i < TEST_NUM_ROWS ; i++) {
            for (int j = 0 ; j < TEST_NUM_COLS ; j++) {
                x->data.F32[0] = (float) j;
                x->data.F32[1] = (float) i;
                psF32 modF = src->modelFlux->data.F32[i][j];
                psF32 imgF = src->pixels->data.F32[i][j];
                if (!TEST_FLOATS_EQUAL(modF, imgF)) {
                    diag("ERROR: src->pixels[%d][%d] is %.2f, should be %.2f", i, j, src->pixels->data.F32[i][j], modF);
                    errorFlag = true;
                }
            }
        }
        ok(!errorFlag, "pmSourceOp() set the image pixels correctly (src->modelFlux cache image)");
        psFree(x);
        psFree(src);
        pmModelClassCleanup();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // pmSourceCacheModel() tests
    // call pmSourceCacheModel() with NULL pmSource input parameter
    {
        psMemId id = psMemGetId();
        bool rc = pmSourceCacheModel(NULL, 0);
        ok(rc == false, "pmSourceCacheModel() returned FALSE with NULL pmSource input parameter");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // pmSourceCacheModel() tests
    // bool pmSourceCacheModel (pmSource *source, psMaskType maskVal) {
    // call pmSourceCacheModel() with acceptable parameters
    {
        psMemId id = psMemGetId();
        pmSource *src = pmSourceAlloc();
        src->pixels = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_F32);
        for (int i = 0 ; i < TEST_NUM_ROWS ; i++) {
            for (int j = 0 ; j < TEST_NUM_COLS ; j++) {
                src->pixels->data.F32[i][j] = 0.0;
            }
        }
        src->peak = pmPeakAlloc(TEST_NUM_COLS/2, TEST_NUM_ROWS/2, 5.0, PM_PEAK_LONE);
        src->type = PM_SOURCE_TYPE_STAR;
        src->modelPSF = pmModelAlloc(pmModelClassGetType("PS_MODEL_GAUSS"));
        psF32 *PAR = src->modelPSF->params->data.F32;
        PAR[PM_PAR_I0] = 1.0;
        PAR[PM_PAR_XPOS] = 0.0;
        PAR[PM_PAR_YPOS] = 0.0;
        PAR[PM_PAR_XPOS] = (float) (TEST_NUM_COLS/2);
        PAR[PM_PAR_YPOS] = (float) (TEST_NUM_ROWS/2);
        PAR[PM_PAR_SXX] = 10.0;
        PAR[PM_PAR_SYY] = 10.0;

        bool rc = pmSourceCacheModel(src, 0);
        ok(rc == true, "pmSourceCacheModel() returned TRUE with acceptable input parameters");

        psVector *x = psVectorAlloc(2, PS_TYPE_F32);
        bool errorFlag = false;
        for (int i = 0 ; i < TEST_NUM_ROWS ; i++) {
            for (int j = 0 ; j < TEST_NUM_COLS ; j++) {
                x->data.F32[0] = (float) j;
                x->data.F32[1] = (float) i;
                psF32 modF = src->modelPSF->modelFunc (NULL, src->modelPSF->params, x);
                psF32 imgF = src->modelFlux->data.F32[i][j];
                if (!TEST_FLOATS_EQUAL(modF, imgF)) {
                    diag("ERROR: src->modelFlux[%d][%d] is %.2f, should be %.2f", i, j, src->modelFlux->data.F32[i][j], modF);
                    errorFlag = true;
                }
            }
        }
        ok(!errorFlag, "pmSourceCacheModel() set the src->modelFlux correctly (PSF function)");
        psFree(x);
        psFree(src);
        pmModelClassCleanup();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // pmSourceCachePSF() tests
    // bool pmSourceCachePSF (pmSource *source, psMaskType maskVal) {
    // call pmSourceCachePSF() with NULL pmSource input parameter
    {
        psMemId id = psMemGetId();
        bool rc = pmSourceCachePSF(NULL, 0);
        ok(rc == false, "pmSourceCachePSF() returned FALSE with NULL pmSource input parameter");
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // call pmSourceCachePSF() with acceptable parameters
    {
        psMemId id = psMemGetId();
        pmSource *src = pmSourceAlloc();
        src->pixels = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_F32);
        for (int i = 0 ; i < TEST_NUM_ROWS ; i++) {
            for (int j = 0 ; j < TEST_NUM_COLS ; j++) {
                src->pixels->data.F32[i][j] = 0.0;
            }
        }
        src->peak = pmPeakAlloc(TEST_NUM_COLS/2, TEST_NUM_ROWS/2, 5.0, PM_PEAK_LONE);
        src->type = PM_SOURCE_TYPE_STAR;
        src->modelPSF = pmModelAlloc(pmModelClassGetType("PS_MODEL_GAUSS"));
        psF32 *PAR = src->modelPSF->params->data.F32;
        PAR[PM_PAR_I0] = 1.0;
        PAR[PM_PAR_XPOS] = 0.0;
        PAR[PM_PAR_YPOS] = 0.0;
        PAR[PM_PAR_XPOS] = (float) (TEST_NUM_COLS/2);
        PAR[PM_PAR_YPOS] = (float) (TEST_NUM_ROWS/2);
        PAR[PM_PAR_SXX] = 10.0;
        PAR[PM_PAR_SYY] = 10.0;

        bool rc = pmSourceCachePSF(src, 0);
        ok(rc == true, "pmSourceCachePSF() returned TRUE with acceptable input parameters");

        psVector *x = psVectorAlloc(2, PS_TYPE_F32);
        bool errorFlag = false;
        for (int i = 0 ; i < TEST_NUM_ROWS ; i++) {
            for (int j = 0 ; j < TEST_NUM_COLS ; j++) {
                x->data.F32[0] = (float) j;
                x->data.F32[1] = (float) i;
                psF32 modF = src->modelPSF->modelFunc (NULL, src->modelPSF->params, x);
                psF32 imgF = src->psfFlux->data.F32[i][j];
                if (!TEST_FLOATS_EQUAL(modF, imgF)) {
                    diag("ERROR: src->psfFlux[%d][%d] is %.2f, should be %.2f", i, j, src->psfFlux->data.F32[i][j], modF);
                    errorFlag = true;
                }
            }
        }
        ok(!errorFlag, "pmSourceCachePSF() set the src->psfFlux correctly (PSF function)");
        psFree(x);
        psFree(src);
        pmModelClassCleanup();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------
    // pmSourceSortByFlux() tests
    // int pmSourceSortByFlux (const void **a, const void **b)
    // Call pmSourceSortByFlux() with acceptable input parameters.
    // XXX: We don't test with NULL input parameters since this function has no PS_ASSERTS to protect
    // against that.
/* XXXX: Compiler errors: fix this
    {
        psMemId id = psMemGetId();
        pmSource *src1 = pmSourceAlloc();
        src1->peak = pmPeakAlloc(3, 4, 2.0, PM_PEAK_LONE);
        src1->peak->SN = 10.0;
        pmSource *src2 = pmSourceAlloc();
        src2->peak = pmPeakAlloc(3, 4, 2.0, PM_PEAK_LONE);
        src2->peak->SN = 20.0;

        int rc = pmSourceSortByFlux((const void **) &src1, (const void **) &src2);
        ok(rc == 1, "pmSourceSortByFlux() returned correct result (source1 < source2) (%d)", rc);
        rc = pmSourceSortByFlux((const void **) &src2, (const void **) &src1);
        ok(rc == -1, "pmSourceSortByFlux() returned correct result (source2 < source1) (%d)", rc);
        rc = pmSourceSortByFlux((const void **) &src1, (const void **) &src1);
        ok(rc == 0, "pmSourceSortByFlux() returned correct result (source1 == source2) (%d)", rc);

        psFree(src1);
        psFree(src2);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------
    // pmSourceSortByY() tests
    // int pmSourceSortByY (const void **a, const void **b)
    // Call pmSourceSortByY() with acceptable input parameters.
    // XXX: We don't test with NULL input parameters since this function has no PS_ASSERTS to protect
    // against that.
    {
        psMemId id = psMemGetId();
        pmSource *src1 = pmSourceAlloc();
        src1->peak = pmPeakAlloc(3, 4, 2.0, PM_PEAK_LONE);
        src1->peak->y = 10.0;
        pmSource *src2 = pmSourceAlloc();
        src2->peak = pmPeakAlloc(3, 4, 2.0, PM_PEAK_LONE);
        src2->peak->y = 20.0;

        int rc = pmSourceSortByY((const void **) &src1, (const void **) &src2);
        ok(rc == -1, "pmSourceSortByY() returned correct result (source1 < source2) (%d)", rc);
        rc = pmSourceSortByY((const void **) &src2, (const void **) &src1);
        ok(rc == 1, "pmSourceSortByY() returned correct result (source2 < source1) (%d)", rc);
        rc = pmSourceSortByY((const void **) &src1, (const void **) &src1);
        ok(rc == 0, "pmSourceSortByY() returned correct result (source1 == source2) (%d)", rc);

        psFree(src1);
        psFree(src2);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
*/

}
