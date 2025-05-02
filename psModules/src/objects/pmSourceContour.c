/** @file  pmSourceContour.c
 *
 *  Functions to measure the local sky and sky variance for sources on images
 *
 *  @author GLG, MHPCC
 *  @author EAM, IfA: significant modifications.
 *
 *  @version $Revision: 1.13 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2008-12-08 02:51:14 $
 *
 *  Copyright 2004 Maui High Performance Computing Center, University of Hawaii
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <math.h>
#include <string.h>
#include "pslib.h"
#include "pmHDU.h"
#include "pmFPA.h"

#include "pmTrend2D.h"
#include "pmResiduals.h"
#include "pmGrowthCurve.h"
#include "pmSpan.h"
#include "pmFootprintSpans.h"
#include "pmFootprint.h"
#include "pmPeaks.h"
#include "pmMoments.h"
#include "pmModelFuncs.h"
#include "pmModelClass.h"
#include "pmModel.h"
#include "pmModelUtils.h"
#include "pmSourceMasks.h"
#include "pmSourceExtendedPars.h"
#include "pmSourceDiffStats.h"
#include "pmSourceSatstar.h"
#include "pmSourceLensing.h"
#include "pmSource.h"

#include "pmSourceContour.h"

/******************************************************************************
findValue(source, level, row, col, dir): a private function which determines
the column coordinate of the model function which has the value "level".  If
dir equals 0, then you loop leftwards from the peak pixel, otherwise,
rightwards.
 
XXX: reverse order of row,col args?
 
XXX: Input row/col are in image coords.
 
XXX: The result is returned in image coords.
*****************************************************************************/
# define LEFT false
# define RIGHT true

// return the first coordinate at or below the threshold in the requested direction
static int findContourNeg(
    psImage *image,
    float threshold,
    int x,
    int y,
    bool right)
{

    psTrace("psModules.objects", 10, "---- %s() begin ----\n", __func__);

    // We define variables incr and lastColumn so that we can use the same loop
    // whether we are stepping leftwards, or rightwards.

    int incr;
    int subCol;
    int lastColumn;
    if (right) {
        incr = 1;
        lastColumn = image->numCols - 1;
    } else {
        incr = -1;
        lastColumn = 0;
    }

    subCol = x;

    while (subCol != lastColumn) {
        float value = image->data.F32[y][subCol];
        if (value <= threshold) {
            psTrace("psModules.objects", 10, "---- %s() end ----\n", __func__);
            return (subCol);
        }
        subCol += incr;
    }
    psTrace("psModules.objects", 10, "---- %s() end ----\n", __func__);
    return (lastColumn);
}

// return the last coordinate at or below the threshold in the requested direction
static int findContourPos(
    psImage *image,
    float threshold,
    int x,
    int y,
    bool right,
    int xEnd)
{

    psTrace("psModules.objects", 10, "---- %s() begin ----\n", __func__);

    // We define variables incr and lastColumn so that we can use the same loop
    // whether we are stepping leftwards, or rightwards.

    int incr;
    int subCol;
    int lastColumn;
    if (right) {
        incr = 1;
        lastColumn = PS_MIN (image->numCols - 1, xEnd);
    } else {
        incr = -1;
        lastColumn = PS_MAX (0, xEnd);
    }

    subCol = x;
    while (subCol != lastColumn) {
        float value = image->data.F32[y][subCol];
        if (value >= threshold) {
            psTrace("psModules.objects", 10, "---- %s() end ----\n", __func__);
            if (subCol == x) {
                return (subCol);
            }
            return (subCol);
        }
        subCol += incr;
    }
    psTrace("psModules.objects", 10, "---- %s() end ----\n", __func__);
    return (lastColumn);
}

