/** @file  psLine.h
 *
 *  @brief charater-string fixed-length line functions
 *
 *  The psLine functions allow manipulation of fixed-length lines.
 *
 *  @author EAM, IFA
 *  @author Paul Price, IFA
 *  @author David Robbins, MHPCC
 *
 *  @version $Revision: 1.8 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2007-08-09 01:40:07 $
 *
 *  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 */

#ifndef PS_LINE_H
#define PS_LINE_H

/// @addtogroup SysUtils System Utilities
/// @{

/** Structure to carry a dynamic string */
typedef struct
{
    long NLINE;                        ///< allocated length
    long Nline;                        ///< current length
    psString line;                     ///< character string data
}
psLine;


/** Allocates a line object of length Nline.
 *
 *  @return psLine*:        the newly allocated line object.
*/
psLine *psLineAlloc(
    long Nline                         ///< length of line object to allocate
) PS_ATTR_MALLOC;


/** Checks the type of a particular pointer.
 *
 *  Uses the appropriate deallocation function in psMemBlock to check the ptr datatype.
 *
 *  @return bool:       True if the pointer matches a psLine structure, false otherwise.
 */
bool psMemCheckLine(
    psPtr ptr                          ///< the pointer whose type to check
);


/** Initializes or re-initializes a line.
 *
 *  Initializes or re-initializes a line, setting the current length to zero and setting
 *  the string data values to 0.  If the function is passed NULL, false is returned.
 *
 *  @return bool:       True if successful, otherwise false.
*/
bool psLineInit(
    psLine *line                       ///< line to (re-)initialize
);


/** Adds the line segment to the string.
 *
 *  Appends a line segment to the string, returning false if the new segment would
 *  overflow the allocated string length.
 *
 *  @return bool:        True if successful, otherwise false.
*/
bool psLineAdd(
    psLine *line,                      ///< the line segment to append
    const char *format,                ///< printf-style format of line
    ...                                ///< any parameters required in format
) PS_ATTR_FORMAT(printf, 2, 3);


/// @}
#endif /* PS_LINE_H */
