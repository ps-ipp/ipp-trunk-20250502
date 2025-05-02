/** @file  psImageGeomManip.c
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
 *  @version $Revision: 1.45 $ $Name: not supported by cvs2svn $
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

#include "psImageGeomManip.h"

#include "psAbort.h"
#include "psError.h"
#include "psImage.h"
#include "psImageStructManip.h"
#include "psStats.h"
#include "psMemory.h"
#include "psAssert.h"
#include "psImageInterpolate.h"
#include "psCoord.h"

psImage* psImageRebin(psImage* out,
                      const psImage* in,
                      const psImage* mask,
                      psImageMaskType maskVal,
                      int scale,
                      const psStats* stats)
{
    psS32 inRows;
    psS32 inCols;
    psS32 outRows;
    psS32 outCols;
    psVector *vec;                     // vector to hold the values of a single bin.
    psVector *maskVec = NULL;          // vector to hold the mask of a single bin.
    psVectorMaskType *maskData = NULL;
    psStats *myStats;

    if (in == NULL) {
        psError(PS_ERR_BAD_PARAMETER_NULL, true,
                _("Can not operate on a NULL psImage."));
        psFree(out);
        return NULL;
    }

    if (scale < 1) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                _("Specified scale value, %d, must be a positive value."),
                scale);
        psFree(out);
        return NULL;
    }

    if (stats == NULL) {
        psError(PS_ERR_BAD_PARAMETER_NULL, true,
                _("Specified statistic can not be NULL."));
        psFree(out);
        return NULL;
    }

    psStatsOptions statistic = psStatsSingleOption(stats->options); // Statistics option to use
    if (statistic == 0) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                _("Specified statistic option, %d, is not valid.  Must specify one and only one statistic type."),
                stats->options);
        psFree(out);
        return NULL;
    }

    vec = psVectorAllocEmpty(scale * scale, in->type.type);

    if (mask != NULL) {
        if (mask->type.type != PS_TYPE_IMAGE_MASK) {
            char* typeStr;
            PS_TYPE_NAME(typeStr,mask->type.type);
            psError(PS_ERR_BAD_PARAMETER_TYPE, true,
                    _("Input psImage mask type, %s, is not the supported mask datatype of %s."),
                    typeStr, PS_TYPE_IMAGE_MASK_NAME);
            psFree(out);
            psFree(vec);
            return NULL;
        }
        maskVec = psVectorAllocEmpty(scale * scale, PS_TYPE_VECTOR_MASK);
        maskData = maskVec->data.PS_TYPE_VECTOR_MASK_DATA;
    }

    myStats = psAlloc(sizeof(psStats));
    *myStats = *stats;

    // create output image.
    inRows = in->numRows;
    inCols = in->numCols;
    outRows = (inRows + scale - 1) / scale;     // round-up for remainders
    outCols = (inCols + scale - 1) / scale;     // round-up for remainders
    out = psImageRecycle(out, outCols, outRows, in->type.type);

#define PS_IMAGE_REBIN_CASE(TYPE)					\
    case PS_TYPE_##TYPE: {						\
        ps##TYPE *outRowData;						\
        ps##TYPE *vecData = vec->data.TYPE;				\
        psImageMaskType *inRowMask = NULL;				\
        for (psS32 row = 0; row < outRows; row++) {			\
            outRowData = out->data.TYPE[row];				\
            psS32 inCurrentRow = row * scale;				\
            psS32 inNextRow = (row + 1) * scale;			\
            for (psS32 col = 0; col < outCols; col++) {			\
                psS32 inCurrentCol = col * scale;			\
                psS32 inNextCol = (col + 1) * scale;			\
                psS32 n = 0;						\
                for (psS32 inRow = inCurrentRow; inRow < inNextRow && inRow < inRows; inRow++) { \
                    ps##TYPE* inRowData = in->data.TYPE[inRow];		\
                    if (mask != NULL) {					\
                        inRowMask = mask->data.PS_TYPE_IMAGE_MASK_DATA[inRow]; \
                    }							\
                    for (psS32 inCol = inCurrentCol; inCol < inNextCol && inCol < inCols; inCol++) { \
                        if (maskData != NULL) {				\
                            maskData[n] = (inRowMask[inCol] & maskVal); \
                        }						\
                        vecData[n++] = inRowData[inCol];		\
                    }							\
                }							\
                vec->n = n;						\
                if (maskVec) {						\
                    maskVec->n = n;					\
                }							\
		if (!psVectorStats(myStats, vec, NULL, maskVec, 0xff)) { /* the mask vector has only 0 or 1 */ \
		    psError(PS_ERR_UNKNOWN, false, "failure to measure stats"); \
		    psFree(out);					\
		    out = NULL;						\
		    goto escape;					\
		}							\
		outRowData[col] = (ps##TYPE)psStatsGetValue(myStats, statistic); \
	    }								\
	}								\
    }									\
    break;

    switch (in->type.type) {
        //        PS_IMAGE_REBIN_CASE(U8);       Not valid since psVectorStats doesn't allow
        PS_IMAGE_REBIN_CASE(U16);
        PS_IMAGE_REBIN_CASE(U32);      // Not a requirement
        PS_IMAGE_REBIN_CASE(U64);      // Not a requirement
        PS_IMAGE_REBIN_CASE(S8);
        //        PS_IMAGE_REBIN_CASE(S16);      Not valid since psVectorStats doesn't allow
        PS_IMAGE_REBIN_CASE(S32);      // Not a requirement
        PS_IMAGE_REBIN_CASE(S64);      // Not a requirement
        PS_IMAGE_REBIN_CASE(F32);
        PS_IMAGE_REBIN_CASE(F64);

    default: {
            char* typeStr;
            PS_TYPE_NAME(typeStr,in->type.type);
            psError(PS_ERR_BAD_PARAMETER_TYPE, true, _("Specified psImage type, %s, is not supported."), typeStr);
            psFree(out);
            out = NULL;
        }
    }