/******************************************************************************
findValue(source, level, row, col, dir): a private function which determines
the column coordinate of the model function which has the value "level".  If
dir equals 0, then you loop leftwards from the peak pixel, otherwise,
rightwards.
 
XXX: reverse order of row,col args?
 
XXX: Input row/col are in image coords.
 
XXX: The result is returned in image coords.
*****************************************************************************/
static psF32 findValue(pmSource *source,
                       psF32 level,
                       psU32 row,
                       psU32 col,
                       psU32 dir)
{
    psTrace("psModules.objects", 10, "---- %s() begin ----\n", __func__);
    //
    // Convert coords to subImage space.
    //
    psU32 subRow = row - source->pixels->row0;
    psU32 subCol = col - source->pixels->col0;

    // Ensure that the starting column is allowable.
    if (!((0 <= subCol) && (subCol < source->pixels->numCols))) {
        psError(PS_ERR_UNKNOWN, true, "Starting column outside subImage range");
        psTrace("psModules.objects", 10, "---- %s(NAN) end ----\n", __func__);
        return(NAN);
    }
    if (!((0 <= subRow) && (subRow < source->pixels->numRows))) {
        psTrace("psModules.objects", 10, "---- %s(NAN) end ----\n", __func__);
        psError(PS_ERR_UNKNOWN, true, "Starting row outside subImage range");
        return(NAN);
    }

    // XXX EAM : i changed this to match pmModelEval above, but see
    // XXX EAM   the note below in pmSourceContour
    psF32 oldValue = pmModelEval(source->modelEXT, source->pixels, subCol, subRow);
    if (oldValue == level) {
        psTrace("psModules.objects", 10, "---- %s() end ----\n", __func__);
        return(((psF32) (subCol + source->pixels->col0)));
    }

    //
    // We define variables incr and lastColumn so that we can use the same loop
    // whether we are stepping leftwards, or rightwards.
    //
    psS32 incr;
    psS32 lastColumn;
    if (dir == 0) {
        incr = -1;
        lastColumn = -1;
    } else {
        incr = 1;
        lastColumn = source->pixels->numCols;
    }
    subCol+=incr;

    while (subCol != lastColumn) {
        psF32 newValue = pmModelEval(source->modelEXT, source->pixels, subCol, subRow);
        if (oldValue == level) {
            psTrace("psModules.objects", 10, "---- %s() end ----\n", __func__);
            return((psF32) (subCol + source->pixels->col0));
        }

        if ((newValue <= level) && (level <= oldValue)) {
            // This is simple linear interpolation.
            psTrace("psModules.objects", 10, "---- %s() end ----\n", __func__);
            return( ((psF32) (subCol + source->pixels->col0)) + ((psF32) incr) * ((level - newValue) / (oldValue - newValue)) );
        }

        if ((oldValue <= level) && (level <= newValue)) {
            // This is simple linear interpolation.
            psTrace("psModules.objects", 10, "---- %s() end ----\n", __func__);
            return( ((psF32) (subCol + source->pixels->col0)) + ((psF32) incr) * ((level - oldValue) / (newValue - oldValue)) );
        }

        subCol+=incr;
    }

    psTrace("psModules.objects", 10, "---- %s(NAN) end ----\n", __func__);
    return(NAN);
}

