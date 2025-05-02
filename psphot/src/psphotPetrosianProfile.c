# include "psphotInternal.h"

// generate the Petrosian radius and flux using elliptical contours

// XXX much of this function is focused on generating the clean contours, which can be used by 
// any number of aperture-like measurements.  probably will want to rename the pmPetrosian
// structure to something the pmRadialProfile

bool psphotPetrosianProfile (pmReadout *readout, pmSource *source, float skynoise) {

    // container to hold results from the radial profile analysis
    pmPetrosian *petrosian = pmPetrosianAlloc();

    // XXX these need to go into recipe values
    int Nsec = 24;
    float Rmax = 200;
    float fluxMin = 0.0;
    float fluxMax = source->peak->flux;

    // generate a series of radial profiles at Nsec evenly spaced angles.  the profile flux
    // is measured by interpolation for small radii; for large radii, the pixels in a box
    // are averaged to increase the S/N (XXX not yet done)
    if (!psphotRadialProfilesByAngles (source, petrosian, Nsec, Rmax)) {
	psError (PS_ERR_UNKNOWN, false, "failed to measure radial profile for petrosian");
	psFree (petrosian);
	return false;
    }

    // use the radial profiles to determine the radius of a given isophote.  this isophote
    // is used to determine the elliptical shape of the object, so it has a relatively high
    // value (nominally 50% of the peak)
    if (!psphotRadiiFromProfiles (source, petrosian, fluxMin, fluxMax)) {
	psError (PS_ERR_UNKNOWN, false, "failed to measure isophotal radii from profiles");
	psFree (petrosian);
	return false;
    }

    // convert the isophotal radius vs angle measurements to an elliptical contour
    if (!psphotEllipticalContour (source, petrosian)) {
	// psLogMsg ("psphot", 3, "failed to measure elliptical contour");
	psFree (petrosian);
	return false;
    }
  
    // generate a single, normalized radial profile following the elliptical contours.
    // the radius is normalized by the axis ratio so that on the major axis, 1 pixel = 1 pixel
    if (!psphotEllipticalProfile (source, petrosian)) {
	psError (PS_ERR_UNKNOWN, false, "failed to generate elliptical profile");
	psFree (petrosian);
	return false;
    }
  
    // integrate the radial profile for radial bins defined for the petrosian measurement:
    // SB_i (r_i) where \alpha r_i < r < \beta r_i
    if (!psphotPetrosianRadialBins (source, petrosian, Rmax, skynoise)) {
	psError (PS_ERR_UNKNOWN, false, "failed to generate elliptical profile");
	psFree (petrosian);
	return false;
    }
  
    // use the SB_i from above to calculate the petrosian radius and the flux within that radius
    if (!psphotPetrosianStats (source, petrosian)) {
	psError (PS_ERR_UNKNOWN, false, "failed to generate elliptical profile");
	psFree (petrosian);
	return false;
    }
  
    // XXX this will only work in the psphot context, not the psphotPetrosianStudy...
    // XXX add the petrosian to the pmSource structure...
    psphotVisualShowPetrosian (source, petrosian);

    psphotPetrosianFreeVectors(petrosian);

    psTrace ("psphot", 3, "source at %f,%f: petrosian radius: %f, flux: %f, axis ratio: %f, angle: %f",
	     source->peak->xf, source->peak->yf, petrosian->petrosianRadius, petrosian->petrosianFlux, petrosian->axes.minor/petrosian->axes.major, PS_DEG_RAD*petrosian->axes.theta);

    psFree (petrosian);
    return true;
}
