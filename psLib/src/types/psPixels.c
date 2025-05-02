/** @file  psPixels.c
 *
 *  @brief Contains psPixel related functions
 *
 *  @ingroup Image
 *
 *  @author Robert DeSonia, MHPCC
 *
 *  @version $Revision: 1.44 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-01-27 06:39:38 $
 *
 *  Copyright 2004-2005 Maui High Performance Computing Center, University of Hawaii
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <string.h>
#include <stdlib.h>

#include "psAbort.h"
#include "psSort.h"
#include "psMemory.h"
#include "psAssert.h"
#include "psError.h"

#include "psPixels.h"

#define PIXELS_DEFAULT_ADD 10           // Default number to add, if not specified

typedef int(*qsortCompareFunc)(const void *, const void *);

static void pixelsFree(psPixels* pixels)
{
    psAssert(pixels, "impossible");
    psFree(pixels->data);
}

static psPixels *pixelsAlloc(const char *file,
                          unsigned int lineno,
                          const char *func,
                          long nalloc)
{
    psPixels* out = p_psAlloc(file, lineno, func, sizeof(psPixels));
    psMemSetDeallocator(out, (psFreeFunc)pixelsFree);

    out->data = p_psAlloc(file, lineno, func, sizeof(psPixelCoord)*nalloc);
    P_PSPIXELS_SET_NALLOC(out,nalloc);

    return out;
}

psPixels* p_psPixelsAlloc(const char *file,
                          unsigned int lineno,
                          const char *func,
                          long nalloc)
{
    psPixels *out = pixelsAlloc(file, lineno, func, nalloc);
    out->n = nalloc;
    return out;
}

psPixels* p_psPixelsAllocEmpty(const char *file,
                             unsigned int lineno,
                             const char *func,
                             long nalloc)
{
    psPixels *out = pixelsAlloc(file, lineno, func, nalloc);
    out->n = 0;
    return out;
}


bool psMemCheckPixels(psPtr ptr)
{
    PS_ASSERT_PTR(ptr, false);
    return ( psMemGetDeallocator(ptr) == (psFreeFunc)pixelsFree );
}


psPixels* p_psPixelsRealloc(const char *file,
                            unsigned int lineno,
                            const char *func,
                            psPixels* pixels,
                            long nalloc)
{
    if (!pixels) {
        return p_psPixelsAlloc(file, lineno, func, nalloc);
    }
    if (nalloc < 0) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                "Can't reallocate a psPixels to negative size.");
        return pixels;
    }

    if (pixels->n == nalloc) {
        return pixels;
    }
    if (nalloc < pixels->n) {
        pixels->n = nalloc;
    }
    pixels->data = p_psRealloc(file, lineno, func, pixels->data, sizeof(psPixelCoord)*nalloc);
    P_PSPIXELS_SET_NALLOC(pixels,nalloc);

    return pixels;
}

psPixels* psPixelsAdd(psPixels* pixels,
                      long growth,
                      float x,
                      float y)
{
    if (growth <= 0) {
        growth = PIXELS_DEFAULT_ADD;
    }

    if (!pixels) {
        pixels = psPixelsAllocEmpty(growth);
    } else if (pixels->n >= pixels->nalloc) {
        pixels = psPixelsRealloc(pixels, pixels->nalloc + growth);
    }

    long n = pixels->n;

    pixels->data[n].x = x;
    pixels->data[n].y = y;

    pixels->n++;

    return pixels;
}

psPixels* p_psPixelsCopy(const char *file,
                         unsigned int lineno,
                         const char *func,
                         psPixels* out,
                         const psPixels* pixels)
{
    PS_ASSERT_PIXELS_NON_NULL(pixels, NULL);

    out = p_psPixelsRealloc(file, lineno, func, out, pixels->n);
    memcpy(out->data,pixels->data, pixels->n*sizeof(psPixelCoord));
    out->n = pixels->n;

    return out;
}

psImage *psPixelsToMask(psImage *out,
                        const psPixels *pixels,
                        psRegion region,
                        psImageMaskType maskVal)
{
    PS_ASSERT_PIXELS_NON_NULL(pixels, NULL);

    float x0 = region.x0;
    float x1 = region.x1;
    float y0 = region.y0;
    float y1 = region.y1;

    if (x0 < 0 || x1 < 0 || y0 < 0 || y1 < 0 || x1 < x0 || y1 < y0) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true,
                _("Specified psRegion, [%d:%d,%d:%d], does not specify a valid region."),
                (int)x0,(int)x1,(int)y0,(int)y1);
        psFree(out);
        return NULL;
    }

    int numRows = y1 - y0 + 1, numCols = x1 - x0 + 1; // Size of image

    out = psImageRecycle(out, numCols, numRows, PS_TYPE_IMAGE_MASK);
    if (!out) {
        psError(PS_ERR_UNKNOWN, false, _("Failed to create image of size %dx%d."), numCols, numRows);
        return NULL;
    }
//    P_PSIMAGE_SET_COL0(out, (int)x0);
//    P_PSIMAGE_SET_ROW0(out, (int)y0);
    psImageInit(out, 0);


    // cycle through the vector of pixels and insert pixels into image
    long length = pixels->n;            // Length of pixel array
    psImageMaskType** outData = out->data.PS_TYPE_IMAGE_MASK_DATA;
    for (int p = 0; p < length; p++) {
        float x = pixels->data[p].x;
        float y = pixels->data[p].y;
        // pixel in region?
        if (x >= x0 && x <= x1 && y >= y0 && y <= y1) {
            outData[(int)(y-y0)][(int)(x-x0)] |= maskVal;
        }
    }

    return out;
}

psPixels* psPixelsFromMask(psPixels* out,
                           const psImage* mask,
                           psImageMaskType maskVal)
{
    PS_ASSERT_IMAGE_NON_NULL(mask, NULL);
    PS_ASSERT_IMAGE_TYPE(mask, PS_TYPE_IMAGE_MASK, NULL);

    int numRows = mask->numRows, numCols = mask->numCols; // Size of image

    // assumption: number of masked pixels is relatively small compared to total pixels, so it is best to just
    // start with a guess and resize if necessary
    long minPixels = PS_MAX(numRows * numCols / 100, 32); // initial guess: 1% of pixels masked;
    if (!out || !out->data || out->nalloc < minPixels) {
        out = psPixelsRealloc(out, minPixels);
    }
    out->n = 0;

    // find the mask pixels in the image
    for (int y = 0; y < numRows; y++) {
        for (int x = 0; x < numCols; x++) {
            if (mask->data.PS_TYPE_IMAGE_MASK_DATA[y][x] & maskVal) {
                psPixelsAdd(out, out->nalloc, x, y);
            }
        }
    }

    return out;
}

psPixels* psPixelsConcatenate(psPixels *out, const psPixels *pixels)
{
    PS_ASSERT_PIXELS_NON_NULL(pixels, NULL);
    if (!out) {
        return psPixelsCopy(NULL, pixels);
    }

    long numIn = pixels->n;             // Number of input pixels
    long numOut = out->n;               // Number of (original) output pixels
    out = psPixelsRealloc(out, numIn + numOut);
    memcpy(&out->data[numOut], pixels->data, numIn * sizeof(psPixelCoord));
    out->n = numIn + numOut;

    return out;
}

// Macro functions for sorting a pixel list
#define PSPIXELS_SORT_COMPARE(A,B) (data[A].y < data[B].y || data[A].x < data[B].x)
#define PSPIXELS_SORT_SWAP(TYPE,A,B) { \
    if (A != B) { \
        TYPE temp = data[A]; \
        data[A] = data[B]; \
        data[B] = temp; \
    } \
}

psPixels* psPixelsDuplicates(psPixels *out, const psPixels *pixels)
{
    PS_ASSERT_PIXELS_NON_NULL(pixels, NULL);
    if (pixels->n <= 1) {
        return psPixelsCopy(out, pixels);
    }

    long numIn = pixels->n;          // Number of input pixels
    out = psPixelsRealloc(out, numIn);

    psPixelCoord *data = pixels->data;  // Dereference input
    PSSORT(numIn, PSPIXELS_SORT_COMPARE, PSPIXELS_SORT_SWAP, psPixelCoord);

    long numOut = 0;                     // Number of output pixels
    for (long i = 0; i < numIn; numOut++) {
        psPixelCoord pix = data[i];     // Coordinates of interest
        for (i++; i < numIn && data[i].x == pix.x && data[i].y == pix.y; i++); // No action
        out->data[numOut] = pix;
    }
    out->n = numOut;

    return out;
}

bool p_psPixelsPrint(FILE *fd,
                     psPixels* pixels,
                     const char *name)
{
    PS_ASSERT_PIXELS_NON_NULL(pixels, false);

    if (fd == NULL) {
        fd = stdout;
    } else {
        if ( fprintf(fd, "\n") < 0 ) {
            psError(PS_ERR_IO, true,
                    "Invalid file pointer in p_psPixelsPrint.  Could not write to fd.\n");
            return false;
        }
    }

    if (name != NULL) {
        fprintf (fd, "psPixels: %s\n", name);
    }

    long n = pixels->n;
    psPixelCoord* data = pixels->data;

    if (data == NULL || n == 0) {
        fprintf(fd,"EMPTY\n\n");
        return true;
    }

    for (long i = 0; i < n; i++) {
        fprintf (fd, "(%f,%f)\n", data[i].x, data[i].y);
    }
    fprintf (fd, "\n");
    return (true);
}

bool psPixelsSet(psPixels *pixels,
                 long position,
                 psPixelCoord value)
{
    PS_ASSERT_PIXELS_NON_NULL(pixels, false);

    if (position < 0) {
        position += pixels->n;
    }
    if (position < 0 || position >= pixels->n) {
        psError(PS_ERR_BAD_PARAMETER_SIZE, true, "Invalid position: %ld\n", position);
        return false;
    }
    pixels->data[position].x = value.x;
    pixels->data[position].y = value.y;

    return true;
}

psPixelCoord psPixelsGet(const psPixels *pixels,
                         long position)
{
    psPixelCoord out;
    if (pixels == NULL) {
        psError(PS_ERR_BAD_PARAMETER_NULL, true, _("Input psPixels can not be NULL."));
        out.x = NAN;
        out.y = NAN;
        return out;
    }
    if (position < 0) {
        position += pixels->n;
    }
    if (position >= pixels->n) {
        psError(PS_ERR_BAD_PARAMETER_SIZE, true, "Invalid position.  Number too large\n");
        out.x = NAN;
        out.y = NAN;
        return out;
    }
    if (position < 0) {
        psError(PS_ERR_BAD_PARAMETER_SIZE, true, "Invalid position.  Negative number too large\n");
        out.x = NAN;
        out.y = NAN;
        return out;
    }
    out.x = pixels->data[position].x;
    out.y = pixels->data[position].y;
    return out;
}

long psPixelsLength(const psPixels *pixels)
{
    PS_ASSERT_PIXELS_NON_NULL(pixels, -1);
    return pixels->n;
}

