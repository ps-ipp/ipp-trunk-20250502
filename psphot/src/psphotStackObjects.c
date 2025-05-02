# include "psphotInternal.h"

// set a consistent position
bool psphotStackObjectsUnifyPosition (psArray *objects) {

    // consistent position: AVERAGE or CHISQ?
    for (int i = 0; i < objects->n; i++) {
        pmPhotObj *object = objects->data[i];
	if (!object) continue;
	if (!object->sources) continue;

	int npts = 0;
	float x = 0.0;
	float y = 0.0;
	// measure the average position (weighted?)
	for (int j = 0; j < object->sources->n; j++) {

	    pmSource *source = object->sources->data[j];
	    if (!source) continue;
	    if (!source->peak) continue;

	    x += source->peak->xf;
	    y += source->peak->yf;
	    npts ++;
	}
	if (npts == 0) continue;

	x /= (float) npts;
	y /= (float) npts;

	// set the positions
	for (int j = 0; j < object->sources->n; j++) {

	    pmSource *source = object->sources->data[j];
	    if (!source) continue;
	    if (!source->peak) continue;

	    source->peak->xf = x;
	    source->peak->yf = y;
	    npts ++;
	}
	object->x = x;
	object->y = y;
    }

    psLogMsg ("psphot", PS_LOG_INFO, "updated positions\n");
    return true;
}

// mark good vs bad objects
bool psphotStackObjectsSelectForAnalysis (pmConfig *config, const pmFPAview *view, const char *filerule, psArray *objects) {

    bool status = false;

    // find the currently selected readout
    pmFPAfile *file = pmFPAfileSelectSingle(config->files, filerule, 0); // use the 0-index image to represent the image area
    psAssert (file, "missing file?");

    pmReadout *readout = pmFPAviewThisReadout(view, file->fpa);
    psAssert (readout, "missing readout?");

    // select the appropriate recipe information
    psMetadata *recipe  = psMetadataLookupPtr (&status, config->recipes, PSPHOT_RECIPE);
    psAssert (recipe, "missing recipe?");

    // option to limit analysis to a specific region
    char *region = psMetadataLookupStr (&status, recipe, "ANALYSIS_REGION");
    psRegion AnalysisRegion = psRegionForImage (readout->image, psRegionFromString (region));
    if (psRegionIsNaN (AnalysisRegion)) psAbort("analysis region mis-defined");

    // S/N limit to perform full non-linear fits
    float SN_LIM_PETRO  = psMetadataLookupF32 (&status, recipe, "EXTENDED_SOURCE_SN_LIM");
    float SN_LIM_RADIAL = psMetadataLookupF32 (&status, recipe, "RADIAL_APERTURES_SN_LIM");

    bool doPetroStars   = psMetadataLookupBool (&status, recipe, "PETROSIAN_FOR_STARS");

    for (int i = 0; i < objects->n; i++) {
        pmPhotObj *object = objects->data[i];
	if (!object) continue;
	if (!object->sources) continue;

	// we check each source for an object and keep the object if any source is valid

	bool keepObjectRadial = false;
	bool keepObjectPetro = false;
	for (int j = 0; j < object->sources->n; j++) {

	    pmSource *source = object->sources->data[j];
	    if (!source) continue;
	    if (!source->peak) continue;

	    // skip PSF-like and non-astronomical objects
	    if (source->type == PM_SOURCE_TYPE_DEFECT) continue;
	    if (source->type == PM_SOURCE_TYPE_SATURATED) continue;
	    if (source->mode & PM_SOURCE_MODE_DEFECT) continue;
	    if (source->mode & PM_SOURCE_MODE_SATSTAR) continue;
	    
	    // limit selection by analysis region (this automatically apply
	    if (source->peak->x < AnalysisRegion.x0) continue;
	    if (source->peak->y < AnalysisRegion.y0) continue;
	    if (source->peak->x > AnalysisRegion.x1) continue;
	    if (source->peak->y > AnalysisRegion.y1) continue;
	    
	    // SN limit tests for RADIAL APERTURES:
	    bool skipSourceRadial = false;
	    if (source->mode & PM_SOURCE_MODE_EXT_LIMIT) {
		skipSourceRadial = (source->moments->KronFlux < SN_LIM_RADIAL * source->moments->KronFluxErr);
	    } else {
		skipSourceRadial = (sqrt(source->peak->detValue) < SN_LIM_RADIAL);
	    }
	    if (!skipSourceRadial) keepObjectRadial = true;

	    // SN limit tests for PETRO
	    bool skipSourcePetro = false;
	    if (source->mode & PM_SOURCE_MODE_EXT_LIMIT) {
		skipSourcePetro = (source->moments->KronFlux < SN_LIM_PETRO * source->moments->KronFluxErr);
	    } else {
		skipSourcePetro = doPetroStars ? (sqrt(source->peak->detValue) < SN_LIM_PETRO) : true;
	    }
	    if (!skipSourcePetro) keepObjectPetro = true;
	}

	for (int j = 0; j < object->sources->n; j++) {
	    pmSource *source = object->sources->data[j];
	    if (!source) continue;
	    if (!source->peak) continue;

	    // we have to set a bit in either case to tell psphotExtendedSourceAnalysis to
	    // avoid the single-detection tests

	    if (keepObjectPetro) {
		source->tmpFlags |=  PM_SOURCE_TMPF_PETRO_KEEP;
		source->tmpFlags &= ~PM_SOURCE_TMPF_PETRO_SKIP;
	    } else {
		source->tmpFlags |=  PM_SOURCE_TMPF_PETRO_SKIP;
		source->tmpFlags &= ~PM_SOURCE_TMPF_PETRO_KEEP;
	    }	    

	    if (keepObjectRadial) {
		source->tmpFlags |=  PM_SOURCE_TMPF_RADIAL_KEEP;
		source->tmpFlags &= ~PM_SOURCE_TMPF_RADIAL_SKIP;
	    } else {
		source->tmpFlags |=  PM_SOURCE_TMPF_RADIAL_SKIP;
		source->tmpFlags &= ~PM_SOURCE_TMPF_RADIAL_KEEP;
	    }	    
	}
    }

    psLogMsg ("psphot", PS_LOG_INFO, "marked good vs bad objects\n");
    return true;
}
