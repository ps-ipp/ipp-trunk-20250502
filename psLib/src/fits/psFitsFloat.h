#ifndef PS_FITS_FLOAT_H
#define PS_FITS_FLOAT_H

#include <psFits.h>
#include <psType.h>
#include <psImage.h>

/// Type of custom floating point
typedef enum {
    PS_FITS_FLOAT_NONE,                 ///< No conversion to be performed
    PS_FITS_FLOAT_16_0,                 ///< Original F16 proposal: 1S 5E 10M
} psFitsFloat;

/// Convert an image to custom floating-point (the disk representation) in preparation for writing as FITS
psImage *psFitsFloatImageToDisk(const psImage *image, ///< Image to convert
                               psFitsFloat type ///< Custom floating point type
    );

/// Convert the custom floating-point image (the disk representation) to a normal floating-point image
psImage *psFitsFloatImageFromDisk(psImage *out, ///< Output image, or NULL
                                  const psImage *in, ///< Image to convert
                                  psFitsFloat type ///< Custom floating point type
    );

/// Return the appropriate element type for a custom floating-point
psElemType psFitsFloatImageType(psFitsFloat type ///< Custom floating-point type
    );

/// Return the custom floating-point type from a string description
psFitsFloat psFitsFloatTypeFromString(const char *string ///< String with name of type
    );

#endif
