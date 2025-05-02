/** @file  psFitsImage.c
 *
 *  @brief Contains Fits I/O routines
 *
 *  @ingroup FileIO
 *
 *  @author Robert DeSonia, MHPCC
 *
 *  @version $Revision: 1.42 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-19 03:15:40 $
 *
 *  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <unistd.h>
#include <assert.h>
#include <string.h>

#include "psAbort.h"
#include "psType.h"
#include "psAssert.h"
#include "psError.h"
#include "psString.h"
#include "psLogMsg.h"
#include "psTrace.h"
#include "psRandom.h"
#include "psImage.h"
#include "psImageStructManip.h"

#include "psFits.h"
#include "psFitsFloat.h"
#include "psFitsFloatFile.h"
#include "psFitsHeader.h"
#include "psFitsScale.h"

#include "psMemory.h"

#include "psFitsImage.h"

#define MAX_STRING_LENGTH 256           // maximum length string for FITS routines

// Information required to read a FITS file
typedef struct {
    int nAxis;                          // Number of axes
    int bitPix;                         // Bits per pixel
    long nAxes[3];                      // Length of each axis
    long firstPixel[3];                 // lower-left corner of image subset
    long lastPixel[3];                  // upper-right corner of image subset
    long increment[3];                  // increment for image subset
    int fitsDatatype;                   // cfitsio data type
    int psDatatype;                     // psLib data type
    bool is_logscaled;                  // is this image log scaled using BOFFSET?
    bool is_asinh;                      // is this image asinh scaled using BSOFTEN?
} p_psFitsReadInfo;

// Read the vital statistics of this FITS image, in preparation for reading the image
static p_psFitsReadInfo *p_psFitsReadInfoAlloc(
    const psFits *fits, // The FITS file handle
    psRegion region, // Region to read
    int z // z-plane to read in cube
    )
{
    PS_ASSERT_FITS_NON_NULL(fits, NULL);
    PS_ASSERT_INT_NONNEGATIVE(z, NULL);

    p_psFitsReadInfo *info = psAlloc(sizeof(p_psFitsReadInfo));
    memset(info, 0, sizeof(p_psFitsReadInfo));

    int status = 0;                     // CFITSIO status

    // check to see if we even are positioned on an image HDU
    int hduType;                        // Type of HDU
    if (fits_get_hdu_type(fits->fd, &hduType, &status) != 0) {
        psFitsError(status, true, "Could not determine the HDU type.");
        goto bad;
    }
    if (hduType != IMAGE_HDU) {
        psError(PS_ERR_IO, true, _("Current FITS HDU type must be an image."));
        goto bad;
    }

    // Get the data type 'bitPix' from the FITS image
    if (fits_get_img_equivtype(fits->fd, &info->bitPix, &status) != 0) {
        psFitsError(status, true, "Could not determine image data type.");
        goto bad;
    }

    /* Get the dimensions 'nAxis' from the FITS image */
    if (fits_get_img_dim(fits->fd, &info->nAxis, &status) != 0) {
        psFitsError(status, true, "Could not determine image dimensions.");
        goto bad;
    }

    /* Validate the number of axis */
    if (info->nAxis > 3) {
        psError(PS_ERR_IO, true,
                _("Image number of dimensions, %d, is not supported."), info->nAxis);
        goto bad;
    }

    /* Get the Image size from the FITS file */
    if (fits_get_img_size(fits->fd, info->nAxis, info->nAxes, &status) != 0) {
        psFitsError(status, true, "Could not determine image size.");
        goto bad;
    }

    info->nAxes[0] = PS_MAX (info->nAxes[0], 1);
    info->nAxes[1] = PS_MAX (info->nAxes[1], 1);
    info->nAxes[2] = PS_MAX (info->nAxes[2], 1);

    info->firstPixel[0] = region.x0 + 1;
    info->firstPixel[1] = region.y0 + 1;
    info->firstPixel[2] = z + 1;

    if (region.x1 > 0) {
        info->lastPixel[0] = region.x1;
    } else {
        info->lastPixel[0] = info->nAxes[0] + region.x1; // n.b., region.x1 < 0
    }
    if (region.y1 > 0) {
        info->lastPixel[1] = region.y1;
    } else {
        info->lastPixel[1] = info->nAxes[1] + region.y1; // n.b., region.y1 < 0
    }
    info->lastPixel[2] = z + 1;

    info->increment[0] = 1;
    info->increment[1] = 1;
    info->increment[2] = 1;

    // Check scale and zero
    double bscale = 0.0, bzero = 0.0, boffset = NAN, bsoften = NAN;    // Scale and zero point
    if (fits_read_key_dbl(fits->fd, "BSCALE", &bscale, NULL, &status) && status != KEY_NO_EXIST) {
        psFitsError(status, true, "Unable to read header.");
        goto bad;
    }
    status = 0;
    if (fits_read_key_dbl(fits->fd, "BZERO", &bzero, NULL, &status) && status != KEY_NO_EXIST) {
        psFitsError(status, true, "Unable to read header.");
        goto bad;
    }
    status = 0;
    if (fits_read_key_dbl(fits->fd, "BOFFSET", &boffset, NULL, &status) && status != KEY_NO_EXIST) {
        psFitsError(status, true, "Unable to read header.");
        goto bad;
    }
    if (status == KEY_NO_EXIST) {
      info->is_logscaled = false;
    }
    else if (isfinite(boffset)) {
      info->is_logscaled = true;
    }
    else {
      info->is_logscaled = false;
    }
    status = 0;
    if (fits_read_key_dbl(fits->fd, "BSOFTEN", &bsoften, NULL, &status) && status != KEY_NO_EXIST) {
        psFitsError(status, true, "Unable to read header.");
        goto bad;
    }
    if (status == KEY_NO_EXIST) {
      info->is_asinh = false;
    }
    else if (isfinite(bsoften)) {
      info->is_asinh = true;
      info->is_logscaled = false;
    }
    else {
      info->is_asinh = false;
    }

    status = 0;

    if ((bscale != 0.0 && bscale != 1.0) || bzero != (int)bzero) {
        // It's a floating-point image that's been quantised
        // cfitsio will apply the scale and zero point for us if we choose the correct data type
        switch (info->bitPix) {
          case BYTE_IMG:
          case SBYTE_IMG:
          case USHORT_IMG:
          case SHORT_IMG:
          case ULONG_IMG:
          case LONG_IMG:
          case FLOAT_IMG:
            info->psDatatype = PS_TYPE_F32;
            info->fitsDatatype = TFLOAT;
            break;
          case LONGLONG_IMG:
          case DOUBLE_IMG:
            info->psDatatype = PS_TYPE_F64;
            info->fitsDatatype = TDOUBLE;
            break;
          default:
            psError(PS_ERR_IO, true, _("FITS image type, BITPIX=%d, is not supported."), info->bitPix);
            goto bad;
        }
    } else {
        switch (info->bitPix) {
          case BYTE_IMG:
            info->psDatatype = PS_TYPE_U8;
            info->fitsDatatype = TBYTE;
            break;
          case SBYTE_IMG:
            info->psDatatype = PS_TYPE_S8;
            info->fitsDatatype = TSBYTE;
            break;
          case USHORT_IMG:
            info->psDatatype = PS_TYPE_U16;
            info->fitsDatatype = TUSHORT;
            break;
          case SHORT_IMG:
            info->psDatatype = PS_TYPE_S16;
            info->fitsDatatype = TSHORT;
            break;
          case ULONG_IMG:
            info->psDatatype = PS_TYPE_U32;
            info->fitsDatatype = TUINT;
            break;
          case LONG_IMG:
            info->psDatatype = PS_TYPE_S32;
            info->fitsDatatype = TINT;
            break;
          case LONGLONG_IMG:
            info->psDatatype = PS_TYPE_S64;
            info->fitsDatatype = TLONGLONG;
            break;
          case FLOAT_IMG:
            info->psDatatype = PS_TYPE_F32;
            info->fitsDatatype = TFLOAT;
            break;
          case DOUBLE_IMG:
            info->psDatatype = PS_TYPE_F64;
            info->fitsDatatype = TDOUBLE;
            break;
          default:
            psError(PS_ERR_IO, true, _("FITS image type, BITPIX=%d, is not supported."), info->bitPix);
            goto bad;
        }
    }
    return info;

    // Common error cleanup
