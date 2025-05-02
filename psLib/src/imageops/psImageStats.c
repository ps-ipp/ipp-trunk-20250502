/** @file psImageStats.c
 *  \brief Routines for calculating statistics on images.
 *  @ingroup ImageStats
 *
 *  This file will hold the prototypes for procedures which calculate
 *  statistic on images, histograms on images, and fit/evaluate Chebyshev
 *  polynomials to images.
 *
 *  @author GLG, MHPCC
 *
 *  @version $Revision: 1.107 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-01-27 06:39:37 $
 *
 *  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <float.h>
#include <math.h>
#include "psMemory.h"
#include "psVector.h"
#include "psTrace.h"
#include "psError.h"
#include "psStats.h"
#include "psImage.h"
#include "psPolynomial.h"
#include "psImageStats.h"
#include "psAssert.h"

#include "psRegion.h"
#include "psRegionForImage.h"

/// This routine must determine the various statistics for the image.
/*****************************************************************************
psImageStats(stats, in, mask, maskVal): this routine simply calls the
psVectorStats() routine, which does the actual statistical calculation.  In
order to do so, we create dummy psVectors and set their "data" pointer to that
of the input psImages.

XXX: use static psVectors

XXX: optimize this.  2k vs 4k, sample mean, takes8 seconds on Gene's machine.
Should take .2.
 *****************************************************************************/
bool psImageStats(psStats* stats,
                      const psImage* in,
                      const psImage* mask,
                      psImageMaskType maskVal)
{
    psVector *junkData = NULL;
    psVector *junkMask = NULL;

    PS_ASSERT_PTR_NON_NULL(stats, false);
    PS_ASSERT_INT_NONZERO(stats->options, false);
    PS_ASSERT_IMAGE_NON_NULL(in, false)
    if (mask != NULL) {
        PS_ASSERT_IMAGE_TYPE(mask, PS_TYPE_IMAGE_MASK, false);
        PS_ASSERT_IMAGES_SIZE_EQUAL(in, mask, false);
    }

    if (in->parent == NULL) {
        // stuff the image data into a psVector struct.
	// XXX this is a bit hackish: does it save much time? (avoids a Nx*Ny alloc)
        junkData = (psVector *) psAlloc(sizeof(psVector));
        junkData->type = in->type;
        P_PSVECTOR_SET_NALLOC(junkData,in->numRows * in->numCols);
        junkData->n = junkData->nalloc;
        junkData->data.U8 = in->data.V[0];      // since psImage data is contiguous...
    } else {
        // image not necessarily contiguous
        int numRows = in->numRows;
        int numCols = in->numCols;
        int rowSize = numCols * (PSELEMTYPE_SIZEOF(in->type.type));

        junkData = psVectorAlloc(numRows*numCols, in->type.type);

        psU8* data = junkData->data.U8;
        for (int row = 0; row < numRows; row++) {
            memcpy(data, in->data.V[row], rowSize);
            data += rowSize;
        }
    }

    if (mask != NULL) {
	// image not necessarily contiguous, generate a temp vector to hold the full image
	int numRows = mask->numRows;
	int numCols = mask->numCols;

	junkMask = psVectorAlloc(numRows*numCols, PS_TYPE_VECTOR_MASK);

	psVectorMaskType *data = junkMask->data.PS_TYPE_VECTOR_MASK_DATA;
	for (int row = 0, nVect = 0; row < numRows; row++) {
	    for (int col = 0; col < numCols; col++, nVect++) {
		data[nVect] = (mask->data.PS_TYPE_IMAGE_MASK_DATA[row][col] & maskVal);
	    }
	}
    }

    if (!psVectorStats(stats, junkData, NULL, junkMask, 0xff)) {
	psFree(junkMask);
	psFree(junkData);
	return false;
    }

    psFree(junkMask);
    psFree(junkData);
    return true;
}

/*****************************************************************************
NOTE: We assume that the psHistogram structure out has already been allocated
and initialized.
 *****************************************************************************/
