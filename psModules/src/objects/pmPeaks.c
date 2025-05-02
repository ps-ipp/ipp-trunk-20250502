/** @file  pmPeaks.c
 *
 *  This file defines functions to detect and manipulate peaks in images
 *
 *  @author GLG, MHPCC
 *  @author EAM, IfA: significant modifications.
 *
 *  @version $Revision: 1.26 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-16 22:30:14 $
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
#include <pslib.h>
#include "pmSpan.h"
#include "pmFootprintSpans.h"
#include "pmFootprint.h"
#include "pmPeaks.h"

/******************************************************************************
AddPeak(): A private function which allocates a psArray, if the peaks
argument is NULL, otherwise it adds the peak to that array.
XXX EAM : row,col now refer to image coords, NOT parent (since this is private) 
XXX EAM : now also calculates fractional peak positions from 3x3 bicube region
*****************************************************************************/
static psArray *AddPeak(psArray *peaks,
                        const psImage *image,
                        psS32 row,
                        psS32 col,
                        pmPeakType type)
{
    psTrace("psModules.objects", 10, "---- begin ----\n");

    if (peaks == NULL) {
        peaks = psArrayAllocEmpty(100);
    }

    // the peak position is in parent coordinates
    pmPeak *peak = pmPeakAlloc(col + image->col0, row + image->row0, image->data.F32[row][col], type);

    // measure fractional peak position using the 3x3 bicube fit

    // ix,iy must land on image with 1 pixel border
    int ix = PS_MAX (PS_MIN (col, image->numCols - 2), 1);
    int iy = PS_MAX (PS_MIN (row, image->numRows - 2), 1);

    // calculate peak position relative to ix,iy
    // XXX these functions need to take a mask, weight, and calculate the errors
    psPolynomial2D *bicube = psImageBicubeFit (image, ix + image->col0, iy + image->row0);
    psPlane min = psImageBicubeMin (bicube);
    psFree (bicube);

    // if min point is too deviant, use the peak value
    // XXX need to calculate dx, dy correctly
    // 0.5 PIX: peaks are calculated using the pixel index and converted here to pixel coords
    if ((fabs(min.x) < 1.5) && (fabs(min.y) < 1.5)) {
        peak->xf = min.x + ix + image->col0 + 0.5;
        peak->yf = min.y + iy + image->row0 + 0.5;

	// These errors are fractional errors, and should be scaled by the 
	// error on the peak pixel (see, eg, psphotFindPeaks)
	peak->dx = min.xErr;
	peak->dy = min.yErr;
	
	// xf,yf must land on image with 0 pixel border
	peak->xf = PS_MAX (PS_MIN (peak->xf, image->numCols - 1), image->col0);
	peak->yf = PS_MAX (PS_MIN (peak->yf, image->numRows - 1), image->row0);
    } else {
        peak->xf = ix + 0.5;
        peak->yf = iy + 0.5; 
	peak->dx = NAN;
	peak->dy = NAN;
    }

    psArrayAdd(peaks, 100, peak);
    psFree (peak);

    psTrace("psModules.objects", 10, "---- end ----\n");
    return(peaks);
}

/******************************************************************************
getRowVectorFromImage(): a private function which simply returns a
psVector containing the specified row of data from the psImage.
 
XXX: Is there a better way to do this?
XXX EAM: does this really need to alloc a new vector???
*****************************************************************************/
static psVector *getRowVectorFromImage(psImage *image,
                                       psU32 row)
{
    psTrace("psModules.objects", 10, "---- %s() begin ----\n", __func__);
    PS_ASSERT_IMAGE_NON_NULL(image, NULL);
    PS_ASSERT_IMAGE_TYPE(image, PS_TYPE_F32, NULL);

    psVector *tmpVector = psVectorAlloc(image->numCols, PS_TYPE_F32);
    for (psU32 col = 0; col < image->numCols ; col++) {
        tmpVector->data.F32[col] = image->data.F32[row][col];
    }
    psTrace("psModules.objects", 10, "---- %s() end ----\n", __func__);
    return(tmpVector);
}