bad:
    psFree(info);
    return NULL;
}


bool psFitsImageSize(int *numCols, int *numRows, psElemType *type, const psFits *fits, psRegion region)
{
    PS_ASSERT_FITS_NON_NULL(fits, NULL);

    if (psFitsCheckCompressedImagePHU(fits, NULL)) {
        // This is really what we want, not the empty PHU
        psTrace("psLib.fits", 1,
                "This PHU should really be a compressed image --- reading that image instead.");
    }

    p_psFitsReadInfo *info = p_psFitsReadInfoAlloc(fits, region, 0); // How big the region to read is

    if (numCols) {
        *numCols = info->lastPixel[0] - info->firstPixel[0] + 1;
    }
    if (numRows) {
        *numRows = info->lastPixel[1] - info->firstPixel[1] + 1;
    }
    if (type) {
        *type = info->psDatatype;
    }

    psFree(info);

    return true;
}


// Convert an image to the desired BITPIX, i.e., the desired disk representation
static psImage *imageToDiskRepresentation(double *bscale, // Scaling applied
                                          double *bzero, // Zero point applied
					  double *boffset, // Log offset applied
					  double *bsoften, // asinh offset applied
                                          long *blank, // Blank value (integer data)
                                          psFitsFloat *floatType, // Type of custom floating-point
                                          psFits *fits, // FITS file pointer
                                          const psImage *image, // Image to convert
                                          const psImage *mask, // Mask image, or NULL
                                          psImageMaskType maskVal, // Value to mask
                                          psRandom *rng, // Random number generator
                                          bool newScaleZero // Determine a new BSCALE and BZERO?
    )
{
    psAssert(bscale, "impossible");
    psAssert(bzero, "impossible");
    psAssert(boffset, "impossible");
    psAssert(bsoften, "impossible");
    psAssert(floatType, "impossible");
    psAssert(fits, "impossible");
    psAssert(image, "impossible");

    *bscale = 1.0;
    *bzero = 0.0;
    *floatType = PS_FITS_FLOAT_NONE;

    psFitsOptions *options = fits->options; // Options for FITS writing

    // Can't PLIO compress U16,U32,U64 directly, so convert them
    if (psFitsCompressionGetType(fits) == PS_FITS_COMPRESS_PLIO && (!options || options->bitpix == 0)) {
        switch (image->type.type) {
          case PS_TYPE_U16:
            return psImageCopy(NULL, image, PS_TYPE_S32);
          case PS_TYPE_U32:
            return psImageCopy(NULL, image, PS_TYPE_S64);
          case PS_TYPE_U64:
            // Conversion would result in numerical overflow
            psError(PS_ERR_IO, true, "Unable to compress U64 images");
            return NULL;
          default:
            // No action
            ;
        }
    }

    if (!options) {
        return psMemIncrRefCounter((psImage*)image); // Casting away const
    }

    // Custom floating-point
    if (PS_IS_PSELEMTYPE_REAL(image->type.type) && options->conventions.psBitpix &&
        options->floatType != PS_FITS_FLOAT_NONE) {
        *floatType = options->floatType;
        return psFitsFloatImageToDisk(image, options->floatType);
    }

    // Automatically select what we're given
    if (options->bitpix == 0) {
        return psMemIncrRefCounter((psImage*)image); // Casting away const
    }

    // Quantise floating-point images
    if (PS_IS_PSELEMTYPE_REAL(image->type.type) && options->bitpix > 0) {
	if (newScaleZero) {
            // Choose an appropriate BSCALE and BZERO
	    if (!psFitsScaleDetermine(bscale, bzero, boffset, bsoften, blank, image, mask, maskVal, fits)) {
		// We can't have the write dying for this reason --- try to save it somehow!
		psWarning("Unable to determine BSCALE and BZERO for image --- refusing to quantise.");
		psErrorClear();
		return psMemIncrRefCounter((psImage*)image);
	    }
	} else {
	    // Don't want to muck with the current BSCALE and BZERO.  Get the current values and use those.
	    int status = 0;                 // Status of cfitsio
	    if (fits_read_key_dbl(fits->fd, "BSCALE", bscale, NULL, &status) && status != KEY_NO_EXIST) {
		psFitsError(status, true, "Unable to read header.");
		return NULL;
            }

            status = 0;
            if (fits_read_key_dbl(fits->fd, "BZERO", bzero, NULL, &status) && status != KEY_NO_EXIST) {
		psFitsError(status, true, "Unable to read header.");
		return NULL;
            }
            status = 0;
            if (*bscale == 0.0) {
		psError(PS_ERR_IO, true,
			"Supposed to use old values of BSCALE and BZERO, but they don't exist.");
		return NULL;
            }
	}
        psImage *out = psFitsScaleForDisk(image, fits, *bscale, *bzero, *boffset, *bsoften, rng);
        return out;
    }

    // Choose the appropriate output type, given the input type and desired bits per pixel
#define CONVERT_TYPE_INT_CASE(OUTTYPE, INTYPE, BITPIX)			\
    case BITPIX:							\
	OUTTYPE = PS_IS_PSELEMTYPE_UNSIGNED(INTYPE) ? PS_TYPE_U##BITPIX : PS_TYPE_S##BITPIX; \
    break;
#define CONVERT_TYPE_FLOAT_CASE(OUTTYPE, BITPIX)		\
    case -BITPIX: /* Note the use of the negative sign */	\
	OUTTYPE = PS_TYPE_F##BITPIX;				\
    break;


    *bscale = 1.0;
    *bzero = 0.0;
    psElemType inType = image->type.type; // Type for input image
    psElemType outType;                 // Type for output image
    switch (options->bitpix) {
        CONVERT_TYPE_INT_CASE(outType, inType, 8);
        CONVERT_TYPE_INT_CASE(outType, inType, 16);
        CONVERT_TYPE_INT_CASE(outType, inType, 32);
        CONVERT_TYPE_INT_CASE(outType, inType, 64);
        CONVERT_TYPE_FLOAT_CASE(outType, 32);
        CONVERT_TYPE_FLOAT_CASE(outType, 64);
      default:
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Target bitpix (%d) is not one of 8,16,32,64",
                options->bitpix);
        return NULL;
    }

    // psMemCheckCorruption (stderr, true); // 3x

    if (outType == inType) {
        return psMemIncrRefCounter((psImage*)image);
    }

    // psMemCheckCorruption (stderr, true); // 5

    if (PSELEMTYPE_SIZEOF(inType) > PSELEMTYPE_SIZEOF(outType)) {
        psWarning("Truncating image pixels to write to disk.");
    }

    // psMemCheckCorruption (stderr, true); // 4x

    return psImageCopy(NULL, image, outType);
}


