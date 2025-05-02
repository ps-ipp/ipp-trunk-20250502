/* @file  pmFPAview.h
 * @brief Tools to manipulate the FPA structure elements.
 *
 * @author EAM, IfA
 * @author PAP, IfA
 *
 * @version $Revision: 1.16 $ $Name: not supported by cvs2svn $
 * @date $Date: 2009-02-06 02:31:24 $
 * Copyright 2004-2005 Institute for Astronomy, University of Hawaii
 */

#ifndef PM_FPA_FILE_FITS_IO_H
#define PM_FPA_FILE_FITS_IO_H

/// @addtogroup Camera Camera Layout
/// @{

/// Read an image into the current view
bool pmFPAviewReadFitsImage(const pmFPAview *view, ///< View specifying level of interest
                            pmFPAfile *file, ///< FPA file into which to read
                            pmConfig *config
                           );

/// Read a mask into the current view
bool pmFPAviewReadFitsMask(const pmFPAview *view, ///< View specifying level of interest
                           pmFPAfile *file, ///< FPA file into which to read
                            pmConfig *config
                          );
/// Read a variance map into the current view
bool pmFPAviewReadFitsVariance(const pmFPAview *view,  ///< View specifying level of interest
                             pmFPAfile *file, ///< FPA file into which to read
                            pmConfig *config
                            );

/// Read a dark into the current view
bool pmFPAviewReadFitsDark(const pmFPAview *view,  ///< View specifying level of interest
                           pmFPAfile *file, ///< FPA file into which to read
                            pmConfig *config
    );

/// Read an image header into the current view
bool pmFPAviewReadFitsHeaderSet(const pmFPAview *view,  ///< View specifying level of interest
                                pmFPAfile *file, ///< FPA file into which to read
                            pmConfig *config
    );

/// Write the image for the specified view
bool pmFPAviewWriteFitsImage(const pmFPAview *view, ///< View specifying level of interest
                             pmFPAfile *file, ///< FPA file to write
                             pmConfig *config ///< Configuration
                            );

/// Write the mask for the specified view
bool pmFPAviewWriteFitsMask(const pmFPAview *view, ///< View specifying level of interest
                            pmFPAfile *file, ///< FPA file to write
                            pmConfig *config ///< Configuration
                           );

/// Write the variance map for the specified view
bool pmFPAviewWriteFitsVariance(const pmFPAview *view, ///< View specifying level of interest
                              pmFPAfile *file, ///< FPA file to write
                              pmConfig *config ///< Configuration
                             );

/// Write the dark for the specified view
bool pmFPAviewWriteFitsDark(const pmFPAview *view, ///< View specifying level of interest
                            pmFPAfile *file, ///< FPA file to write
                            pmConfig *config ///< Configuration
    );

/// Write a PHU for a fits image if needed
bool pmFPAviewFitsWritePHU (const pmFPAview *view, pmFPAfile *file, pmConfig *config);

/// Free the data for the specified view
bool pmFPAviewFreeData(const pmFPAview *view, ///< View specifying level of interest
                       pmFPAfile *file  ///< FPA file to free data
                      );

/// Read a table into the current view
bool pmFPAviewReadFitsTable(const pmFPAview *view, ///<  View specifying level of interest
                            pmFPAfile *file, ///< FPA file into which to read
                            const char *name ///< Name of table
                           );

/// Write the table for the specified view
bool pmFPAviewWriteFitsTable(const pmFPAview *view, ///<  View specifying level of interest
                             pmFPAfile *file, ///< FPA file to write
                             const char *name, ///< Name of table
                             pmConfig *config ///< Configuration
                            );

/// Produce a suitable FPA for writing, on the basis of the input FPAfile
///
/// A new FPA with a changed format is generated if required (file->format is set and file->camera is equal to
/// the default, indicating a change in the format without changing the camera --- changes to the camera are
/// handled using other systems --- see pmFPAfileDefineChipMosaic, pmFPAfileDefineFPAMosaic).  Otherwise the
/// file->fpa is returned (incremented).
pmFPA *pmFPAfileSuitableFPA(const pmFPAfile *file,///< File containing the fpa
                            const pmFPAview *view, ///< View at which to produce the fpa
                            pmConfig *config, ///< Configuration
                            bool pixels ///< Worry about copying the pixels?
                           );

/// @}

# endif
