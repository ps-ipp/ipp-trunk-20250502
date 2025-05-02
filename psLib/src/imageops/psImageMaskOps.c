/** @file  psImageMaskOps.c
 *
 *  @brief Contains basic image pixel and geometry manipulation operations, as
 *         specified in the PSLIB SDRS sections "Mask Operations"
 *
 *  @ingroup Image
 *
 *  @author David Robbins, MHPCC
 *
 *  @version $Revision: 1.8 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-01-27 06:39:37 $
 *
 *  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <math.h>                          // for isfinite(), etc.
#include <stdlib.h>
#include <string.h>                        // for memcpy, etc.

#include "psImageMaskOps.h"

#include "psError.h"
#include "psImage.h"
#include "psStats.h"
#include "psMemory.h"
#include "psAssert.h"

#include "psCoord.h"

// mask the area contained by the region
// the region is defined wrt the parent image
void psImageMaskRegion(psImage *image,
                       psRegion region,
                       const char *op,
                       psImageMaskType maskValue)
{
    if (image == NULL) {
        psError(PS_ERR_BAD_PARAMETER_NULL, true,
                "Invalid image input.  Image is NULL.\n");
        return;
    }


# define MASK_IT(OP) \
      for (int j = 0; j < image->numRows; j++) { \
	if ((j + image->row0) < region.y0) continue; \
	if ((j + image->row0) > region.y1) continue; /* is this correct (not >= ?) */ \
	for (int i = 0; i < image->numCols; i++) { \
	  if ((i + image->col0) < region.x0) continue; \
	  if ((i + image->col0) > region.x1) continue; /* is this correct (not >= ?) */ \
	  image->data.PS_TYPE_IMAGE_MASK_DATA[j][i] OP maskValue; \
	} \
      }

    if ( !strncmp(op, "&", 2) || !strncmp(op, "AND", 5) ) {
      MASK_IT (&=);
      return;
    }
    if ( !strncmp(op, "|", 2) || !strncmp(op, "OR", 5) ) {
      MASK_IT (|=);
      return;
    }
    if ( !strncmp(op, "=", 2) || !strncmp(op, "EQUAL", 5) ) {
      MASK_IT (=);
      return;
    }
    if ( !strncmp(op, "^", 2) || !strncmp(op, "XOR", 5) ) {
      MASK_IT (^=);
      return;
    }

    psError(PS_ERR_BAD_PARAMETER_VALUE,true,
	    "The logical operation specified is incorrect\n");
    return;
}

// mask the area not contained by the region
// the region is defined wrt the parent image
void psImageKeepRegion(psImage *image,
                       psRegion region,
                       const char *op,
                       psImageMaskType maskValue)
{
    if (image == NULL) {
        psError(PS_ERR_BAD_PARAMETER_NULL, true,
                "Invalid image input.  Image is NULL.\n");
        return;
    }


# define KEEP_IT(OP) \
    for (int j = 0; j < image->numRows; j++) { \
      for (int i = 0; i < image->numCols; i++) { \
	if ((j + image->row0) < region.y0 || \
	    (j + image->row0) > region.y1 || \
	    (i + image->col0) < region.x0 || \
	    (i + image->col0) > region.x1 ) { \
	  image->data.PS_TYPE_IMAGE_MASK_DATA[j][i] OP maskValue; \
	} } }

    if ( !strncmp(op, "&", 2) || !strncmp(op, "AND", 5) ) {
      KEEP_IT(&=);
      return;
    }
    if ( !strncmp(op, "|", 2) || !strncmp(op, "OR", 5) ) {
      KEEP_IT(|=);
      return;
    }
    if ( !strncmp(op, "=", 2) || !strncmp(op, "EQUAL", 5) ) {
      KEEP_IT(=);
      return;
    }
    if ( !strncmp(op, "^", 2) || !strncmp(op, "XOR", 5) ) {
      KEEP_IT(^=);
      return;
    }
    psError(PS_ERR_BAD_PARAMETER_VALUE,true,
	    "The logical operation specified is incorrect\n");
    return;
}

