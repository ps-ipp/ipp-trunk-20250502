/** @file  psImage.c
 *
 *  @brief Contains basic image definitions and operations.
 *
 *  This file defines the basic type for an image struct and functions useful
 *  in manupulating images.
 *
 *  @author Robert DeSonia, MHPCC
 *  @author Ross Harman, MHPCC
 *
 *  @version $Revision: 1.133 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-06 01:05:58 $
 *
 *  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 *
 *  That is the routine used to generate matrices.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <string.h>
#include <math.h>
#include <unistd.h>
#include <errno.h>

#include "psMemory.h"
#include "psError.h"
#include "psAssert.h"
#include "psAbort.h"
#include "psTrace.h"

#include "psImage.h"
#include "psString.h"


static void imageFree(psImage* image)
{
    if (image == NULL) {
        return;
    }

    psImage *parent = (psImage *) image->parent;

    // if I am a child, remove me from my parent's array of children
    if (parent != NULL) {
        // sanity check : a child cannot also be a parent
        if ((image->children != NULL) && (image->children->n > 0)) {
            psAbort ("psImage cannot be both child and parent!");
        }

        // break the back-pointer first so we don't loop
        image->parent = NULL;

        // drop my entry on my parent's array of children
        // lock parent before freeing child : psArrayRemoveDataNoFree is NOT thread safe
        psMutexLock (parent);
        psArrayRemoveDataNoFree (parent->children, image);
        psMutexUnlock (parent);

        // drop my reference to my parent
        psFree (parent);
    }

    // sanity check: this function should never be reached if an image still has live children;
    // they should each be holding a pointer to the image, forcing the number of references to
    // be > 1.
    if (image->children && (image->children->n > 0)) {
        psAbort ("psImage memory management programming error : imageFree called on image with live children");
    }

    psMutexDestroy(image);
    psFree(image->children);
    psFree(image->p_rawDataBuffer);
    psFree(image->data.V);
}

psImage* p_psImageAlloc(const char *file,
                        unsigned int lineno,
                        const char *func,
                        int numCols,
                        int numRows,
                        psElemType type)
{
    int elementSize = PSELEMTYPE_SIZEOF(type);  // element size in bytes
    int rowSize = numCols * elementSize;        // row size in bytes.

    if (numRows < 1 || numCols < 1) {
        psAbort(_("Specified number of rows (%d) or columns (%d) is invalid."), numRows, numCols);
    }

    size_t numBytes = (size_t) numRows * (size_t) numCols * (size_t) elementSize;

    psImage* image = (psImage* ) p_psAlloc(file, lineno, func, sizeof(psImage));

    psMemSetDeallocator(image, (psFreeFunc) imageFree);

    image->data.V = p_psAlloc(file, lineno, func, sizeof(psPtr ) * numRows);

    image->p_rawDataBuffer = p_psAlloc(file, lineno, func, numBytes);

    // set the row pointers.
    image->data.V[0] = image->p_rawDataBuffer;
    for (psS32 i = 1; i < numRows; i++) {
        image->data.V[i] = (psPtr )((int8_t *) image->data.V[i - 1] + rowSize);
    }

    P_PSIMAGE_SET_COL0 (image, 0);
    P_PSIMAGE_SET_ROW0 (image, 0);
    P_PSIMAGE_SET_NUMCOLS (image, numCols);
    P_PSIMAGE_SET_NUMROWS (image, numRows);

    psMathType imageType;
    imageType.dimen = PS_DIMEN_IMAGE;
    imageType.type = type;
    P_PSIMAGE_SET_TYPE (image, imageType);

    image->parent = NULL;
    image->children = NULL;

    // XXX for now, we will add a mutex to all images.  in the future, allow images to be
    // threaded or unthreaded independently?
    psMutexInit (image);

    return image;
}


bool psMemCheckImage(psPtr ptr)
{
    PS_ASSERT_PTR(ptr, false);
    return ( psMemGetDeallocator(ptr) == (psFreeFunc)imageFree );
}



// Image initialisation for integer types
#define IMAGEINIT_INTCASE(TYPE) \
    case PS_TYPE_##TYPE: { \
            if (value == 0.0) { \
                size_t numBytes = image->numCols * sizeof(ps##TYPE); \
                for (int y = 0; y < image->numRows; y++) { \
                    memset(image->data.TYPE[y], 0, numBytes); \
                } \
            } else { \
                if (value < (double)PS_MIN_##TYPE || value > (double)PS_MAX_##TYPE) { \
                    psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Error: Value %f out of range for type %s.\n", \
                            value, #TYPE); \
                    return false; \
                } \
                ps##TYPE castValue = (ps##TYPE)value; \
                for (int iy = 0; iy < image->numRows; iy++) { \
                    ps##TYPE *row = image->data.TYPE[iy]; \
                    for (int ix = 0; ix < image->numCols; ix++) { \
                        row[ix] = castValue; \
                    } \
                } \
            } \
            return true; \
        }

// Image initialisation for char-size integer types
#define IMAGEINIT_CHARCASE(TYPE) \
    case PS_TYPE_##TYPE: { \
            if (value < (double)PS_MIN_##TYPE || value > (double)PS_MAX_##TYPE) { \
                psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Error: Value %f out of range for type %s.\n", \
                        value, #TYPE); \
                return false; \
            } \
            size_t numBytes = image->numCols * sizeof(ps##TYPE); \
            ps##TYPE castValue = (ps##TYPE)value; \
            for (int y = 0; y < image->numRows; y++) { \
                memset(image->data.TYPE[y], castValue, numBytes); \
            } \
            return true; \
        }
// Image initialisation for floating point types
#define IMAGEINIT_FLOATCASE(TYPE) \
    case PS_TYPE_##TYPE: { \
            if (value == 0.0) { \
                size_t numBytes = image->numCols * sizeof(ps##TYPE); \
                for (int y = 0; y < image->numRows; y++) { \
                    memset(image->data.TYPE[y], 0, numBytes); \
                } \
            } else { \
                ps##TYPE castValue = (ps##TYPE)value; \
                for (int iy = 0; iy < image->numRows; iy++) { \
                    ps##TYPE *row = image->data.TYPE[iy]; \
                    for (int ix = 0; ix < image->numCols; ix++) { \
                        row[ix] = castValue; \
                    } \
                } \
            } \
            return true; \
        }


bool psImageInit (psImage *image, double value)
{
    PS_ASSERT_PTR(image, false);
    PS_ASSERT_IMAGE_NON_NULL(image, false);

    switch (image->type.type) {
        IMAGEINIT_CHARCASE(U8)
        IMAGEINIT_INTCASE(U16)
        IMAGEINIT_INTCASE(U32)
        IMAGEINIT_CHARCASE(S8)
        IMAGEINIT_INTCASE(S16)
        IMAGEINIT_INTCASE(S32)
        IMAGEINIT_INTCASE(U64)
        IMAGEINIT_INTCASE(S64)
        IMAGEINIT_FLOATCASE(F32)
        IMAGEINIT_FLOATCASE(F64)
    default:
        psError(PS_ERR_BAD_PARAMETER_TYPE, true, "Type %x not supported", image->type.type);
    }
    return (false);
}

bool psImageSet(psImage *image,
                int x,
                int y,
                double value)
{
    PS_ASSERT_IMAGE_NON_NULL(image, false);
    PS_ASSERT_INT_NONNEGATIVE(image->col0, false);
    PS_ASSERT_INT_NONNEGATIVE(image->row0, false);
    PS_ASSERT_INT_POSITIVE(image->numCols, false);
    PS_ASSERT_INT_POSITIVE(image->numRows, false);
    if ( x >= (image->col0 + image->numCols) ) {
        psError(PS_ERR_BAD_PARAMETER_SIZE, true,
                "Invalid x-position %d.  Position out of range (%d-%d)\n",
                x, image->col0, image->numCols+image->col0-1 );
        return false;
    } else if ( y >= (image->row0 + image->numRows) ) {
        psError(PS_ERR_BAD_PARAMETER_SIZE, true,
                "Invalid y-position %d.  Position out of range (%d-%d)\n",
                y, image->row0, image->numRows+image->row0-1 );
        return false;
    } else if (x < image->col0 && x >= 0) {
        psError(PS_ERR_BAD_PARAMETER_SIZE, true,
                "Invalid x-position %d.  Position out of range (%d-%d)\n",
                x, image->col0, image->numCols+image->col0-1 );
        return false;
    } else if (y < image->row0 && y >= 0) {
        psError(PS_ERR_BAD_PARAMETER_SIZE, true,
                "Invalid y-position %d.  Position out of range (%d-%d)\n",
                y, image->row0, image->numRows+image->row0-1 );
        return false;
    } else if (x < 0 || y < 0) {
        if (x < 0) {
            x += image->numCols;
        }
        if (x < 0) {
            psError(PS_ERR_BAD_PARAMETER_SIZE, true,
                    "Invalid x-position %d.  Position out of range (%d-%d)\n",
                    (x+image->col0), image->col0, image->numCols+image->col0-1 );
            return false;
        }
        if (y < 0) {
            y += image->numRows;
        }
        if (y < 0) {
            psError(PS_ERR_BAD_PARAMETER_SIZE, true,
                    "Invalid y-position %d.  Position out of range (%d-%d)\n",
                    (y+image->row0), image->row0, image->numRows+image->row0-1 );
            return false;
        }
    } else {
        x -= image->col0;
        y -= image->row0;
    }

    #define IMAGE_SET_CASE(TYPE) \
case PS_TYPE_##TYPE: \
    image->data.TYPE[y][x] = (ps##TYPE)value; \
    break;

    switch (image->type.type) {
        IMAGE_SET_CASE(U8);
        IMAGE_SET_CASE(U16);
        IMAGE_SET_CASE(U32);
        IMAGE_SET_CASE(U64);
        IMAGE_SET_CASE(S8);
        IMAGE_SET_CASE(S16);
        IMAGE_SET_CASE(S32);
        IMAGE_SET_CASE(S64);
        IMAGE_SET_CASE(F32);
        IMAGE_SET_CASE(F64);
    default:
        psError(PS_ERR_BAD_PARAMETER_TYPE, true, "Invalid psImage Data Type\n");
        return false;
    }

    return true;
}

double psImageGet(const psImage *image,
                          int x,
                          int y)
{
    PS_ASSERT_IMAGE_NON_NULL(image, NAN);
    PS_ASSERT_INT_NONNEGATIVE(image->col0, NAN);
    PS_ASSERT_INT_NONNEGATIVE(image->row0, NAN);
    PS_ASSERT_INT_POSITIVE(image->numCols, NAN);
    PS_ASSERT_INT_POSITIVE(image->numRows, NAN);
    if ( x >= (image->col0 + image->numCols) ) {
        psError(PS_ERR_BAD_PARAMETER_SIZE, true,
                "Invalid x-position %d.  Position out of range (%d-%d)\n",
                x, image->col0, image->numCols+image->col0-1 );
        return NAN;
    } else if ( y >= (image->row0 + image->numRows) ) {
        psError(PS_ERR_BAD_PARAMETER_SIZE, true,
                "Invalid y-position %d.  Position out of range (%d-%d)\n",
                y, image->row0, image->numRows+image->row0-1 );
        return NAN;
    } else if (x < image->col0 && x >= 0) {
        psError(PS_ERR_BAD_PARAMETER_SIZE, true,
                "Invalid x-position %d.  Position out of range (%d-%d)\n",
                x, image->col0, image->numCols+image->col0-1 );
        return NAN;
    } else if (y < image->row0 && y >= 0) {
        psError(PS_ERR_BAD_PARAMETER_SIZE, true,
                "Invalid y-position %d.  Position out of range (%d-%d)\n",
                y, image->row0, image->numRows+image->row0-1 );
        return NAN;
    } else if (x < 0 || y < 0) {
        if (x < 0) {
            x += image->numCols;
        }
        if (x < 0) {
            psError(PS_ERR_BAD_PARAMETER_SIZE, true,
                    "Invalid x-position %d.  Position out of range (%d-%d)\n",
                    (x+image->col0), image->col0, image->numCols+image->col0-1 );
            return NAN;
        }
        if (y < 0) {
            y += image->numRows;
        }
        if (y < 0) {
            psError(PS_ERR_BAD_PARAMETER_SIZE, true,
                    "Invalid y-position %d.  Position out of range (%d-%d)\n",
                    (y+image->row0), image->row0, image->numRows+image->row0-1 );
            return NAN;
        }
    } else {
        x -= image->col0;
        y -= image->row0;
    }

    #define IMAGE_GET_CASE(TYPE) \
case PS_TYPE_##TYPE: \
    return image->data.TYPE[y][x];

    switch (image->type.type) {
        IMAGE_GET_CASE(U8);
        IMAGE_GET_CASE(U16);
        IMAGE_GET_CASE(U32);
        IMAGE_GET_CASE(U64);
        IMAGE_GET_CASE(S8);
        IMAGE_GET_CASE(S16);
        IMAGE_GET_CASE(S32);
        IMAGE_GET_CASE(S64);
        IMAGE_GET_CASE(F32);
        IMAGE_GET_CASE(F64);
    default:
        psError(PS_ERR_BAD_PARAMETER_TYPE, true, "Invalid psImage Data Type\n");
        return NAN;
    }
}

psImage* p_psImageRecycle(const char *file,
                        unsigned int lineno,
                        const char *func,
                        psImage* old,
                        int numCols,
                        int numRows,
                        const psElemType type)
{
    psS32 elementSize = PSELEMTYPE_SIZEOF(type);  // element size in bytes
    psS32 rowSize = numCols * elementSize;        // row size in bytes.

    if (old == NULL) {
        old = p_psImageAlloc(file, lineno, func, numCols, numRows, type);
        return old;
    }

    if (old->type.dimen != PS_DIMEN_IMAGE) {
        psFree(old);
        psError(PS_ERR_BAD_PARAMETER_TYPE, true,
                _("The input psImage must have a PS_DIMEN_IMAGE dimension type."));
        return NULL;
    }

    /* image already the right size/type? */
    if (numCols == old->numCols && numRows == old->numRows &&
            type == old->type.type) {
        return old;
    }
    // Resize the image buffer
    old->p_rawDataBuffer = psRealloc(old->data.V[0],
                                     numCols * numRows * elementSize);
    old->data.V = (psPtr *)psRealloc(old->data.V, numRows * sizeof(psPtr ));

    // recreate the row pointers
    old->data.V[0] = old->p_rawDataBuffer;
    for (psS32 i = 1; i < numRows; i++) {
        old->data.V[i] = (psPtr )((int8_t *) old->data.V[i - 1] + rowSize);
    }

    *(psU32 *)&old->numCols = numCols;
    *(psU32 *)&old->numRows = numRows;
    *(psElemType* ) & old->type.type = type;

    return old;
}

