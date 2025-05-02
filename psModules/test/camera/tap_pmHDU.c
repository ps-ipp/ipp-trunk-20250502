#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include <psmodules.h>
#include "tap.h"
#include "pstap.h"
/* STATUS:
    All functions are tested.
*/

#define NUM_HDUS 8
char *fitsFilename = "tmp.fits";

psS32 main(psS32 argc, char* argv[])
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    psTraceSetLevel("err", 0);
    plan_tests(159);


    // ----------------------------------------------------------------------
    // pmHDUAlloc() tests
    // Test pmHDUAlloc() with a NULL extname
    {
        psMemId id = psMemGetId();
        pmHDU *hdu = pmHDUAlloc(NULL);
        ok(hdu, "pmHDUAlloc(NULL) returned non-NULL");
        skip_start(!hdu, 7, "Skipping tests because pmHDUAlloc(NULL) returned NULL");
        ok(hdu->blankPHU == true, "pmHDUAlloc(NULL) set hdu->blankPHU correctly");
        ok(hdu->extname == NULL, "pmHDUAlloc(NULL) set hdu->extname correctly");
        ok(hdu->format == NULL, "pmHDUAlloc(NULL) set hdu->format correctly");
        ok(hdu->header == NULL, "pmHDUAlloc(NULL) set hdu->header correctly");
        ok(hdu->images == NULL, "pmHDUAlloc(NULL) set hdu->images correctly");
        ok(hdu->weights == NULL, "pmHDUAlloc(NULL) set hdu->weights correctly");
        ok(hdu->masks == NULL, "pmHDUAlloc(NULL) set hdu->masks correctly");
        psFree(hdu);
        skip_end();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Test pmHDUAlloc() with a non-NULL extname
    {
        psMemId id = psMemGetId();
        pmHDU *hdu = pmHDUAlloc("ext-0");
        ok(hdu, "pmHDUAlloc(extname) returned non-NULL");
        skip_start(!hdu, 7, "Skipping tests because pmHDUAlloc(extname) returned NULL");
        ok(hdu->blankPHU == false, "pmHDUAlloc(extname) set hdu->blankPHU correctly");
        ok(0 == strcmp(hdu->extname, "ext-0"), "pmHDUAlloc(extname) set hdu->extname correctly");
        ok(hdu->format == NULL, "pmHDUAlloc(extname) set hdu->format correctly");
        ok(hdu->header == NULL, "pmHDUAlloc(extname) set hdu->header correctly");
        ok(hdu->images == NULL, "pmHDUAlloc(extname) set hdu->images correctly");
        ok(hdu->weights == NULL, "pmHDUAlloc(extname) set hdu->weights correctly");
        ok(hdu->masks == NULL, "pmHDUAlloc(extname) set hdu->masks correctly");
        psFree(hdu);
        skip_end();
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // pmHDUWrite(), pmHDURead tests
    // Test pmHDUWrite() with NULL pmHDU input argument
    {
        psMemId id = psMemGetId();
        // Create simple FITS file
        psFits* fitsFile = psFitsOpen(".tmp00", "w");
        ok(fitsFile != NULL, "psFitsOpen() was successful");
        psMetadata* header = psMetadataAlloc();
        psImage* image = psImageAlloc(16, 16, PS_TYPE_F32);
        ok(psFitsWriteImage(fitsFile, header, image, 0, "extname"), "psFitsWriteImage() successful");
        psFree(header);
        psFree(image);
        bool rc = pmHDUWrite(NULL, fitsFile, NULL);
        ok(rc == false, "pmHDUWrite() returned FALSE with NULL psHDU as input");
        psFitsClose(fitsFile);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Test pmHDUWrite() with NULL psFits input argument
    {
        psMemId id = psMemGetId();
        pmHDU *hdu = pmHDUAlloc("extname");
        hdu->header = psMetadataAlloc();
        psMetadataAdd(hdu->header, PS_LIST_TAIL, "MYINT", PS_DATA_S32, "psS32 Item", 0);
        bool rc = pmConfigFileRead(&hdu->format, "../dataFiles/camera0/format0.config", "Camera 0 Config Format");
        if (!rc) {
            rc = pmConfigFileRead(&hdu->format, "dataFiles/camera0/format0.config", "Camera 0 Config Format");
	}
        ok(rc == true, "pmConfigFileRead() was successful");
        rc = pmHDUWrite(hdu, NULL, NULL);
        ok(rc == false, "pmHDUWrite() returned FALSE with NULL psFits file as input");
        psFree(hdu);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // Test pmHDURead() with NULL pmHDU input argument
    {
        psMemId id = psMemGetId();
        psFits* fitsFile = psFitsOpen("../dataFiles/sampleFitsFile.fits", "r");
        if (!fitsFile) {
            fitsFile = psFitsOpen("dataFiles/sampleFitsFile.fits", "r");
	}
        ok(fitsFile != NULL, "psFitsOpen() was successful");
        bool rc = pmHDURead(NULL, fitsFile);
        ok(rc == false, "pmHDURead() returned FALSE with NULL psHDU as input");
        psFitsClose(fitsFile);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Test pmHDURead() with NULL pmHDU input argument
    {
        psMemId id = psMemGetId();
        pmHDU *hdu = pmHDUAlloc("extname");
        hdu->header = psMetadataAlloc();
        bool rc = pmHDURead(hdu, NULL);
        ok(rc == false, "pmHDURead() returned FALSE with NULL psFits file as input");
        psFree(hdu);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // 1. Create a simple FITS file
    // 2. Create a simple pmHDU header
    // 3. Use pmHDUWrite() to write that header to the FITS file
    // 4. Close the FITS file
    // 5. Read that fits file from disk
    // 6. Create another pmHDU
    // 7. Call pmHDURead()which will read the FITS data into hdu->images
    // 8. Verify that hdu->images was set correctly.
    // 
    // XXX: Add code to delete .tmp00
    // XXX: Should we try with multiple images in the psArray hdu->images?
    #define BASE 20
    {
        psMemId id = psMemGetId();
        // 1. Create a simple FITS file
        psFits* fitsFileW = psFitsOpen(".tmp00", "w");
        ok(fitsFileW != NULL, "psFitsOpen() was successful");
        // 2. Create a simple pmHDU header
        //    Allocate and format hdu->header
        //    Set hdu->images
        pmHDU *hdu = pmHDUAlloc("extname");
        bool rc = pmConfigFileRead(&hdu->format, "../dataFiles/camera0/format0.config", "Camera format 0");
        if (!rc) {
            rc = pmConfigFileRead(&hdu->format, "dataFiles/camera0/format0.config", "Camera format 0");
	}
        ok(rc, "pmConfigFileRead() was successful");
        hdu->images = psArrayAlloc(NUM_HDUS);
        for (int k = 0 ; k < NUM_HDUS ; k++) {
            hdu->images->data[k] = psImageAlloc(4, 4, PS_TYPE_F32);
            psImageInit(hdu->images->data[k], (float) (BASE+k));
	}
        // 3. Use pmHDUWrite() to write that header to the FITS file
        rc = pmHDUWrite(hdu, fitsFileW, NULL);
        ok(rc == true, "pmHDUWrite() returned TRUE");
        // 4. Close the FITS file, free memory
        psFree(hdu);
        psFitsClose(fitsFileW);

        // Now, try to read that FITS file from disk.
        // 5. Read that fits file from disk
        psFits* fitsFileR = psFitsOpen(".tmp00", "r");
        ok(fitsFileR != NULL, "psFitsOpen() was successful");
        // 6. Create another pmHDU
        hdu = pmHDUAlloc("extname");
        hdu->images = NULL;
        // 7. Call pmHDURead()which will read the FITS data into hdu->images
        rc = pmHDURead(hdu, fitsFileR);
        ok(rc == true, "pmHDURead() returned TRUE");
        ok(hdu->images != NULL, "pmHDURead() created hdu->images");
        // 8. Verify that hdu->images was set correctly.
        bool errorFlag = false;
        for (int k = 0 ; k < NUM_HDUS ; k++) {
            psImage *img = (psImage *) hdu->images->data[k];
            for (psS32 i = 0 ; i < img->numRows ; i++) {
                for (psS32 j = 0 ; j < img->numCols ; j++) {
                    if (((float) k+BASE) != img->data.F32[i][j]) {
                        errorFlag = true;
                        diag("TEST ERROR (image %d): img[%d][%d] is %f, should be %f\n",
                              k, i, j, img->data.F32[i][j], ((float)k+BASE));
        	    }
            	}
	    }
	}
        ok(!errorFlag, "pmHDURead() correctly returned the image data");
        psFitsClose(fitsFileR);
        psFree(hdu);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // pmHDUWriteWeight(), pmHDUReadWeight() tests
    // Test pmHDUWriteWeight() with NULL pmHDU input argument
    {
        psMemId id = psMemGetId();
        // Create simple FITS file
        psFits* fitsFile = psFitsOpen(".tmp00", "w");
        ok(fitsFile != NULL, "psFitsOpen() was successful");
        psMetadata* header = psMetadataAlloc();
        psImage* image = psImageAlloc(16, 16, PS_TYPE_F32);
        ok(psFitsWriteImage(fitsFile, header, image, 0, "extname"), "psFitsWriteImage() successful");
        psFree(header);
        psFree(image);
        bool rc = pmHDUWriteWeight(NULL, fitsFile, NULL);
        ok(rc == false, "pmHDUWriteWeight() returned FALSE with NULL psHDU as input");
        psFitsClose(fitsFile);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Test pmHDUWriteWeight() with NULL psFits input argument
    {
        psMemId id = psMemGetId();
        pmHDU *hdu = pmHDUAlloc("extname");
        hdu->header = psMetadataAlloc();
        psMetadataAdd(hdu->header, PS_LIST_TAIL, "MYINT", PS_DATA_S32, "psS32 Item", 0);
        bool rc = pmConfigFileRead(&hdu->format, "../dataFiles/camera0/format0.config", "Camera 0 Config Format");
        if (!rc) {
            rc = pmConfigFileRead(&hdu->format, "dataFiles/camera0/format0.config", "Camera 0 Config Format");
	}
        ok(rc == true, "pmConfigFileRead() was successful");
        rc = pmHDUWriteWeight(hdu, NULL, NULL);
        ok(rc == false, "pmHDUWriteWeight    () returned FALSE with NULL psFits file as input");
        psFree(hdu);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Test pmHDUReadWeight() with NULL pmHDU input argument
    {
        psMemId id = psMemGetId();
        psFits* fitsFile = psFitsOpen("../dataFiles/sampleFitsFile.fits", "r");
        if (!fitsFile) {
            fitsFile = psFitsOpen("dataFiles/sampleFitsFile.fits", "r");
	}
        ok(fitsFile != NULL, "psFitsOpen() was successful");
        bool rc = pmHDUReadWeight(NULL, fitsFile);
        ok(rc == false, "pmHDUReadWeight() returned FALSE with NULL psHDU as input");
        psFitsClose(fitsFile);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Test pmHDUReadWeight() with NULL pmHDU input argument
    {
        psMemId id = psMemGetId();
        pmHDU *hdu = pmHDUAlloc("extname");
        hdu->header = psMetadataAlloc();
        bool rc = pmHDUReadWeight(hdu, NULL);
        ok(rc == false, "pmHDUReadWeight    () returned FALSE with NULL psFits file as input");
        psFree(hdu);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // 1. Create a simple FITS file
    // 2. Create a simple pmHDU header
    // 3. Use pmHDUWriteWeight() to write that header to the FITS file
    // 4. Close the FITS file
    // 5. Read that fits file from disk
    // 6. Create another pmHDU
    // 7. Call pmHDUReadWeight()which will read the FITS data into hdu->images
    // 8. Verify that hdu->images was set correctly.
    // 
    // XXX: Add code to delete .tmp00
    // XXX: Should we try with multiple images in the psArray hdu->images?
    #define BASE 20
    {
        psMemId id = psMemGetId();
        // 1. Create a simple FITS file
        psFits* fitsFileW = psFitsOpen(".tmp00", "w");
        ok(fitsFileW != NULL, "psFitsOpen() was successful");
        // 2. Create a simple pmHDU header
        //    Allocate and format hdu->header
        //    Set hdu->images
        pmHDU *hdu = pmHDUAlloc("extname");
        bool rc = pmConfigFileRead(&hdu->format, "../dataFiles/camera0/format0.config", "Camera format 0");
        if (!rc) {
            rc = pmConfigFileRead(&hdu->format, "dataFiles/camera0/format0.config", "Camera format 0");
	}
        ok(rc, "pmConfigFileRead() was successful");
        hdu->weights = psArrayAlloc(NUM_HDUS);
        for (int k = 0 ; k < NUM_HDUS ; k++) {
            hdu->weights->data[k] = psImageAlloc(4, 4, PS_TYPE_F32);
            psImageInit(hdu->weights->data[k], (float) (BASE+k));
	}
        // 3. Use pmHDUWriteWeight() to write that header to the FITS file
        rc = pmHDUWriteWeight(hdu, fitsFileW, NULL);
        ok(rc == true, "pmHDUWriteWeight() returned TRUE");
        // 4. Close the FITS file, free memory
        psFree(hdu);
        psFitsClose(fitsFileW);

        // Now, try to read that FITS file from disk.
        // 5. Read that fits file from disk
        psFits* fitsFileR = psFitsOpen(".tmp00", "r");
        ok(fitsFileR != NULL, "psFitsOpen() was successful");
        // 6. Create another pmHDU
        hdu = pmHDUAlloc("extname");
        hdu->weights = NULL;
        // 7. Call pmHDUReadWeight()which will read the FITS data into hdu->weights
        rc = pmHDUReadWeight(hdu, fitsFileR);
        ok(rc == true, "pmHDUReadWeight() returned TRUE");
        ok(hdu->weights != NULL, "pmHDUReadWeight() created hdu->weights");
        // 8. Verify that hdu->weights was set correctly.
        bool errorFlag = false;
        for (int k = 0 ; k < NUM_HDUS ; k++) {
            psImage *img = (psImage *) hdu->weights->data[k];
            for (psS32 i = 0 ; i < img->numRows ; i++) {
                for (psS32 j = 0 ; j < img->numCols ; j++) {
                    if (((float) k+BASE) != img->data.F32[i][j]) {
                        errorFlag = true;
                        diag("TEST ERROR (image %d): img[%d][%d] is %f, should be %f\n",
                              k, i, j, img->data.F32[i][j], ((float)k+BASE));
        	    }
            	}
	    }
	}
        ok(!errorFlag, "pmHDUReadWeight() correctly returned the image data");
        psFitsClose(fitsFileR);
        psFree(hdu);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // pmHDUWriteMask(), pmHDUReadMask() tests
    // Test pmHDUWriteMask() with NULL pmHDU input argument
    {
        psMemId id = psMemGetId();
        // Create simple FITS file
        psFits* fitsFile = psFitsOpen(".tmp00", "w");
        ok(fitsFile != NULL, "psFitsOpen() was successful");
        psMetadata* header = psMetadataAlloc();
        psImage* image = psImageAlloc(16, 16, PS_TYPE_F32);
        ok(psFitsWriteImage(fitsFile, header, image, 0, "extname"), "psFitsWriteImage() successful");
        psFree(header);
        psFree(image);
        bool rc = pmHDUWriteMask(NULL, fitsFile, NULL);
        ok(rc == false, "pmHDUWriteMask() returned FALSE with NULL psHDU as input");
        psFitsClose(fitsFile);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Test pmHDUWriteMask() with NULL psFits input argument
    {
        psMemId id = psMemGetId();
        pmHDU *hdu = pmHDUAlloc("extname");
        hdu->header = psMetadataAlloc();
        psMetadataAdd(hdu->header, PS_LIST_TAIL, "MYINT", PS_DATA_S32, "psS32 Item", 0);
        bool rc = pmConfigFileRead(&hdu->format, "dataFiles/camera0/format0.config", "Camera 0 Config Format");
        if (!rc) {
             pmConfigFileRead(&hdu->format, "../dataFiles/camera0/format0.config", "Camera 0 Config Format");
	}
        ok(rc == true, "pmConfigFileRead() was successful");
        rc = pmHDUWriteMask(hdu, NULL, NULL);
        ok(rc == false, "pmHDUWriteMask() returned FALSE with NULL psFits file as input");
        psFree(hdu);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Test pmHDUReadMask() with NULL pmHDU input argument
    {
        psMemId id = psMemGetId();
        psFits* fitsFile = psFitsOpen("../dataFiles/sampleFitsFile.fits", "r");
        if (!fitsFile) {
            fitsFile = psFitsOpen("dataFiles/sampleFitsFile.fits", "r");
	}
        ok(fitsFile != NULL, "psFitsOpen() was successful");
        bool rc = pmHDUReadMask(NULL, fitsFile);
        ok(rc == false, "pmHDUReadMask() returned FALSE with NULL psHDU as input");
        psFitsClose(fitsFile);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Test pmHDUReadMask() with NULL pmHDU input argument
    {
        psMemId id = psMemGetId();
        pmHDU *hdu = pmHDUAlloc("extname");
        hdu->header = psMetadataAlloc();
        bool rc = pmHDUReadMask(hdu, NULL);
        ok(rc == false, "pmHDUReadMask() returned FALSE with NULL psFits file as input");
        psFree(hdu);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // 1. Create a simple FITS file
    // 2. Create a simple pmHDU header
    // 3. Use pmHDUWriteMask() to write that header to the FITS file
    // 4. Close the FITS file
    // 5. Read that fits file from disk
    // 6. Create another pmHDU
    // 7. Call pmHDUReadMask()which will read the FITS data into hdu->images
    // 8. Verify that hdu->images was set correctly.
    // 
    // XXX: Add code to delete .tmp00
    // XXX: Should we try with multiple images in the psArray hdu->images?
    #define BASE 20
    {
        psMemId id = psMemGetId();
        // 1. Create a simple FITS file
        psFits* fitsFileW = psFitsOpen(".tmp00", "w");
        ok(fitsFileW != NULL, "psFitsOpen() was successful");
        // 2. Create a simple pmHDU header
        //    Allocate and format hdu->header
        //    Set hdu->images
        pmHDU *hdu = pmHDUAlloc("extname");
        bool rc = pmConfigFileRead(&hdu->format, "../dataFiles/camera0/format0.config", "Camera format 0");
        if (!rc) {
            rc = pmConfigFileRead(&hdu->format, "dataFiles/camera0/format0.config", "Camera format 0");
	}
        ok(rc, "pmConfigFileRead() was successful");
        hdu->masks = psArrayAlloc(NUM_HDUS);
        for (int k = 0 ; k < NUM_HDUS ; k++) {
            hdu->masks->data[k] = psImageAlloc(4, 4, PS_TYPE_F32);
            psImageInit(hdu->masks->data[k], (float) (BASE+k));
	}
        // 3. Use pmHDUWriteMask() to write that header to the FITS file
        rc = pmHDUWriteMask(hdu, fitsFileW, NULL);
        ok(rc == true, "pmHDUWriteMask() returned TRUE");
        // 4. Close the FITS file, free memory
        psFree(hdu);
        psFitsClose(fitsFileW);

        // Now, try to read that FITS file from disk.
        // 5. Read that fits file from disk
        psFits* fitsFileR = psFitsOpen(".tmp00", "r");
        ok(fitsFileR != NULL, "psFitsOpen() was successful");
        // 6. Create another pmHDU
        hdu = pmHDUAlloc("extname");
        hdu->masks = NULL;
        // 7. Call pmHDUReadMask()which will read the FITS data into hdu->masks
        rc = pmHDUReadMask(hdu, fitsFileR);
        ok(rc == true, "pmHDUReadMask() returned TRUE");
        ok(hdu->masks != NULL, "pmHDUReadMask() created hdu->masks");
        // 8. Verify that hdu->masks was set correctly.
        bool errorFlag = false;
        for (int k = 0 ; k < NUM_HDUS ; k++) {
            psImage *img = (psImage *) hdu->masks->data[k];
            for (psS32 i = 0 ; i < img->numRows ; i++) {
                for (psS32 j = 0 ; j < img->numCols ; j++) {
                    if (((float) k+BASE) != img->data.F32[i][j]) {
                        errorFlag = true;
                        diag("TEST ERROR (image %d): img[%d][%d] is %f, should be %f\n",
                              k, i, j, img->data.F32[i][j], ((float)k+BASE));
        	    }
            	}
	    }
	}
        ok(!errorFlag, "pmHDUReadMask() correctly returned the image data");
        psFitsClose(fitsFileR);
        psFree(hdu);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // pmHDUReadHeader(), pmHDUWriteHeader() tests
    // Test pmHDUReadHeader() with NULL pmHDU input argument
    {
        psMemId id = psMemGetId();
        psFits* fitsFile = psFitsOpen("../dataFiles/sampleFitsFile.fits", "r");
        if (!fitsFile) {
            fitsFile = psFitsOpen("dataFiles/sampleFitsFile.fits", "r");
	}
        ok(fitsFile != NULL, "psFitsOpen() was successful");
        bool rc = pmHDUReadHeader(NULL, fitsFile);
        ok(rc == false, "pmHDUReadHeader() returned FALSE with NULL psHDU as input");
        psFitsClose(fitsFile);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Test pmHDUReadHeader() with NULL psFits input argument
    {
        psMemId id = psMemGetId();
        pmHDU *hdu = pmHDUAlloc("extname");
        hdu->header = psMetadataAlloc();
        bool rc = pmHDUReadHeader(hdu, NULL);
        ok(rc == false, "pmHDUReadHeader() returned FALSE with NULL psFits file as input");
        psFree(hdu);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Test pmHDUReadHeader() and pmHDUWriteHeader()
    {
        psMemId id = psMemGetId();
        psFits* fitsFileW = psFitsOpen(fitsFilename, "w");
        ok(fitsFileW != NULL, "psFitsOpen() opened the FITS file");
        char extname[80];
        for (int lcv = 0; lcv < NUM_HDUS; lcv++) {
            snprintf(extname, 80, "ext-%d", lcv);
            pmHDU *hdu = pmHDUAlloc(extname);
            hdu->header = psMetadataAlloc();
            psMetadataAdd(hdu->header, PS_LIST_TAIL, "MYINT", PS_DATA_S32,
                         "psS32 Item", (psS32)lcv);
            psMetadataAdd(hdu->header, PS_LIST_TAIL, "MYFLT", PS_DATA_F32,
                         "psF32 Item", (float)(1.0f/(float)(1+lcv)));
            psMetadataAdd(hdu->header, PS_LIST_TAIL, "MYDBL", PS_DATA_F64,
                         "psF64 Item", (double)(1.0/(double)(1+lcv)));
            psMetadataAdd(hdu->header, PS_LIST_TAIL, "MYBOOL", PS_DATA_BOOL,
                         "psBool Item", (lcv%2 == 0));
            psMetadataAdd(hdu->header, PS_LIST_TAIL, "MYSTR", PS_DATA_STRING,
                         "psStr Item", extname);

            bool rc = pmConfigFileRead(&hdu->format, "../dataFiles/camera0/format0.config", "Camera 0 Config Format");
            if (!rc) {
                rc = pmConfigFileRead(&hdu->format, "dataFiles/camera0/format0.config", "Camera 0 Config Format");
	    }
            ok(rc == true, "pmConfigFileRead() was successful");
            rc = pmHDUWrite(hdu, fitsFileW, NULL);
            ok(rc == true, "pmHDUWrite() successfully wrote the header");
            psFree(hdu);
        }
        psFitsClose(fitsFileW);

        psFits* fitsFileR = psFitsOpen(fitsFilename, "r");
        ok(fitsFileR != NULL, "psFitsOpen returned non-NULL on existing file");
        int numHDUs = psFitsGetSize(fitsFileR);
        ok(numHDUs == NUM_HDUS, "The test FITS file has %d HDUs", numHDUs);

        for (int hdunum = 0; hdunum < NUM_HDUS; hdunum++)
        {
            char extname[80];
            snprintf(extname,80, "ext-%d", hdunum);
            psFitsMoveExtNum(fitsFileR, hdunum, false);
            pmHDU *hdu = pmHDUAlloc(extname);
            bool rc = pmHDUReadHeader(hdu, fitsFileR);
            ok(rc == true, "pmHDUReadHeader() returned TRUE");
            ok(hdu->header != NULL && psMemCheckMetadata(hdu->header),
              "pmHDUReadHeader() correctly returned the hdu->header member");

            psS32 intItem = psMetadataLookupS32(NULL, hdu->header, "MYINT");
            ok(intItem == hdunum, "Retrieved psS32 metadata item from file");

            psF32 fltItem = psMetadataLookupF32(NULL, hdu->header, "MYFLT");
            ok(fabsf(fltItem - 1.0f/(float)(1+hdunum)) <= FLT_EPSILON,
               "Retrieved psF32 metadata item from file.  Got %f vs %f",
               fltItem,1.0f/(float)(1+hdunum));

            psF64 dblItem = psMetadataLookupF64(NULL, hdu->header, "MYDBL");
            ok(abs(dblItem - 1.0/(double)(1+hdunum)) <= DBL_EPSILON,
               "Retrieved psF64 metadata item from file.  Got %g vs %g",
               dblItem, 1.0/(double)(1+hdunum));

            psMetadataItem* boolItem = psMetadataLookup(hdu->header, "MYBOOL");
            ok(boolItem != NULL && boolItem->type == PS_DATA_BOOL,
               "Retrieved psBool metadata item from file");

            psString strItem = psMetadataLookupStr(NULL, hdu->header, "MYSTR");
            ok(strItem != NULL && strncmp(strItem,extname,strlen(extname)) == 0,
               "Retrieved string metadata item from file.  Got '%s' vs '%s' (%d)",
               strItem,extname,strlen(extname));

            psFree(hdu);
        }
        psFitsClose(fitsFileR);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
}
