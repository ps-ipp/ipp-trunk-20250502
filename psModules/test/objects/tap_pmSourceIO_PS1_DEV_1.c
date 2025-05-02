#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include <psmodules.h>
#include "tap.h"
#include "pstap.h"
/* STATUS:
    No test for pmSourcesWrite_PS1_DEV_1_XSRC() since there is no associated
        read function.
    All other functions are tested.
    XX: These tests read/write a file.  Must choose a more unique name.
*/

#define MISC_NUM                32
#define MISC_NAME              "META00"
#define NUM_BIAS_DATA           10
#define TEST_NUM_ROWS           (8)
#define TEST_NUM_COLS           (16)
#define VERBOSE                 0
#define ERR_TRACE_LEVEL         0
#define TEST_FLOATS_EQUAL(X, Y) (abs(X - Y) < 0.0001)
#define NUM_SOURCES		5
#define TABLE_FILENAME	"table.fits"
const char* tableFilename = "table.fits";

int main(int argc, char* argv[])
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    psTraceSetLevel("err", ERR_TRACE_LEVEL);
    plan_tests(79);


    // ----------------------------------------------------------------------
    // pmSourcesWrite_PS1_DEV_1() tests
    // Call pmSourcesWrite_PS1_DEV_1() with NULL psFits input parameter
    {
        psMemId id = psMemGetId();
        psFits* fitsFile = psFitsOpen(TABLE_FILENAME, "w");
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
        bool rc = pmSourcesWrite_PS1_DEV_1(NULL, sources, imageHeader, tableHeader, extname);
        ok(rc == false, "pmSourcesWrite_PS1_DEV_1() returned FALSE with NULL psFits input parameter");
        psFree(fitsFile);
        psFree(sources);
        psFree(imageHeader);
        psFree(tableHeader);
        psFree(extname);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmSourcesWrite_PS1_DEV_1() with NULL pmSource input parameter
    {
        psMemId id = psMemGetId();
        psFits* fitsFile = psFitsOpen(TABLE_FILENAME, "w");
        psArray *sources = psArrayAlloc(NUM_SOURCES);
        for (int i = 0 ; i < sources->n ; i++) {
            pmSource *src = pmSourceAlloc();
            src->peak = pmPeakAlloc(1, 2, 3.0, PM_PEAK_LONE);
            sources->data[i] = (psPtr *) src;
	}
        psMetadata *imageHeader = psMetadataAlloc();
        psMetadata *tableHeader = psMetadataAlloc();
        psString extname = psStringCopy("ext");
        bool rc = pmSourcesWrite_PS1_DEV_1(fitsFile, NULL, imageHeader, tableHeader, extname);
        ok(rc == false, "pmSourcesWrite_PS1_DEV_1() returned FALSE with NULL pmSource input parameter");
        psFree(fitsFile);
        psFree(sources);
        psFree(imageHeader);
        psFree(tableHeader);
        psFree(extname);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmSourcesWrite_PS1_DEV_1() with NULL extname input parameter
    {
        psMemId id = psMemGetId();
        psFits* fitsFile = psFitsOpen(TABLE_FILENAME, "w");
        psArray *sources = psArrayAlloc(NUM_SOURCES);
        for (int i = 0 ; i < sources->n ; i++) {
            pmSource *src = pmSourceAlloc();
            src->peak = pmPeakAlloc(1, 2, 3.0, PM_PEAK_LONE);
            sources->data[i] = (psPtr *) src;
	}
        psMetadata *imageHeader = psMetadataAlloc();
        psMetadata *tableHeader = psMetadataAlloc();
        psString extname = psStringCopy("ext");
        bool rc = pmSourcesWrite_PS1_DEV_1(fitsFile, sources, imageHeader, tableHeader, NULL);
        ok(rc == false, "pmSourcesWrite_PS1_DEV_1() returned FALSE with NULL extname input parameter");
        psFree(fitsFile);
        psFree(sources);
        psFree(imageHeader);
        psFree(tableHeader);
        psFree(extname);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // pmSourcesRead_PS1_DEV_1() tests
    // Call pmSourcesRead_PS1_DEV_1() with NULL psFits input parameter
    {
        psMemId id = psMemGetId();
        psFits* fitsFile = psFitsOpen(TABLE_FILENAME, "r");
        psMetadata *header = psMetadataAlloc();
        psArray *array = pmSourcesRead_PS1_DEV_1(NULL, header);
        ok(array == NULL, "pmSourcesRead_PS1_DEV_1() returned NULL with NULL psFits input parameter");
        psFree(fitsFile);
        psFree(header);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmSourcesRead_PS1_DEV_1() with NULL header input parameter
    {
        psMemId id = psMemGetId();
        psFits* fitsFile = psFitsOpen(TABLE_FILENAME, "r");
        psMetadata *header = psMetadataAlloc();
        psArray *array = pmSourcesRead_PS1_DEV_1(fitsFile, NULL);
        ok(array == NULL, "pmSourcesRead_PS1_DEV_1() returned NULL with NULL header input parameter");
        psFree(fitsFile);
        psFree(header);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // Call pmSourcesWrite_PS1_DEV_1() with acceptable input parameters
    #define TEST_BASE_X_POS             10.0
    #define TEST_BASE_Y_POS             20.0
    #define TEST_BASE_X_ERR             30.0
    #define TEST_BASE_Y_ERR             40.0
    #define TEST_BASE_PSF_MAG		50.0
    #define TEST_BASE_ERR_MAG		60.0
    #define TEST_BASE_SKY               500.0
    #define TEST_BASE_SKY_ERR		80.0
    #define TEST_BASE_PIX_WEIGHT        90.0
    #define TEST_BASE_PEAK_FLUX		120.0
    #define TEST_BASE_PSF_PROB		150.0
    #define TEST_BASE_CR_N_SIGMA	160.0
    #define TEST_BASE_EXT_N_SIGMA       170.0   
    #define TEST_BASE_MODE		1

    {
        psMemId id = psMemGetId();
        psFits* fitsFile = psFitsOpen(TABLE_FILENAME, "w");
        if (fitsFile == NULL) {
            diag("ERROR: Could not create 'table' FITS file");
            return false;
        }

        if (1) {
            // make the PHU an image (per FITS standard, it must be)
            psImage* image = psImageAlloc(16, 16, PS_TYPE_F32);
            if (!psFitsWriteImage(fitsFile, NULL, image, 1, NULL)) {
                diag("ERROR: Could not write PHU image");
                return false;
            }
            psFree(image);
        }

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
            sources->data[i] = (psPtr *) src;
            src->psfMag = TEST_BASE_PSF_MAG + (float) i;
            src->psfMagErr = TEST_BASE_ERR_MAG + (float) i;
            src->sky = TEST_BASE_SKY + (float) i;
            src->skyErr = TEST_BASE_SKY_ERR + (float) i;
//            src->psfProb = TEST_BASE_PSF_PROB + (float) i;
            src->crNsigma = TEST_BASE_CR_N_SIGMA + (float) i;
            src->extNsigma = TEST_BASE_EXT_N_SIGMA + (float) i;
            src->mode = TEST_BASE_MODE + i;
            src->peak->SN = (float) (10 - i);
            sources->data[i] = (psPtr *) src;
	}
        psMetadata *imageHeader = psMetadataAlloc();
        psMetadata *tableHeader = psMetadataAlloc();
        psString extname = psStringCopy("ext");
        bool rc = pmSourcesWrite_PS1_DEV_1(fitsFile, sources, imageHeader, tableHeader, extname);
        ok(rc == true, "pmSourcesWrite_PS1_DEV_1() returned TRUE with acceptable input parameters");
        psFree(fitsFile);
        psFree(sources);
        psFree(imageHeader);
        psFree(tableHeader);
        psFree(extname);
        pmModelClassCleanup();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Call pmSourcesRead_PS1_DEV_1() with acceptable input parameters
    {
        psMemId id = psMemGetId();
        psFits* fitsFile = psFitsOpen(tableFilename, "rw");
        if (fitsFile == NULL) {
            diag("ERROR: Could not create 'table' FITS file");
            return false;
        }

        // XXX: I'm not exactly sure why, but without this, the psFitsReadTableSize() call
        // in pmSourcesRead_PS1_DEV_1() fails.  However, Robert also did this in psFits.
        if (1) {
            psFitsMoveExtNum(fitsFile, 1, false);
	}

        // XX: Debugging purposes only.  Trying to duplicate the call to
        // psFitsTableRead() from DEV_0
        if (0) {
            psArray *table = psFitsReadTable(fitsFile);
            if (table == NULL) {
                printf("ERROR: table is NULL\n");
                exit(0);
	    }
            for (int i = 0; i < table->n; i++) {
                psMetadata *row = table->data[i];
                float sky = psMetadataLookupF32(NULL, row, "SKY");
                printf("For row %d, the psFitsReadTable() produces a sky of %.2f\n", i, sky);
	    }
	}

        psMetadata *header = psMetadataAlloc();
        psArray *array = pmSourcesRead_PS1_DEV_1(fitsFile, header);
        ok(array != NULL, "pmSourcesRead_PS1_DEV_1() returned non-NULL with acceptable input parameters");
        skip_start(array == NULL, 1, "Skipping tests because pmSourcesRead_PS1_DEV_1() returned NULL");
        for (int i = 0 ; i < array->n ; i++) {
             pmSource *src = (pmSource *) array->data[i];
             ok(src != NULL && psMemCheckSource(src), "pmSourcesRead_PS1_DEV_1() read source %d correctly", i);

             // XXX: Source code always sets the type to PM_SOURCE_TYPE_STAR.  Is that right?
             ok(src->type == PM_SOURCE_TYPE_STAR, "pmSourcesRead_PS1_DEV_1() set the source type correctly (is %d, should be %d)",
                src->type, PM_SOURCE_TYPE_STAR);

             ok(src->sky == (TEST_BASE_SKY + (float) i), "pmSourcesRead_PS1_DEV_1() set src->sky correctly (is %.2f, should be %.2f)",
                src->sky, (TEST_BASE_SKY + (float) i));
             ok(src->skyErr == (TEST_BASE_SKY_ERR + (float) i), "pmSourcesRead_PS1_DEV_1() set src->skyErr correctly (is %.2f, should be %.2f)",
                src->skyErr, (TEST_BASE_SKY_ERR + (float) i));
             ok(src->pixWeight == (TEST_BASE_PIX_WEIGHT + (float) i), "pmSourcesRead_PS1_DEV_1() set src->pixWeight correctly (is %.2f, should be %.2f)",
                src->pixWeight, (TEST_BASE_PIX_WEIGHT + (float) i));
             ok(TEST_FLOATS_EQUAL(src->peak->flux, (TEST_BASE_PEAK_FLUX + (float) i)), "pmSourcesRead_PS1_DEV_1() set src->peak->flux correctly (is %.2f, should be %.2f)",
                src->peak->flux, (TEST_BASE_PEAK_FLUX + (float) i));
             ok(src->psfMag == (TEST_BASE_PSF_MAG + (float) i), "pmSourcesRead_PS1_DEV_1() set src->psfMag correctly (is %.2f, should be %.2f)",
                src->psfMag, (TEST_BASE_PSF_MAG + (float) i));
             ok(src->psfMagErr == (TEST_BASE_ERR_MAG + (float) i), "pmSourcesRead_PS1_DEV_1() set src->psfMagErr correctly (is %.2f, should be %.2f)",
                src->psfMagErr, (TEST_BASE_ERR_MAG + (float) i));

             // XXX: Source code always sets src->modelPSF.  Is that right?
             pmModel *model = src->modelPSF;
             ok(model != NULL  && psMemCheckModel(model), "pmSourcesRead_PS1_DEV_1() set src->modelPSF correctly");
             skip_start(model == NULL, 2, "Skipping tests because pmSourcesRead_PS1_DEV_1() did not set src->modelPSF");
             ok(model->params->data.F32[PM_PAR_XPOS] == (TEST_BASE_X_POS + (float) i),
               "pmSourcesRead_PS1_DEV_1() set src->model->params->data.F32[PM_PAR_XPOS] correctly (is %.2f, should be %.2f)",
                model->params->data.F32[PM_PAR_XPOS], (TEST_BASE_X_POS + (float) i));
             ok(model->params->data.F32[PM_PAR_YPOS] == (TEST_BASE_Y_POS + (float) i),
               "pmSourcesRead_PS1_DEV_1() set src->model->params->data.F32[PM_PAR_YPOS] correctly (is %.2f, should be %.2f)",
                model->params->data.F32[PM_PAR_YPOS], (TEST_BASE_Y_POS + (float) i));
             ok(model->dparams->data.F32[PM_PAR_XPOS] == (TEST_BASE_X_ERR + (float) i),
               "pmSourcesRead_PS1_DEV_1() set src->model->dparams->data.F32[PM_PAR_XPOS] correctly (is %.2f, should be %.2f)",
                model->dparams->data.F32[PM_PAR_XPOS], (TEST_BASE_X_ERR + (float) i));
             ok(model->dparams->data.F32[PM_PAR_YPOS] == (TEST_BASE_Y_ERR + (float) i),
               "pmSourcesRead_PS1_DEV_1() set src->model->dparams->data.F32[PM_PAR_YPOS] correctly (is %.2f, should be %.2f)",
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