bool p_psImageCopyToRawBuffer(void* buffer,
                              const psImage* input,
                              psElemType type)
{
    psElemType inDatatype;
    psS32 numRows;
    psS32 numCols;

    if (input == NULL || input->data.V == NULL) {
        psError(PS_ERR_BAD_PARAMETER_NULL, true,
                _("Can not operate on a NULL psImage."));
        return false;
    }

    if (input->type.dimen != PS_DIMEN_IMAGE) {
        psError(PS_ERR_BAD_PARAMETER_TYPE, true,
                _("The input psImage must have a PS_DIMEN_IMAGE dimension type."));
        return false;
    }

    inDatatype = input->type.type;
    numRows = input->numRows;
    numCols = input->numCols;

    // cover the trival case of copy of the same
    // datatype.
    if (type == inDatatype) {
        int rowSize = PSELEMTYPE_SIZEOF(inDatatype)*numCols;
        for (psS32 row=0;row<numRows;row++) {
            memcpy(&((psS8*)buffer)[row*rowSize], input->data.V[row], rowSize);
        }
        return true;
    }

    #define PSIMAGE_BUFFER_COPY(INTYPE,OUTTYPE) { \
        ps##INTYPE *in; \
        ps##OUTTYPE *out = buffer; \
        for(psS32 row=0;row<numRows;row++) { \
            in = input->data.INTYPE[row]; \
            for (psS32 col=0;col<numCols;col++) { \
                *(out++) = *(in++); \
            } \
        } \
    }

    #define PSIMAGE_BUFFER_COPY_CASE(OUT,OUTTYPE) { \
        switch (inDatatype) { \
        case PS_TYPE_S8: \
            PSIMAGE_BUFFER_COPY(S8,OUTTYPE); \
            break; \
        case PS_TYPE_S16: \
            PSIMAGE_BUFFER_COPY(S16,OUTTYPE); \
            break; \
        case PS_TYPE_S32: \
            PSIMAGE_BUFFER_COPY(S32,OUTTYPE); \
            break; \
        case PS_TYPE_S64: \
            PSIMAGE_BUFFER_COPY(S64,OUTTYPE); \
            break; \
        case PS_TYPE_U8: \
            PSIMAGE_BUFFER_COPY(U8,OUTTYPE); \
            break; \
        case PS_TYPE_U16: \
            PSIMAGE_BUFFER_COPY(U16,OUTTYPE); \
            break; \
        case PS_TYPE_U32: \
            PSIMAGE_BUFFER_COPY(U32,OUTTYPE); \
            break; \
        case PS_TYPE_U64: \
            PSIMAGE_BUFFER_COPY(U64,OUTTYPE); \
            break; \
        case PS_TYPE_F32: \
            PSIMAGE_BUFFER_COPY(F32,OUTTYPE); \
            break; \
        case PS_TYPE_F64: \
            PSIMAGE_BUFFER_COPY(F64,OUTTYPE); \
            break; \
        default: \
            break; \
        } \
    }

    switch (type) {
    case PS_TYPE_S8:
        PSIMAGE_BUFFER_COPY_CASE(output, S8);
        break;
    case PS_TYPE_S16:
        PSIMAGE_BUFFER_COPY_CASE(output, S16);
        break;
    case PS_TYPE_S32:
        PSIMAGE_BUFFER_COPY_CASE(output, S32);
        break;
    case PS_TYPE_S64:
        PSIMAGE_BUFFER_COPY_CASE(output, S64);
        break;
    case PS_TYPE_U8:
        PSIMAGE_BUFFER_COPY_CASE(output, U8);
        break;
    case PS_TYPE_U16:
        PSIMAGE_BUFFER_COPY_CASE(output, U16);
        break;
    case PS_TYPE_U32:
        PSIMAGE_BUFFER_COPY_CASE(output, U32);
        break;
    case PS_TYPE_U64:
        PSIMAGE_BUFFER_COPY_CASE(output, U64);
        break;
    case PS_TYPE_F32:
        PSIMAGE_BUFFER_COPY_CASE(output, F32);
        break;
    case PS_TYPE_F64:
        PSIMAGE_BUFFER_COPY_CASE(output, F64);
        break;
    default: {
            char* typeStr;
            PS_TYPE_NAME(typeStr,type);
            psError(PS_ERR_BAD_PARAMETER_TYPE, true,
                    _("Specified psImage type, %s, is not supported."),
                    typeStr);
            break;
        }
    }
    return true;
}

