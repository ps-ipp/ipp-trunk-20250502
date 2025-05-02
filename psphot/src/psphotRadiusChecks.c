# include "psphotInternal.h"
# define RADIUS_TYPE int

static float PSF_FIT_NSIGMA;
static float PSF_FIT_PADDING;
static float PSF_APERTURE = 0;  // radius to use in PSF aperture mags
static float PSF_FIT_RADIUS = 0;        // radius to use in fitting (ignored if <= 0,
                                        // and a per-object radius is calculated)

bool psphotInitRadiusPSF(psMetadata *recipe, pmReadout *readout) {

    bool status = true;

    PSF_FIT_NSIGMA = psMetadataLookupF32(&status, recipe, "PSF_FIT_NSIGMA");
    PSF_FIT_PADDING = psMetadataLookupF32(&status, recipe, "PSF_FIT_PADDING");

    PSF_FIT_RADIUS =  psMetadataLookupF32(&status, readout->analysis, "PSF_FIT_RADIUS");
    if (!status) {
        PSF_FIT_RADIUS = psMetadataLookupF32(&status, recipe, "PSF_FIT_RADIUS");
    }

    PSF_APERTURE =  psMetadataLookupF32(&status, readout->analysis, "PSF_APERTURE");
    if (!status) {
        PSF_APERTURE =  psMetadataLookupF32(&status, recipe, "PSF_APERTURE");
    }

    // The PSF_FIT_RADIUS and PSF_APERTURE may not be set if the PSF was loaded and not chosen

    if (PSF_FIT_RADIUS == 0.0) {
        float gaussSigma = psMetadataLookupF32(&status, readout->analysis, "MOMENTS_GAUSS_SIGMA");
        if (!status) {
            gaussSigma = psMetadataLookupF32(&status, recipe, "MOMENTS_GAUSS_SIGMA");
        }
        float fitScale = psMetadataLookupF32(&status, recipe, "PSF_FIT_RADIUS_SCALE");
        PSF_FIT_RADIUS = (int)(fitScale*gaussSigma);
    }

    if (PSF_APERTURE == 0.0) {
        float gaussSigma = psMetadataLookupF32(&status, readout->analysis, "MOMENTS_GAUSS_SIGMA");
        if (!status) {
            gaussSigma = psMetadataLookupF32(&status, recipe, "MOMENTS_GAUSS_SIGMA");
        }
        float apScale = psMetadataLookupF32(&status, recipe, "PSF_APERTURE_SCALE");
        PSF_APERTURE = (int)(apScale*gaussSigma);
    }

    return true;
}

// call this function whenever you (re)-define the PSF model
bool psphotCheckRadiusPSF (pmReadout *readout, pmSource *source, pmModel *model, psImageMaskType markVal)
{
    psF32 *PAR = model->params->data.F32;

    // XXX do we have a better value for the sky noise level?  not really...
    pmMoments *moments = source->moments;

    // set the fit radius based on the object flux limit and the model
    float radiusFit = PSF_FIT_RADIUS;
    if (radiusFit <= 0) {               // use fixed radius
        if (moments == NULL) {
            radiusFit = model->class->modelRadius(model->params, PSF_FIT_NSIGMA*moments->dSky);
        } else {
            radiusFit = model->class->modelRadius(model->params, 1.0);
        }
        model->fitRadius = (RADIUS_TYPE)(radiusFit + PSF_FIT_PADDING);
    } else {
        model->fitRadius = radiusFit;
    }
    if (isnan(model->fitRadius)) psAbort("error in radius");

    if (source->mode & PM_SOURCE_MODE_SATSTAR) {
        model->fitRadius *= 2;
    }

    // radius used to measure aperture photometry
    source->apRadius = PSF_APERTURE;

    bool status = pmSourceRedefinePixels (source, readout, PAR[PM_PAR_XPOS], PAR[PM_PAR_YPOS], model->fitRadius);

    // set the mask to flag the excluded pixels
    psImageKeepCircle (source->maskObj, PAR[PM_PAR_XPOS], PAR[PM_PAR_YPOS], model->fitRadius, "OR", markVal);
    return status;
}

