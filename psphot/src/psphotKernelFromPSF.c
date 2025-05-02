# include "psphotInternal.h"

psKernel *psphotKernelFromPSF (pmSource *source, int nPix) {

    assert (source);
    assert (source->psfImage); // XXX build if needed?

    int x0 = source->peak->xf - source->psfImage->col0;
    int y0 = source->peak->yf - source->psfImage->row0;

    // need to decide on the size: dynamically? statically?
    psKernel *psf = psKernelAlloc (-nPix, +nPix, -nPix, +nPix);

    // XXX we should just re-construct a PSF at this location 
    // psModelAdd (psf->image, NULL, source->modelPSF, PM_MODEL_OP_FULL | PM_MODEL_OP_NORM | PM_MODEL_OP_CENTER);
  
    // if the realized PSF for this object does not cover the full kernel, give up for now
    if (x0 + psf->xMin < 0) goto escape;
    if (x0 + psf->xMax >= source->psfImage->numCols) goto escape;
    if (y0 + psf->yMin < 0) goto escape;
    if (y0 + psf->yMax >= source->psfImage->numRows) goto escape;

    double sum = 0.0;
    for (int j = psf->yMin; j <= psf->yMax; j++) {
	for (int i = psf->xMin; i <= psf->xMax; i++) {
	    double value = source->psfImage->data.F32[y0 + j][x0 + i];
	    psf->kernel[j][i] = value;
	    sum += value;
	}
    }
    assert (sum > 0.0);

    // psf must be normalized (integral = 1.0)
    for (int i = 0; i < psf->image->numRows; i++) {
	for (int j = 0; j < psf->image->numCols; j++) {
	    psf->image->data.F32[i][j] /= sum;
	}
    }

    return psf;

escape:
    psFree (psf);
    return NULL;
}