bool psImageHistogram(psHistogram* out,
                              const psImage* in,
                              const psImage* mask,
                              psImageMaskType maskVal)
{
    PS_ASSERT_PTR_NON_NULL(out, false);
    PS_ASSERT_PTR_NON_NULL(in, false);
    if (mask != NULL) {
        PS_ASSERT_IMAGE_TYPE(mask, PS_TYPE_IMAGE_MASK, false);
        PS_ASSERT_IMAGES_SIZE_EQUAL(in, mask, false);
    }
    psVector* junkData = NULL;
    psVector* junkMask = NULL;

    if (in->parent == NULL) {
        // stuff the image data into a psVector struct.
        junkData = (psVector *) psAlloc(sizeof(psVector));
        junkData->type = in->type;
        P_PSVECTOR_SET_NALLOC(junkData,in->numRows * in->numCols);
        junkData->n = junkData->nalloc;
        junkData->data.U8 = in->data.V[0];      // since psImage data is contiguous...
    } else {
        // image not necessarily contiguous
        int numRows = in->numRows;
        int numCols = in->numCols;
        int rowSize = numCols * (PSELEMTYPE_SIZEOF(in->type.type));

        junkData = psVectorAlloc(numRows*numCols, in->type.type);

        psU8* data = junkData->data.U8;
        for (int row = 0; row < numRows; row++) {
            memcpy(data, in->data.V[row], rowSize);
            data += rowSize;
        }
    }

    if (mask != NULL) {
	// image not necessarily contiguous; vector & image mask types do not match
	int numRows = mask->numRows;
	int numCols = mask->numCols;

	junkMask = psVectorAlloc(numRows*numCols, PS_TYPE_VECTOR_MASK);

	psVectorMaskType *data = junkMask->data.PS_TYPE_VECTOR_MASK_DATA;
	for (int row = 0, nVect = 0; row < numRows; row++) {
	    for (int col = 0; col < numCols; col++, nVect++) {
		data[nVect] = (mask->data.PS_TYPE_IMAGE_MASK_DATA[row][col] & maskVal);
	    }
	}
    }

    if (!psVectorHistogram(out, junkData, NULL, junkMask, maskVal)) {
        psError(PS_ERR_UNKNOWN, false, "Unable to generate image histogram.\n");
        psFree(junkMask);
        psFree(junkData);
        return false;
    }

    psFree(junkMask);
    psFree(junkData);

    return true;
}

/*****************************************************************************
calcScaleFactorsEval(n): The Chebyshev polynomials are defined over the
interval [-1.0 : 1.0].  Images typically have sizes of 512x512 or more.  In
order to use Chebyshev polynomials, we must scale the coordinates from
0:512 to -1:1.  This routine takes as input an integer N and produces as
output a vector of evenly spaced floating point values between -1.0:1.0.

XXX: Use the p_psNormalizeVector here?
 *****************************************************************************/
static double* calcScaleFactors(psS32 n)
{
    PS_ASSERT_INT_NONNEGATIVE(n, NULL);
    psS32 i = 0;
    double tmp = 0.0;
    double *scalingFactors = (double *)psAlloc(n * sizeof(double));

    for (i = 0; i < n; i++) {
        tmp = (double)(n - i);
        tmp = (M_PI * (tmp - 0.5)) / ((double)n);
        scalingFactors[i] = cos(tmp);
    }

    return (scalingFactors);
}

/*****************************************************************************
psImageFitPolynomial(): This routine takes as input a 2-D image and produces
as output the coefficients of the Chebyshev polynomials which match that
input image.
  Input:
  Output:
  Internal Data Structures:
    chebPolys[i][j]
    sums[i][j]: This will contain the sum of
                input->data.F32[y][x] *
                psPolynomial1DEval(
chebPolys[i],
(float) x) *
                psPolynomial1DEval(
chebPolys[j],
(float) y,
);
        over all pixels (x,y) in the image.
  *****************************************************************************/
