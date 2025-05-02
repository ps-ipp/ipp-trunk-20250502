/** @file  psType.h
*
*  @brief Contains support for basic types
*
*  This file defines common datatypes used throughout psLib.
*
*  @ingroup DataContainer
*
*  @author Robert DeSonia, MHPCC
*  @author Ross Harman, MHPCC
*
*  @version $Revision: 1.63 $ $Name: not supported by cvs2svn $
*  @date $Date: 2009-01-27 06:39:38 $
*
*  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
*/

#ifndef PS_TYPE_H
#define PS_TYPE_H

/// @addtogroup DataContainer Data Containers
/// @{

#include <inttypes.h> // According to C99, this includes stdint.h
#include <float.h>
#include <stdbool.h>

// Make sure we have INFINITY and NAN
// Use GSL --- they solved the problem so we don't have to
#ifndef INFINITY
#include <gsl/gsl_nan.h>
#define INFINITY GSL_POSINF
#endif
#ifndef NAN
#include <gsl/gsl_sys.h>
#include <gsl/gsl_nan.h>
#define NAN GSL_NAN
#endif

/******************************************************************************/

/*  TYPE DEFINITIONS                                                          */

/******************************************************************************/

/** Basic data types used by the containers.
 *
 * The basic types of the primitives used by psLib are defined within this enum. This enum is in turn used by
 * the psType struct.
 *
 */

typedef uint8_t psU8;                  ///< 8-bit unsigned int
typedef uint16_t psU16;                ///< 16-bit unsigned int
typedef uint32_t psU32;                ///< 32-bit unsigned int
typedef uint64_t psU64;                ///< 64-bit unsigned int
typedef int8_t psS8;                   ///< 8-bit signed int
typedef int16_t psS16;                 ///< 16-bit signed int
typedef int32_t psS32;                 ///< 32-bit signed int
typedef int64_t psS64;                 ///< 64-bit signed int
typedef float psF32;                   ///< 32-bit floating point
typedef double psF64;                  ///< 64-bit floating point
typedef char* psString;                ///< string value
typedef void* psPtr;                   ///< void pointer

// XXX psBool can't be removed until all macros that 'generate' this type are
// fixed -JH
// #ifdef __GNUC__
// typedef bool psBool __attribute__ ((deprecated)); ///< boolean value
//#else // ifdef __GNUC__
typedef bool psBool;                   ///< boolean value
// #endif // fdef __GNUC__
typedef bool psBOOL;                 ///< allow psBOOL to be used instead of psBool (for macros)

/** Enumeration of data types for function elements.
 *  Contains replacements for native types.
 */
typedef enum {
    PS_TYPE_S8   = 0x0101,             ///< Character.
    PS_TYPE_S16  = 0x0102,             ///< Short integer.
    PS_TYPE_S32  = 0x0104,             ///< Integer.
    PS_TYPE_S64  = 0x0108,             ///< Long integer.
    PS_TYPE_U8   = 0x0301,             ///< Unsigned character.
    PS_TYPE_U16  = 0x0302,             ///< Unsigned psS16 integer.
    PS_TYPE_U32  = 0x0304,             ///< Unsigned integer.
    PS_TYPE_U64  = 0x0308,             ///< Unsigned psS64 integer.
    PS_TYPE_F32  = 0x0404,             ///< Single-precision Floating point.
    PS_TYPE_F64  = 0x0408,             ///< Double-precision floating point.
    PS_TYPE_BOOL = 0x1301              ///< Boolean.
} psElemType;

