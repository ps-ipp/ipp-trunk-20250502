/* @file pmFPALevel.h
 * @brief Defines enum and string representations for the FPA levels
 *
 * @author Eugene Magnier, IfA
 * @author Paul Price, IfA
 *
 * @version $Revision: 1.4 $ $Name: not supported by cvs2svn $
 * @date $Date: 2008-02-07 00:10:08 $
 * Copyright 2005-2008 Institute for Astronomy, University of Hawaii
 */

#ifndef PM_FPA_LEVEL_H
#define PM_FPA_LEVEL_H

/// @addtogroup Camera Camera Layout
/// @{

/// Specify the level of the FPA hierarchy
typedef enum {
    PM_FPA_LEVEL_NONE,                  ///< No particular level specified
    PM_FPA_LEVEL_FPA,                   ///< Level corresponds to an FPA
    PM_FPA_LEVEL_CHIP,                  ///< Level corresponds to a Chip
    PM_FPA_LEVEL_CELL,                  ///< Level corresponds to a Cell
    PM_FPA_LEVEL_READOUT,               ///< Level corresponds to a Readout
    PM_FPA_LEVEL_CHUNK,                 ///< Level corresponds to a chunk
} pmFPALevel;


/// Return the string representation of the FPA level
const char *pmFPALevelToName(pmFPALevel level ///< Level enum
                            );

/// Return the enum representation of the FPA level
pmFPALevel pmFPALevelFromName(const char *name ///< Level name
                             );
/// @}
#endif // #ifndef PM_FPA_LEVEL_H
