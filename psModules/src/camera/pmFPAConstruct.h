/* @file pmFPAConstruct.h
 * @brief Functions to create an FPA, and add data sources to it.
 *
 * @author Paul Price, IfA
 *
 * @version $Revision: 1.11 $ $Name: not supported by cvs2svn $
 * @date $Date: 2008-06-05 01:31:33 $
 * Copyright 2005-2006 Institute for Astronomy, University of Hawaii
 */

#ifndef PM_FPA_CONSTRUCT_H
#define PM_FPA_CONSTRUCT_H

/// @addtogroup Camera Camera Layout
/// @{

/// Construct an FPA instance on the basis of a camera configuration
///
/// This is the function that creates the FPA hierarchy on the basis of the camera configuration.  The "FPA"
/// entry in the camera configuration specifies the chips (each of type STR) with their component cells listed
/// as the corresponding values (whitespace separated).  The FPA hierarchy is created devoid of any
/// input/output sources (i.e., HDUs).
pmFPA *pmFPAConstruct(const psMetadata *camera, ///< The camera configuration
                      const char *cameraName ///< Name of the camera (for FPA.CAMERA concept)
                     );

/// Add a source to the focal plane hierarchy, specified by a camera format
///
/// This is suitable for generating an output FPA given the desired format.
bool pmFPAAddSourceFromFormat(pmFPA *fpa, ///< The FPA
                              const psMetadata *format ///< Format of file
    );

/// Add an (input or output) source to the focal plane hierarchy, specified by a view
///
/// Given an FPA, add an HDU by specifying where it goes (i.e., by an FPAview).  The camera format
/// configuration is required in order to describe how the FPA is laid out in terms of disk files.
bool pmFPAAddSourceFromView(pmFPA *fpa,   ///< The FPA
                            const pmFPAview *phuView, ///< The view, corresponding to the PHU
                            const psMetadata *format ///< Format of file
                           );

/// Add an (input or output) source to the focal plane hierarchy, specified by a (primary) header
///
/// Given an FPA, add an HDU by specifying a primary header, which is used to determine the FITS file
/// contents, and therefore the proper location for the HDU.  The camera format configuration is required in
/// order to describe how the FPA is laid out in terms of disk files.
pmFPAview *pmFPAAddSourceFromHeader(pmFPA *fpa, ///< The FPA
                                    psMetadata *phu, ///< Primary header of file
                                    const psMetadata *format ///< Format of file
                                   );

/// Identify a source in the focal plane hierarchy, specified by a (primary) header
///
/// This is the same as pmFPAAddSourceFromHeader, except the input is not added into the FPA hierarchy.
/// This function serves only to identify where in the hierarchy it should go, not prepare for reading, etc.
pmFPAview *pmFPAIdentifySourceFromHeader(pmFPA *fpa, psMetadata *phu, const psMetadata *format);

/// Print a representation of the FPA, including its headers and concepts.
///
/// This function is intended for testing and development purposes.
void pmFPAPrint(FILE *fd,               ///< File descriptor to which to print
                const pmFPA *fpa,       ///< FPA to print
                bool header,            ///< Print headers?
                bool concepts           ///< Print concepts?
               );

/// Return the PHU level for an FPA, given the format
pmFPALevel pmFPAPHULevel(const psMetadata *format);

/// Return the Extensions level for an FPA, given the format
pmFPALevel pmFPAExtensionsLevel(const psMetadata *format);

/// @}

#endif