// mask the area contained by the region
// the region is defined wrt the parent image
void psImageMaskCircle(psImage *image,
                       double x,
                       double y,
                       double radius,
                       const char *op,
                       psImageMaskType maskValue)
{
    if (image == NULL) {
        psError(PS_ERR_BAD_PARAMETER_NULL, true,
                "Invalid image input.  Image is NULL.\n");
        return;
    }

    double dx, dy, r2, R2;

    R2 = PS_SQR(radius);

# define MASK_IT_CIRCLE(OP) \
    for (int iy = 0; iy < image->numRows; iy++) { \
        for (int ix = 0; ix < image->numCols; ix++) { \
            dx = ix + 0.5 + image->col0 - x; \
            dy = iy + 0.5 + image->row0 - y; \
            r2 = PS_SQR(dx) + PS_SQR(dy); \
            if (r2 <= R2) { \
	      image->data.PS_TYPE_IMAGE_MASK_DATA[iy][ix] OP maskValue; \
            } } }

    if ( !strncmp(op, "&", 2) || !strncmp(op, "AND", 5) ) {
      MASK_IT_CIRCLE (&=);
      return;
    } 
    if ( !strncmp(op, "|", 2) || !strncmp(op, "OR", 5) ) {
      MASK_IT_CIRCLE (|=);
      return;
    } 
    if ( !strncmp(op, "=", 2) || !strncmp(op, "EQUAL", 5) ) {
      MASK_IT_CIRCLE (=);
      return;
    } 
    if ( !strncmp(op, "^", 2) || !strncmp(op, "XOR", 5) ) {
      MASK_IT_CIRCLE (^=);
      return;
    } 

    psError(PS_ERR_BAD_PARAMETER_VALUE,true,
	    "The logical operation specified is incorrect\n");
    return;
}

// mask the area contained by the region
// the region is defined wrt the parent image
void psImageKeepCircle(psImage *image,
                       double x,
                       double y,
                       double radius,
                       const char *op,
                       psImageMaskType maskValue)
{

    if (image == NULL) {
        psError(PS_ERR_BAD_PARAMETER_NULL, true,
                "Invalid image input.  Image is NULL.\n");
        return;
    }
    double dx, dy, r2, R2;

    R2 = PS_SQR(radius);

# define KEEP_IT_CIRCLE(OP) \
    for (int iy = 0; iy < image->numRows; iy++) { \
        for (int ix = 0; ix < image->numCols; ix++) { \
            dx = ix + 0.5 + image->col0 - x; \
            dy = iy + 0.5 + image->row0 - y; \
            r2 = PS_SQR(dx) + PS_SQR(dy); \
            if (r2 > R2) { \
	      image->data.PS_TYPE_IMAGE_MASK_DATA[iy][ix] OP maskValue; \
            } } }

    if ( !strncmp(op, "&", 2) || !strncmp(op, "AND", 5) ) {
      KEEP_IT_CIRCLE (&=);
      return;
    } 
    if ( !strncmp(op, "|", 2) || !strncmp(op, "OR", 5) ) {
      KEEP_IT_CIRCLE (|=);
      return;
    } 
    if ( !strncmp(op, "=", 2) || !strncmp(op, "EQUAL", 5) ) {
      KEEP_IT_CIRCLE (=);
      return;
    } 
    if ( !strncmp(op, "^", 2) || !strncmp(op, "XOR", 5) ) {
      KEEP_IT_CIRCLE (^=);
      return;
    } 

    psError(PS_ERR_BAD_PARAMETER_VALUE,true,
	    "The logical operation specified is incorrect\n");
    return;
}

