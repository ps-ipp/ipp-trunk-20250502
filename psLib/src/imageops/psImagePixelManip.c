/** @file  psImagePixelManip.c
 *
 *  @brief Contains basic image pixel and geometry manipulation operations, as
 *         specified in the PSLIB SDRS sections "Image Pixel Manipulations" and
 *         "Image Geometry Manipulations".
 *
 *  @ingroup Image
 *
 *  @author Robert DeSonia, MHPCC
 *  @author Ross Harman, MHPCC
 *
 *  @version $Revision: 1.24 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2007-04-22 22:22:09 $
 *
 *  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <math.h>                          // for isfinite(), etc.
#include <stdlib.h>
#include <string.h>                        // for memcpy, etc.

#include "psImagePixelManip.h"

#include "psError.h"
#include "psImage.h"
#include "psStats.h"
#include "psMemory.h"
#include "psAssert.h"

#include "psCoord.h"

int psImageClip(psImage* input,
                double min,
                double vmin,
                double max,
                double vmax)
{
    psS32 numClipped = 0;
    psU32 numRows;
    psU32 numCols;

    if (input == NULL) {
        return 0;
    }

    if (max < min) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                _("Specified min value, %g, can not be greater than the specified max value, %g."),
                (double)min,(double)max);
        return 0;
    }

    numRows = input->numRows;
    numCols = input->numCols;

    switch (input->type.type) {

        #define psImageClipCase(type) \
    case PS_TYPE_##type: { \
            if (vmin < PS_MIN_##type || vmin > PS_MAX_##type) { \
                psError(PS_ERR_BAD_PARAMETER_VALUE, true, \
                        _("Specified %s value, %g, is outside of psImage type's range (%s: %g to %g)."), \
                        "vmin",vmin, PS_TYPE_##type##_NAME, \
                        (psF64)PS_MIN_##type,(psF64)PS_MAX_##type); \
            } \
            if (vmax > PS_MAX_##type || vmax < PS_MIN_##type) { \
                psError(PS_ERR_BAD_PARAMETER_VALUE, true, \
                        _("Specified %s value, %g, is outside of psImage type's range (%s: %g to %g)."), \
                        "vmax",vmax, PS_TYPE_##type##_NAME, \
                        (psF64)PS_MIN_##type,(psF64)PS_MAX_##type); \
            } \
            for (psU32 row = 0;row<numRows;row++) { \
                ps##type* inputRow = input->data.type[row]; \
                for (psU32 col = 0; col < numCols; col++) { \
                    if ((psF64)inputRow[col] < min) { \
                        inputRow[col] = (ps##type)vmin; \
                        numClipped++; \
                    } else if ((psF64)inputRow[col] > max) { \
                        inputRow[col] = (ps##type)vmax; \
                        numClipped++; \
                    } \
                } \
            } \
        } \
        break;

        psImageClipCase(S8)
        psImageClipCase(S16)
        psImageClipCase(S32)            // Not a requirement
        psImageClipCase(S64)            // Not a requirement
        psImageClipCase(U8)
        psImageClipCase(U16)
        psImageClipCase(U32)            // Not a requirement
        psImageClipCase(U64)            // Not a requirement
        psImageClipCase(F32)
        psImageClipCase(F64)

    default: {
            char* typeStr;
            PS_TYPE_NAME(typeStr,input->type.type);
            psError(PS_ERR_BAD_PARAMETER_TYPE, true,
                    _("Specified psImage type, %s, is not supported."),
                    typeStr);
        }
    }

    return numClipped;
}

int psImageClipNaN(psImage* input,
                   float value)
{
    psS32 numClipped = 0;
    psU32 numRows;
    psU32 numCols;

    if (input == NULL) {
        return 0;
    }
    numRows = input->numRows;
    numCols = input->numCols;

    switch (input->type.type) {

        #define psImageClipNaNCase(type) \
    case PS_TYPE_##type: \
        for (psU32 row = 0;row<numRows;row++) { \
            ps##type* inputRow = input->data.type[row]; \
            for (psU32 col = 0; col < numCols; col++) { \
                if (! isfinite(inputRow[col])) { \
                    inputRow[col] = (ps##type)value; \
                    numClipped++; \
                } \
            } \
        } \
        break;

        psImageClipNaNCase(F32)
        psImageClipNaNCase(F64)

    default: {
            char* typeStr;
            PS_TYPE_NAME(typeStr,input->type.type);
            psError(PS_ERR_BAD_PARAMETER_TYPE, true,
                    _("Specified psImage type, %s, is not supported."),
                    typeStr);
        }
    }

    return numClipped;
}

// XXX why does this have an x0,y0?  does it respect col0,row0 (in either image?)
int psImageOverlaySection(psImage* image,
                          const psImage* overlay,
                          int x0,
                          int y0,
                          const char *op)
{
    psU32 imageNumRows;
    psU32 imageNumCols;
    psU32 overlayNumRows;
    psU32 overlayNumCols;
    psU32 imageRowLimit;
    psU32 imageColLimit;
    psElemType type;
    psU32 pixelsOverlaid = 0;

    if (image == NULL || overlay == NULL) {
        psError(PS_ERR_BAD_PARAMETER_NULL, true,
                _("Can not operate on a NULL psImage."));
        return pixelsOverlaid;
    }

    if (op == NULL) {
        psError(PS_ERR_BAD_PARAMETER_NULL, true,
                _("Operation can not be NULL."));
        return pixelsOverlaid;
    }

    type = image->type.type;

    if (type != overlay->type.type) {
        char* typeStr;
        char* typeStrOverlay;
        PS_TYPE_NAME(typeStr,type);
        PS_TYPE_NAME(typeStrOverlay,overlay->type.type);
        psError(PS_ERR_BAD_PARAMETER_TYPE, true,
                _("Input overlay psImage type, %s, must match input psImage type, %s."),
                typeStrOverlay, typeStr);
        return pixelsOverlaid;
    }

    imageNumRows = image->numRows;
    imageNumCols = image->numCols;
    overlayNumRows = overlay->numRows;
    overlayNumCols = overlay->numCols;
    imageRowLimit = y0 + overlayNumRows;
    imageColLimit = x0 + overlayNumCols;

    /* check to see if overlay is within the input image */
    if ( y0 < 0 ||
            x0 < 0 ||
            imageRowLimit > imageNumRows ||
            imageColLimit > imageNumCols) {

        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                _("Specified subset range, [%d:%d,%d:%d], is invalid or outside input psImage's boundaries, [0:%d,0:%d]."),
                x0, imageColLimit, y0, imageRowLimit,
                imageNumCols, imageNumRows);
        return pixelsOverlaid;
    }

    // this function is only called for float-type values