bool psImageFitPolynomial(psPolynomial2D* coeffs,
                                     const psImage* input)
{
    PS_ASSERT_IMAGE_NON_NULL(input, false);
    PS_ASSERT_IMAGE_NON_EMPTY(input, false);
    if ((input->type.type != PS_TYPE_S8) &&
            (input->type.type != PS_TYPE_U16) &&
            (input->type.type != PS_TYPE_F32) &&
            (input->type.type != PS_TYPE_F64)) {
        psError(PS_ERR_BAD_PARAMETER_TYPE, true, "Unallowable image type.\n");
        return false;
    }
    PS_ASSERT_POLY_NON_NULL(coeffs, false);
    PS_ASSERT_POLY_TYPE(coeffs, PS_POLYNOMIAL_CHEB, false);
    psS32 x = 0;
    psS32 y = 0;
    psS32 i = 0;
    psS32 j = 0;
    double **sums = NULL;
    psPolynomial1D* *chebPolys = NULL;
    psS32 maxChebyPoly = 0;
    double *cScalingFactors = NULL;
    double *rScalingFactors = NULL;

    // Create the sums[][] data structure.  This
    // will hold the LHS of
    // equation
    // 29 in the ADD: sums[k][l] = SUM {
    // image(x,y) * Tk(x) * Tl(y) }
    sums = (double **)psAlloc((1 + coeffs->nX) * sizeof(double *));
    for (i = 0; i < (1 + coeffs->nX); i++) {
        sums[i] = (double *)psAlloc((1 + coeffs->nY) * sizeof(double));
    }
    // We scale the pixel positions to values
    // between -1.0 and 1.0
    //    rScalingFactors = calcScaleFactors(input->numRows);
    //    cScalingFactors = calcScaleFactors(input->numCols);
    rScalingFactors = calcScaleFactors(input->row0 + input->numRows);
    cScalingFactors = calcScaleFactors(input->col0 + input->numCols);

    // Determine how many Chebyshev polynomials
    // are needed, then create them.
    // XXX: record or verify the poly order/nterm change
    maxChebyPoly = coeffs->nX;
    if (coeffs->nY > coeffs->nX) {
        maxChebyPoly = coeffs->nY;
    }
    chebPolys = p_psCreateChebyshevPolys(maxChebyPoly + 1);

    // Compute the sums[][] data structure.
    for (i = 0; i < (1 + coeffs->nX); i++) {
        for (j = 0; j < (1 + coeffs->nY); j++) {
            sums[i][j] = 0.0;
            //            for (x = 0; x < input->numRows; x++) {
            //                for (y = 0; y < input->numCols; y++) {
            for (y = input->row0; y < (input->row0 + input->numRows); y++) {
                for (x = input->col0; x < (input->col0 + input->numCols); x++) {
                    double pixel = 0.0;
                    if (input->type.type == PS_TYPE_S8) {
                        pixel = (double) input->data.S8[y][x];
                    } else if (input->type.type == PS_TYPE_U16) {
                        pixel = (double) input->data.U16[y][x];
                    } else if (input->type.type == PS_TYPE_F32) {
                        pixel = (double) input->data.F32[y][x];
                    } else if (input->type.type == PS_TYPE_F64) {
                        pixel = input->data.F64[y][x];
                    }
                    sums[i][j] += pixel * psPolynomial1DEval(chebPolys[i],rScalingFactors[y]) *
                                  psPolynomial1DEval(chebPolys[j], cScalingFactors[x]);
                }
            }
        }
    }

    for (i = 0; i < (1 + coeffs->nX); i++) {
        for (j = 0; j < (1 + coeffs->nY); j++) {
            coeffs->coeff[i][j] = sums[i][j];
            coeffs->coeff[i][j] /= (double)(input->numRows * input->numCols);

            if ((i != 0) && (j != 0)) {
                coeffs->coeff[i][j] *= 4.0;
            } else if ((i == 0) && (j == 0)) {
                coeffs->coeff[i][j] *= 1.0;
            } else {
                coeffs->coeff[i][j] *= 2.0;
            }
        }
    }

    // Free the Chebyshev polynomials that were
    // created in this routine.
    for (i = 0; i < maxChebyPoly + 1; i++) {
        psFree(chebPolys[i]);
    }
    psFree(chebPolys);

    // Free some data
    for (i = 0; i < (1 + coeffs->nX); i++) {
        psFree(sums[i]);
    }
    psFree(sums);
    psFree(cScalingFactors);
    psFree(rScalingFactors);

    return true;
}

