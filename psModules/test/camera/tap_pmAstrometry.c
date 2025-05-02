/** @file  tst_psAstrometry01.c
*
* XXX: The source code that is tested here is in the pmAstrometry.c and
* pmAstrometry.h files.  However, those files are not included in the
* current automake configuration.  These tests are therefore, not
* runnable.  We include them in this distribution for the future.
*
* XXX: These tests need to be converted to tap format.
*
* XXX: Add tests were the coordinate does not transform to any legitimate cell
* or chip, or FPA, or whatever.
*
* XXX: For each function, add tests for bad input parameters, as well as failed transforms.
*
*  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
*/
#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include <psmodules.h>
#include "tap.h"
#include "pstap.h"

#define PS_PERCENT_COMPARE(X, Y, PERCENT_FRACTION) (fabs((Y)-(X))/fabs(X) < (PERCENT_FRACTION))
#define VERBOSE 0
#define NUM_READOUTS 1
#define READOUT_NUM_ROWS 10
#define READOUT_NUM_COLS 10

#define NUM_CELLS 4
#define CELL_GAP 2
#define CELL_WIDTH READOUT_NUM_COLS
#define CELL_HEIGHT READOUT_NUM_ROWS
#define CELL_MIN_X 0
#define CELL_MAX_X CELL_HEIGHT
#define CELL_MIN_Y 0
#define CELL_MAX_Y CELL_WIDTH

#define NUM_CHIPS 2
#define CHIP_GAP 2
#define CHIP_MIN_X 0
#define CHIP_MAX_X CELL_HEIGHT
#define CHIP_MIN_Y 0
#define CHIP_MAX_Y ((NUM_CELLS * CELL_WIDTH) + ((NUM_CELLS) * CELL_GAP))
#define CHIP_WIDTH CHIP_MAX_Y
#define CHIP_HEIGHT CHIP_MAX_X

#define NUM_FPAS 1
#define FPA_MIN_X 0
#define FPA_MAX_X CHIP_HEIGHT
#define FPA_MIN_Y 0
#define FPA_MAX_Y (((NUM_CHIPS) * CHIP_WIDTH) + (((NUM_CHIPS)-1) * CHIP_GAP))

#define PROJECTION_SCALE_X 1.0
#define PROJECTION_SCALE_Y 1.0

psS32 currentId = 0;
psS32 memLeaks = 0;

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
genSystem(): This routine will create a system of FPAs/Chips/Cells/Readouts.  For
simplicity, an FPA is defined as a linear array of chips, and a chip is
defined as a linear array of cells, both in the y (cols) direction.  The
transforms between the various layers take into account the cell/chip and
the boundaries between each.
 *****************************************************************************/