// Read into an extant image of just the right size
static bool fitsReadImage(psImage *output,   // Output image
                          const psFits *fits, // FITS file handle
                          p_psFitsReadInfo *info // Info on how to read
                         )
{
    // n.b., this assumes contiguous image buffer
    psAssert(output, "impossible");
    psAssert(fits, "impossible");
    psAssert(info, "impossible");
    psAssert(output->numCols == info->lastPixel[0] - info->firstPixel[0] + 1, "impossible");
    psAssert(output->numRows == info->lastPixel[1] - info->firstPixel[1] + 1, "impossible"); // Right size
    psAssert(!output->parent, "impossible");            // No parents means the buffer is contiguous

    void *nullValue = NULL;             // Null value for data
    float nullFloat = NAN;              // Null value for floating point
    double nullDouble = NAN;            // Null value for double
    switch (info->psDatatype) {
      case PS_TYPE_F32:
        nullValue = &nullFloat;
        break;
      case PS_TYPE_F64:
        nullValue = &nullDouble;
        break;
      case PS_TYPE_U8:
      case PS_TYPE_U16:
      case PS_TYPE_U32:
      case PS_TYPE_U64:
      case PS_TYPE_S8:
      case PS_TYPE_S16:
      case PS_TYPE_S32:
      case PS_TYPE_S64:
        // Can't mark bad pixels any further than what is in the FITS image
        break;
      default:
        psAbort("Unknown type: %x", info->psDatatype);
    }

    int anynull = 0;                    // Are there any NULLs in the data?
    int status = 0;                     // cfitsio status
    if (fits_read_subset(fits->fd, info->fitsDatatype, info->firstPixel, info->lastPixel,
                         info->increment, nullValue, output->data.V[0], &anynull, &status) != 0) {
        psFitsError(status, true, "Reading FITS file %s failed.", fits->fd->Fptr->filename);
        return false;
    }

    // No need to apply the BSCALE, BZERO because cfitsio does this for us
    
    return true;
}

