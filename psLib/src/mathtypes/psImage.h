/* @file  psImage.h
 * @brief Basic image definitions and operations
 *
 * This file defines the basic type for an image struct and functions useful
 * in manupulating images.
 *
 * @author Robert DeSonia, MHPCC
 * @author Ross Harman, MHPCC
 * @author Joshua Hoblitt, University of Hawaii
 *
 * @version $Revision: 1.97 $ $Name: not supported by cvs2svn $
 * @date $Date: 2008-08-14 03:18:41 $
 * Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 */

#ifndef PS_IMAGE_H
#define PS_IMAGE_H

/// @addtogroup MathOps Mathematical Operations
/// @{

#include <stdio.h>
#include "psType.h"
#include "psArray.h"
#include "psConstants.h"
#include "psMutex.h"

/** Basic image data structure.
 *
 * Struct for maintaining image data of varying types. It also contains
 * information about image size, parent images and children images.
 *
 */
typedef struct psImage
{
    const psMathType type;             ///< Image data type and dimension.
    const int numCols;                 ///< Number of columns in image
    const int numRows;                 ///< Number of rows in image.
    const int col0;                     ///< Column position relative to parent.
    const int row0;                     ///< Row position relative to parent.

    union {
        psS8**  S8;                    ///< Signed 8-bit integer data.
        psS16** S16;                   ///< Signed 16-bit integer data.
        psS32** S32;                   ///< Signed 32-bit integer data.
        psS64** S64;                   ///< Signed 64-bit integer data.
        psU8**  U8;                    ///< Unsigned 8-bit integer data.
        psU16** U16;                   ///< Unsigned 16-bit integer data.
        psU32** U32;                   ///< Unsigned 32-bit integer data.
        psU64** U64;                   ///< Unsigned 64-bit integer data.
        psF32** F32;                   ///< Single-precision float data.
        psF64** F64;                   ///< Double-precision float data.
        psPtr*  V;                     ///< Pointer to data.
    } data;                            ///< Union for data types.
    const struct psImage* parent;      ///< Parent, if a subimage.
    psPtr p_rawDataBuffer;             ///< Raw data buffer for Allocating/Freeing Images; private
    psArray* children;                 ///< Children of this region.
    psMutex lock;                       ///< Optional lock for thread safety
}
psImage;


#define P_PSIMAGE_SET_NUMCOLS(img,nc) {*(int*)&img->numCols = nc;}
#define P_PSIMAGE_SET_NUMROWS(img,nr) {*(int*)&img->numRows = nr;}
#define P_PSIMAGE_SET_COL0(img,c0) {*(int*)&img->col0 = c0;}
#define P_PSIMAGE_SET_ROW0(img,r0) {*(int*)&img->row0 = r0;}
#define P_PSIMAGE_SET_TYPE(img,t) {*(psMathType*)&img->type = t;}
#define P_PSIMAGE_GET_TYPE(img) ((img)->type->type)

/** Create an image of the specified size and type.
 *
 * Uses psLib memory allocation functions to create an image struct of the
 * specified size and type.
 *
 * @return psImage* : Pointer to psImage.
 *
 */
#ifdef DOXYGEN
psImage* psImageAlloc(
    int numCols,                       ///< Number of columns in image.
    int numRows,                       ///< Number of rows in image.
    psElemType type                    ///< Type of data for image.
);
#else // ifdef DOXYGEN
psImage* p_psImageAlloc(
    const char *file,                   ///< File of caller
    unsigned int lineno,                ///< Line number of caller
    const char *func,                   ///< Function name of caller
    int numCols,                       ///< Number of columns in image.
    int numRows,                       ///< Number of rows in image.
    psElemType type                    ///< Type of data for image.
) PS_ATTR_MALLOC;
#define psImageAlloc(numCols, numRows, type) \
      p_psImageAlloc(__FILE__, __LINE__, __func__, numCols, numRows, type)