pmFPA *genSystem()
{
    //
    // Create top pmFPA structure.
    //
    const psMetadata *camera = psMetadataAlloc();
    pmFPA* myFPA = pmFPAAlloc(camera);
    if (myFPA == NULL) {
        psLogMsg(__func__,PS_LOG_ERROR, "TEST ERROR: pmFPAAlloc() returned a NULL.\n");
        return NULL;
    }
    myFPA->fromTPA = PS_CREATE_2D_IDENTITY_PLANE_TRANSFORM();
    myFPA->toTPA = PS_CREATE_2D_IDENTITY_PLANE_TRANSFORM();
    myFPA->projection = psProjectionAlloc(0.0,0.0,PROJECTION_SCALE_X,PROJECTION_SCALE_X,PS_PROJ_TAN);

    myFPA->chips = psArrayRealloc(myFPA->chips, NUM_CHIPS);
    for (psS32 chipID=0 ; chipID<NUM_CHIPS ; chipID++) {
        pmChip *myChip = pmChipAlloc(myFPA, "ChipName");
        if (myChip == NULL) {
            psLogMsg(__func__,PS_LOG_ERROR, "TEST ERROR: pmChipAlloc() returned a NULL.\n");
            return NULL;
        }
        myFPA->chips->data[chipID] = (psPtr *) myChip;
        int myChipRow0 = 0;
        int myChipCol0 = chipID * (CHIP_WIDTH + CHIP_GAP);

        // We create the transforms between the chip and FPA.  The
        // transform is a simple identity transform.
        myChip->toFPA = PS_CREATE_2D_IDENTITY_PLANE_TRANSFORM();
        myChip->toFPA->y->coeff[0][0] = (psF64) myChipCol0;
        myChip->toFPA->y->coeff[1][1] = 0.0;
        myChip->toFPA->x->coeff[0][0] = (psF64) myChipRow0;
        myChip->toFPA->x->coeff[1][1] = 0.0;

        myChip->fromFPA = PS_CREATE_2D_IDENTITY_PLANE_TRANSFORM();
        myChip->fromFPA->y->coeff[0][0] = (psF64) (- myChipCol0);
        myChip->fromFPA->y->coeff[1][1] = 0.0;
        myChip->fromFPA->x->coeff[0][0] = (psF64) (- myChipRow0);
        myChip->fromFPA->x->coeff[1][1] = 0.0;

        myChip->cells = psArrayRealloc(myChip->cells, NUM_CELLS);
        for (psS32 cellID=0 ; cellID<NUM_CELLS ; cellID++) {
            pmCell *myCell = pmCellAlloc(myChip, NULL, "CellName");
            if (myCell == NULL) {
                psLogMsg(__func__,PS_LOG_ERROR, "TEST ERROR: pmCellAlloc() returned a NULL.\n");
                return NULL;
            }
            myChip->cells->data[cellID] = (psPtr *) myCell;
            int myCellRow0 = 0;
            int myCellCol0 = cellID * (CELL_WIDTH + CELL_GAP);
            myCell->toChip = PS_CREATE_2D_IDENTITY_PLANE_TRANSFORM();
            myCell->toChip->y->coeff[0][0] = (psF64) myCellCol0;
            myCell->toChip->y->coeff[1][1] = 0.0;
            myCell->toChip->x->coeff[0][0] = (psF64) myCellRow0;
            myCell->toChip->x->coeff[1][1] = 0.0;

            myCell->toFPA = PS_CREATE_2D_IDENTITY_PLANE_TRANSFORM();
            myCell->toFPA->y->coeff[0][0] = (psF64) (myCellCol0 + myChipCol0);
            myCell->toFPA->y->coeff[1][1] = 0.0;
            myCell->toFPA->x->coeff[0][0] = (psF64) (myCellRow0 + myChipRow0);
            myCell->toFPA->x->coeff[1][1] = 0.0;

            myCell->toSky = PS_CREATE_2D_IDENTITY_PLANE_TRANSFORM();
            myCell->toSky->y->coeff[0][0] = myCell->toFPA->y->coeff[0][0];
            myCell->toSky->y->coeff[1][1] = 0.0;
            myCell->toSky->x->coeff[0][0] = myCell->toFPA->x->coeff[0][0];
            myCell->toSky->x->coeff[1][1] = 0.0;

            myCell->readouts = psArrayRealloc(myCell->readouts, NUM_READOUTS);
            for (psS32 readoutID=0 ; readoutID<NUM_READOUTS ; readoutID++) {
                pmReadout *myReadout = pmReadoutAlloc(myCell);
                if (myReadout == NULL) {
                    psLogMsg(__func__,PS_LOG_ERROR, "TEST ERROR: pmReadoutAlloc() returned a NULL.\n");
                    return NULL;
                }
                myCell->readouts->data[readoutID] = (psPtr *) myReadout;
                myReadout->image = psImageAlloc(READOUT_NUM_COLS, READOUT_NUM_ROWS, PS_TYPE_F32);
                for (psS32 row=0;row<READOUT_NUM_ROWS;row++) {
                    for(psS32 col=0;col<READOUT_NUM_COLS;col++) {
                        myReadout->image->data.F32[row][col] = (psF32) ((chipID * (CHIP_WIDTH + CHIP_GAP)) +
                                                               (cellID * (CELL_WIDTH + CELL_GAP)));
                    }
                }
                int myReadoutRow0 = myCellRow0;
                int myReadoutCol0 = myCellCol0;
                myReadout->rowBins = 0;
                myReadout->colBins = 0;
            }
            if (VERBOSE) {
                printf("\n\n\n\nFor chip %d cell %d the cell->toFPA transform is:\n", chipID, cellID);
                PS_PRINT_PLANE_TRANSFORM(myCell->toFPA);
                printf("\n\n\n\nFor chip %d cell %d the cell->toChip transform is:\n", chipID, cellID);
                PS_PRINT_PLANE_TRANSFORM(myCell->toChip);
                printf("\n\n\n\nFor chip %d cell %d the cell->toSky transform is:\n", chipID, cellID);
                PS_PRINT_PLANE_TRANSFORM(myCell->toSky);
            }
        }
        if (VERBOSE) {
            printf("\n\n\n\nFor chip %d the chip->toFPA transform is:\n", chipID);
            PS_PRINT_PLANE_TRANSFORM(myChip->toFPA);
            printf("\n\n\n\nFor chip %d the chip->fromFPA transform is:\n", chipID);
            PS_PRINT_PLANE_TRANSFORM(myChip->fromFPA);
        }
    }

    return(myFPA);
}


