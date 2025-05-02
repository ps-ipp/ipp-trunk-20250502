/** @file  psBits.c
 *
 *  @brief Creates an array of bytes of arbitrary length for storing individual bits.
 *
 *  Bit masks are useful tools for toggling various flags and options. This set of functions module provides
 *  a mechanism to create an array of bits of arbitrary length and manipulate them with basic binary
 *  operations. A print function is also provided to display the entire set of bits in binary format as a
 *  string.
 *
 *  @author Ross Harman, MHPCC
 *  @author Robert DeSonia, MHPCC
 *
 *  @version $Revision: 1.42 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2007-08-09 03:30:16 $
 *
 *  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <math.h>

#include "psBits.h"
#include "psMemory.h"
#include "psError.h"
#include "psAbort.h"
#include "psString.h"
#include "psAssert.h"


static void bitsFree(psBits *inBits);

/** Private function to create a mask.
 *
 *  Creates an eight bit mask with the given bit set. All other bits in the byte are zero. The input bit uses
 *  zero-based indexing, and is the cumulitive index within the array, not the localized byte's bit position.
 *
 *  @return  char*: Pointer to byte in which bit is contained.
 */
PS_ATTR_PURE static inline char mask(long bit)
{
    psAssert(bit < 8, "Bad bit: %ld", bit);
    return (char)0x01 << (bit % 8);
}

// Return the byte with the bit of interest
static inline psU8 *bitsGetByte(const psBits *bits, long bit)
{
    return bits->bits + bit / 8;
}


static void bitsFree(psBits *inBits)
{
    psFree(inBits->bits);
}

bool psMemCheckBits(psPtr ptr)
{
    PS_ASSERT_PTR(ptr, false);
    return (psMemGetDeallocator(ptr) == (psFreeFunc)bitsFree);
}


psBits *p_psBitsAlloc(const char *file, unsigned int lineno, const char *func, long nalloc)
{
    psAssert(nalloc >= 0, "The number of bits in a psBits (%ld) must be greater than zero.", nalloc);

    int numBytes = ceil((float)nalloc / 8.0); // Number of bytes to use
    psBits *bits = p_psAlloc(file, lineno, func, sizeof(psBits));
    psMemSetDeallocator(bits, (psFreeFunc)bitsFree);
    bits->n = nalloc;

    bits->bits = p_psAlloc(file, lineno, func, numBytes);
    memset(bits->bits, 0, numBytes);

    return bits;
}


bool psBitsSet(psBits *bits, long bit)
{
    PS_ASSERT_BITS_NON_NULL(bits, false);
    PS_ASSERT_BITS_VALID_BIT(bits, bit, false);

    psU8 *byte = bitsGetByte(bits, bit);  // Byte with the bit of interest
    *byte |= mask(bit);

    return true;
}

bool psBitsClear(psBits *bits, long bit)
{
    PS_ASSERT_BITS_NON_NULL(bits, false);
    PS_ASSERT_BITS_VALID_BIT(bits, bit, false);

    psU8 *byte = bitsGetByte(bits, bit);  // Byte with the bit of interest
    *byte &= !mask(bit);

    return true;
}


bool psBitsTest(const psBits *bits, long bit)
{
    // XXX These errors probably cannot be caught
    PS_ASSERT_BITS_NON_NULL(bits, false);
    PS_ASSERT_BITS_VALID_BIT(bits, bit, false);

    psU8 *byte = bitsGetByte(bits, bit);  // Byte with the bit of interest
    return ((*byte & mask(bit)) != 0);
}

psBits *psBitsOp(psBits *outBits, const psBits *bits1, const char *operator, const psBits *bits2)
{
    PS_ASSERT_BITS_NON_NULL(bits1, NULL);
    PS_ASSERT_STRING_NON_EMPTY(operator, NULL);

    enum {
        UNKNOWN_OP,
        AND_OP,
        OR_OP,
        XOR_OP,
        NOT_OP
    } op = UNKNOWN_OP;

    // Parse the operator
    if (strcmp(operator, "AND") == 0) {
        op = AND_OP;
    } else if (strcmp(operator, "OR") == 0) {
        op = OR_OP;
    } else if (strcmp(operator, "XOR") == 0) {
        op = XOR_OP;
    } else if (strcmp(operator, "NOT") == 0) {
        op = NOT_OP;
    } else {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                _("Specified operator, %s, is invalid.  Valid operators are AND, OR, XOR, NOT."),
                operator);
        return NULL;
    }

    if (op != NOT_OP) {
        PS_ASSERT_BITS_NON_NULL(bits2, false);
        if (bits1->n != bits2->n) {
            psError(PS_ERR_BAD_PARAMETER_SIZE, true,
                    "The psBits operands must be the same size: %ld vs %ld",
                    bits1->n, bits2->n);
            return NULL;
        }
    }

    int numBits = bits1->n;                   // Number of bits
    int numBytes = ceil((float)numBits / 8.0); // Number of bytes

    if (!outBits) {
        outBits = psBitsAlloc(numBits);
    } else if (outBits->n != numBits) {
        outBits->n = numBits;
        outBits->bits = psRealloc(outBits->bits, numBytes);
    }

    psU8 *in1 = bits1->bits;                  // Bits for input 1
    psU8 *in2 = (op == NOT_OP) ? NULL : bits2->bits; // Bits for input 2
    psU8 *out = outBits->bits;                       // Bits for output

    switch (op) {
    case AND_OP:
        for (int i = 0; i < numBytes; i++) {
            out[i] = in1[i] & in2[i];
        }
        break;
    case OR_OP:
        for (int i = 0; i < numBytes; i++) {
            out[i] = in1[i] | in2[i];
        }
        break;
    case XOR_OP:
        for (int i = 0; i < numBytes; i++) {
            out[i] = in1[i] ^ in2[i];
        }
        break;
    case NOT_OP:
    default:
        for (int i = 0; i < numBytes; i++) {
            out[i] = ~in1[i];
        }
        break;
    }

    return outBits;
}

psBits *psBitsNot(psBits *outBits, const psBits *inBits)
{
    return psBitsOp(outBits, inBits, "NOT", NULL);
}

psString psBitsToString(const psBits *bits)
{
    PS_ASSERT_BITS_NON_NULL(bits, NULL);

    psS32 numBits = bits->n;
    psString string = psStringAlloc(numBits + 1);

    char *out = &string[numBits];
    *out = '\0';
    out--;
    for (long i = 0; i < numBits; i++, out--) {
        *out = psBitsTest(bits, i) ? '1' : '0';
    }

    return string;
}
