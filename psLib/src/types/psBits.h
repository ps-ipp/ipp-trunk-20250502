/** @file  psBits.h
 *
 *  @brief Creates an array of bytes of arbitrary length for storing individual bits.
 *
 *  Bit masks are useful tools for toggling various flags and options. This set of functions module provides
 *  a mechanism to create an array of bits of arbitrary length and manipulate them with basic binary
 *  operations. A print function is also provided to display the entire set of bits in binary format as a
 *  string.
 *
 *  @author PAP, EAM, IfA
 *  @author Ross Harman, MHPCC
 *
 *  @version $Revision: 1.32 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2008-08-14 03:18:41 $
 *
 *  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 */

#ifndef PSBITS_H
#define PSBITS_H

#include "psType.h"
#include "psMutex.h"

/// @addtogroup DataContainer Data Containers
/// @{

/******************************************************************************/
/*  TYPE DEFINITIONS                                                          */
/******************************************************************************/

/** Struct containing array of bytes to hold bit data and corresponding array length.
 *
 *  The bits in the struct are assembled in as an array of bytes with eight bits per
 *  byte. The bits are arranged with the LSB in first (right most) position of the
 *  first array element.
 */
typedef struct
{
    long n;                             ///< Number of bits in the array
    psU8 *bits;                         ///< Aray of bytes holding bits
    psMutex lock;                       ///< Optional lock for thread safety
} psBits;

/*****************************************************************************/
/* FUNCTION PROTOTYPES                                                       */
/*****************************************************************************/

/** Checks the type of a particular pointer.
 *
 *  Uses the appropriate deallocation function in psMemBlock to check the ptr
 *  datatype.
 *
 *  @return bool:     True if the pointer matches a psBits structure, false otherwise.
 */
bool psMemCheckBits(
    psPtr ptr                          ///< the pointer whose type to check
);


/** Allocate a psBits.
 *
 *  Create a psBits with the number of bits specified by the user. All bits
 *  are set to zero upon allocation.
 *
 *  @return  psBits* : Pointer to struct containing array of bits and size of array.
 */
#ifdef DOXYGEN
psBits* psBitsAlloc(
    long nalloc                        ///< Number of bits in psBits array
);
#else // ifdef DOXYGEN
psBits* p_psBitsAlloc(
    const char *file,                   ///< File of caller
    unsigned int lineno,                ///< Line number of caller
    const char *func,                   ///< Function name of caller
    long nalloc                        ///< Number of bits in psBits array
) PS_ATTR_MALLOC;
#define psBitsAlloc(nalloc) \
      p_psBitsAlloc(__FILE__, __LINE__, __func__, nalloc)
#endif // ifdef DOXYGEN




/** Set a bit.
 *
 *  Sets a bit at a given bit location. The bit is set based on a zero index
 *  with the first bit set in the zero bit slot of the zero element of the byte
 *  array. As an example, setting bit 3 in an array with two elements would
 *  result in an psBits that looks like 00000000 00001000.
 *
 *  @return  bool : Successful operation?
 */
bool psBitsSet(
    psBits* bits,                  ///< Pointer to psBits to be set.
    long bit                           ///< Bit to be set.
);


/** Clear a bit.
 *
 *  Clear a bit at a given bit location. The bit is cleared based on a zero
 *  index with the first bit set in the zero bit slot of the zero element of
 *  the byte array.
 *
 *  @return  bool : Successful operation?
 */
bool psBitsClear(
    psBits* bits,                  ///< Pointer to psBits to be cleared.
    long bit                           ///< Bit to be cleared.
);


/** Test the value of a bit.
 *
 *  Prints the value of a bit at a given bit location, either one or zero. The
 *  resulting bit is based on a zero index format with the first bit set in the
 *  zero bit slot of the zero element of the byte array.  As an example,
 *  testing bit 3 in a psBits with two bytes that looks like 00000000
 *  00001000 would return a value of one, since that is the value that was set.
 *
 *  @return  bool:      True if successful, otherwise false
 */
bool psBitsTest(
    const psBits* bits,            ///< Pointer psBits to be tested.
    long bit                           ///< Bit to be tested.
);


/** Perform a binary operation on two psBitss
 *
 *  Perform an AND, OR, or XOR on two psBitss. If the BitMasks are not the
 *  same size, the operation will not be performed and an error message will be
 *  logged.
 *
 *  @return  psBits* : Pointer to struct containing result of binary operation.
 */
psBits* psBitsOp(
    psBits* outBits,               ///< Resulting psBits from binary operation
    const psBits* inBits1,         ///< First psBits on which to operate
    const char *operator,              ///< Bit operation
    const psBits* inBits2          ///< Second psBits on which to operate
);


/** Perform a not operation on a psBits
 *
 *  Toggles bits in a psBits. All zero bits are set to one and all one bits
 *  are set to zero.
 *
 *  @return  psBits* : Pointer to struct containing result of operation.
 */
psBits* psBitsNot(
    psBits* outBits,               ///< Resulting psBits from operation
    const psBits* inBits           ///< Input psBits
);


/** Convert the psBits to a string of ones and zeros.
 *
 *  Converts the contents of a psBits to a string representation of its
 *  binary form of ones and zeros. The LSB is the right-most chracter. Each set
 *  of eight characters represents one byte.
 *
 *  @return  psString:      Pointer to character array containing string data.
 */
psString psBitsToString(
    const psBits* bits             ///< psBits to convert
);

#define PS_ASSERT_BITS_NON_NULL(NAME, RVAL) \
if ((NAME) == NULL || (NAME)->bits == NULL || (NAME)->n < 0) { \
    psError(PS_ERR_BAD_PARAMETER_NULL, true, \
            "Unallowable operation: psBits %s or its data is NULL.", \
            #NAME); \
    return RVAL; \
} \

#define PS_ASSERT_BITS_VALID_BIT(NAME, BIT, RVAL)              \
    if (BIT < 0 || BIT >= (NAME)->n) {     \
    psError(PS_ERR_BAD_PARAMETER_VALUE, true, \
            "Unallowable operation: psBits %s or its data is NULL.", \
            #NAME); \
    return RVAL; \
} \



/// @}
#endif // #ifndef PSBITS_H