psImage *psFitsReadImage(const psFits *fits, // the psFits object
                         psRegion region, // the region in the FITS image to read
                         int z          // the z-plane in the FITS image cube to read
                        )
{
    PS_ASSERT_FITS_NON_NULL(fits, NULL);
    PS_ASSERT_INT_NONNEGATIVE(z, NULL);

    if (psFitsCheckCompressedImagePHU(fits, NULL)) {
        // This is really what we want, not the empty PHU
        psTrace("psLib.fits", 1,
                "This PHU should really be a compressed image --- reading that image instead.");
    }

    p_psFitsReadInfo *info = p_psFitsReadInfoAlloc(fits, region, z);
    if (!info) {
        psError(PS_ERR_IO, false, "Unable to read FITS information");
        return NULL;
    }

    // Size of image
    int numCols = info->lastPixel[0] - info->firstPixel[0] + 1;
    int numRows = info->lastPixel[1] - info->firstPixel[1] + 1;

    psImage *inImage = psImageAlloc(numCols, numRows, info->psDatatype); // Image to read in

    psFitsFloat floatType = psFitsFloatImageCheck(fits); // Type of custom floating-point
    psImage *outImage = (floatType == PS_FITS_FLOAT_NONE ? psMemIncrRefCounter(inImage) :
                         psImageAlloc(numCols, numRows, psFitsFloatImageType(floatType))); // Output image

    if (!fitsReadImage(inImage, fits, info)) {
        psFree(info);
        psFree(inImage);
        return NULL;
    }

    if (floatType != PS_FITS_FLOAT_NONE) {
        outImage = psFitsFloatImageFromDisk(outImage, inImage, floatType);
    }
    // Need to apply BOFFSET if info->is_logscaled is true

    if (info->is_logscaled) {
      double boffset;
      int status;
      status = 0;
      fits_read_key_dbl(fits->fd, "BOFFSET", &boffset, NULL, &status);
      psImage *newImage = psFitsScaleFromDisk(outImage,boffset,0.0);
      psFree (outImage);
      outImage = newImage;
    }
    else if (info->is_asinh) {
      double bsoften,boffset;
      int status;
      status = 0;
      fits_read_key_dbl(fits->fd, "BSOFTEN", &bsoften, NULL, &status);
      fits_read_key_dbl(fits->fd, "BOFFSET", &boffset, NULL, &status);
      psImage *newImage = psFitsScaleFromDisk(outImage,boffset,bsoften);
      psFree (outImage);
      outImage = newImage;
    }
    psFree(info);

    psFree(inImage);

    return outImage;
}

psImage *psFitsReadImageBuffer(psImage *outImage, // Output image buffer
                               const psFits *fits, // the psFits object
                               psRegion region, // the region in the FITS image to read
                               int z           // the z-plane in the FITS image cube to read
                              )
{
    PS_ASSERT_FITS_NON_NULL(fits, NULL);
    PS_ASSERT_INT_NONNEGATIVE(z, NULL);

    if (outImage && outImage->parent) {
        psError(PS_ERR_IO, true, "Unable to read into a buffer for a child image.\n");
        return NULL;
    }

    if (psFitsCheckCompressedImagePHU(fits, NULL)) {
        // This is really what we want, not the empty PHU
        psTrace("psLib.fits", 1,
                "This PHU should really be a compressed image --- reading that image instead.");
    }

    p_psFitsReadInfo *info = p_psFitsReadInfoAlloc(fits, region, z);

    // Size of image
    int numCols = info->lastPixel[0] - info->firstPixel[0] + 1;
    int numRows = info->lastPixel[1] - info->firstPixel[1] + 1;

    psFitsFloat floatType = psFitsFloatImageCheck(fits); // Type of custom floating-point
    psImage *inImage;                   // Image to read in
    if (floatType == PS_FITS_FLOAT_NONE) {
        if (!outImage || outImage->type.type == info->psDatatype) {
            outImage = psImageRecycle(outImage, numCols, numRows, info->psDatatype);
            inImage = psMemIncrRefCounter(outImage);
        } else {
            outImage = psImageRecycle(outImage, numCols, numRows, outImage->type.type);
            inImage = psImageAlloc(numCols, numRows, info->psDatatype);
        }
    } else {
        inImage = psImageAlloc(numCols, numRows, info->psDatatype);
        outImage = psImageRecycle(outImage, numCols, numRows, psFitsFloatImageType(floatType));
    }

    if (!fitsReadImage(inImage, fits, info)) {
        psFree(info);
        psFree(inImage);
        psFree(outImage);
        return NULL;
    }

    if (floatType != PS_FITS_FLOAT_NONE) {
        outImage = psFitsFloatImageFromDisk(outImage, inImage, floatType);
    } else if (outImage->type.type != info->psDatatype) {
        outImage = psImageCopy(outImage, inImage, outImage->type.type);
    }
    psFree(info);
    psFree(inImage);

    return outImage;
}

bool psFitsWriteImage(psFits *fits, psMetadata *header, const psImage *input,
                      int numZPlanes, const char *extname)
{
    return psFitsWriteImageWithMask(fits, header, input, NULL, 0, numZPlanes, extname);
}

bool psFitsWriteImageWithMask(psFits *fits, psMetadata *header, const psImage *input,
                              const psImage *mask, psImageMaskType maskVal, int numZPlanes,
                              const char *extname)
{
    PS_ASSERT_FITS_NON_NULL(fits, false);
    PS_ASSERT_FITS_WRITABLE(fits, false);
    PS_ASSERT_IMAGE_NON_NULL(input, false);
    // this is equivalent to insert after the last HDU

    psFitsMoveLast(fits);
    return psFitsInsertImageWithMask(fits, header, input, mask, maskVal, numZPlanes, extname, true);
}

bool psFitsInsertImage(psFits *fits, psMetadata *header, const psImage *image, int numZPlanes,
                       const char *extname, bool after)
{
    return psFitsInsertImageWithMask(fits, header, image, NULL, 0, numZPlanes, extname, after);
}

