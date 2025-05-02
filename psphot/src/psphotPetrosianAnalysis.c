# include "psphotInternal.h"

// aperture-like measurements for extended sources
bool psphotPetrosianAnalysis (pmReadout *readout, psArray *sources, psMetadata *recipe) {

    bool status;

    // user-defined masks to test for good/bad pixels (build from recipe list if not yet set)
    psImageMaskType maskVal = psMetadataLookupImageMask(&status, recipe, "MASK.PSPHOT"); // Mask value for bad pixels
    assert (maskVal);

    // XXX temporary user-supplied systematic sky noise measurement (derive from background model)
    float skynoise = psMetadataLookupF32 (&status, recipe, "SKY.NOISE");

    // S/N limit to perform full non-linear fits
    float SN_LIM = psMetadataLookupF32 (&status, recipe, "EXTENDED_SOURCE_SN_LIM");

    // option to limit analysis to a specific region
    char *region = psMetadataLookupStr (&status, recipe, "ANALYSIS_REGION");
    psRegion AnalysisRegion = psRegionForImage (readout->image, psRegionFromString (region));
    if (psRegionIsNaN (AnalysisRegion)) psAbort("analysis region mis-defined");

    // source analysis is done in S/N order (brightest first)
    sources = psArraySort (sources, pmSourceSortByFlux);

    // choose the sources of interest
    for (int i = 0; i < sources->n; i++) {

	pmSource *source = sources->data[i];

	// skip PSF-like and non-astronomical objects
	if (source->type == PM_SOURCE_TYPE_STAR) continue;
	if (source->type == PM_SOURCE_TYPE_DEFECT) continue;
	if (source->type == PM_SOURCE_TYPE_SATURATED) continue;
	if (source->mode & PM_SOURCE_MODE_DEFECT) continue;
	if (source->mode & PM_SOURCE_MODE_SATSTAR) continue;
	if (!(source->mode & PM_SOURCE_MODE_EXT_LIMIT)) continue;

	// limit selection to some SN limit
	assert (source->peak); // how can a source not have a peak?
	if (source->peak->SN < SN_LIM) continue;

	// limit selection by analysis region
	if (source->peak->x < AnalysisRegion.x0) continue;
	if (source->peak->y < AnalysisRegion.y0) continue;
	if (source->peak->x > AnalysisRegion.x1) continue;
	if (source->peak->y > AnalysisRegion.y1) continue;

	// replace object in image
	if (source->tmpFlags & PM_SOURCE_TMPF_SUBTRACTED) {
	    pmSourceAdd (source, PM_MODEL_OP_FULL, maskVal);
	}

	psphotPetrosianProfile (readout, source, skynoise);

	pmSourceSub (source, PM_MODEL_OP_FULL, maskVal);
    }

    psphotVisualShowResidualImage (readout, false);
    return true;
}
