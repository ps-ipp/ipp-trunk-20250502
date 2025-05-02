# include "psphotInternal.h"

bool psphotEllipticalProfile (pmSource *source, bool RAW_RADIUS) {

    psAssert (source, "missing source");
    psAssert (source->extpars, "missing extpars");
    psAssert (source->pixels, "missing pixels");

    pmSourceExtendedPars *extpars = source->extpars;

    if (!source->extpars->ellipticalFlux) {
	source->extpars->ellipticalFlux = pmSourceEllipticalFluxAlloc();
    }
    pmSourceEllipticalFlux *profile = source->extpars->ellipticalFlux;

    profile->radiusElliptical = psVectorAllocEmpty(100, PS_TYPE_F32);
    profile->fluxElliptical = psVectorAllocEmpty(100, PS_TYPE_F32);

    psVector *radius = profile->radiusElliptical;
    psVector *flux = profile->fluxElliptical;

    // the psEllipse functions use z = 0.5(x/Sxx)^2 + 0.5(y/Syy)^2 + x y Sxy
    // which are converted to z = 0.5(x/a)^2 + 0.5(y/b)^2
    // we have major and minor axes of a specific ellipse with r^2 = (x/A)^2 + (y/B)^2
    // a = A / sqrt(2)

    // we have the shape parameters of the elliptical contour at the reference isophote.
    // use the axis ratio (major/minor) to rescale the radial profile so that 1 pixel
    // along the major axis is 1 pixel, and a smaller amount on the minor axis

    psEllipseAxes axes;
    if (RAW_RADIUS) {
	// force circular profile
	axes.major = M_SQRT1_2;
	axes.minor = M_SQRT1_2;
    } else {
	axes.major = M_SQRT1_2;
	axes.minor = M_SQRT1_2 * (extpars->axes.minor / extpars->axes.major);
    }

    // axes.major = 1.0;
    // axes.minor = extpars->axes.minor / extpars->axes.major;

    axes.theta = extpars->axes.theta;
    psEllipseShape shape = psEllipseAxesToShape (axes);

    float Sxx = shape.sx;
    float Sxy = shape.sxy;
    float Syy = shape.sy;

    // XXX drop these two vectors?
    psVector *radiusRaw = psVectorAllocEmpty(100, PS_TYPE_F32);
    psVector *fluxRaw = psVectorAllocEmpty(100, PS_TYPE_F32);

    for (int iy = 0; iy < source->pixels->numRows; iy++) {
	for (int ix = 0; ix < source->pixels->numCols; ix++) {

	    // 0.5 PIX: get radius as a function of pixel coord
	    float x = ix + 0.5 - source->peak->xf + source->pixels->col0;
	    float y = iy + 0.5 - source->peak->yf + source->pixels->row0;

	    float r2 = 0.5*PS_SQR(x/Sxx) + 0.5*PS_SQR(y/Syy) + x*y*Sxy;
	    float Rraw = hypot(x, y);

	    psVectorAppend(radius, sqrt(r2));
	    psVectorAppend(flux, source->pixels->data.F32[iy][ix]);

	    psVectorAppend(radiusRaw, Rraw);
	    psVectorAppend(fluxRaw, source->pixels->data.F32[iy][ix]);
	}
    }

    // psVector *radiusRaw = psVectorAllocEmpty(100, PS_TYPE_F32);
    // psVector *fluxRaw = psVectorAllocEmpty(100, PS_TYPE_F32);
    // for (int i = 0; i < profile->radii->n; i++) {
    //   psVector *r = profile->radii->data[i];
    //   psVector *f = profile->fluxes->data[i];
    //   for (int j = 0; j < r->n; j++) {
    // 	psVectorAppend(radiusRaw, r->data.F32[j]);
    // 	psVectorAppend(fluxRaw, f->data.F32[j]);
    //   }
    // }

    psphotPetrosianVisualProfileRadii (radius, flux, radiusRaw, fluxRaw, source->peak->rawFlux, 0.0);
    // psphotPetrosianVisualProfileByAngle (radius, flux);

    psFree (radiusRaw);
    psFree (fluxRaw);
    return true;
}