/******************************************************************************
This routine tests many Astrometry functions.  It loops through all valid
pixels of all cells of all chips and computes the corresponding (x,y)
coordinates in the FPA plane.  It then calls the pmChipInFPA() then
pmCellInFPA() with the FPA coordinate and determines the chip/cell that that
coordinate corresponds to.  Following that it does a variety of tests on the
various functions that tharnsform coordinates within the pmFPA hierarchy.
 
List of tested functions:
    pmCellInFPA()  yes
    pmChipInFPA()  yes
    pmCellInChip()  yes
 
    pmCoordCellToFPA()  yes
    pmCoordChipToFPA()  yes
    pmCoordFPAToChip()  yes
    pmCoordCellToChip()  yes
    pmCoordChipToCell()  yes
 
    pmCoordFPAToTP()  yes
    pmCoordTPToFPA()  yes
 
    pmCoordTPToSky()  yes
    pmCoordSkyToTP()  yes
    pmCoordSkyToCell()  yes
    pmCoordCellToSky()  yes
    pmCoordCellToSkyQuick() yes
    pmCoordSkyToCellQuick() yes
 *****************************************************************************/
psS32 test3( void )
{
    psS32 x;
    psS32 y;
    psPlane fpaCoord;
    pmFPA *myFPA = genSystem();
    pmCell *myCell = NULL;
    psPlane chipCoord;
    psPlane cellCoord;
    psPlane testCoord;
    psSphere *skyCoord = psSphereAlloc();
    // XXX: This code causes a seg fault.
    //    psSphere skyTmp;
    //    psMemCheckType(PS_DATA_SPHERE, &skyTmp);
    psPlane tpCoord;
    psS32 testStatus = 0;

    //
    // I'm not convinced that the p_psProject() and p_psDeproject() functions work
    // correctly.  If we project a set of coordinates over a wide range of (R, D)
    // values, then deproject them, the original (R, D) values are only produced
    // when D is larger than 0.  This code demonstrates that.  I also created tests
    // that currently fail in tst_psCoord01.c.  I have a workaround in the function
    // XXXDeproject() in pmAstrometry.c.
    //
    if (0) {
        // This loop goes from (R, D) -> (X, Y) -> (R, D)
        psPlane planeCoord01;
        psSphere skyCoord01;
        psSphere skyCoord02;
        #define DEG_INC 15.0

        for (psF32 R = -90.0 ; R <= 90.0 ; R+= DEG_INC) {
            for (psF32 D = -90.0 ; D <= 90.0 ; D+= DEG_INC) {
                if ((fabs(R) != 90.0) && (fabs(D) != 90.0)) {
                    skyCoord01.r = DEG_TO_RAD(R);
                    skyCoord01.d = DEG_TO_RAD(D);
                    p_psProject(&planeCoord01, &skyCoord01, myFPA->projection);
                    p_psDeproject(&skyCoord02, &planeCoord01, myFPA->projection);
                    printf("(%.2fr %.2fd) (%.2fr %.2fd) -> (%.2f %.2f) -> (%.2fr %.2fd)", R, D,
                           skyCoord01.r, skyCoord01.d,
                           planeCoord01.x, planeCoord01.y,
                           skyCoord02.r, skyCoord02.d);
                    if ((fabs(skyCoord01.r - skyCoord02.r) < FLT_EPSILON) &&
                            (fabs(skyCoord01.d - skyCoord02.d) < FLT_EPSILON)) {
                        printf(": CORRECT\n");
                    } else {
                        printf(": WRONG\n");
                    }
                }
            }
        }
        psFree(myFPA);
        return(0);
    }
    if (0) {
        // This loop goes from (X, Y) -> (R, D) -> (X, Y)
        #define SPACE_INC 4.0
        for(testCoord.x=-CELL_HEIGHT;testCoord.x<=CELL_HEIGHT;testCoord.x+=SPACE_INC)
        {
            for (testCoord.y=-CELL_WIDTH;testCoord.y<=CELL_WIDTH;testCoord.y+=SPACE_INC) {
                psPlane planeCoord01;
                psPlane planeCoord02;
                psSphere skyCoord01;
                psSphere skyCoord02;
                p_psDeproject(&skyCoord01, &testCoord, myFPA->projection);
                p_psProject(&planeCoord01, &skyCoord01, myFPA->projection);
                p_psDeproject(&skyCoord02, &planeCoord01, myFPA->projection);
                p_psProject(&planeCoord02, &skyCoord02, myFPA->projection);
                printf("Plane: (%.2f %.2f) -> (%.2fr %.2fd) -> (%.2f %.2f)\n",
                       testCoord.x, testCoord.y,
                       skyCoord01.r, skyCoord01.d,
                       planeCoord01.x, planeCoord01.y);
                /*
                                printf("Plane: (%.2f %.2f) -> (%.2f %.2f) -> (%.2f %.2f)\n",
                                        testCoord.x, testCoord.y, planeCoord01.x, planeCoord01.y,
                                        planeCoord02.x, planeCoord02.y);
                                printf("Sphere: (%.2f %.2f) -> (%.2f %.2f)\n",
                                        skyCoord01.r, skyCoord01.d, skyCoord02.r, skyCoord02.d);
                                printf("Plane: (%.2f %.2f) -> (%.2fd %.2fr) -> (%.2f %.2f) -> (%.2fd %.2fr) -> (%.2f %.2f)\n",
                                        testCoord.x, testCoord.y,
                                        skyCoord01.r, skyCoord01.d,
                                        planeCoord01.x, planeCoord01.y,
                                        skyCoord02.r, skyCoord02.d,
                                        planeCoord02.x, planeCoord02.y);
                */
            }
        }
        psFree(myFPA);
        return(0);
    }
    //
    // We iterate through all cells on all chips on the fpa.  We determine
    // the expected fpaCcoord.
    //

    for (psS32 chip=0;chip<NUM_CHIPS;chip++) {
        for (psS32 cell=0;cell<NUM_CELLS;cell++) {
            for(x=0;x<CELL_HEIGHT;x++) {
                for (y=0;y<CELL_WIDTH;y++) {
                    fpaCoord.x = (psF64) x;
                    fpaCoord.y = (psF64) (y + (chip * (CHIP_WIDTH + CHIP_GAP)) +
                                          (cell * (CELL_WIDTH + CELL_GAP)));
                    if (VERBOSE) {
                        printf("------------------ (%.2f, %.2f) ------------------\n", fpaCoord.x, fpaCoord.y);
                        printf("(chip, cell, x, y) is (%d, %d, %d, %d)\n", chip, cell, x, y);
                    }
                    pmChip* tmpChip = pmChipInFPA(&fpaCoord, myFPA);
                    myCell = pmCellInFPA(&fpaCoord, myFPA);

                    if ((myCell == NULL) || (tmpChip == NULL)) {
                        if (tmpChip == NULL) {
                            printf("TEST ERROR: pmChipInFPA() returned NULL\n");
                            testStatus = 1;
                        } else if (myCell == NULL) {
                            printf("TEST ERROR: pmCellInFPA(): returned NULL\n");
                            testStatus = 1;
                        }
                    } else {
                        pmCoordFPAToChip(&chipCoord, &fpaCoord, tmpChip);
                        pmCoordChipToCell(&cellCoord, &chipCoord, myCell);

                        if (x != (psS32) cellCoord.x) {
                            printf("TEST ERROR: pmCoordFPAToChip()->pmCoordChipToCell(): x coord was %d (%f), should be %d\n", (psS32) cellCoord.x, cellCoord.x, x);
                            testStatus = 1;
                        }
                        if (y != (psS32) cellCoord.y) {
                            printf("TEST ERROR: pmCoordFPAToChip()->pmCoordChipToCell(): y coord was %d (%f), should be %d\n", (psS32) cellCoord.y, cellCoord.y, y);
                            testStatus = 1;
                        }

                        pmCoordCellToChip(&testCoord, &cellCoord, myCell);
                        if (fabs(testCoord.x - chipCoord.x) > FLT_EPSILON) {
                            printf("TEST ERROR: pmCoordCellToChip() x coord was %.2f, should be %d\n", cellCoord.x, x);
                            testStatus = 1;
                        }
                        if (fabs(testCoord.y - chipCoord.y) > FLT_EPSILON) {
                            printf("TEST ERROR: pmCoordCellToChip() y coord was %.2f, should be %d\n", cellCoord.y, y);
                            testStatus = 1;
                        }

                        pmCell *myCell2 = pmCellInChip(&chipCoord, tmpChip);
                        if (myCell2 != myCell) {
                            printf("TEST ERROR: pmCellInChip() != pmCellInChip(pmChipInFPA()) (%p %p)\n", myCell2, myCell);
                            testStatus = 1;
                        }

                        pmCoordChipToFPA(&testCoord, &chipCoord, tmpChip);
                        if (fabs(testCoord.x - x) > FLT_EPSILON) {
                            printf("TEST ERROR: pmCoordChipToFPA() x coord was %.2f, should be %d\n", cellCoord.x, x);
                            testStatus = 1;
                        }
                        if (fabs(testCoord.y - fpaCoord.y) > FLT_EPSILON) {
                            printf("TEST ERROR: pmCoordChipToFPA() y coord was %.2f, should be %.2f\n", cellCoord.y, fpaCoord.y);
                            testStatus = 1;
                        }

                        pmCoordFPAToTP(&testCoord, &fpaCoord, 0.0, 0.0, myFPA);
                        if (fabs(testCoord.x - fpaCoord.x) > FLT_EPSILON) {
                            printf("TEST ERROR: pmCoordFPAToTP() x coord was %.2f, should be %d\n", cellCoord.x, x);
                            testStatus = 1;
                        }
                        if (fabs(testCoord.y - fpaCoord.y) > FLT_EPSILON) {
                            printf("TEST ERROR: pmCoordFPAToTP() y coord was %.2f, should be %d\n", cellCoord.y, y);
                            testStatus = 1;
                        }

                        //
                        // Test pmCoordTPToSky() -> pmCoordSkyToTP()
                        //
                        if (1) {
                            psSphere *rc = pmCoordTPToSky(skyCoord, &testCoord, myFPA->projection);
                            if (rc == NULL) {
                                printf("pmCoordTPToSky() failed.\n");
                            } else {
                                psPlane *rc = pmCoordSkyToTP(&tpCoord, skyCoord, myFPA->projection);
                                if (rc == NULL) {
                                    printf("pmCoordSkyToTP() failed.\n");
                                } else {
                                    if (fabs(testCoord.x - tpCoord.x) > FLT_EPSILON) {
                                        printf("TEST ERROR: pmCoordTPToSky()/pmCoordSkyToTP() x coord was %.2f, should be %.2f\n", tpCoord.x, testCoord.x);
                                        testStatus = 1;
                                    }
                                    if (fabs(testCoord.y - tpCoord.y) > FLT_EPSILON) {
                                        printf("TEST ERROR: pmCoordTPToSky()/pmCoordSkyToTP() y coord was %.2f, should be %.2f\n", tpCoord.y, testCoord.y);
                                        testStatus = 1;
                                    }
                                    if (VERBOSE) {
                                        printf("(%.2f %.2f) -> (%.2f %.2f) -> (%.2f %.2f)\n", testCoord.x, testCoord.y, skyCoord->r, skyCoord->d, tpCoord.x, tpCoord.y);
                                    }
                                }
                            }
                        }

                        //
                        // Test pmCoordCellToSky() -> pmCoordSkyToCell()
                        //
                        if (1) {
                            psPlane tmpCellCoord;
                            psSphere *rc = pmCoordCellToSky(skyCoord, &cellCoord, 0.0, 0.0, myCell);
                            if (rc == NULL) {
                                printf("pmCoordCellToSky() failed.\n");
                            } else {
                                psPlane *rc = pmCoordSkyToCell(&tmpCellCoord, skyCoord, 0.0, 0.0, myCell);
                                if (rc == NULL) {
                                    printf("pmCoordSkyToCell() failed.\n");
                                } else {
                                    if (fabs(cellCoord.x - tmpCellCoord.x) > FLT_EPSILON) {
                                        printf("TEST ERROR: pmCoordCellToSky()/pmCoordSkyToCell() x coord was %.2f, should be %.2f\n", tmpCellCoord.x, cellCoord.x);
                                        testStatus = 1;
                                    }
                                    if (fabs(cellCoord.y - tmpCellCoord.y) > FLT_EPSILON) {
                                        printf("TEST ERROR: pmCoordCellToSky()/pmCoordSkyToCell() y coord was %.2f, should be %.2f\n", tmpCellCoord.y, cellCoord.y);
                                        testStatus = 1;
                                    }
                                    if (VERBOSE) {
                                        printf("(%.2f %.2f) -> (%.2f %.2f) -> (%.2f %.2f)\n", testCoord.x, testCoord.y, skyCoord->r, skyCoord->d, tmpCellCoord.x, tmpCellCoord.y);
                                    }
                                }
                            }
                        }

                        //
                        // Test pmCoordCellToSkyQuick() -> pmCoordSkyToCellQuick()
                        // I'm not sure how to test this in a system with chip and cell gaps.
                        // There's no way to create an accurate polynomial transform from
                        // a cell to the sky where there are cell, or chip gaps.
                        //
                        if ((NUM_CHIPS == 1) && (NUM_CELLS == 1)) {
                            psPlane tmpCellCoord;
                            psSphere *rc = pmCoordCellToSkyQuick(skyCoord, &cellCoord, myCell);
                            if (rc == NULL) {
                                printf("pmCoordCellToSkyQuick() failed.\n");
                            } else {
                                psPlane *rc = pmCoordSkyToCellQuick(&tmpCellCoord, skyCoord, myCell);
                                if (rc == NULL) {
                                    printf("pmCoordSkyToCellQuick() failed.\n");
                                } else {
                                    if (fabs(cellCoord.x - tmpCellCoord.x) > FLT_EPSILON) {
                                        printf("TEST ERROR: pmCoordCellToSky()/pmCoordSkyToCell() x coord was %.2f, should be %.2f\n", tmpCellCoord.x, cellCoord.x);
                                        testStatus = 1;
                                    }
                                    if (fabs(cellCoord.y - tmpCellCoord.y) > FLT_EPSILON) {
                                        printf("TEST ERROR: pmCoordCellToSky()/pmCoordSkyToCell() y coord was %.2f, should be %.2f\n", tmpCellCoord.y, cellCoord.y);
                                        testStatus = 1;
                                    }
                                    if (VERBOSE) {
                                        printf("(%.2f %.2f) -> (%.2f %.2f) -> (%.2f %.2f)\n", testCoord.x, testCoord.y, skyCoord->r, skyCoord->d, tmpCellCoord.x, tmpCellCoord.y);
                                    }
                                }
                            }
                        }

                        pmCoordCellToFPA(&testCoord, &cellCoord, myCell);
                        if (fabs(testCoord.x - fpaCoord.x) > FLT_EPSILON) {
                            printf("TEST ERROR: pmCoordCellToFPA() x coord was %.2f, should be %d\n", cellCoord.x, x);
                            testStatus = 1;
                        }
                        if (fabs(testCoord.y - fpaCoord.y) > FLT_EPSILON) {
                            printf("TEST ERROR: pmCoordCellToFPA() y coord was %.2f, should be %d\n", cellCoord.y, y);
                            testStatus = 1;
                        }

                        pmCoordTPToFPA(&testCoord, &fpaCoord, 0.0, 0.0, myFPA);
                        if (fabs(testCoord.x - fpaCoord.x) > FLT_EPSILON) {
                            printf("TEST ERROR: pmCoordTPToFPA() x coord was %.2f, should be %d\n", cellCoord.x, x);
                            testStatus = 1;
                        }
                        if (fabs(testCoord.y - fpaCoord.y) > FLT_EPSILON) {
                            printf("TEST ERROR: pmCoordTPToFPA() y coord was %.2f, should be %d\n", cellCoord.y, y);
                            testStatus = 1;
                        }
                    }
                }
            }
        }
    }
    psFree(myFPA);
    psFree(skyCoord);

    return(testStatus);
}

