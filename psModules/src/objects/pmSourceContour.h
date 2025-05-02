/* @file  pmSourceContour.h
 *
 * @author EAM, IfA; GLG, MHPCC
 *
 * @version $Revision: 1.3 $ $Name: not supported by cvs2svn $
 * @date $Date: 2007-01-24 02:54:15 $
 * Copyright 2004 Maui High Performance Computing Center, University of Hawaii
 */

# ifndef PM_SOURCE_CONTOUR_H
# define PM_SOURCE_CONTOUR_H

/// @addtogroup Objects Object Detection / Analysis Functions
/// @{

psArray *pmSourceContour (psImage *image, int xc, int yc, float threshold);


/** pmSourceContour()
 *
 * Find points in a contour for the given source at the given level. If type
 * is PM_CONTOUR_CRUDE, the contour is found by starting at the source peak,
 * running along each pixel row until the level is crossed, then interpolating to
 * the level coordinate for that row. This is done for each row, with the
 * starting point determined by the midpoint of the previous row, until the
 * starting point has a value below the contour level. The returned contour
 * consists of two vectors giving the x and y coordinates of the contour levels.
 * This function may be used as part of the model guess inputs.  Other contour
 * types may be specified in the future for more refined contours (TBD)
 *
 */
psArray *pmSourceContour_Crude(
    pmSource *source,   ///< The input pmSource
    const psImage *image,  ///< The input image (float) (this arg should be removed)
    float level   ///< The level of the contour
);

/// @}
# endif /* PM_SOURCE_PHOTOMETRY_H */