bool p_psImagePrint (int fd,
                     psImage *a,
                     char *name)
{
    if (write(fd,"matrix: ",8)) {;} //ignore return value
    if (write(fd,name,strlen(name))) {;} //ignore return value
    if (write(fd,"\n",1)) {;} //ignore return value

    char buffer[20];
    //    for (int j = 0; j < a[0].numRows; j++) {
    //        for (int i = 0; i < a[0].numCols; i++) {
    for (int j = 0; j < a->numRows; j++) {
        for (int i = 0; i < a->numCols; i++) {
            snprintf (buffer,20, "%f  ", p_psImageGetElementF64(a, i, j));
            if (write(fd,buffer,strlen(buffer))) {;} //ignore return value
        }
        if (write(fd,"\n",1)) {;} //ignore return value
    }
    if (write(fd,"\n",1)) {;} //ignore return value
    return (true);
}


psF64 p_psImageGetElementF64(psImage* image,
                             int col,
                             int row)
{
    if (image == NULL) {
        return NAN;
    }
    if (col < 0 || col >= image->numCols) {
        return NAN;
    }
    if (row < 0 || row >= image->numRows) {
        return NAN;
    }

    switch (image->type.type) {
    case PS_TYPE_U8:
        return image->data.U8[row][col];
        break;
    case PS_TYPE_U16:
        return image->data.U16[row][col];
        break;
    case PS_TYPE_U32:
        return image->data.U32[row][col];
        break;
    case PS_TYPE_U64:
        return image->data.U64[row][col];
        break;
    case PS_TYPE_S8:
        return image->data.S8[row][col];
        break;
    case PS_TYPE_S16:
        return image->data.S16[row][col];
        break;
    case PS_TYPE_S32:
        return image->data.S32[row][col];
        break;
    case PS_TYPE_S64:
        return image->data.S64[row][col];
        break;
    case PS_TYPE_F32:
        return image->data.F32[row][col];
        break;
    case PS_TYPE_F64:
        return image->data.F64[row][col];
    default:
        return NAN;
    }
}
