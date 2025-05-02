/* @file  psSlurp.h
 * @brief read complete files into strings
 *
 * @author Joshua Hoblitt, University of Hawaii
 *
 * @version $Revision: 1.8 $ $Name: not supported by cvs2svn $
 * @date $Date: 2007-10-29 21:30:03 $
 */

#ifndef PS_SLURP_H
#define PS_SLURP_H

#include <psString.h>

# define PS_SLURP_GZIP 1

# if (PS_SLURP_GZIP) 
# include <zlib.h>
# endif

/// @addtogroup FileIO Input/Output
/// @{

// Read ("slurp") a file (specified by file descriptor)
// and return a string containing the entire file.
psString psSlurpFD(int fd               // File descriptor to read
                  );

// Read ("slurp") a file (specified by file stream)
// and return a string containing the entire file.
psString psSlurpFile(FILE *stream       // File stream to read
                    );

// Read ("slurp") a file (specified by filename)
// and return a string containing the entire file.
psString psSlurpFilename(const char *filename // Filename
                    );

# if (PS_SLURP_GZIP)
psString psSlurpGZIP(gzFile fd);
# endif

/// @}
#endif
