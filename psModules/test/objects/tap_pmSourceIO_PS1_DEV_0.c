#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include <psmodules.h>
#include "tap.h"
#include "pstap.h"
/* STATUS:
    All functions are tested.
    XX: These tests read/write a file.  Must choose a more unique name.
*/

#define VERBOSE                 0
#define ERR_TRACE_LEVEL         0
#define TEST_FLOATS_EQUAL(X, Y) (abs(X - Y) < 0.0001)
#define NUM_SOURCES		5
#define FITS_FILENAME  ".tmp00"
int main(int argc, char* argv[])
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    psTraceSetLevel("err", ERR_TRACE_LEVEL);
    plan_tests(79);


    // ----------------------------------------------------------------------
    // pmSourcesWrite_PS1_DEV_0() tests
    // Call pmSourcesWrite_PS1_DEV_0() with NULL psFits input parameter
    {
        psMemId id = psMemGetId();
        psFits* fitsFile = psFitsOpen(FITS_FILENAME, "w");
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
        bool rc = pmSourcesWrite_PS1_DEV_0(NULL, sources, imageHeader, tableHeader, extname);
        ok(rc == false, "pmSourcesWrite_PS1_DEV_0() returned FALSE with NULL psFits input parameter");
        psFree(fitsFile);
        psFree(sources);
        psFree(imageHeader);
        psFree(tableHeader);
        psFree(extname);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmSourcesWrite_PS1_DEV_0() with NULL pmSource input parameter
    {
        psMemId id = psMemGetId();
        psFits* fitsFile = psFitsOpen(FITS_FILENAME, "w");
        psArray *sources = psArrayAlloc(NUM_SOURCES);
        for (int i = 0 ; i < sources->n ; i++) {
            pmSource *src = pmSourceAlloc();
            src->peak = pmPeakAlloc(1, 2, 3.0, PM_PEAK_LONE);
            sources->data[i] = (psPtr *) src;
	}
        psMetadata *imageHeader = psMetadataAlloc();
        psMetadata *tableHeader = psMetadataAlloc();
        psString extname = psStringCopy("ext");
        bool rc = pmSourcesWrite_PS1_DEV_0(fitsFile, NULL, imageHeader, tableHeader, extname);
        ok(rc == false, "pmSourcesWrite_PS1_DEV_0() returned FALSE with NULL pmSource input parameter");
        psFree(fitsFile);
        psFree(sources);
        psFree(imageHeader);
        psFree(tableHeader);
        psFree(extname);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmSourcesWrite_PS1_DEV_0() with NULL extname input parameter
    {
        psMemId id = psMemGetId();
        psFits* fitsFile = psFitsOpen(FITS_FILENAME, "w");
        psArray *sources = psArrayAlloc(NUM_SOURCES);
        for (int i = 0 ; i < sources->n ; i++) {
            pmSource *src = pmSourceAlloc();
            src->peak = pmPeakAlloc(1, 2, 3.0, PM_PEAK_LONE);
            sources->data[i] = (psPtr *) src;
	}
        psMetadata *imageHeader = psMetadataAlloc();
        psMetadata *tableHeader = psMetadataAlloc();
        psString extname = psStringCopy("ext");
        bool rc = pmSourcesWrite_PS1_DEV_0(fitsFile, sources, imageHeader, tableHeader, NULL);
        ok(rc == false, "pmSourcesWrite_PS1_DEV_0() returned FALSE with NULL extname input parameter");
        psFree(fitsFile);
        psFree(sources);
        psFree(imageHeader);
        psFree(tableHeader);
        psFree(extname);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // pmSourcesRead_PS1_DEV_0() tests
    // Call pmSourcesRead_PS1_DEV_0() with NULL psFits input parameter
    {
        psMemId id = psMemGetId();
        psFits* fitsFile = psFitsOpen(FITS_FILENAME, "r");
        psMetadata *header = psMetadataAlloc();
        psArray *array = pmSourcesRead_PS1_DEV_0(NULL, header);
        ok(array == NULL, "pmSourcesRead_PS1_DEV_0() returned NULL with NULL psFits input parameter");
        psFree(fitsFile);
        psFree(header);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmSourcesRead_PS1_DEV_0() with NULL header input parameter
    {
        psMemId id = psMemGetId();
        psFits* fitsFile = psFitsOpen(FITS_FILENAME, "r");
        psMetadata *header = psMetadataAlloc();
        psArray *array = pmSourcesRead_PS1_DEV_0(fitsFile, NULL);
        ok(array == NULL, "pmSourcesRead_PS1_DEV_0() returned NULL with NULL header input parameter");
        psFree(fitsFile);
        psFree(header);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // Call pmSourcesWrite_PS1_DEV_0() with acceptable input parameters
    #define TEST_BASE_X_POS		10.0
    #define TEST_BASE_Y_POS		20.0
    #define TEST_BASE_X_ERR		30.0
    #define TEST_BASE_Y_ERR		40.0
    #define TEST_BASE_PSF_MAG	50.0
    #define TEST_BASE_ERR_MAG	60.0
    #define TEST_BASE_SKY		70.0
    #define TEST_BASE_SKY_ERR	80.0
    #define TEST_BASE_PIX_WEIGHT	90.0
    #define TEST_BASE_PEAK_FLUX	120.0
    {
        psMemId id = psMemGetId();
        psFits* fitsFile = psFitsOpen(FITS_FILENAME, "w");
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
 	    //
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
            sources->data[i] = (psPtr *) src;
	}
        psMetadata *imageHeader = psMetadataAlloc();
        psMetadata *tableHeader = psMetadataAlloc();
        psString extname = psStringCopy("ext");
        bool rc = pmSourcesWrite_PS1_DEV_0(fitsFile, sources, imageHeader, tableHeader, extname);
        ok(rc == true, "pmSourcesWrite_PS1_DEV_0() returned TRUE with acceptable input parameters");
        psFree(fitsFile);
        psFree(sources);
        psFree(imageHeader);
        psFree(tableHeader);
        psFree(extname);
        pmModelClassCleanup();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmSourcesRead_PS1_DEV_0() with acceptable input parameters
    {
        psMemId id = psMemGetId();
        psFits* fitsFile = psFitsOpen(FITS_FILENAME, "r");
        psMetadata *header = psMetadataAlloc();
        psArray *array = pmSourcesRead_PS1_DEV_0(fitsFile, header);
        ok(array != NULL, "pmSourcesRead_PS1_DEV_0() returned non-NULL with acceptable input parameters");
        skip_start(array == NULL, 1, "Skipping tests because pmSourcesRead_PS1_DEV_0() returned NULL");
        for (int i = 0 ; i < array->n ; i++) {
             pmSource *src = (pmSource *) array->data[i];
             ok(src != NULL && psMemCheckSource(src), "pmSourcesRead_PS1_DEV_0() read source %d correctly", i);

             // XXX: Source code always sets the type to PM_SOURCE_TYPE_STAR.  Is that right?
             ok(src->type == PM_SOURCE_TYPE_STAR, "pmSourcesRead_PS1_DEV_0() set the source type correctly (is %d, should be %d)",
                src->type, PM_SOURCE_TYPE_STAR);

             ok(src->sky == (TEST_BASE_SKY + (float) i), "pmSourcesRead_PS1_DEV_0() set src->sky correctly (is %.2f, should be %.2f)",
                src->sky, (TEST_BASE_SKY + (float) i));
             ok(src->skyErr == (TEST_BASE_SKY_ERR + (float) i), "pmSourcesRead_PS1_DEV_0() set src->skyErr correctly (is %.2f, should be %.2f)",
                src->skyErr, (TEST_BASE_SKY_ERR + (float) i));
             ok(src->pixWeight == (TEST_BASE_PIX_WEIGHT + (float) i), "pmSourcesRead_PS1_DEV_0() set src->pixWeight correctly (is %.2f, should be %.2f)",
                src->pixWeight, (TEST_BASE_PIX_WEIGHT + (float) i));
             ok(TEST_FLOATS_EQUAL(src->peak->flux, (TEST_BASE_PEAK_FLUX + (float) i)), "pmSourcesRead_PS1_DEV_0() set src->peak->flux correctly (is %.2f, should be %.2f)",
                src->peak->flux, (TEST_BASE_PEAK_FLUX + (float) i));
             ok(src->psfMag == (TEST_BASE_PSF_MAG + (float) i), "pmSourcesRead_PS1_DEV_0() set src->psfMag correctly (is %.2f, should be %.2f)",
                src->psfMag, (TEST_BASE_PSF_MAG + (float) i));
             ok(src->psfMagErr == (TEST_BASE_ERR_MAG + (float) i), "pmSourcesRead_PS1_DEV_0() set src->psfMagErr correctly (is %.2f, should be %.2f)",
                src->psfMagErr, (TEST_BASE_ERR_MAG + (float) i));

             // XXX: Source code always sets src->modelPSF.  Is that right?
             pmModel *model = src->modelPSF;
             ok(model != NULL  && psMemCheckModel(model), "pmSourcesRead_PS1_DEV_0() set src->modelPSF correctly");
             skip_start(model == NULL, 2, "Skipping tests because pmSourcesRead_PS1_DEV_0() did not set src->modelPSF");
             ok(model->params->data.F32[PM_PAR_XPOS] == (TEST_BASE_X_POS + (float) i),
               "pmSourcesRead_PS1_DEV_0() set src->model->params->data.F32[PM_PAR_XPOS] correctly (is %.2f, should be %.2f)",
                model->params->data.F32[PM_PAR_XPOS], (TEST_BASE_X_POS + (float) i));
             ok(model->params->data.F32[PM_PAR_YPOS] == (TEST_BASE_Y_POS + (float) i),
               "pmSourcesRead_PS1_DEV_0() set src->model->params->data.F32[PM_PAR_YPOS] correctly (is %.2f, should be %.2f)",
                model->params->data.F32[PM_PAR_YPOS], (TEST_BASE_Y_POS + (float) i));
             ok(model->dparams->data.F32[PM_PAR_XPOS] == (TEST_BASE_X_ERR + (float) i),
               "pmSourcesRead_PS1_DEV_0() set src->model->dparams->data.F32[PM_PAR_XPOS] correctly (is %.2f, should be %.2f)",
                model->dparams->data.F32[PM_PAR_XPOS], (TEST_BASE_X_ERR + (float) i));
             ok(model->dparams->data.F32[PM_PAR_YPOS] == (TEST_BASE_Y_ERR + (float) i),
               "pmSourcesRead_PS1_DEV_0() set src->model->dparams->data.F32[PM_PAR_YPOS] correctly (is %.2f, should be %.2f)",
                model->dparams->data.F32[PM_PAR_YPOS], (TEST_BASE_Y_ERR + (float) i));
             skip_end();
	}
        skip_end();
        psFree(fitsFile);
        psFree(header);
        psFree(array);
        pmModelClassCleanup();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
}
