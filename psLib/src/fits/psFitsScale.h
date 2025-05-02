#ifndef PS_FITS_SCALE_H
#define PS_FITS_SCALE_H

#include <psFits.h>
#include <psImage.h>
#include <psType.h>
#include <psRandom.h>

/// Determine BSCALE and BZERO for an image
bool psFitsScaleDetermine(double *bscale, ///< Scaling, to return
                          double *bzero, ///< Zero point, to return
			  double *boffset, ///< Log offset, to return
			  double *bsoften, ///< asinh softening parameter, to return
                          long *blank,  ///< Blank value, to return
                          const psImage *image, ///< Image to scale
                          const psImage *mask, ///< Mask image
                          psImageMaskType maskVal, ///< Value to mask
                          const psFits *fits ///< FITS options
    );

/// Apply the BSCALE and BZERO for an image, so that we get the image as it should be written to disk.
///
/// "Fuzz" may be optionally added.  The idea is that the "fuzz" (adding a random number between 0 and 1)
/// preserves the expectation value of the image (e.g., a value of 0.1 will get translated to zero 90% of the
/// time, and unity 10% of the time), though at the cost of adding an additional variance of 1/12 (a standard
/// deviation of ~0.29).
psImage *psFitsScaleForDisk(const psImage *image, ///< Image to which to apply BSCALE and BZERO
                            const psFits *fits, ///< FITS file
                            double bscale, ///< Scaling
                            double bzero, ///< Zero point
			    double boffset, ///< Log offset
			    double bsoften, ///< asinh softening parameter
                            psRandom *rng ///< Random number generator (for the "fuzz"), or NULL
    );
psImage *psFitsScaleFromDisk(const psImage *image, ///< Image to to unapply BOFFSET
			     double boffset,        ///< Log offset
			     double bsoften         ///< asinh softening parameter
			     );
/// Interpret a string as a scaling method
psFitsScaling psFitsScalingFromString(const char *string ///< String to interpret
    );

#endif