escape:
    psFree(vec);
    psFree(maskVec);
    psFree(myStats);

    return out;
}

psImage* psImageResample(psImage* out,
                         const psImage* in,
                         int scale,
                         psImageInterpolateMode mode)
{
    psS32 outRows;
    psS32 outCols;
    float invScale;

    if (in == NULL) {
        psError(PS_ERR_BAD_PARAMETER_NULL, true,
                _("Can not operate on a NULL psImage."));
        psFree(out);
        return NULL;
    }

    if (scale < 1) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                _("Specified scale value, %d, must be a positive value."),
                scale);
        psFree(out);
        return NULL;
    }

    // create an output image of the same size
    // and type
    outRows = in->numRows * scale;
    outCols = in->numCols * scale;
    invScale = 1.0f / (float)scale;

    psImageInterpolation *interp = psImageInterpolationAlloc(mode, in, NULL, NULL, 0, NAN, NAN, 0, 0, 0, 0);

    #define PSIMAGE_RESAMPLE_CASE(TYPE) \
case PS_TYPE_##TYPE: { \
        out = psImageRecycle(out,outCols, outRows, PS_TYPE_##TYPE); \
        for (psS32 row=0;row<outRows;row++) { \
            ps##TYPE* rowData = out->data.TYPE[row]; \
            float inRow = (float)row * invScale; \
            for (psS32 col=0;col<outCols;col++) { \
                double value; \
                if (!psImageInterpolate(&value, NULL, NULL, (float)col*invScale, inRow, interp)) { \
                    psError(PS_ERR_UNKNOWN, false, "Unable to interpolate image."); \
                    psFree(interp); \
                    psFree(out); \
                    return NULL; \
                } \
                rowData[col] = value; \
            } \
        }  \
        break; \
    }

    switch (in->type.type) {
        PSIMAGE_RESAMPLE_CASE(U8)
        PSIMAGE_RESAMPLE_CASE(U16)
        PSIMAGE_RESAMPLE_CASE(U32)
        PSIMAGE_RESAMPLE_CASE(U64)
        PSIMAGE_RESAMPLE_CASE(S8)
        PSIMAGE_RESAMPLE_CASE(S16)
        PSIMAGE_RESAMPLE_CASE(S32)
        PSIMAGE_RESAMPLE_CASE(S64)
        PSIMAGE_RESAMPLE_CASE(F32)
        PSIMAGE_RESAMPLE_CASE(F64)
    default: {
            char* typeStr;
            PS_TYPE_NAME(typeStr,in->type.type);
            psError(PS_ERR_BAD_PARAMETER_TYPE, true,
                    _("Specified psImage type, %s, is not supported."),
                    typeStr);
            psFree(out);
            out = NULL;
        }
    }

    psFree(interp);

    return out;
}

