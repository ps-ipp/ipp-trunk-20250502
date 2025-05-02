#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include <psmodules.h>
#include "tap.h"
#include "pstap.h"
/* STATUS:
    All functions are tested.
*/

#define CHIP_ALLOC_NAME        "ChipName"
#define CELL_ALLOC_NAME        "CellName"
#define MISC_NUM                32
#define MISC_NAME              "META00"
#define MISC_NAME2             "META01"
#define NUM_BIAS_DATA           10
#define TEST_NUM_ROWS           4
#define TEST_NUM_COLS           4
#define NUM_READOUTS            3
#define NUM_CELLS               10
#define NUM_CHIPS               8
#define NUM_HDUS                5
#define BASE_IMAGE              10
#define BASE_MASK               40
#define BASE_WEIGHT             70
#define VERBOSE                 0
#define ERR_TRACE_LEVEL         0
char *fitsFilename = "tmp.fits";

psPlaneTransform *PS_CREATE_2D_IDENTITY_PLANE_TRANSFORM()
{
    psPlaneTransform *pt = psPlaneTransformAlloc(1, 1);
    pt->x->coeff[1][0] = 1.0;
    pt->y->coeff[0][1] = 1.0;
    return(pt);
}

psPlaneDistort *PS_CREATE_4D_IDENTITY_PLANE_DISTORT()
{
    psPlaneDistort *pd = psPlaneDistortAlloc(1, 1, 1, 1);
    pd->x->coeff[1][0][0][0] = 1.0;
    pd->y->coeff[0][1][0][0] = 1.0;
    return(pd);
}

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

/******************************************************************************
generateSimpleCell(): This function generates a pmCell data structure and then
populates its members with real data.
 *****************************************************************************/
pmCell *generateSimpleCell(pmChip *chip, int cellID)
{
    pmCell *cell = pmCellAlloc(chip, CELL_ALLOC_NAME);

    psMetadataAddS32(cell->analysis, PS_LIST_HEAD, MISC_NAME, 0, NULL, MISC_NUM);
    psMetadataAddS32(cell->concepts, PS_LIST_HEAD, MISC_NAME, 0, NULL, MISC_NUM);
    psArrayRealloc(cell->readouts, NUM_READOUTS);
    char extname[80];
    snprintf(extname,80, "ext-%d", cellID);
    cell->hdu = pmHDUAlloc(extname);
    for (int i = 0 ; i < NUM_READOUTS ; i++) {
        cell->readouts->data[i] = psMemDecrRefCounter((psPtr) generateSimpleReadout(cell));
    }

    // First try to read data from ../dataFiles, then try dataFiles.
    bool rc = pmConfigFileRead(&cell->hdu->format, "../dataFiles/camera0/format0.config", "Camera format 0");
    if (!rc) {
        rc = pmConfigFileRead(&cell->hdu->format, "dataFiles/camera0/format0.config", "Camera format 0");
        if (!rc) {
            diag("pmConfigFileRead() was unsuccessful (from generateSimpleCell())");
	}
    }

    cell->hdu->images = psArrayAlloc(NUM_HDUS);
    cell->hdu->masks = psArrayAlloc(NUM_HDUS);
    cell->hdu->variances = psArrayAlloc(NUM_HDUS);
    for (int k = 0 ; k < NUM_HDUS ; k++) {
        cell->hdu->images->data[k]  = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_F32);
        cell->hdu->masks->data[k]   = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_MASK);
        cell->hdu->variances->data[k] = psImageAlloc(TEST_NUM_COLS, TEST_NUM_ROWS, PS_TYPE_F32);
        psImageInit(cell->hdu->images->data[k], (float) (BASE_IMAGE+k));
        psImageInit(cell->hdu->masks->data[k], (psU8) (BASE_MASK+k));
        psImageInit(cell->hdu->variances->data[k], (float) (BASE_WEIGHT+k));
    }

    psRegion *region = psRegionAlloc(0.0, 0.0, 0.0, 0.0);
    // You shouldn't have to remove the key from the metadata.
    psMetadataRemoveKey(cell->concepts, "CELL.TRIMSEC");
    psMetadataAddPtr(cell->concepts, PS_LIST_TAIL|PS_META_REPLACE, "CELL.TRIMSEC", PS_DATA_REGION, "I am a region", region);
    psFree(region);
    return(cell);
}

/******************************************************************************
generateSimpleChip(): This function generates a pmChip data structure and then
populates its members with real data.
 *****************************************************************************/
pmChip *generateSimpleChip(pmFPA *fpa, int chipID)
{
    pmChip *chip = pmChipAlloc(fpa, CHIP_ALLOC_NAME);
    chip->toFPA = PS_CREATE_2D_IDENTITY_PLANE_TRANSFORM();
    chip->fromFPA = PS_CREATE_2D_IDENTITY_PLANE_TRANSFORM();
    psMetadataAddS32(chip->analysis, PS_LIST_HEAD, MISC_NAME, 0, NULL, MISC_NUM);
    psMetadataAddS32(chip->concepts, PS_LIST_HEAD, MISC_NAME, 0, NULL, MISC_NUM);

    if (0) {
        char extname[80];
        snprintf(extname,80, "ext-%d", chipID);
        chip->hdu = pmHDUAlloc(extname);
    }

    psArrayRealloc(chip->cells, NUM_CELLS);
    for (int i = 0 ; i < NUM_CELLS ; i++) {
        chip->cells->data[i] = psMemDecrRefCounter((psPtr) generateSimpleCell(chip, i));
    }
    return(chip);
}