bool psFitsInsertImageWithMask(psFits *fits, psMetadata *header, const psImage *image,
                               const psImage *mask, psImageMaskType maskVal, int numZPlanes,
                               const char *extname, bool after)
{
    PS_ASSERT_FITS_NON_NULL(fits, false);
    PS_ASSERT_FITS_WRITABLE(fits, false);
    PS_ASSERT_IMAGE_NON_NULL(image, false);
    if (mask) {
        PS_ASSERT_IMAGE_TYPE(mask, PS_TYPE_IMAGE_MASK, false);
        PS_ASSERT_IMAGES_SIZE_EQUAL(mask, image, false);
    }

    int numCols = image->numCols;       // Number of columns for image
    int numRows = image->numRows;       // Number of rows for image
    int status = 0;                     // Status from cfitsio
    bool success = true;
    psFitsCompression *compress = NULL; // FITS compression parameters; to save state

    double bscale = NAN, bzero = NAN;   // Scale and zero point to put in header
    double boffset = NAN;               // Log offset to put into header.
    double bsoften = NAN;               // Asinh softening parameter to put into header
    long blank = 0;                     // Blank (undefined) value for image
    psFitsFloat floatType;              // Custom floating-point convention type

    psImage *diskImage = imageToDiskRepresentation(&bscale, &bzero, &boffset, &bsoften, &blank, &floatType, 
						   fits, image, mask, maskVal, NULL, true); // Image to write out

    if (!diskImage) {
        psError(PS_ERR_UNKNOWN, false, "Unable to convert image to desired disk format.");
        success = false;
        goto INSERT_DONE;
    }

    bool useRequestedScale = true;
    if (!isfinite(bscale) || !isfinite(bzero)) {
        // Couldn't scale, so don't compress.  Save compression parameters for later
        useRequestedScale = false;
        compress = psFitsCompressionGet(fits);
        if (!psFitsSetCompression(fits, PS_FITS_COMPRESS_NONE, NULL, 0, 0, 0)) {
            psError(PS_ERR_IO, false, "Unable to unset compression.");
            success = false;
            goto INSERT_DONE;
        }
    }

    // determine the FITS-equivalent parameters
    int bitPix;                         // Bits per pixel
    double cfitsioBzero = 0.0;          // Zero point for cfitsio to apply
    int dataType;                       // cfitsio data type
    if (!p_psFitsTypeToCfitsio(diskImage->type.type, &bitPix, &cfitsioBzero, &dataType)) {
        success = false;
        goto INSERT_DONE;
    }

    if (cfitsioBzero != 0.0) {
        psAssert(bzero == 0.0 && bscale == 1.0,
                 "p_psFitsTypeToCfitsio and imageToDiskRepresentation must not clash!");
        bscale = 1.0;
        bzero = cfitsioBzero;
    }

    psFitsOptions *options = fits->options; // FITS I/O options
     
    psAssert(!useRequestedScale || !options || bitPix == options->bitpix || options->bitpix == 0,
             "Something's not consistent");

    int naxis = 3;                      // Number of axes
    long naxes[3];                      // Length of each axis
    naxes[0] = numCols;
    naxes[1] = numRows;
    naxes[2] = numZPlanes;

    if (numZPlanes < 2) {
        naxis = 2;
    }

    // psMemCheckCorruption (stderr, true);

    bool createPHU = false;             // Are we creating a PHU?

    // Create the image HDU
    int hdus = psFitsGetSize(fits);     // Number of HDUs in file
    if (hdus == 0) {
        // We're creating the first image
        fits_create_img(fits->fd, bitPix, naxis, naxes, &status);
        createPHU = true;
    } else {
        if (!after) {
            if (psFitsGetExtNum(fits) == 0) {
                // We're creating a replacement primary HDU.
                // Set status to signal fits_insert_img to insert a new primary HDU
                status = PREPEND_PRIMARY;
                createPHU = true;
            } else {
                // Move back one to perform an insert after the previous HDU
                psFitsMoveExtNum(fits, -1, true);
            }
        }
        // Insert after the current position
        fits_insert_img(fits->fd, bitPix, naxis, naxes, &status);
    }
    if (psFitsError(status, true, "Could not create image HDU.")) {
        success = false;
        goto INSERT_DONE;
    }

    // Remove any BOFFSET values that exist in the header if we are not using that scaling anymore
    if (options && (!((options->scaling == PS_FITS_SCALE_LOG_RANGE)||
		      (options->scaling == PS_FITS_SCALE_LOG_MANUAL)||
		      (options->scaling == PS_FITS_SCALE_LOG_STDEV_POSITIVE)||
		      (options->scaling == PS_FITS_SCALE_LOG_STDEV_NEGATIVE)||
		      (options->scaling == PS_FITS_SCALE_LOG_STDEV_BOTH)||
		      (options->scaling == PS_FITS_SCALE_ASINH_RANGE)||
		      (options->scaling == PS_FITS_SCALE_ASINH_MANUAL)||
		      (options->scaling == PS_FITS_SCALE_ASINH_STDEV_POSITIVE)||
		      (options->scaling == PS_FITS_SCALE_ASINH_STDEV_NEGATIVE)||
		      (options->scaling == PS_FITS_SCALE_ASINH_STDEV_BOTH)))) {
      if (header && psMetadataLookup(header,"BOFFSET")) {
	psMetadataRemoveKey(header,"BOFFSET");
      }
    }	
    // Remove any BSOFTEN values that exist in the header if we are not using that scaling anymore
    if (options && (!((options->scaling == PS_FITS_SCALE_ASINH_RANGE)||
		      (options->scaling == PS_FITS_SCALE_ASINH_MANUAL)||
		      (options->scaling == PS_FITS_SCALE_ASINH_STDEV_POSITIVE)||
		      (options->scaling == PS_FITS_SCALE_ASINH_STDEV_NEGATIVE)||
		      (options->scaling == PS_FITS_SCALE_ASINH_STDEV_BOTH)))) {
      if (header && psMetadataLookup(header,"BSOFTEN")) {
	psMetadataRemoveKey(header,"BSOFTEN");
      }
    }	

    // If there are no options set, remove all our non-standard keywords, because no one asked for them.
    if (!options) {
      if (header && psMetadataLookup(header,"BOFFSET")) {
	psMetadataRemoveKey(header,"BOFFSET");
      }
      if (header && psMetadataLookup(header,"BSOFTEN")) {
	psMetadataRemoveKey(header,"BSOFTEN");
      }
    }      
    
    // write the header, if any.
    if (header && !psFitsWriteHeaderImage(fits, header, createPHU)) {
        psError(PS_ERR_IO, false, "Unable to write FITS header.\n");
        success = false;
        goto INSERT_DONE;
    }

    // We only want cfitsio to do the scale and zero if the type conversion requires it (e.g., input type is
    // an unsigned integer type).  In all other cases, we have already converted the image to use the
    // appropriate scale and zero (because we want to apply a randomiser to the quantisation).
    fits_set_bscale(fits->fd, 1.0, cfitsioBzero, &status);

    if (isfinite(bzero) && isfinite(bscale) && (bscale != 0.0)) {
	fits_write_key_dbl(fits->fd, "BZERO", bzero, 12,
                           "Scaling: TRUE = BZERO + BSCALE * DISK", &status);
        fits_write_key_dbl(fits->fd, "BSCALE", bscale, 12,
                           "Scaling: TRUE = BZERO + BSCALE * DISK", &status);
	if (options&&(((options->scaling == PS_FITS_SCALE_LOG_RANGE)||
		       (options->scaling == PS_FITS_SCALE_LOG_MANUAL)||
		       (options->scaling == PS_FITS_SCALE_LOG_STDEV_POSITIVE)||
		       (options->scaling == PS_FITS_SCALE_LOG_STDEV_NEGATIVE)||
		       (options->scaling == PS_FITS_SCALE_LOG_STDEV_BOTH)))) {
	  fits_write_key_dbl(fits->fd, "BOFFSET", boffset, 12,
			     "Scaling: UNCOMP = 10**(TRUE) + BOFFSET", &status);
	}	
	if (options&&(((options->scaling == PS_FITS_SCALE_ASINH_RANGE)||
		       (options->scaling == PS_FITS_SCALE_ASINH_MANUAL)||
		       (options->scaling == PS_FITS_SCALE_ASINH_STDEV_POSITIVE)||
		       (options->scaling == PS_FITS_SCALE_ASINH_STDEV_NEGATIVE)||
		       (options->scaling == PS_FITS_SCALE_ASINH_STDEV_BOTH)))) {
	  fits_write_key_dbl(fits->fd, "BSOFTEN", bsoften, 12,
			     "Scaling: LINEAR = 2 * BSOFTEN * sinh(TRUE/a)",&status);
	  fits_write_key_dbl(fits->fd, "BOFFSET", boffset, 12,
			     "Scaling: UNCOMP = BOFFSET + LINEAR",&status);
	}	

        if (psFitsError(status, true, "Could not write BSCALE/BZERO headers to file.")) {
            success = false;
            goto INSERT_DONE;
        }
    }

    if (blank != 0 && bitPix > 0) {
        // Some quantisation has taken place --- record the blank ("magic") pixel value.
        //
        // According to http://heasarc.gsfc.nasa.gov/docs/heasarc/fits/compress/compress_image.html,
        // the keyword is "BLANK" when BITPIX > 0, and "ZBLANK" when BITPIX < 0.  Since we do our
        // own quantisation, the correct keyword is always "BLANK" instead of "ZBLANK".
        fits_write_key_lng(fits->fd, "BLANK", blank, "Value for undefined pixels", &status);
        // But it seems that cfitsio uses ZBLANK
        fits_write_key_lng(fits->fd, "ZBLANK", blank, "Value for undefined pixels", &status);
        fits_set_imgnull(fits->fd, blank, &status);
        if (psFitsError(status, true, "Could not write BLANK header to file.")) {
            success = false;
            goto INSERT_DONE;
        }
    }

    if (floatType != PS_FITS_FLOAT_NONE) {
        psFitsFloatImageSet(fits, floatType);
    }

    if (extname && strlen(extname) > 0) {
        psFitsSetExtName(fits, extname);
    }

    if (image->parent == NULL) {
        // if no parent, assume that the image data is contiguous
	fits_write_img(fits->fd, dataType, 1, numCols*numRows, diskImage->data.V[0], &status);
    } else {
        // image data may not be contiguous; write one row at a time
        int firstPixel = 1;
        for (int row = 0; row < numRows; row++) {
            fits_write_img(fits->fd, dataType, firstPixel, numCols, diskImage->data.V[row], &status);
            firstPixel += numCols;
        }
    }

    // psMemCheckCorruption (stderr, true);

    if (psFitsError(status, true, "Could not write image to file.")) {
        success = false;
        goto INSERT_DONE;
    }

    // This forces a re-scan of the header to ensure everything's kosher.
    // Without this, compressed HDUs have been written out with PCOUNT=0 and TFORM1 not correctly set.
    ffrdef(fits->fd, &status);
    if (psFitsError(status, true, "Could not re-scan HDU.")) {
        success = false;
        goto INSERT_DONE;
    }


    // psMemCheckCorruption (stderr, true);

 INSERT_DONE:
    psFree(diskImage);

    if (compress) {
        // Restore compression state
        psFitsCompressionApply(fits, compress);
        psFree(compress);
    }

    return success;
}

