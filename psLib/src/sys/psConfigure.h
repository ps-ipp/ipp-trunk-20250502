/** @file  psConfigure.h
 *
 *  @brief Functions to init and cleanup psLib
 *
 *  These functions initalize psLib data before the beginning of a run and
 *  remove (finalize) the same data after the run is complete.  A function is
 *  also provided to return the current psLib version.
 *
 *
 *  @author Ross Harman, MHPCC
 *  @author Robert DeSonia, MHPCC
 *
 *  @version $Revision: 1.12 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2007-06-22 02:28:48 $
 *
 *  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 */

#ifndef PS_CONFIGURE_H
#define PS_CONFIGURE_H

#include <psString.h>

/// @addtogroup SysUtils System Utilities
/// @{

/** Get current psLib version
 *
 *  Returns the current psLib version name as a string.
 *
 *  @return psString: String with version name.
 */
psString psLibVersion(void);

/** Get current psLib revision number
 *
 *  Returns the current psLib revision number as a string.
 *
 *  @return psString: String with revision number.
 */
psString psLibRevision(void);

/** Get current psLib source
 *
 * Returns the current psLib source name as a string.
 *
 * @return psString: String with source name.
 */
psString psLibSource(void);

/** Get psLib dependencies' versions
 *
 * Returns the psLib dependency versions as a string.
 *
 * @return psString: String with dependencies.
 */
psString psLibDependencies(void);

/** Get current psLib version (full identification)
 *
 *  Returns the current psLib version name and other information identifying the compilation.
 *
 *  @return psString: String with identity.
 */
psString psLibVersionLong(void);


/** Initializes persistent memory.
 *
 *  Creates persistant memory items used throughout psLib. Items created
 *  within this method should be freed with the psLibFinalize function.
 *  current, a non-NULL psErr is returned with code PS_ERR_NONE.
 *
 */
bool psLibInit(
    const char* timeConfig           ///< Filename of config file for psTime.
);


/** Removes persistant memory created with the psLibInit function.
 *
 *  The memory created but not freed by psLib modules should be freed
 *  within this function at the end of a psLib execution cycle.
 *
 *  @return void: void.
 */
void psLibFinalize(
    void
);


// Check the memory; intended for use on exit, but might be used elsewhere
void p_psMemoryCheck(void);


/// @}
#endif // #ifndef PS_CONFIGURE_H
