# include "psphotInternal.h"

enum {
    PSPHOT_ADD_NONE = 0,
    PSPHOT_ADD_MODEL = 1,
    PSPHOT_ADD_R0 = 2,
    PSPHOT_ADD_R1 = 4,
};

bool psphotAddModel(psImage *image,
		    pmModel *model,
		    int mode
    )
{
    psTrace("psModules.objects", 3, "---- %s() begin ----\n", __func__);

    PS_ASSERT_PTR_NON_NULL(model, false);
    PS_ASSERT_IMAGE_NON_NULL(image, false);
    PS_ASSERT_IMAGE_TYPE(image, PS_TYPE_F32, false);

    psVector *x = psVectorAlloc(2, PS_TYPE_F32);
    psVector *params = model->params;
    psS32 imageCol;
    psS32 imageRow;
    psF32 skyValue = params->data.F32[0];
    psF32 pixelValue;
    
    float xCenter = model->params->data.F32[PM_PAR_XPOS];
    float yCenter = model->params->data.F32[PM_PAR_YPOS];
    float Io = model->params->data.F32[PM_PAR_I0];

    int xBin = 1;
    int yBin = 1;
    float xResidCenter = 0.0;
    float yResidCenter = 0.0;

    psImageInterpolateOptions *Ro = NULL;
    psImageInterpolateOptions *Rx = NULL;
    psImageInterpolateOptions *Ry = NULL;
    if (model->residuals && (mode & (PSPHOT_ADD_R0 | PSPHOT_ADD_R1))) {
	Ro = psImageInterpolateOptionsAlloc(
	    PS_INTERPOLATE_BILINEAR,
	    model->residuals->Ro, NULL, NULL, 0, 0.0, 0.0, 1, 0, 0.0);
	Rx = psImageInterpolateOptionsAlloc(
	    PS_INTERPOLATE_BILINEAR,
	    model->residuals->Rx, NULL, NULL, 0, 0.0, 0.0, 1, 0, 0.0);
	Ry = psImageInterpolateOptionsAlloc(
	    PS_INTERPOLATE_BILINEAR,
	    model->residuals->Ry, NULL, NULL, 0, 0.0, 0.0, 1, 0, 0.0);

	xBin = model->residuals->xBin;
	yBin = model->residuals->yBin;
	xResidCenter = model->residuals->xCenter;
	yResidCenter = model->residuals->yCenter;
    }

    for (psS32 iy = 0; iy < image->numRows; iy++) {
        for (psS32 ix = 0; ix < image->numCols; ix++) {

            // Convert i/j to image coord space:
	    imageCol = ix + image->col0;
	    imageRow = iy + image->row0;

            x->data.F32[0] = (float) imageCol;
            x->data.F32[1] = (float) imageRow;

            // set the appropriate pixel value for this coordinate
	    if (mode & PSPHOT_ADD_MODEL) {
		pixelValue = model->modelFunc (NULL, params, x) - skyValue;
	    } else {
		pixelValue = 0.0;
	    }

	    // get the contribution from the residual model
	    // XXX for a test, do this for all sources and all pixels
	    if (Ro) {
		// fractional image position
		// this is wrong for the 'center' case
		float ox = xBin*(ix + 0.5 + image->col0 - xCenter) + xResidCenter;
		float oy = yBin*(iy + 0.5 + image->row0 - yCenter) + yResidCenter;

		psImageMaskType mflux = 0;
		double Fo = 0.0;
		double Fx = 0.0;
		double Fy = 0.0;
		psImageInterpolate (&Fo, NULL, &mflux, ox, oy, Ro);
		psImageInterpolate (&Fx, NULL, &mflux, ox, oy, Rx);
		psImageInterpolate (&Fy, NULL, &mflux, ox, oy, Ry);

		if (!mflux && isfinite(Fo) && isfinite(Fx) && isfinite(Fy)) {
		    if (mode & PSPHOT_ADD_R0) {
			pixelValue += Io*Fo;
		    }
		    if (mode & PSPHOT_ADD_R1) {
			pixelValue += Io*(xCenter*Fx + yCenter*Fy);
		    }
		}
	    }
	    image->data.F32[iy][ix] += pixelValue;
        }
    }
    psFree(x);
    psFree(Ro);
    psFree(Rx);
    psFree(Ry);
    psTrace("psModules.objects", 3, "---- %s(true) end ----\n", __func__);
    return(true);
}

// construct an initial PSF model for each object 
bool psphotTestSourceOutput (pmReadout *readout, psArray *sources, psMetadata *recipe, pmPSF *psf) {

    psImage *imMo = psImageAlloc (readout->image->numCols, readout->image->numRows, PS_TYPE_F32);
    psImage *imR0 = psImageAlloc (readout->image->numCols, readout->image->numRows, PS_TYPE_F32);
    psImage *imR1 = psImageAlloc (readout->image->numCols, readout->image->numRows, PS_TYPE_F32);
    
    // create template model
    pmModel *modelRef = pmModelAlloc(psf->type);
    modelRef->params->data.F32[PM_PAR_SKY] = 0;
    modelRef->params->data.F32[PM_PAR_I0] = 1000;

    int dx = 25;
    int dy = 25;

    // generate a grid of fake sources with amplitude 1000
    for (int iy = 50; iy < imMo->numRows; iy += 100) {
	for (int ix = 50; ix < imMo->numCols; ix += 100) {
	    
	    // assign the x and y coords to the image center
	    modelRef->params->data.F32[PM_PAR_XPOS] = ix;
	    modelRef->params->data.F32[PM_PAR_YPOS] = iy;
	    
	    // create modelPSF from this model
	    pmModel *model = pmModelFromPSF (modelRef, psf);
	    model->residuals = psf->residuals;

	    // generate working image for this source
	    psRegion region = psRegionSet(ix - dx, ix + dx, iy - dy, iy + dy);

	    psImage *vM = psImageSubset (imMo, region);
	    psImage *v0 = psImageSubset (imR0, region);
	    psImage *v1 = psImageSubset (imR1, region);

	    // we want to make one image o
	    psphotAddModel (vM, model, PSPHOT_ADD_MODEL);
	    psphotAddModel (v0, model, PSPHOT_ADD_R0);
	    psphotAddModel (v1, model, PSPHOT_ADD_R1);
	}
    }

    psphotSaveImage (NULL, imMo, "grid.Mo.fits");
    psphotSaveImage (NULL, imR0, "grid.R0.fits");
    psphotSaveImage (NULL, imR1, "grid.R1.fits");

    exit (0);
}