psImage* psImageRoll(psImage* out,
                     const psImage* input,
                     int dx,
                     int dy)
{
    psS32 outRows;
    psS32 outCols;
    psS32 elementSize;

    if (input == NULL) {
        psError(PS_ERR_BAD_PARAMETER_NULL, true,
                _("Can not operate on a NULL psImage."));
        psFree(out);
        return NULL;
    }
    // create an output image of the same size
    // and type
    outRows = input->numRows;
    outCols = input->numCols;
    elementSize = PSELEMTYPE_SIZEOF(input->type.type);
    out = psImageRecycle(out, outCols, outRows, input->type.type);

    // make dx and dy between 0 and outCols or
    // outRows, respectively
    dx = dx % outCols;
    dy = dy % outRows;
    if (dx < 0) {
        dx += outCols;
    }
    if (dy < 0) {
        dy += outRows;
    }

    psS32 segment1Size = elementSize * (outCols - dx);
    psS32 segment2Size = elementSize * dx;

    for (psS32 row = 0; row < outRows; row++) {
        psS32 inRowNumber = row + dy;

        if (inRowNumber >= outRows) {
            inRowNumber -= outRows;
        }
        psU8* inRow = input->data.U8[inRowNumber]; // use byte arithmetic for all types
        psU8* outRow = out->data.U8[row];

        memcpy(outRow, inRow + segment2Size, segment1Size);
        memcpy(outRow + segment1Size, inRow, segment2Size);
    }

    return out;
}

