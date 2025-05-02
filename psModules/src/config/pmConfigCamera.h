/*  @file pmConfigCamera.h
 *  @brief Camera Configuration functions
 *
 *  @author Paul Price, IfA
 *  @author Eugene Magnier, IfA
 *
 *  @version $Revision: 1.6 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2008-08-06 03:40:45 $
 *  Copyright 2005-2006 Institute for Astronomy, University of Hawaii
 */

#ifndef PM_CONFIG_CAMERA_H
#define PM_CONFIG_CAMERA_H

/// @addtogroup Config Configuration System
/// @{

// Return the name of the original ("root") camera
//
// The root name is the name of the camera before it was made into a derivative (e.g., skycell, chip, fpa).
psString pmConfigCameraRootName(const char *name // Name of camera
    );

// Return the name of the Skycell derivative camera
psString pmConfigCameraSkycellName(const char *name // Name of camera
    );

// Return the name of the Chip derivative camera
psString pmConfigCameraChipName(const char *name // Name of camera
    );

// Return the name of the FPA derivative camera
psString pmConfigCameraFPAName(const char *name // Name of camera
    );

// Generate a skycell version of a camera configuration
bool pmConfigGenerateSkycellVersion(psMetadata *oldCameras, // Old list of camera configurations
                                    psMetadata *newCameras, // New list of camera configurations
                                    const char *name, // Name of original camera configuration
                                    const psMetadata *site // The site configuration
    );

/// Generate the skycell version of a particular camera configuration
bool pmConfigCameraSkycellVersion(psMetadata *site, // The site configuration
                                  const char *name // Name of the un-mosaicked camera
    );


/// Generate skycell versions of all the camera configurations
bool pmConfigCameraSkycellVersionsAll(psMetadata *site // Site configuration
    );

// Generate a mosaicked version of a camera configuration
bool pmConfigGenerateMosaickedVersion(psMetadata *oldCameras, // Old list of camera configurations
                                      psMetadata *newCameras, // New list of camera configurations
                                      const char *name, // Name of original camera configuration
                                      pmFPALevel mosaicLevel // Level to which we are mosaicking
    );

/// Generate the chip mosaicked version of a particular camera configuration
bool pmConfigCameraMosaickedVersions(psMetadata *site, // Site configuration
                                     const char *name // Name of the un-mosaicked camera
    );

/// Generate chip- and fpa-mosaicked versions of all the camera configurations
bool pmConfigCameraMosaickedVersionsAll(psMetadata *site // Site configuration
    );

/// @}
#endif
