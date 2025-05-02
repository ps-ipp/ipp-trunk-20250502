# include "psphotInternal.h"

static int Nskip1 = 0;
static int Nskip2 = 0;
static int Nskip3 = 0;
static int Nskip4 = 0;
static int Nskip5 = 0;

# define SKIP(VALUE) { VALUE++; return false; }

bool psphotRadialProfile (pmSource *source, psMetadata *recipe, float skynoise, psImageMaskType maskVal) {

    bool status;

    // allocate pmSourceExtendedParameters, if not already defined
    if (!source->extpars) {
        source->extpars = pmSourceExtendedParsAlloc ();
    }

    // XXX these need to go into recipe values
    int Nsec = 24;
    float Rmax = 200;
    float fluxMin = 0.0;
    float fluxMax = source->peak->rawFlux;

    bool RAW_RADIUS = psMetadataLookupBool (&status, recipe, "EXTENDED_SOURCE_RAW_RADIUS");

    // generate a series of radial profiles at Nsec evenly spaced angles.  the profile flux
    // is measured by interpolation for small radii; for large radii, the pixels in a box
    // are averaged to increase the S/N
    if (!psphotRadialProfilesByAngles (source, Nsec, Rmax)) {
	psError (PS_ERR_UNKNOWN, false, "failed to measure radial profile for petrosian");
	SKIP (Nskip1);
    }
    // allocate: extpars->radFlux->radii,fluxes,theta

    // use the radial profiles to determine the radius of a given isophote.  this isophote
    // is used to determine the elliptical shape of the object, so it has a relatively high
    // value (nominally 25% of the peak)
    if (!psphotRadiiFromProfiles (source, fluxMin, fluxMax)) {
	psError (PS_ERR_UNKNOWN, false, "failed to measure isophotal radii from profiles");
	SKIP (Nskip2);
    }
    // allocate : extpars->radFlux->isophotalRadii (use profile->radii,fluxes)


    // convert the isophotal radius vs angle measurements to an elliptical contour
    if (!psphotEllipticalContour (source)) {
	// psLogMsg ("psphot", 3, "failed to measure elliptical contour");
	SKIP (Nskip3);
    }
    // use extpars->radFlux->isophotalRadii,theta (result in extpars->axes)

    // generate a single, normalized radial profile following the elliptical contours.
    // the radius is normalized by the axis ratio so that on the major axis, 1 pixel = 1 pixel
    if (!psphotEllipticalProfile (source, RAW_RADIUS)) {
	psError (PS_ERR_UNKNOWN, false, "failed to generate elliptical profile");
	SKIP (Nskip4);
    }
    // allocate extpars->ellipticalFlux->radiusElliptical,fluxElliptical (use axes to scale raw pixels)
  
    // generated profile in averaged bins
    if (!psphotRadialBins (recipe, source, Rmax, skynoise)) {
	psError (PS_ERR_UNKNOWN, false, "failed to generate radial bins");
	SKIP (Nskip5);
    }
    // allocate extpars->radProfile->binSB, binSBstdv, binSum, binFill, radialBins, area (small lengths)
    // use radiusElliptical, fluxElliptical, 
  
    return true;
}

void psphotRadialProfileShowSkips () {
# if (PS_TRACE_ON)
  fprintf (stderr, "radial profile skipped @ 1  : %d\n", Nskip1);
  fprintf (stderr, "radial profile skipped @ 2  : %d\n", Nskip2);
  fprintf (stderr, "radial profile skipped @ 3  : %d\n", Nskip3);
  fprintf (stderr, "radial profile skipped @ 4  : %d\n", Nskip4);
  fprintf (stderr, "radial profile skipped @ 5  : %d\n", Nskip5);
#endif
}