bool psFitsUpdateImage(psFits *fits, const psImage *input, int x0, int y0, int z)
{
    return psFitsUpdateImageWithMask(fits, input, NULL, 0, x0, y0, z);
}

bool psFitsUpdateImageWithMask(psFits *fits, const psImage *input, const psImage *mask, psImageMaskType maskVal,
                               int x0, int y0, int z)
{
    PS_ASSERT_FITS_NON_NULL(fits, false);
    PS_ASSERT_FITS_WRITABLE(fits, false);
    PS_ASSERT_IMAGE_NON_NULL(input, false);
    if (mask) {
        PS_ASSERT_IMAGE_TYPE(mask, PS_TYPE_IMAGE_MASK, false);
        PS_ASSERT_IMAGES_SIZE_EQUAL(mask, input, false);
    }

    int status = 0;

    // check to see if we are positioned on an image HDU
    int hduType;
    if (fits_get_hdu_type(fits->fd, &hduType, &status) != 0) {
        psFitsError(status, true, "Could not determine the HDU type.");
        return NULL;
    }
    if (hduType != IMAGE_HDU) {
        psError(PS_ERR_IO, true, _("Current FITS HDU type must be an image."));
        return NULL;
    }

    int numCols = input->numCols;
    int numRows = input->numRows;
    psFitsCompression *compress = NULL; // FITS compression parameters; to save state

    bool success = true;                // Successful update?
    double bscale = NAN, bzero = NAN;   // Scale and zero point to put in header
    double boffset = NAN;               // Log offset to put in header
    double bsoften = NAN;               // Asinh softening parameter to put in header
    long blank = 0;                     // Blank (undefined) value for image
    psFitsFloat floatType;              // Custom floating-point convention type
    psImage *diskImage = imageToDiskRepresentation(&bscale, &bzero, &boffset, &bsoften, &blank, &floatType, fits, input,
                                                   mask, maskVal, NULL, false); // Image to write out
    
    if (!diskImage) {
        psError(PS_ERR_UNKNOWN, false, "Unable to convert image to desired disk format.");
        success = false;
        goto UPDATE_DONE;
    }

    bool useRequestedScale = true;
    if (!isfinite(bscale) || !isfinite(bzero)) {
        // Couldn't scale, so don't compress.  Save compression parameters for later
        useRequestedScale = false;
        compress = psFitsCompressionGet(fits);
        if (!psFitsSetCompression(fits, PS_FITS_COMPRESS_NONE, NULL, 0, 0, 0)) {
            psError(PS_ERR_IO, false, "Unable to unset compression.");
            success = false;
            goto UPDATE_DONE;
        }
    }

    // determine the FITS-equivalent parameters
    int bitPix;                         // Bits per pixel
    double cfitsioBzero = 0.0;          // Zero point for cfitsio to apply
    int dataType;                       // cfitsio data type
    if (!p_psFitsTypeToCfitsio(diskImage->type.type, &bitPix, &cfitsioBzero, &dataType)) {
        success = false;
        goto UPDATE_DONE;
    }

    if (cfitsioBzero != 0.0) {
        psAssert(bzero == 0.0 && bscale == 1.0,
                 "p_psFitsTypeToCfitsio and imageToDiskRepresentation must not clash!");
        bscale = 1.0;
        bzero = cfitsioBzero;
    }
    psFitsOptions *options = fits->options; // FITS I/O options
    psAssert(!useRequestedScale || !options || bitPix == options->bitpix || options->bitpix == 0,
             "Something's not consistent");

    // Check to see if the HDU has the same datatype
    int fileBitpix;
    int naxis;
    long nAxes[3];
    nAxes[2] = 1;
    fits_get_img_param(fits->fd, 3, &fileBitpix, &naxis, nAxes, &status);

    //check if the HDU has the z-plane requested
    if (z >= nAxes[2]) {
        psError(PS_ERR_BAD_PARAMETER_SIZE, true,
                _("Current FITS HDU has %ld z-planes, but z-plane %d was specified."), nAxes[2], z);
        success = false;
        goto UPDATE_DONE;
    }

    // determine the region in the FITS file domain
    long firstPixel[3];
    long lastPixel[3];

    firstPixel[0] = x0 + 1;
    firstPixel[1] = y0 + 1;
    firstPixel[2] = z + 1;

    lastPixel[0] = x0 + numCols;
    lastPixel[1] = y0 + numRows;
    lastPixel[2] = z + 1;

    if (firstPixel[0] < 1 || firstPixel[0] > nAxes[0] ||
        firstPixel[1] < 1 || firstPixel[1] > nAxes[1] ||
        lastPixel[0] < 1 || lastPixel[0] > nAxes[0] ||
        lastPixel[1] < 1 || lastPixel[1] > nAxes[1]) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                "Input image [size of %ix%i] at position (%i,%i) does not all lay in the %lix%li FITS image.",
                numCols, numRows, x0, y0, nAxes[0], nAxes[1]);
        success = false;
        goto UPDATE_DONE;
    }
    
    // We only want cfitsio to do the scale and zero if the type conversion requires it (e.g., input type is
    // an unsigned integer type).  In all other cases, we have already converted the image to use the
    // appropriate scale and zero (because we want to apply a randomiser to the quantisation).
    fits_set_bscale(fits->fd, 1.0, cfitsioBzero, &status);
    fits_write_subset(fits->fd, dataType, firstPixel, lastPixel, diskImage->data.V[0], &status);
    if (psFitsError(status, true, "Could not write data to file.")) {
        success = false;
        goto UPDATE_DONE;
    }

    // This forces a re-scan of the header to ensure everything's kosher.  We found this occassionally
    // necessary for compressed images, which are tables, so perhaps it helps here too.  I guess it can't
    // hurt.
    ffrdef(fits->fd, &status);
    if (psFitsError(status, true, "Could not re-scan HDU.")) {
        success = false;
        goto UPDATE_DONE;
    }

