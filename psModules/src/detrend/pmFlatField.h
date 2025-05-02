/* @file pmFlatField.h
 * @brief Apply flat field calibration
 *
 * @author Ross Harman, MHPCC
 * @author Paul Price, IfA
 *
 * @version $Revision: 1.16 $ $Name: not supported by cvs2svn $
 * @date $Date: 2009-02-12 19:25:52 $
 * Copyright 2004-2006 Institute for Astronomy, University of Hawaii
 */

#ifndef PM_FLAT_FIELD_H
#define PM_FLAT_FIELD_H

/// @addtogroup detrend Detrend Creation and Application
/// @{

/// Apply flat field calibration to a readout
///
/// This function applies the flat field calibration to the input readout.  Support is available for different
/// image types, though the input and flat images must have the same type.  The relative offsets between the
/// input and flat images is determined from the readout row0,col0 and the CELL.X0 and CELL.Y0 concepts.
/// Normalisation of the flat is left as the responsibility of the caller.  Non-positive pixels in the flat
/// are masked, if there is a mask present in the input readout.
bool pmFlatField(pmReadout *in,         ///< Readout with input image
                 const pmReadout *flat,  ///< Readout with flat image
                 psImageMaskType badFlat     ///< Mask value to give bad flat pixels
                );

/// Thread entry point for flat-fielding
bool pmFlatFieldScan_Threaded(psThreadJob *job ///< Job to exectute
    );

/// Flat-field a scan
bool pmFlatFieldScan(
    psImage *inImage,                   ///< Input image to correct
    psImage *inMask,                    ///< Input mask image
    psImage *inVar,                     ///< Input variance image
    const psImage *flatImage,           ///< Flat-field image
    const psImage *flatMask,            ///< Flat-field mask
    psImageMaskType badFlag,            ///< Mask value to give bad pixels
    int xOffset, int yOffset,           ///< Offset between input and flat-field
    int rowStart, int rowStop           ///< Scan range
    );


/// @}
#endif
