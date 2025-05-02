/** @file  psImageStructManip.c
 *
 *  @brief Contains basic image structure manipulations, as specified in the
 *         PSLIB SDRS sections "Image Structure Manipulation".
 *
 *  @ingroup Image
 *
 *  @author Robert DeSonia, MHPCC
 *
 *  @version $Revision: 1.21 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-06 01:05:58 $
 *
 *  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 *
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <string.h>
#include <assert.h>
#include <errno.h>

#include "psMemory.h"
#include "psError.h"
#include "psAbort.h"
#include "psTrace.h"

#include "psImageStructManip.h"

// col0,row0 are the starting pixel in the input image coordinate frame
// col1,row1 are the ending pixel in the input image coordinate frame
// note that these are relative to the input col0,row0
// also note that col0,row0 may not be less than input->col0,row0
static psImage* imageSubset(const char *file, // File name of caller
                            unsigned int lineno, // Line number of caller
                            const char *func, // Function name of caller
                            psImage* out,
                            psImage* image,
                            psS32 col0,
                            psS32 row0,
                            psS32 col1,
                            psS32 row1)
{
    psU32 elementSize;          // size of image element in bytes
    psS32 inputColOffset;       // offset in **bytes** to first subset pixel in input row
    psS32 inputRowOffset;       // offset in **rows*** to first input row

    if (image == NULL || image->data.V == NULL) {
        psError(PS_ERR_BAD_PARAMETER_NULL, true,
                _("Can not operate on a NULL psImage."));
        return NULL;
    }

    if ( col0 < image->col0 || row0 < image->row0 ) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                _("Specified subset range, [%d:%d,%d:%d], is invalid or outside input psImage's boundaries, [%d:%d,%d:%d]."),
                col0, col1-1, row0, row1-1, image->col0, image->col0 + image->numCols-1, image->row0,
                image->row0 + image->numRows-1);
        return NULL;
    }

    if (image->type.dimen != PS_DIMEN_IMAGE) {
        psError(PS_ERR_BAD_PARAMETER_TYPE, true,
                _("The input psImage must have a PS_DIMEN_IMAGE dimension type."));
        return NULL;
    }

    if (col1 < 1) {
        col1 = image->col0 + image->numCols + col1;
    }
    if (row1 < 1) {
        row1 = image->row0 + image->numRows + row1;
    }

    if (col1 <= col0 ||
        row1 <= row0 ||
        col0 >= image->col0 + image->numCols ||
        row0 >= image->row0 + image->numRows ||
        col1 > image->col0 + image->numCols ||
        row1 > image->row0 + image->numRows ) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                _("Specified subset range, [%d:%d,%d:%d], is invalid or outside input psImage's boundaries, [%d:%d,%d:%d]."),
                col0, col1-1, row0, row1-1,
                image->col0, image->col0 + image->numCols-1, image->row0, image->row0 + image->numRows-1);
        return NULL;
    }

    psS32 numRows = row1-row0;
    psS32 numCols = col1-col0;

    elementSize = PSELEMTYPE_SIZEOF(image->type.type);

    // if this is a child, we need to start working with parent pixels
    // the subset region (col0,row0 - col1,row1) is in the parent frame
    if (image->parent != NULL) {
        image = (psImage*)image->parent;
    }

    // increment the raw data buffer before freeing anything in the 'out'
    psPtr rawData = psMemIncrRefCounter(image->p_rawDataBuffer);

    if (out != NULL) {
        // if a child, need to orphan (disassociate from parent) first
        psImage *parent = (psImage *) out->parent;
        if (parent != NULL) {
            // break the back-pointer first so we don't loop
            out->parent = NULL;

            // drop my entry on my parent's array of children
	    psMutexLock (out);
            psArrayRemoveDataNoFree (out->parent->children, out);
	    psMutexUnlock (out);

            // drop my reference to my old parent
            psFree (parent);
        }

        // we recycle out->data.V
        psFree(out->p_rawDataBuffer); // free the previous data reference
    } else {
        out = p_psAlloc(file, lineno, func, sizeof(psImage));
        out->data.V = NULL;
	psMutexInit (out);
    }

    out->data.V = p_psRealloc(file, lineno, func, out->data.V,
                              sizeof(psPtr)*numRows); // resize row pointer array
    P_PSIMAGE_SET_TYPE(out, image->type);
    P_PSIMAGE_SET_NUMCOLS(out, numCols);
    P_PSIMAGE_SET_NUMROWS(out, numRows);
    P_PSIMAGE_SET_COL0(out, col0);
    P_PSIMAGE_SET_ROW0(out, row0);

    // As long as I have a valid image reference, no one else can free it to zero (I have at
    // least the last reference)
    out->parent = psMemIncrRefCounter(image); // track references to parents
    out->children = NULL;
    out->p_rawDataBuffer = rawData;

    // set the new psImage's deallocator to the same as the input image
    psMemSetDeallocator(out,psMemGetDeallocator(image));

    inputRowOffset = (row0 - image->row0);
    inputColOffset = (col0 - image->col0)*elementSize;
    assert (inputRowOffset >= 0);
    assert (inputColOffset >= 0);
    for (psS32 row = 0; row < numRows; row++) {
        out->data.V[row] = image->data.U8[row + inputRowOffset] + inputColOffset;
    }

    // Add output image as a child of the input image.  Lock image before performing this
    // operation (psArrayAdd is NOT thread-safe)
    psMutexLock (image);
    image->children = p_psArrayAdd(file, lineno, func, image->children, 16, out);
    psMutexUnlock (image);

    psFree (out); // the image->children array is an array of views only
    return (out);
}

psImage* p_psImageSubset(const char *file, unsigned int lineno, const char *func,
                       psImage* image, psRegion region)
{
    return imageSubset(file, lineno, func, NULL, image, region.x0, region.y0, region.x1, region.y1);
}

psImage* psImageCopyView(psImage *output, psImage *input)
{
    psRegion region = {0, 0, 0, 0};
    region = psRegionForImage (input, region);
    psImage *result = imageSubset(__FILE__, __LINE__, __func__, output, input,
                                  region.x0, region.y0, region.x1, region.y1);
    return result;
}

psImage* p_psImageCopy(const char *file, unsigned int lineno, const char *func,
                       psImage* output, const psImage* input, psElemType type)
{
    psElemType inDatatype;
    psS32 elementSize;
    psS32 elements;
    psS32 numRows;
    psS32 numCols;

    if (input == NULL || input->data.V == NULL) {
        psError(PS_ERR_BAD_PARAMETER_NULL, true,
                _("Can not operate on a NULL psImage."));
        psFree(output);
        return NULL;
    }

    if (input == output) {
        psError(PS_ERR_BAD_PARAMETER_NULL, true,
                _("Specified input and output psImage can not reference the same psImage."));
        psFree(output);
        return NULL;
    }

    if (input->type.dimen != PS_DIMEN_IMAGE) {
        psError(PS_ERR_BAD_PARAMETER_TYPE, true,
                _("The input psImage must have a PS_DIMEN_IMAGE dimension type."));
        psFree(output);
        return NULL;
    }

    inDatatype = input->type.type;
    numRows = input->numRows;
    numCols = input->numCols;
    elements = numRows * numCols;
    elementSize = PSELEMTYPE_SIZEOF(inDatatype);

    if (0) { fprintf (stderr, "%d elements, %d total memory\n", elements, elements * elementSize); }

    output = p_psImageRecycle(file, lineno, func, output, numCols, numRows, type);
    P_PSIMAGE_SET_COL0(output, input->col0);
    P_PSIMAGE_SET_ROW0(output, input->row0);

    // cover the trival case of copy of the same
    // datatype.
    if (type == inDatatype) {
        for (psS32 row=0;row<numRows;row++) {
            memcpy(output->data.V[row], input->data.V[row], elementSize * numCols);
        }
        return output;
    }

    #define PSIMAGE_ELEMENT_COPY(IN,INTYPE,OUT,OUTTYPE,ELEMENTS) { \
        ps##INTYPE *in; \
        ps##OUTTYPE *out; \
        for(psS32 row=0;row<numRows;row++) { \
            in = IN->data.INTYPE[row]; \
            out = OUT->data.OUTTYPE[row]; \
            for (psS32 col=0;col<numCols;col++) { \
                *(out++) = *(in++); \
            } \
        } \
    }

    #define PSIMAGE_COPY_CASE(OUT,OUTTYPE) { \
        switch (inDatatype) { \
        case PS_TYPE_S8: \
            PSIMAGE_ELEMENT_COPY(input,S8,OUT,OUTTYPE,elements); \
            break; \
        case PS_TYPE_S16: \
            PSIMAGE_ELEMENT_COPY(input,S16,OUT,OUTTYPE,elements); \
            break; \
        case PS_TYPE_S32: \
            PSIMAGE_ELEMENT_COPY(input,S32,OUT,OUTTYPE,elements); \
            break; \
        case PS_TYPE_S64: \
            PSIMAGE_ELEMENT_COPY(input,S64,OUT,OUTTYPE,elements); \
            break; \
        case PS_TYPE_U8: \
            PSIMAGE_ELEMENT_COPY(input,U8,OUT,OUTTYPE,elements); \
            break; \
        case PS_TYPE_U16: \
            PSIMAGE_ELEMENT_COPY(input,U16,OUT,OUTTYPE,elements); \
            break; \
        case PS_TYPE_U32: \
            PSIMAGE_ELEMENT_COPY(input,U32,OUT,OUTTYPE,elements); \
            break; \
        case PS_TYPE_U64: \
            PSIMAGE_ELEMENT_COPY(input,U64,OUT,OUTTYPE,elements); \
            break; \
        case PS_TYPE_F32: \
            PSIMAGE_ELEMENT_COPY(input,F32,OUT,OUTTYPE,elements); \
            break; \
        case PS_TYPE_F64: \
            PSIMAGE_ELEMENT_COPY(input,F64,OUT,OUTTYPE,elements); \
            break; \
        default: \
            break; \
        } \
    }

    switch (type) {
    case PS_TYPE_S8:
        PSIMAGE_COPY_CASE(output, S8);
        break;
    case PS_TYPE_S16:
        PSIMAGE_COPY_CASE(output, S16);
        break;
    case PS_TYPE_S32:
        PSIMAGE_COPY_CASE(output, S32);
        break;
    case PS_TYPE_S64:
        PSIMAGE_COPY_CASE(output, S64);
        break;
    case PS_TYPE_U8:
        PSIMAGE_COPY_CASE(output, U8);
        break;
    case PS_TYPE_U16:
        PSIMAGE_COPY_CASE(output, U16);
        break;
    case PS_TYPE_U32:
        PSIMAGE_COPY_CASE(output, U32);
        break;
    case PS_TYPE_U64:
        PSIMAGE_COPY_CASE(output, U64);
        break;
    case PS_TYPE_F32:
        PSIMAGE_COPY_CASE(output, F32);
        break;
    case PS_TYPE_F64:
        PSIMAGE_COPY_CASE(output, F64);
        break;
    default: {
            char* typeStr;
            PS_TYPE_NAME(typeStr,type);
            psError(PS_ERR_BAD_PARAMETER_TYPE, true,
                    _("Specified psImage type, %s, is not supported."),
                    typeStr);
            psFree(output);

            break;
        }
    }
    return output;
}

psImage* psImageTrim(psImage* image,
                     psRegion region)
{
    if (image == NULL || image->data.V == NULL) {
        psError(PS_ERR_BAD_PARAMETER_NULL, true,
                _("Can not operate on a NULL psImage."));
        return NULL;
    }

    if ((image->children != NULL) && (image->children->n > 0)) {
        psAbort ("cannot trim an image with outstanding children");
    }

    if (image->parent != NULL) {
        return imageSubset(__FILE__, __LINE__, __func__, image, (psImage*)image->parent,
                           region.x0+image->col0, region.y0+image->row0,
                           region.x1+image->col0, region.y1+image->row0);
    }

    int col0 = region.x0;
    int row0 = region.y0;
    int col1 = region.x1;
    int row1 = region.y1;

    if (col1 < 1) {
        col1 += image->numCols;
    }

    if (row1 < 1) {
        row1 += image->numRows;
    }

    if (    col0 < 0 ||
            row0 < 0 ||
            col1 > image->numCols ||
            row1 > image->numRows ||
            col0 >= col1 ||
            row0 >= row1 ) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                _("Specified subset range, [%d:%d,%d:%d], is invalid or outside input psImage's boundaries, [0:%d,0:%d]."),
                col0, col1-1, row0, row1-1,
                image->numCols-1, image->numRows-1);
        psFree(image);
        return NULL;
    }

    psU32 elementSize = PSELEMTYPE_SIZEOF(image->type.type);
    psU32 numCols = col1-col0;
    psU32 numRows = row1-row0;
    psU32 rowSize = elementSize*numCols;
    psU32 colOffset = elementSize * col0;
    psU8* imageData = image->p_rawDataBuffer;
    for (psS32 row = row0; row < row1; row++) {
        memmove(imageData,image->data.U8[row] + colOffset,rowSize);
        imageData += rowSize;
    }

    P_PSIMAGE_SET_NUMCOLS(image, numCols);
    P_PSIMAGE_SET_NUMROWS(image, numRows);

    // resize the buffers to the new image size
    image->data.V = psRealloc(image->data.V,sizeof(psPtr)*numRows);
    image->p_rawDataBuffer = psRealloc(image->p_rawDataBuffer,rowSize*numRows);

    image->data.V[0] = image->p_rawDataBuffer;
    for (psS32 r = 1; r < numRows; r++) {
        image->data.U8[r] = image->data.U8[r-1] + rowSize;
    }

    return (image);
}

