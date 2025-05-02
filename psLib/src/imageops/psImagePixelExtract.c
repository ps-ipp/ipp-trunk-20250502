/** @file  psImagePixelExtract.c
 *
 *  @brief Contains basic image extraction operations, as specified in the
 *         PSLIB SDRS sections "Image Pixel Extractions".
 *
 *  @ingroup Image
 *
 *  @author Robert DeSonia, MHPCC
 *
 *  @version $Revision: 1.34 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-01-27 06:39:37 $
 *
 *  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 *
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <string.h>

#include "psMemory.h"
#include "psVector.h"
#include "psError.h"
#include "psImage.h"
#include "psImageInterpolate.h"
#include "psImagePixelExtract.h"

#define VECTOR_STORE_ROW_CASE(TYPE) \
case PS_TYPE_##TYPE: \
memcpy(out->data.TYPE, input->data.TYPE[row], input->numCols*sizeof(ps##TYPE)); \
break;

psVector *psImageRow(psVector *out,
                     const psImage *input,
                     int row)
{
    if (input == NULL || input->data.V == NULL) {
        psError(PS_ERR_BAD_PARAMETER_NULL, true,
                _("Can not operate on a NULL psImage."));
        psFree(out);
        return NULL;
    }
    if (input->col0 < 0) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                "psImage input is invalid.  col0 cannot be negative.\n");
        psFree(out);
        return NULL;
    }
    if (input->row0 < 0) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                "psImage input is invalid.  row0 cannot be negative.\n");
        psFree(out);
        return NULL;
    }
    if (input->numCols < 1) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                "psImage input is invalid.  numCols must be greater than 0.\n");
        psFree(out);
        return NULL;
    }
    if (input->numRows < 1) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                "psImage input is invalid.  numRows must be greater than 0.\n");
        psFree(out);
        return NULL;
    }
    if (row >= (input->numRows + input->row0) ) {
        psError(PS_ERR_BAD_PARAMETER_NULL, true,
                "Specified row number is out of range for specified image.\n");
        psFree(out);
        return NULL;
    } else if ( row < input->row0 && row >= 0 ) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                "Specified row number is out of range for specified image.\n");
        psFree(out);
        return NULL;
    } else if ( row < 0 ) {
        row += input->numRows;
        if ( row < 0 ) {
            psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                    "Specified row number is out of range for specified image.\n");
            psFree(out);
            return NULL;
        }
    } else {
        row -= input->row0;
    }

    out = psVectorRecycle(out, input->numCols, input->type.type);
    out->n = input->numCols;

    switch (input->type.type) {
        VECTOR_STORE_ROW_CASE(S8);
        VECTOR_STORE_ROW_CASE(S16);
        VECTOR_STORE_ROW_CASE(S32);
        VECTOR_STORE_ROW_CASE(S64);
        VECTOR_STORE_ROW_CASE(U8);
        VECTOR_STORE_ROW_CASE(U16);
        VECTOR_STORE_ROW_CASE(U32);
        VECTOR_STORE_ROW_CASE(U64);
        VECTOR_STORE_ROW_CASE(F32);
        VECTOR_STORE_ROW_CASE(F64);
    default:
        psError(PS_ERR_BAD_PARAMETER_TYPE, true,
                "Specified psImage has invalid type for this function.\n");
        psFree(out);
        return NULL;
    }

    return out;
}


#define VECTOR_STORE_COL_CASE(TYPE) \
case PS_TYPE_##TYPE: \
for (int i = 0; i < input->numRows; i++) { \
    out->data.TYPE[i] = input->data.TYPE[i][column]; \
} \
break;

psVector *psImageCol(psVector *out,
                     const psImage *input,
                     int column)
{
    if (input == NULL || input->data.V == NULL) {
        psError(PS_ERR_BAD_PARAMETER_NULL, true,
                _("Can not operate on a NULL psImage."));
        psFree(out);
        return NULL;
    }
    if (input->col0 < 0) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                "psImage input is invalid.  col0 cannot be negative.\n");
        psFree(out);
        return NULL;
    }
    if (input->row0 < 0) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                "psImage input is invalid.  row0 cannot be negative.\n");
        psFree(out);
        return NULL;
    }
    if (input->numCols < 1) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                "psImage input is invalid.  numCols must be greater than 0.\n");
        psFree(out);
        return NULL;
    }
    if (input->numRows < 1) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                "psImage input is invalid.  numRows must be greater than 0.\n");
        psFree(out);
        return NULL;
    }
    if (column >= (input->numCols + input->col0) ) {
        psError(PS_ERR_BAD_PARAMETER_NULL, true,
                "Specified column number is out of range for specified image.\n");
        psFree(out);
        return NULL;
    } else if ( column < input->col0 && column >= 0 ) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                "Specified column number is out of range for specified image.\n");
        psFree(out);
        return NULL;
    } else if ( column < 0 ) {
        column += input->numCols;
        if ( column < 0 ) {
            psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                    "Specified column number is out of range for specified image.\n");
            psFree(out);
            return NULL;
        }
    } else {
        column -= input->col0;
    }

    out = psVectorRecycle(out, input->numRows, input->type.type);
    out->n = input->numRows;

    switch (input->type.type) {
        VECTOR_STORE_COL_CASE(S8);
        VECTOR_STORE_COL_CASE(S16);
        VECTOR_STORE_COL_CASE(S32);
        VECTOR_STORE_COL_CASE(S64);
        VECTOR_STORE_COL_CASE(U8);
        VECTOR_STORE_COL_CASE(U16);
        VECTOR_STORE_COL_CASE(U32);
        VECTOR_STORE_COL_CASE(U64);
        VECTOR_STORE_COL_CASE(F32);
        VECTOR_STORE_COL_CASE(F64);
    default:
        psError(PS_ERR_BAD_PARAMETER_TYPE, true,
                "Specified psImage has invalid type for this function.\n");
        psFree(out);
        return NULL;
    }

    return out;

}

psVector* psImageSlice(psVector* out,
                       psPixels* coords,
                       const psImage* input,
                       const psImage* mask,
                       psImageMaskType maskVal,
                       psRegion region,
                       psImageCutDirection direction,
                       const psStats* stats)
{
    psStats* myStats;
    psElemType type;
    psS32 inRows;
    psS32 inCols;
    psS32 delta = 1;
    psF64* outData;
    psS32 row0 = region.y0;
    psS32 row1 = region.y1;
    psS32 col0 = region.x0;
    psS32 col1 = region.x1;

    if (input == NULL || input->data.V == NULL) {
        psError(PS_ERR_BAD_PARAMETER_NULL, true,
                _("Can not operate on a NULL psImage."));
        psFree(out);
        return NULL;
    }
    if (input->col0 < 0) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                "psImage input is invalid.  col0 cannot be negative.\n");
        psFree(out);
        return NULL;
    }
    if (input->row0 < 0) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                "psImage input is invalid.  row0 cannot be negative.\n");
        psFree(out);
        return NULL;
    }
    if (input->numCols < 1) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                "psImage input is invalid.  numCols must be greater than 0.\n");
        psFree(out);
        return NULL;
    }
    if (input->numRows < 1) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                "psImage input is invalid.  numRows must be greater than 0.\n");
        psFree(out);
        return NULL;
    }

    //If [0,0,0,0] specified, the whole image is to be included
    if (row0 == 0 && col0 == 0 && row1 == 0 && col1 == 0) {
        row0 = input->row0;
        col0 = input->col0;
        row1 = input->row0 + input->numRows - 1;
        col1 = input->col0 + input->numCols - 1;
    }

    //Make sure x0 of region is inside image.  If so, set col0 to corresponding index number.
    if (col0 >= input->col0 && col0 < (input->col0 + input->numCols) ) {
        col0 -= input->col0;
    } else {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                "Specified psRegion parameter, x0=%f, is out of range [%d,%d].\n",
                region.x0, input->col0, input->col0+input->numCols-1);
        psFree(out);
        return NULL;
    }
    //Make sure y0 of region is inside image.  If so, set row0 to corresponding index number.
    if (row0 >= input->row0 && row0 < (input->row0 + input->numRows) ) {
        row0 -= input->row0;
    } else {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                "Specified psRegion parameter, y0=%f, is out of range [%d,%d].\n",
                region.y0, input->row0, input->row0+input->numRows-1);
        psFree(out);
        return NULL;
    }

    //Make sure x1 of region is valid.  If negative, index from tail (if valid).
    if (col1 < 0) {
        col1 += input->numCols;
        if (col1 < 0) {
            psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                    "Specified psRegion parameter, x1=%f=%d, is out of range [%d,%d].\n",
                    region.x1, col1+input->col0, input->col0, input->col0+input->numCols-1);
            psFree(out);
            return NULL;
        }
    } else if (col1 >= input->col0 && col1 < (input->col0 + input->numCols) ) {
        col1 -= input->col0;
    } else {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                "Specified psRegion parameter, x1=%f=%d, is out of range [%d,%d].\n",
                region.x1, col1, input->col0, input->col0+input->numCols-1);
        psFree(out);
        return NULL;
    }
    //Make sure y1 of region is valid.  If negative, index from tail (if valid).
    if (row1 < 0) {
        row1 += input->numRows;
        if (row1 < 0) {
            psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                    "Specified psRegion parameter, y1=%f=%d, is out of range [%d,%d].\n",
                    region.y1, row1+input->row0, input->row0, input->row0+input->numRows-1);
            psFree(out);
            return NULL;
        }
    } else if (row1 >= input->row0 && row1 < (input->row0 + input->numRows) ) {
        row1 -= input->row0;
    } else {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                "Specified psRegion parameter, y1=%f=%d, is out of range [%d,%d].\n",
                region.y1, row1, input->row0, input->row0+input->numRows-1);
        psFree(out);
        return NULL;
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
        psFree(out);
        return NULL;
    } else if (col0 == col1 && row0 == row1) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                "Invalid psRegion specified.  Region contains only 1 pixel.\n");
        psFree(out);
        return NULL;
    }

    type = input->type.type;
    inRows = input->numRows;
    inCols = input->numCols;

    if (direction == PS_CUT_X_NEG || direction == PS_CUT_Y_NEG) {
        delta = -1;
    }

    if (mask != NULL) {
        if (inRows != mask->numRows || inCols != mask->numCols) {
            psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                    _("Input psImage mask size, %dx%d, does not match psImage input size, %dx%d."),
                    mask->numCols,mask->numRows,
                    inCols, inRows);
            psFree(out);
            return NULL;
        }
        if (mask->type.type != PS_TYPE_IMAGE_MASK) {
            char* typeStr;
            PS_TYPE_NAME(typeStr,mask->type.type);
            psError(PS_ERR_BAD_PARAMETER_TYPE, true,
                    _("Input psImage mask type, %s, is not the supported mask datatype of %s."),
                    typeStr, PS_TYPE_IMAGE_MASK_NAME);
            psFree(out);
            return NULL;
        }
    }

    if (stats == NULL) {
        psError(PS_ERR_BAD_PARAMETER_NULL, true,
                _("Specified statistic can not be NULL."));
        psFree(out);
        return NULL;
    }

    // verify that the stats struct specifies a single stats operation
    psStatsOptions statistic = psStatsSingleOption(stats->options);
    if (statistic == 0) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, false,
                _("Specified statistic option, %d, is not valid.  Must specify one and only one statistic type."),stats->options);
        psFree(out);
        return NULL;
    }
    // since stats input is const, I need to
    // create a 'scratch' stats struct
    myStats = psAlloc(sizeof(psStats));
    *myStats = *stats;

    psS32 numCols = col1-col0;
    psS32 numRows = row1-row0;

    if (direction == PS_CUT_X_POS || direction == PS_CUT_X_NEG) {
        psVector* imgVec = psVectorAlloc(numRows, type);
        psVector* maskVec = NULL;
        psImageMaskType* maskData = NULL;
        psPixelCoord* outPosition = NULL;

        // recycle output to make a proper sized/type output structure
        // n.b. type is double as that is the type given for all stats is
        // psStats.
        out = psVectorRecycle(out, numCols, PS_TYPE_F64);
        out->n = numCols;
        if (coords != NULL) {
            coords = psPixelsRealloc(coords, numCols);
            coords->n = numCols;
            outPosition = coords->data;
        }
        outData = out->data.F64;
        if (delta < 0) {
            outData += numCols - 1;
            if (outPosition != NULL) {
                outPosition += numCols - 1;
            }
        }

        if (mask != NULL) {
            maskVec = psVectorAlloc(numRows, PS_TYPE_VECTOR_MASK);
        }

#define PSIMAGE_CUT_VERTICAL(TYPE)					\
	case PS_TYPE_##TYPE: {						\
            psVectorMaskType* maskVecData = NULL;			\
            for (psS32 c = col0; c < col1; c++) {			\
                ps##TYPE *imgData = input->data.TYPE[row0] + c;		\
                ps##TYPE *imgVecData = imgVec->data.TYPE;		\
                if (maskVec != NULL) {					\
                    maskVecData = maskVec->data.PS_TYPE_VECTOR_MASK_DATA; \
                    maskData = &mask->data.PS_TYPE_IMAGE_MASK_DATA[row0][c]; /* XXX double check this... */ \
                    /** old entry: maskData = (psMaskType* )(mask->data.PS_TYPE_IMAGE_MASK_DATA[row0]) + c; */ \
                }							\
                for (psS32 r = row0; r < row1; r++) {			\
		    *imgVecData = *imgData;				\
                    imgVecData ++;					\
                    imgData += inCols;					\
                    if (maskVecData != NULL) {				\
                        *maskVecData = (*maskData & maskVal);		\
                        maskVecData ++;					\
                        maskData += inCols;				\
                    }							\
                }							\
                if (!psVectorStats(myStats, imgVec, NULL, maskVec, 0xff)) { \
		    psError(PS_ERR_UNKNOWN, false, "failure to measure stats"); \
		    psFree(out);					\
		    out = NULL;						\
		    break;						\
		}							\
                *outData = psStatsGetValue(myStats, statistic);		\
                if (outPosition != NULL) {				\
                    outPosition->x = c;					\
                    outPosition->y = row0;				\
                    outPosition += delta;				\
                }							\
                outData += delta;					\
            }								\
            break;							\
        }

        switch (type) {
            PSIMAGE_CUT_VERTICAL(U8);  // Not a requirement
            PSIMAGE_CUT_VERTICAL(U16);
            PSIMAGE_CUT_VERTICAL(U32); // Not a requirement
            PSIMAGE_CUT_VERTICAL(U64); // Not a requirement
            PSIMAGE_CUT_VERTICAL(S8);
            PSIMAGE_CUT_VERTICAL(S16); // Not a requirement
            PSIMAGE_CUT_VERTICAL(S32); // Not a requirement
            PSIMAGE_CUT_VERTICAL(S64); // Not a requirement
            PSIMAGE_CUT_VERTICAL(F32);
            PSIMAGE_CUT_VERTICAL(F64);
        default: {
                char* typeStr;
                PS_TYPE_NAME(typeStr,type);
                psError(PS_ERR_BAD_PARAMETER_TYPE, true, _("Specified psImage type, %s, is not supported."), typeStr);
                psFree(out);
                out = NULL;
            }
        }
        psFree(imgVec);
        psFree(maskVec);

    } else if (direction == PS_CUT_Y_POS || direction == PS_CUT_Y_NEG) {
        // Cut in Y direction
        // XXX use this if we drop the hackish stuff below: psVector* imgVec = psVectorAlloc(numCols, type);
        psVector* imgVec = NULL;
        psVector* maskVec = NULL;
        psS32 elementSize = PSELEMTYPE_SIZEOF(type);
        psPixelCoord* outPosition = NULL;

        // fill in psVector to fake out the statistics functions.
	// XXX EAM : this seems rather hackish: just use the needed psVectorAlloc (like above)?
        imgVec = psAlloc(sizeof(psVector));
        imgVec->type = input->type;
        P_PSVECTOR_SET_NALLOC(imgVec,numCols);
        imgVec->n = imgVec->nalloc;

        // recycle output to make a proper sized/type output structure
        // n.b. type is double as that is the type given for all stats in
        // psStats.
        out = psVectorRecycle(out, numRows, PS_TYPE_F64);
        out->n = numRows;
        if (coords != NULL) {
            coords = psPixelsRealloc(coords, numRows);
            coords->n = numRows;
            outPosition = coords->data;
        }
        outData = out->data.F64;
        if (delta < 0) {
            outData += numRows-1;
            if (outPosition != NULL) {
                outPosition += numRows-1;
            }
        }

        if (mask != NULL) {
            maskVec = psVectorAlloc(numRows, PS_TYPE_VECTOR_MASK);
	    // XXX the old code (below) faked out the mask vector
            // maskVec = psAlloc(sizeof(psVector));
            // maskVec->type = mask->type;
            // P_PSVECTOR_SET_NALLOC(maskVec,numCols);
	    // maskVec->n = maskVec->nalloc;
        }

        for (psS32 r = row0; r < row1; r++) {
            // point the vector struct to the
            // data to calculate the stats
            imgVec->data.U8 = (psPtr )(input->data.U8[r] + col0 * elementSize);

	    // set the vector mask pixels based on the image pixels
            if (maskVec != NULL) {
		psVectorMaskType *maskVecData = maskVec->data.PS_TYPE_VECTOR_MASK_DATA;
		psImageMaskType *maskData = &mask->data.PS_TYPE_IMAGE_MASK_DATA[r][col0];
                for (psS32 c = col0; c < col1; c++) { 
		    *maskVecData = (*maskData & maskVal); 
		    maskVecData ++; 
		    maskData ++;
                } 
	    }

            if (!psVectorStats(myStats, imgVec, NULL, maskVec, 0xff)) {
		psError(PS_ERR_UNKNOWN, false, "failure to measure stats");
		psFree (out);
		out = NULL;
		break;
	    }
            *outData = psStatsGetValue(myStats, statistic);
            if (outPosition != NULL) {
                outPosition->y = r;
                outPosition->x = col0;
                outPosition += delta;

            }
            outData += delta;
        }
        psFree(imgVec);
        psFree(maskVec);
    } else { // don't know what the direction flag is
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                _("Specified slice direction, %d, is invalid."),
                direction);
        psFree(out);
        out = NULL;
    }

    psFree(myStats);

    return out;
}