/******************************************************************************
isItInThisRegion(): a private function which simply returns a
boolean denoting if specified coordinate is in the region.
XXX: Macro this.
*****************************************************************************/
# if (0)
static bool isItInThisRegion(const psRegion valid,
                             psS32 x,
                             psS32 y)
{
    psTrace("psModules.objects", 10, "---- %s() begin ----\n", __func__);
    if ((x >= valid.x0) &&
            (x <= valid.x1) &&
            (y >= valid.y0) &&
            (y <= valid.y1)) {
        psTrace("psModules.objects", 10, "---- %s(true) end ----\n", __func__);
        return(true);
    }
    psTrace("psModules.objects", 10, "---- %s(false) end ----\n", __func__);
    return(false);
}
# endif

/******************************************************************************
pmPeakAlloc(): Allocate the pmPeak data structure and set appropriate members.
*****************************************************************************/
static void peakFree(pmPeak *tmp)
{
    if (!tmp) return;
    psFree (tmp->saddlePoints);
    return;
}

pmPeak *pmPeakAlloc(psS32 x,
                    psS32 y,
                    psF32 value,
                    pmPeakType type)
{
    psTrace("psModules.objects", 10, "---- %s() begin ----\n", __func__);
    static int id = 1;
    pmPeak *tmp = (pmPeak *) psAlloc(sizeof(pmPeak));
    *(int *)&tmp->id = id++;
    tmp->x = x;
    tmp->y = y;
    tmp->xf = x;
    tmp->yf = y;
    tmp->dx = NAN;
    tmp->dy = NAN;
    tmp->detValue      	 = value;
    tmp->rawFlux       	 = value; // set this by default: it is up to the user to supply a better value
    tmp->rawFluxStdev  	 = NAN;
    tmp->smoothFlux    	 = value; // set this by default: it is up to the user to supply a better value
    tmp->smoothFluxStdev = NAN;
    tmp->assigned = false;
    tmp->type = type;
    tmp->footprint = NULL;
    tmp->saddlePoints = NULL;

    psMemSetDeallocator(tmp, (psFreeFunc) peakFree);

    psTrace("psModules.objects", 10, "---- %s() end ----\n", __func__);
    return(tmp);
}

// copy to an already allocated peak
bool pmPeakCopy(pmPeak *out, pmPeak *in)
{
    out->x  		 = in->x;
    out->y  		 = in->y;
    out->xf 		 = in->xf;
    out->yf 		 = in->yf;
    out->dx 		 = in->dx;
    out->dy 		 = in->dy;
    out->detValue      	 = in->detValue;
    out->rawFlux       	 = in->rawFlux;
    out->rawFluxStdev  	 = in->rawFluxStdev;
    out->smoothFlux    	 = in->smoothFlux;
    out->smoothFluxStdev = in->smoothFluxStdev;
    out->assigned        = in->assigned;
    out->type      	 = in->type;
    out->footprint       = in->footprint;

    return true;
}

bool psMemCheckPeak(psPtr ptr)
{
    PS_ASSERT_PTR(ptr, false);
    return ( psMemGetDeallocator(ptr) == (psFreeFunc) peakFree);
}