bool psphotCheckRadiusPSFBlend (pmReadout *readout, pmSource *source, pmModel *model, psImageMaskType markVal, float dR) {

    psF32 *PAR = model->params->data.F32;

    pmMoments *moments = source->moments;
    if (moments == NULL) return false;

    // set the fit radius based on the object flux limit and the model
    float radiusFit = PSF_FIT_RADIUS;
    if (radiusFit <= 0) {               // use fixed radius
        if (moments == NULL) {
            radiusFit = model->class->modelRadius(model->params, PSF_FIT_NSIGMA*moments->dSky);
        } else {
            radiusFit = model->class->modelRadius(model->params, 1.0);
        }
        model->fitRadius = (RADIUS_TYPE)(radiusFit + PSF_FIT_PADDING);
    } else {
        model->fitRadius = radiusFit;
    }
    if (isnan(model->fitRadius)) psAbort("error in radius");

    // above sets a radius for a single star, bump by blend separation
    model->fitRadius += dR;

    if (source->mode &  PM_SOURCE_MODE_SATSTAR) {
        model->fitRadius *= 2;
    }

    // radius used to measure aperture photometry
    source->apRadius = PSF_APERTURE;

    bool status = pmSourceRedefinePixels (source, readout, PAR[PM_PAR_XPOS], PAR[PM_PAR_YPOS], model->fitRadius);

    // set the mask to flag the excluded pixels
    psImageKeepCircle (source->maskObj, PAR[PM_PAR_XPOS], PAR[PM_PAR_YPOS], model->fitRadius, "OR", markVal);
    return status;
}

static float EXT_FIT_SKY_SIG;
static float EXT_FIT_NSIGMA;
static float EXT_FIT_PADDING;
static float EXT_FIT_MAX_RADIUS;

bool psphotInitRadiusEXT (psMetadata *recipe, pmReadout *readout) {

    bool status;

    EXT_FIT_NSIGMA     = psMetadataLookupF32 (&status, recipe, "EXT_FIT_NSIGMA");
    EXT_FIT_PADDING    = psMetadataLookupF32 (&status, recipe, "EXT_FIT_PADDING");
    EXT_FIT_MAX_RADIUS = psMetadataLookupF32 (&status, recipe, "EXT_FIT_MAX_RADIUS");

    float skyStdev = psMetadataLookupF32 (&status, readout->analysis, "SKY_STDEV");

    EXT_FIT_SKY_SIG = skyStdev;

    return true;
}

# define MIN_WINDOW 5.0
# define SCALE1 5.0
# define SCALE2 12.0

// call this function whenever you (re)-define the EXT model
// XXX this function does not shrink the window
bool psphotSetRadiusMoments (float *fitRadius, float *windowRadius, pmReadout *readout, pmSource *source, psImageMaskType markVal) {

    psAssert (source, "source not defined??");
    psAssert (source->moments, "moments not defined??");

    *fitRadius = SCALE1 * source->moments->Mrf;
    *fitRadius = PS_MIN (PS_MAX(*fitRadius, MIN_WINDOW), EXT_FIT_MAX_RADIUS);

    *windowRadius = SCALE2 * source->moments->Mrf;
    *windowRadius = PS_MIN (PS_MAX(*windowRadius, 2.5*MIN_WINDOW), 2.5*EXT_FIT_MAX_RADIUS);

    // redefine the pixels if needed
    pmSourceRedefinePixels (source, readout, source->peak->xf, source->peak->yf, *windowRadius);

    // set the mask to flag the excluded pixels
    psImageKeepCircle (source->maskObj, source->peak->xf, source->peak->yf, *fitRadius, "OR", markVal);

    return true;
}
# undef SCALE1
# undef SCALE2
# undef MIN_WINDOW

// XXX EAM : 20130724 : for a test, double the window size parameters
// # define MIN_WINDOW 10.0
// # define SCALE1 7.0
# define MIN_WINDOW 5.0
# define SCALE1 3.0
# define PAD_WINDOW 3.0

// call this function whenever you (re)-define the EXT model
// XXX alternate function to set exactly the desired window size
bool psphotSetRadiusMomentsExact (float *fitRadius, float *windowRadius, pmReadout *readout, pmSource *source, psImageMaskType markVal) {

    psRegion newRegion;

    psAssert (source, "source not defined??");
    psAssert (source->moments, "moments not defined??");

    *fitRadius = SCALE1 * source->moments->Mrf;
    *fitRadius = PS_MIN (PS_MAX(*fitRadius, MIN_WINDOW), EXT_FIT_MAX_RADIUS);

    *windowRadius = *fitRadius + PAD_WINDOW;

    // check to see if new region is completely contained within old region
    newRegion = psRegionForSquare (source->peak->xf, source->peak->yf, *windowRadius);
    newRegion = psRegionForImage (readout->image, newRegion);

    // redefine the pixels to match
    pmSourceRedefinePixelsByRegion (source, readout, newRegion);

    // set the mask to flag the excluded pixels
    psImageKeepCircle (source->maskObj, source->peak->xf, source->peak->yf, *fitRadius, "OR", markVal);

    return true;
}