/******************************************************************************
test4(): This routine wil test the pmFPACheckParents() function.  We generate
an pmFPA hierarchy, then set the parents of each readout/cell/chip to NULL,
then call pmFPACheckParents() to restore them, then we ensure they were
restored.
 *****************************************************************************/
psS32 test4( void )
{
    psS32 testStatus = 0;

    //
    // Generate a pmFPA hierarchy.
    //
    pmFPA *tmpFPA = genSystem();

    //
    // We set the parents of each readout/cell/chip to NULL.
    //
    for (psS32 chipID = 0; chipID < tmpFPA->chips->n ; chipID++) {
        pmChip *tmpChip = (pmChip *) tmpFPA->chips->data[chipID];
        tmpChip->parent = NULL;

        for (psS32 cellID = 0; cellID < tmpChip->cells->n ; cellID++) {
            pmCell *tmpCell = (pmCell *) tmpChip->cells->data[cellID];
            tmpCell->parent = NULL;

            for (psS32 readoutID = 0; readoutID < tmpCell->readouts->n ; readoutID++) {
                pmReadout *tmpReadout = (pmReadout *) tmpCell->readouts->data[readoutID];
                tmpReadout->parent = NULL;
            }
        }
    }

    //
    // Ensure that pmFPACheckParents() returned FALSE.
    //
    psBool rc = pmFPACheckParents(tmpFPA);
    if (rc != false) {
        printf("TEST ERROR: pmCheckParents() returned TRUE.\n");
        testStatus = 1;
    }

    //
    // Ensure that the parent members are right.
    //
    for (psS32 chipID = 0; chipID < tmpFPA->chips->n ; chipID++) {
        pmChip *tmpChip = (pmChip *) tmpFPA->chips->data[chipID];
        if (tmpChip->parent != tmpFPA) {
            printf("TEST ERROR: pmCheckParents() did not restore Chip->parent.\n");
            testStatus = 2;
        }

        for (psS32 cellID = 0; cellID < tmpChip->cells->n ; cellID++) {
            pmCell *tmpCell = (pmCell *) tmpChip->cells->data[cellID];
            if (tmpCell->parent != tmpChip) {
                printf("TEST ERROR: pmCheckParents() did not restore Cell->parent.\n");
                testStatus = 3;
            }

            for (psS32 readoutID = 0; readoutID < tmpCell->readouts->n ; readoutID++) {
                pmReadout *tmpReadout = (pmReadout *) tmpCell->readouts->data[readoutID];
                if (tmpReadout->parent != tmpCell) {
                    printf("TEST ERROR: pmCheckParents() did not restore Readout->parent.\n");
                    testStatus = 4;
                }
            }
        }
    }

    psFree(tmpFPA);
    return(testStatus);
}

