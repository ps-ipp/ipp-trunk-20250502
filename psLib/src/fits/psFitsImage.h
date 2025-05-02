/* @file  psFitsImage.h
 * @brief Contains Fits I/O routines
 *
 * @author Robert DeSonia, MHPCC
 *
 * @version $Revision: 1.11 $ $Name: not supported by cvs2svn $
 * @date $Date: 2009-01-27 06:39:37 $
 * Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 */

#ifndef PS_FITSIMAGE_H
#define PS_FITSIMAGE_H

/// @addtogroup FileIO Input/Output
/// @{

#include "psFits.h"
#include "psType.h"
#include "psArray.h"
#include "psVector.h"
#include "psMetadata.h"
#include "psImage.h"

/// Return the dimensions and type of the FITS image
bool psFitsImageSize(
    int *numCols, int *numRows,         ///< Size of image
    psElemType *type,                   ///< Type of image
    const psFits *fits,                 ///< FITS file pointer
    psRegion region                     ///< Region in the FITS image to read
    );

/** Reads an image, given the desired region and z-plane.
 *
 *  @return psImage*     the read image or NULL if there was an error.
 */
psImage *psFitsReadImage(
    const psFits *fits,                 ///< the psFits object
    psRegion region,                    ///< the region in the FITS image to read
    int z                               ///< the z-plane in the FITS image cube to read
);

/** Read an image into an extant buffer
 */
psImage *psFitsReadImageBuffer(
    psImage *output,                    ///< Output image buffer
    const psFits *fits,                 ///< the psFits object
    psRegion region,                    ///< the region in the FITS image to read
    int z                               ///< the z-plane in the FITS image cube to read
    );

/** Writes an image to a FITS file
 *
 * A new IMAGE HDU is appended to the end of the FITS file.
 *
 *  @return bool        TRUE is the write was successful, otherwise FALSE.
 */
bool psFitsWriteImage(
    psFits *fits,                       ///< the psFits object
    psMetadata *header,                 ///< header items for the new HDU.  Can be NULL.
    const psImage *input,               ///< the image to output
    int depth,                          ///< the number of z-planes of the FITS image data cube
    const char *extname                 ///< FITS extension name
);

/** Writes an image to a FITS file, optionally using the supplied mask image to do statistics when compressing
 *
 * A new IMAGE HDU is appended to the end of the FITS file.
 *
 *  @return bool        TRUE is the write was successful, otherwise FALSE.
 */
bool psFitsWriteImageWithMask(
    psFits *fits,                       ///< the psFits object
    psMetadata *header,                 ///< header items for the new HDU.  Can be NULL.
    const psImage *input,               ///< the image to output
    const psImage *mask,                ///< the mask image
    psImageMaskType maskVal,		///< value to mask
    int depth,                          ///< the number of z-planes of the FITS image data cube
    const char *extname                 ///< FITS extension name
);

/** Insert an image in a FITS file
 *
 *  @return bool        TRUE is the write was successful, otherwise FALSE.
 */
bool psFitsInsertImage(
    psFits *fits,                       ///< the psFits object
    psMetadata *header,                 ///< header items for the new HDU.  Can be NULL.
    const psImage *input,               ///< the image to output
    int depth,                          ///< the number of z-planes of the FITS image data cube
    const char *extname,                ///< FITS extension name
    bool after                          ///< if TRUE, inserts HDU after current HDU, otherwise before
);

/** Insert an image in a FITS file, optionally using the supplied mask image to do statistics when compressing
 *
 *  @return bool        TRUE is the write was successful, otherwise FALSE.
 */
bool psFitsInsertImageWithMask(
    psFits *fits,                       ///< the psFits object
    psMetadata *header,                 ///< header items for the new HDU.  Can be NULL.
    const psImage *input,               ///< the image to output
    const psImage *mask,                ///< the mask image
    psImageMaskType maskVal,		///< value to mask
    int depth,                          ///< the number of z-planes of the FITS image data cube
    const char *extname,                ///< FITS extension name
    bool after                          ///< if TRUE, inserts HDU after current HDU, otherwise before
);

/** Updates an existing FITS file image
 *
 *  @return bool        TRUE is the write was successful, otherwise FALSE.
 */
bool psFitsUpdateImage(
    psFits *fits,                       ///< the psFits object
    const psImage *input,               ///< the image to output
    int x0,                             ///< psImage's x-axis origin in FITS image coordinates
    int y0,                             ///< psImage's y-axis origin in FITS image coordinates
    int z                               ///< the z-planes of the FITS image data cube to write
);

/** Updates an existing FITS file image, optionally using the supplied mask image to do statistics when
 ** compressing
 *
 *  @return bool        TRUE is the write was successful, otherwise FALSE.
 */
bool psFitsUpdateImageWithMask(
    psFits *fits,                       ///< the psFits object
    const psImage *input,               ///< the image to output
    const psImage *mask,                ///< the mask image
    psImageMaskType maskVal,		///< value to mask
    int x0,                             ///< psImage's x-axis origin in FITS image coordinates
    int y0,                             ///< psImage's y-axis origin in FITS image coordinates
    int z                               ///< the z-planes of the FITS image data cube to write
);

/// Read an image cube (3D image with each plane a separate image)
///
/// Images are returned in an array of psImage
psArray *psFitsReadImageCube(
    const psFits *fits,                 ///< FITS file to read
    psRegion region                     ///< Region to read
    );

/// Write an image cube (3D image from an array of images)
bool psFitsWriteImageCube(
    psFits *fits,                       ///< FITS file to write
    psMetadata *header,                 ///< Header to write
    const psArray *input,               ///< Array of images
    const char *extname                 ///< Name of extension
    );

/// Write an image cube (3D image from an array of images), optionally using the supplied mask images to do
/// statistics when compressing
bool psFitsWriteImageCubeWithMask(
    psFits *fits,                       ///< FITS file to write
    psMetadata *header,                 ///< Header to write
    const psArray *input,               ///< Array of images
    const psArray *masks,               ///< Array of masks
    psImageMaskType maskVal,		///< Value to mask
    const char *extname                 ///< Name of extension
    );

/// Update an image cube (3D image from an array of images)
bool psFitsUpdateImageCube(
    psFits *fits,                       ///< FITS file to update
    const psArray *input,               ///< Array of images
    int x0,                             ///< x origin of images in FITS image coordinates
    int y0                              ///< y origin of images in FITS image coordinates
    );

/// Update an image cube (3D image from an array of images), optionally using the supplied mask images to do
/// statistics when compressing
bool psFitsUpdateImageCubeWithMask(
    psFits *fits,                       ///< FITS file to update
    const psArray *input,               ///< Array of images
    const psArray *masks,               ///< Array of masks
    psImageMaskType maskVal,		///< Value to mask
    int x0,                             ///< x origin of images in FITS image coordinates
    int y0                              ///< y origin of images in FITS image coordinates
    );

/// @}
#endif // #ifndef PS_FITS_H
