#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include <psmodules.h>
#include "tap.h"
#include "pstap.h"
/* STATUS:
    All functions are tested.
*/

#define VERBOSE                 0
#define ERR_TRACE_LEVEL         0
#define TEST_FLOATS_EQUAL(X, Y) (abs(X - Y) < 0.0001)
#define NUM_SOURCES		5
int main(int argc, char* argv[])
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    psTraceSetLevel("err", ERR_TRACE_LEVEL);
    plan_tests(69);


    // ----------------------------------------------------------------------
    // pmSourcesWrite_SMPDATA() tests
    // Call pmSourcesWrite_SMPDATA() with NULL psFits input parameter
    {
        psMemId id = psMemGetId();
        psFits* fitsFile = psFitsOpen(".tmp00", "w");
        psArray *sources = psArrayAlloc(NUM_SOURCES);
        for (int i = 0 ; i < sources->n ; i++) {
            pmSource *src = pmSourceAlloc();
            src->peak = pmPeakAlloc(1, 2, 3.0, PM_PEAK_LONE);
            src->type = PM_SOURCE_TYPE_STAR;
            sources->data[i] = (psPtr *) src;
	}
        psMetadata *imageHeader = psMetadataAlloc();
        psMetadata *tableHeader = psMetadataAlloc();
        psString extname = psStringCopy("ext");
        bool rc = pmSourcesWrite_SMPDATA(NULL, sources, imageHeader, tableHeader, extname);
        ok(rc == false, "pmSourcesWrite_SMPDATA() returned FALSE with NULL psFits input parameter");
        psFree(fitsFile);
        psFree(sources);
        psFree(imageHeader);
        psFree(tableHeader);
        psFree(extname);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmSourcesWrite_SMPDATA() with NULL pmSource input parameter
    {
        psMemId id = psMemGetId();
        psFits* fitsFile = psFitsOpen(".tmp00", "w");
        psArray *sources = psArrayAlloc(NUM_SOURCES);
        for (int i = 0 ; i < sources->n ; i++) {
            pmSource *src = pmSourceAlloc();
            src->peak = pmPeakAlloc(1, 2, 3.0, PM_PEAK_LONE);
            sources->data[i] = (psPtr *) src;
	}
        psMetadata *imageHeader = psMetadataAlloc();
        psMetadata *tableHeader = psMetadataAlloc();
        psString extname = psStringCopy("ext");
        bool rc = pmSourcesWrite_SMPDATA(fitsFile, NULL, imageHeader, tableHeader, extname);
        ok(rc == false, "pmSourcesWrite_SMPDATA() returned FALSE with NULL pmSource input parameter");
        psFree(fitsFile);
        psFree(sources);
        psFree(imageHeader);
        psFree(tableHeader);
        psFree(extname);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmSourcesWrite_SMPDATA() with NULL extname input parameter
    {
        psMemId id = psMemGetId();
        psFits* fitsFile = psFitsOpen(".tmp00", "w");
        psArray *sources = psArrayAlloc(NUM_SOURCES);
        for (int i = 0 ; i < sources->n ; i++) {
            pmSource *src = pmSourceAlloc();
            src->peak = pmPeakAlloc(1, 2, 3.0, PM_PEAK_LONE);
            sources->data[i] = (psPtr *) src;
	}
        psMetadata *imageHeader = psMetadataAlloc();
        psMetadata *tableHeader = psMetadataAlloc();
        psString extname = psStringCopy("ext");
        bool rc = pmSourcesWrite_SMPDATA(fitsFile, sources, imageHeader, tableHeader, NULL);
        ok(rc == false, "pmSourcesWrite_SMPDATA() returned FALSE with NULL extname input parameter");
        psFree(fitsFile);
        psFree(sources);
        psFree(imageHeader);
        psFree(tableHeader);
        psFree(extname);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // pmSourcesRead_SMPDATA() tests
    // Call pmSourcesRead_SMPDATA() with NULL psFits input parameter
    {
        psMemId id = psMemGetId();
        psFits* fitsFile = psFitsOpen(".tmp00", "r");
        psMetadata *header = psMetadataAlloc();
        psArray *array = pmSourcesRead_SMPDATA(NULL, header);
        ok(array == NULL, "pmSourcesRead_SMPDATA() returned NULL with NULL psFits input parameter");
        psFree(fitsFile);
        psFree(header);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmSourcesRead_SMPDATA() with NULL header input parameter
    {
        psMemId id = psMemGetId();
        psFits* fitsFile = psFitsOpen(".tmp00", "r");
        psMetadata *header = psMetadataAlloc();
        psArray *array = pmSourcesRead_SMPDATA(fitsFile, NULL);
        ok(array == NULL, "pmSourcesRead_SMPDATA() returned NULL with NULL header input parameter");
        psFree(fitsFile);
        psFree(header);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // Call pmSourcesWrite_SMPDATA() with acceptable input parameters
    #define TEST_BASE_X_POS		10.0
    #define TEST_BASE_Y_POS		20.0
    #define TEST_BASE_X_ERR		30.0
    #define TEST_BASE_Y_ERR		40.0
    #define TEST_BASE_PSF_MAG		50.0
    #define TEST_BASE_ERR_MAG		60.0
    #define TEST_BASE_SKY		70.0
    #define TEST_BASE_SKY_ERR		80.0
    #define TEST_BASE_PIX_WEIGHT	90.0
    #define TEST_BASE_PEAK_FLUX		120.0
    #define TEST_BASE_EXT_MAG		150.0
    #define TEST_BASE_AP_MAG		160.0
    // XXX: The following metadata items are not tested: FWHM_X, FWHM_Y, THETA
    {
        psMemId id = psMemGetId();
        psFits* fitsFile = psFitsOpen(".tmp00", "w");
        psArray *sources = psArrayAlloc(NUM_SOURCES);
        for (int i = 0 ; i < sources->n ; i++) {
            pmSource *src = pmSourceAlloc();
            src->peak = pmPeakAlloc(1, 2, 3.0, PM_PEAK_LONE);
            // Set even numbered sources to PM_SOURCE_TYPE_STAR
            pmModel *model = NULL;
            if (i%2) {
                src->type = PM_SOURCE_TYPE_STAR;
                src->modelPSF = pmModelAlloc(1);
                model = src->modelPSF;
	    //} else {
            //    src->type = PM_SOURCE_TYPE_EXTENDED;
            //    src->modelConv = pmModelAlloc(1);
            //    model = src->modelConv;
 	    //}
	    }
            for (int p = 0 ; p < model->params->n ; p++) {
                model->params->data.F32[p] = (float) (i + p);
                model->dparams->data.F32[p] = (float) (i + p);
	    }
            model->params->data.F32[PM_PAR_XPOS] = TEST_BASE_X_POS + (float) i;
            model->params->data.F32[PM_PAR_YPOS] = TEST_BASE_Y_POS + (float) i;
            model->dparams->data.F32[PM_PAR_XPOS] = TEST_BASE_X_ERR + (float) i;
            model->dparams->data.F32[PM_PAR_YPOS] = TEST_BASE_Y_ERR + (float) i;
            src->psfMag = TEST_BASE_PSF_MAG + (float) i;
            src->psfMagErr = TEST_BASE_ERR_MAG + (float) i;
            src->peak->flux = TEST_BASE_PEAK_FLUX + (float) i;
            src->sky = TEST_BASE_SKY + (float) i;
            src->skyErr = TEST_BASE_SKY_ERR + (float) i;
            src->pixWeight = TEST_BASE_PIX_WEIGHT + (float) i;
            src->extMag = TEST_BASE_EXT_MAG + (float) i;
            src->apMag = TEST_BASE_AP_MAG + (float) i;
            sources->data[i] = (psPtr *) src;
	}
        psMetadata *imageHeader = psMetadataAlloc();
        psMetadata *tableHeader = psMetadataAlloc();
        psString extname = psStringCopy("ext");
        bool rc = pmSourcesWrite_SMPDATA(fitsFile, sources, imageHeader, tableHeader, extname);
        ok(rc == true, "pmSourcesWrite_SMPDATA() returned TRUE with acceptable input parameters");
        psFree(fitsFile);
        psFree(sources);
        psFree(imageHeader);
        psFree(tableHeader);
        psFree(extname);
        pmModelClassCleanup();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Call pmSourcesRead_SMPDATA() with acceptable input parameters
    {
        psMemId id = psMemGetId();
        psFits* fitsFile = psFitsOpen(".tmp00", "r");
        float ZERO_POINT = 25.0;
        psMetadata *header = psMetadataAlloc();
        psArray *array = pmSourcesRead_SMPDATA(fitsFile, header);
        ok(array != NULL, "pmSourcesRead_SMPDATA() returned non-NULL with acceptable input parameters");
        skip_start(array == NULL, 1, "Skipping tests because pmSourcesRead_SMPDATA() returned NULL");
        for (int i = 0 ; i < array->n ; i++) {
             pmSource *src = (pmSource *) array->data[i];
             ok(src != NULL && psMemCheckSource(src), "pmSourcesRead_SMPDATA() read source %d correctly", i);

             // src->model data
             pmModel *model = src->modelPSF;
             ok(model != NULL  && psMemCheckModel(model), "pmSourcesRead_SMPDATA() set src->modelPSF correctly");
             skip_start(model == NULL, 2, "Skipping tests because pmSourcesRead_SMPDATA() did not set src->modelPSF");
             {
                 ok(model->params->data.F32[PM_PAR_XPOS] == (TEST_BASE_X_POS + (float) i),
                   "pmSourcesRead_SMPDATA() set src->model->params->data.F32[PM_PAR_XPOS] correctly (is %.2f, should be %.2f)",
                    model->params->data.F32[PM_PAR_XPOS], (TEST_BASE_X_POS + (float) i));

                 ok(model->params->data.F32[PM_PAR_YPOS] == (TEST_BASE_Y_POS + (float) i),
                   "pmSourcesRead_SMPDATA() set src->model->params->data.F32[PM_PAR_YPOS] correctly (is %.2f, should be %.2f)",
                    model->params->data.F32[PM_PAR_YPOS], (TEST_BASE_Y_POS + (float) i));

                 float tmpSrcSky = TEST_BASE_SKY + (float) i;
                 float lsky = (tmpSrcSky < 1.0) ? 0.0 : log10(tmpSrcSky);
                 float tmpF = pow(10.0, lsky);
                 ok(model->params->data.F32[PM_PAR_SKY] == tmpF,
                   "pmSourcesRead_SMPDATA() set src->model->params->data.F32[PM_PAR_SKY] correctly (is %.2f, should be %.2f)",
                    model->params->data.F32[PM_PAR_SKY], tmpF);
	     }
             ok(src->psfMag == (TEST_BASE_PSF_MAG + (float) i), "pmSourcesRead_SMPDATA() set src->psfMag correctly (is %.2f, should be %.2f)",
                src->psfMag, (TEST_BASE_PSF_MAG + (float) i));
             float tmpF =  0.001 * PS_MIN(999, (1000 * (TEST_BASE_ERR_MAG + (float) i)));
             ok(src->psfMagErr == tmpF, "pmSourcesRead_SMPDATA() set src->psfMagErr correctly (is %.2f, should be %.2f)",
                src->psfMagErr, tmpF);
             tmpF = PS_MIN(99.0, (TEST_BASE_EXT_MAG + ZERO_POINT)) - ZERO_POINT;
             ok(src->extMag == tmpF, "pmSourcesRead_SMPDATA() set src->extMag correctly (is %.2f, should be %.2f)",
                src->extMag, tmpF);
             tmpF = PS_MIN(99.0, (TEST_BASE_AP_MAG + ZERO_POINT)) - ZERO_POINT;
             ok(src->apMag == tmpF, "pmSourcesRead_SMPDATA() set src->apMag correctly (is %.2f, should be %.2f)",
                src->apMag, tmpF);
             if (i%2) {
                 ok(src->type == PM_SOURCE_TYPE_STAR, "pmSourcesRead_SMPDATA() set the source type correctly (is %d, should be %d)",
                    src->type, PM_SOURCE_TYPE_STAR);
	     } else {
                 ok(src->type == PM_SOURCE_TYPE_EXTENDED, "pmSourcesRead_SMPDATA() set the source type correctly (is %d, should be %d)",
                    src->type, PM_SOURCE_TYPE_EXTENDED);
	     }
             psU8 tmpU8 = (psU8) PS_MIN(255, PS_MAX(0, (255*(TEST_BASE_PIX_WEIGHT + (float) i))));
             tmpF = (psF32) (tmpU8 / 255.0);
             ok(src->pixWeight == tmpF, "pmSourcesRead_SMPDATA() set src->pixWeight correctly (is %.2f, should be %.2f)",
                src->pixWeight, tmpF);

             skip_end();

             if (0) { // OLD
                 // XXX: Source code always sets the type to PM_SOURCE_TYPE_STAR.  Is that right?
                 ok(src->sky == (TEST_BASE_SKY + (float) i), "pmSourcesRead_SMPDATA() set src->sky correctly (is %.2f, should be %.2f)",
                    src->sky, (TEST_BASE_SKY + (float) i));
                 if (0) {
                     ok(src->pixWeight == (TEST_BASE_PIX_WEIGHT + (float) i), "pmSourcesRead_SMPDATA() set src->pixWeight correctly (is %.2f, should be %.2f)",
                        src->pixWeight, (TEST_BASE_PIX_WEIGHT + (float) i));
		 }
                 ok(TEST_FLOATS_EQUAL(src->peak->flux, 0.0), "pmSourcesRead_SMPDATA() set src->peak->flux correctly (is %.2f, should be %.2f)",
                    src->peak->flux, 0.0);
                 // XXX: Source code always sets src->modelPSF.  Is that right?
                 ok(model->dparams->data.F32[PM_PAR_XPOS] == (TEST_BASE_X_ERR + (float) i),
                   "pmSourcesRead_SMPDATA() set src->model->dparams->data.F32[PM_PAR_XPOS] correctly (is %.2f, should be %.2f)",
                    model->dparams->data.F32[PM_PAR_XPOS], (TEST_BASE_X_ERR + (float) i));
                 ok(model->dparams->data.F32[PM_PAR_YPOS] == (TEST_BASE_Y_ERR + (float) i),
                   "pmSourcesRead_SMPDATA() set src->model->dparams->data.F32[PM_PAR_YPOS] correctly (is %.2f, should be %.2f)",
                    model->dparams->data.F32[PM_PAR_YPOS], (TEST_BASE_Y_ERR + (float) i));
	     }
	}
        skip_end();
        psFree(fitsFile);
        psFree(header);
        psFree(array);
        pmModelClassCleanup();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
}