/** Enumeration primarily used with metadata which defines a data structure
 *  e.g., list, array, FITS file, etc.
*/
typedef enum {
    PS_DATA_S8   = PS_TYPE_S8,         ///< psS8
    PS_DATA_S16  = PS_TYPE_S16,        ///< psS16
    PS_DATA_S32  = PS_TYPE_S32,        ///< psS32
    PS_DATA_S64  = PS_TYPE_S64,        ///< psS64
    PS_DATA_U8   = PS_TYPE_U8,         ///< psU8
    PS_DATA_U16  = PS_TYPE_U16,        ///< psU16
    PS_DATA_U32  = PS_TYPE_U32,        ///< psU32
    PS_DATA_U64  = PS_TYPE_U64,        ///< psU64
    PS_DATA_F32  = PS_TYPE_F32,        ///< psF32
    PS_DATA_F64  = PS_TYPE_F64,        ///< psF64
    PS_DATA_BOOL = PS_TYPE_BOOL,       ///< psBool
    PS_DATA_STRING = 0x10000,          ///< psString (char *)
    PS_DATA_ARRAY,                     ///< psArray
    PS_DATA_BITS,                      ///< psBits
    PS_DATA_CUBE,                      ///< psCube
    PS_DATA_FITS,                      ///< psFits
    PS_DATA_HASH,                      ///< psHash
    PS_DATA_HISTOGRAM,                 ///< psHistogram
    PS_DATA_IMAGE,                     ///< psImage
    PS_DATA_KERNEL,                    ///< psKernel
    PS_DATA_LINE,                      ///< psLine
    PS_DATA_LIST,                      ///< psList
    PS_DATA_LOOKUPTABLE,               ///< psLookupTable
    PS_DATA_METADATA,                  ///< psMetadata
    PS_DATA_METADATAITEM,              ///< psMetadataItem
    PS_DATA_MINIMIZATION,              ///< psMinimization
    PS_DATA_PIXELS,                    ///< psPixels
    PS_DATA_PLANE,                     ///< psPlane
    PS_DATA_PLANEDISTORT,              ///< psPlaneDistort
    PS_DATA_PLANETRANSFORM,            ///< psPlaneTransform
    PS_DATA_POLYNOMIAL1D,              ///< psPolynomial1D
    PS_DATA_POLYNOMIAL2D,              ///< psPolynomial2D
    PS_DATA_POLYNOMIAL3D,              ///< psPolynomial3D
    PS_DATA_POLYNOMIAL4D,              ///< psPolynomial4D
    PS_DATA_PROJECTION,                ///< psProjection
    PS_DATA_REGION,                    ///< psRegion
    PS_DATA_SCALAR,                    ///< psScalar
    PS_DATA_SPHERE,                    ///< psSphere
    PS_DATA_SPHEREROT,                 ///< psSphereTransform
    PS_DATA_SPLINE1D,                  ///< psSpline1D
    PS_DATA_STATS,                     ///< psStats
    PS_DATA_TIME,                      ///< psTime
    PS_DATA_VECTOR,                    ///< psVector
    PS_DATA_UNKNOWN,                   ///< Other data of an unknown type
    PS_DATA_METADATA_MULTI             ///< Used internally for metadata; not a 'real' type
} psDataType;

// macros to abstract the generic mask type : these values must be consistent
#define PS_TYPE_MASK PS_TYPE_U8        /**< the psElemType to use for mask image */
#define PS_TYPE_MASK_DATA U8           /**< the data member to use for mask image */
#define PS_TYPE_MASK_NAME "psU8"       /**< the data type for mask as a string */
#define PS_MIN_MASK_TYPE 0             /**< minimum valid Mask value */
#define PS_MAX_MASK_TYPE UINT8_MAX     /**< maximum valid Mask value */
typedef psU8 psMaskType;               ///< the C datatype for a mask image
#define PS_NOT_MASK(A)(UINT8_MAX-(A))

// alternate versions if needed
// #define PS_NOT_MASK(A)(UINT16_MAX-(A))
// #define PS_NOT_MASK(A)(UINT32_MAX-(A))
// #define PS_NOT_MASK(A)(UINT64_MAX-(A))