psVector* psImageCut(psVector* out,
                     psVector* cutCols,
                     psVector* cutRows,
                     const psImage* input,
                     const psImage* mask,
                     psImageMaskType maskVal,
                     psRegion region,
                     unsigned int nSamples,
                     psImageInterpolateMode mode)
{

    if (input == NULL || input->data.V == NULL) {
        psError(PS_ERR_BAD_PARAMETER_NULL, true,
                _("Can not operate on a NULL psImage."));
        psFree(out);
        return NULL;
    }
    if (input->col0 < 0) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                "psImage input is invalid.  col0 cannot be negative.\n");
        psFree(out);
        return NULL;
    }
    if (input->row0 < 0) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                "psImage input is invalid.  row0 cannot be negative.\n");
        psFree(out);
        return NULL;
    }
    if (input->numCols < 1) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                "psImage input is invalid.  numCols must be greater than 0.\n");
        psFree(out);
        return NULL;
    }
    if (input->numRows < 1) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                "psImage input is invalid.  numRows must be greater than 0.\n");
        psFree(out);
        return NULL;
    }
    psS32 numCols = input->numCols;
    psS32 numRows = input->numRows;

    if (nSamples < 2) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                _("Specified number of samples, %d, must be greater than 1 to make a line."),
                nSamples);
        psFree(out);
        return NULL;
    }

    float col0 = region.x0;
    float row0 = region.y0;
    float col1 = region.x1;
    float row1 = region.y1;

    //If [0,0,0,0] specified, the whole image is to be included
    if (row0 == 0 && col0 == 0 && row1 == 0 && col1 == 0) {
        row0 = input->row0;
        col0 = input->col0;
        row1 = input->row0 + input->numRows - 1;
        col1 = input->col0 + input->numCols - 1;
    }

    //Make sure x0 of region is inside image.  If so, set col0 to corresponding index number.
    if (col0 >= input->col0 && col0 < (input->col0 + input->numCols) ) {
        col0 -= input->col0;
    } else {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                "Specified psRegion parameter, x0=%f, is out of range [%d,%d].\n",
                region.x0, input->col0, input->col0+input->numCols);
        psFree(out);
        return NULL;
    }
    //Make sure y0 of region is inside image.  If so, set row0 to corresponding index number.
    if (row0 >= input->row0 && row0 < (input->row0 + input->numRows) ) {
        row0 -= input->row0;
    } else {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                "Specified psRegion parameter, y0=%f, is out of range [%d,%d].\n",
                region.y0, input->row0, input->row0+input->numRows);
        psFree(out);
        return NULL;
    }
    if (col1 < 0 || row1 < 0 || col0 < 0 || row0 < 0 || col0 >= numCols || col1 >= numCols ||
            row0 >= numRows || row1 >= numRows) {
        psFree(out);
        return NULL;
    }
    float startCol = col0;
    float startRow = row0;
    float endCol = col1;
    float endRow = row1;

    if (mode < PS_INTERPOLATE_FLAT ) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                _("Specified interpolation mode, %d, is unsupported."),
                mode);
        psFree(out);
        return NULL;
    }

    if (mask != NULL) {
        if (numRows != mask->numRows || numCols != mask->numCols) {
            psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                    _("Input psImage mask size, %dx%d, does not match psImage input size, %dx%d."),
                    mask->numCols,mask->numRows,
                    numCols-1, numRows);
            psFree(out);
            return NULL;
        }
        if (mask->type.type != PS_TYPE_IMAGE_MASK) {
            char* typeStr;
            PS_TYPE_NAME(typeStr,mask->type.type);
            psError(PS_ERR_BAD_PARAMETER_TYPE, true,
                    _("Input psImage mask type, %s, is not the supported mask datatype of %s."),
                    typeStr, PS_TYPE_IMAGE_MASK_NAME);
            psFree(out);
            return NULL;
        }
    }

    //resize the vectors for the coordinate output
    psF32* cutColsData = NULL;
    psF32* cutRowsData = NULL;
    if (cutCols != NULL) {
        cutCols = psVectorRecycle(cutCols, nSamples, PS_TYPE_F32);
        cutColsData = cutCols->data.F32;
    }
    if (cutRows != NULL) {
        cutRows = psVectorRecycle(cutRows, nSamples, PS_TYPE_F32);
        cutRowsData = cutRows->data.F32;
    }

    out = psVectorRecycle(out, nSamples, input->type.type);

    float dX = (endCol - startCol) / (float)(nSamples-1);
    float dY = (endRow - startRow) / (float)(nSamples-1);

    psImageInterpolation *interp = psImageInterpolationAlloc(mode, input, NULL, mask, maskVal,
                                                             0, 0, 0, 0, 0, 0);

    #define LINEAR_CUT_CASE(TYPE) \
