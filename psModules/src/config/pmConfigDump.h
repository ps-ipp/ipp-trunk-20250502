/*  @file pmConfigDump.h
 *  @brief Configuration dumping function
 *
 *  @author Paul Price, IfA
 *
 *  @version $Revision: 1.3 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2008-09-05 22:41:58 $
 *  Copyright 2005-2006 Institute for Astronomy, University of Hawaii
 */

#ifndef PM_CONFIG_DUMP_H
#define PM_CONFIG_DUMP_H

#include <pmConfig.h>
#include <pmFPA.h>

/// @addtogroup Config Configuration System
/// @{

/// Cull recipes from the configuration file, apart from the ones listed
bool pmConfigRecipesCull(pmConfig *config, ///< Configuration
                         const char *save ///< List of recipes to save, comma-separated
    );

/// Cull cameras from the configuration file, apart from the one in use
bool pmConfigCamerasCull(pmConfig *config, ///< Configuration
                         const char *additional ///< List of additional cameras to save, comma-separated
    );

/// Dump the configuration to a file
///
bool pmConfigDump(const pmConfig *config, ///< Configuration to dump
                  const char *filename    ///< Output file name
    );



/// @}
#endif
