/* @file  psMD5.h
 * @brief support for MD5 hashes
 *
 * $Revision: 1.2 $ $Name: not supported by cvs2svn $
 * $Date: 2007-01-23 22:47:23 $
 * Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 */

#ifndef PS_MD5_H
#define PS_MD5_h

/// @addtogroup MathOps Mathematical Operations
/// @{

#include "psVector.h"
#include "psString.h"
#include "psImage.h"

/// Return an MD5 hash of the supplied string.
///
/// The MD5 hash is returned in a U8 vector of size 16.
psVector *psStringMD5(const char *string   ///< String to hash
                     );

/// Return an MD5 hash of the supplied vector.
///
/// The MD5 hash is returned in a U8 vector of size 16.
psVector *psVectorMD5(const psVector *vector ///< Vector to hash
                     );

/// Return an MD5 hash of the supplied image.
///
/// The MD5 hash is returned in a U8 vector of size 16.
psVector *psImageMD5(const psImage *image ///< Image to hash
                    );

/// Convert an MD5 hash into a string, for printing.
psString psMD5toString(const psVector *hash ///< Hash to stringify
                      );

/// @}
#endif