/******************************************************************************
generateSimpleFPA(): This function generates a pmFPA data structure and then
populates its members with real data.
 *****************************************************************************/
pmFPA* generateSimpleFPA(psMetadata *camera)
{
    pmFPA* fpa = pmFPAAlloc(camera, NULL);
    fpa->fromTPA = PS_CREATE_2D_IDENTITY_PLANE_TRANSFORM();
    fpa->toTPA = PS_CREATE_2D_IDENTITY_PLANE_TRANSFORM();
    fpa->toSky = psProjectionAlloc(0.0,0.0,10.0,10.0,PS_PROJ_TAN);
    psMetadataAddS32(fpa->analysis, PS_LIST_HEAD, MISC_NAME, 0, NULL, MISC_NUM);
    if (camera != NULL) {
        psMetadataAddS32((psMetadata *) fpa->camera, PS_LIST_HEAD, MISC_NAME, 0, NULL, MISC_NUM);
    }
    psMetadataAddS32(fpa->concepts, PS_LIST_HEAD, MISC_NAME, 0, NULL, MISC_NUM);

    psArrayRealloc(fpa->chips, NUM_CHIPS);
    for (int i = 0 ; i < NUM_CHIPS ; i++) {
        fpa->chips->data[i] = psMemDecrRefCounter((psPtr) generateSimpleChip(fpa, i));
    }
    pmConceptsBlankFPA(fpa);
    return(fpa);
}