psImage* psImageRotate(psImage* out,
                       const psImage* input,
                       float angle,
                       double exposed,
                       psImageInterpolateMode mode)
{
    if (input == NULL) {
        psError(PS_ERR_BAD_PARAMETER_NULL, true,
                _("Can not operate on a NULL psImage."));
        psFree(out);
        return NULL;
    }
    // put the angle in the range of 0...2PI.
    angle = (float)((double)angle - (2.0*M_PI) * floor(angle / (2.0*M_PI)));

    if (fabsf(angle - M_PI_2) < FLT_EPSILON) {
        // perform 1/4 rotate counter-clockwise
        psS32 numRows = input->numCols;
        psS32 numCols = input->numRows;
        psS32 lastCol = numCols - 1;
        psElemType type = input->type.type;

        out = psImageRecycle(out, numCols, numRows, type);

        #define PSIMAGE_ROTATE_LEFT_90(TYPE) \
    case PS_TYPE_##TYPE: { \
            ps##TYPE** inData = input->data.TYPE; \
            for (psS32 row=0;row<numRows;row++) { \
                ps##TYPE* outRow = out->data.TYPE[row]; \
                for (psS32 col=0;col<numCols;col++) { \
                    outRow[col] = inData[lastCol-col][row]; \
                } \
            } \
        } \
        break;

        switch (type) {
            PSIMAGE_ROTATE_LEFT_90(U8);
            PSIMAGE_ROTATE_LEFT_90(U16);
            PSIMAGE_ROTATE_LEFT_90(U32);    //  Not a requirement
            PSIMAGE_ROTATE_LEFT_90(U64);    //  Not a requirement
            PSIMAGE_ROTATE_LEFT_90(S8);
            PSIMAGE_ROTATE_LEFT_90(S16);
            PSIMAGE_ROTATE_LEFT_90(S32);    //  Not a requirement
            PSIMAGE_ROTATE_LEFT_90(S64);    //  Not a requirement
            PSIMAGE_ROTATE_LEFT_90(F32);
            PSIMAGE_ROTATE_LEFT_90(F64);

        default: {
                char* typeStr;
                PS_TYPE_NAME(typeStr,type);
                psError(PS_ERR_BAD_PARAMETER_TYPE, true,
                        _("Specified psImage type, %s, is not supported."),
                        typeStr);
                psFree(out);
                return NULL;
            }
        }
    } else if (fabsf(angle - M_PI) < FLT_EPSILON) {
        // perform 1/2 rotate
        psS32 numRows = input->numRows;
        psS32 lastRow = numRows - 1;
        psS32 numCols = input->numCols;
        psS32 lastCol = numCols - 1;
        psElemType type = input->type.type;

        out = psImageRecycle(out, numCols, numRows, type);

        #define PSIMAGE_ROTATE_180_CASE(TYPE) \
    case PS_TYPE_##TYPE: { \
            for (psS32 row=0;row<numRows;row++) { \
                ps##TYPE* outRow = out->data.TYPE[row]; \
                ps##TYPE* inRow = input->data.TYPE[lastRow-row]; \
                for (psS32 col=0;col<numCols;col++) { \
                    outRow[col] = inRow[lastCol - col]; \
                } \
            } \
        } \
        break;

        switch (type) {
            PSIMAGE_ROTATE_180_CASE(U8);
            PSIMAGE_ROTATE_180_CASE(U16);
            PSIMAGE_ROTATE_180_CASE(U32);    // Not a requirement
            PSIMAGE_ROTATE_180_CASE(U64);    // Not a requirement
            PSIMAGE_ROTATE_180_CASE(S8);
            PSIMAGE_ROTATE_180_CASE(S16);
            PSIMAGE_ROTATE_180_CASE(S32);    // Not a requirement
            PSIMAGE_ROTATE_180_CASE(S64);    // Not a requirement
            PSIMAGE_ROTATE_180_CASE(F32);
            PSIMAGE_ROTATE_180_CASE(F64);

        default: {
                char* typeStr;
                PS_TYPE_NAME(typeStr,type);
                psError(PS_ERR_BAD_PARAMETER_TYPE, true,
                        _("Specified psImage type, %s, is not supported."),
                        typeStr);
                psFree(out);
                return NULL;
            }
        }
    } else if (fabsf(angle - (M_PI+M_PI_2)) < FLT_EPSILON) {
        // perform 1/4 rotate clockwise
        psS32 numRows = input->numCols;
        psS32 lastRow = numRows - 1;
        psS32 numCols = input->numRows;
        psElemType type = input->type.type;

        out = psImageRecycle(out, numCols, numRows, type);

        #define PSIMAGE_ROTATE_RIGHT_90(TYPE) \
    case PS_TYPE_##TYPE: { \
            ps##TYPE** inData = input->data.TYPE; \
            for (psS32 row=0;row<numRows;row++) { \
                ps##TYPE* outRow = out->data.TYPE[row]; \
                for (psS32 col=0;col<numCols;col++) { \
                    outRow[col] = inData[col][lastRow-row]; \
                } \
            } \
        } \
        break;

        switch (type) {
            PSIMAGE_ROTATE_RIGHT_90(U8);
            PSIMAGE_ROTATE_RIGHT_90(U16);
            PSIMAGE_ROTATE_RIGHT_90(U32);     // Not a requirement
            PSIMAGE_ROTATE_RIGHT_90(U64);     // Not a requirement
            PSIMAGE_ROTATE_RIGHT_90(S8);
            PSIMAGE_ROTATE_RIGHT_90(S16);
            PSIMAGE_ROTATE_RIGHT_90(S32);     // Not a requirement
            PSIMAGE_ROTATE_RIGHT_90(S64);     // Not a requirement
            PSIMAGE_ROTATE_RIGHT_90(F32);
            PSIMAGE_ROTATE_RIGHT_90(F64);

        default: {
                char* typeStr;
                PS_TYPE_NAME(typeStr,type);
                psError(PS_ERR_BAD_PARAMETER_TYPE, true,
                        _("Specified psImage type, %s, is not supported."),
                        typeStr);
                psFree(out);
                return NULL;
            }
        }
    } else if (fabsf(angle) < FLT_EPSILON) {
        out = psImageCopy(out, input, input->type.type);
    } else {
        psElemType type = input->type.type;
        psS32 numRows = input->numRows;
        psS32 numCols = input->numCols;
        float centerX = (float)(numCols) / 2.0f;
        float centerY = (float)(numRows) / 2.0f;
        double cosT = cosf(angle);
        double sinT = sinf(angle);

        // calculate the corners of the rotated image so we know the proper output image size.
        // x' = x cos(t) + y sin(t); i.e, x' = (x-centerX)*cosT + (y-centerY)*sinT;
        // y' = y cos(t) - x sin(t); i.e. y' = (y-centerY)*cosT - (x-centerX)*sinT;

        psS32 outCols = ceil(abs(numCols * cosT) + abs(numRows * sinT)) + 1;
        psS32 outRows = ceil(abs(numCols * sinT) + abs(numRows * cosT)) + 1;
        float minX = (float)outCols / -2.0f;
        psS32 intMinY = outRows / -2;

        out = psImageRecycle(out, outCols, outRows, type);

        /* optimized public domain rotation routine by Karl Lager
         *
         * float cosT,sinT;
         * cosT = cos(t);
         * sinT = sin(t);
         * for (y = min_y; y <= max_y; y++) {
         *     x' = min_x * cosT + y * sinT + x1';
         *     y' = y * cosT - min_x * sinT + y1';
         *     for (x = min_x; x <= max_x; x++) {
         *         if (x', y') is in the bounds of the bitmap, get pixel
         *            (x', y') and plot the pixel to (x, y) on screen.
         *         x' += cosT;
         *         y' -= sinT;
         *     }
         * }
         */

        // precalculate some figures that are used within loop
        float minXTimesCosTPlusCenterX = minX * cosT + centerX;
        float CenterYMinusminXTimesSinT = centerY - minX * sinT;

        psImageInterpolation *interp = psImageInterpolationAlloc(mode, input, NULL, NULL, 0,
                                                                 exposed, NAN, 0, 0, 0.0, 0);

        #define PSIMAGE_ROTATE_ARBITRARY_LOOP(TYPE)  \
          case PS_TYPE_##TYPE: { \
            if (exposed < PS_MIN_##TYPE || \
                    exposed > PS_MAX_##TYPE || \
                    exposed < PS_MIN_##TYPE || \
                    exposed > PS_MAX_##TYPE) { \
                psError(PS_ERR_BAD_PARAMETER_VALUE, true, \
                        _("Specified %s value, %g%+gi, is not the the range of input psImage's valid pixel values (%s), i.e. [%g:%g]."), \
                        "exposed", \
                        exposed, exposed, \
                        PS_TYPE_##TYPE##_NAME,  \
                        (double)PS_MIN_##TYPE,(double)PS_MAX_##TYPE); \
                psFree(out); \
                out = NULL; \
                break; \
            } \
            float inX; \
            float inY; \
            ps##TYPE* outRow; \
            for (psS32 y = 0; y < outRows; y++) { \
                inX = minXTimesCosTPlusCenterX + (y+intMinY) * sinT; \
                inY = CenterYMinusminXTimesSinT + (y+intMinY) * cosT; \
                outRow = out->data.TYPE[y]; \
                for (psS32 x = 0; x < outCols; x++) { \
                    double value; \
                    if (!psImageInterpolate(&value, NULL, NULL, inX, inY, interp)) { \
                        psError(PS_ERR_UNKNOWN, false, "Unable to interpolate image."); \
                        psFree(out); \
                        psFree(interp); \
                        return NULL; \
                    } \
                    outRow[x] = value; \
                    inX += cosT; \
                    inY -= sinT; \
                } \
            } \
            break; \
        }

        switch (type) {
            PSIMAGE_ROTATE_ARBITRARY_LOOP(U8);
            PSIMAGE_ROTATE_ARBITRARY_LOOP(U16);
            PSIMAGE_ROTATE_ARBITRARY_LOOP(U32);
            PSIMAGE_ROTATE_ARBITRARY_LOOP(U64);
            PSIMAGE_ROTATE_ARBITRARY_LOOP(S8);
            PSIMAGE_ROTATE_ARBITRARY_LOOP(S16);
            PSIMAGE_ROTATE_ARBITRARY_LOOP(S32);
            PSIMAGE_ROTATE_ARBITRARY_LOOP(S64);
            PSIMAGE_ROTATE_ARBITRARY_LOOP(F32);
            PSIMAGE_ROTATE_ARBITRARY_LOOP(F64);
          default: {
              char* typeStr;
              PS_TYPE_NAME(typeStr,type);
              psError(PS_ERR_BAD_PARAMETER_TYPE, true,
                      _("Specified psImage type, %s, is not supported."),
                      typeStr);
              psFree(out);
              psFree(interp);
              out = NULL;
          }
        }

        psFree(interp);

    }

    return out;
}

