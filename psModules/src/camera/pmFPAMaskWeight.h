/* @file pmFPAHeader.h
 * @brief Functions read FITS headers for FPA components
 *
 * @author Paul Price, IfA
 * @author Eugene Magnier, IfA
 *
 * @version $Revision: 1.18 $ $Name: not supported by cvs2svn $
 * @date $Date: 2009-02-06 02:31:24 $
 * Copyright 2005-2006 Institute for Astronomy, University of Hawaii
 */

#ifndef PM_FPA_MASK_WEIGHT_H
#define PM_FPA_MASK_WEIGHT_H

/// @addtogroup Camera Camera Layout
/// @{

#define PM_READOUT_ANALYSIS_RENORM "READOUT.RENORM" // Name on analysis metadata for renormalisation

/// Set a temporary readout mask using CELL.SATURATION and CELL.BAD
///
/// Identifies pixels that are saturated (>= CELL.SATURATION) or bad (<= CELL.BAD).  The mask that is produced
/// within the readout is temporary --- it is not added to the HDU.  This is intended for when the user is
/// iterating using pmReadoutReadNext, in which case the HDU can't be generated.
bool pmReadoutSetMask(pmReadout *readout, ///< Readout for which to set mask
                      psImageMaskType satMask, ///< Mask value to give saturated pixels
                      psImageMaskType badMask  ///< Mask value to give bad (low) pixels
    );


/// Set a temporary readout variance map using CELL.GAIN and CELL.READNOISE
///
/// Calculates variances for each pixel using photon statistics and the cell gain (CELL.GAIN) and read noise
/// (CELL.READNOISE).  The weight map that is produced within the readout is temporary --- it is not added to
/// the HDU.  This is intended for when the user is iterating using pmReadoutReadNext, in which case the HDU
/// can't be generated.
bool pmReadoutSetVariance(pmReadout *readout, ///< Readout for which to set variance
                          const psImage *noiseMap, ///< 2D image of the read noise in DN
                          bool poisson    ///< Include poisson variance (in addition to read noise)?
    );

/// Generate a readout mask (suitable for output) using CELL.SATURATION and CELL.BAD
///
/// Identifies pixels that are saturated (>= CELL.SATURATION) or bad (<= CELL.BAD).  The mask that is produced
/// is suitable for output (complete with HDU entry).  This is intended for most operations.
bool pmReadoutGenerateMask(pmReadout *readout, ///< Readout for which to generate mask
                           psImageMaskType sat, ///< Mask value to give saturated pixels
                           psImageMaskType bad ///< Mask value to give bad (low) pixels
    );

/// Generate a variance map (suitable for output) using CELL.GAIN and CELL.READNOISE
///
/// Calculates variances for each pixel using photon statistics and the cell gain (CELL.GAIN) and read noise
/// (CELL.READNOISE).  The variance map that is produced within the readout is suitable for output (complete
/// with HDU entry).  This is intended for most operations.
bool pmReadoutGenerateVariance(pmReadout *readout, ///< Readout for which to generate variance
                          const psImage *noiseMap, ///< 2D image of the read noise in DN
                               bool poisson    ///< Include poisson variance (in addition to read noise)?
    );

/// Generate mask and variance map for a readout
///
/// Calls pmReadoutGenerateMask and pmReadoutGenerateVariance for the readout
bool pmReadoutGenerateMaskVariance(pmReadout *readout, ///< Readout for which to generate mask and variance
                                   psImageMaskType sat, ///< Mask value to give saturated pixels
                                   psImageMaskType bad, ///< Mask value to give bad (low) pixels
                                   const psImage *noiseMap, ///< 2D image of the read noise in DN
                                   bool poisson ///< Include poisson variance (in addition to read noise)?
    );

/// Generate mask and variance maps for all readouts within a cell
///
/// Calls pmReadoutGenerateMaskVariance for each readout within the cell.
bool pmCellGenerateMaskVariance(pmCell *cell, ///< Cell for which to generate mask and variance
                                psImageMaskType sat, ///< Mask value to give saturated pixels
                                psImageMaskType bad, ///< Mask value to give bad (low) pixels
                                const psImage *noiseMap, ///< 2D image of the read noise in DN
                                bool poisson ///< Include poisson variance (in addition to read noise)?
    );

/// Renormalise the variance map to match the actual pixel variance
///
/// The variance map is adjusted so that the mean matches the actual pixel variance in the image
bool pmReadoutVarianceRenormalise(
    const pmReadout *readout,           ///< Readout to normalise
    psImageMaskType maskVal,            ///< Value to mask
    int sample,                         ///< Sample size
    float minValid,                     ///< Minimum valid renormalisation, or NAN
    float maxValid                      ///< Maximum valid renormalisation, or NAN
    );

/// Explicitly mask non-finite pixels
///
/// Since unmasked non-finite pixels can occur (e.g., by out-of-range in quantisation), it is sometimes
/// necessary to mask them explicitly.  Non-finite pixels in the image or variance have their mask OR-ed with
/// the provided value.
bool pmReadoutMaskNonfinite(pmReadout *readout, ///< Readout to mask
                            psImageMaskType maskVal ///< Mask value to give non-finite pixels
    );

// find any pixels which are not already masked (with maskTest) which are not valid and raise maskSet bits
bool pmReadoutMaskInvalid (const pmReadout *readout, psImageMaskType maskTest, psImageMaskType maskSet);

/// Apply a mask to the image and variance map
///
/// Unfortunately, image subtraction may result in a bi-modal image in masked areas, which can upset image
/// statistics (very important for quantising images so that a product can be written out!).  This function
/// sets masked areas to NAN in the image and variance.
bool pmReadoutMaskApply(pmReadout *readout, ///< Readout to mask
                        psImageMaskType maskVal ///< Mask value for which to apply mask
    );

/// Interpolate over bad pixels
///
/// Scan the mask image for bad pixels, and interpolate over them using the nominated options
bool pmReadoutInterpolateBadPixels(pmReadout *readout, ///< Readout to work on
                                   psImageMaskType maskVal, ///< Value to mask
                                   psImageInterpolateMode mode, ///< Interpolation mode
                                   float poorFrac, ///< Maximum bad fraction of kernel for "poor" status
                                   psImageMaskType maskPoor, ///< Mask value to give poor pixels
                                   psImageMaskType maskBad ///< Mask value to give bad pixels
    );

/// @}
#endif