// macros to abstract the vector mask type : these values must be consistent
#define PS_TYPE_VECTOR_MASK PS_TYPE_U8        /**< the psElemType to use for mask image */
#define PS_TYPE_VECTOR_MASK_DATA U8           /**< the data member to use for mask image */
#define PS_TYPE_VECTOR_MASK_NAME "psU8"       /**< the data type for mask as a string */
#define PS_MIN_VECTOR_MASK_TYPE 0             /**< minimum valid Vector Mask value */
#define PS_MAX_VECTOR_MASK_TYPE UINT8_MAX     /**< maximum valid Vector Mask value */
typedef psU8 psVectorMaskType;                    ///< the C datatype for a mask image
#define PS_NOT_VECTOR_MASK(A)(UINT8_MAX-(A))

// macros to abstract the image mask type : these values must be consistent
#define PS_TYPE_IMAGE_MASK PS_TYPE_U16       /**< the psElemType to use for mask image */
#define PS_TYPE_IMAGE_MASK_DATA U16          /**< the data member to use for mask image */
#define PS_TYPE_IMAGE_MASK_NAME "psU16"      /**< the data type for mask as a string */
#define PS_MIN_IMAGE_MASK_TYPE 0             /**< minimum valid Image Mask value */
#define PS_MAX_IMAGE_MASK_TYPE UINT16_MAX    /**< maximum valid Image Mask value */
typedef psU16 psImageMaskType;               ///< the C datatype for a mask image
#define PS_NOT_IMAGE_MASK(A)(UINT16_MAX-(A))

#define PS_MIN_S8        INT8_MIN      /**< minimum valid psS8 value */
#define PS_MIN_S16       INT16_MIN     /**< minimum valid psS16 value */
#define PS_MIN_S32       INT32_MIN     /**< minimum valid psS32 value */
#define PS_MIN_S64       INT64_MIN     /**< minimum valid psS64 value */
#define PS_MIN_U8        0             /**< minimum valid psU8 value */
#define PS_MIN_U16       0             /**< minimum valid psU16 value */
#define PS_MIN_U32       0             /**< minimum valid psU32 value */
#define PS_MIN_U64       0             /**< minimum valid psU64 value */
#define PS_MIN_F32       -FLT_MAX      /**< minimum valid psF32 value */
#define PS_MIN_F64       -DBL_MAX      /**< minimum valid psF64 value */

#define PS_MAX_S8        INT8_MAX      /**< maximum valid psS8 value */
#define PS_MAX_S16       INT16_MAX     /**< maximum valid psS16 value */
#define PS_MAX_S32       INT32_MAX     /**< maximum valid psS32 value */
#define PS_MAX_S64       INT64_MAX     /**< maximum valid psS64 value */
#define PS_MAX_U8        UINT8_MAX     /**< maximum valid psU8 value */
#define PS_MAX_U16       UINT16_MAX    /**< maximum valid psU16 value */
#define PS_MAX_U32       UINT32_MAX    /**< maximum valid psU32 value */
#define PS_MAX_U64       UINT64_MAX    /**< maximum valid psU64 value */
#define PS_MAX_F32       FLT_MAX       /**< maximum valid psF32 value */
#define PS_MAX_F64       DBL_MAX       /**< maximum valid psF64 value */

#define PS_TYPE_BOOL_NAME "psBool"
#define PS_TYPE_S8_NAME   "psS8"
#define PS_TYPE_S16_NAME  "psS16"
#define PS_TYPE_S32_NAME  "psS32"
#define PS_TYPE_S64_NAME  "psS64"
#define PS_TYPE_U8_NAME   "psU8"
#define PS_TYPE_U16_NAME  "psU16"
#define PS_TYPE_U32_NAME  "psU32"
#define PS_TYPE_U64_NAME  "psU64"
#define PS_TYPE_F32_NAME  "psF32"
#define PS_TYPE_F64_NAME  "psF64"

