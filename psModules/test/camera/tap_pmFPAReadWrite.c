#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include <psmodules.h>
#include "tap.h"
#include "pstap.h"
// XXX: Use better name for the temporary FITS file
// XXX: The code to generate and free the FPA hierarchy was copied from
// tap-pmFPA.c.  EIther include it directly, or library, or something.
// Also, get rid of the manual free functions and use psFree() once
// it correctly frees child members
// XXX: For the genSimpleFPA() code, add IDs to each function so that
// the values set in each chip-?cell-?hdu-?image are unique

#define CHIP_ALLOC_NAME		"ChipName"
#define CELL_ALLOC_NAME		"CellName"
#define MISC_NUM		32
#define MISC_NAME		"META00"
#define MISC_NAME2		"META01"
#define NUM_BIAS_DATA		10
#define TEST_NUM_ROWS		4
#define TEST_NUM_COLS		4
#define NUM_READOUTS		3
#define NUM_CELLS		10
#define NUM_CHIPS		8
#define NUM_HDUS		5
#define BASE_IMAGE		10
#define BASE_MASK		40
#define BASE_WEIGHT		70
#define VERBOSE			0
#define ERR_TRACE_LEVEL		10

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
pmCell *generateSimpleCell(pmChip *chip)
{
    pmCell *cell = pmCellAlloc(chip, CELL_ALLOC_NAME);

    psMetadataAddS32(cell->analysis, PS_LIST_HEAD, MISC_NAME, 0, NULL, MISC_NUM);
    psMetadataAddS32(cell->concepts, PS_LIST_HEAD, MISC_NAME, 0, NULL, MISC_NUM);
    psArrayRealloc(cell->readouts, NUM_READOUTS);
    cell->hdu = pmHDUAlloc("cellExtName");
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

    //XXX: Should the region be set some other way?  Like through the various config files?
    psRegion *region = psRegionAlloc(0.0, 0.0, 0.0, 0.0);
    // You shouldn't have to remove the key from the metadata.  Find out how to simply change the key value.
    psMetadataRemoveKey(cell->concepts, "CELL.TRIMSEC");
    psMetadataAddPtr(cell->concepts, PS_LIST_TAIL|PS_META_REPLACE, "CELL.TRIMSEC", PS_DATA_REGION, "I am a region", region);
    psFree(region);
    return(cell);
}

/******************************************************************************
generateSimpleChip(): This function generates a pmChip data structure and then
populates its members with real data.
 *****************************************************************************/
pmChip *generateSimpleChip(pmFPA *fpa)
{
    pmChip *chip = pmChipAlloc(fpa, CHIP_ALLOC_NAME);
    chip->toFPA = PS_CREATE_2D_IDENTITY_PLANE_TRANSFORM();
    chip->fromFPA = PS_CREATE_2D_IDENTITY_PLANE_TRANSFORM();
    psMetadataAddS32(chip->analysis, PS_LIST_HEAD, MISC_NAME, 0, NULL, MISC_NUM);
    psMetadataAddS32(chip->concepts, PS_LIST_HEAD, MISC_NAME, 0, NULL, MISC_NUM);

    psArrayRealloc(chip->cells, NUM_CELLS);
    for (int i = 0 ; i < NUM_CELLS ; i++) {
        chip->cells->data[i] = psMemDecrRefCounter((psPtr) generateSimpleCell(chip));
    }

    // XXX: Add code to initialize chip pmConcepts


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
        fpa->chips->data[i] = psMemDecrRefCounter((psPtr) generateSimpleChip(fpa));
    }
    pmConceptsBlankFPA(fpa);
    return(fpa);
}