// psSort comparison functions for peaks
// XXX: Add error-checking for NULL args
int pmPeaksSortByDetValueAscend (const void **a, const void **b)
{
    pmPeak *A = *(pmPeak **)a;
    pmPeak *B = *(pmPeak **)b;

    psF32 diff;

    diff = A->detValue - B->detValue;
    if (diff < FLT_EPSILON) {
        return (-1);
    } else if (diff > FLT_EPSILON) {
        return (+1);
    }
    return (0);
}
int pmPeaksSortByDetValueDescend (const void **a, const void **b)
{
    pmPeak *A = *(pmPeak **)a;
    pmPeak *B = *(pmPeak **)b;

    psF32 diff;

    diff = A->detValue - B->detValue;
    if (diff < FLT_EPSILON) {
        return (+1);
    } else if (diff > FLT_EPSILON) {
        return (-1);
    }
    return (0);
}
int pmPeaksSortByRawFluxAscend (const void **a, const void **b)
{
    pmPeak *A = *(pmPeak **)a;
    pmPeak *B = *(pmPeak **)b;

    psF32 diff;

    diff = A->rawFlux - B->rawFlux;
    if (diff < FLT_EPSILON) {
        return (-1);
    } else if (diff > FLT_EPSILON) {
        return (+1);
    }
    return (0);
}
int pmPeaksSortByRawFluxDescend (const void **a, const void **b)
{
    pmPeak *A = *(pmPeak **)a;
    pmPeak *B = *(pmPeak **)b;

    psF32 diff;

    diff = A->rawFlux - B->rawFlux;
    if (diff < FLT_EPSILON) {
        return (+1);
    } else if (diff > FLT_EPSILON) {
        return (-1);
    }
    return (0);
}
int pmPeaksSortBySmoothFluxAscend (const void **a, const void **b)
{
    pmPeak *A = *(pmPeak **)a;
    pmPeak *B = *(pmPeak **)b;

    psF32 diff;

    diff = A->smoothFlux - B->smoothFlux;
    if (diff < FLT_EPSILON) {
        return (-1);
    } else if (diff > FLT_EPSILON) {
        return (+1);
    }
    return (0);
}
int pmPeaksSortBySmoothFluxDescend (const void **a, const void **b)
{
    pmPeak *A = *(pmPeak **)a;
    pmPeak *B = *(pmPeak **)b;

    psF32 diff;

    diff = A->smoothFlux - B->smoothFlux;
    if (diff < FLT_EPSILON) {
        return (+1);
    } else if (diff > FLT_EPSILON) {
        return (-1);
    }
    return (0);
}

// // sort by SN (descending)
// int pmPeakSortBySN (const void **a, const void **b)
// {
//     pmPeak *A = *(pmPeak **)a;
//     pmPeak *B = *(pmPeak **)b;
// 
//     psF32 fA = A->flux;
//     psF32 fB = B->flux;
//     if (isnan (fA)) fA = 0;
//     if (isnan (fB)) fB = 0;
// 
//     psF32 diff = fA - fB;
//     if (diff > FLT_EPSILON) return (-1);
//     if (diff < FLT_EPSILON) return (+1);
//     return (0);
// }
// 
// // sort by Y (ascending)
// int pmPeakSortByY (const void **a, const void **b)
// {
//     pmPeak *A = *(pmPeak **)a;
//     pmPeak *B = *(pmPeak **)b;
// 
//     psF32 fA = A->y;
//     psF32 fB = B->y;
// 
//     psF32 diff = fA - fB;
//     if (diff > FLT_EPSILON) return (+1);
//     if (diff < FLT_EPSILON) return (-1);
//     return (0);
// }

