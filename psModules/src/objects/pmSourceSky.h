/* @file  pmSourceSky.h
 * @author EAM, IfA; GLG, MHPCC
 *
 * @version $Revision: 1.5 $ $Name: not supported by cvs2svn $
 * @date $Date: 2009-01-27 06:39:38 $
 * Copyright 2004 Maui High Performance Computing Center, University of Hawaii
 */

# ifndef PM_SOURCE_SKY_H
# define PM_SOURCE_SKY_H

/// @addtogroup Objects Object Detection / Analysis Functions
/// @{

/** pmSourceLocalSky()
 *
 * Measure the local sky in the vicinity of the given source. The Radius
 * defines the square aperture in which the moments will be measured. This
 * function assumes the source pixels have been defined, and that the value of
 * Radius here is smaller than the value of Radius used to define the pixels. The
 * annular region not contained within the radius defined here is used to measure
 * the local background in the vicinity of the source. The local background
 * measurement uses the specified statistic passed in via the statsOptions entry.
 * This function allocates the pmMoments structure. The resulting sky is used to
 * set the value of the pmMoments.sky element of the provided pmSource structure.
 *
 */
bool pmSourceLocalSky(
    pmSource *source,   ///< The input image (float)
    psStatsOptions statsOptions, ///< The statistic used in calculating the background sky
    float Radius,   ///< The inner radius of the square annulus to exclude
    psImageMaskType maskVal,                 ///< Value to mask
    psImageMaskType mark                     ///< Mask value for marking
);


// A complementary function to pmSourceLocalSky: calculate the local sky variance
bool pmSourceLocalSkyVariance(
    pmSource *source,   ///< The input image (float)
    psStatsOptions statsOptions, ///< The statistic used in calculating the background sky
    float Radius,   ///< The inner radius of the square annulus to exclude
    psImageMaskType maskVal,                 ///< Value to mask
    psImageMaskType mark                     ///< Mask value for marking
);

/// @}
# endif /* PM_SOURCE_PHOTOMETRY_H */
