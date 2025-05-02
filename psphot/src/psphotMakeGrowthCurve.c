# include "psphotInternal.h"

bool psphotMakeGrowthCurve (pmReadout *readout, psMetadata *recipe, pmPSF *psf, psArray *sources) {

    bool status;

    psTimerStart ("psphot.growth");

    // set limits on the aperture magnitudes
    pmSourceMagnitudesInit (NULL, recipe);

    // bit-masks to test for good/bad pixels
    psImageMaskType maskVal = psMetadataLookupImageMask(&status, recipe, "MASK.PSPHOT");
    assert (maskVal);

    // bit-mask to mark pixels not used in analysis
    psImageMaskType markVal = psMetadataLookupImageMask(&status, recipe, "MARK.PSPHOT");
    assert (markVal);

    // maskVal is used to test for rejected pixels, and must include markVal
    maskVal |= markVal;

    // measure the aperture loss as a function of radius for PSF
    float REF_RADIUS = psMetadataLookupF32 (&status, recipe, "PSF_REF_RADIUS");
    float PSF_FIT_PAD   = psMetadataLookupF32 (&status, recipe, "PSF_FIT_PADDING");
    
    float gaussSigma = psMetadataLookupF32(&status, readout->analysis, "MOMENTS_GAUSS_SIGMA");
    if (!status) {
	gaussSigma = psMetadataLookupF32(&status, recipe, "MOMENTS_GAUSS_SIGMA");
    }
    float apScale = psMetadataLookupF32(&status, recipe, "PSF_APERTURE_SCALE");
    float PSF_APERTURE = (int)(apScale*gaussSigma);

    psf->growth = pmGrowthCurveAlloc (PSF_FIT_PAD, 100.0, REF_RADIUS);

    bool GROWTH_FROM_SOURCES = psMetadataLookupBool (&status, recipe, "GROWTH_FROM_SOURCES");
    if (!status) GROWTH_FROM_SOURCES = false;

    if (GROWTH_FROM_SOURCES) {
	bool INTERPOLATE_AP = psMetadataLookupBool (&status, recipe, "INTERPOLATE_AP");
	if (!pmGrowthCurveGenerateFromSources (readout, psf, sources, INTERPOLATE_AP, maskVal, markVal)) {
	    // psError(PSPHOT_ERR_APERTURE, false, "Fitting aperture corrections");
	    psWarning("Failed to measure the growth curve for aperture corrections (from sources)");
	    psFree(psf->growth); psf->growth = NULL;
	    return true;
	}

    } else {
	bool IGNORE_GROWTH = psMetadataLookupBool (&status, recipe, "IGNORE_GROWTH");
	if (!pmGrowthCurveGenerate (readout, psf, IGNORE_GROWTH, maskVal, markVal)) {
	    // psError(PSPHOT_ERR_APERTURE, false, "Fitting aperture corrections");
	    psWarning("Failed to measure the growth curve for aperture corrections (from model)");
	    psFree(psf->growth); psf->growth = NULL;
	    return true;
	}
    }

    psLogMsg ("psphot", PS_LOG_MINUTIA, "built growth curve: %f sec\n", psTimerMark ("psphot.growth"));

    float offset = pmGrowthCurveCorrect (psf->growth, PSF_APERTURE);
    psLogMsg ("psphot", PS_LOG_DETAIL, "correction from %f to %f: %f mags\n", PSF_APERTURE, REF_RADIUS, offset);

    return true;
}