/******************************************************************************
pmPeaksInVector(vector, threshold): Find all local peaks in the given vector
above the given threshold.  Returns a vector of type PS_TYPE_U32 containing
the location (x value) of all peaks.
 
XXX: What types should be supported?  Only F32 is implemented.
 
XXX: We currently step through the input vector twice; once to determine the
size of the output vector, then to set the values of the output vector.
Depending upon actual use, this may need to be optimized.
*****************************************************************************/
psVector *pmPeaksInVector(const psVector *vector,
			 psF32 threshold)
{
    psTrace("psModules.objects", 10, "---- %s() begin ----\n", __func__);
    PS_ASSERT_VECTOR_NON_NULL(vector, NULL);
    PS_ASSERT_VECTOR_NON_EMPTY(vector, NULL);
    PS_ASSERT_VECTOR_TYPE(vector, PS_TYPE_F32, NULL);
    int count = 0;
    int n = vector->n;

    //
    // Special case: the input vector has a single element.
    //
    if (n == 1) {
        psVector *tmpVector = NULL;
        if (vector->data.F32[0] > threshold) {
            tmpVector = psVectorAlloc(1, PS_TYPE_U32);
            tmpVector->data.U32[0] = 0;
        } else {
            tmpVector = psVectorAlloc(0, PS_TYPE_U32);
        }
        psTrace("psModules.objects", 10, "---- %s() end ----\n", __func__);
        return(tmpVector);
    }

    //
    // Determine if first pixel is a peak
    //
    if ((vector->data.F32[0] > vector->data.F32[1]) &&
            (vector->data.F32[0] > threshold)) {
        count++;
    }

    //
    // Determine if interior pixels are peaks
    //
    for (psU32 i = 1; i < n-1 ; i++) {
        if ((vector->data.F32[i] > vector->data.F32[i-1]) &&
                (vector->data.F32[i] >= vector->data.F32[i+1]) &&
                (vector->data.F32[i] > threshold)) {
            count++;
        }
    }

    //
    // Determine if last pixel is a peak
    //
    if ((vector->data.F32[n-1] > vector->data.F32[n-2]) &&
            (vector->data.F32[n-1] > threshold)) {
        count++;
    }

    //
    // We know how many peaks exist, so we now allocate a psVector to store
    // those peaks.
    //
    psVector *tmpVector = psVectorAlloc(count, PS_TYPE_U32);
    count = 0;

    //
    // Determine if first pixel is a peak
    //
    if ((vector->data.F32[0] > vector->data.F32[1]) &&
            (vector->data.F32[0] > threshold)) {
        tmpVector->data.U32[count++] = 0;
    }

    //
    // Determine if interior pixels are peaks
    //
    for (psU32 i = 1; i < (n-1) ; i++) {
        if ((vector->data.F32[i] > vector->data.F32[i-1]) &&
                (vector->data.F32[i] >= vector->data.F32[i+1]) &&
                (vector->data.F32[i] > threshold)) {
            tmpVector->data.U32[count++] = i;
        }
    }

    //
    // Determine if last pixel is a peak
    //
    if ((vector->data.F32[n-1] > vector->data.F32[n-2]) &&
            (vector->data.F32[n-1] > threshold)) {
        tmpVector->data.U32[count++] = n-1;
    }

    psTrace("psModules.objects", 10, "---- %s() end ----\n", __func__);
    return(tmpVector);
}