UPDATE_DONE:
    psFree(diskImage);
    if (compress) {
        // Restore compression state
        psFitsCompressionApply(fits, compress);
        psFree(compress);
    }

    return success;
}

psArray *psFitsReadImageCube(const psFits *fits, psRegion region)
{
    PS_ASSERT_FITS_NON_NULL(fits, NULL);

    int nAxis = 0;                      // Number of axes
    long nAxes[3];                      // Number of pixels on each axis
    int status = 0;                     // cfitsio status value

    // Some of this replicates what is in psFitsReadImage, so it's a little inefficient.  But it saves
    // code replication, and should be sufficient for our needs.

    if (psFitsCheckCompressedImagePHU(fits, NULL)) {
        // This is really what we want, not the empty PHU
        psTrace("psLib.fits", 1,
                "This PHU should really be a compressed image --- reading that image instead.");
    }

    if (fits_get_img_dim(fits->fd, &nAxis, &status) != 0) {
        psFitsError(status, true, "Could not determine image dimensions.");
        return NULL;
    }

    if (nAxis == 2) {
	psImage *image = psFitsReadImage(fits, region, 0);
	if (!image) {
            psFitsError(status, true, "Could not read image into cube");
            return NULL;
	}
        psArray *images = psArrayAlloc(1); // Single image plane
        images->data[0] = image;
        return images;
    }
    if (nAxis == 3) {
        if (fits_get_img_size(fits->fd, nAxis, nAxes, &status) != 0) {
            psFitsError(status, true, "Could not determine image size.");
            return NULL;
        }

        psArray *images = psArrayAlloc(nAxes[2]); // Array of image planes
        for (int i = 0; i < nAxes[2]; i++) {
            images->data[i] = psFitsReadImage(fits, region, i);
        }

        return images;
    }

    // Bad dimensionality
    psError(PS_ERR_IO, true, _("Image number of dimensions, %d, is not supported."), nAxis);
    return NULL;
}