/******************************************************************************
new implementation of source contour function
*****************************************************************************/
psArray *pmSourceContour (psImage *image, int xc, int yc, float threshold)
{
    psTrace("psModules.objects", 10, "---- %s() begin ----\n", __func__);
    PS_ASSERT_PTR_NON_NULL(image, NULL);

    int xR, yR, x0, x1, x0s, x1s;
    int x = xc - image->col0;
    int y = yc - image->row0;

    // Ensure that the starting column is allowable.
    if (x < 0) {
        return NULL;
    }
    if (y < 0) {
        return NULL;
    }
    if (x >= image->numCols) {
        return NULL;
    }
    if (y >= image->numRows) {
        return NULL;
    }

    // the requested point must be within the contour
    if (image->data.F32[y][x] < threshold) {
        return NULL;
    }

    // Allocate data for x/y pairs.
    psVector *xVec = psVectorAllocEmpty(100, PS_TYPE_F32);
    psVector *yVec = psVectorAllocEmpty(100, PS_TYPE_F32);

    // First row: find the left and right end-points
    int Npt = 0;

    x0 = findContourNeg (image, threshold, x, y, LEFT);
    x1 = findContourNeg (image, threshold, x, y, RIGHT);
    xVec->data.F32[Npt + 0] = image->col0 + x0;
    xVec->data.F32[Npt + 1] = image->col0 + x1;
    yVec->data.F32[Npt + 0] = image->row0 + y;
    yVec->data.F32[Npt + 1] = image->row0 + y;
    Npt += 2;

    x0s = x0;
    x1s = x1;

    // look for contour outline above row
    xR = x0s;
    yR = y + 1;
    while (yR < image->numRows) {
        if (image->data.F32[yR][xR] < threshold) {
            x0 = findContourPos (image, threshold, xR, yR, RIGHT, x1);
            if (x0 == x1) {
	      // fprintf (stderr, "top: %d (%d - %d)\n", yR, xR, x1);
                goto pt1;
            }
            x1 = findContourNeg (image, threshold, x0, yR, RIGHT);
            x0--;
        } else {
            x0 = findContourNeg (image, threshold, xR, yR, LEFT);
            x1 = findContourNeg (image, threshold, xR, yR, RIGHT);
        }
	// fprintf (stderr, "pos: %d (%d - %d)\n", yR, x0, x1);

        xVec->data.F32[Npt + 0] = image->col0 + x0;
        xVec->data.F32[Npt + 1] = image->col0 + x1;
        yVec->data.F32[Npt + 0] = image->row0 + yR;
        yVec->data.F32[Npt + 1] = image->row0 + yR;

        Npt += 2;

        if (Npt >= xVec->nalloc - 1) {
            psVectorRealloc (xVec, xVec->nalloc + 100);
            psVectorRealloc (yVec, yVec->nalloc + 100);
        }
        yR ++;
        xR = x0;
    }

pt1:
    // look for contour outline below row
    xR = x0s;
    x1 = x1s;
    yR = y - 1;
    while (yR >= 0) {
        if (image->data.F32[yR][xR] < threshold) {
            x0 = findContourPos (image, threshold, xR, yR, RIGHT, x1);
            if (x0 == x1) {
	      // fprintf (stderr, "top: %d (%d - %d)\n", yR, xR, x1);
                goto pt2;
            }
            x1 = findContourNeg (image, threshold, x0, yR, RIGHT);
            x0--;
        } else {
            x0 = findContourNeg (image, threshold, xR, yR, LEFT);
            x1 = findContourNeg (image, threshold, xR, yR, RIGHT);
        }
        // fprintf (stderr, "neg: %d (%d - %d)\n", yR, x0, x1);

        xVec->data.F32[Npt + 0] = image->col0 + x0;
        xVec->data.F32[Npt + 1] = image->col0 + x1;
        yVec->data.F32[Npt + 0] = image->row0 + yR;
        yVec->data.F32[Npt + 1] = image->row0 + yR;

        Npt += 2;

        if (Npt >= xVec->nalloc - 1) {
            psVectorRealloc (xVec, xVec->nalloc + 100);
            psVectorRealloc (yVec, yVec->nalloc + 100);
        }
        yR --;
    }
pt2:
    xVec->n = Npt;
    yVec->n = Npt;

    psArray *tmpArray = psArrayAlloc(2);

    tmpArray->data[0] = (psPtr *) xVec;
    tmpArray->data[1] = (psPtr *) yVec;
    psTrace("psModules.objects", 10, "---- %s() end ----\n", __func__);
    return(tmpArray);
}