psS32 main(psS32 argc, char* argv[])
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    psTraceSetLevel("err", ERR_TRACE_LEVEL);
    plan_tests(75);


    // ----------------------------------------------------------------------
    // pmCellReadHeader() tests: NULL input fits file
    {
        psMemId id = psMemGetId();
        psMetadata *camera = psMetadataAlloc();
        pmFPA *fpa = generateSimpleFPA(camera);
        pmChip *chip = fpa->chips->data[0];
        pmCell *cell = chip->cells->data[0];
        ok(fpa != NULL, "Allocated a pmFPA successfully");
        ok(chip != NULL, "Allocated a pmChip successfully");
        ok(cell != NULL, "Allocated a pmCell successfully");
        ok(!pmCellReadHeader(cell, NULL, NULL), "pmCellReadHeader(cell, NULL) returned FALSE");
        psFree(fpa);
        psFree(camera);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // pmCellReadHeader() tests: NULL input pmCell
    {
        psMemId id = psMemGetId();
        psFits* fitsFileW = psFitsOpen(fitsFilename, "w");
        ok(fitsFileW != NULL, "psFitsOpen() opened the FITS file");
        ok(!pmCellReadHeader(NULL, fitsFileW, NULL), "pmCellReadHeader(NULL, fitsFile) returned FALSE");
        psFree(fitsFileW);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // pmCellReadHeader() tests: acceptable data
    {
        psMemId id = psMemGetId();
        // Create a FITS file for this test
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
                         "String Item", extname);
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

        // Now, open that FITS file, and create an pmFPA hierarchy
        psFits* fitsFileR = psFitsOpen(fitsFilename, "r");
        ok(fitsFileR != NULL, "psFitsOpen returned non-NULL on existing file");
        psMetadata *camera = psMetadataAlloc();
        pmFPA *fpa = generateSimpleFPA(camera);
        pmChip *chip = fpa->chips->data[0];
        pmCell *cell = chip->cells->data[0];
        ok(fpa != NULL, "Allocated a pmFPA successfully");
        ok(chip != NULL, "Allocated a pmChip successfully");
        ok(cell != NULL, "Allocated a pmCell successfully");

        ok(pmCellReadHeader(cell, fitsFileR, NULL), "pmCellReadHeader() returned TRUE with acceptable data");

        // XXX: It's not clear if we should test if the HDU and pmConcepts actually
        // get rid, since pmCellReadHeader() simply calls functions that are tested
        // elsewhere.  However, if we should test it, test it here.

        psFree(fpa);
        psFree(camera);
        psFree(fitsFileR);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // pmChipReadHeader() tests: NULL input fits file
    {
        psMemId id = psMemGetId();
        psMetadata *camera = psMetadataAlloc();
        pmFPA *fpa = generateSimpleFPA(camera);
        pmChip *chip = fpa->chips->data[0];
        pmCell *cell = chip->cells->data[0];
        ok(fpa != NULL, "Allocated a pmFPA successfully");
        ok(chip != NULL, "Allocated a pmChip successfully");
        ok(cell != NULL, "Allocated a pmCell successfully");
        ok(!pmChipReadHeader(chip, NULL, NULL), "pmChipReadHeader(chip, NULL) returned FALSE");
        psFree(fpa);
        psFree(camera);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // pmChipReadHeader() tests: NULL input pmCell
    {
        psMemId id = psMemGetId();
        psFits* fitsFileW = psFitsOpen(fitsFilename, "w");
        ok(fitsFileW != NULL, "psFitsOpen() opened the FITS file");
        ok(!pmChipReadHeader(NULL, fitsFileW, NULL), "pmChipReadHeader(NULL, fitsFile) returned FALSE");
        psFree(fitsFileW);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // pmChipReadHeader() tests: acceptable data
    {
        psMemId id = psMemGetId();

        // Create a FITS file for this test
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
                         "String Item", extname);
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

        // Now, open that FITS file, and create an pmFPA hierarchy
        psFits* fitsFileR = psFitsOpen(fitsFilename, "r");
        ok(fitsFileR != NULL, "psFitsOpen returned non-NULL on existing file");
        psMetadata *camera = psMetadataAlloc();
        pmFPA *fpa = generateSimpleFPA(camera);
        pmChip *chip = fpa->chips->data[0];
        pmCell *cell = chip->cells->data[0];
        ok(fpa != NULL, "Allocated a pmFPA successfully");
        ok(chip != NULL, "Allocated a pmChip successfully");
        ok(cell != NULL, "Allocated a pmCell successfully");

        // Set the correct extension names for the chips (if we put this in the
        // generateChip() code, then psMOdules aborts.
        for (int chipID = 0 ; chipID < fpa->chips->n ; chipID++) {
            pmChip *chip = fpa->chips->data[chipID];
            char extname[80];
            snprintf(extname,80, "ext-%d", chipID);
            chip->hdu = pmHDUAlloc(extname);
	}

        ok(pmChipReadHeader(chip, fitsFileR, NULL), "pmChipReadHeader() returned TRUE with acceptable data");


        // XXX: It's not clear if we should test if the HDU and pmConcepts actually
        // get rid, since pmCellReadHeader() simply calls functions that are tested
        // elsewhere.  However, if we should test it, test it here.

        psFree(fpa);
        psFree(camera);
        psFree(fitsFileR);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // pmFPAReadHeader() tests: NULL input fits file
    {
        psMemId id = psMemGetId();
        psMetadata *camera = psMetadataAlloc();
        pmFPA *fpa = generateSimpleFPA(camera);
        pmChip *chip = fpa->chips->data[0];
        pmCell *cell = chip->cells->data[0];
        ok(fpa != NULL, "Allocated a pmFPA successfully");
        ok(chip != NULL, "Allocated a pmChip successfully");
        ok(cell != NULL, "Allocated a pmCell successfully");
        ok(!pmFPAReadHeader(fpa, NULL, NULL), "pmFPAReadHeader(fpa, NULL) returned FALSE");
        psFree(fpa);
        psFree(camera);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // pmFPAReadHeader() tests: NULL input pmCell
    {
        psMemId id = psMemGetId();
        psFits* fitsFileW = psFitsOpen(fitsFilename, "w");
        ok(fitsFileW != NULL, "psFitsOpen() opened the FITS file");
        ok(!pmFPAReadHeader(NULL, fitsFileW, NULL), "pmFPAReadHeader(NULL, fitsFile) returned FALSE");
        psFree(fitsFileW);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // pmFPAReadHeader() tests: acceptable data
    {
        psMemId id = psMemGetId();

        // Create a FITS file for this test
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
                         "String Item", extname);
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

        // Now, open that FITS file, and create an pmFPA hierarchy
        psFits* fitsFileR = psFitsOpen(fitsFilename, "r");
        ok(fitsFileR != NULL, "psFitsOpen returned non-NULL on existing file");
        psMetadata *camera = psMetadataAlloc();
        pmFPA *fpa = generateSimpleFPA(camera);
        pmChip *chip = fpa->chips->data[0];
        pmCell *cell = chip->cells->data[0];
        ok(fpa != NULL, "Allocated a pmFPA successfully");
        ok(chip != NULL, "Allocated a pmChip successfully");
        ok(cell != NULL, "Allocated a pmCell successfully");

        // Set the correct extension names for the chips (if we put this in the
        // generateChip() code, then psMOdules aborts.
        for (int chipID = 0 ; chipID < fpa->chips->n ; chipID++) {
            pmChip *chip = fpa->chips->data[chipID];
            char extname[80];
            snprintf(extname,80, "ext-%d", chipID);
            chip->hdu = pmHDUAlloc(extname);
	}
        snprintf(extname,80, "ext-%d", 0);
        fpa->hdu = pmHDUAlloc(extname);

        ok(pmFPAReadHeader(fpa, fitsFileR, NULL), "pmFPAReadHeader() returned TRUE with acceptable data");

        // XXX: It's not clear if we should test if the HDU and pmConcepts actually
        // get rid, since pmCellReadHeader() simply calls functions that are tested
        // elsewhere.  However, if we should test it, test it here.

        psFree(fpa);
        psFree(camera);
        psFree(fitsFileR);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
}

