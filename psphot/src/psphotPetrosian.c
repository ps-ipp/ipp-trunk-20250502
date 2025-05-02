# include "psphotInternal.h"

bool psphotPetrosian (pmSource *source, psMetadata *recipe, float skynoise, psImageMaskType maskVal) {

    // XXX these need to go into recipe values
    float Rmax = 200;

    psAssert (source->extpars, "need to run psphotRadialProfile first");
    psAssert (source->extpars->ellipticalFlux, "need to run psphotRadialProfile first");

    // integrate the radial profile for radial bins defined for the petrosian measurement:
    // SB_i (r_i) where \alpha r_i < r < \beta r_i
    if (!psphotPetrosianRadialBins (source, Rmax, skynoise)) {
	psError (PS_ERR_UNKNOWN, false, "failed to generate elliptical profile");
	return false;
    }
  
    // use the SB_i from above to calculate the petrosian radius and the flux within that radius
    if (!psphotPetrosianStats (source)) {
	psError (PS_ERR_UNKNOWN, false, "failed to generate elliptical profile");
	return false;
    }
  
    psTrace ("psphot", 3, "source at %f,%f: petrosian radius: %f, flux: %f, axis ratio: %f, angle: %f",
	     source->peak->xf, source->peak->yf, 
	     source->extpars->petrosianRadius, 
	     source->extpars->petrosianFlux, 
	     source->extpars->axes.minor/source->extpars->axes.major, 
	     source->extpars->axes.theta*PS_DEG_RAD);

    return true;
}
