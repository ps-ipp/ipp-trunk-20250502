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
// XXX: For the genSimpleFPA() code, write masks and weights as well

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
#define NUM_FPAS		4
#define NUM_HDUS                5
#define BASE_IMAGE              10
#define BASE_MASK               40
#define BASE_WEIGHT             70
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
//    psRegion *region = psRegionAlloc(0.0, TEST_NUM_COLS-1, 0.0, TEST_NUM_ROWS-1);
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
    plan_tests(50);

    // ----------------------------------------------------------------------
    // pmConceptsAverageFPAs() tests: NULL input pmFPA *target
    {
        psMemId id = psMemGetId();
        psMetadata *camera = psMetadataAlloc();
        pmFPA *targetFPA = generateSimpleFPA(camera);
        pmFPA *sourceFPA[NUM_FPAS];

        sourceFPA[0] = generateSimpleFPA(camera);
        psList *sources = psListAlloc(sourceFPA[0]);
        for (int fpaID = 1 ; fpaID < NUM_FPAS ; fpaID++) {
            sourceFPA[fpaID] = generateSimpleFPA(camera);
            bool rc = psListAdd(sources, PS_LIST_HEAD, sourceFPA[fpaID]);
            ok(rc, "Successfully added FPA %d to list", fpaID);
	}
        ok(!pmConceptsAverageFPAs(NULL, sources), "pmConceptsAverage(NULL, sources) returned FALSE");

        for (int fpaID = 0 ; fpaID < NUM_FPAS ; fpaID++) {
            psFree(sourceFPA[fpaID]);
	}
        psFree(sources);
        psFree(targetFPA);
        psFree(camera);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // pmConceptsAverageFPAs() tests: NULL input psList *sources
    {
        psMemId id = psMemGetId();
        psMetadata *camera = psMetadataAlloc();
        pmFPA *targetFPA = generateSimpleFPA(camera);
        pmFPA *sourceFPA[NUM_FPAS];

        sourceFPA[0] = generateSimpleFPA(camera);
        psList *sources = psListAlloc(sourceFPA[0]);
        for (int fpaID = 1 ; fpaID < NUM_FPAS ; fpaID++) {
            sourceFPA[fpaID] = generateSimpleFPA(camera);
            bool rc = psListAdd(sources, PS_LIST_HEAD, sourceFPA[fpaID]);
            ok(rc, "Successfully added FPA %d to list", fpaID);
	}
        ok(!pmConceptsAverageFPAs(targetFPA, NULL), "pmConceptsAverage(NULL, sources) returned FALSE");

        for (int fpaID = 0 ; fpaID < NUM_FPAS ; fpaID++) {
            psFree(sourceFPA[fpaID]);
	}
        psFree(sources);
        psFree(targetFPA);
        psFree(camera);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


    // pmConceptsAverageFPAs() tests: acceptable inputs
    // XXX: There's a memory leak somewhere in this test, not sure where.
    {
        psMemId id = psMemGetId();
        psMetadata *camera = psMetadataAlloc();
        pmFPA *targetFPA = generateSimpleFPA(camera);
        pmFPA *sourceFPA[NUM_FPAS];
        psMetadata *cameras[NUM_FPAS];

        // Ensure that the FPA.TIME average is computed correctly
        psList *sources = NULL;
        psF64 actualTime = 0.0;
        for (int fpaID = 0 ; fpaID < NUM_FPAS ; fpaID++) {
            cameras[fpaID] = psMetadataAlloc();
            sourceFPA[fpaID] = generateSimpleFPA(cameras[fpaID]);
            psTime *fpaTime = psMetadataLookupPtr(NULL, (sourceFPA[fpaID])->concepts, "FPA.TIME");
            // Add a small value to the psTime so that we can test/ensure that pmConceptsAverageFPAs()
            // is actually calculating an average.
            fpaTime->sec += (double) (fpaID * 1000);
            actualTime+= psTimeToMJD(fpaTime);
            if (0 == fpaID) {
                sources = psListAlloc(sourceFPA[fpaID]);
	    } else {
                bool rc = psListAdd(sources, PS_LIST_HEAD, sourceFPA[fpaID]);
                ok(rc, "Successfully added FPA %d to list", fpaID);
	    }
	}
        // XXX: The memory leak occurs during the following single call
        ok(pmConceptsAverageFPAs(targetFPA, sources), "pmConceptsAverage(targetFPA, sources) returned TRUE");
        actualTime/= (float) NUM_FPAS;
        psTime *fpaTime = psMetadataLookupPtr(NULL, targetFPA->concepts, "FPA.TIME");
        ok(abs(actualTime - psTimeToMJD(fpaTime)) < 1e-4, "pmConceptsAverageFPAs() calculated the average time correctly");

        // Replace the FPA.TIMESYS with a non-conforming value, verify that pmConceptsAverageFPAs() returns an error

        psTimeType timeSys = psMetadataLookupS32(NULL, sourceFPA[0]->concepts, "FPA.TIMESYS");
        psMetadataAddS32(sourceFPA[NUM_FPAS-1]->concepts, PS_LIST_HEAD, "FPA.TIMESYS", PS_META_REPLACE, NULL, timeSys+10);
        ok(!pmConceptsAverageFPAs(targetFPA, sources), "pmConceptsAverage(NULL, sources) returned FALSE with nonequal FPA.TIMESYS metadata");
        psMetadataAddS32(sourceFPA[NUM_FPAS-1]->concepts, PS_LIST_HEAD, "FPA.TIMESYS", PS_META_REPLACE, NULL, timeSys);
        ok(pmConceptsAverageFPAs(targetFPA, sources), "pmConceptsAverage(NULL, sources) returned TRUE with equal FPA.TIMESYS metadata");


        // Replace the FPA.TIMESYS with a non-conforming value, verify that pmConceptsAverageFPAs() returnes an error        
        psString filter = psMetadataLookupStr(NULL, sourceFPA[0]->concepts, "FPA.FILTER");
        psMetadataAddStr(sourceFPA[NUM_FPAS-1]->concepts, PS_LIST_HEAD, "FPA.FILTER", PS_META_REPLACE, NULL, "BOGUS STRING");
        ok(!pmConceptsAverageFPAs(targetFPA, sources), "pmConceptsAverage(NULL, sources) returned FALSE with nonequal FPA.FILTER metadata");
        psMetadataAddStr(sourceFPA[NUM_FPAS-1]->concepts, PS_LIST_HEAD, "FPA.FILTER", PS_META_REPLACE, NULL, filter);
        ok(pmConceptsAverageFPAs(targetFPA, sources), "pmConceptsAverage(NULL, sources) returned TRUE with equal FPA.FILTER metadata");

        // Free data, check for memory leaks
        for (int fpaID = 0 ; fpaID < NUM_FPAS ; fpaID++) {
            psFree(sourceFPA[fpaID]);
            psFree(cameras[fpaID]);
	}
        psFree(sources);
//        psFree(filter);
        psFree(targetFPA);
        psFree(camera);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }

    // ----------------------------------------------------------------------
    // pmConceptsAverageCells() tests: NULL input pmFPA *target
    // bool pmConceptsAverageCells(pmCell *target, psList *sources, psRegion *trimsec, psRegion *biassec, bool same)
    {
        psMemId id = psMemGetId();
        psMetadata *tgtCamera = psMetadataAlloc();
        pmFPA *tgtFPA = generateSimpleFPA(tgtCamera);
        pmChip *tgtChip = tgtFPA->chips->data[0];
        pmCell *tgtCell = tgtChip->cells->data[0];
        psMetadata *srcCamera = psMetadataAlloc();
        pmFPA *srcFPA = generateSimpleFPA(srcCamera);
        pmChip *srcChip = srcFPA->chips->data[0];

        psList *sources = NULL;
        psF32 tstGain = 0.0;
        psF32 tstReadnoise = 0.0;
        psF32 tstExposure = 0.0;
        psF32 tstDarktime = 0.0;
        psF32 tstSaturation = 0.0;
        psF32 tstBad = 0.0;
        
        for (int cellID = 0 ; cellID < srcChip->cells->n ; cellID++) {
            pmCell *cell = srcChip->cells->data[cellID];
            // Set the various concepts which we will test later
            psMetadataAddF32(cell->concepts, PS_LIST_HEAD, "CELL.GAIN", PS_META_REPLACE, NULL, 0.0 + (float) cellID);
            tstGain+= 0.0 + (float) cellID;
            psMetadataAddF32(cell->concepts, PS_LIST_HEAD, "CELL.READNOISE", PS_META_REPLACE, NULL, 10.0 + (float) cellID);
            tstReadnoise+= 10.0 + (float) cellID;
            psMetadataAddF32(cell->concepts, PS_LIST_HEAD, "CELL.EXPOSURE", PS_META_REPLACE, NULL, 20.0 + (float) cellID);
            tstExposure+= 20.0 + (float) cellID;
            psMetadataAddF32(cell->concepts, PS_LIST_HEAD, "CELL.DARKTIME", PS_META_REPLACE, NULL, 30.0 + (float) cellID);
            tstDarktime+= 30.0 + (float) cellID;
            psMetadataAddF32(cell->concepts, PS_LIST_HEAD, "CELL.SATURATION", PS_META_REPLACE, NULL, 40.0 + (float) cellID);
            if (cellID == 0)
                tstSaturation = 40.0 + (float) cellID;
            psMetadataAddF32(cell->concepts, PS_LIST_HEAD, "CELL.BAD", PS_META_REPLACE, NULL, 50.0 + (float) cellID);
            if (cellID == (srcChip->cells->n - 1))
                tstBad = 50.0 + (float) cellID;
            if (cellID == 0) {
                sources = psListAlloc(srcChip->cells->data[cellID]);
	    } else {
                bool rc = psListAdd(sources, PS_LIST_HEAD, srcChip->cells->data[cellID]);
                ok(rc, "Successfully added cell %d to list", cellID);
	    }
	}
        tstGain /= (psF64) srcChip->cells->n;
        tstReadnoise /= (psF64) srcChip->cells->n;
        tstExposure /= (psF64) srcChip->cells->n;
        tstDarktime /= (psF64) srcChip->cells->n;

        psRegion *trimsec = psRegionAlloc(0, 1, 2, 3);
        psRegion *biassec = psRegionAlloc(4, 5, 6, 7);

        // Ensure pmConceptsAverageCells() returns NULL with NULL tgtCell input
        bool rc = pmConceptsAverageCells(NULL, sources, trimsec, biassec, false);
        ok(!rc, "pmConceptsAverageCells() returned FALSE with NULL sources input");

        // Ensure pmConceptsAverageCells() returns NULL with NULL sources input
        rc = pmConceptsAverageCells(tgtCell, NULL, trimsec, biassec, false);
        ok(!rc, "pmConceptsAverageCells() returned FALSE with NULL targetCell input");

        // Ensure pmConceptsAverageCells() returns NULL with sources->n = 0
        sources->n = 0;
        rc = pmConceptsAverageCells(tgtCell, sources, trimsec, biassec, false);
        ok(!rc, "pmConceptsAverageCells() returned FALSE with NULL sources->n = 0");
        sources->n = NUM_CELLS;

        // Call pmConceptsAverageCells() with acceptable input data
        rc = pmConceptsAverageCells(tgtCell, sources, trimsec, biassec, false);
        ok(rc, "pmConceptsAverageCells() returned TRUE with acceptable input data");
        psF32 tmpF32;
        tmpF32 = psMetadataLookupF32(NULL, tgtCell->concepts, "CELL.GAIN");
        ok(abs(tmpF32 - tstGain) < 1e-4, "pmConceptsAverageCells() calculated the average CELL.GAIN correctly");
        tmpF32 = psMetadataLookupF32(NULL, tgtCell->concepts, "CELL.READNOISE");
        ok(abs(tmpF32 - tstReadnoise) < 1e-4, "pmConceptsAverageCells() calculated the average CELL.READNOISE correctly");
        tmpF32 = psMetadataLookupF32(NULL, tgtCell->concepts, "CELL.EXPOSURE");
        ok(abs(tmpF32 - tstExposure) < 1e-4, "pmConceptsAverageCells() calculated the average CELL.EXPOSURE correctly");
        tmpF32 = psMetadataLookupF32(NULL, tgtCell->concepts, "CELL.DARKTIME");
        ok(abs(tmpF32 - tstDarktime) < 1e-4, "pmConceptsAverageCells() calculated the average CELL.DARKTIME correctly");
        tmpF32 = psMetadataLookupF32(NULL, tgtCell->concepts, "CELL.SATURATION");
        ok(abs(tmpF32 - tstSaturation) < 1e-4, "pmConceptsAverageCells() calculated the average CELL.SATURATION correctly (%f %f)", tmpF32, tstSaturation);
        tmpF32 = psMetadataLookupF32(NULL, tgtCell->concepts, "CELL.BAD");
        ok(abs(tmpF32 - tstBad) < 1e-4, "pmConceptsAverageCells() calculated the average CELL.BAD correctly");
        psRegion *tstTrimsec = psMetadataLookupPtr(NULL, tgtCell->concepts, "CELL.TRIMSEC");
        ok(tstTrimsec->x0 == 0.0 && tstTrimsec->x1 == 1.0 && tstTrimsec->y0 == 2.0 && tstTrimsec->y1 == 3.0,
           "pmConceptsAverageCells() set the CELL>TRIMSEC region correctly");
        psRegion *tstBiassec = psMetadataLookupPtr(NULL, tgtCell->concepts, "CELL.BIASSEC");
        ok(tstBiassec->x0 == 4.0 && tstBiassec->x1 == 5.0 && tstBiassec->y0 == 6.0 && tstBiassec->y1 == 7.0,
           "pmConceptsAverageCells() set the CELL>TRIMSEC region correctly");

        psS32 tmpS32;
        pmCell *srcCell = srcChip->cells->data[NUM_CELLS - 1];

        // Set the CELL.TIMESYS metadata unequal, verify that error occurs
        rc = psMetadataAddS32(srcCell->concepts, PS_LIST_HEAD, "CELL.TIMESYS", PS_META_REPLACE, NULL, 232);
        tmpS32 = psMetadataLookupS32(&rc, srcCell->concepts, "CELL.TIMESYS");
        rc = pmConceptsAverageCells(tgtCell, sources, trimsec, biassec, false);
        ok(!rc, "pmConceptsAverageCells() returned FALSE with non equal CELL.TIMESYS metadata");
        psMetadataAddS32(srcCell->concepts, PS_LIST_HEAD, "CELL.TIMESYS", PS_META_REPLACE, NULL, tmpS32);

        // Set the CELL.READDIR metadata unequal, verify that error occurs
        rc = psMetadataAddS32(srcCell->concepts, PS_LIST_HEAD, "CELL.READDIR", PS_META_REPLACE, NULL, 232);
        tmpS32 = psMetadataLookupS32(&rc, srcCell->concepts, "CELL.READDIR");
        rc = pmConceptsAverageCells(tgtCell, sources, trimsec, biassec, false);
        ok(!rc, "pmConceptsAverageCells() returned FALSE with non equal CELL.READDIR metadata");
        psMetadataAddS32(srcCell->concepts, PS_LIST_HEAD, "CELL.READDIR", PS_META_REPLACE, NULL, tmpS32);

        // Set the CELL.XBIN metadata unequal, verify that error occurs
        rc = psMetadataAddS32(srcCell->concepts, PS_LIST_HEAD, "CELL.XBIN", PS_META_REPLACE, NULL, 232);
        tmpS32 = psMetadataLookupS32(&rc, srcCell->concepts, "CELL.XBIN");
        rc = pmConceptsAverageCells(tgtCell, sources, trimsec, biassec, false);
        ok(!rc, "pmConceptsAverageCells() returned FALSE with non equal CELL.XBIN metadata");
        psMetadataAddS32(srcCell->concepts, PS_LIST_HEAD, "CELL.XBIN", PS_META_REPLACE, NULL, tmpS32);

        // Set the CELL.YBIN metadata unequal, verify that error occurs
        rc = psMetadataAddS32(srcCell->concepts, PS_LIST_HEAD, "CELL.YBIN", PS_META_REPLACE, NULL, 232);
        tmpS32 = psMetadataLookupS32(&rc, srcCell->concepts, "CELL.YBIN");
        rc = pmConceptsAverageCells(tgtCell, sources, trimsec, biassec, false);
        ok(!rc, "pmConceptsAverageCells() returned FALSE with non equal CELL.YBIN metadata");
        psMetadataAddS32(srcCell->concepts, PS_LIST_HEAD, "CELL.YBIN", PS_META_REPLACE, NULL, tmpS32);

        // Set the CELL.X0 metadata unequal, verify that error occurs
        rc = psMetadataAddS32(srcCell->concepts, PS_LIST_HEAD, "CELL.X0", PS_META_REPLACE, NULL, 232);
        tmpS32 = psMetadataLookupS32(&rc, srcCell->concepts, "CELL.X0");
        rc = pmConceptsAverageCells(tgtCell, sources, trimsec, biassec, true);
        ok(!rc, "pmConceptsAverageCells() returned FALSE with non equal CELL.X0 metadata (same = true)");
        psMetadataAddS32(srcCell->concepts, PS_LIST_HEAD, "CELL.X0", PS_META_REPLACE, NULL, tmpS32);

        // Set the CELL.Y0 metadata unequal, verify that error occurs
        rc = psMetadataAddS32(srcCell->concepts, PS_LIST_HEAD, "CELL.Y0", PS_META_REPLACE, NULL, 232);
        tmpS32 = psMetadataLookupS32(&rc, srcCell->concepts, "CELL.Y0");
        rc = pmConceptsAverageCells(tgtCell, sources, trimsec, biassec, true);
        ok(!rc, "pmConceptsAverageCells() returned FALSE with non equal CELL.Y0 metadata (same = true)");
        psMetadataAddS32(srcCell->concepts, PS_LIST_HEAD, "CELL.Y0", PS_META_REPLACE, NULL, tmpS32);

        // Set the CELL.X0 metadata unequal, verify that no error occurs with same==false
        rc = psMetadataAddS32(srcCell->concepts, PS_LIST_HEAD, "CELL.X0", PS_META_REPLACE, NULL, 232);
        tmpS32 = psMetadataLookupS32(&rc, srcCell->concepts, "CELL.X0");
        rc = pmConceptsAverageCells(tgtCell, sources, trimsec, biassec, false);
        ok(!rc, "pmConceptsAverageCells() returned FALSE with non equal CELL.X0 metadata (same = false)");
        psMetadataAddS32(srcCell->concepts, PS_LIST_HEAD, "CELL.X0", PS_META_REPLACE, NULL, tmpS32);

        // Set the CELL.Y0 metadata unequal, verify that no error occurs with same==false
        rc = psMetadataAddS32(srcCell->concepts, PS_LIST_HEAD, "CELL.Y0", PS_META_REPLACE, NULL, 232);
        tmpS32 = psMetadataLookupS32(&rc, srcCell->concepts, "CELL.Y0");
        rc = pmConceptsAverageCells(tgtCell, sources, trimsec, biassec, false);
        ok(!rc, "pmConceptsAverageCells() returned FALSE with non equal CELL.Y0 metadata (same = false)");
        psMetadataAddS32(srcCell->concepts, PS_LIST_HEAD, "CELL.Y0", PS_META_REPLACE, NULL, tmpS32);

        psFree(tgtFPA);
        psFree(srcFPA);
        psFree(tgtCamera);
        psFree(srcCamera);
        psFree(biassec);
        psFree(trimsec);
        psFree(sources);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }


}