/******************************************************************************
    pmSourceContour(src, img, level, mode): For an input subImage, and model, this
    routine returns a psArray of coordinates that evaluate to the specified level.
 
    XXX: Probably should remove the "image" argument.
    XXX: What type should the output coordinate vectors consist of?  col,row?
    XXX: Why a pmArray output?
    XXX: doex x,y correspond with col,row or row/col?
    XXX: What is mode?
    XXX: The top, bottom of the contour is not correctly determined.
    XXX EAM : this function is using the model for the contour, but it should
              be using only the image counts
*****************************************************************************/
psArray *pmSourceContour_Crude(pmSource *source,
                               const psImage *image,
                               psF32 level)
{
    psTrace("psModules.objects", 10, "---- %s() begin ----\n", __func__);
    PS_ASSERT_PTR_NON_NULL(source, false);
    PS_ASSERT_PTR_NON_NULL(image, false);
    PS_ASSERT_PTR_NON_NULL(source->moments, false);
    PS_ASSERT_PTR_NON_NULL(source->peak, false);
    PS_ASSERT_PTR_NON_NULL(source->pixels, false);
    PS_ASSERT_PTR_NON_NULL(source->modelEXT, false);
    // XXX EAM : what is the purpose of modelPSF/modelEXT?

    //
    // Allocate data for x/y pairs.
    //
    psVector *xVec = psVectorAlloc(2 * source->pixels->numRows, PS_TYPE_F32);
    psVector *yVec = psVectorAlloc(2 * source->pixels->numRows, PS_TYPE_F32);

    //
    // Start at the row with peak pixel, then decrement.
    //
    psS32 col = source->peak->x;
    for (psS32 row = source->peak->y; row>= 0 ; row--) {
        // XXX: yVec contain no real information.  Do we really need it?
        yVec->data.F32[row] = (psF32) (source->pixels->row0 + row);
        yVec->data.F32[row+yVec->n] = (psF32) (source->pixels->row0 + row);

        // Starting at peak pixel, search leftwards for the column intercept.
        psF32 leftIntercept = findValue(source, level, row, col, 0);
        if (isnan(leftIntercept)) {
            psError(PS_ERR_UNKNOWN, true, "Could not find contour edge (NAN)");
            psFree(xVec);
            psFree(yVec);
            psTrace("psModules.objects", 10, "---- %s(NULL) end ----\n", __func__);
            return(NULL);
            //psLogMsg(__func__, PS_LOG_WARN, "WARNING: Could not find contour edge (NAN)\n");
        }
        xVec->data.F32[row] = ((psF32) source->pixels->col0) + leftIntercept;

        // Starting at peak pixel, search rightwards for the column intercept.

        psF32 rightIntercept = findValue(source, level, row, col, 1);
        if (isnan(rightIntercept)) {
            psError(PS_ERR_UNKNOWN, true, "Could not find contour edge (NAN)");
            psFree(xVec);
            psFree(yVec);
            psTrace("psModules.objects", 10, "---- %s(NULL) end ----\n", __func__);
            return(NULL);
            //psLogMsg(__func__, PS_LOG_WARN, "WARNING: Could not find contour edge (NAN)\n");
        }
        psTrace("psModules.objects", 4, "The intercepts are (%.2f, %.2f)\n", leftIntercept, rightIntercept);
        xVec->data.F32[row+xVec->n] = ((psF32) source->pixels->col0) + rightIntercept;

        // Set starting column for next row
        col = (psS32) ((leftIntercept + rightIntercept) / 2.0);
    }
    //
    // Start at the row (+1) with peak pixel, then increment.
    //
    col = source->peak->x;
    for (psS32 row = 1 + source->peak->y; row < source->pixels->numRows ; row++) {
        // XXX: yVec contain no real information.  Do we really need it?
        yVec->data.F32[row] = (psF32) (source->pixels->row0 + row);
        yVec->data.F32[row+yVec->n] = (psF32) (source->pixels->row0 + row);

        // Starting at peak pixel, search leftwards for the column intercept.
        psF32 leftIntercept = findValue(source, level, row, col, 0);
        if (isnan(leftIntercept)) {
            psError(PS_ERR_UNKNOWN, true, "Could not find contour edge (NAN)");
            psFree(xVec);
            psFree(yVec);
            psTrace("psModules.objects", 10, "---- %s(NULL) end ----\n", __func__);
            return(NULL);
            //psLogMsg(__func__, PS_LOG_WARN, "WARNING: Could not find contour edge (NAN)\n");
        }
        xVec->data.F32[row] = ((psF32) source->pixels->col0) + leftIntercept;

        // Starting at peak pixel, search rightwards for the column intercept.
        psF32 rightIntercept = findValue(source, level, row, col, 1);
        if (isnan(rightIntercept)) {
            psError(PS_ERR_UNKNOWN, true, "Could not find contour edge (NAN)");
            psFree(xVec);
            psFree(yVec);
            psTrace("psModules.objects", 10, "---- %s(NULL) end ----\n", __func__);
            return(NULL);
            //psLogMsg(__func__, PS_LOG_WARN, "WARNING: Could not find contour edge (NAN)\n");
        }
        xVec->data.F32[row+xVec->n] = ((psF32) source->pixels->col0) + rightIntercept;

        // Set starting column for next row
        col = (psS32) ((leftIntercept + rightIntercept) / 2.0);
    }

    //
    // Allocate an array for result, store coord vectors there.
    //
    psArray *tmpArray = psArrayAlloc(2);

    tmpArray->data[0] = (psPtr *) yVec;
    tmpArray->data[1] = (psPtr *) xVec;
    psTrace("psModules.objects", 10, "---- %s() end ----\n", __func__);
    return(tmpArray);
}
