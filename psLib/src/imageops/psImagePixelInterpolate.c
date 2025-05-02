/** @file  psImagePixelInterpolate.c
 *
 *  @brief Functions for interpolating bad pixels in images
 *
 *  these functions test and set masked pixels in an image.  These functions are complementary
 *  to the psImageInterpolate functions, which perform sub-pixel interpolation.  Those
 *  functions require all pixels surrounding the sub-pixel interpolation to have values which
 *  are valid.  These functions enable interpolation of complete missing pixels, potentially
 *  across large spans.
 *
 *  @author Eugene Magnier, IfA
 *
 *  @version $Revision: 1.4 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-01-27 06:39:37 $
 *
 *  Copyright 2007 Institute for Astronomy, University of Hawaii
 *
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>

#include "psAbort.h"
#include "psMemory.h"
#include "psError.h"
#include "psAssert.h"
#include "psString.h"
#include "psPolynomial.h"
#include "psPolynomialUtils.h"
#include "psMinimizePolyFit.h"
#include "psImage.h"
#include "psImageInterpolate.h"
#include "psImagePixelInterpolate.h"

#include "psFits.h"
#include "psFitsImage.h"

# define PS_IMAGE_ITER_STATE(XS,XE,YS,YE,N_MIN,TYPE) \
	    nGood = 0; \
	    for (int jy = YS; jy <= YE; jy++) { \
		/* stick to pixels in image grid */ \
		if (jy + iy < 0) { continue; } \
		if (jy + iy >= mask->numRows) { continue; } \
		for (int jx = XS; jx <= XE; jx++) { \
		    /* stick to pixels in image grid */ \
		    if (jx + ix < 0) { continue; } \
		    if (jx + ix >= mask->numCols) { continue; } \
		    /* do not test self */ \
		    if (!jx && !jy) { continue; } \
		    if (mask->data.PS_TYPE_IMAGE_MASK_DATA[iy+jy][ix+jx] & maskVal) { continue; } \
		    nGood ++; \
		} \
	    } \
	    if (nGood >= N_MIN) {  \
		nPoor ++; \
		result->data.S32[iy][ix] = TYPE; \
		continue; \
	    }

// count and mark pixels based on their potential for being interpolated.  the input image is
// just the mask, the output image contains enum values which define the type of interpolation which
// can be performed
psImage *psImagePixelInterpolateState (int *nBad, int *nPoor, psImage *mask, psImageMaskType maskVal) {

    psImage *result = psImageAlloc (mask->numCols, mask->numRows, PS_TYPE_S32);
    psImageInit (result, 0);
    
    *nPoor = 0;
    *nBad = 0;

    for (int iy = 0; iy < mask->numRows; iy++) {
	for (int ix = 0; ix < mask->numCols; ix++) {

	    // state of the good pixels (unmasked)
	    if (!(mask->data.PS_TYPE_IMAGE_MASK_DATA[iy][ix] & maskVal)) { 
		// count good neighbor pixels (+ self)
		int nGood = 0;
		int minX = +1;
		int maxX = -1;
		int minY = +1;
		int maxY = -1;
		for (int jy = -1; jy <= +1; jy++) {
		    /* stick to pixels in image grid */
		    if (jy + iy < 0) { continue; }
		    if (jy + iy >= mask->numRows) { continue; }
		    for (int jx = -1; jx <= +1; jx++) {
			/* stick to pixels in image grid */
			if (jx + ix < 0) { continue; }
			if (jx + ix >= mask->numCols) { continue; }
			if (mask->data.PS_TYPE_IMAGE_MASK_DATA[iy+jy][ix+jx] & maskVal) { continue; }
			nGood ++;
			minX = PS_MIN (minX, jx);
			maxX = PS_MAX (maxX, jx);
			minY = PS_MIN (minY, jy);
			maxY = PS_MAX (maxY, jy);
		    }
		}
		int dX = maxX - minX;
		int dY = maxY - minY;
		// what type of local interpolation can we use?
		if ((nGood >= 6) && (dX == 2) && (dY == 2)) {
		    result->data.S32[iy][ix] = PS_IMAGE_INTERPOLATE_GOOD2;
		    continue; 
		}
		if (nGood >= 3) {
		    result->data.S32[iy][ix] = PS_IMAGE_INTERPOLATE_GOOD1;
		    continue; 
		}
		result->data.S32[iy][ix] = PS_IMAGE_INTERPOLATE_GOOD0;
		continue; 
	    }

	    // examine the neighbors.  If at least 6 of the 8 surrounding, or 3 of the 4 corner
	    // neighbors are valid, this is a pixel which can be interpolated.

	    int nGood;

	    // check for poor pixels
	    PS_IMAGE_ITER_STATE (-1,+1,-1,+1,6, PS_IMAGE_INTERPOLATE_CENTER);
	    PS_IMAGE_ITER_STATE (-1,+0,-1,+0,3, PS_IMAGE_INTERPOLATE_UR);
	    PS_IMAGE_ITER_STATE (-1,+0,+0,+1,3, PS_IMAGE_INTERPOLATE_LR);
	    PS_IMAGE_ITER_STATE (+0,+1,-1,+0,3, PS_IMAGE_INTERPOLATE_UL);
	    PS_IMAGE_ITER_STATE (+0,+1,+0,+1,3, PS_IMAGE_INTERPOLATE_LL);

	    nBad ++;
	    result->data.S32[iy][ix] = PS_IMAGE_INTERPOLATE_BAD;
	}	    
    }
    return result;
}

