#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include <psmodules.h>
#include "tap.h"
#include "pstap.h"
// XXX: Use better name for the temporary FITS file
// XXX: For the genSimpleFPA() code, add IDs to each function so that
// the values set in each chip-?cell-?hdu-?image are unique
// XXX: For the genSimpleFPA() code, write masks and weights as well
// XXX: Must add tests for pmReadoutGenerateWeight()
// XXX: We don't test pmReadoutGenerateMaskWeight() and pmCellGenerateMaskWeight()
// because they are simply calls to the above tested functions

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
#define SATURATION_LEVEL	10000.0
#define BAD_LEVEL		100.0
#define SATURATION_MASK		1
#define BAD_MASK		2
#define CELL_GAIN		1.0
#define CELL_READNOISE		2.0
#define VERBOSE                 0
#define ERR_TRACE_LEVEL         0

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
    for (int i = 0 ; i < TEST_NUM_ROWS ; i++) {
        for (int j = 0 ; j < TEST_NUM_COLS ; j++) {
            readout->image->data.F32[i][j] = 32.0;
            readout->mask->data.U8[i][j] = 0;
            readout->variance->data.F32[i][j] = 1.0;
	}
    }

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
    plan_tests(18);

    // ----------------------------------------------------------------------
    // pmReadoutSetMask() tests: NULL inputs
    // bool pmReadoutSetMask(pmReadout *readout, psMaskType satMask, psMaskType badMask)
    if (1) {
        psMemId id = psMemGetId();
        psMetadata *camera = psMetadataAlloc();
        pmFPA* fpa = generateSimpleFPA(camera);
        pmChip *chip = fpa->chips->data[0];
        pmCell *cell = chip->cells->data[0];
        pmReadout *readout = cell->readouts->data[0];
        bool rc;

        // Set readout == NULL, ensure pmReadoutSetMask() returnes FALSE, with no seg faults, memory leaks
        rc = pmReadoutSetMask(NULL, SATURATION_MASK, BAD_MASK);
        ok(!rc, "pmReadoutSetMask(NULL, SATURATION_MASK, BAD_MASK) returned FALSE with null pmReadout input");

        // Set readout->image, ensure pmReadoutSetMask() returnes FALSE, with no seg faults, memory leaks
        psImage *saveImg = readout->image;
        readout->image = NULL;
        rc = pmReadoutSetMask(readout, SATURATION_MASK, BAD_MASK);
        ok(!rc, "pmReadoutSetMask(readout, SATURATION_MASK, BAD_MASK) returned FALSE with null pmReadout->image input");
        readout->image = saveImg;

        // Set pixels in the upper-left quadrant to values [10000:11000] range
        // Set pixels in the upper-right quadrant to values [0:1000] range
        // Set pixels in the lower-left quadrant to values [100000:1000000] range
        // Set pixels in the lower-right quadrant to values [100000:1000000] range

        for (int i = 0 ; i < readout->image->numRows ; i++) {
            for (int j = 0 ; j < readout->image->numCols ; j++) {
                if (i < readout->image->numRows/2) {
                    if (j < readout->image->numCols/2) {
                        readout->image->data.F32[i][j] = 10000.0 + (float) (i + j);
		    } else {
                        readout->image->data.F32[i][j] = (float) (i + j);
		    }
		} else {
                    readout->image->data.F32[i][j] = 100000.0 + (float) (i + j);
		}
	    }
	}

        // Set the acceptable pixel range to [100.0 : 20000.0]
        rc = psMetadataAddF32(readout->parent->concepts, PS_LIST_HEAD, "CELL.SATURATION", PS_META_REPLACE, NULL, 20000.0);
        rc|= psMetadataAddF32(readout->parent->concepts, PS_LIST_HEAD, "CELL.BAD", PS_META_REPLACE, NULL, 100.0);
        ok(rc, "Set pixel range in cell->concepts successfully");

        // Call pmReadoutSetMask() and then verify that the mask data was set correctly
        rc = pmReadoutSetMask(readout, SATURATION_MASK, BAD_MASK);
        ok(rc, "pmReadoutSetMask(readout, SATURATION_MASK, BAD_MASK) returned TRUE with acceptable input data");
        bool errorFlag = false;
        for (int i = 0 ; i < readout->image->numRows ; i++) {
            for (int j = 0 ; j < readout->image->numCols ; j++) {
                if (i < readout->image->numRows/2) {
                    if (j < readout->image->numCols/2) {
                        if(readout->mask->data.U8[i][j] != 0) {
                            if (VERBOSE) {
                                diag("TEST ERROR: mask[%d][%d] is %d, should be 0\n",
                                      i, j, readout->mask->data.U8[i][j]);
			    }
                            errorFlag = true;
			}
		    } else {
                        if(readout->mask->data.U8[i][j] != BAD_MASK) {
                            if (VERBOSE) {
                                diag("TEST ERROR: mask[%d][%d] is %d, should be %d\n",
                                      i, j, readout->mask->data.U8[i][j], BAD_MASK);
			    }
                            errorFlag = true;
			}
		    }
		} else {
                    if(readout->mask->data.U8[i][j] != SATURATION_MASK) {
                        if (VERBOSE) {
                            diag("TEST ERROR: mask[%d][%d] is %d, should be %d\n",
                                  i, j, readout->mask->data.U8[i][j], SATURATION_MASK);
                        }
                        errorFlag = true;
		    }
		}
	    }
	}
        ok(!errorFlag, "pmReadoutSetMask() set the mask values correctly");
        psFree(fpa);    
        psFree(camera);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }



    // ----------------------------------------------------------------------
    // pmReadoutGenerateMask() tests: NULL inputs
    // bool pmReadoutGenerateMask(pmReadout *readout, psMaskType satMask, psMaskType badMask)
    // XXX: This test is a duplicate of the above pmReadoutSetMask() test since the actual
    // code is almost the same.  Must test with the readout->mask == NULL.
    if (1) {
        psMemId id = psMemGetId();
        psMetadata *camera = psMetadataAlloc();
        pmFPA* fpa = generateSimpleFPA(camera);
        pmChip *chip = fpa->chips->data[0];
        pmCell *cell = chip->cells->data[0];
        pmReadout *readout = cell->readouts->data[0];
        bool rc;

        // Set readout == NULL, ensure pmReadoutGenerateMask() returnes FALSE, with no seg faults, memory leaks
        rc = pmReadoutGenerateMask(NULL, SATURATION_MASK, BAD_MASK);
        ok(!rc, "pmReadoutGenerateMask(NULL, SATURATION_MASK, BAD_MASK) returned FALSE with null pmReadout input");

        // Set pixels in the upper-left quadrant to values [10000:11000] range
        // Set pixels in the upper-right quadrant to values [0:1000] range
        // Set pixels in the lower-left quadrant to values [100000:1000000] range
        // Set pixels in the lower-right quadrant to values [100000:1000000] range

        for (int i = 0 ; i < readout->image->numRows ; i++) {
            for (int j = 0 ; j < readout->image->numCols ; j++) {
                if (i < readout->image->numRows/2) {
                    if (j < readout->image->numCols/2) {
                        readout->image->data.F32[i][j] = 10000.0 + (float) (i + j);
		    } else {
                        readout->image->data.F32[i][j] = (float) (i + j);
		    }
		} else {
                    readout->image->data.F32[i][j] = 100000.0 + (float) (i + j);
		}
	    }
	}

        // Set the acceptable pixel range to [100.0 : 20000.0]
        rc = psMetadataAddF32(readout->parent->concepts, PS_LIST_HEAD, "CELL.SATURATION", PS_META_REPLACE, NULL, 20000.0);
        rc|= psMetadataAddF32(readout->parent->concepts, PS_LIST_HEAD, "CELL.BAD", PS_META_REPLACE, NULL, 100.0);
        ok(rc, "Set pixel range in cell->concepts successfully");

        // Call pmReadoutGenerateMask() and then verify that the mask data was set correctly
        rc = pmReadoutGenerateMask(readout, SATURATION_MASK, BAD_MASK);
        ok(rc, "pmReadoutGenerateMask(readout, SATURATION_MASK, BAD_MASK) returned TRUE with acceptable input data");
        bool errorFlag = false;
        for (int i = 0 ; i < readout->image->numRows ; i++) {
            for (int j = 0 ; j < readout->image->numCols ; j++) {
                if (i < readout->image->numRows/2) {
                    if (j < readout->image->numCols/2) {
                        if(readout->mask->data.U8[i][j] != 0) {
                            if (VERBOSE) {
                                diag("TEST ERROR: mask[%d][%d] is %d, should be 0\n",
                                      i, j, readout->mask->data.U8[i][j]);
			    }
                            errorFlag = true;
			}
		    } else {
                        if(readout->mask->data.U8[i][j] != BAD_MASK) {
                            if (VERBOSE) {
                                diag("TEST ERROR: mask[%d][%d] is %d, should be %d\n",
                                      i, j, readout->mask->data.U8[i][j], BAD_MASK);
                                errorFlag = true;
			    }
			}
		    }
		} else {
                    if(readout->mask->data.U8[i][j] != SATURATION_MASK) {
                        if (VERBOSE) {
                            diag("TEST ERROR: mask[%d][%d] is %d, should be %d\n",
                                  i, j, readout->mask->data.U8[i][j], SATURATION_MASK);
                        }
                        errorFlag = true;
		    }
		}
	    }
	}
        ok(!errorFlag, "pmReadoutGenerateMask() set the mask values correctly");

        psFree(fpa);    
        psFree(camera);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }



    // ----------------------------------------------------------------------
    // pmReadoutSetVariance() tests: NULL inputs
    // bool pmReadoutSetVariance(pmReadout *readout, const psImage *noiseMap, bool poisson)
    if (1) {
        psMemId id = psMemGetId();
        psMetadata *camera = psMetadataAlloc();
        pmFPA* fpa = generateSimpleFPA(camera);
        pmChip *chip = fpa->chips->data[0];
        pmCell *cell = chip->cells->data[0];
        pmReadout *readout = cell->readouts->data[0];
        bool rc;

        // Set readout == NULL, ensure pmReadoutSetVariance() returnes FALSE, with no seg faults, memory leaks
        rc = pmReadoutSetVariance(NULL, NULL, false);
        ok(!rc, "pmReadoutSetVariance(NULL, NULL, false) returned FALSE with null pmReadout input");


        // Set the acceptable pixel range to [100.0 : 20000.0]
        rc = psMetadataAddF32(readout->parent->concepts, PS_LIST_HEAD, "CELL.GAIN", PS_META_REPLACE, NULL, CELL_GAIN);
        rc|= psMetadataAddF32(readout->parent->concepts, PS_LIST_HEAD, "CELL.READNOISE", PS_META_REPLACE, NULL, CELL_READNOISE);
        ok(rc, "Set GAIN and READNOISE in cell->concepts successfully");
/*
 * Getting the section below to run requires generating a noiseMap
 *
        // Call pmReadoutSetVariance() and then verify that the mask data was set correctly
        rc = pmReadoutSetVariance(readout, false);
        ok(rc, "pmReadoutSetVariance(readout, false) returned TRUE with acceptable input data");
        bool errorFlag = false;
        for (int i = 0 ; i < readout->variance->numRows ; i++) {
            for (int j = 0 ; j < readout->variance->numCols ; j++) {
                psF32 exp = CELL_READNOISE * CELL_READNOISE / CELL_GAIN / CELL_GAIN;
                if(abs(readout->variance->data.F32[i][j] - exp) > 1e-4) {
                    if (VERBOSE) {
                        diag("TEST ERROR: weight[%d][%d] is %.2f, should be %.2f\n",
                              i, j, readout->variance->data.F32[i][j], exp);
		    }
                    errorFlag = true;
		}
	    }
	}
        ok(!errorFlag, "pmReadoutSetVariance() set the weight values correctly (non-Poisson)");

        for (int i = 0 ; i < readout->image->numRows ; i++) {
            for (int j = 0 ; j < readout->image->numCols ; j++) {
               readout->image->data.F32[i][j] = 100.0 + (float) (i + j);
	    }
	}
        // Call pmReadoutSetVariance() and then verify that the mask data was set correctly
        rc = pmReadoutSetVariance(readout, true);
        ok(rc, "pmReadoutSetVariance(readout, true) returned TRUE with acceptable input data");
        errorFlag = false;
        for (int i = 0 ; i < readout->variance->numRows ; i++) {
            for (int j = 0 ; j < readout->variance->numCols ; j++) {
                psF32 exp = abs(readout->image->data.F32[i][j] / CELL_GAIN); 
                if (exp < 1.0) exp = 1.0;
                exp+= CELL_READNOISE * CELL_READNOISE / CELL_GAIN / CELL_GAIN;
                if(abs(readout->variance->data.F32[i][j] - exp) > 1e-4) {
                    if (VERBOSE) {
                        diag("TEST ERROR: weight[%d][%d] is %.2f, should be %.2f\n",
                              i, j, readout->variance->data.F32[i][j], exp);
		    }
                    errorFlag = true;
		}
	    }
	}

        ok(!errorFlag, "pmReadoutSetWeight() set the weight values correctly (Poisson)");
*/	
        psFree(fpa);    
        psFree(camera);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
}

