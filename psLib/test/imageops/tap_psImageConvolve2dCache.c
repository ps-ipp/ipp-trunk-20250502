/** @file  tap_psImageConvolve2dCache.c
 *
 *  @brief Contains the tests for psImageConvolve2dCache.[ch]
 *  @author Eugene Magnier, IfA
 *  @date $Date: 2007-05-01 00:08:52 $
 *
 *  Copyright 2013 IfA, University of Hawaii
 */

#include <stdio.h>
#include <string.h>
#include <pslib.h>
#include "tap.h"
#include "pstap.h"

bool tap_convolve_delta (int Nx, int Ny, int Xo, int Yo, float Nsigma, float sigma, float kappa);

int main(int argc, char **argv) {

    plan_tests(141);

    psMemId id = psMemGetId();

    // generate an image to convolve
    tap_convolve_delta (301, 301, 150, 150, 3.0, 2.0, 1.0);
    tap_convolve_delta (301, 301, 100, 200, 3.0, 2.0, 1.0);
    tap_convolve_delta (301, 301, 200, 100, 3.0, 2.0, 1.0);
    tap_convolve_delta (301, 301, 200, 200, 3.0, 2.0, 1.0);
    tap_convolve_delta (301, 301, 100, 100, 3.0, 2.0, 1.0);

    tap_convolve_delta (301, 301, 150, 150, 3.0, 3.0, 1.0);
    tap_convolve_delta (301, 301, 100, 200, 3.0, 3.0, 1.0);
    tap_convolve_delta (301, 301, 200, 100, 3.0, 3.0, 1.0);
    tap_convolve_delta (301, 301, 200, 200, 3.0, 3.0, 1.0);
    tap_convolve_delta (301, 301, 100, 100, 3.0, 3.0, 1.0);

    tap_convolve_delta (301, 301, 150, 150, 3.0, 1.0, 1.0);
    tap_convolve_delta (301, 301, 100, 200, 3.0, 1.0, 1.0);
    tap_convolve_delta (301, 301, 200, 100, 3.0, 1.0, 1.0);
    tap_convolve_delta (301, 301, 200, 200, 3.0, 1.0, 1.0);
    tap_convolve_delta (301, 301, 100, 100, 3.0, 1.0, 1.0);

    tap_convolve_delta (301, 301, 150, 150, 3.0, 2.0, 2.0);
    tap_convolve_delta (301, 301, 100, 200, 3.0, 2.0, 2.0);
    tap_convolve_delta (301, 301, 200, 100, 3.0, 2.0, 2.0);
    tap_convolve_delta (301, 301, 200, 200, 3.0, 2.0, 2.0);
    tap_convolve_delta (301, 301, 100, 100, 3.0, 2.0, 2.0);

    tap_convolve_delta (301, 301, 150, 150, 4.0, 3.0, 1.0);
    tap_convolve_delta (301, 301, 100, 200, 4.0, 3.0, 1.0);
    tap_convolve_delta (301, 301, 200, 100, 4.0, 3.0, 1.0);
    tap_convolve_delta (301, 301, 200, 200, 4.0, 3.0, 1.0);
    tap_convolve_delta (301, 301, 100, 100, 4.0, 3.0, 1.0);

    tap_convolve_delta (301, 301, 150, 150, 5.0, 3.0, 1.0);
    tap_convolve_delta (301, 301, 100, 200, 5.0, 3.0, 1.0);
    tap_convolve_delta (301, 301, 200, 100, 5.0, 3.0, 1.0);
    tap_convolve_delta (301, 301, 200, 200, 5.0, 3.0, 1.0);
    tap_convolve_delta (301, 301, 100, 100, 5.0, 3.0, 1.0);

    tap_convolve_delta (301, 301, 150, 150, 5.0, 1.0, 1.0);
    tap_convolve_delta (301, 301, 100, 200, 5.0, 1.0, 1.0);
    tap_convolve_delta (301, 301, 200, 100, 5.0, 1.0, 1.0);
    tap_convolve_delta (301, 301, 200, 200, 5.0, 1.0, 1.0);
    tap_convolve_delta (301, 301, 100, 100, 5.0, 1.0, 1.0);

    ok(!psMemCheckLeaks (id, NULL, stdout, false), "no memory leaks");

    return exit_status();
}

bool tap_convolve_delta (int Nx, int Ny, int Xo, int Yo, float Nsigma, float sigma, float kappa) {

    bool status;

    psImage *input = psImageAlloc (Nx, Ny, PS_TYPE_F32);
    psImageInit (input, 0.0);
    input->data.F32[Yo][Xo] = 1.0;

    psImageSmooth2dCacheData *smdata = psImageSmooth2dCacheAlloc (Nsigma);
    ok(smdata, "init smdata");
    ok(!smdata->radflux, "init radflux to NULL");
    ok(smdata->Nsigma == Nsigma, "init Nsigma");
    
    status = psImageSmooth2dCacheKernel_PS1_V1 (smdata, sigma, kappa);
    ok(status, "generate a kernel");
    
    status = psImageSmooth2dCache_F32 (input, smdata);
    ok(status, "smoothed image");
    
    double s = 0.0;

    psImage *model = psImageAlloc (Nx, Ny, PS_TYPE_F32);
    psImageInit (model, 0.0);
    for (int iy = 0; iy < model->numRows; iy++) {
	for (int ix = 0; ix < model->numCols; ix++) {
	    float z = 0.5*PS_SQR((ix + 0.5 - Xo - 0.5) / sigma) + 0.5*PS_SQR((iy + 0.5 - Yo - 0.5) / sigma);
	    if (z > 0.5*PS_SQR(smdata->Nsigma)) continue;
	   
	    float f = 1.0 / (1.0 + z*kappa + pow(z, 1.666));

	    model->data.F32[iy][ix] = f;
	    s += f;
	}
    }
    for (int iy = 0; iy < model->numRows; iy++) {
	for (int ix = 0; ix < model->numCols; ix++) {
	    model->data.F32[iy][ix] /= s;
	}
    }

    float min_df = +10.0;
    float max_df = -10.0;
    for (int iy = 0; iy < model->numRows; iy++) {
	for (int ix = 0; ix < model->numCols; ix++) {
	    float df = input->data.F32[iy][ix] - model->data.F32[iy][ix];
	    min_df = PS_MIN (min_df, df);
	    max_df = PS_MAX (max_df, df);
	}
    }
    ok (fabs(min_df) < 3e-5, "minor deviations");
    ok (fabs(max_df) < 3e-5, "minor deviations");

    if ((fabs(min_df) >= 3e-5) || (fabs(max_df) >= 3e-5)) {
	psFits *fits = NULL;
	fits = psFitsOpen ("input.fits", "w");
	psFitsWriteImage (fits, NULL, input, 0, NULL);
	psFitsClose (fits);

	fits = psFitsOpen ("model.fits", "w");
	psFitsWriteImage (fits, NULL, model, 0, NULL);
	psFitsClose (fits);
    }

    psFree (input);
    psFree (model);
    psFree (smdata);

    return true;
}
