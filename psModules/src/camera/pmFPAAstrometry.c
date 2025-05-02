#include <stdio.h>
#include "pslib.h"
#include "pmFPAAstrometry.h"
#include "pmFPA.h"

/********** 

	    EAM : this file is not included in the psModules build
	    alternative versions are defined by psastroMaskUtils.c

 *********/


/*****************************************************************************
checkValidImageCoords(): this is a private function which simply determines if
the supplied x,y coordinates are in the range for the supplied psImage.
 
XXX: What about col0 and row0
XXX: This should return a psBool.
XXX: Macro this for speed.
 *****************************************************************************/
static psS32 checkValidImageCoords(
    double x,
    double y,
    psImage* tmpImage)
{
    PS_ASSERT_IMAGE_NON_NULL(tmpImage, 0);

    // The FLT_EPSILON is because -0.0 was failing this.
    if (((x+FLT_EPSILON) < 0.0) || (x > (double)tmpImage->numCols) ||
            ((y+FLT_EPSILON) < 0.0) || (y > (double)tmpImage->numRows)) {
        return (0);
    }

    return (1);
}

/*****************************************************************************/
/* FUNCTION IMPLEMENTATION - PUBLIC                                          */
/*****************************************************************************/

pmCell* pmCellInFPA(
    const psPlane* fpaCoord,
    const pmFPA* FPA)
{
    PS_ASSERT_PTR_NON_NULL(fpaCoord, NULL);
    PS_ASSERT_PTR_NON_NULL(FPA, NULL);

    pmChip* tmpChip = NULL;
    psPlane chipCoord;
    pmCell* outCell = NULL;

    // Determine which chip contains the fpaCoords.
    tmpChip = pmChipInFPA(fpaCoord, FPA);
    if (tmpChip == NULL) {
        return(NULL);
    }

    // Convert to those chip coordinates.
    psPlane *rc = pmCoordFPAToChip(&chipCoord, fpaCoord, tmpChip);
    if (rc == NULL) {
        psLogMsg(__func__, PS_LOG_WARN, "WARNING: could not determine Chip coords.\n");
        return(NULL);
    }

    // Determine which cell contains those chip coordinates.
    outCell = pmCellInChip(&chipCoord, tmpChip);
    if (outCell == NULL) {
        psLogMsg(__func__, PS_LOG_WARN, "WARNING: could not determine the cell.\n");
        return(NULL);
    }

    return (outCell);
}

pmChip* pmChipInFPA(
    const psPlane* fpaCoord,
    const pmFPA* FPA)
{
    PS_ASSERT_PTR_NON_NULL(fpaCoord, NULL);
    PS_ASSERT_PTR_NON_NULL(FPA, NULL);
    PS_ASSERT_PTR_NON_NULL(FPA->chips, NULL);

    psArray* chips = FPA->chips;
    psS32 nChips = chips->n;
    psPlane chipCoord;
    pmCell *tmpCell = NULL;

    //
    // Loop through every chip in this FPA.  Convert the original FPA
    // coordinates to chip coordinates for that chip.  Then, determine if any
    // cells in that chip contain those chip coordinates.
    // XXX: Depending on the number of chips, and their topology, there may be
    // a much more efficient way of doing this.
    //
    for (psS32 i = 0; i < nChips; i++) {
        pmChip* tmpChip = chips->data[i];
        PS_ASSERT_PTR_NON_NULL(tmpChip, NULL);
        PS_ASSERT_PTR_NON_NULL(tmpChip->fromFPA, NULL);

        psPlaneTransformApply(&chipCoord, tmpChip->fromFPA, fpaCoord);

        tmpCell = pmCellInChip(&chipCoord, tmpChip);
        if (tmpCell != NULL) {
            return(tmpChip);
        }
    }

    psLogMsg(__func__, PS_LOG_WARN, "WARNING: could not determine the chip.\n");
    return (NULL);
}