// interpolate the poor pixels using the available options
bool psImagePixelInterpolatePoor (psImage *image, psImage *state, psImage *mask, psImageMaskType maskVal) {

    assert (image->numCols == state->numCols);
    assert (image->numRows == state->numRows);
    assert (image->numCols == mask->numCols);
    assert (image->numRows == mask->numRows);

    // allocate the vectors for the 2nd order fit below
    psVector *f  = psVectorAlloc (9, PS_TYPE_F32);
    // XXX if we add the weight above, include df
    // psVector *df = psVectorAlloc (9, PS_TYPE_F32); 
    psVector *x  = psVectorAlloc (9, PS_TYPE_F32);
    psVector *y  = psVectorAlloc (9, PS_TYPE_F32);

    // allocate a 2D polynomial to fit a quadratic to the valid neighbor pixels.
    psPolynomial2D *poly = psPolynomial2DAlloc (PS_POLYNOMIAL_ORD, 2, 2);
    poly->coeffMask[2][2] = PS_POLY_MASK_SET;
    poly->coeffMask[2][1] = PS_POLY_MASK_SET;
    poly->coeffMask[1][2] = PS_POLY_MASK_SET;

    for (int iy = 0; iy < state->numRows; iy++) {
	for (int ix = 0; ix < state->numCols; ix++) {

	    switch (state->data.S32[iy][ix]) {
	      case PS_IMAGE_INTERPOLATE_GOOD0: 
	      case PS_IMAGE_INTERPOLATE_GOOD1: 
	      case PS_IMAGE_INTERPOLATE_GOOD2: 
		// skip the good pixels
		break; 

	      case PS_IMAGE_INTERPOLATE_BAD:
		// skip the bad pixels
		break; 

	      case PS_IMAGE_INTERPOLATE_CENTER: {
		  // XXX is there a fit-image-region function?
		  int n = 0;
		  for (int jy = -1; jy <= +1; jy++) {
		      // skip invalid pixels 
		      if (jy + iy < 0) { continue; }
		      if (jy + iy >= image->numRows) { continue; }
		      for (int jx = -1; jx <= +1; jx++) {
			  // skip invalid pixels 
			  if (jx + ix < 0) { continue; } 
			  if (jx + ix >= image->numCols) { continue; } 
			  // skip self 
			  if (!jx && !jy) { continue; } 
			  // skip masked pixels
			  if (mask->data.PS_TYPE_IMAGE_MASK_DATA[iy+jy][ix+jx] & maskVal) { continue; }
			  x->data.F32[n] = jx;
			  y->data.F32[n] = jy;
			  f->data.F32[n] = image->data.F32[iy+jy][ix+jx];
			  // df->data.F32[n] = weight->data.F32[iy+jy][ix+jx];
			  n++;
		      }
		  }
		  // set vector lengths here
		  x->n = n;
		  y->n = n;
		  f->n = n;
		  // df->n = n;
		  // psVectorFitPolynomial2D (poly, NULL, 0xff, f, df, x, y);
		  psVectorFitPolynomial2D (poly, NULL, 0xff, f, NULL, x, y);
		  // apply the fitted quadratic to get the poor pixel value
		  image->data.F32[iy][ix] = poly->coeff[0][0];
		  break; }

		// XXX should I use 1 1D polynomial fitting all unmasked pixels in the 3x3 grid?
		// XXX that would automatically extend to regions where only 2 pixels are valid...
	      case PS_IMAGE_INTERPOLATE_LL: {
		  // fit a plane to the 3 pixels at (0,1),(1,0),(1,1), extend to pixel at (0,0)
		  image->data.F32[iy][ix] = image->data.F32[iy+1][ix+1] - image->data.F32[iy+0][ix+1] - image->data.F32[iy+1][ix+0];
		  break; }

	      case PS_IMAGE_INTERPOLATE_LR: {
		  // fit a plane to the 3 pixels at (0,1),(-1,0),(-1,1), extend to pixel at (0,0)
		  image->data.F32[iy][ix] = image->data.F32[iy+1][ix-1] - image->data.F32[iy+0][ix-1] - image->data.F32[iy+1][ix+0];
		  break; }

	      case PS_IMAGE_INTERPOLATE_UL: {
		  // fit a plane to the 3 pixels at (0,-1),(1,0),(1,-1), extend to pixel at (0,0)
		  image->data.F32[iy][ix] = image->data.F32[iy-1][ix+1] - image->data.F32[iy+0][ix+1] - image->data.F32[iy-1][ix+0];
		  break; }

	      case PS_IMAGE_INTERPOLATE_UR: {
		  // fit a plane to the 3 pixels at (0,-1),(-1,0),(-1,-1), extend to pixel at (0,0)
		  image->data.F32[iy][ix] = image->data.F32[iy-1][ix-1] - image->data.F32[iy+0][ix-1] - image->data.F32[iy-1][ix+0];
		  break; }

	      default:
		psAbort("impossible case in __func__");
	    }
	}	    
    }

    psFree (x);
    psFree (y);
    psFree (f);

    psFree (poly);
    return true;
}
    
