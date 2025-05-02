/** @file  psSlurp.c
 *
 *  @brief Contains functions for slurping files
 *
 *  @author Joshua Hoblitt, University of Hawaii
 *
 *  @version $Revision: 1.9 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2007-02-27 03:31:56 $
 *
 *  Copyright 2006 University of Hawaii
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "psType.h"
#include "psAssert.h"
#include "psError.h"
#include "psSlurp.h"
#include "psMemory.h"

# define SLURP_SIZE 4096

# if (PS_SLURP_GZIP) 

psString psSlurpFD(int fd) {

 gzFile file = gzdopen (fd, "r");
 if (file == Z_NULL) {
     psError(PS_ERR_IO, true, "Failed to open file\n");
     return NULL;
 }
 
 psString str = psSlurpGZIP(file);

 return str;
}

# else

psString psSlurpFD(int fd)
{
    psString str = NULL;                // String to which to write
    size_t size  = 1;                   // bytes allocated -  make sure there is room for '\0'
    size_t used = 0;                    // bytes actually used
    ssize_t bytes;                      // Number of bytes read
    do {
        // increase the allocated string size
        size += SLURP_SIZE;
        str = psStringRealloc(str, size);

        // read a block from the stream
        bytes = read(fd, str + used, SLURP_SIZE);
        if (bytes < 0) {
            // it's an error
            psError(PS_ERR_IO, true, "slurp failed on read");
            psFree(str);
            return NULL;
        }

        // Increase the size of the known string
        used += bytes;

    } while (bytes != 0);

    // append '\0' to the end of the string
    str[used] = '\0';

    return str;
}
# endif

# if (PS_SLURP_GZIP)
psString psSlurpGZIP(gzFile fd)
{
    psString str = NULL;                // String to which to write
    size_t size  = 1;                   // bytes allocated -  make sure there is room for '\0'
    size_t used = 0;                    // bytes actually used
    ssize_t bytes;                      // Number of bytes read
    do {
        // increase the allocated string size
        size += SLURP_SIZE;
        str = psStringRealloc(str, size);

        // read a block from the stream
        bytes = gzread(fd, str + used, SLURP_SIZE);
        if (bytes < 0) {
            // it's an error
            psError(PS_ERR_IO, true, "slurp failed on read");
            psFree(str);
            return NULL;
        }

        // Increase the size of the known string
        used += bytes;

    } while (bytes != 0);

    // append '\0' to the end of the string
    str[used] = '\0';

    return str;
}
# endif

psString psSlurpFile(FILE *stream)
{
    return psSlurpFD(fileno(stream));
}


psString psSlurpFilename(const char *filename)
{
    PS_ASSERT_PTR_NON_NULL(filename, NULL);
    
# if (PS_SLURP_GZIP)
    gzFile fd = gzopen(filename, "r");
    if (fd == Z_NULL) {
        psError(PS_ERR_IO, true, "Failed to open specified file, %s\n", filename);
        return NULL;
    }
    psString text = psSlurpGZIP(fd);

    if (gzclose(fd) != Z_OK) {
        psError(PS_ERR_IO, true, "Failed to close specified file, %s\n", filename);
        psFree(text);
        return NULL;
    }

# else

    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        psError(PS_ERR_IO, true, "Failed to open specified file, %s\n", filename);
        return NULL;
    }
    psString text = psSlurpFD(fd);

    if (close(fd) != 0) {
        psError(PS_ERR_IO, true, "Failed to close specified file, %s\n", filename);
        psFree(text);
        return NULL;
    }
# endif

    return text;
}