#define psImageOverlayLoopClean(DATATYPE,OP) {	 \
      for (int row=y0;row<imageRowLimit;row++) {	    \
	ps##DATATYPE* imageRow = image->data.DATATYPE[row];	   \
	ps##DATATYPE* overlayRow = overlay->data.DATATYPE[row-y0]; \
	for (int col=x0;col<imageColLimit;col++) {		   \
	  if (!isfinite(imageRow[col])) {			   \
	    imageRow[col] OP overlayRow[col-x0];		   \
	  }							   \
	  else if (!isfinite(overlayRow[col-x0])) {		   \
	    imageRow[col] = imageRow[col];			   \
	  }							   \
	  else {						   \
	    imageRow[col] = overlayRow[col-x0];			   \
	  }								\
	}								\
      }									\
      pixelsOverlaid += (imageRowLimit - y0) * (imageColLimit - x0);	\
    }

#define psImageOverlayLoopCleanInt(DATATYPE,OP) {			\
      for (int row=y0;row<imageRowLimit;row++) {			\
	ps##DATATYPE* imageRow = image->data.DATATYPE[row];		\
	ps##DATATYPE* overlayRow = overlay->data.DATATYPE[row-y0];	\
	for (int col=x0;col<imageColLimit;col++) {			\
	  imageRow[col] = overlayRow[col-x0];				\
	}								\
      }									\
      pixelsOverlaid += (imageRowLimit - y0) * (imageColLimit - x0);	\
    }

    // test for int type (but compiler does not allow int type to go to isfinit)
    //	bool isFloatType = ((PS_TYPE_##DATATYPE == PS_TYPE_F32) || (PS_TYPE_##DATATYPE == PS_TYPE_F64)); 

    // this function is only called for int type data, so we cannot test for isfinite
