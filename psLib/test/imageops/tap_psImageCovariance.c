/** @file  tap_psImageCovariance.c
 *
 *  @brief Contains the tests for psImageCovariance.[ch]
 *
 *  @version $Revision: 1.2 $
 *  @date $Date: 2007-05-02 04:14:33 $
 *
 *  Copyright 2011 Institute for Astronomy, University of Hawaii
 */

#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include "tap.h"
#include "pstap.h"

psKernel *psKernelReadSimple(char *filename) {

    psRegion region = {0, 0, 0, 0};

    psFits *fits = psFitsOpen(filename, "r");
    psFitsDumpErrors(0, "foo");
    ok(fits, "opened FITS file");

    psImage *image = psFitsReadImage(fits, region, 0);
    psFitsDumpErrors(0, "foo");
    ok(image, "read image from FITS file");

    psFitsClose(fits);

    int x0 = image->numCols / 2;
    int y0 = image->numRows / 2;
    psKernel *kernel = psKernelAllocFromImage(image, x0, y0);
    ok(kernel, "allocated kernel from image");

    psFree(image);

    return kernel;
}

int main(int argc, char **argv) {

    plan_tests(4);

    // return null for null input image
    {
        psMemId id = psMemGetId();

	// read in the kernel and the covariance matrix
	psKernel *kernel = psKernelReadSimple("kernel.fits");
        ok(kernel, "read kernel");

	psKernel *inputCov = psKernelReadSimple("cov.input.fits");
        ok(inputCov, "read input covariance");

	psKernel *outputCov = psImageCovarianceCalculate(kernel, inputCov);
        ok(outputCov, "calculated covariance");

	psFits *fits = psFitsOpen("cov.output.fits", "w");
	psFitsWriteImage(fits, NULL, outputCov->image, 0, NULL);
	psFitsClose(fits);

	psFree(kernel);
        psFree(inputCov);
        psFree(outputCov);
        ok(!psMemCheckLeaks (id, NULL, NULL, false), "no memory leaks");
    }
}

    