#endif // ifdef DOXYGEN


/** Checks the type of a particular pointer.
 *
 *  Uses the appropriate deallocation function in psMemBlock to check the ptr * datatype.
 *  @return bool:       True if the pointer matches a psImage structure, false otherwise.
 */
bool psMemCheckImage(
    psPtr ptr                          ///< the pointer whose type to check
);

/** Initializes the image with the given value.
 *
 *  The input data is cast to match the image datatype.
 *
 *  @return bool:       True on success, otherwise false.
 */
bool psImageInit(
    psImage *image,                    ///< the image to be initialized
    double value                        ///< Value to which to initialise
);


/** Sets the value of the image at the specified x,y position to value.
 *
 *  A negative value for the x or y positions means index from the end.
 *
 *  @return bool:       True on success, otherwise false.
 */
bool psImageSet(
    psImage *image,                     ///< the image to set
    int x,                              ///< x-position
    int y,                              ///< y-position
    double value                        ///< specified value to set
);

/** Returns the value of the image at the specified x,y position.
 *
 *  A negative value for the x or y positions means index from the end.
 *
 *  @return double: The value at the specified x,y position.
 */
double psImageGet(
    const psImage *image,              ///< the image from which to get
    int x,                             ///< x-position
    int y                              ///< y-position
);

/** Resize a given image to the given size/type.
 *
 *  @return psImage* Resized psImage.
 */
#ifdef DOXYGEN
psImage* psImageRecycle(
    psImage* old,                      ///< the psImage to recycle by resizing image buffer
    int numCols,                       ///< the desired number of columns in image
    int numRows,                       ///< the desired number of rows in image
    const psElemType type              ///< the desired datatype of the image
);
#else // ifdef DOXYGEN
psImage* p_psImageRecycle(
    const char *file,                   ///< File of caller
    unsigned int lineno,                ///< Line number of caller
    const char *func,                   ///< Function name of caller
    psImage* old,                      ///< the psImage to recycle by resizing image buffer
    int numCols,                       ///< the desired number of columns in image
    int numRows,                       ///< the desired number of rows in image
    const psElemType type              ///< the desired datatype of the image
);
#define psImageRecycle(old, numCols, numRows, type) \
      p_psImageRecycle(__FILE__, __LINE__, __func__, old, numCols, numRows, type)
#endif // ifdef DOXYGEN


/** Copy an image to a new buffer
 *
 *  @return True if image copied or false if error
 */
bool p_psImageCopyToRawBuffer(
    void* buffer,                      ///< the buffer used to copy the image
    const psImage* input,              ///< the input image to be copied
    psElemType type                    ///< the datatype of the image to be copied
);


/** Frees all children of a psImage.
 *
 *  @return int      Number of children freed.
 */
int psImageFreeChildren(
    psImage* image                     ///< psImage in which all children shall be deallocated
);


/** get an element of an image as a psF64.
 *
 *  @return psF64   pixel value at specified location
 */
psF64 p_psImageGetElementF64(
    psImage* image,                    ///< input image
    int col,                           ///< pixel column
    int row                            ///< pixel row
);


/** print image pixel values.
 *
 *  @return bool    TRUE is successful, otherwise FALSE.
 */
bool p_psImagePrint(
    int fd,                            ///< Destination file descriptor
    psImage *a,                        ///< image to print
    char *name                         ///< name of the image (for title)
);