#define psImageOverlayLoopMask(DATATYPE) {				\
      for (int row=y0;row<imageRowLimit;row++) {			\
	ps##DATATYPE* imageRow = image->data.DATATYPE[row];		\
	ps##DATATYPE* overlayRow = overlay->data.DATATYPE[row-y0];	\
	for (int col=x0;col<imageColLimit;col++) {			\
	  imageRow[col] &= overlayRow[col-x0];				\
	}								\
      }									\
      pixelsOverlaid += (imageRowLimit - y0) * (imageColLimit - x0);	\
    }
	
    
    
    #define psImageOverlayLoop(DATATYPE,OP) { \
        for (int row=y0;row<imageRowLimit;row++) { \
            ps##DATATYPE* imageRow = image->data.DATATYPE[row]; \
            ps##DATATYPE* overlayRow = overlay->data.DATATYPE[row-y0]; \
            for (int col=x0;col<imageColLimit;col++) {		       \
                imageRow[col] OP overlayRow[col-x0];		       \
	      }							       \
        }							       \
        pixelsOverlaid += (imageRowLimit - y0) * (imageColLimit - x0); \
      }

    #define psImageOverlayLoopDivide(DATATYPE,BADVALUE) { \
        for (int row=y0;row<imageRowLimit;row++) { \
            ps##DATATYPE* imageRow = image->data.DATATYPE[row]; \
            ps##DATATYPE* overlayRow = overlay->data.DATATYPE[row-y0]; \
            for (int col=x0;col<imageColLimit;col++) { \
                if (overlayRow[col-x0] == 0) { \
                    imageRow[col] = BADVALUE; \
                    continue; \
                } \
                imageRow[col] /= overlayRow[col-x0]; \
            } \
        } \
        pixelsOverlaid += (imageRowLimit - y0) * (imageColLimit - x0); \
    }

    // Use memcpy to perform the '=' operation.  Depending on the particular application, it can be about 20%
    // faster than using a 'for' loop.  Josh Hoblitt says it has an additional advantage that it doesn't blow
    // away the L2 cache.  Of course, if you want to use the result immediately afterwards, perhaps this is
    // a drawback?  We fall back on the loop if we have to change types.
    #define psImageOverlaySetLoop(DATATYPE) { \
        if (image->type.type == overlay->type.type) { \
            int numBytes = (imageColLimit - x0) * sizeof(ps##DATATYPE); \
            for (int row = y0; row < imageRowLimit; row++) { \
                ps##DATATYPE *imageRow = image->data.DATATYPE[row]; \
                ps##DATATYPE *overlayRow = overlay->data.DATATYPE[row - y0]; \
                memcpy(&imageRow[x0], overlayRow, numBytes); \
            } \
            pixelsOverlaid += (imageRowLimit - y0) * (imageColLimit - x0); \
        } else { \
            psImageOverlayLoop(DATATYPE,=); \
        } \
    }

    #define psImageOverlayCase(DATATYPE,BADVALUE) \
case PS_TYPE_##DATATYPE: \
    switch (*op) { \
    case '+': \
        psImageOverlayLoop(DATATYPE,+=); \
        break; \
    case '-': \
        psImageOverlayLoop(DATATYPE,-=); \
        break; \
    case '*': \
        psImageOverlayLoop(DATATYPE,*=); \
        break; \
    case 'E': \
      psImageOverlayLoopClean(DATATYPE,=); \
      break;				   \
    case '/': \
        psImageOverlayLoopDivide(DATATYPE,BADVALUE); \
        break; \
    case '=': \
        psImageOverlaySetLoop(DATATYPE);		\
        break; \
    default: \
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, \
                _("Specified operation, '%s', is not supported."), \
                op); \
        return pixelsOverlaid; \
    } \
    break;

    // this function is only called for Int type data
    #define psImageOverlayCaseInt(DATATYPE,BADVALUE) \
case PS_TYPE_##DATATYPE: \
    switch (*op) { \
    case '+': \
        psImageOverlayLoop(DATATYPE,+=); \
        break; \
    case '-': \
        psImageOverlayLoop(DATATYPE,-=); \
        break; \
    case '*': \
        psImageOverlayLoop(DATATYPE,*=); \
        break; \
    case 'E': \
      psImageOverlayLoopCleanInt(DATATYPE,=); \
      break;				   \
    case 'M':				   \
      psImageOverlayLoopMask(DATATYPE);	   \
      break;				   \
    case '/': \
        psImageOverlayLoopDivide(DATATYPE,BADVALUE); \
        break; \
    case '=': \
        psImageOverlaySetLoop(DATATYPE);		\
        break; \
    default: \
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, \
                _("Specified operation, '%s', is not supported."), \
                op); \
        return pixelsOverlaid; \
    } \
    break;

    switch (type) {
        psImageOverlayCaseInt(U8, 0);
        psImageOverlayCaseInt(U16,0);
        psImageOverlayCaseInt(U32,0);       // Not a requirement
        psImageOverlayCaseInt(U64,0);       // Not a requirement
        psImageOverlayCaseInt(S8, 0);
        psImageOverlayCaseInt(S16,0);
        psImageOverlayCaseInt(S32,0);       // Not a requirement
        psImageOverlayCaseInt(S64,0);       // Not a requirement
        psImageOverlayCase(F32,NAN);
        psImageOverlayCase(F64,NAN);

    default: {
            char* typeStr;
            PS_TYPE_NAME(typeStr,type);
            psError(PS_ERR_BAD_PARAMETER_TYPE, true,
                    _("Specified psImage type, %s, is not supported."),
                    typeStr);
            return pixelsOverlaid;
        }
    }

    return pixelsOverlaid;
}