case PS_TYPE_##TYPE: { \
        ps##TYPE* outData = out->data.TYPE; \
        for (psS32 i = 0; i < nSamples; i++) { \
            float x = startCol + (float)i*dX; \
            float y = startRow + (float)i*dY; \
            /* store off the location of the sample. */ \
            if (cutColsData != NULL) { \
                cutColsData[i] = x; \
            } \
            if (cutRowsData != NULL) { \
                cutRowsData[i] = y; \
            } \
            double value; \
            if (!psImageInterpolate(&value, NULL, NULL, x, y, interp)) { \
                psError(PS_ERR_UNKNOWN, false, "Unable to interpolate image."); \
                psFree(interp); \
                psFree(out); \
                return NULL; \
            } \
            outData[i] = value; \
        } \
    } \
    break;


    switch (input->type.type) {
        LINEAR_CUT_CASE(U8);
        LINEAR_CUT_CASE(U16);
        LINEAR_CUT_CASE(U32);
        LINEAR_CUT_CASE(U64);
        LINEAR_CUT_CASE(S8);
        LINEAR_CUT_CASE(S16);
        LINEAR_CUT_CASE(S32);
        LINEAR_CUT_CASE(S64);
        LINEAR_CUT_CASE(F32);
        LINEAR_CUT_CASE(F64);
      default: {
          char* typeStr;
          PS_TYPE_NAME(typeStr,input->type.type);
          psError(PS_ERR_BAD_PARAMETER_TYPE, true,
                  _("Specified psImage type, %s, is not supported."),
                  typeStr);
          psFree(interp);
          psFree(out);
          out = NULL;
      }
    }

    psFree(interp);

    return out;
}