// EAM : this function does not handle non-linear transformations.  is it used?
pmCell* pmCellInChip(
    const psPlane* chipCoord,
    const pmChip* chip)
{
    PS_ASSERT_PTR_NON_NULL(chipCoord, NULL);
    PS_ASSERT_PTR_NON_NULL(chip, NULL);

    psPlane cellCoord;
    psArray* cells;

    cells = chip->cells;
    if (cells == NULL) {
        return NULL;
    }

    //
    // We loop over each cell in the chip.  We transform the chipCoord into
    // a cellCoord for that cell and determine if that cellCoord is valid.
    // If so, then we return that cell.
    // XXX: Depending on the number of cells, and their topology, there may be
    // a much more efficient way of doing this.
    //
    for (psS32 i = 0; i < cells->n; i++) {
        pmCell* tmpCell = (pmCell* ) cells->data[i];
        PS_ASSERT_PTR_NON_NULL(tmpCell, NULL);

        psPlaneTransform *chipToCell = NULL;
        if (true ==  p_psIsProjectionLinear(tmpCell->toChip)) {
            chipToCell = p_psPlaneTransformLinearInvert(tmpCell->toChip);
        } else {
            psLogMsg(__func__, PS_LOG_WARN, "WARNING: non-linear cell->chip transforms are not yet implemented.\n");
            //chipToCell = psPlaneTransformInvert(NULL, tmpCell->toChip, NULL, -1);
            chipToCell = NULL;
        }
        if (chipToCell == NULL) {
            psLogMsg(__func__, PS_LOG_WARN, "WARNING: could not invert the Cell->toChip transform.\n");
            return(NULL);
        }
        psArray* readouts = tmpCell->readouts;

        if (readouts != NULL) {
            for (psS32 j = 0; j < readouts->n; j++) {
                pmReadout* tmpReadout = readouts->data[j];
                PS_ASSERT_READOUT_NON_NULL(tmpReadout, NULL);

                psPlaneTransformApply(&cellCoord,
                                      chipToCell,
                                      chipCoord);

                if (checkValidImageCoords(cellCoord.x,
                                          cellCoord.y,
                                          tmpReadout->image)) {
                    psFree(chipToCell);
                    return (tmpCell);
                }
            }
        }
        psFree(chipToCell);
    }

    //psLogMsg(__func__, PS_LOG_WARN, "WARNING: could not determine the cell.\n");
    return (NULL);
}


psPlane* pmCoordCellToFPA(
    psPlane* fpaCoord,
    const psPlane* cellCoord,
    const pmCell* cell)
{
    PS_ASSERT_PTR_NON_NULL(cellCoord, NULL);
    PS_ASSERT_PTR_NON_NULL(cell, NULL);

    psPlane *rc = psPlaneTransformApply(fpaCoord, cell->toFPA, cellCoord);
    if (rc == NULL) {
        psLogMsg(__func__, PS_LOG_WARN, "WARNING: could not transform cell coords to FPA coords.\n");
    }
    return(rc);
}


psPlane* pmCoordChipToFPA(
    psPlane* outCoord,
    const psPlane* inCoord,
    const pmChip* chip)
{
    PS_ASSERT_PTR_NON_NULL(inCoord, NULL);
    PS_ASSERT_PTR_NON_NULL(chip, NULL);

    psPlane *rc = psPlaneTransformApply(outCoord, chip->toFPA, inCoord);
    if (rc == NULL) {
        psLogMsg(__func__, PS_LOG_WARN, "WARNING: could not transform chip coords to FPA coords.\n");
    }
    return(rc);
}


psPlane* pmCoordFPAToChip(
    psPlane* chipCoord,
    const psPlane* fpaCoord,
    const pmChip* chip)
{
    PS_ASSERT_PTR_NON_NULL(fpaCoord, NULL);
    PS_ASSERT_PTR_NON_NULL(chip, NULL);
    PS_ASSERT_PTR_NON_NULL(chip->fromFPA, NULL);

    psPlane *rc = psPlaneTransformApply(chipCoord, chip->fromFPA, fpaCoord);
    if (rc == NULL) {
        psLogMsg(__func__, PS_LOG_WARN, "WARNING: could not transform FPA coords to Chip coords.\n");
    }
    return(rc);
}