psImage* psImageShift(psImage* out,
                      const psImage* input,
                      float dx,
                      float dy,
                      double exposed,
                      psImageInterpolateMode mode)
{
    psS32 outRows;
    psS32 outCols;
    psElemType type;

    if (input == NULL) {
        psError(PS_ERR_BAD_PARAMETER_NULL, true,
                _("Can not operate on a NULL psImage."));
        psFree(out);
        return NULL;
    }
    // create an output image of the same size
    // and type
    outRows = input->numRows;
    outCols = input->numCols;
    type = input->type.type;
    // XXX unused psS32 elementSize = PSELEMTYPE_SIZEOF(type);
    out = psImageRecycle(out, outCols, outRows, type);

    psImageInterpolation *interp = psImageInterpolationAlloc(mode, input, NULL, NULL, 0,
                                                             exposed, NAN, 0, 0, 0.0, 0);

    #define PSIMAGE_SHIFT_CASE(TYPE) \
case PS_TYPE_##TYPE: \
    if (exposed < PS_MIN_##TYPE || \
            exposed > PS_MAX_##TYPE || \
            exposed < PS_MIN_##TYPE || \
            exposed > PS_MAX_##TYPE) { \
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, \
                _("Specified %s value, %g%+gi, is not the the range of input psImage's valid pixel values (%s), i.e. [%g:%g]."), \
                "exposed", \
                exposed,exposed, \
                PS_TYPE_##TYPE##_NAME,  \
                (double)PS_MIN_##TYPE,(double)PS_MAX_##TYPE); \
        psFree(out); \
        out = NULL; \
        break; \
    } \
    /* note: output(i,j) = input(i+0.5-dx,j+0.5-dy) */ \
    /* positive dx,dy moves pixel i-dx,j-dy to i,y */ \
    /* also: pixel center is 0.5,0.5 */ \
    for (psS32 row=0;row<outRows;row++) { \
        ps##TYPE* outRow = out->data.TYPE[row]; \
        float y = row + 0.5 - dy; \
        for (psS32 col=0;col<outCols;col++) { \
            float x = col + 0.5 - dx; \
            double value; \
            if (!psImageInterpolate(&value, NULL, NULL, x, y, interp)) { \
                psError(PS_ERR_UNKNOWN, false, "Unable to interpolate image."); \
                psFree(interp); \
                psFree(out); \
                return NULL; \
            } \
            outRow[col] = value; \
        } \
    } \
    break;

    switch (input->type.type) {
        PSIMAGE_SHIFT_CASE(U8);
        PSIMAGE_SHIFT_CASE(U16);
        PSIMAGE_SHIFT_CASE(U32);
        PSIMAGE_SHIFT_CASE(U64);
        PSIMAGE_SHIFT_CASE(S8);
        PSIMAGE_SHIFT_CASE(S16);
        PSIMAGE_SHIFT_CASE(S32);
        PSIMAGE_SHIFT_CASE(S64);
        PSIMAGE_SHIFT_CASE(F32);
        PSIMAGE_SHIFT_CASE(F64);
      default: {
          char* typeStr;
          PS_TYPE_NAME(typeStr,type);
          psError(PS_ERR_BAD_PARAMETER_TYPE, true, _("Specified psImage type, %s, is not supported."),
                  typeStr);
          psFree(out);
          psFree(interp);
          return NULL;
        }
    }

    psFree(interp);
    return out;
}