bool psFitsWriteImageCube(psFits *fits, psMetadata *header, const psArray *input, const char *extname)
{
    return psFitsWriteImageCubeWithMask(fits, header, input, NULL, 0, extname);
}

bool psFitsWriteImageCubeWithMask(psFits *fits, psMetadata *header, const psArray *input,
                                  const psArray *masks, psImageMaskType maskVal, const char *extname)
{
    PS_ASSERT_FITS_NON_NULL(fits, false);
    PS_ASSERT_FITS_WRITABLE(fits, false);
    PS_ASSERT_ARRAY_NON_NULL(input, false);
    if (masks) {
        PS_ASSERT_ARRAY_NON_NULL(masks, false);
        PS_ASSERT_ARRAYS_SIZE_EQUAL(masks, input, false);
    }

    if (input->n == 0) {
        psError(PS_ERR_BAD_PARAMETER_SIZE, true, _("The input array was empty."));
        return false;
    }

    if (input->n == 1) {
        // The problem reduces to one already solved
        return psFitsWriteImageWithMask(fits, header, input->data[0],
                                        masks ? masks->data[0] : NULL, maskVal, 1, extname);
    }

    // Check that all images are of the same size
    psImage *testImage = input->data[0];// First image off the array
    int numCols = testImage->numCols;   // Number of columns
    int numRows = testImage->numRows;   // Number of rows
    for (int i = 1; i < input->n; i++) {
        testImage = input->data[i];
        if (testImage->numCols != numCols || testImage->numRows != numRows) {
            psError(PS_ERR_BAD_PARAMETER_SIZE, true, _("The sizes of images in the array differ."));
            return false;
        }
    }

    // Need to check the header to make sure NAXIS and NAXIS[1-3] are correct
    psMetadata *headerCopy = NULL;      // Copy of header
    if (header) {
        headerCopy = psMemIncrRefCounter(header);
    } else {
        headerCopy = psMetadataAlloc();
    }
    bool update = psMetadataAddS32(headerCopy, PS_LIST_HEAD, "NAXIS", PS_META_REPLACE, "Dimensionality", 3) &&
        psMetadataAddS32(headerCopy, PS_LIST_HEAD, "NAXIS1", PS_META_REPLACE, "Number of columns", numCols) &&
        psMetadataAddS32(headerCopy, PS_LIST_HEAD, "NAXIS2", PS_META_REPLACE, "Number of rows", numRows) &&
        psMetadataAddS32(headerCopy, PS_LIST_HEAD, "NAXIS3", PS_META_REPLACE, "Number of image planes",
                         input->n);
    if (!update) {
        psError(PS_ERR_UNKNOWN, false, _("Failed to add metadata item, %s."),
                "NAXIS, NAXIS1, NAXIS2, NAXIS3");
        psFree(headerCopy);
        return false;
    }

    // Now we can safely write the images out.
    // The first is an psFitsImageWrite to create the extension.
    // The next are psFitsImageUpdate to write into the extension.
    if (!psFitsWriteImageWithMask(fits, headerCopy, input->data[0],
                                  masks ? masks->data[0] : NULL, maskVal, input->n, extname)) {
        psError(PS_ERR_UNKNOWN, false, _("Could not write image plane %d."), 0);
        psFree(headerCopy);
        return false;
    }
    psFree(headerCopy);                 // Free, or drop reference

    for (int i = 1; i < input->n; i++) {
        if (!psFitsUpdateImageWithMask(fits, input->data[i],
                                       masks ? masks->data[i] : NULL, maskVal, 0, 0, i)) {
            psError(PS_ERR_UNKNOWN, false, _("Could not write image plane %d."), i);
            return false;
        }
    }

    return true;
}

bool psFitsUpdateImageCube(psFits *fits, const psArray *input, int x0, int y0)
{
    return psFitsUpdateImageCubeWithMask(fits, input, NULL, 0, x0, y0);
}

bool psFitsUpdateImageCubeWithMask(psFits *fits, const psArray *input,
                                   const psArray *masks, psImageMaskType maskVal, int x0, int y0)
{
    PS_ASSERT_FITS_NON_NULL(fits, false);
    PS_ASSERT_FITS_WRITABLE(fits, false);
    PS_ASSERT_ARRAY_NON_NULL(input, false);

    if (input->n == 0) {
        psError(PS_ERR_BAD_PARAMETER_SIZE, true, _("The input array was empty."));
        return false;
    }

    for (int i = 0; i < input->n; i++) {
        if (!psFitsUpdateImageWithMask(fits, input->data[i],
                                       masks ? masks->data[i] : NULL, maskVal, x0, y0, i)) {
            psError(PS_ERR_UNKNOWN, false, _("Could not update image plane %d."), i);
            return false;
        }
    }

    return true;
}