psPlane* pmCoordCellToChip(
    psPlane* outCoord,
    const psPlane* inCoord,
    const pmCell* cell)
{
    PS_ASSERT_PTR_NON_NULL(inCoord, NULL);
    PS_ASSERT_PTR_NON_NULL(cell, NULL);

    psPlane *rc = psPlaneTransformApply(outCoord, cell->toChip, inCoord);
    if (rc == NULL) {
        psLogMsg(__func__, PS_LOG_WARN, "WARNING: could not transform Cell coords to Chip coords.\n");
    }
    return(rc);
}

// EAM : this function does not handle non-linear transformations.  is it used?
psPlane* pmCoordChipToCell(
    psPlane* cellCoord,
    const psPlane* chipCoord,
    const pmCell* cell)
{
    PS_ASSERT_PTR_NON_NULL(chipCoord, NULL);
    PS_ASSERT_PTR_NON_NULL(cell, NULL);
    PS_ASSERT_PTR_NON_NULL(cell->parent, NULL);

    pmCell *tmpCell = pmCellInChip(chipCoord, cell->parent);
    if (tmpCell == NULL) {
        psLogMsg(__func__, PS_LOG_WARN, "WARNING: could not determine the proper cell.\n");
        return(NULL);
    }

    psPlaneTransform *tmpChipToCell = NULL;
    PS_ASSERT_PTR_NON_NULL(tmpCell->toChip, NULL);
    if (true ==  p_psIsProjectionLinear(tmpCell->toChip)) {
        tmpChipToCell = p_psPlaneTransformLinearInvert(tmpCell->toChip);
    } else {
        psLogMsg(__func__, PS_LOG_WARN, "WARNING: non-linear cell->chip transforms are not yet implemented.\n");
        // XXX: tmpChipToCell = psPlaneTransformInvert(NULL, tmpCell->toChip, NULL, -1);
        tmpChipToCell = NULL;
    }
    if (tmpChipToCell == NULL) {
        psLogMsg(__func__, PS_LOG_WARN, "WARNING: could not invert the Cell->toChip transform.\n");
        return(NULL);
    }

    psPlane *rc = psPlaneTransformApply(cellCoord, tmpChipToCell, chipCoord);
    if (rc == NULL) {
        psLogMsg(__func__, PS_LOG_WARN, "WARNING: could not transform Chip coords to Cell coords.\n");
    }
    psFree(tmpChipToCell);
    return(rc);
}

psPlane* pmCoordFPAToTP(
    psPlane* outCoord,
    const psPlane* inCoord,
    double color,
    double magnitude,
    const pmFPA* fpa)
{
    PS_ASSERT_PTR_NON_NULL(inCoord, NULL);
    PS_ASSERT_PTR_NON_NULL(fpa, NULL);

    psPlane *rc = psPlaneDistortApply(outCoord, fpa->toTangentPlane, inCoord, color, magnitude);
    if (rc == NULL) {
        psLogMsg(__func__, PS_LOG_WARN, "WARNING: could not transform FPA coords to tangent plane coords.\n");
    }
    return(rc);
}

psPlane* pmCoordTPToFPA(
    psPlane* fpaCoord,
    const psPlane* tpCoord,
    double color,
    double magnitude,
    const pmFPA* fpa)
{
    PS_ASSERT_PTR_NON_NULL(tpCoord, NULL);
    PS_ASSERT_PTR_NON_NULL(fpa, NULL);
    PS_ASSERT_PTR_NON_NULL(fpa->fromTangentPlane, NULL);

    psPlane *rc = psPlaneDistortApply(fpaCoord, fpa->fromTangentPlane, tpCoord, color, magnitude);
    if (rc == NULL) {
        psLogMsg(__func__, PS_LOG_WARN, "WARNING: could not transform tangent plane coords to FPA coords.\n");
    }
    return(rc);
}


