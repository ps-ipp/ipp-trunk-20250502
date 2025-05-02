/*
** Copyright (C) 2008 Institute for Astronomy, University of Hawaii
**
** This is free software; you can redistribute it and/or
** modify it under the terms of the GNU Lesser General Public
** License as published by the Free Software Foundation; either
** version 2.1 of the License, or (at your option) any later version.
**
** The GNU C Library is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
** Lesser General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public
** License along with the GNU C Library; if not, write to the Free
** Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
** 02111-1307 USA.
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <assert.h>

#include "psAbort.h"
#include "psType.h"
#include "psError.h"
#include "psImage.h"
#include "psFits.h"
#include "psTrace.h"
#include "psMemory.h"

#include "psFitsFloat.h"


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Private functions
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

// This definition of IEEE754 floating-point, with some modifications, is from the GNU C Library, ieee754.h
// Copyright (C) 1992, 1995, 1996, 1999 Free Software Foundation, Inc.
union float_ieee754 {
    float f;                            // Floating point
    struct {                            // IEEE 754 single-precision format
#ifdef WORDS_BIGENDIAN
        unsigned int negative:1;
        unsigned int exponent:8;
        unsigned int mantissa:23;
#else
        unsigned int mantissa:23;
        unsigned int exponent:8;
        unsigned int negative:1;
#endif
    } ieee;
};
#define BIAS_FLOAT_IEEE754         0x7f // Exponent bias for IEEE754
// End definition from ieee754.h


// 16-bit floating point
union float_16_0 {
    psS16 s16;                          // 16-bit integer version
        struct {                        // Floating-point version
        unsigned int negative:1;        // Sign bit
        unsigned int exponent:5;        // Exponent bits
        unsigned int mantissa:10;       // Mantissa bits
    } f16;
};
#define BIAS_FLOAT_16_0              10 // Exponent bias for FLOAT_16_0


static inline psS16 convertF32toFloat16_0(psF32 value)
{
    union float_ieee754 in;             // Input value
    union float_16_0 out;               // Output value

    // XXX What happens to NAN and INF?
    in.f = value;
    out.f16.negative = in.ieee.negative;
    out.f16.exponent = in.ieee.exponent - BIAS_FLOAT_IEEE754 + BIAS_FLOAT_16_0;
    out.f16.mantissa = in.ieee.mantissa >> 13;

    return out.s16;
}

static inline psF32 convertF32fromFloat16_0(psS16 value)
{
    union float_16_0 in;                // Input value
    union float_ieee754 out;            // Output value

    in.s16 = value;
    out.ieee.negative = in.f16.negative;
    out.ieee.exponent = in.f16.exponent + BIAS_FLOAT_IEEE754 - BIAS_FLOAT_16_0;
    out.ieee.mantissa = in.f16.mantissa << 13;

    return out.f;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Public functions
//////////////////////////////////////////////////////////////////////////////////////////////////////////////


psImage *psFitsFloatImageToDisk(const psImage *image, psFitsFloat type)
{
    PS_ASSERT_IMAGE_NON_NULL(image, NULL);
    PS_ASSERT_IMAGE_TYPE(image, PS_TYPE_F32, NULL);

    psImage *output = NULL;             // Output image, to return

    switch (type) {
      case PS_FITS_FLOAT_NONE:
        // No conversion to be performed
        return psMemIncrRefCounter((psImage*)image); // Casting away "const"
      case PS_FITS_FLOAT_16_0:
        output = psImageAlloc(image->numCols, image->numRows, PS_TYPE_S16); // Output image
        for (int y = 0; y < image->numRows; y++) {
            for (int x = 0; x < image->numCols; x++) {
                output->data.S16[y][x] = convertF32toFloat16_0(image->data.F32[y][x]);
            }
        }
        break;
      default:
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Unrecognised custom floating-point type: %x", type);
        return NULL;
    }

    return output;
}


psImage *psFitsFloatImageFromDisk(psImage *out, const psImage *in, psFitsFloat type)
{
    PS_ASSERT_IMAGE_NON_NULL(in, NULL);

    psElemType elem = psFitsFloatImageType(type); // Type for elements
    if (elem == 0) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Unable to recognise convention: %x", type);
        return NULL;
    }

    int numCols = in->numCols, numRows = in->numRows; // Size of image

    out = psImageRecycle(out, numCols, numRows, elem);

    switch (type) {
      case PS_FITS_FLOAT_16_0:
        if (in->type.type != PS_TYPE_S16) {
            psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                    "Convention claims to be FLOAT_16_0, but image type is not S16.");
            return NULL;
        }
        psAssert(out->type.type == PS_TYPE_F32, "impossible");
        for (int y = 0; y < numRows; y++) {
            for (int x = 0; x < numCols; x++) {
                out->data.F32[y][x] = convertF32fromFloat16_0(in->data.S16[y][x]);
            }
        }
        return out;
      case PS_FITS_FLOAT_NONE:
      default:
        psAbort("Should be unreachable");
    }
    return NULL;
}

psElemType psFitsFloatImageType(psFitsFloat type)
{
    switch (type) {
      case PS_FITS_FLOAT_16_0:
        return PS_TYPE_F32;
      case PS_FITS_FLOAT_NONE:
      default:
        return 0;                       // Doesn't correspond to ANY type --- should flag a real error
    }
}


psFitsFloat psFitsFloatTypeFromString(const char *string)
{
    PS_ASSERT_STRING_NON_EMPTY(string, PS_FITS_FLOAT_NONE);

    if (strcmp(string, "FLOAT_16_0") == 0) {
        return PS_FITS_FLOAT_16_0;
    }

    psError(PS_ERR_BAD_PARAMETER_VALUE, true,
            "Unable to recognise custom floating-point convention name: %s", string);
    return PS_FITS_FLOAT_NONE;
}