#define PS_TYPE_NAME(value,type) \
switch(type) { \
case PS_TYPE_BOOL: \
    value = PS_TYPE_BOOL_NAME; \
    break; \
case PS_TYPE_S8: \
    value = PS_TYPE_S8_NAME; \
    break; \
case PS_TYPE_S16: \
    value = PS_TYPE_S16_NAME; \
    break; \
case PS_TYPE_S32: \
    value = PS_TYPE_S32_NAME; \
    break; \
case PS_TYPE_S64: \
    value = PS_TYPE_S64_NAME; \
    break; \
case PS_TYPE_U8: \
    value = PS_TYPE_U8_NAME; \
    break; \
case PS_TYPE_U16: \
    value = PS_TYPE_U16_NAME; \
    break; \
case PS_TYPE_U32: \
    value = PS_TYPE_U32_NAME; \
    break; \
case PS_TYPE_U64: \
    value = PS_TYPE_U64_NAME; \
    break; \
case PS_TYPE_F32: \
    value = PS_TYPE_F32_NAME; \
    break; \
case PS_TYPE_F64: \
    value = PS_TYPE_F64_NAME; \
    break; \
default: \
    value = "unknown"; \
};

/// Macro to determine if the psElemType is an integer.
#define PS_IS_PSELEMTYPE_INT(x) ((x & 0x100) == 0x100)
/// Macro to determine if the psElemType is unsigned.
#define PS_IS_PSELEMTYPE_UNSIGNED(x) ((x & 0x200) == 0x200)
/// Macro to determine if the psElemType is a real floating-point type.
#define PS_IS_PSELEMTYPE_REAL(x) ((x & 0x400) == 0x400)
/// Macro to determine if the psElemType is boolean type.
#define PS_IS_PSELEMTYPE_BOOL(x) ((x & 0x1000) == 0x1000)
/// Macro to determine the storage size, in bytes, of the psElemType.
#define PSELEMTYPE_SIZEOF(x) (x & 0xFF)

#ifdef __GNUC__
#define PS_ATTR_MALLOC __attribute__((__malloc__))
#else // __GNUC__
#define PS_ATTR_MALLOC
#endif // __GNUC__

#ifdef __GNUC__
#define PS_ATTR_NORETURN __attribute__((noreturn))
#else // __GNUC__
#define PS_ATTR_NORETURN
#endif // __GNUC__

#ifdef __GNUC__
#define PS_ATTR_FORMAT(style, start, end) __attribute__((format(style, start, end)))
#else // __GNUC__
#define PS_ATTR_FORMAT(style, start, end)
#endif // __GNUC__

#ifdef __GNUC__
#define PS_ATTR_PURE __attribute__((pure))
#else // __GNUC__
#define PS_ATTR_PURE
#endif // __GNUC__

/** Dimensions of a data type.
 *
 * The dimensions of containers used by psLib are defined within this enum. This enum is used by the psType
struct. *
 */
typedef enum {
    PS_DIMEN_SCALAR,            ///< Scalar.
    PS_DIMEN_VECTOR,            ///< Vector.
    PS_DIMEN_TRANSV,            ///< Transposed vector.
    PS_DIMEN_IMAGE,             ///< Image.
    PS_DIMEN_OTHER              ///< Something else that's not supported for arithmetic.
} psDimen;

/** The type of a data type.
 *
 * All psLib complex types consist of primitive components. This struct provides the description of those
 * primitives.
 *
 */
typedef struct
{
    psElemType type;                   ///< The type
    psDimen dimen;                     ///< The dimensionality.
}
psMathType;

/** The type of a basic data type
 *
 *  All psLib complex types consist of primitive components.  This structure provides the ability to cast
 *  an unknown data structure to safely test the underlining data type.
 *
 */
typedef struct
{
    psMathType type;              ///< Data type information
}
psMath;

/** Checks the deallocator to see if the pointer matches the desired datatype.
 *
 *  @return bool:       True if type matches, otherwise false.
 */
bool psMemCheckType(
    psDataType type,                   ///< The desired psDataType to match
    psPtr ptr                          ///< The desired pointer to match
);

/// @}
#endif // #ifndef PS_TYPE_H