/*****************************************************************************
    PS_IMAGE macros:
*****************************************************************************/
#define PS_ASSERT_IMAGE_NON_NULL(NAME, RVAL) PS_ASSERT_GENERAL_IMAGE_NON_NULL(NAME, return RVAL)
#define PS_ASSERT_GENERAL_IMAGE_NON_NULL(NAME, CLEANUP) \
if ((NAME) == NULL || (NAME)->data.V == NULL) { \
    psError(PS_ERR_BAD_PARAMETER_NULL, true, \
            "Unallowable operation: psImage %s or its data is NULL.", \
            #NAME); \
    CLEANUP; \
}

#define PS_ASSERT_IMAGE_NON_EMPTY(NAME, RVAL) PS_ASSERT_GENERAL_IMAGE_NON_EMPTY(NAME, return RVAL)
#define PS_ASSERT_GENERAL_IMAGE_NON_EMPTY(NAME, CLEANUP) \
if ((NAME)->numCols < 1 || (NAME)->numRows < 1) { \
    psError(PS_ERR_BAD_PARAMETER_SIZE, true, \
            "Unallowable operation: psImage %s has zero rows or columns (%dx%d).", \
            #NAME, (NAME)->numCols, (NAME)->numRows); \
    CLEANUP; \
}

#define PS_ASSERT_IMAGE_TYPE(NAME, TYPE, RVAL) \
if ((NAME)->type.type != TYPE) { \
    char *imageType, *desiredType; \
    PS_TYPE_NAME(imageType, (NAME)->type.type); \
    PS_TYPE_NAME(desiredType, TYPE); \
    psError(PS_ERR_BAD_PARAMETER_TYPE, true, \
            "Unallowable operation: psImage %s has incorrect type: %s instead of %s.", \
            #NAME, imageType, desiredType); \
    return(RVAL); \
}

#define PS_ASSERT_IMAGE_TYPE_F32_OR_F64(NAME, RVAL) \
if ((NAME)->type.type != PS_TYPE_F32 && (NAME)->type.type != PS_TYPE_F64) { \
    char *imageType; \
    PS_TYPE_NAME(imageType, (NAME)->type.type); \
    psError(PS_ERR_BAD_PARAMETER_TYPE, true, \
            "Unallowable operation: psImage %s is not of type F32 or F64: %s.", \
            #NAME, imageType); \
    return(RVAL); \
}

#define PS_ASSERT_IMAGES_SIZE_EQUAL(NAME1, NAME2, RVAL) \
if (((NAME1)->numCols != (NAME2)->numCols) || \
        ((NAME1)->numRows != (NAME2)->numRows)) { \
    psError(PS_ERR_BAD_PARAMETER_SIZE, true, \
            "Unallowable operation: psImages %s and %s are not the same size: %dx%d vs %dx%d.", \
            #NAME1, #NAME2, (NAME1)->numCols, (NAME1)->numRows, (NAME2)->numCols, (NAME2)->numRows); \
    return(RVAL); \
}

#define PS_ASSERT_IMAGE_SIZE(NAME1, NUM_COLS, NUM_ROWS, RVAL) \
if (((NAME1)->numCols != NUM_COLS) || \
        ((NAME1)->numRows != NUM_ROWS)) { \
    psError(PS_ERR_BAD_PARAMETER_SIZE, true, \
            "Unallowable operation: psImages %s is not the correct size: %dx%d instead of %dx%d.", \
            #NAME1, (NAME1)->numCols, (NAME1)->numRows, NUM_COLS, NUM_ROWS); \
    return(RVAL); \
}

#define PS_IMAGE_PRINT_F32(NAME) \
printf("======== printing %s ========\n", #NAME); \
for (int i = 0 ; i < (NAME)->numRows ; i++) { \
    for (int j = 0 ; j < (NAME)->numCols ; j++) { \
        printf("%.2f ", (NAME)->data.F32[i][j]); \
    } \
    printf("\n"); \
}\

#define PS_IMAGE_PRINT_F64(NAME) \
printf("======== printing %s ========\n", #NAME); \
for (int i = 0 ; i < (NAME)->numRows ; i++) { \
    for (int j = 0 ; j < (NAME)->numCols ; j++) { \
        printf("%.2f ", (NAME)->data.F64[i][j]); \
    } \
    printf("\n"); \
}\

/// @}
#endif // PS_IMAGE_H
