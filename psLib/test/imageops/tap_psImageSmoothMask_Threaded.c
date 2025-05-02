/** @file  tap_psImageSmoothMask_Threaded.c
 *
 *  This code tests the psImageSmoothMask_Threaded() routine.
 *
 *  @author EAM
 *  @date $Date: 2007-02-27 23:56:12 $
 *
 *  Copyright 2013 IfA, University of Hawaii
 */
#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include "tap.h"
#include "pstap.h"

# define NX 4800
# define NY 4800
# define SIGMA 1.5
# define NSIGMA 3.0

bool saveImage (psImage *image, char *filename) {
    psFits *fits = psFitsOpen (filename, "w");
    psFitsWriteImage (fits, NULL, image, 0, NULL);
    psFitsClose (fits);
    return true;
}

int main(int argc, char **argv)
{
    bool status;

    psLogSetFormat("HLNM");
    psLogSetLevel(PS_LOG_INFO);
    plan_tests(14);

    psMemId id = psMemGetId();

    psTimerStart ("timer");
    psImage *image = psImageAlloc(NX, NY, PS_TYPE_F32);

    // Set a checkboard pattern
    for (int iy = 0 ; iy < NY ; iy++) {
      for (int ix = 0 ; ix < NX ; ix++) {
	if ((ix%2) != (iy%2)) {
	  image->data.F32[iy][ix] = 1.0;
	} else {
	  image->data.F32[iy][ix] = 0.0;
	}
      }
    }
    saveImage (image, "test.im.fits");
    fprintf (stderr, "create image : %f sec\n", psTimerMark("timer")); psTimerClear("timer");

    psImage *mask = psImageAlloc(NX, NY, PS_TYPE_IMAGE_MASK);
    psImageInit (mask, 0);
	
    // mask a corner
    for (int iy = 0 ; iy < 0.5*NY ; iy++) {
      for (int ix = 0 ; ix < 0.5*NX ; ix++) {
	mask->data.PS_TYPE_IMAGE_MASK_DATA[iy][ix] = 1;
      }
    }
    saveImage (mask, "test.mk.fits");
    fprintf (stderr, "create mask : %f sec\n", psTimerMark("timer")); psTimerClear("timer");

    psImage *alt = psImageCopy(NULL, image, image->type.type);

    psTimerClear("timer");
    status = psImageSmooth(alt, SIGMA, NSIGMA);
    ok(status, "basic image smooth");
    fprintf (stderr, "basic smooth : %f sec\n", psTimerMark("timer")); 

    saveImage (alt, "test.sm1.fits");
    psFree(alt);

    psImage *alt2 = psImageCopy(NULL, image, image->type.type);
    psImage *mask2 = psImageCopy(NULL, mask, mask->type.type);

    psTimerClear("timer");
    status = psImageSmoothMaskF32(alt2, mask2, 0xff, SIGMA, NSIGMA);
    ok(status, "basic image smooth");
    fprintf (stderr, "mask smooth : %f sec\n", psTimerMark("timer")); 

    saveImage (alt2, "test.sm2.fits");
    psFree(alt2);
    psFree(mask2);

    psThreadPoolInit (4);
    psImageConvolveSetThreads(true);

    psTimerClear("timer");
    psImage *alt3 = psImageSmoothMask_Threaded(NULL, image, mask, 0xff, SIGMA, NSIGMA, 0.25);
    fprintf (stderr, "mask smooth, threaded : %f sec\n", psTimerMark("timer")); 
    saveImage (alt3, "test.sm3.fits");
    psFree(alt3);

    psTimerClear("timer");
    psImage *alt4 = psImageSmoothNoMask_Threaded(NULL, image, SIGMA, NSIGMA, 0.25);
    fprintf (stderr, "basic smooth, threaded : %f sec\n", psTimerMark("timer")); 
    saveImage (alt4, "test.sm4.fits");
    psFree(alt4);

    psImageConvolveSetThreads(false);

    psFree(image);
    psFree(mask);
    psTimerStop();

    psThreadPoolFinalize();

    ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
}