bool psImageShiftMask(psImage **out, psImage **outMask, const psImage* in, const psImage *inMask,
                      psImageMaskType maskVal, float dx, float dy, double exposed, psImageMaskType blank,
                      psImageInterpolateMode mode)
{
    PS_ASSERT(out, false);
    PS_ASSERT_IMAGE_NON_NULL(in, false);
    if (inMask) {
        PS_ASSERT_IMAGE_NON_NULL(inMask, false);
        PS_ASSERT_IMAGES_SIZE_EQUAL(in, inMask, false);
        PS_ASSERT_IMAGE_TYPE(inMask, PS_TYPE_IMAGE_MASK, false);
    }

    int numRows = in->numRows, numCols = in->numCols; // Size of image
    psElemType type = in->type.type;    // Type of image

    *out = psImageRecycle(*out, numCols, numRows, type);
    if (outMask) {
        *outMask = psImageRecycle(*outMask, numCols, numRows, PS_TYPE_IMAGE_MASK);
    }

    psImageInterpolation *interp = psImageInterpolationAlloc(mode, in, NULL, inMask, maskVal, exposed, NAN,
                                                             blank, blank, 0.0, 0);

    #define PSIMAGE_SHIFT_MASK_CASE(TYPE) \
case PS_TYPE_##TYPE: \
    if (exposed < PS_MIN_##TYPE || \
            exposed > PS_MAX_##TYPE || \
            exposed < PS_MIN_##TYPE || \
            exposed > PS_MAX_##TYPE) { \
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, \
                _("Specified exposed value, %g%+gi, is not the the range of input image's " \
                  "valid pixel values (%s), i.e. [%g:%g]."), \
                exposed, exposed, \
                PS_TYPE_##TYPE##_NAME,  \
                (double)PS_MIN_##TYPE,(double)PS_MAX_##TYPE); \
        psFree(out); \
        out = NULL; \
        break; \
    } \
    /* note: output(i,j) = input(i+0.5-dx,j+0.5-dy) */ \
    /* positive dx,dy moves pixel i-dx,j-dy to i,j */ \
    /* also: pixel center is 0.5,0.5 */ \
    for (int row = 0; row < numRows; row++) { \
        ps##TYPE* outRow = (*out)->data.TYPE[row]; \
        psImageMaskType *outMaskRow = (outMask ? (*outMask)->data.PS_TYPE_IMAGE_MASK_DATA[row] : NULL); \
        float y = row + 0.5 - dy; \
        for (int col = 0; col < numCols; col++) { \
            float x = col + 0.5 - dx; \
            double value; \
            psImageMaskType valueMask = 0; \
            if (!psImageInterpolate(&value, NULL, &valueMask, x, y, interp)) { \
                psError(PS_ERR_UNKNOWN, false, "Unable to interpolate image."); \
                psFree(interp); \
                return false; \
            } \
            outRow[col] = value; \
            if (outMask) { \
                outMaskRow[col] = valueMask; \
            } \
        } \
    } \
    break;

    switch (type) {
        PSIMAGE_SHIFT_MASK_CASE(U8);
        PSIMAGE_SHIFT_MASK_CASE(U16);
        PSIMAGE_SHIFT_MASK_CASE(U32);
        PSIMAGE_SHIFT_MASK_CASE(U64);
        PSIMAGE_SHIFT_MASK_CASE(S8);
        PSIMAGE_SHIFT_MASK_CASE(S16);
        PSIMAGE_SHIFT_MASK_CASE(S32);
        PSIMAGE_SHIFT_MASK_CASE(S64);
        PSIMAGE_SHIFT_MASK_CASE(F32);
        PSIMAGE_SHIFT_MASK_CASE(F64);
      default: {
          char* typeStr;
          PS_TYPE_NAME(typeStr,type);
          psError(PS_ERR_BAD_PARAMETER_TYPE, true, _("Specified psImage type, %s, is not supported."),
                  typeStr);
          psFree(interp);
          return false;
        }
    }

    psFree(interp);
    return true;
}