// call this function whenever you (re)-define the EXT model
bool psphotSetRadiusFootprint (float *radius, pmReadout *readout, pmSource *source, psImageMaskType markVal, float factor) {

    psAssert (source, "source not defined??");
    psAssert (source->peak, "peak not defined??");

    pmPeak *peak = source->peak;

    // set the radius based on the footprint:
    if (!peak->footprint) return false;
    pmFootprint *footprint = peak->footprint;
    if (!footprint->spans) return false;
    if (footprint->spans->n < 1) return false;

    // find the max radius
    float rawRadius = 0.0;
    for (int j = 0; j < footprint->spans->n; j++) {
        pmSpan *span = footprint->spans->data[j];

        float dY  = span->y  - peak->yf;
        float dX0 = span->x0 - peak->xf;
        float dX1 = span->x1 - peak->xf;

        rawRadius = PS_MAX (rawRadius, hypot(dY, dX0));
        rawRadius = PS_MAX (rawRadius, hypot(dY, dX1));
    }
    if (isnan(rawRadius)) return false;
    rawRadius = PS_MIN (factor*rawRadius + EXT_FIT_PADDING, EXT_FIT_MAX_RADIUS);

    // redefine the pixels if needed
    pmSourceRedefinePixels (source, readout, peak->xf, peak->yf, rawRadius);

    // set the mask to flag the excluded pixels
    psImageKeepCircle (source->maskObj, peak->xf, peak->yf, rawRadius, "OR", markVal);

    *radius = rawRadius;
    return true;
}

// call this function whenever you (re)-define the EXT model
bool psphotMaskFootprint (pmReadout *readout, pmSource *source, psImageMaskType markVal) {

    psAssert (source, "source not defined??");
    psAssert (source->peak, "peak not defined??");

    pmPeak *peak = source->peak;

    // set the radius based on the footprint:
    if (!peak->footprint) return false;
    pmFootprint *footprint = peak->footprint;
    if (!footprint->spans) return false;
    if (footprint->spans->n < 1) return false;

    int Xo = source->maskObj->col0;
    int Yo = source->maskObj->row0;

    // mark all pixels 
    for (int j = 0; j < source->maskObj->numRows; j++) {
	for (int i = 0; i < source->maskObj->numCols; i++) {
	    source->maskObj->data.PS_TYPE_IMAGE_MASK_DATA[j][i] |= markVal;
	}
    }

    psImageMaskType clearVal = PS_NOT_IMAGE_MASK(markVal);

    for (int j = 0; j < footprint->spans->n; j++) {
        pmSpan *span = footprint->spans->data[j];

	// mask the rows before and after each span
	int minX = span->x0 - Xo - 2;
	int maxX = span->x1 - Xo + 2;
	int myY = span->y - Yo;

	// unmark pixels inside the footprint
	for (int i = minX; i <= maxX; i++) {
	    source->maskObj->data.PS_TYPE_IMAGE_MASK_DATA[myY][i] &= clearVal;
	}
    }
    return true;
}

// alternative EXT radius based on model guess (for use without footprints)
bool psphotSetRadiusModel (pmModel *model, pmReadout *readout, pmSource *source, psImageMaskType markVal, bool deep) {

    pmPeak *peak = source->peak;

    // set the fit radius based on the object flux limit and the model
    float flux = deep ? EXT_FIT_NSIGMA*EXT_FIT_SKY_SIG : 0.1 * model->params->data.F32[PM_PAR_I0];

    float rawRadius = model->class->modelRadius (model->params, flux);
    if (isnan(rawRadius)) return false;

    rawRadius = PS_MIN (rawRadius + EXT_FIT_PADDING, EXT_FIT_MAX_RADIUS);
    model->fitRadius = rawRadius;

    // redefine the pixels if needed
    pmSourceRedefinePixels (source, readout, peak->xf, peak->yf, model->fitRadius);

    // set the mask to flag the excluded pixels
    psImageKeepCircle (source->maskObj, peak->xf, peak->yf, model->fitRadius, "OR", markVal);
    return true;
}
