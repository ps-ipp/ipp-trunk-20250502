/* @file  psScalar.h
 * @brief Basic scalar definitions and operations
 *
 * This file defines the basic type for a scalar struct and functions useful
 * in manupulating scalars.
 *
 * @author Ross Harman, MHPCC
 * @author Joshua Hoblitt, University of Hawaii
 *
 * @version $Revision: 1.25 $ $Name: not supported by cvs2svn $
 * @date $Date: 2008-08-05 01:23:59 $
 * Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 */

#ifndef PS_SCALAR_H
#define PS_SCALAR_H

/// @addtogroup MathOps Mathematical Operations
/// @{

#include "psType.h"

# define PS_SCALAR_VALUE(DATA,TYPE) (((psScalar *)(DATA))->data.TYPE)

/** Basic scalar data structure.
 *
 * Struct for maintaining a scalar of frequently used primitive types.
 *
 */
typedef struct
{
    psMathType type;            ///< Type of data.

    union {
        psS8 S8;                ///< Signed 8-bit integer data.
        psS16 S16;              ///< Signed 16-bit integer data.
        psS32 S32;              ///< Signed 32-bit integer data.
        psS64 S64;              ///< Signed 64-bit integer data.
        psU8 U8;                ///< Unsigned 8-bit integer data.
        psU16 U16;              ///< Unsigned 16-bit integer data.
        psU32 U32;              ///< Unsigned 32-bit integer data.
        psU64 U64;              ///< Unsigned 64-bit integer data.
        psF32 F32;              ///< Single-precision float data.
        psF64 F64;              ///< Double-precision float data.
    } data;                     ///< Union for data types.
}
psScalar;

/*****************************************************************************/

/* FUNCTION PROTOTYPES                                                       */

/*****************************************************************************/

/** Allocate a scalar.
 *
 * Uses psLib memory allocation functions to create scalar data as defined by the psType type.
 * Accepts a double for input value, as max size, but resizes according to correct type.
 *
 * @return psScalar*   Pointer to a new psScalar.
 */
#ifdef DOXYGEN
psScalar* psScalarAlloc(
    double value,                       ///< Data to be put into psScalar
    psElemType type                     ///< Type of data to be held by psScalar
);
#else // ifdef DOXYGEN
psScalar* p_psScalarAlloc(
    const char *file,                   ///< File of caller
    unsigned int lineno,                ///< Line number of caller
    const char *func,                   ///< Function name of caller
    double value,                       ///< Data to be put into psScalar
    psElemType type                     ///< Type of data to be held by psScalar
) PS_ATTR_MALLOC;
#define psScalarAlloc(value, type) \
      p_psScalarAlloc(__FILE__, __LINE__, __func__, value, type)
#endif // ifdef DOXYGEN


/** Checks the type of a particular pointer.
 *
 *  Uses the appropriate deallocation function in psMemBlock to check the ptr datatype.
 *
 *  @return bool:       True if the pointer matches a psScalar structure, false otherwise.
 */
bool psMemCheckScalar(
    psPtr ptr                          ///< the pointer whose type to check
);


/** Copy a scalar.
 *
 * Uses psLib memory allocation functions to copy a scalar.
 *
 * @return psScalar*    A copy of the input scalar
 */
#ifdef DOXYGEN
psScalar* psScalarCopy(
    const psScalar *value               ///< Scalar to copy
);
#else // ifdef DOXYGEN
psScalar* p_psScalarCopy(
    const char *file,                   ///< File of caller
    unsigned int lineno,                ///< Line number of caller
    const char *func,                   ///< Function name of caller
    const psScalar *value               ///< Scalar to copy.
);
#define psScalarCopy(value) \
      p_psScalarCopy(__FILE__, __LINE__, __func__, value)
#endif // ifdef DOXYGEN


/// @}
#endif // #ifndef PS_SCALAR_H
