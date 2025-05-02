/** @file  psImageUnbin.h
 *
 *  @brief Functions to unbin images
 *
 *  @author EAM, IfA
 *  @author Robert DeSonia, MHPCC
 *
 *  $Revision: 1.6 $ $Name: not supported by cvs2svn $
 *  $Date: 2008-11-17 02:38:46 $
 *  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 */

#ifndef PS_IMAGE_UNBIN_H
#define PS_IMAGE_UNBIN_H

/// @addtogroup ImageOps Image Operations
/// @{

// This needs to be considered more carefully
psImage *psImageUnbin (psImage *out,    //!< Output image
                       const psImage *in, //!< Input image
                       const psImageBinning *binning ///< binning definition
    );

double psImageUnbinPixel(const double xFine, const double yFine, // desired Unbinned point (parent coords)
			    const psImage *in, // binned image
			    const psImageBinning *binning   //!< Overhang
    );

double psImageInterpolatePixelBilinear (const double xIn, const double yIn, const psImage *in);

/// @}
#endif

# if (0)
/* old version */
double psImageUnbinPixel(const int ix, const int iy, //!< desired Unbinned point
                         const psImage *in, //!< binned image
			 const psImageBinning *binning ///< binning definition
                        );
# endif