/*****************************************************************************
XXXDeproject(outSphere, coord, projection): This private routine is a wrapper
for p_psDeproject().  The reason: p_psDeproject() and p_psProject() combined
do not seem to produce the original coordinates when they even though they
should.  XXXDeproject() simply negates the ->r and ->d members of the output
psSphere if the input ->y is larger than 0.0.  I don't know why it works.
 
I'm guessing the p_psProject() and p_psDeproject() functions have bugs.
 
XXX: It appears that p_psProject() and p_psDeproject() have been fixed.
Remove this.
 *****************************************************************************/
psSphere* XXXDeproject(
    psSphere *outSphere,
    const psPlane* coord,
    const psProjection* projection)
{
    psSphere *rc = p_psDeproject(outSphere, coord, projection);

    if (coord->y >= 0.0) {
        rc->d = -rc->d;
        rc->r = -rc->r;
    }

    return(rc);
}

/*****************************************************************************
  *****************************************************************************/
psSphere* pmCoordTPToSky(
    psSphere* outSphere,
    const psPlane* tpCoord,
    const psProjection *projection)
{
    PS_ASSERT_PTR_NON_NULL(tpCoord, NULL);
    PS_ASSERT_PTR_NON_NULL(projection, NULL);

    //    psSphere *rc = XXXDeproject(outSphere, tpCoord, projection);
    psSphere *rc = p_psDeproject(outSphere, tpCoord, projection);
    if (rc == NULL) {
        psLogMsg(__func__, PS_LOG_WARN, "WARNING: could not transform tangent plane coords to sky coords.\n");
    }
    return(rc);
}

/*****************************************************************************
 *****************************************************************************/
psPlane* pmCoordSkyToTP(
    psPlane* tpCoord,
    const psSphere* in,
    const psProjection *projection)
{
    PS_ASSERT_PTR_NON_NULL(in, NULL);
    PS_ASSERT_PTR_NON_NULL(projection, NULL);

    psPlane *rc = p_psProject(tpCoord, in, projection);
    if (rc == NULL) {
        psLogMsg(__func__, PS_LOG_WARN, "WARNING: could not transform sky to tangent plane coords.\n");
    }
    return(rc);
}

/*****************************************************************************
 *****************************************************************************/
psSphere* pmCoordCellToSky(
    psSphere* skyCoord,
    const psPlane* cellCoord,
    double color,
    double magnitude,
    const pmCell* cell)
{
    PS_ASSERT_PTR_NON_NULL(cellCoord, NULL);
    PS_ASSERT_PTR_NON_NULL(cell, NULL);
    PS_ASSERT_PTR_NON_NULL(cell->toFPA, NULL);
    PS_ASSERT_PTR_NON_NULL(cell->parent, NULL);
    PS_ASSERT_PTR_NON_NULL(cell->parent->parent, NULL);
    PS_ASSERT_PTR_NON_NULL(cell->parent->parent->toTangentPlane, NULL);
    PS_ASSERT_PTR_NON_NULL(cell->parent->parent->projection, NULL);
    psPlane fpaCoord;
    psPlane tpCoord;
    psPlane *rc;
    pmFPA* parFPA = (cell->parent)->parent;

    // Convert the input cell coordinates to FPA coordinates.
    rc = psPlaneTransformApply(&fpaCoord, cell->toFPA, cellCoord);
    if (rc == NULL) {
        psLogMsg(__func__, PS_LOG_WARN, "WARNING: could transform cell coords to FPA coords.\n");
        return(NULL);
    }

    // Convert the FPA coordinates to tangent plane Coordinates.
    rc = psPlaneDistortApply(&tpCoord, parFPA->toTangentPlane, &fpaCoord, color, magnitude);
    if (rc == NULL) {
        psLogMsg(__func__, PS_LOG_WARN, "WARNING: could transform FPA coords to tangent plane coords.\n");
        return(NULL);
    }

    // Convert the tangent plane Coordinates to sky coordinates.
    psSphere *rc2 = pmCoordTPToSky(skyCoord, &tpCoord, parFPA->projection);
    if (rc2 == NULL) {
        psLogMsg(__func__, PS_LOG_WARN, "WARNING: could not transform cell coords to sky coords.\n");
    }

    return(rc2);
}