psS32 main(psS32 argc, char* argv[])
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    psTraceSetLevel("err", ERR_TRACE_LEVEL);
    plan_tests(22);

    // ----------------------------------------------------------------------
    // ----------------------------------------------------------------------
    // pmCellWrite(): tests
    // Verify pmCellWrite() with NULL pmCell arg
    if (0) {
        psMemId id = psMemGetId();
        psFits* fitsFileW = psFitsOpen(".tmp00", "w");
        ok(!pmCellWrite(NULL, fitsFileW, NULL, false), "pmCellWrite() returned FALSE with NULL pmCell input");
        psFitsClose(fitsFileW);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Verify pmCellWrite() with NULL pmCell arg
    // XXXX: Big problem: Without the next code, everything else fails.  Why?
    if (0) {
        psMemId id = psMemGetId();
        psFits* fitsFileW = psFitsOpen(".tmp00", "w");
        ok(!pmCellWrite(NULL, fitsFileW, NULL, false), "pmCellWrite() returned FALSE with NULL pmCell input");
        psFitsClose(fitsFileW);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Verify pmCellWrite() with NULL pmFits arg
    if (0) {
        psMemId id = psMemGetId();
        psMetadata *camera = psMetadataAlloc();
        pmFPA* fpa = generateSimpleFPA(camera);
        pmChip *chip = fpa->chips->data[0];
        pmCell *cell = chip->cells->data[0];
        ok(cell != NULL, "Allocated a pmCell successfully");
        ok(!pmCellWrite(cell, NULL, NULL, false), "pmCellWrite() returned FALSE with NULL psFits param");
        psFree(fpa);
        psFree(camera);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Verify pmCellWrite() with acceptable input params
    // We first write a FITS file with the pmCellWrite(), then we read it and verify.
    // First call pmCellWrite()
    {
        psMemId id = psMemGetId();
        psFits* fitsFileW = psFitsOpen(".tmp00", "w");
        // Generate the pmFPA heirarchy
        psMetadata *camera = psMetadataAlloc();
        pmFPA* fpa = generateSimpleFPA(camera);
        pmChip *chip = fpa->chips->data[0];
        pmCell *cell = chip->cells->data[0];
        ok(cell != NULL, "Allocated a pmCell successfully");

        //  Use pmCellWrite() to write image data to the FITS file
        bool rc = pmCellWrite(cell, fitsFileW, NULL, false);
        ok(rc, "pmCellWrite() returned TRUE");

        //  Close the FITS file, free memory
        psFitsClose(fitsFileW);
        psFree(fpa);
        psFree(camera);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // ----------------------------------------------------------------------
    // pmCellRead() tests 
    // Verify pmCellRead() with NULL pmCell param
    {
        psMemId id = psMemGetId();
        psFits* fitsFileR = psFitsOpen(".tmp00", "r");
        ok(!pmCellRead(NULL, fitsFileR, NULL), "pmCellRead() returned FALSE with NULL pmCell param");
        psFitsClose(fitsFileR);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Verify pmCellRead() with NULL pmFits param
    {
        psMemId id = psMemGetId();
        psMetadata *camera = psMetadataAlloc();
        pmFPA* fpa = generateSimpleFPA(camera);
        pmChip *chip = fpa->chips->data[0];
        pmCell *cell = chip->cells->data[0];
        ok(!pmCellRead(cell, NULL, NULL), "pmCellRead() returned FALSE with NULL pmFits param");
        psFree(fpa);
        psFree(camera);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Verify pmCellRead() with acceptable data (using the FITS file created above)
    {
        psMemId id = psMemGetId();
        bool rc;
        // Generate the pmFPA heirarchy
        psMetadata *camera = psMetadataAlloc();
        pmFPA* fpa = generateSimpleFPA(camera);
        pmChip *chip = fpa->chips->data[0];
        pmCell *cell = chip->cells->data[0];
        // Free the existing cell hdu image data (so we can verify that pmCellRead() actually reads the data
        psFree(cell->hdu->images);
        cell->hdu->images = NULL;
        psFits* fitsFileR = psFitsOpen(".tmp00", "r");

        rc = pmCellRead(cell, fitsFileR, NULL);
        ok(rc, "pmCellRead() returned TRUE");
        skip_start(!rc, 1, "Skipping tests because pmCellRead returned NULL");
        for (int k = 0 ; k < cell->hdu->images->n ; k++) {
            bool errorFlag = false;
            psImage *img = cell->hdu->images->data[k];
            for (int i = 0 ; i < img->numRows ; i++) {
                for (int j = 0 ; j < img->numCols ; j++) {
                    if (((float) (BASE_IMAGE+k)) != img->data.F32[i][j]) {
                        diag("TEST ERROR: img[%d][%d] is %.2f, should be %.2f\n", i, j,
                              img->data.F32[i][j], ((float) (BASE_IMAGE+k)));
                        errorFlag = true;
		    }
		}
	    }
            ok(!errorFlag, "pmCellWrite()/pmCellRead() properly set the image data (image %d)", k);
        }
        skip_end();
        psFitsClose(fitsFileR);
        psFree(fpa);
        psFree(camera);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // ----------------------------------------------------------------------
    // pmCellWriteVariance(): tests
    // Verify pmCellWriteVariance() with NULL pmCell arg
    {
        psMemId id = psMemGetId();
        psFits* fitsFileW = psFitsOpen(".tmp00", "w");
        ok(!pmCellWriteVariance(NULL, fitsFileW, NULL, false), "pmCellWriteVariance() returned FALSE with NULL pmCell input");
        psFitsClose(fitsFileW);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Verify pmCellWriteVariance() with NULL pmFits arg
    {
        psMemId id = psMemGetId();
        psMetadata *camera = psMetadataAlloc();
        pmFPA* fpa = generateSimpleFPA(camera);
        pmChip *chip = fpa->chips->data[0];
        pmCell *cell = chip->cells->data[0];
        ok(cell != NULL, "Allocated a pmCell successfully");
        ok(!pmCellWriteVariance(cell, NULL, NULL, false), "pmCellWriteVariance() returned FALSE with NULL psFits param");
        psFree(fpa);
        psFree(camera);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Verify pmCellWriteVariance() with acceptable input params
    // We first write a FITS file with the pmCellWriteVariance(), then we read it and verify.
    // First call pmCellWriteVariance()
    {
        psMemId id = psMemGetId();
        psFits* fitsFileW = psFitsOpen(".tmp00", "w");
        // Generate the pmFPA heirarchy
        psMetadata *camera = psMetadataAlloc();
        pmFPA* fpa = generateSimpleFPA(camera);
        pmChip *chip = fpa->chips->data[0];
        pmCell *cell = chip->cells->data[0];
        ok(cell != NULL, "Allocated a pmCell successfully");

        //  Use pmCellWriteVariance() to write weight data to the FITS file
        bool rc = pmCellWriteVariance(cell, fitsFileW, NULL, false);
        ok(rc, "pmCellWriteVariance() returned TRUE");

        //  Close the FITS file, free memory
        psFitsClose(fitsFileW);
        psFree(fpa);
        psFree(camera);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // pmCellReadVariance() tests 
    // Verify pmCellReadVariance() with NULL pmCell param
    {
        psMemId id = psMemGetId();
        psFits* fitsFileR = psFitsOpen(".tmp00", "r");
        ok(!pmCellReadVariance(NULL, fitsFileR, NULL), "pmCellReadVariance() returned FALSE with NULL pmCell param");
        psFitsClose(fitsFileR);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Verify pmCellReadVariance() with NULL pmFits param
    {
        psMemId id = psMemGetId();
        psMetadata *camera = psMetadataAlloc();
        pmFPA* fpa = generateSimpleFPA(camera);
        pmChip *chip = fpa->chips->data[0];
        pmCell *cell = chip->cells->data[0];
        ok(!pmCellReadVariance(cell, NULL, NULL), "pmCellReadVariance() returned FALSE with NULL pmFits param");
        psFree(fpa);
        psFree(camera);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Verify pmCellReadVariance() with acceptable data (using the FITS file created above)
    {
        psMemId id = psMemGetId();
        bool rc;
        // Generate the pmFPA heirarchy
        psMetadata *camera = psMetadataAlloc();
        pmFPA* fpa = generateSimpleFPA(camera);
        pmChip *chip = fpa->chips->data[0];
        pmCell *cell = chip->cells->data[0];
        // Free the existing cell hdu weight data (so we can verify that pmCellReadWeight() actually reads the data
        psFree(cell->hdu->variances);
        cell->hdu->variances = NULL;
        psFits* fitsFileR = psFitsOpen(".tmp00", "r");

        rc = pmCellReadVariance(cell, fitsFileR, NULL);
        ok(rc, "pmCellReadVariance() returned TRUE");
        for (int k = 0 ; k < cell->hdu->variances->n ; k++) {
            bool errorFlag = false;
            psImage *msk = cell->hdu->variances->data[k];
            for (int i = 0 ; i < msk->numRows ; i++) {
                for (int j = 0 ; j < msk->numCols ; j++) {
                    if (((float) (BASE_WEIGHT+k)) != msk->data.F32[i][j]) {
                        diag("TEST ERROR: msk[%d][%d] is %.2f, should be %.2f\n", i, j,
                              msk->data.F32[i][j], ((float) (BASE_WEIGHT+k)));
                        errorFlag = true;
		    }
		}
	    }
            ok(!errorFlag, "pmCellWriteVariance()/pmCellReadVariance() properly set the weight data (image %d)", k);
        }
        psFitsClose(fitsFileR);
        psFree(fpa);
        psFree(camera);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }



    // ----------------------------------------------------------------------
    // ----------------------------------------------------------------------
    // pmCellWriteMask(): tests
    // Verify pmCellWriteMask() with NULL pmCell arg
    {
        psMemId id = psMemGetId();
        psFits* fitsFileW = psFitsOpen(".tmp00", "w");
        ok(!pmCellWriteMask(NULL, fitsFileW, NULL, false), "pmCellWriteMask() returned FALSE with NULL pmCell input");
        psFitsClose(fitsFileW);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Verify pmCellWriteMask() with NULL pmFits arg
    {
        psMemId id = psMemGetId();
        psMetadata *camera = psMetadataAlloc();
        pmFPA* fpa = generateSimpleFPA(camera);
        pmChip *chip = fpa->chips->data[0];
        pmCell *cell = chip->cells->data[0];
        ok(cell != NULL, "Allocated a pmCell successfully");
        ok(!pmCellWriteMask(cell, NULL, NULL, false), "pmCellWriteMask() returned FALSE with NULL psFits param");
        psFree(fpa);
        psFree(camera);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Verify pmCellWriteMask() with acceptable input params
    // We first write a FITS file with the pmCellWriteMask(), then we read it and verify.
    // First call pmCellWriteMask()
    {
        psMemId id = psMemGetId();
        psFits* fitsFileW = psFitsOpen(".tmp00", "w");
        // Generate the pmFPA heirarchy
        psMetadata *camera = psMetadataAlloc();
        pmFPA* fpa = generateSimpleFPA(camera);
        pmChip *chip = fpa->chips->data[0];
        pmCell *cell = chip->cells->data[0];
        ok(cell != NULL, "Allocated a pmCell successfully");

        //  Use pmCellWriteMask() to write mask data to the FITS file
        bool rc = pmCellWriteMask(cell, fitsFileW, NULL, false);
        ok(rc, "pmCellWriteMask() returned TRUE");

        //  Close the FITS file, free memory
        psFitsClose(fitsFileW);
        psFree(fpa);
        psFree(camera);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // pmCellReadMask() tests 
    // Verify pmCellReadMask() with NULL pmCell param
    {
        psMemId id = psMemGetId();
        psFits* fitsFileR = psFitsOpen(".tmp00", "r");
        ok(!pmCellReadMask(NULL, fitsFileR, NULL), "pmCellReadMask() returned FALSE with NULL pmCell param");
        psFitsClose(fitsFileR);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Verify pmCellReadMask() with NULL pmFits param
    {
        psMemId id = psMemGetId();
        psMetadata *camera = psMetadataAlloc();
        pmFPA* fpa = generateSimpleFPA(camera);
        pmChip *chip = fpa->chips->data[0];
        pmCell *cell = chip->cells->data[0];
        ok(!pmCellReadMask(cell, NULL, NULL), "pmCellReadMask() returned FALSE with NULL pmFits param");
        psFree(fpa);
        psFree(camera);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Verify pmCellReadMask() with acceptable data (using the FITS file created above)
    {
        psMemId id = psMemGetId();
        bool rc;
        // Generate the pmFPA heirarchy
        psMetadata *camera = psMetadataAlloc();
        pmFPA* fpa = generateSimpleFPA(camera);
        pmChip *chip = fpa->chips->data[0];
        pmCell *cell = chip->cells->data[0];
        // Free the existing cell hdu mask data (so we can verify that pmCellReadMask() actually reads the data
        psFree(cell->hdu->masks);
        cell->hdu->masks = NULL;
        psFits* fitsFileR = psFitsOpen(".tmp00", "r");

        rc = pmCellReadMask(cell, fitsFileR, NULL);
        ok(rc, "pmCellReadMask() returned TRUE");
        for (int k = 0 ; k < cell->hdu->masks->n ; k++) {
            bool errorFlag = false;
            psImage *msk = cell->hdu->masks->data[k];
            for (int i = 0 ; i < msk->numRows ; i++) {
                for (int j = 0 ; j < msk->numCols ; j++) {
                    if (0) {
                        if (((float) (BASE_WEIGHT+k)) != msk->data.F32[i][j]) {
                            diag("TEST ERROR: msk[%d][%d] is %.2f, should be %.2f\n", i, j,
                                  msk->data.F32[i][j], ((float) (BASE_WEIGHT+k)));
                            errorFlag = true;
                        }
		    }
                    if (1) {
                        if (((BASE_MASK+k)) != msk->data.U8[i][j]) {
                            diag("TEST ERROR: msk[%d][%d] is %.2f, should be %.2f\n", i, j,
                                  msk->data.F32[i][j], ((float) (BASE_MASK+k)));
                            errorFlag = true;
                        }
		    }
    		}
	    }
            ok(!errorFlag, "pmCellWriteMask()/pmCellReadMask() properly set the mask data (image %d)", k);
        }
        psFitsClose(fitsFileR);
        psFree(fpa);
        psFree(camera);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }



    // ----------------------------------------------------------------------
    // ----------------------------------------------------------------------
    // pmChipWrite() tests
    // Verify pmChipWrite() with NULL pmChip param
    {
        psMemId id = psMemGetId();
        psFits* fitsFileW = psFitsOpen(".tmp01", "w");
        ok(!pmChipWrite(NULL, fitsFileW, NULL, false, true), "pmChipWrite() returned NULL with NULL pmChip param");
        psFitsClose(fitsFileW);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Verify pmChipWrite() with NULL pmFits param
    {
        psMemId id = psMemGetId();
        // Generate the pmFPA heirarchy
        psMetadata *camera = psMetadataAlloc();
        pmFPA* fpa = generateSimpleFPA(camera);
        pmChip *chip = fpa->chips->data[0];
        ok(chip != NULL, "Allocated a pmChip successfully");
        ok(!pmChipWrite(chip, NULL, NULL, false, true), "pmChipWrite() returned NULL with NULL pmFits param");
        psFree(fpa);
        psFree(camera);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Verify pmChipWrite() with acceptable data
    {
        psMemId id = psMemGetId();
        psFits* fitsFileW = psFitsOpen(".tmp01", "w");
        // Generate the pmFPA heirarchy
        psMetadata *camera = psMetadataAlloc();
        pmFPA* fpa = generateSimpleFPA(camera);
        pmChip *chip = fpa->chips->data[0];
        ok(chip != NULL, "Allocated a pmChip successfully");

        //  Use pmChipWrite() to write image data to the FITS file
        bool rc = pmChipWrite(chip, fitsFileW, NULL, false, true);
        ok(rc, "pmChipWrite() returned TRUE");

        //  Close the FITS file, free memory
        psFitsClose(fitsFileW);
        psFree(fpa);
        psFree(camera);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // pmChipRead() tests
    // Verify pmChipRead() with NULL pmChip param
    {
        psMemId id = psMemGetId();
        psFits* fitsFileR = psFitsOpen(".tmp00", "r");
        ok(!pmChipRead(NULL, fitsFileR, NULL), "pmChipRead() returned NULL with NULL pmChip param");
        psFitsClose(fitsFileR);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Verify pmChipRead() with NULL pmFits param
    {
        psMemId id = psMemGetId();
        psMetadata *camera = psMetadataAlloc();
        pmFPA* fpa = generateSimpleFPA(camera);
        pmChip *chip = fpa->chips->data[0];
        ok(!pmChipRead(chip, NULL, NULL), "pmChipRead() returned NULL with NULL pmFits param");
        psFree(fpa);
        psFree(camera);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Verify pmChipRead() with acceptable input data
    {
        psMemId id = psMemGetId();
        bool rc;
        // Generate the pmFPA heirarchy
        psMetadata *camera = psMetadataAlloc();
        pmFPA* fpa = generateSimpleFPA(camera);
        pmChip *chip = fpa->chips->data[0];
        // Free the cells for chip 0 so we can verify that pmChipRead() actually reads the data from file
        for (int chipID = 0 ; chipID < chip->cells->n ; chipID++) {
            pmCell *cell = (pmCell *) chip->cells->data[chipID];
            psFree(cell);
            cell = NULL;
	}

        psFits* fitsFileR = psFitsOpen(".tmp00", "r");
        rc = pmChipRead(chip, fitsFileR, NULL);
        ok(rc, "pmChipRead() returned TRUE");
        bool errorFlag = false;
        // XXX: chipID should be cellID
        for (int chipID = 0 ; chipID < chip->cells->n ; chipID++) {
            if (VERBOSE) diag("Reading cell %d\n", chipID);
            pmCell *cell = (pmCell *) chip->cells->data[chipID];
            for (int k = 0 ; k < cell->hdu->images->n ; k++) {
                if (VERBOSE) diag("NOTE: image %d\n", k);
                psImage *img = cell->hdu->images->data[k];
                for (int i = 0 ; i < img->numRows ; i++) {
                    for (int j = 0 ; j < img->numCols ; j++) {
                        if (((float) (BASE_IMAGE+k)) != img->data.F32[i][j]) {
                            diag("TEST ERROR: img[%d][%d] is %.2f, should be %.2f\n", i, j,
                                  img->data.F32[i][j], ((float) (BASE_IMAGE+k)));
                            errorFlag = true;
			}
		    }
		}
	    }
            ok(!errorFlag, "pmChipWrite()/pmChipRead() properly set the image data (cell %d)", chipID);
	}

        psFitsClose(fitsFileR);
        psFree(fpa);
        psFree(camera);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // pmChipWriteVariance() tests
    // Verify pmChipWriteVariance() with NULL pmChip param
    {
        psMemId id = psMemGetId();
        psFits* fitsFileW = psFitsOpen(".tmp01", "w");
        ok(!pmChipWriteVariance(NULL, fitsFileW, NULL, false, true), "pmChipWriteVariance() returned NULL with NULL pmChip param");
        psFitsClose(fitsFileW);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Verify pmChipWriteVariance() with NULL pmFits param
    {
        psMemId id = psMemGetId();
        // Generate the pmFPA heirarchy
        psMetadata *camera = psMetadataAlloc();
        pmFPA* fpa = generateSimpleFPA(camera);
        pmChip *chip = fpa->chips->data[0];
        ok(chip != NULL, "Allocated a pmChip successfully");
        ok(!pmChipWriteVariance(chip, NULL, NULL, false, true), "pmChipWriteVariance() returned NULL with NULL pmFits param");
        psFree(fpa);
        psFree(camera);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Verify pmChipWriteVariance() with acceptable data
    {
        psMemId id = psMemGetId();
        psFits* fitsFileW = psFitsOpen(".tmp01", "w");
        // Generate the pmFPA heirarchy
        psMetadata *camera = psMetadataAlloc();
        pmFPA* fpa = generateSimpleFPA(camera);
        pmChip *chip = fpa->chips->data[0];
        ok(chip != NULL, "Allocated a pmChip successfully");

        //  Use pmChipWriteVariance() to write image data to the FITS file
        bool rc = pmChipWriteVariance(chip, fitsFileW, NULL, false, true);
        ok(rc, "pmChipWriteVariance() returned TRUE");

        //  Close the FITS file, free memory
        psFitsClose(fitsFileW);
        psFree(fpa);
        psFree(camera);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // pmChipReadMask() tests
    // Verify pmChipReadMask() with NULL pmChip param
    {
        psMemId id = psMemGetId();
        psFits* fitsFileR = psFitsOpen(".tmp00", "r");
        ok(!pmChipReadMask(NULL, fitsFileR, NULL), "pmChipReadMask() returned NULL with NULL pmChip param");
        psFitsClose(fitsFileR);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Verify pmChipReadMask() with NULL pmFits param
    {
        psMemId id = psMemGetId();
        psMetadata *camera = psMetadataAlloc();
        pmFPA* fpa = generateSimpleFPA(camera);
        pmChip *chip = fpa->chips->data[0];
        ok(!pmChipReadMask(chip, NULL, NULL), "pmChipReadMask() returned NULL with NULL pmFits param");
        psFree(fpa);
        psFree(camera);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Verify pmChipReadMask() with acceptable input data
    {
        psMemId id = psMemGetId();
        bool rc;
        // Generate the pmFPA heirarchy
        psMetadata *camera = psMetadataAlloc();
        pmFPA* fpa = generateSimpleFPA(camera);
        pmChip *chip = fpa->chips->data[0];
        // Free the cells for chip 0 so we can verify that pmChipReadMask() actually reads the data from file
        for (int chipID = 0 ; chipID < chip->cells->n ; chipID++) {
            pmCell *cell = (pmCell *) chip->cells->data[chipID];
            psFree(cell);
            cell = NULL;
	}

        psFits* fitsFileR = psFitsOpen(".tmp00", "r");
        rc = pmChipReadMask(chip, fitsFileR, NULL);
        ok(rc, "pmChipReadMask() returned TRUE");
        bool errorFlag = false;
        // XXX: chipID should be cellID
        for (int chipID = 0 ; chipID < chip->cells->n ; chipID++) {
            if (VERBOSE) diag("Reading cell %d\n", chipID);
            pmCell *cell = (pmCell *) chip->cells->data[chipID];
            for (int k = 0 ; k < cell->hdu->variances->n ; k++) {
                if (VERBOSE) diag("NOTE: image %d\n", k);
                psImage *wgt = cell->hdu->variances->data[k];
                for (int i = 0 ; i < wgt->numRows ; i++) {
                    for (int j = 0 ; j < wgt->numCols ; j++) {
                        if (((float) (BASE_WEIGHT+k)) != wgt->data.F32[i][j]) {
                            diag("TEST ERROR: wgt[%d][%d] is %.2f, should be %.2f\n", i, j,
                                  wgt->data.F32[i][j], ((float) (BASE_WEIGHT+k)));
                            errorFlag = true;
			}
		    }
		}
	    }
            ok(!errorFlag, "pmChipWriteVariance()/pmChipReadVariance() properly set the variance data (cell %d)", chipID);
	}

        psFitsClose(fitsFileR);
        psFree(fpa);
        psFree(camera);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // pmChipReadMask() tests
    // Verify pmChipReadMask() with NULL pmChip param
    {
        psMemId id = psMemGetId();
        psFits* fitsFileR = psFitsOpen(".tmp00", "r");
        ok(!pmChipReadMask(NULL, fitsFileR, NULL), "pmChipReadMask() returned NULL with NULL pmChip param");
        psFitsClose(fitsFileR);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Verify pmChipReadMask() with NULL pmFits param
    {
        psMemId id = psMemGetId();
        psMetadata *camera = psMetadataAlloc();
        pmFPA* fpa = generateSimpleFPA(camera);
        pmChip *chip = fpa->chips->data[0];
        ok(!pmChipReadMask(chip, NULL, NULL), "pmChipReadMask() returned NULL with NULL pmFits param");
        psFree(fpa);
        psFree(camera);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Verify pmChipReadMask() with acceptable input data
    {
        psMemId id = psMemGetId();
        bool rc;
        // Generate the pmFPA heirarchy
        psMetadata *camera = psMetadataAlloc();
        pmFPA* fpa = generateSimpleFPA(camera);
        pmChip *chip = fpa->chips->data[0];
        // Free the cells for chip 0 so we can verify that pmChipReadMask() actually reads the data from file
        for (int chipID = 0 ; chipID < chip->cells->n ; chipID++) {
            pmCell *cell = (pmCell *) chip->cells->data[chipID];
            psFree(cell);
            cell = NULL;
	}

        psFits* fitsFileR = psFitsOpen(".tmp00", "r");
        rc = pmChipReadMask(chip, fitsFileR, NULL);
        ok(rc, "pmChipReadMask() returned TRUE");
        bool errorFlag = false;
        // XXX: chipID should be cellID
        for (int chipID = 0 ; chipID < chip->cells->n ; chipID++) {
            if (VERBOSE) diag("Reading cell %d\n", chipID);
            pmCell *cell = (pmCell *) chip->cells->data[chipID];
            for (int k = 0 ; k < cell->hdu->masks->n ; k++) {
                if (VERBOSE) diag("NOTE: image %d\n", k);
                psImage *msk = cell->hdu->masks->data[k];
                for (int i = 0 ; i < msk->numRows ; i++) {
                    for (int j = 0 ; j < msk->numCols ; j++) {
                        if (((BASE_MASK+k)) != msk->data.U8[i][j]) {
                            diag("TEST ERROR: msk[%d][%d] is %.2f, should be %.2f\n", i, j,
                                  msk->data.F32[i][j], ((float) (BASE_MASK+k)));
                            errorFlag = true;
			}
		    }
		}
	    }
            ok(!errorFlag, "pmChipWriteMask()/pmChipReadMask() properly set the mask data (cell %d)", chipID);
	}

        psFitsClose(fitsFileR);
        psFree(fpa);
        psFree(camera);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // ----------------------------------------------------------------------
    // pmFPAWrite() tests
    // pmFPAWrite(pmFPA *fpa, psFits *fits, psDB *db, bool blank, bool recurse)
    // Verify pmFPAWrite() with NULL pmFPA param
    {
        psMemId id = psMemGetId();
        psFits* fitsFileW = psFitsOpen(".tmp01", "w");
        ok(!pmFPAWrite(NULL, fitsFileW, NULL, false, true), "pmFPAWrite() returned NULL with NULL pmFPA param");
        psFitsClose(fitsFileW);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Verify pmFPAWrite() with NULL pmFits param
    {
        psMemId id = psMemGetId();
        // Generate the pmFPA heirarchy
        psMetadata *camera = psMetadataAlloc();
        pmFPA* fpa = generateSimpleFPA(camera);
        ok(fpa != NULL, "Allocated a pmFPA successfully");
        ok(!pmFPAWrite(fpa, NULL, NULL, false, true), "pmFPAWrite() returned NULL with NULL pmFits param");
        psFree(fpa);
        psFree(camera);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Verify pmFPAWrite() with acceptable data
    {
        psMemId id = psMemGetId();
        psFits* fitsFileW = psFitsOpen(".tmp01", "w");
        // Generate the pmFPA heirarchy
        psMetadata *camera = psMetadataAlloc();
        pmFPA* fpa = generateSimpleFPA(camera);
        ok(fpa != NULL, "Allocated a pmFPA successfully");
        bool rc = pmFPAWrite(fpa, fitsFileW, NULL, false, true);
        ok(rc, "pmFPAWrite() returned TRUE");
        //  Close the FITS file, free memory
        psFitsClose(fitsFileW);
        psFree(fpa);
        psFree(camera);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // pmFPARead() tests
    // Verify pmFPARead() with NULL pmFPA param
    {
        psMemId id = psMemGetId();
        psFits* fitsFileR = psFitsOpen(".tmp00", "r");
        ok(!pmFPARead(NULL, fitsFileR, NULL), "pmFPARead() returned NULL with NULL pmFPA param");
        psFitsClose(fitsFileR);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Verify pmFPARead() with NULL pmFits param
    {
        psMemId id = psMemGetId();
        psMetadata *camera = psMetadataAlloc();
        pmFPA* fpa = generateSimpleFPA(camera);
        ok(!pmFPARead(fpa, NULL, NULL), "pmFPARead() returned NULL with NULL pmFits param");
        psFree(fpa);
        psFree(camera);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Verify pmFPARead() with acceptable input data
    {
        psMemId id = psMemGetId();
        bool rc;
        // Generate the pmFPA heirarchy
        psMetadata *camera = psMetadataAlloc();
        pmFPA* fpa = generateSimpleFPA(camera);
        pmChip *chip = fpa->chips->data[0];
        // Free the cells for chip 0 so we can verify that pmFPARead() actually reads the data from file
        if (0) {
            for (int chipID = 0 ; chipID < chip->cells->n ; chipID++) {
                pmCell *cell = (pmCell *) chip->cells->data[chipID];
                psFree(cell);
                cell = NULL;
            }
	}

        psFits* fitsFileR = psFitsOpen(".tmp00", "r");
        rc = pmFPARead(fpa, fitsFileR, NULL);
        ok(rc, "pmFPARead() returned TRUE");
        bool errorFlag = false;
        // XXX: fpaID should be chipID
        // XXX: chipID should be cellID
        for (int fpaID = 0 ; fpaID < fpa->chips->n ; fpaID++) {
            pmChip *chip = fpa->chips->data[fpaID];
            for (int chipID = 0 ; chipID < chip->cells->n ; chipID++) {
                if (VERBOSE) diag("Reading cell %d\n", chipID);
                pmCell *cell = (pmCell *) chip->cells->data[chipID];
                for (int k = 0 ; k < cell->hdu->images->n ; k++) {
                    if (VERBOSE) diag("NOTE: image %d\n", k);
                    psImage *img = cell->hdu->images->data[k];
                    for (int i = 0 ; i < img->numRows ; i++) {
                        for (int j = 0 ; j < img->numCols ; j++) {
                            if (((float) (BASE_IMAGE+k)) != img->data.F32[i][j]) {
                                diag("TEST ERROR: img[%d][%d] is %.2f, should be %.2f\n", i, j,
                                      img->data.F32[i][j], ((float) (BASE_IMAGE+k)));
                                errorFlag = true;
        			}
        		    }
        		}
        	    }
                ok(!errorFlag, "pmFPAWrite()/pmFPARead() properly set the image data (chip %d, cell %d)", fpaID, chipID);
    	    }
	}

        psFitsClose(fitsFileR);
        psFree(fpa);
        psFree(camera);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // ----------------------------------------------------------------------
    // pmFPAWriteVariance() tests
    // pmFPAWriteVariance(pmFPA *fpa, psFits *fits, psDB *db, bool blank, bool recurse)
    // Verify pmFPAWriteVariance() with NULL pmFPA param
    {
        psMemId id = psMemGetId();
        psFits* fitsFileW = psFitsOpen(".tmp01", "w");
        ok(!pmFPAWriteVariance(NULL, fitsFileW, NULL, false, true), "pmFPAWriteVariance() returned NULL with NULL pmFPA param");
        psFitsClose(fitsFileW);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Verify pmFPAWriteVariance() with NULL pmFits param
    {
        psMemId id = psMemGetId();
        // Generate the pmFPA heirarchy
        psMetadata *camera = psMetadataAlloc();
        pmFPA* fpa = generateSimpleFPA(camera);
        ok(fpa != NULL, "Allocated a pmFPA successfully");
        ok(!pmFPAWriteVariance(fpa, NULL, NULL, false, true), "pmFPAWriteVariance() returned NULL with NULL pmFits param");
        psFree(fpa);
        psFree(camera);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Verify pmFPAWriteVariance() with acceptable data
    {
        psMemId id = psMemGetId();
        psFits* fitsFileW = psFitsOpen(".tmp01", "w");
        // Generate the pmFPA heirarchy
        psMetadata *camera = psMetadataAlloc();
        pmFPA* fpa = generateSimpleFPA(camera);
        ok(fpa != NULL, "Allocated a pmFPA successfully");
        bool rc = pmFPAWriteVariance(fpa, fitsFileW, NULL, false, true);
        ok(rc, "pmFPAWriteVariance() returned TRUE");
        //  Close the FITS file, free memory
        psFitsClose(fitsFileW);
        psFree(fpa);
        psFree(camera);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // pmFPAReadVariance() tests
    // Verify pmFPAReadWeight() with NULL pmFPA param
    {
        psMemId id = psMemGetId();
        psFits* fitsFileR = psFitsOpen(".tmp00", "r");
        ok(!pmFPARead(NULL, fitsFileR, NULL), "pmFPAReadVariance() returned NULL with NULL pmFPA param");
        psFitsClose(fitsFileR);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Verify pmFPAReadVariance() with NULL pmFits param
    {
        psMemId id = psMemGetId();
        psMetadata *camera = psMetadataAlloc();
        pmFPA* fpa = generateSimpleFPA(camera);
        ok(!pmFPARead(fpa, NULL, NULL), "pmFPAReadVariance() returned NULL with NULL pmFits param");
        psFree(fpa);
        psFree(camera);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Verify pmFPAReadVariance() with acceptable input data
    {
        psMemId id = psMemGetId();
        bool rc;
        // Generate the pmFPA heirarchy
        psMetadata *camera = psMetadataAlloc();
        pmFPA* fpa = generateSimpleFPA(camera);
        pmChip *chip = fpa->chips->data[0];
        // Free the cells for chip 0 so we can verify that pmFPAReadVariance() actually reads the data from file
        if (0) {
            for (int chipID = 0 ; chipID < chip->cells->n ; chipID++) {
                pmCell *cell = (pmCell *) chip->cells->data[chipID];
                psFree(cell);
                cell = NULL;
            }
	}

        psFits* fitsFileR = psFitsOpen(".tmp00", "r");
        rc = pmFPARead(fpa, fitsFileR, NULL);
        ok(rc, "pmFPAReadVariance() returned TRUE");
        bool errorFlag = false;
        // XXX: fpaID should be chipID
        // XXX: chipID should be cellID
        for (int fpaID = 0 ; fpaID < fpa->chips->n ; fpaID++) {
            pmChip *chip = fpa->chips->data[fpaID];
            for (int chipID = 0 ; chipID < chip->cells->n ; chipID++) {
                if (VERBOSE) diag("Reading cell %d\n", chipID);
                pmCell *cell = (pmCell *) chip->cells->data[chipID];
                for (int k = 0 ; k < cell->hdu->variances->n ; k++) {
                    if (VERBOSE) diag("NOTE: image %d\n", k);
                    psImage *wgt = cell->hdu->variances->data[k];
                    for (int i = 0 ; i < wgt->numRows ; i++) {
                        for (int j = 0 ; j < wgt->numCols ; j++) {
                            if (((float) (BASE_WEIGHT+k)) != wgt->data.F32[i][j]) {
                                diag("TEST ERROR: wgt[%d][%d] is %.2f, should be %.2f\n", i, j,
                                      wgt->data.F32[i][j], ((float) (BASE_WEIGHT+k)));
                                errorFlag = true;
        			}
        		    }
        		}
        	    }
                ok(!errorFlag, "pmFPAWriteVariance()/pmFPAReadVariance() properly set the image data (chip %d, cell %d)", fpaID, chipID);
    	    }
	}

        psFitsClose(fitsFileR);
        psFree(fpa);
        psFree(camera);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // ----------------------------------------------------------------------
    // pmFPAWriteMask() tests
    // pmFPAWriteMask(pmFPA *fpa, psFits *fits, psDB *db, bool blank, bool recurse)
    // Verify pmFPAWriteMask() with NULL pmFPA param
    {
        psMemId id = psMemGetId();
        psFits* fitsFileW = psFitsOpen(".tmp01", "w");
        ok(!pmFPAWriteMask(NULL, fitsFileW, NULL, false, true), "pmFPAWriteMask() returned NULL with NULL pmFPA param");
        psFitsClose(fitsFileW);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Verify pmFPAWriteMask() with NULL pmFits param
    {
        psMemId id = psMemGetId();
        // Generate the pmFPA heirarchy
        psMetadata *camera = psMetadataAlloc();
        pmFPA* fpa = generateSimpleFPA(camera);
        ok(fpa != NULL, "Allocated a pmFPA successfully");
        ok(!pmFPAWriteMask(fpa, NULL, NULL, false, true), "pmFPAWriteMask() returned NULL with NULL pmFits param");
        psFree(fpa);
        psFree(camera);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Verify pmFPAWriteMask() with acceptable data
    {
        psMemId id = psMemGetId();
        psFits* fitsFileW = psFitsOpen(".tmp01", "w");
        // Generate the pmFPA heirarchy
        psMetadata *camera = psMetadataAlloc();
        pmFPA* fpa = generateSimpleFPA(camera);
        ok(fpa != NULL, "Allocated a pmFPA successfully");
        bool rc = pmFPAWriteMask(fpa, fitsFileW, NULL, false, true);
        ok(rc, "pmFPAWriteMask() returned TRUE");
        //  Close the FITS file, free memory
        psFitsClose(fitsFileW);
        psFree(fpa);
        psFree(camera);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // ----------------------------------------------------------------------
    // pmFPAReadMask() tests
    // Verify pmFPAReadMask() with NULL pmFPA param
    {
        psMemId id = psMemGetId();
        psFits* fitsFileR = psFitsOpen(".tmp00", "r");
        ok(!pmFPARead(NULL, fitsFileR, NULL), "pmFPAReadMask() returned NULL with NULL pmFPA param");
        psFitsClose(fitsFileR);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Verify pmFPAReadMask() with NULL pmFits param
    {
        psMemId id = psMemGetId();
        psMetadata *camera = psMetadataAlloc();
        pmFPA* fpa = generateSimpleFPA(camera);
        ok(!pmFPARead(fpa, NULL, NULL), "pmFPAReadMask() returned NULL with NULL pmFits param");
        psFree(fpa);
        psFree(camera);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // Verify pmFPAReadMask() with acceptable input data
    {
        psMemId id = psMemGetId();
        bool rc;
        // Generate the pmFPA heirarchy
        psMetadata *camera = psMetadataAlloc();
        pmFPA* fpa = generateSimpleFPA(camera);
        pmChip *chip = fpa->chips->data[0];
        // Free the cells for chip 0 so we can verify that pmFPAReadMask() actually reads the data from file
        if (0) {
            for (int chipID = 0 ; chipID < chip->cells->n ; chipID++) {
                pmCell *cell = (pmCell *) chip->cells->data[chipID];
                psFree(cell);
                cell = NULL;
            }
	}

        psFits* fitsFileR = psFitsOpen(".tmp00", "r");
        rc = pmFPARead(fpa, fitsFileR, NULL);
        ok(rc, "pmFPAReadMask() returned TRUE");
        bool errorFlag = false;
        // XXX: fpaID should be chipID
        // XXX: chipID should be cellID
        for (int fpaID = 0 ; fpaID < fpa->chips->n ; fpaID++) {
            pmChip *chip = fpa->chips->data[fpaID];
            for (int chipID = 0 ; chipID < chip->cells->n ; chipID++) {
                if (VERBOSE) diag("Reading cell %d\n", chipID);
                pmCell *cell = (pmCell *) chip->cells->data[chipID];
                for (int k = 0 ; k < cell->hdu->masks->n ; k++) {
                    if (VERBOSE) diag("NOTE: image %d\n", k);
                    psImage *msk = cell->hdu->masks->data[k];
                    for (int i = 0 ; i < msk->numRows ; i++) {
                        for (int j = 0 ; j < msk->numCols ; j++) {
                            if (((BASE_MASK+k)) != msk->data.U8[i][j]) {
                                diag("TEST ERROR: msk[%d][%d] is %.2f, should be %.2f\n", i, j,
                                      msk->data.F32[i][j], ((float) (BASE_MASK+k)));
                                errorFlag = true;
        			}
        		    }
        		}
        	    }
                ok(!errorFlag, "pmFPAWriteMask()/pmFPAReadMask() properly set the image data (chip %d, cell %d)", fpaID, chipID);
    	    }
	}

        psFitsClose(fitsFileR);
        psFree(fpa);
        psFree(camera);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
}

