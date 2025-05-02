/*  @file pmVersion.h
 *  @brief Version functions
 *
 *  @author Paul Price, IfA
 *  @author Eugene Magnier, IfA
 *
 *  @version $Revision: 1.3 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2007-03-30 21:12:56 $
 *  Copyright 2005-2006 Institute for Astronomy, University of Hawaii
 */

#ifndef PM_VERSION_H
#define PM_VERSION_H

/// @addtogroup Config Configuration System
/// @{

/** Get current psModules version
 *
 *  Returns the current psModules version name as a string.
 *
 *  @return psString: String with version name.
 */
psString psModulesVersion(void);

/** Get current psModules source
 *
 *  Returns the current psModules source as a string.
 *
 *  @return psString: String with source.
 */
psString psModulesSource(void);

/** Get current psModules version (full identification)
 *
 *  Returns the current psModules version name and other information identifying the compilation.
 *
 *  @return psString: String with identity.
 */
psString psModulesVersionLong(void);

/// Populate a header with version information
bool psModulesVersionHeader(
    psMetadata *header                  ///< Header to populate
    );


/// @}
#endif
