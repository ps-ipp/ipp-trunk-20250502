/* @file  IOBuffer.h
 * @brief input/output character buffer
 *
 * @author EAM, IfA
 *
 * @version $Revision: 1.2 $ $Name: not supported by cvs2svn $
 * @date $Date: 2007-01-24 02:54:15 $
 * Copyright 2004-2005 Institute for Astronomy, University of Hawaii
 */

#ifndef PS_IO_BUFFER_H
#define PS_IO_BUFFER_H

/// @addtogroup Extras Miscellaneous Funtions
/// @{

typedef struct
{
    char *data;
    int nAlloc;    // current size of allocated buffer
    int nReset;    // size to set buffer after flush
    int nBlock;    // number of bytes to try to read at a time
    int n;    // current size of filled data
}
psIOBuffer;

// psIOBuffer functions
psIOBuffer *psIOBufferAlloc (int nBuffer);
bool psIOBufferFlush (psIOBuffer *buffer);
int psIOBufferRead (psIOBuffer *buffer, int fd);
int psIOBufferReadEmpty (psIOBuffer *buffer, int maxRetries, int fd);

/// @}
# endif