/*****************************************************************************
XXX: Use static variables for Chebyshev polynomials and scaling factors.
 *****************************************************************************/
psImage* p_psImageEvalPolynomialCheb(psImage* input,
                                     const psPolynomial2D* coeffs)
{
    PS_ASSERT_POLY_TYPE(coeffs, PS_POLYNOMIAL_CHEB, NULL);

    psPolynomial1D* *chebPolys = NULL;
    long maxChebyPoly = 0;
    double *cScalingFactors = NULL;
    double *rScalingFactors = NULL;

    // We scale the pixel positions to values between -1.0 and 1.0
    // Use static data structures here.
    //    rScalingFactors = calcScaleFactors(input->numRows);
    //    cScalingFactors = calcScaleFactors(input->numCols);
    rScalingFactors = calcScaleFactors(input->numRows+input->row0);
    cScalingFactors = calcScaleFactors(input->numCols+input->col0);

    // Determine how many Chebyshev polynomials
    // are needed, then create them.
    maxChebyPoly = coeffs->nX;
    if (coeffs->nY > coeffs->nX) {
        maxChebyPoly = coeffs->nY;
    }

    chebPolys = p_psCreateChebyshevPolys(maxChebyPoly + 1);

    for (long y = input->row0; y < (input->row0 + input->numRows); y++) {
        for (long x = input->col0; x < (input->col0 + input->numCols); x++) {
            double polySum = 0.0;
            for (long i = 0; i < (1 + coeffs->nX); i++) {
                for (long j = 0; j < (1 + coeffs->nY); j++) {
                    polySum +=
                        psPolynomial1DEval(chebPolys[i], rScalingFactors[y]) *
                        psPolynomial1DEval(chebPolys[j], cScalingFactors[x]) *
                        coeffs->coeff[i][j];
                }
            }

            if (input->type.type == PS_TYPE_S8) {
                input->data.S8[y][x] = (char) polySum;
            } else if (input->type.type == PS_TYPE_U16) {
                input->data.U16[y][x] = (short int) polySum;
            } else if (input->type.type == PS_TYPE_F32) {
                input->data.F32[y][x] = (float) polySum;
            } else if (input->type.type == PS_TYPE_F64) {
                input->data.F64[y][x] = polySum;
            }
        }
    }

    // Free the Chebyshev polynomials that were
    // created in this routine.
    // XXX: Use static data structures here.
    for (long i = 0; i < maxChebyPoly + 1; i++) {
        psFree(chebPolys[i]);
    }
    psFree(chebPolys);

    psFree(cScalingFactors);
    psFree(rScalingFactors);

    return input;
}

psImage* p_psImageEvalPolynomialOrd(psImage* input,
                                    const psPolynomial2D* coeffs)
{
    PS_ASSERT_POLY_TYPE(coeffs, PS_POLYNOMIAL_ORD, NULL);

    for (int row = 0; row < input->numRows ; row++) {
        for (int col = 0; col < input->numCols ; col++) {
            if (input->type.type == PS_TYPE_S8) {
                input->data.S8[row][col] = (psS8) psPolynomial2DEval(coeffs,
                                           (psF32) (row + input->row0), (psF32) (col + input->col0));
            } else if (input->type.type == PS_TYPE_U16) {
                input->data.U16[row][col] = (psS16) psPolynomial2DEval(coeffs,
                                            (psF32) (row + input->row0), (psF32) (col + input->col0));
            } else if (input->type.type == PS_TYPE_F32) {
                input->data.F32[row][col] = psPolynomial2DEval(coeffs,
                                            (psF32) (row + input->row0), (psF32) (col + input->col0));
            } else if (input->type.type == PS_TYPE_F64) {
                input->data.F64[row][col] = (psF64) psPolynomial2DEval(coeffs,
                                            (psF32) (row + input->row0), (psF32) (col + input->col0));
            }
        }
    }

    return(input);
}


