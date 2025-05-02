/** @file  psLookupTable.h
*
*  @brief This file defines the structure and functions for table lookups.
*
*  @author PAP, IfA
*  @author Ross Harman, MHPCC
*
*  @version $Revision: 1.21 $ $Name: not supported by cvs2svn $
*  @date $Date: 2007-08-09 01:40:08 $
*
*  Copyright 2004 Maui High Performance Computing Center, University of Hawaii
*/

#ifndef PS_LOOKUPTABLE_H
#define PS_LOOKUPTABLE_H

/// @addtogroup DataContainer Data Containers
/// @{

#include "psType.h"
#include "psVector.h"
#include "psArray.h"
#include "psLogMsg.h"
#include "psString.h"

/** Lookup table structure
 *
 *  Holds table data read from external data files.
 *
 */
typedef struct {
    psString filename;                  ///< Name of file with table
    psString format;                    ///< scanf-like format string for file
    long indexCol;                     ///< Column of the index vector (starting at zero)
    psVector *index;                   ///< Vector of independent index values
    psArray *values;                   ///< Array of dependent table values corresponding to index values
    const double validFrom;            ///< Lower bound for rable read
    const double validTo;              ///< Upper bound for table read
} psLookupTable;

#define PS_ASSERT_LOOKUPTABLE_NON_NULL(NAME, RVAL) \
if (!(NAME) || !(NAME)->filename || strlen((NAME)->filename) == 0 || \
    !(NAME)->format || strlen((NAME)->format) == 0 || (NAME)->index < 0) { \
    psError(PS_ERR_BAD_PARAMETER_VALUE, true, \
            "Error: Lookup table %s or one of its components is NULL.", \
            #NAME); \
    return RVAL; \
}

/** Lookup table lookup status and error conditions
 *
 *  Success, failure, and status conditions for table lookups.
 */
typedef enum {
    PS_LOOKUP_SUCCESS             = 0x0000,        ///< Table lookup succeeded
    PS_LOOKUP_PAST_TOP            = 0x0101,        ///< Lookup off top of table
    PS_LOOKUP_PAST_BOTTOM         = 0x0102,        ///< Lookup off bottom of table
    PS_LOOKUP_ERROR               = 0x0104         ///< Any other type of lookup error
} psLookupStatusType;


/** Lookup table parse status and error conditions
 *
 *  Success, failure, and status conditions for table parsing.
 */
typedef enum {
    PS_PARSE_SUCCESS              = 0x0000,        ///< Table lookup succeeded
    PS_PARSE_ERROR_TYPE           = 0x0101,        ///< Error parsing type
    PS_PARSE_ERROR_VALUE          = 0x0102,        ///< Error parsing numerical value
    PS_PARSE_ERROR_GENERAL        = 0x0104         ///< Any other type of lookup error
}psParseErrorType;


/** Checks the type of a particular pointer.
 *
 *  Uses the appropriate deallocation function in psMemBlock to check the ptr
 *  datatype.
 *
 *  @return bool:       True if the pointer matches a psLookupTable structure,
 *  false otherwise.
 */
bool psMemCheckLookupTable(
    psPtr ptr                          ///< the pointer whose type to check
);


/** Allocator for psLookupTable struct
 *
 *  Allocates a new psLookupTable struct.
 *
 *  @return psLookupTable*     New psLookupTable struct.
 */
#ifdef DOXYGEN
psLookupTable* psLookupTableAlloc(
    const char *filename,              ///< Name of file to read
    const char *format,                ///< scanf-like format string
    long indexCol                      ///< Column of the index vector (starting at zero)
);
#else // ifdef DOXYGEN
psLookupTable* p_psLookupTableAlloc(
    const char *file,                   ///< File of caller
    unsigned int lineno,                ///< Line number of caller
    const char *func,                   ///< Function name of caller
    const char *filename,              ///< Name of file to read
    const char *format,                ///< scanf-like format string
    long indexCol                      ///< Column of the index vector (starting at zero)
) PS_ATTR_MALLOC;
#define psLookupTableAlloc(filename, format, indexCol) \
      p_psLookupTableAlloc(__FILE__, __LINE__, __func__, filename, format, indexCol)
#endif // ifdef DOXYGEN


/** Read vectors from file
 *
 *  Read numeric vectors from ASCII text file
 *
 *  @return psArray*  New array of psVectors corresponding to the columns of the table
 */
psArray *psVectorsReadFromFile(
    const char* filename,              ///< File to be read
    const char* format                 ///< scanf-like format string
);


/** Import arrays of vectors into a table
 *
 *  Import array of vectors read from text file into a table structure
 *
 *  @return psLookupTable*  Lookup table structure with array vector data
 */
#ifdef DOXYGEN
bool psLookupTableImport(
    psLookupTable *table,              ///< Lookup table into which to import
    const psArray *vectors,            ///< Array of vectors
    long indexCol                      ///< Index of the index vector in the array of vectors
);
#else // ifdef DOXYGEN
bool p_psLookupTableImport(
    const char *file,                   ///< File of caller
    unsigned int lineno,                ///< Line number of caller
    const char *func,                   ///< Function name of caller
    psLookupTable *table,               ///< Lookup table into which to import
    psArray *vectors,                   ///< Array of vectors
    long indexCol                       ///< Index of the index vector in the array of vectors
);
#define psLookupTableImport(table, vectors, indexCol) \
      p_psLookupTableImport(__FILE__, __LINE__, __func__, table, vectors, indexCol)
#endif // ifdef DOXYGEN


/** Read lookup table
 *
 *  Reads a lookup table and fills corresponding psLookupTable struct.
 *
 *  @return long:     Number of valid lines read
 */
long psLookupTableRead(
    psLookupTable *table               ///< Table to read
);


/** Lookup and interpolate value from table.
 *
 *  Interpolates value from table. Sets status bit for success or one of
 *  several possible failure conditions.
 *
 *  @return double     Interpolation value at index
 */
double psLookupTableInterpolate(
    const psLookupTable *table,        ///< Table with data
    double index,                      ///< Value to be interpolated
    long column                        ///< Column in table to be interpolated
);


/** Lookup and interpolate all values from table.
 *
 *  Interpolates all values from table. Sets status bit for success or one of
 *  several possible failure conditions.
 *
 *  @return psVector*     Interpolation values calculated at index
 */
psVector* psLookupTableInterpolateAll(
    const psLookupTable *table,         ///< Table with data
    double index                        ///< Value to be interpolated
);


/// @}
#endif // #ifndef PS_LOOKUPTABLE_H
