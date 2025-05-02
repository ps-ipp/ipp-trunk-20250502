/* @file pmMaskBadPixels.h
 * @brief Mask bad pixels
 *
 * @author Ross Harman, MHPCC
 * @author Eugene Magnier, IfA
 *
 * @version $Revision: 1.17 $ $Name: not supported by cvs2svn $
 * @date $Date: 2009-01-27 06:39:38 $
 * Copyright 2004 Institute for Astronomy, University of Hawaii
 */

#ifndef PM_MASK_BAD_PIXELS_H
#define PM_MASK_BAD_PIXELS_H

/// @addtogroup detrend Detrend Creation and Application
/// @{

#define PM_MASK_ANALYSIS_SUSPECT "MASK.SUSPECT" // Readout analysis metadata keyword for suspect image
#define PM_MASK_ANALYSIS_NUM "MASK.NUM" // Readout analysis metadata keyword for number of inputs


typedef enum {
  PM_MASK_ID_NONE,
  PM_MASK_ID_VALUE,
  PM_MASK_ID_FRACTION,
  PM_MASK_ID_SIGMA,
  PM_MASK_ID_POISSON,
} pmMaskIdentifyMode;

pmMaskIdentifyMode pmMaskIdentifyModeFromString (const char *string);

/// Applies the bad pixel mask to the input
///
/// Pixels marked as bad within the mask are marked as bad within the input image's mask.  If maskVal is
/// non-zero, all pixels in the mask have any of the same bits sets as maskVal shall have the corresponding
/// bits raised.  If maskVal is zero, any zero pixels in the mask are OR-ed with PM_MASK_BAD.  Position
/// offsets (such as due to trimming) between the input and mask are applied so that the same pixels are
/// referred to.  The science readout must already have a supplied mask element (use eg. pmReadoutSetMask).
/// The supplied mask image must be of MASK type
bool pmMaskBadPixels(pmReadout *input,  ///< Input science image
                     const pmReadout *mask, ///< Mask image to apply
                     psImageMaskType maskVal ///< Mask value to apply
                    );

/// Find pixels outlying from the background, flagging suspect pixels
///
/// Pixels more than "rej" standard deviations from the background level (in flat-fielded,
/// background-subtracted images) have the corresponding pixel in the "suspect pixels" image
/// incremented.  After accumulating over a suitable sample of images, bad pixels should have a
/// high value in the suspect pixels image, allowing them to be identified.  The suspect pixels
/// image is of type S32.  The relevant median and standard deviation must be supplied in the
/// readout->analysis metadata as READOUT.MEDIAN, READOUT.STDEV
bool pmMaskFlagSuspectPixelsBySigma(pmReadout *output, ///< Output readout, optionally with suspect pixels image
                             const pmReadout *readout, ///< Readout to inspect
                             float median, ///< Image median
                             float stdev, ///< Image standard deviation
                             float rej, ///< Rejection threshold (standard deviations)
                             psImageMaskType maskVal ///< Mask value for statistics
    );

/// Find out-of-range pixels and flag them
///
/// Pixels great > max or < min have the corresponding pixel in the "suspect pixels" image
/// incremented.  After accumulating over a suitable sample of images, bad pixels should have a
/// high value in the suspect pixels image, allowing them to be identified.  The suspect pixels
/// image is of type S32.  The relevant median and standard deviation must be supplied in the
/// readout->analysis metadata as READOUT.MEDIAN, READOUT.STDEV
bool pmMaskFlagSuspectPixelsByValue(pmReadout *output, ///< Output readout, optionally with suspect pixels image
                             const pmReadout *readout, ///< Readout to inspect
                             float min, ///< Image min acceptable value
                             float max, ///< Image max acceptable value
                             psImageMaskType maskVal ///< Mask value for statistics
    );

/// Identify bad pixels from the suspect pixels image
///
/// Bad pixels are identified from the suspect pixels image (accumulated over a large number of images),
/// according to the chosen mode.
bool pmMaskIdentifyBadPixels(pmReadout *output, ///< Output readout, with suspect pixels imageOut
                             psImageMaskType maskVal, ///< Value to set for bad pixels
                             float thresh, ///< Threshold for bad pixel
                             pmMaskIdentifyMode mode ///< Mode for identifying bad pixels
    );
/// @}
#endif