/*****************************************************************************
XXX: I added normal polynomials to this routine.  Let IfA know, put it in the
psLib SDR.
 *****************************************************************************/
psImage* psImageEvalPolynomial(psImage* input,
                               const psPolynomial2D* coeffs)
{
    PS_ASSERT_IMAGE_NON_NULL(input, NULL);
    PS_ASSERT_IMAGE_NON_EMPTY(input, NULL);
    if ((input->type.type != PS_TYPE_S8) &&
            (input->type.type != PS_TYPE_U16) &&
            (input->type.type != PS_TYPE_F32) &&
            (input->type.type != PS_TYPE_F64)) {
        psError(PS_ERR_BAD_PARAMETER_TYPE, true,
                "Unallowable image type.\n");
    }
    PS_ASSERT_POLY_NON_NULL(coeffs, NULL);

    if (coeffs->type == PS_POLYNOMIAL_ORD) {
        return(p_psImageEvalPolynomialOrd(input, coeffs));
    } else if (coeffs->type == PS_POLYNOMIAL_CHEB) {
        return(p_psImageEvalPolynomialCheb(input, coeffs));
    } else {
        psError(PS_ERR_UNKNOWN, false, "Incorrect Polynomial Type.\n");
    }
    return(NULL);
}

// count number of pixels with given mask value
long psImageCountPixelMask (psImage *mask,
                            psRegion region,
                            psImageMaskType value)
{
    long Npixels = 0;
    int x0 = 0;
    int y0 = 0;
    int x1 = 0;
    int y1 = 0;

    // this is not a valid error: a psRegion with ranges outside the valid pixels
    // should saturate on the valid pixels, not result in an error (per SDRS)
    //    if (region.x1 > mask->numCols || region.y1 > mask->numRows) {
    //        psError(PS_ERR_BAD_PARAMETER_SIZE, true,
    //                "psRegion input is outside of image boundary\n");
    //        return -1;
    //    }
    //    if (region.x0 <= 0 || region.x1 <= 0 || region.y0 <= 0 || region.y1 <= 0) {
    //        region = psRegionForImage(mask, region);
    //    }
    // not a valid error: if region coordinates are out of order, they should be flipped
    //    if (region.x0 > region.x1 || region.y0 > region.y1) {
    //        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
    //               "Invalid region.  Lower boundary greater than upper boundary.\n");
    //        return -1;
    //    }

    // rationalize the region

    /*
        if (mask == NULL) {
            psError(PS_ERR_BAD_PARAMETER_NULL, true,
                    _("Can not operate on a NULL psImage."));
            return -1;
        }
        region = psRegionForImage(mask, region);

        if (region.x0 == region.x1 || region.y0 == region.y1) {
            psError(PS_ERR_BAD_PARAMETER_SIZE, true,
                    "psRegion input contains 0 pixels\n");
            return -1;
        }
    */
    PS_ASSERT_IMAGE_NON_NULL(mask, -1);
    PS_ASSERT_INT_NONNEGATIVE(mask->col0, -1);
    PS_ASSERT_INT_NONNEGATIVE(mask->row0, -1);
    PS_ASSERT_INT_POSITIVE(mask->numCols, -1);
    PS_ASSERT_INT_POSITIVE(mask->numRows, -1);

    int col0 = (int)(roundf(region.x0));
    int col1 = (int)(roundf(region.x1));
    int row0 = (int)(roundf(region.y0));
    int row1 = (int)(roundf(region.y1));
    //If (0,0,0,0) specified, the whole image is to be used.
    if (col0 == 0 && col1 == 0 && row0 == 0 && row1 == 0) {
        col0 = mask->col0;
        col1 = mask->col0 + mask->numCols;
        row0 = mask->row0;
        row1 = mask->row0 + mask->numRows;
    }

    //Make sure x0 of region is inside image.  If so, set col0 to corresponding index number.
    if (col0 >= mask->col0 && col0 <= (mask->col0 + mask->numCols) ) {
        col0 -= mask->col0;
    } else {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                "Specified psRegion parameter, x0=%f, is out of range [%d,%d].\n",
                region.x0, mask->col0, mask->col0+mask->numCols);
        return -1;
    }
    //Make sure y0 of region is inside image.  If so, set row0 to corresponding index number.
    if (row0 >= mask->row0 && row0 <= (mask->row0 + mask->numRows) ) {
        row0 -= mask->row0;
    } else {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                "Specified psRegion parameter, y0=%f, is out of range [%d,%d].\n",
                region.y0, mask->row0, mask->row0+mask->numRows);
        return -1;
    }

    //Make sure x1 of region is valid.  If negative, index from tail (if valid).
    if (col1 < 0) {
        col1 += mask->numCols;
        if (col1 < 0) {
            psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                    "Specified psRegion parameter, x1=%f=%d, is out of range [%d,%d].\n",
                    region.x1, col1+mask->col0, mask->col0, mask->col0+mask->numCols);
            return -1;
        }
    } else if (col1 >= mask->col0 && col1 <= (mask->col0 + mask->numCols) ) {
        col1 -= mask->col0;
    } else {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                "Specified psRegion parameter, x1=%f=%d, is out of range [%d,%d].\n",
                region.x1, col1, mask->col0, mask->col0+mask->numCols);
        return -1;
    }
    //Make sure y1 of region is valid.  If negative, index from tail (if valid).
    if (row1 < 0) {
        row1 += mask->numRows;
        if (row1 < 0) {
            psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                    "Specified psRegion parameter, y1=%f=%d, is out of range [%d,%d].\n",
                    region.y1, row1+mask->row0, mask->row0, mask->row0+mask->numRows);
            return -1;
        }
    } else if (row1 >= mask->row0 && row1 <= (mask->row0 + mask->numRows) ) {
        row1 -= mask->row0;
    } else {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                "Specified psRegion parameter, y1=%f=%d, is out of range [%d,%d].\n",
                region.y1, row1, mask->row0, mask->row0+mask->numRows);
        return -1;
    }
    //Now make sure that the region makes sense.
    if (col0 > col1 || row0 > row1) {
        if (col0 > col1) {
            psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                    "Invalid psRegion specified.  x0=%f=%d is greater than x1=%f=%d.\n",
                    region.x0, col0, region.x1, col1);
        } else {
            psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                    "Invalid psRegion specified.  y0=%f=%d is greater than y1=%f=%d.\n",
                    region.y0, row0, region.y1, row1);
        }
        return -1;
    }/* else if (col0 == col1 && row0 == row1) {
                                                            psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                                                                    "Invalid psRegion specified.  Region contains only 1 pixel.\n");
                                                            return -1;
                                                        }
                                                    */
    x0 = col0;
    x1 = col1;
    y0 = row0;
    y1 = row1;