psVector* psImageRadialCut(psVector* out,
                           const psImage* input,
                           const psImage* mask,
                           psImageMaskType maskVal,
                           float x,
                           float y,
                           const psVector* radii,
                           const psStats* stats)
{
    /* check the parameters */

    if (input == NULL || input->data.V == NULL) {
        psError(PS_ERR_BAD_PARAMETER_NULL, true,
                _("Can not operate on a NULL psImage."));
        psFree(out);
        return NULL;
    }
    psS32 numCols = input->numCols;
    psS32 numRows = input->numRows;

    if (mask != NULL) {
        if (numRows != mask->numRows || numCols != mask->numCols) {
            psError(PS_ERR_BAD_PARAMETER_SIZE, true,
                    _("Input psImage mask size, %dx%d, does not match psImage input size, %dx%d."),
                    mask->numCols,mask->numRows,
                    numCols, numRows);
            psFree(out);
            return NULL;
        }
        if (mask->type.type != PS_TYPE_IMAGE_MASK) {
            char* typeStr;
            PS_TYPE_NAME(typeStr,mask->type.type);
            psError(PS_ERR_BAD_PARAMETER_TYPE, true,
                    _("Input psImage mask type, %s, is not the supported mask datatype of %s."),
                    typeStr, PS_TYPE_IMAGE_MASK_NAME);
            psFree(out);
            return NULL;
        }
    }

    //    if (x < 0 || x >= numCols ||
    //            y < 0 || y >= numRows) {
    if (x < input->col0 || x >= (input->col0 + numCols) ||
            y < input->row0 || y >= (input->row0 + numRows) ) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                _("Specified center, (%g,%g), is outside of the psImage boundaries, [0:%d,0:%d]."),
                x, y,
                numCols-1, numRows-1);
        psFree(out);
        return NULL;
    }

    if (radii == NULL) {
        psError(PS_ERR_BAD_PARAMETER_NULL, true,
                _("Specified radii vector can not be NULL."));
        psFree(out);
        return NULL;
    }

    if (radii->n < 2) {
        psError(PS_ERR_BAD_PARAMETER_SIZE, true,
                _("Input radii vector size, %ld, can not be less than 2."),
                radii->n);
        psFree(out);
        return NULL;
    }

    if (stats == NULL) {
        psError(PS_ERR_BAD_PARAMETER_NULL, true,
                _("Specified statistic can not be NULL."));
        psFree(out);
        return NULL;
    }

    // verify that the stats struct specifies a single stats operation
    psStatsOptions statistic = psStatsSingleOption(stats->options);
    if (statistic == 0) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, false,
                _("Specified statistic option, %d, is not valid.  Must specify one and only one statistic type."),
                stats->options);
        psFree(out);
        return NULL;
    }

    /* completed checking the parameters */

    // size the output vector to proper size.
    psS32 numOut = radii->n - 1;
    out = psVectorRecycle(out, numOut, PS_TYPE_F64);
    psF64* outData = out->data.F64;

    psVector* rSqVec = psVectorCopy(NULL, radii, PS_TYPE_F32);
    psF32* rSq = rSqVec->data.F32;

    psS32 startRow = y - rSq[numOut];
    psS32 endRow = y + rSq[numOut];
    psS32 startCol = x - rSq[numOut];
    psS32 endCol = x + rSq[numOut];

    if (startRow < 0) {
        startRow = 0;
    }

    if (startCol < 0) {
        startCol = 0;
    }

    if (endRow >= numRows) {
        endRow = numRows - 1;
    }

    if (endCol >= numCols) {
        endCol = numCols - 1;
    }

    // Square the radii data
    for (psS32 d = 0; d <= numOut; d++) {
        rSq[d] *= rSq[d];
    }

    // create temporary vectors for the data binning step
    psVector** buffer = psAlloc(sizeof(psVector*)*numOut);
    psVector** bufferMask = psAlloc(sizeof(psVector*)*numOut);
    for (psS32 lcv = 0; lcv < numOut; lcv++) {
        // n.b. alloc enough for the data by making the vectors slightly larger
        // than the area of the region of interest.
        buffer[lcv] = psVectorAllocEmpty(1+4*(rSq[lcv+1]-rSq[lcv]),
                                         input->type.type);

        bufferMask[lcv] = NULL;
        if (mask != NULL) {
            bufferMask[lcv] = psVectorAllocEmpty(1+4*(rSq[lcv+1]-rSq[lcv]),
                                                 PS_TYPE_VECTOR_MASK);
        }
    }

    float dX;
    float dY;
    float dist;
    for (psS32 row=startRow; row <= endRow; row++) {
        psF32* inRow = input->data.F32[row];
        psImageMaskType* maskRow = NULL;
        if (mask != NULL) {
            maskRow = mask->data.PS_TYPE_IMAGE_MASK_DATA[row];
        }
        for (psS32 col=startCol; col <= endCol; col++) {
            dX = x - (float)col - 0.5f;
            dY = y - (float)row - 0.5f;
            dist = dX*dX+dY*dY;
            for (psS32 r = 0; r < numOut; r++) {
                if (rSq[r] < dist && dist < rSq[r+1]) {
                    psS32 n = buffer[r]->n;
                    if (n == buffer[r]->nalloc) { // in case buffers already full, expand
                        buffer[r] = psVectorRealloc(buffer[r], n*2);
                        if (bufferMask[r] != NULL) {
                            bufferMask[r] = psVectorRealloc(bufferMask[r], n*2);
                        }
                    }

                    buffer[r]->data.F32[n] = inRow[col];
                    buffer[r]->n = n+1;

                    if (maskRow != NULL) {
                        bufferMask[r]->data.PS_TYPE_VECTOR_MASK_DATA[n] = (maskRow[col] & maskVal);
                        bufferMask[r]->n = n+1;
                    }

                    break;
                }
            }
        }
    }

    psStats* myStats = psAlloc(sizeof(psStats));
    *myStats = *stats;

    for (psS32 r = 0; r < numOut; r++) {
        if (!psVectorStats(myStats, buffer[r], NULL, bufferMask[r], 0xff)){
	    psError(PS_ERR_UNKNOWN, false, "failure to measure stats");
	    psFree(out);
	    out = NULL;
	    break;
	}
        outData[r] = psStatsGetValue(myStats, statistic);
    }

    psFree(myStats);

    for (psS32 lcv = 0; lcv < numOut; lcv++) {
        psFree(buffer[lcv]);
        psFree(bufferMask[lcv]);
    }
    psFree(buffer);
    psFree(bufferMask);
    psFree(rSqVec);
    return out;
}