/******************************************************************************
pmPeaksInImage(image, threshold): Find all local peaks in the given psImage
above the given threshold.  Returns a psArray containing location (x/y value)
of all peaks.
 
XXX: I'm not convinced the peak type definition in the SDRS is mutually
exclusive.  Some peaks can have multiple types.  Edges for sure.  Also, a
digonal line with the same value at each point will have a peak for every
point on that line.
 
XXX: This does not work if image has either a single row, or a single column.
 
The peak is returned in the image parent coordinates

*****************************************************************************/
psArray *pmPeaksInImage(const psImage *image, psF32 threshold)
{
    psTrace("psModules.objects", 10, "---- %s() begin ----\n", __func__);
    PS_ASSERT_IMAGE_NON_NULL(image, NULL);
    PS_ASSERT_IMAGE_TYPE(image, PS_TYPE_F32, NULL);
    if ((image->numRows == 1) || (image->numCols == 1)) {
        psError(PS_ERR_UNKNOWN, true, "Currently, input image must have at least 2 rows and 2 columns.");
        psTrace("psModules.objects", 10, "---- %s(NULL) end ----\n", __func__);
        return(NULL);
    }
    psVector *tmpRow = NULL;
    psU32 col = 0;
    psU32 row = 0;
    psArray *list = psArrayAllocEmpty(100);

    // Find peaks in row 0 only.
    row = 0;
    tmpRow = getRowVectorFromImage((psImage *) image, row);
    psVector *row1 = pmPeaksInVector(tmpRow, threshold);
    // pmPeaksInVector returns coords in the vector, not corrected for col0

    for (psU32 i = 0 ; i < row1->n ; i++ ) {
        col = row1->data.U32[i];
        // is pixel (0,0) is a peak?
        if (col == 0) {
            if ( (image->data.F32[row][col] >  image->data.F32[row][col+1]) &&
                    (image->data.F32[row][col] >  image->data.F32[row+1][col]) &&
                    (image->data.F32[row][col] >= image->data.F32[row+1][col+1])) {

                if (image->data.F32[row][col] > threshold) {
                    list = AddPeak(list, image, row, col, PM_PEAK_EDGE);
                }
            }
        } else if (col < (image->numCols - 1)) {
            if ( (image->data.F32[row][col] >= image->data.F32[row][col-1]) &&
                    (image->data.F32[row][col] >  image->data.F32[row][col+1]) &&
                    (image->data.F32[row][col] >= image->data.F32[row+1][col-1]) &&
                    (image->data.F32[row][col] >  image->data.F32[row+1][col]) &&
                    (image->data.F32[row][col] >= image->data.F32[row+1][col+1])) {
                if (image->data.F32[row][col] > threshold) {
                    list = AddPeak(list, image, row, col, PM_PEAK_EDGE);
                }
            }

        } else if (col == (image->numCols - 1)) {
            if ( (image->data.F32[row][col] >= image->data.F32[row][col-1]) &&
                    (image->data.F32[row][col] > image->data.F32[row+1][col]) &&
                    (image->data.F32[row][col] >= image->data.F32[row+1][col-1])) {
                if (image->data.F32[row][col] > threshold) {
                    list = AddPeak(list, image, row, col, PM_PEAK_EDGE);
                }
            }

        } else {
            psLogMsg ("psModules.objects", 5, "peak specified outside valid column range.");
        }
    }
    psFree (tmpRow);
    psFree (row1);

    //
    // Exit if this image has a single row.
    //
    if (image->numRows == 1) {
        psTrace("psModules.objects", 10, "---- %s() end ----\n", __func__);
        return(list);
    }

    //
    // Find peaks in interior rows only.
    //
    for (row = 1 ; row < (image->numRows - 1) ; row++) {
        tmpRow = getRowVectorFromImage((psImage *) image, row);
        row1 = pmPeaksInVector(tmpRow, threshold);

        // Step through all local peaks in this row.
        for (psU32 i = 0 ; i < row1->n ; i++ ) {
            pmPeakType myType = PM_PEAK_UNDEF;
            col = row1->data.U32[i];

            if (col == 0) {
                // If col==0, then we can not read col-1 pixels
                if ((image->data.F32[row][col] >  image->data.F32[row-1][col]) &&
                        (image->data.F32[row][col] >= image->data.F32[row-1][col+1]) &&
                        (image->data.F32[row][col] >= image->data.F32[row][col+1]) &&
                        (image->data.F32[row][col] >= image->data.F32[row+1][col]) &&
                        (image->data.F32[row][col] >= image->data.F32[row+1][col+1])) {
                    myType = PM_PEAK_EDGE;
                    list = AddPeak(list, image, row, col, myType);
                }
            } else if (col < (image->numCols - 1)) {
                // This is an interior pixel
                if ((image->data.F32[row][col] >= image->data.F32[row-1][col-1]) &&
                        (image->data.F32[row][col] >  image->data.F32[row-1][col]) &&
                        (image->data.F32[row][col] >= image->data.F32[row-1][col+1]) &&
                        (image->data.F32[row][col] > image->data.F32[row][col-1]) &&
                        (image->data.F32[row][col] >= image->data.F32[row][col+1]) &&
                        (image->data.F32[row][col] >= image->data.F32[row+1][col-1]) &&
                        (image->data.F32[row][col] >= image->data.F32[row+1][col]) &&
                        (image->data.F32[row][col] >= image->data.F32[row+1][col+1])) {
                    if (image->data.F32[row][col] > threshold) {
                        if ((image->data.F32[row][col] > image->data.F32[row-1][col-1]) &&
                                (image->data.F32[row][col] > image->data.F32[row-1][col]) &&
                                (image->data.F32[row][col] > image->data.F32[row-1][col+1]) &&
                                (image->data.F32[row][col] > image->data.F32[row][col-1]) &&
                                (image->data.F32[row][col] > image->data.F32[row][col+1]) &&
                                (image->data.F32[row][col] > image->data.F32[row+1][col-1]) &&
                                (image->data.F32[row][col] > image->data.F32[row+1][col]) &&
                                (image->data.F32[row][col] > image->data.F32[row+1][col+1])) {
                            myType = PM_PEAK_LONE;
                        }

                        if ((image->data.F32[row][col] == image->data.F32[row-1][col-1]) ||
                                (image->data.F32[row][col] == image->data.F32[row-1][col]) ||
                                (image->data.F32[row][col] == image->data.F32[row-1][col+1]) ||
                                (image->data.F32[row][col] == image->data.F32[row][col-1]) ||
                                (image->data.F32[row][col] == image->data.F32[row][col+1]) ||
                                (image->data.F32[row][col] == image->data.F32[row+1][col-1]) ||
                                (image->data.F32[row][col] == image->data.F32[row+1][col]) ||
                                (image->data.F32[row][col] == image->data.F32[row+1][col+1])) {
                            myType = PM_PEAK_FLAT;
                        }

                        list = AddPeak(list, image, row, col, myType);

                    }
                }
            } else if (col == (image->numCols - 1)) {
                // If col==numCols - 1, then we can not read col+1 pixels
                if ((image->data.F32[row][col] >= image->data.F32[row-1][col-1]) &&
                        (image->data.F32[row][col] >  image->data.F32[row-1][col]) &&
                        (image->data.F32[row][col] > image->data.F32[row][col-1]) &&
                        (image->data.F32[row][col] >= image->data.F32[row][col+1]) &&
                        (image->data.F32[row][col] >= image->data.F32[row+1][col-1]) &&
                        (image->data.F32[row][col] >= image->data.F32[row+1][col])) {
                    myType = PM_PEAK_EDGE;
                    list = AddPeak(list, image, row, col, myType);
                }
            } else {
		psLogMsg ("psModules.objects", 5, "peak specified outside valid column range.");
            }

        }
        psFree (tmpRow);
        psFree (row1);
    }

    //
    // Find peaks in the last row only.
    //
    row = image->numRows - 1;
    tmpRow = getRowVectorFromImage((psImage *) image, row);
    row1 = pmPeaksInVector(tmpRow, threshold);
    for (psU32 i = 0 ; i < row1->n ; i++ ) {
        col = row1->data.U32[i];
        if (col == 0) {
            if ( (image->data.F32[row][col] >  image->data.F32[row-1][col]) &&
                    (image->data.F32[row][col] >= image->data.F32[row-1][col+1]) &&
                    (image->data.F32[row][col] >  image->data.F32[row][col+1])) {
                if (image->data.F32[row][col] > threshold) {
                    list = AddPeak(list, image, row, col, PM_PEAK_EDGE);
                }
            }
        } else if (col < (image->numCols - 1)) {
            if ( (image->data.F32[row][col] >= image->data.F32[row-1][col-1]) &&
                    (image->data.F32[row][col] >  image->data.F32[row-1][col]) &&
                    (image->data.F32[row][col] >= image->data.F32[row-1][col+1]) &&
                    (image->data.F32[row][col] >  image->data.F32[row][col-1]) &&
                    (image->data.F32[row][col] >= image->data.F32[row][col+1])) {
                if (image->data.F32[row][col] > threshold) {
                    list = AddPeak(list, image, row, col, PM_PEAK_EDGE);
                }
            }

        } else if (col == (image->numCols - 1)) {
            if ( (image->data.F32[row][col] >= image->data.F32[row-1][col-1]) &&
                    (image->data.F32[row][col] >  image->data.F32[row-1][col]) &&
                    (image->data.F32[row][col] >  image->data.F32[row][col-1])) {
                if (image->data.F32[row][col] > threshold) {
                    list = AddPeak(list, image, row, col, PM_PEAK_EDGE);
                }
            }
        } else {
            psLogMsg ("psModules.objects", 5, "peak specified outside valid column range.");
        }
    }
    psFree (tmpRow);
    psFree (row1);
    psTrace("psModules.objects", 10, "---- %s() end ----\n", __func__);
    return(list);
}