/*****************************************************************************
 *****************************************************************************/
psPlane* pmCoordSkyToCell(
    psPlane* cellCoord,
    const psSphere* skyCoord,
    float color,
    float magnitude,
    const pmCell* cell)
{
    PS_ASSERT_PTR_NON_NULL(skyCoord, NULL);
    PS_ASSERT_PTR_NON_NULL(cell, NULL);
    PS_ASSERT_PTR_NON_NULL(cell->parent, NULL);
    PS_ASSERT_PTR_NON_NULL(cell->parent->parent, NULL);
    pmChip *parChip = cell->parent;
    pmFPA *parFPA = parChip->parent;
    psPlane tpCoord;
    psPlane fpaCoord;
    psPlane chipCoord;
    psPlane *rc;

    // Convert the skyCoords to tangent plane coords.
    rc = pmCoordSkyToTP(&tpCoord, skyCoord, parFPA->projection);
    if (rc == NULL) {
        psLogMsg(__func__, PS_LOG_WARN, "WARNING: could not determine tangent plane coords.\n");
        return(NULL);
    }

    // Convert the tangent plane coords to FPA coords.
    rc = pmCoordTPToFPA(&fpaCoord, &tpCoord, color, magnitude, parFPA);
    if (rc == NULL) {
        psLogMsg(__func__, PS_LOG_WARN, "WARNING: could not determine FPA coords.\n");
        return(NULL);
    }

    // Convert the FPA coords to chip coords.
    rc = pmCoordFPAToChip(&chipCoord, &fpaCoord, parChip);
    if (rc == NULL) {
        psLogMsg(__func__, PS_LOG_WARN, "WARNING: could not determine chip coords.\n");
        return(NULL);
    }

    // Convert the chip coords to cell coords.
    rc = pmCoordChipToCell(cellCoord, &chipCoord, cell);
    if (rc == NULL) {
        psLogMsg(__func__, PS_LOG_WARN, "WARNING: could not determine cell coords.\n");
        return(NULL);
    }

    return (cellCoord);
}

/*****************************************************************************
 *****************************************************************************/
psSphere* pmCoordCellToSkyQuick(
    psSphere* outSphere,
    const psPlane* cellCoord,
    const pmCell* cell)
{
    PS_ASSERT_PTR_NON_NULL(cellCoord, NULL);
    PS_ASSERT_PTR_NON_NULL(cell, NULL);
    PS_ASSERT_PTR_NON_NULL(cell->toSky, NULL);
    psPlane outPlane;
    psPlane *rc;
    rc = psPlaneTransformApply(&outPlane, cell->toSky, cellCoord);
    if (rc == NULL) {
        psLogMsg(__func__, PS_LOG_WARN, "WARNING: could transform cell coords to sky coords.\n");
        return(NULL);
    }

    psSphere *out = outSphere;
    if (out == NULL) {
        out = psSphereAlloc();
    }
    out->r = outPlane.y;
    out->d = outPlane.x;

    return(out);
}

/*****************************************************************************
 *****************************************************************************/
psPlane* pmCoordSkyToCellQuick(
    psPlane* cellCoord,
    const psSphere* skyCoord,
    const pmCell* cell)
{
    PS_ASSERT_PTR_NON_NULL(skyCoord, NULL);
    PS_ASSERT_PTR_NON_NULL(cell, NULL);
    PS_ASSERT_PTR_NON_NULL(cell->toSky, NULL);
    psPlane skyPlane;
    skyPlane.y = skyCoord->r;
    skyPlane.x = skyCoord->d;

    psPlane *rc = psPlaneTransformApply(cellCoord, cell->toSky, &skyPlane);
    if (rc == NULL) {
        psLogMsg(__func__, PS_LOG_WARN, "WARNING: could not transform sky to cell coords.\n");
    }
    return(cellCoord);
}
