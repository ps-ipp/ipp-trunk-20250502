/* @file pmHDUGenerate.h
 * @brief Generate HDU pixels from FPA components that have pixels
 *
 * @author Paul Price, IfA
 *
 * @version $Revision: 1.4 $ $Name: not supported by cvs2svn $
 * @date $Date: 2007-03-30 21:12:56 $
 * Copyright 2005-2006 Institute for Astronomy, University of Hawaii
 */

#ifndef PM_HDU_GENERATE_H
#define PM_HDU_GENERATE_H

/// @addtogroup Camera Camera Layout
/// @{

/// Generate an HDU (with CELL.TRIMSEC, CELL.BIASSEC and pixels) for a cell with pixels
///
/// The write functions for the FPA hierarchy use pmHDUWrite, which assumes that the images in the readouts
/// are subimages of the pixels in the HDU structure.  If this is not the case, the HDU pixels can be
/// generated using some simple assumptions.  Splices the images and overscans together without regard for
/// CELL.X0 and CELL.Y0 (for a proper mosaic, see pmFPAMosaic), though it should respect CELL.READDIR (so that
/// the bias and trim sections match properly).  A warning may be generated after running this function if the
/// bias and trim sections are specified in the camera format by default values rather than in the header.
/// Failure of this function is often due to a bad camera format file.
bool pmHDUGenerateForCell(pmCell *cell  ///< The cell for which to generate an HDU
                         );

/// Generate an HDU (with CELL.TRIMSEC, CELL.BIASSEC and pixels) for a cell with pixels
///
/// The write functions for the FPA hierarchy use pmHDUWrite, which assumes that the images in the readouts
/// are subimages of the pixels in the HDU structure.  If this is not the case, the HDU pixels can be
/// generated using some simple assumptions.  Splices the images and overscans together without regard for
/// CELL.X0 and CELL.Y0 (for a proper mosaic, see pmFPAMosaic), though it should respect CELL.READDIR (so that
/// the bias and trim sections match properly).  A warning may be generated after running this function if the
/// bias and trim sections are specified in the camera format by default values rather than in the header.
/// Failure of this function is often due to a bad camera format file.
bool pmHDUGenerateForChip(pmChip *chip  ///< The chip for which to generate an HDU
                         );

// Generate an HDU (with CELL.TRIMSEC, CELL.BIASSEC and pixels) from an FPA with pixels
///
/// The write functions for the FPA hierarchy use pmHDUWrite, which assumes that the images in the readouts
/// are subimages of the pixels in the HDU structure.  If this is not the case, the HDU pixels can be
/// generated using some simple assumptions.  Splices the images and overscans together without regard for
/// CELL.X0 and CELL.Y0 (for a proper mosaic, see pmFPAMosaic), though it should respect CELL.READDIR (so that
/// the bias and trim sections match properly).  A warning may be generated after running this function if the
/// bias and trim sections are specified in the camera format by default values rather than in the header.
/// Failure of this function is often due to a bad camera format file.
bool pmHDUGenerateForFPA(pmFPA *fpa     ///< The fpa for which to generate an HDU
                        );
/// @}
#endif
