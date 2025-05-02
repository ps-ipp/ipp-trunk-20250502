/* @file  pmFootprintArrayGrow.c
 *
 * @author RHL, Princeton & IfA; EAM, IfA
 *
 * @version $Revision: 1.12 $ $Name: not supported by cvs2svn $
 * @date $Date: 2009-01-27 06:39:38 $
 * Copyright 2006 Institute for Astronomy, University of Hawaii
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <strings.h>
#include <pslib.h>
#include "pmSpan.h"
#include "pmFootprintSpans.h"
#include "pmFootprint.h"
#include "pmPeaks.h"

# define USE_FFTS_TO_CONVOLVE 1

/*
 * Grow a psArray of pmFootprints isotropically by r pixels, returning a new psArray of new pmFootprints
 */
psArray *pmFootprintArrayGrow(const psArray *footprints, // footprints to grow
                              int r) {  // how much to grow each footprint
    assert (footprints->n == 0 || pmFootprintTest(footprints->data[0]));

    psTimerStart ("grow");

    if (footprints->n == 0) {           // we don't know the size of the footprint's region
        return psArrayAlloc(0);
    }
    /*
     * We'll insert the footprints into an image, then convolve with a disk,
     * then extract a footprint from the result --- this is magically what we want.
     */
    psImage *idImage = pmSetFootprintArrayIDs(footprints, true);
    psLogMsg ("psphot", PS_LOG_MINUTIA, "set footprint array IDs: %f sec\n", psTimerMark ("grow"));

#if 1
    // Use a separable convolution: should be faster
    idImage = (psImage*)psBinaryOp(idImage, idImage, "MIN", psScalarAlloc(1, PS_TYPE_S32));
    psImage *idImageMask = psImageCopy(NULL, idImage, PS_TYPE_IMAGE_MASK); // Image with 1 = object
    psImage *grownIdImage = psImageConvolveMask(NULL, idImageMask, 0x01, 0x01, -r, r, -r, r); // Grown mask
    if (!grownIdImage) {
        psError(PS_ERR_UNKNOWN, false, "Unable to grow mask.");
        psFree(grownIdImage);
        psFree(idImage);
        return NULL;
    }
    psFree(idImageMask);
#else
    if (r <= 0) {
        r = 1;                          // r == 1 => no grow
    }
    psKernel *circle = psKernelAlloc(-r, r, -r, r);
    assert (circle->image->numRows == 2*r + 1 && circle->image->numCols == circle->image->numRows);
    for (int i = 0; i <= r; i++) {
        for (int j = 0; j <= r; j++) {
            if (i*i + j*j <= r*r) {
                circle->kernel[i][j] =
                    circle->kernel[i][-j] =
                    circle->kernel[-i][j] =
                    circle->kernel[-i][-j] = 1;
            }
        }
    }

# if (USE_FFTS_TO_CONVOLVE)
    psImage *f32ImageIn = psImageCopy (NULL, idImage, PS_TYPE_F32);
    psImage *f32ImageOut = psImageConvolveFFT(NULL, f32ImageIn, NULL, 0, circle);
    psImage *grownIdImage = psImageCopy (NULL, f32ImageOut, PS_TYPE_S32);
    psFree (f32ImageIn);
    psFree (f32ImageOut);
#else
    psImage *grownIdImage = psImageConvolveDirect(NULL, idImage, circle); // Here's the actual grow step
#endif // USE_FFTS_TO_CONVOLVE
    psFree(circle);
#endif // Don't bother at all

    psFree(idImage);

    psLogMsg ("psphot", PS_LOG_MINUTIA, "convolved with grow disc: %f sec\n", psTimerMark ("grow"));

    psArray *grown = pmFootprintsFind(grownIdImage, 0.5, 1); // and here we rebuild the grown footprints
    psLogMsg ("psphot", PS_LOG_MINUTIA, "found grown footprints: %f sec\n", psTimerMark ("grow"));

    assert (grown != NULL);
    psFree(grownIdImage);

    /*
     * Now assign the peaks appropriately.  We could do this more efficiently
     * using grownIdImage (which we just freed), but this is easy and probably fast enough
     */
    psArray *peaks = pmFootprintArrayToPeaks(footprints);
    pmFootprintsAssignPeaks(grown, peaks);
    psFree(peaks);




    psLogMsg ("psphot", PS_LOG_MINUTIA, "finished grow: %f sec\n", psTimerMark ("grow"));

    return grown;

}

