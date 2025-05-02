# include "psphotInternal.h"

// XXX option to choose a consistent elliptical contour
// XXX SDSS uses the r-band petrosian radius to measure petrosian fluxes in all bands

// aperture-like measurements for extended sources
bool psphotExtendedSourceAnalysisByObject (pmConfig *config, psArray *objects, const pmFPAview *view, const char *filerule) {

    bool status;
    int Next = 0;
    int Npetro = 0;
    int Nannuli = 0;

    psTimerStart ("psphot.extended");

    // select the appropriate recipe information
    psMetadata *recipe  = psMetadataLookupPtr (&status, config->recipes, PSPHOT_RECIPE);
    psAssert (recipe, "missing recipe?");

    // perform full non-linear fits / extended source analysis?
    if (!psMetadataLookupBool (&status, recipe, "EXTENDED_SOURCE_ANALYSIS")) {
	psLogMsg ("psphot", PS_LOG_INFO, "skipping extended source measurements\n");
	return true;
    }

    // user-defined masks to test for good/bad pixels (build from recipe list if not yet set)
    psImageMaskType maskVal = psMetadataLookupImageMask(&status, recipe, "MASK.PSPHOT"); // Mask value for bad pixels
    assert (maskVal);

    // XXX require petrosian analysis for non-linear fits? 

    // XXX temporary user-supplied systematic sky noise measurement (derive from background model)
    float skynoise = psMetadataLookupF32 (&status, recipe, "SKY.NOISE");

    // S/N limit to perform full non-linear fits
    float SN_LIM = psMetadataLookupF32 (&status, recipe, "EXTENDED_SOURCE_SN_LIM");

    // which extended source analyses should we perform?
    bool doPetrosian    = psMetadataLookupBool (&status, recipe, "EXTENDED_SOURCE_PETROSIAN");
    bool doAnnuli       = psMetadataLookupBool (&status, recipe, "EXTENDED_SOURCE_ANNULI");

    // number of images used to define sources
    int nImages = psphotFileruleCount(config, filerule);

    // generate look-up arrays for readouts
    psArray *readouts = psArrayAlloc(nImages);
    for (int i = 0; i < nImages; i++) {

	// find the currently selected readout
	pmFPAfile *file = pmFPAfileSelectSingle(config->files, filerule, i); // File of interest
	psAssert (file, "missing file?");

	pmReadout *readout = pmFPAviewThisReadout(view, file->fpa);
	psAssert (readout, "missing readout?");

	psLogMsg("psphot", PS_LOG_INFO, "petrosians for image %d", i);
	psphotVisualShowImage(readout);

	readouts->data[i] = psMemIncrRefCounter(readout);
    }

    // source analysis is done in S/N order (brightest first)
    objects = psArraySort (objects, pmPhotObjSortByFlux);

    // process the objects in order.  
    for (int i = 0; i < objects->n; i++) {
        pmPhotObj *object = objects->data[i];
	if (!object) continue;
	if (!object->sources) continue;

	// we need to decide for an object if we are going to measure all sources or not
	// simple rule : if *any* of the sources would be measured, measure the object

	// choose the sources of interest
	bool measureSource = false;
	for (int j = 0; !measureSource && (j < object->sources->n); j++) {

	    pmSource *source = object->sources->data[j];

	    // skip PSF-like and non-astronomical objects
	    if (source->type == PM_SOURCE_TYPE_STAR) continue;
	    if (source->type == PM_SOURCE_TYPE_DEFECT) continue;
	    if (source->type == PM_SOURCE_TYPE_SATURATED) continue;
	    if (source->mode & PM_SOURCE_MODE_DEFECT) continue;
	    if (source->mode & PM_SOURCE_MODE_SATSTAR) continue;
	    if (!(source->mode & PM_SOURCE_MODE_EXT_LIMIT)) continue;

	    // limit selection to some SN limit
	    assert (source->peak); // how can a source not have a peak?
	    if (sqrt(source->peak->detValue) < SN_LIM) continue;
	    measureSource = true;
	}
	if (!measureSource) continue;

	// choose the sources of interest
	for (int j = 0; j < object->sources->n; j++) {

	    pmSource *source = object->sources->data[j];
	    psAssert (source, "programming error"); // all entries in object->sources must exist, right?
	    psAssert (source->peak, "programming error"); // how can a source not have a peak?

	    // replace object in image
	    if (source->tmpFlags & PM_SOURCE_TMPF_SUBTRACTED) {
		pmSourceAdd (source, PM_MODEL_OP_FULL, maskVal);
	    }
	    Next ++;

	    int index = source->imageID;
	    pmReadout *readout = readouts->data[index];

	    // force source image to be a bit larger...
	    float radius = source->peak->xf - source->pixels->col0;
	    radius = PS_MAX (radius, source->peak->yf - source->pixels->row0);
	    radius = PS_MAX (radius, source->pixels->numRows - source->peak->yf + source->pixels->row0);
	    radius = PS_MAX (radius, source->pixels->numCols - source->peak->xf + source->pixels->col0);
	    pmSourceRedefinePixels (source, readout, source->peak->xf, source->peak->yf, 1.5*radius);

	    // if we request any of these measurements, we require the radial profile
	    if (doPetrosian || doAnnuli) {
		if (!psphotRadialProfile (source, recipe, skynoise, maskVal)) {
		    // all measurements below require the radial profile; skip them all
		    // re-subtract the object, leave local sky
		    psTrace ("psphot", 5, "FAILED radial profile for source at %7.1f, %7.1f", source->moments->Mx, source->moments->My);
		    pmSourceSub (source, PM_MODEL_OP_FULL, maskVal);
		    continue;
		} else {
		    psTrace ("psphot", 5, "measured radial profile for source at %7.1f, %7.1f", source->moments->Mx, source->moments->My);
		    Nannuli ++;
		    source->mode |= PM_SOURCE_MODE_RADIAL_FLUX;
		}
	    }

	    // Petrosian Mags
	    if (doPetrosian) {
		if (!psphotPetrosian (source, recipe, skynoise, maskVal)) {
		    psTrace ("psphot", 5, "FAILED petrosian flux & radius for source at %7.1f, %7.1f", source->moments->Mx, source->moments->My);
		} else {
		    psTrace ("psphot", 5, "measured petrosian flux & radius for source at %7.1f, %7.1f", source->moments->Mx, source->moments->My);
		    Npetro ++;
		    source->mode |= PM_SOURCE_MODE_EXTENDED_STATS;
		}
	    }

	    // re-subtract the object, leave local sky
	    pmSourceSub (source, PM_MODEL_OP_FULL, maskVal);

	    if (source->extpars) {
		psFree(source->extpars->radFlux);
		psFree(source->extpars->ellipticalFlux);
		psFree(source->extpars->petProfile);
	    }
	}
    }

    psLogMsg ("psphot", PS_LOG_INFO, "extended source analysis: %f sec for %d objects\n", psTimerMark ("psphot.extended"), Next);
    psLogMsg ("psphot", PS_LOG_INFO, "  %d petrosian\n", Npetro);
    psLogMsg ("psphot", PS_LOG_INFO, "  %d annuli\n", Nannuli);

    psFree(readouts);
    return true;
}