psImage* psImageTransform(psImage *output,
                          psPixels** blankPixels,
                          const psImage *input,
                          const psImage *inputMask,
                          psImageMaskType inputMaskVal,
                          const psPlaneTransform *outToIn,
                          psRegion region,
                          const psPixels* pixels,
                          psImageInterpolateMode mode,
                          double exposedValue)
{
    if (input == NULL) {
        psError(PS_ERR_BAD_PARAMETER_NULL, true,
                _("Can not operate on a NULL psImage."));
        psFree(output);
        return NULL;
    }
    psElemType type = input->type.type;

    if (blankPixels != NULL && *blankPixels == NULL) {
        *blankPixels = psPixelsAlloc(0);
    }

    if (inputMask != NULL) {
        if (input->numRows != inputMask->numRows || input->numCols != inputMask->numCols) {
            psError(PS_ERR_BAD_PARAMETER_SIZE, true,
                    _("Input psImage mask size, %dx%d, does not match psImage input size, %dx%d."),
                    input->numCols, input->numRows,
                    inputMask->numCols, inputMask->numRows );
            psFree(output);
            return NULL;
        }
        if (inputMask->type.type != PS_TYPE_IMAGE_MASK) {
            char* typeStr;
            PS_TYPE_NAME(typeStr,inputMask->type.type);
            psError(PS_ERR_BAD_PARAMETER_TYPE, true,
                    _("Input psImage mask type, %s, is not the supported mask datatype of %s."),
                    typeStr, PS_TYPE_IMAGE_MASK_NAME);
            psFree(output);
            return NULL;
        }
    }

    if (outToIn == NULL) {
        psError(PS_ERR_BAD_PARAMETER_NULL, true,
                _("Specified input transform can not be NULL."));
        return NULL;
    }

    int row0;
    int row1;
    int col0;
    int col1;
    int numRows;
    int numCols;
    if (output == NULL) { // output image size is determined by psRegion
        row0 = region.y0;
        row1 = region.y1;
        col0 = region.x0;
        col1 = region.x1;
        //If [0,0,0,0] specified, the whole image is to be included
        if (row0 == 0 && col0 == 0 && row1 == 0 && col1 == 0) {
            row0 = input->row0;
            col0 = input->col0;
            row1 = input->row0 + input->numRows - 1;
            col1 = input->col0 + input->numCols - 1;
        }
        if (col1 < 1) {
            col1 += input->col0 + input->numCols;
        }

        if (row1 < 1) {
            row1 += input->row0 + input->numRows;
        }

        numRows = row1 - row0;
        numCols = col1 - col0;

        if (numRows < 1 || numCols < 1) {
            psError(PS_ERR_BAD_PARAMETER_SIZE, true,
                    "The specified region is invalid.");
            psFree(output);
            return NULL;
        }
        // create the output image.
        output = psImageRecycle(output, numCols, numRows, input->type.type);
        P_PSIMAGE_SET_COL0(output, region.x0);
        P_PSIMAGE_SET_ROW0(output, region.y0);
    } else { // size of output is determined by output parameter
        numRows = output->numRows;
        numCols = output->numCols;
        row0 = output->row0;
        col0 = output->col0;
        row1 = row0+numRows -1;
        col1 = col0+numCols -1;
    }

    // loop through the output image using the domain above and transform
    // each output pixel to input coordinates and use psImageInterpolate
    // to determine the pixel value.
    psPlane outPosition;
    psPlane* inPosition = NULL;

    psImageInterpolation *interp = psImageInterpolationAlloc(mode, input, NULL, inputMask,
                                                             inputMaskVal, NAN, NAN, 0, 0, 0.0, 0);


    #define PSIMAGE_TRANSFORM_DOTRANSFORM(TYPE) \
    /* apply the transform to get the position in the input image */ \
    inPosition = psPlaneTransformApply(inPosition, outToIn, &outPosition); \
    \
    if (inPosition == NULL) { \
        psError(PS_ERR_UNKNOWN, false, \
                "Failed to apply the transform"); \
        psFree(output); \
        return NULL; \
    } \
    /* interpolate the cooresponding input pixel to get the output pixel value. */ \
    double value; \
    if (!psImageInterpolate(&value, NULL, NULL, inPosition->x, inPosition->y, interp)) { \
        psError(PS_ERR_UNKNOWN, false, "Unable to interpolate image."); \
        psFree(output); \
        psFree(interp); \
        return NULL; \
    } \
    /*    psFree(inPosition); */\
    if (isnan(value)) { \
        if (blankPixels != NULL) { \
            psPixelsAdd(*blankPixels, (*blankPixels)->nalloc, outPosition.x, outPosition.y); \
        } \
        value = exposedValue; \
    } \

    #define PSIMAGE_TRANSFORM_CASE(TYPE) \
      case PS_TYPE_##TYPE: { \
        for (int row = 0; row < numRows; row++) { \
            outPosition.y = row+row0; \
            ps##TYPE* outputData=output->data.TYPE[row]; \
            for (int col = 0; col < numCols; col++) { \
                outPosition.x = col+col0; \
                PSIMAGE_TRANSFORM_DOTRANSFORM(TYPE) \
                outputData[col] = value; \
            } \
        } \
        break; \
    }

    switch (type) {
        PSIMAGE_TRANSFORM_CASE(F32);
        PSIMAGE_TRANSFORM_CASE(F64);
      default: {
          char* typeStr;
          PS_TYPE_NAME(typeStr,type);
          psError(PS_ERR_BAD_PARAMETER_TYPE, true, _("Specified psImage type, %s, is not supported."),
                  typeStr);
          psFree(output);
          psFree(inPosition);
          return NULL;
      }
    }

    psFree(inPosition);

    return output;
}