/******************************************************************************
test5(): This routine wil test the pmFPASelectChip() and pmFPAExcludeChip()
functions.  We generate an pmFPA hierarchy, then set the ->valid members with
those routines, then verify.
 *****************************************************************************/
psS32 test5( void )
{
    psS32 testStatus = 0;
    pmChip *tmpChip = NULL;

    //
    // Generate a pmFPA hierarchy.
    //
    pmFPA *tmpFPA = genSystem();

    //
    // We test the ->valid member for each chip.
    //
    for (psS32 i = 0 ; i < tmpFPA->chips->n ; i++) {
        tmpChip = (pmChip *) tmpFPA->chips->data[i];
        if ((tmpChip == NULL) || (tmpChip->valid != false)) {
            printf("TEST ERROR: Could not properly generate an FPA hierarchy.\n");
            testStatus = 1;
        }
    }

    //
    // Exclude chip number 0, include all others, then test return value
    //
    psS32 numChips = pmFPAExcludeChip(tmpFPA, 0);
    if (numChips != (NUM_CHIPS-1)) {
        printf("TEST ERROR: pmFPAExcludeChip() did not return the correct number of chips.\n");
        testStatus = 2;
    }

    //
    // We test the ->valid member for each chip.
    //
    tmpChip = (pmChip *) tmpFPA->chips->data[0];
    if (tmpChip->valid != false) {
        printf("TEST ERROR: pmFPAExcludeChip() did not set the proper chip->valid to FALSE.\n");
        testStatus = 3;
    }
    for (psS32 i = 1 ; i < tmpFPA->chips->n ; i++) {
        pmChip *tmpChip = (pmChip *) tmpFPA->chips->data[i];
        if (tmpChip->valid != true) {
            printf("TEST ERROR: pmFPAExcludeChip() did not set the proper chip->valids to FALSE.\n");
            testStatus = 4;
        }
    }


    //
    // Include chip number 0, exclude all others, then test return value
    //
    psBool tmpBool = pmFPASelectChip(tmpFPA, 0);
    if (tmpBool != true) {
        printf("TEST ERROR: pmFPASelectChip() returned FALSE.\n");
        testStatus = 5;
    }

    //
    // We test the ->valid member for each chip.
    //
    tmpChip = (pmChip *) tmpFPA->chips->data[0];
    if (tmpChip->valid != true) {
        printf("TEST ERROR: pmFPASelectChip() did not set the proper chip->valid to FALSE.\n");
        testStatus = 6;
    }
    for (psS32 i = 1 ; i < tmpFPA->chips->n ; i++) {
        pmChip *tmpChip = (pmChip *) tmpFPA->chips->data[i];
        if (tmpChip->valid != false) {
            printf("TEST ERROR: pmFPASelectChip() did not set the proper chip->valids to FALSE.\n");
            testStatus = 7;
        }
    }

    psFree(tmpFPA);
    return(testStatus);
}

psS32 main( psS32 argc, char* argv[] )
{
    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    plan_tests(92);

    test3();
    test4();
    test5();
}