# define PS_IMAGE_COUNT_PIXEL_MASK(NAME,TYPE) \
    case PS_TYPE_##NAME: \
        for (long j = y0; j < y1; j++) { \
            for (long i = x0; i < x1; i++) { \
                if (mask->data.TYPE[j][i] & value) { \
                    Npixels ++; \
                } \
            } \
        } \
        break;

    psElemType type = mask->type.type;
    switch (type) {
	PS_IMAGE_COUNT_PIXEL_MASK(U8, U8);
	PS_IMAGE_COUNT_PIXEL_MASK(U16,U16);
	PS_IMAGE_COUNT_PIXEL_MASK(U32,U32);
	PS_IMAGE_COUNT_PIXEL_MASK(U64,U64);
	PS_IMAGE_COUNT_PIXEL_MASK(S8, S8);
	PS_IMAGE_COUNT_PIXEL_MASK(S16,S16);
	PS_IMAGE_COUNT_PIXEL_MASK(S32,S32);
	PS_IMAGE_COUNT_PIXEL_MASK(S64,S64);

    default:
        // XXX this should include the mask type (as a string)
        psError(PS_ERR_BAD_PARAMETER_TYPE, true,
                _("Input psImage is an unsupported datatype"));

        return -1;
    }
    return (Npixels);


}