#define FLIP_X_CASE(TYPENAME,TYPE) \
case TYPENAME: { \
    long numRows = input->numRows; \
    long numCols = input->numCols; \
    for (long i = 0; i < numRows; i++) { \
        for (long j = 0; j < numCols; j++) { \
            output->data.TYPE[i][j] = input->data.TYPE[i][numCols - j - 1]; \
        } \
    } \
    break; \
}

#define FLIP_Y_CASE(TYPENAME,TYPE) \
case TYPENAME: { \
    long numRows = input->numRows; \
    long numCols = input->numCols; \
    for (long i = 0; i < numRows; i++) { \
        for (long j = 0; j < numCols; j++) { \
            output->data.TYPE[i][j] = input->data.TYPE[numRows - i - 1][j]; \
        } \
    } \
    break; \
}

psImage *psImageFlip(psImage *output, const psImage *input, bool xFlip, bool yFlip)
{
    PS_ASSERT_IMAGE_NON_NULL(input, NULL);

    if (xFlip && yFlip) {
        // This is equivalent to a 180 degree rotation;
        return psImageRotate(output, input, M_PI, NAN, PS_INTERPOLATE_BILINEAR);
    }

    if (!xFlip && !yFlip) {
        // They want something, so let's give it to them
        return psImageCopy(output, input, input->type.type);
    }

    output = psImageRecycle(output, input->numCols, input->numRows, input->type.type);

    if (xFlip) {
        switch (input->type.type) {
            FLIP_X_CASE(PS_TYPE_U8,  U8);
            FLIP_X_CASE(PS_TYPE_U16, U16);
            FLIP_X_CASE(PS_TYPE_U32, U32);
            FLIP_X_CASE(PS_TYPE_U64, U64);
            FLIP_X_CASE(PS_TYPE_S8,  S8);
            FLIP_X_CASE(PS_TYPE_S16, S16);
            FLIP_X_CASE(PS_TYPE_S32, S32);
            FLIP_X_CASE(PS_TYPE_S64, S64);
            FLIP_X_CASE(PS_TYPE_F32, F32);
            FLIP_X_CASE(PS_TYPE_F64, F64);
        default:
            psFree(output);
            psError(PS_ERR_BAD_PARAMETER_TYPE, true, "Unknown type for input image: %x\n", input->type.type);
            return NULL;
        }
        return output;
    }
    if (yFlip) {
        switch (input->type.type) {
            FLIP_Y_CASE(PS_TYPE_U8,  U8);
            FLIP_Y_CASE(PS_TYPE_U16, U16);
            FLIP_Y_CASE(PS_TYPE_U32, U32);
            FLIP_Y_CASE(PS_TYPE_U64, U64);
            FLIP_Y_CASE(PS_TYPE_S8,  S8);
            FLIP_Y_CASE(PS_TYPE_S16, S16);
            FLIP_Y_CASE(PS_TYPE_S32, S32);
            FLIP_Y_CASE(PS_TYPE_S64, S64);
            FLIP_Y_CASE(PS_TYPE_F32, F32);
            FLIP_Y_CASE(PS_TYPE_F64, F64);
        default:
            psFree(output);
            psError(PS_ERR_BAD_PARAMETER_TYPE, true, "Unknown type for input image: %x\n", input->type.type);
            return NULL;
        }
        return output;
    }

    psAbort("Should never get here.\n");
    return NULL;
}