// interpolate the good pixels to their true centers
bool psImagePixelInterpolateCenter (psImage *value, psImage *xCoord, psImage *yCoord, psImage *state, psImage *mask, psImageMaskType maskVal) {

    assert (value->numCols == state->numCols);
    assert (value->numRows == state->numRows);
    assert (value->numCols == mask->numCols);
    assert (value->numRows == mask->numRows);

# if (0)
    psFits *fits = NULL;

    fits = psFitsOpen ("xcoords.fits", "w");
    psFitsWriteImage (fits, NULL, xCoord, 0, NULL);
    psFitsClose (fits);

    fits = psFitsOpen ("ycoords.fits", "w");
    psFitsWriteImage (fits, NULL, yCoord, 0, NULL);
    psFitsClose (fits);

    fits = psFitsOpen ("value.fits", "w");
    psFitsWriteImage (fits, NULL, value, 0, NULL);
    psFitsClose (fits);
# endif

    psImage *output = psImageAlloc (value->numCols, value->numRows, PS_TYPE_F32);

    // allocate the vectors for the 2nd order fit below
    psVector *f  = psVectorAlloc (9, PS_TYPE_F32);
    // XXX if we add the weight above, include df
    // psVector *df = psVectorAlloc (9, PS_TYPE_F32); 
    psVector *x  = psVectorAlloc (9, PS_TYPE_F32);
    psVector *y  = psVectorAlloc (9, PS_TYPE_F32);

    // allocate a 2D polynomial to fit a quadratic to the valid neighbor pixels.
    psPolynomial2D *poly2o = psPolynomial2DAlloc (PS_POLYNOMIAL_ORD, 2, 2);
    poly2o->coeffMask[2][2] = PS_POLY_MASK_SET;
    poly2o->coeffMask[2][1] = PS_POLY_MASK_SET;
    poly2o->coeffMask[1][2] = PS_POLY_MASK_SET;

    // allocate a 2D polynomial to fit a plane to the valid neighbor pixels.
    psPolynomial2D *poly1o = psPolynomial2DAlloc (PS_POLYNOMIAL_ORD, 1, 1);
    poly2o->coeffMask[1][1] = PS_POLY_MASK_SET;

    for (int iy = 0; iy < state->numRows; iy++) {
	for (int ix = 0; ix < state->numCols; ix++) {

	    switch (state->data.S32[iy][ix]) {
	      case PS_IMAGE_INTERPOLATE_GOOD2: {
		  // XXX is there a fit-image-region function?
		  int n = 0;
		  for (int jy = -1; jy <= +1; jy++) {
		      // skip invalid pixels 
		      if (jy + iy < 0) { continue; }
		      if (jy + iy >= value->numRows) { continue; }
		      for (int jx = -1; jx <= +1; jx++) {
			  // skip invalid pixels 
			  if (jx + ix < 0) { continue; } 
			  if (jx + ix >= value->numCols) { continue; } 
			  // skip masked pixels
			  if (mask->data.PS_TYPE_IMAGE_MASK_DATA[iy+jy][ix+jx] & maskVal) { continue; }
			  x->data.F32[n] = xCoord->data.F32[iy+jy][ix+jx];
			  y->data.F32[n] = yCoord->data.F32[iy+jy][ix+jx];
			  f->data.F32[n] = value->data.F32[iy+jy][ix+jx];
			  // df->data.F32[n] = weight->data.F32[iy+jy][ix+jx];
			  n++;
		      }
		  }
		  // set vector lengths here
		  x->n = n;
		  y->n = n;
		  f->n = n;
		  // df->n = n;
		  // psVectorFitPolynomial2D (poly, NULL, 0xff, f, df, x, y);
		  psVectorFitPolynomial2D (poly2o, NULL, 0xff, f, NULL, x, y);
		  // apply the fitted quadratic to get the poor pixel value
		  // center of pixel is 0.5,0.5
		  output->data.F32[iy][ix] = psPolynomial2DEval (poly2o, ix + 0.5, iy + 0.5);
		  break; }

	      case PS_IMAGE_INTERPOLATE_GOOD1: {
		  // XXX is there a fit-image-region function?
		  int n = 0;
		  for (int jy = -1; jy <= +1; jy++) {
		      // skip invalid pixels 
		      if (jy + iy < 0) { continue; }
		      if (jy + iy >= value->numRows) { continue; }
		      for (int jx = -1; jx <= +1; jx++) {
			  // skip invalid pixels 
			  if (jx + ix < 0) { continue; } 
			  if (jx + ix >= value->numCols) { continue; } 
			  // skip masked pixels
			  if (mask->data.PS_TYPE_IMAGE_MASK_DATA[iy+jy][ix+jx] & maskVal) { continue; }
			  x->data.F32[n] = xCoord->data.F32[iy+jy][ix+jx];
			  y->data.F32[n] = yCoord->data.F32[iy+jy][ix+jx];
			  f->data.F32[n] = value->data.F32[iy+jy][ix+jx];
			  // df->data.F32[n] = weight->data.F32[iy+jy][ix+jx];
			  n++;
		      }
		  }
		  // set vector lengths here
		  x->n = n;
		  y->n = n;
		  f->n = n;
		  // df->n = n;
		  // psVectorFitPolynomial2D (poly1o, NULL, 0xff, f, df, x, y);
		  psVectorFitPolynomial2D (poly1o, NULL, 0xff, f, NULL, x, y);
		  // apply the fitted quadratic to get the poor pixel value
		  output->data.F32[iy][ix] = psPolynomial2DEval (poly1o, ix + 0.5, iy + 0.5);
		  break; }

	      case PS_IMAGE_INTERPOLATE_GOOD0: {
		  output->data.F32[iy][ix] = value->data.F32[iy][ix];
		  break; }

	      default:
		// skip poor or bad pixels (interpolate later)
		break;
	    }
	}	    
    }

    for (int iy = 0; iy < value->numRows; iy++) {
	for (int ix = 0; ix < value->numCols; ix++) {
	  if (mask->data.PS_TYPE_IMAGE_MASK_DATA[iy][ix] & maskVal) { continue; }
	  value->data.F32[iy][ix] = output->data.F32[iy][ix];
	}
    }

    psFree (x);
    psFree (y);
    psFree (f);

    psFree (poly2o);
    psFree (poly1o);

    psFree (output);
    return true;
}