// perform the mask operation on the image pixels
void psImageMaskPixels(psImage *image,
                       const char *op,
                       psImageMaskType maskValue)
{
    if (image == NULL) {
        psError(PS_ERR_BAD_PARAMETER_NULL, true, "Invalid image input.  Image is NULL.\n");
        return;
    }

# define MASK_IT_IMAGE(OP) \
    for (int iy = 0; iy < image->numRows; iy++) { \
        for (int ix = 0; ix < image->numCols; ix++) { \
	    image->data.PS_TYPE_IMAGE_MASK_DATA[iy][ix] OP maskValue; \
	} }

    if ( !strncmp(op, "&", 2) || !strncmp(op, "AND", 5) ) {
      MASK_IT_IMAGE (&=);
      return;
    } 
    if ( !strncmp(op, "|", 2) || !strncmp(op, "OR", 5) ) {
      MASK_IT_IMAGE (|=);
      return;
    } 
    if ( !strncmp(op, "=", 2) || !strncmp(op, "EQUAL", 5) ) {
      MASK_IT_IMAGE (=);
      return;
    } 
    if ( !strncmp(op, "^", 2) || !strncmp(op, "XOR", 5) ) {
      MASK_IT_IMAGE (^=);
      return;
    } 

    psError(PS_ERR_BAD_PARAMETER_VALUE, true, "The logical operation specified is incorrect\n");
    return;
}

psImage *psImageGrowMask(psImage *out,
                         const psImage *in,
                         psImageMaskType maskVal,
                         unsigned int growSize,
                         psImageMaskType growVal)
{
    if (in == NULL) {
        psError(PS_ERR_BAD_PARAMETER_NULL, true,
                "Invalid input image.  Input image cannot be NULL.\n");
        return NULL;
    }
    if (in->type.type != PS_TYPE_IMAGE_MASK) {
        psError(PS_ERR_BAD_PARAMETER_TYPE, true,
                "Invalid input image.  Input image type must match psImageMaskType.\n");
        return NULL;
    }
    if (out != NULL) {
        if (out->numCols != in->numCols || out->numRows != in->numRows) {
            psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                    "Invalid out image.  Size of out does not match size of in.\n");
            return NULL;
        }
        if (out->type.type != in->type.type) {
            psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                    "Invalid out image.  Type of out does not match type of in.\n");
            return NULL;
        }
    }
    if (out == NULL) {
        out = psImageAlloc(in->numCols, in->numRows, in->type.type);
    }
    psImage *changed = psImageAlloc(in->numCols, in->numRows, in->type.type);
    int k,l,m,n;
    for (k = 0; k < in->numRows; k++) {
        for (l = 0; l < in->numCols; l++) {
            out->data.PS_TYPE_IMAGE_MASK_DATA[k][l] = in->data.PS_TYPE_IMAGE_MASK_DATA[k][l];
            changed->data.PS_TYPE_IMAGE_MASK_DATA[k][l] = 0;
        }
    }

    for (int i = 0; i < in->numRows; i++) {
        for (int j = 0; j < in->numCols; j++) {
            if ( (in->data.PS_TYPE_IMAGE_MASK_DATA[i][j] & maskVal) != 0 &&
                    changed->data.PS_TYPE_IMAGE_MASK_DATA[i][j] == 0) {
                m = i - growSize;
                if (m < 0) {
                    m = 0;
                }
                for (k = m; k <= (i + growSize) && k < in->numRows; k++) {
                    n = j - growSize;
                    if (n < 0) {
                        n = 0;
                    }
                    for (l = n; l <= (j + growSize) && l < in->numCols; l++) {
                        if (((k-i)*(k-i) + (l-j)*(l-j)) <= (growSize*growSize)) {
                            out->data.PS_TYPE_IMAGE_MASK_DATA[k][l] |= growVal;
                            if ( (in->data.PS_TYPE_IMAGE_MASK_DATA[i][j] & maskVal) == 0 ) {
                                changed->data.PS_TYPE_IMAGE_MASK_DATA[k][l] = 1;
                            }
                        }
                    }
                }
            }
        }
    }
    psFree(changed);
    return out;
}

