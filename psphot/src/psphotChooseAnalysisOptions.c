# include "psphotInternal.h"

// choose which sources will be processed for the petrosian radii and/or galaxy/trail model fits
// a source for which we want galaxy models or petrosians gets 

// currently we also skip the following items (regardless of recipe values):
// TYPE_DEFECT, TYPE_SATURATED, MODE_DEFECT, MODE_SATSTAR, SATSTAR_PROFILE 

// RULES:
// * global override:
// ** PSPHOT.EXT.FIT.ALL.SOURCES
// ** PSPHOT.EXT.FIT.ALL.THRESH (density limit)

// * select by Star/Galaxy?
// ** PSPHOT.EXT.NSIGMA.LIMIT (sets EXT_LIMIT bit -- star or galaxy)
// ** EXT.NSIGMA.LIMIT.USE (apply or ignore EXT.NSIGMA.LIMIT above?)

// * select by S/N?
// ** EXTENDED_SOURCE_MODELS : SNLIM (per model value)

// * select by Flux?
// * needs to depend on the filter
// * define a metadata block 
// -- EXT.ANALYSIS.MAG.LIMITS (metadata block per filter)

// * select by galactic latitude (global override)?
// * if (|b| > LIM) do not limit the number of sources by density 
// -- GLAT_MAX

// * de-select by density (global override)?
// ??

// for which sources do we want to run the extended source analysis (petrosian and/or galaxy fits)?
bool psphotChooseAnalysisOptions (pmConfig *config, const pmFPAview *view, const char *filerule)
{
    bool status = true;

    fprintf (stdout, "\n");
    psLogMsg ("psphot", PS_LOG_INFO, "--- psphot Choose Analysis Options ---");

    // select the appropriate recipe information
    psMetadata *recipe  = psMetadataLookupPtr (&status, config->recipes, PSPHOT_RECIPE);
    psAssert (recipe, "missing recipe?");

    int num = psphotFileruleCount(config, filerule);

    // skip the chisq image (optionally?)
    int chisqNum = psMetadataLookupS32 (&status, config->arguments, "PSPHOT.CHISQ.NUM");
    if (!status) chisqNum = -1;

    // loop over the available readouts
    for (int i = 0; i < num; i++) {
        if (i == chisqNum) continue; // skip chisq image
        if (!psphotChooseAnalysisOptionsReadout (config, view, filerule, i, recipe)) {
	    psError (PSPHOT_ERR_CONFIG, false, "failed on source size analysis for %s entry %d", filerule, i);
            return false;
        }
    }
    return true;
}

static bool GetGalacticCoords (psSphere *ptGal, psSphere *ptSky, psSphereRot *toGal, pmChip *chip, float xPos, float yPos);

// this function use an internal flag to mark sources which have already been measured
bool psphotChooseAnalysisOptionsReadout(pmConfig *config, const pmFPAview *view, const char *filerule, int index, psMetadata *recipe)
{
    bool status;

    psTimerStart ("psphot.options");

    // find the currently selected readout
    pmFPAfile *file = pmFPAfileSelectSingle(config->files, filerule, index); // File of interest
    psAssert (file, "missing file?");

    pmReadout *readout = pmFPAviewThisReadout(view, file->fpa);
    psAssert (readout, "missing readout?");

    pmChip *chip = pmFPAviewThisChip(view, file->fpa);
    psAssert (chip, "missing chip?");

    pmDetections *detections = psMetadataLookupPtr (&status, readout->analysis, "PSPHOT.DETECTIONS");
    psAssert (detections, "missing detections?");

    psArray *sources = detections->allSources;
    psAssert (sources, "missing sources?");

    if (!sources->n) {
        psLogMsg ("psphot", PS_LOG_INFO, "no sources, skipping source size");
        return true;
    }

    // we do the petrosian analysis for the same sources as the extended source fits IFF
    // this recipe value is turned on (otherwise we only make a selection based on the mag limits)
    bool doPetrosian    = psMetadataLookupBool (&status, recipe, "EXTENDED_SOURCE_PETROSIAN");

    // Option to enable fitting of all objects with extended model.
    bool extFitAll = psMetadataLookupBool(&status, recipe, "PSPHOT.EXT.FIT.ALL.SOURCES");
    assert (status);

    // Fitting everything is fine, but if the source density is high, we probably shouldn't.
    float extFitAllThresh = psMetadataLookupF32(&status, recipe, "PSPHOT.EXT.FIT.ALL.THRESH");
    assert (status);
    
    // Determine if this readout is above the threshold to ext fit all sources
    if (extFitAll) {
      float maskFrac = psMetadataLookupF32(&status,readout->analysis,"READOUT.MASK.FRAC");
      if (status) maskFrac = 0.0;
      if (sources->n * (1.0 - maskFrac) > extFitAllThresh) {
	extFitAll = false;
      }
    }

    // use EXT_LIMIT bit to select objects?
    bool useEXT_LIMIT = psMetadataLookupBool(&status, recipe, "EXT.NSIGMA.LIMIT.USE");
    assert (status);

    float SN_LIM = psMetadataLookupF32 (&status, recipe, "EXTENDED_SOURCE_SN_LIM");
    assert (status);

    // use GAL_LIMIT to select / skip objects?
    psSphereRot *toGal = NULL;
    float GAL_LIMIT = 0.0;
    float GAL_LIMIT_BULGE = 0.0;
    float GAL_LIMIT_SIGMA2 = 0.0;
    bool useGAL_LIMIT = psMetadataLookupBool(&status, recipe, "EXT.FIT.MIN.GAL.LIMIT.USE");
    assert (status);
    if (useGAL_LIMIT) {
	toGal = psSphereRotICRSToGalactic();
	GAL_LIMIT = psMetadataLookupF32(&status, recipe, "EXT.FIT.MIN.GAL.LIMIT");
        assert (status);
        GAL_LIMIT = DEG_TO_RAD(GAL_LIMIT);
	GAL_LIMIT_BULGE = psMetadataLookupF32(&status, recipe, "EXT.FIT.MIN.GAL.LIMIT.BULGE");
        assert (status);
        GAL_LIMIT_BULGE = DEG_TO_RAD(GAL_LIMIT_BULGE);
	float GAL_LIMIT_SIGMA = psMetadataLookupF32(&status, recipe, "EXT.FIT.MIN.GAL.LIMIT.BULGE.SIGMA");
        assert (status);
        assert(GAL_LIMIT_SIGMA > 0);
        GAL_LIMIT_SIGMA = DEG_TO_RAD(GAL_LIMIT_SIGMA);
	GAL_LIMIT_SIGMA2 = GAL_LIMIT_SIGMA * GAL_LIMIT_SIGMA;
    }

    // use MAG.LIMITS
    psMetadata *magLimits = psMetadataLookupPtr (&status, recipe, "EXT.ANALYSIS.MAG.LIMITS");
    if (!status || !magLimits) {   
        psLogMsg ("psphot", PS_LOG_WARN, "EXT.ANALYSIS.MAG.LIMITS not found in the recipe, will use other criteria.");
        magLimits = NULL;
    }

    float petroFluxLim = NAN;
    float extFitFluxLim = NAN;

    if (magLimits) {
	float extFitMagLimDefault = NAN;
	float extFitMagLim = NAN;
	float petroMagLimDefault = NAN;
	float petroMagLim = NAN;

	// match to the given filter
	psString filterID = psMetadataLookupStr(&status, file->fpa->concepts, "FPA.FILTERID");
	psAssert (filterID, "missing FPA.FILTERID?");

        psMetadataIterator *iter = psMetadataIteratorAlloc(magLimits, PS_LIST_HEAD, NULL);
        psMetadataItem *item = NULL;
        while ((item = psMetadataGetAndIncrement (iter)) != NULL) {
            if (item->type != PS_DATA_METADATA) {
                psAbort ("Invalid type for EXT.ANALYSIS.MAG.LIMITS: %s, not a metadata folder", item->name);
            }
            psString thisFilter = psMetadataLookupStr (&status, item->data.md, "FILTER.ID");
            psAssert(thisFilter, "missing FILTER.ID");

	    // find a matching filter or default to 'any'
	    if (!strcasecmp (thisFilter, "any")) {
		psString petroMagLimStr = psMetadataLookupStr (&status, item->data.md, "MAG.LIMIT.PETRO");
		psAssert(petroMagLimStr, "missing MAG.LIMIT.PETRO");
		petroMagLimDefault = atof (petroMagLimStr);

		psString extFitMagLimStr = psMetadataLookupStr (&status, item->data.md, "MAG.LIMIT.EXTFIT");
		psAssert(extFitMagLimStr, "missing MAG.LIMIT.EXTFIT");
		extFitMagLimDefault = atof (extFitMagLimStr);
	    }

	    if (!strcasecmp (thisFilter, filterID)) {
		psString petroMagLimStr = psMetadataLookupStr (&status, item->data.md, "MAG.LIMIT.PETRO");
		psAssert(petroMagLimStr, "missing MAG.LIMIT.PETRO");
		petroMagLim = atof (petroMagLimStr);

		psString extFitMagLimStr = psMetadataLookupStr (&status, item->data.md, "MAG.LIMIT.EXTFIT");
		psAssert(extFitMagLimStr, "missing MAG.LIMIT.EXTFIT");
		extFitMagLim = atof (extFitMagLimStr);
		break;
	    }
	}
        psFree(iter);
	if (!isfinite (petroMagLim)) petroMagLim = petroMagLimDefault;
	if (!isfinite (extFitMagLim)) extFitMagLim = extFitMagLimDefault;

	// now I need to convert the mag limits into instrumental flux limits
	// I need to get a zero point and exposure time for this image
	
	// select the exposure time
	float exptime = psMetadataLookupF32(&status, file->fpa->concepts, "CELL.EXPOSURE");
	if (!status) {
	    exptime = psMetadataLookupF32(&status, file->fpa->concepts, "FPA.EXPOSURE");
	    psAssert (status, "missing CELL.EXPOSURE and FPA.EXPOSURE?");
	}

	// select the exposure time
	float zeropt = psMetadataLookupF32(&status, file->fpa->concepts, "FPA.ZP");
	psAssert (status, "missing FPA.ZP?");

	petroFluxLim = exptime * pow (10.0, 0.4*(zeropt - petroMagLim));
	extFitFluxLim = exptime * pow (10.0, 0.4*(zeropt - extFitMagLim));
    }

    pmSourceTmpF clearBits = ~(PM_SOURCE_TMPF_EXT_FIT | PM_SOURCE_TMPF_PETRO);
    for (psS32 i = 0 ; i < sources->n ; i++) {

        pmSource *source = (pmSource *) sources->data[i];

	// clear the 2 relevant bits
	source->tmpFlags &= clearBits;

        // skip PSF-like and non-astronomical objects
        if (source->type == PM_SOURCE_TYPE_DEFECT) continue;
        if (source->type == PM_SOURCE_TYPE_SATURATED) continue;
	if (source->mode & PM_SOURCE_MODE_DEFECT) continue;
	if (source->mode & PM_SOURCE_MODE_SATSTAR) continue;

	// skip saturated stars modeled with a radial profile 
        if (source->mode2 & PM_SOURCE_MODE2_SATSTAR_PROFILE) continue;

        // skip sources without a psf model
        if (source->modelPSF == NULL) continue;

	// Do the fits if the recipe requests we do extended source fits to everything
	if (extFitAll) {
	  source->tmpFlags |= PM_SOURCE_TMPF_EXT_FIT;
	  if (doPetrosian) source->tmpFlags |= PM_SOURCE_TMPF_PETRO;
	  continue;
	}

	if (useEXT_LIMIT) {
	    if (!(source->mode & PM_SOURCE_MODE_EXT_LIMIT)) {
		// not extended so skip
		continue;
	    }
	}

	if (useGAL_LIMIT) {
	    psSphere ptGal, ptSky;
	    GetGalacticCoords (&ptGal, &ptSky, toGal, chip, source->peak->xf, source->peak->yf);
            float l = ptGal.r;
            float b_min = GAL_LIMIT + GAL_LIMIT_BULGE * exp(-0.5*(l*l/GAL_LIMIT_SIGMA2));
            if (fabs(ptGal.d) < b_min) continue;
	    // include an exception for low density skycells below the limit?
	}

	// for petro and extFit, we will either use the mag limits or the S/N
        if (doPetrosian) {
            if (isfinite(petroFluxLim)) {
                if (source->moments->KronFlux > petroFluxLim) {
                    source->tmpFlags |= PM_SOURCE_TMPF_PETRO;
                }
            } else if (source->moments->KronFlux > SN_LIM * source->moments->KronFluxErr) {
                source->tmpFlags |= PM_SOURCE_TMPF_PETRO;
	    }
	}
	if (isfinite(extFitFluxLim)) {
	    if (source->moments->KronFlux > extFitFluxLim) {
		source->tmpFlags |= PM_SOURCE_TMPF_EXT_FIT;
	    }
	} else {
	    if (source->moments->KronFlux > SN_LIM * source->moments->KronFluxErr) {
		source->tmpFlags |= PM_SOURCE_TMPF_EXT_FIT;
	    }
	}
    }

    psLogMsg ("psphot.options", PS_LOG_WARN, "choose analysis options for %ld sources: %f sec\n", sources->n, psTimerMark ("psphot.options"));

    psFree(toGal);

    return true;
}

// this function use an internal flag to mark sources which have already been measured
bool psphotChooseAnalysisOptionsByObject(pmConfig *config, const pmFPAview *view, const char *filerule, psArray *objects)
{
    bool status;

    psTimerStart ("psphot.options");

    fprintf (stdout, "\n");
    psLogMsg ("psphot", PS_LOG_INFO, "--- psphot Choose Analysis Options (By Object) ---");

    // select the appropriate recipe information
    psMetadata *recipe  = psMetadataLookupPtr (&status, config->recipes, PSPHOT_RECIPE);
    psAssert (recipe, "missing recipe?");

    // S/N lim to perform radial aperture analysis
    float SN_LIM_RADIAL = psMetadataLookupF32 (&status, recipe, "RADIAL_APERTURES_SN_LIM");

    // we do the petrosian analysis for the same sources as the extended source fits IFF
    // this recipe value is turned on (otherwise we only make a selection based on the mag limits)
    bool doPetrosian    = psMetadataLookupBool (&status, recipe, "EXTENDED_SOURCE_PETROSIAN");

    // Option to enable fitting of all objects with extended model.
    bool extFitAll = psMetadataLookupBool(&status, recipe, "PSPHOT.EXT.FIT.ALL.SOURCES");
    assert (status);

    // Fitting everything is fine, but if the source density is high, we probably shouldn't.
    float extFitAllThresh = psMetadataLookupF32(&status, recipe, "PSPHOT.EXT.FIT.ALL.THRESH");
    assert (status);
    
    // use EXT_LIMIT bit to select objects?
    bool useEXT_LIMIT = psMetadataLookupBool(&status, recipe, "EXT.NSIGMA.LIMIT.USE");
    assert (status);

    float SN_LIM = psMetadataLookupF32 (&status, recipe, "EXTENDED_SOURCE_SN_LIM");
    assert (status);

    // use GAL_LIMIT to select / skip objects?
    psSphereRot *toGal = NULL;
    float GAL_LIMIT = 0.0;
    float GAL_LIMIT_BULGE = 0.0;
    float GAL_LIMIT_SIGMA2 = 0.0;
    bool useGAL_LIMIT = psMetadataLookupBool(&status, recipe, "EXT.FIT.MIN.GAL.LIMIT.USE");
    assert (status);
    if (useGAL_LIMIT) {
	toGal = psSphereRotICRSToGalactic();
	GAL_LIMIT = psMetadataLookupF32(&status, recipe, "EXT.FIT.MIN.GAL.LIMIT");
        assert (status);
        GAL_LIMIT = DEG_TO_RAD(GAL_LIMIT);
	GAL_LIMIT_BULGE = psMetadataLookupF32(&status, recipe, "EXT.FIT.MIN.GAL.LIMIT.BULGE");
        assert (status);
        GAL_LIMIT_BULGE = DEG_TO_RAD(GAL_LIMIT_BULGE);
	float GAL_LIMIT_SIGMA = psMetadataLookupF32(&status, recipe, "EXT.FIT.MIN.GAL.LIMIT.BULGE.SIGMA");
        assert (status);
        assert(GAL_LIMIT_SIGMA > 0);
        GAL_LIMIT_SIGMA = DEG_TO_RAD(GAL_LIMIT_SIGMA);
	GAL_LIMIT_SIGMA2 = GAL_LIMIT_SIGMA * GAL_LIMIT_SIGMA;
    }

    // use MAG.LIMITS
    psMetadata *magLimits = psMetadataLookupPtr (&status, recipe, "EXT.ANALYSIS.MAG.LIMITS");
    if (!status || !magLimits) {   
        psLogMsg ("psphot", PS_LOG_WARN, "EXT.ANALYSIS.MAG.LIMITS not found in the recipe, will use other criteria.");
        magLimits = NULL;
    }

    int num = psphotFileruleCount(config, filerule);

    int chisqNum = psMetadataLookupS32 (&status, config->arguments, "PSPHOT.CHISQ.NUM");
    if (!status) chisqNum = -1;

    // Find the filter for each of the inputs in fpa concepts
    psArray *inputFilters = psArrayAlloc(num);
    psArray *chips = psArrayAlloc(num);
    psVector *zeropt = psVectorAlloc (inputFilters->n, PS_TYPE_F32);
    psVector *exptime = psVectorAlloc (inputFilters->n, PS_TYPE_F32);

    // we will not use the chisq image to set the fitting limits
    if (chisqNum >= 0) {
      inputFilters->data[chisqNum] = psStringCopy("chisq");
      chips->data[chisqNum] = NULL;
      zeropt->data.F32[chisqNum] = NAN;
      exptime->data.F32[chisqNum] = NAN;
    }

    // get the needed metadata for the non-chisq images
    for (int i = 0 ; i < num; i++) {
      if (i == chisqNum) continue;

      pmFPAfile *file = pmFPAfileSelectSingle(config->files, filerule, i);
      psAssert (file, "missing file?");

      pmChip *chip = pmFPAviewThisChip(view, file->fpa);
      psAssert (chip, "missing chip?");
      chips->data[i] = psMemIncrRefCounter (chip);

      pmReadout *readout = pmFPAviewThisReadout(view, file->fpa);
      psAssert (readout, "missing readout?");

      pmDetections *detections = psMetadataLookupPtr (&status, readout->analysis, "PSPHOT.DETECTIONS");
      psAssert (detections, "missing detections?");

      psArray *sources = detections->allSources;
      psAssert (sources, "missing sources?");

      float maskFrac = psMetadataLookupF32(&status,readout->analysis,"READOUT.MASK.FRAC");
      if (!status) maskFrac = 0.0;
      if (sources->n * (1.0 - maskFrac) > extFitAllThresh) {
	extFitAll = false;
      }

      // select the filterID for this image
      char *filterID = psMetadataLookupStr(&status, file->fpa->concepts, "FPA.FILTERID");
      psAssert (status, "missing FPA.FILTERID?");
      inputFilters->data[i] = psStringCopy (filterID);

      // select the exposure time for this image
      exptime->data.F32[i] = psMetadataLookupF32(&status, file->fpa->concepts, "CELL.EXPOSURE");
      if (!status) {
	exptime->data.F32[i] = psMetadataLookupF32(&status, file->fpa->concepts, "FPA.EXPOSURE");
	psAssert (status, "missing CELL.EXPOSURE and FPA.EXPOSURE?");
      }

      // select the zero point for this image
      zeropt->data.F32[i] = psMetadataLookupF32(&status, file->fpa->concepts, "FPA.ZP");
      psAssert (status, "missing FPA.ZP?");
    }

    // find extFitFluxLim->data.F32[i] for i == image number
    psVector *extFitFluxLim = NULL;
    psVector *petroFluxLim = NULL;
    if (magLimits) {
	extFitFluxLim = psVectorAlloc (inputFilters->n, PS_TYPE_F32);
        petroFluxLim = psVectorAlloc (inputFilters->n, PS_TYPE_F32);
	psVectorInit (extFitFluxLim, NAN);
	psVectorInit (petroFluxLim, NAN);

	float extFitMagLimDefault = NAN;
	float petroMagLimDefault = NAN;

	// match mag limits (flux limits) to the filters
        psMetadataIterator *iter = psMetadataIteratorAlloc(magLimits, PS_LIST_HEAD, NULL);
        psMetadataItem *item = NULL;
        while ((item = psMetadataGetAndIncrement (iter)) != NULL) {
            if (item->type != PS_DATA_METADATA) {
                psAbort ("Invalid type for EXT.ANALYSIS.MAG.LIMITS: %s, not a metadata folder", item->name);
            }
            psString thisFilter = psMetadataLookupStr (&status, item->data.md, "FILTER.ID");
            psAssert(thisFilter, "missing FILTER.ID");

	    // save the default value to assign to unset filters
	    if (!strcasecmp (thisFilter, "any")) {
		psString magString = psMetadataLookupStr (&status, item->data.md, "MAG.LIMIT.EXTFIT");
		psAssert(magString, "missing MAG.LIMIT.EXTFIT");
		extFitMagLimDefault = atof (magString);

		magString = psMetadataLookupStr (&status, item->data.md, "MAG.LIMIT.PETRO");
		psAssert(magString, "missing MAG.LIMIT.PETRO");
		petroMagLimDefault = atof (magString);
		continue;
	    }

	    // not every entry in the metadata block needs to match to an image in our list
	    for (int i = 0; i < num; i++) {
		if (i == chisqNum) continue;
		if (!strcasecmp (thisFilter, inputFilters->data[i])) {
		    psString magString = psMetadataLookupStr (&status, item->data.md, "MAG.LIMIT.PETRO");
		    psAssert(magString, "missing MAG.LIMIT.PETRO");
		    float magvalue = atof (magString);
		    petroFluxLim->data.F32[i] = exptime->data.F32[i] * pow (10.0, 0.4*(zeropt->data.F32[i] - magvalue));

		    magString = psMetadataLookupStr (&status, item->data.md, "MAG.LIMIT.EXTFIT");
		    psAssert(magString, "missing MAG.LIMIT.EXTFIT");
		    magvalue = atof (magString);
		    extFitFluxLim->data.F32[i] = exptime->data.F32[i] * pow (10.0, 0.4*(zeropt->data.F32[i] - magvalue));
		    break;
		}
	    }
	}
        psFree(iter);

	for (int i = 0; i < num; i++) {
	    if (i == chisqNum) continue;
	    if (!isfinite(petroFluxLim->data.F32[i])) {
                petroFluxLim->data.F32[i] = exptime->data.F32[i] * pow (10.0, 0.4*(zeropt->data.F32[i] - petroMagLimDefault));
            }
	    if (!isfinite(extFitFluxLim->data.F32[i])) {
                extFitFluxLim->data.F32[i] = exptime->data.F32[i] * pow (10.0, 0.4*(zeropt->data.F32[i] - extFitMagLimDefault));
            }
	}
    }

    pmSourceTmpF clearBits = ~(PM_SOURCE_TMPF_EXT_FIT | PM_SOURCE_TMPF_PETRO);

    for (int i = 0; i < objects->n; i++) {
        pmPhotObj *object = objects->data[i];
	if (!object) continue;
	if (!object->sources) continue;

	// we check each source for an object and keep the object if any source is valid

	bool doObjectRadial = false;
	bool doObjectExtFit = false;
	bool doObjectPetrosian = false;
	for (int j = 0; !doObjectExtFit && !doObjectRadial && !doObjectPetrosian && (j < object->sources->n); j++) {

	    pmSource *source = object->sources->data[j];
	    if (!source) continue;
	    if (!source->peak) continue;
	    if (source->imageID < 0) continue; // skip sources which come from other images?
	    if (source->imageID >= num) continue; // skip sources which come from other images?

	    // skip PSF-like and non-astronomical objects
	    if (source->type == PM_SOURCE_TYPE_DEFECT) continue;
	    if (source->type == PM_SOURCE_TYPE_SATURATED) continue;
	    if (source->mode & PM_SOURCE_MODE_DEFECT) continue;
	    if (source->mode & PM_SOURCE_MODE_SATSTAR) continue;
	
	    // skip saturated stars modeled with a radial profile 
	    if (source->mode2 & PM_SOURCE_MODE2_SATSTAR_PROFILE) continue;
	
	    // XXX should I fit all even if one of the detections matches the above?

	    // check on radial aperture analysis (fewer options for now)
	    if (source->mode & PM_SOURCE_MODE_EXT_LIMIT) {
		bool doSourceRadial = (source->moments->KronFlux > SN_LIM_RADIAL * source->moments->KronFluxErr);
		doObjectRadial = doObjectRadial | doSourceRadial;
	    } else {
		bool doSourceRadial = (sqrt(source->peak->detValue) > SN_LIM_RADIAL);
		doObjectRadial = doObjectRadial | doSourceRadial;
	    }

	    // Do the fits if the recipe requests we do extended source fits to everything
	    if (extFitAll) {
		doObjectExtFit = true;
		doObjectPetrosian = doPetrosian;
		continue;
	    }

	    if (useEXT_LIMIT) {
		if (!(source->mode & PM_SOURCE_MODE_EXT_LIMIT)) {
		    continue; // not extended so skip
		}
	    }

	    int imageID = source->imageID;

	    if (useGAL_LIMIT) {
		psSphere ptGal, ptSky;
		GetGalacticCoords (&ptGal, &ptSky, toGal, chips->data[imageID], source->peak->xf, source->peak->yf);
                float l = ptGal.r;
                float b_min = GAL_LIMIT + GAL_LIMIT_BULGE * exp(-0.5*(l*l/GAL_LIMIT_SIGMA2));
		if (fabs(ptGal.d) < b_min) continue;
	    }

	    float fluxLim = NAN;
	    // for petro and extFit, we will either use the mag limits or the S/N
            if (doPetrosian) {
                fluxLim = petroFluxLim ? petroFluxLim->data.F32[imageID] : NAN;
                if (isfinite(fluxLim)) {
                    if (source->moments->KronFlux > extFitFluxLim->data.F32[imageID]) {
                        doObjectPetrosian = true;
                    }
                } else {
                    if (source->moments->KronFlux > SN_LIM * source->moments->KronFluxErr) {
                        doObjectPetrosian = true;
                    }
                }
	    }
	    fluxLim = extFitFluxLim ? extFitFluxLim->data.F32[imageID] : NAN;
	    // for petro and extFit, we will either use the mag limits or the S/N
	    if (isfinite(fluxLim)) {
		if (source->moments->KronFlux > extFitFluxLim->data.F32[imageID]) {
		    doObjectExtFit = true;
		}
	    } else {
		if (source->moments->KronFlux > SN_LIM * source->moments->KronFluxErr) {
		    doObjectExtFit = true;
		}
	    }
	}

	for (int j = 0; j < object->sources->n; j++) {
		
	    pmSource *source = object->sources->data[j];
	    if (!source) continue;
	    if (!source->peak) continue;
		
	    // clear the 2 relevant bits
	    source->tmpFlags &= clearBits;
		
	    // skip PSF-like and non-astronomical objects??
	    if (source->type == PM_SOURCE_TYPE_DEFECT) continue;
	    if (source->type == PM_SOURCE_TYPE_SATURATED) continue;
	    if (source->mode & PM_SOURCE_MODE_DEFECT) continue;
	    if (source->mode & PM_SOURCE_MODE_SATSTAR) continue;
		
	    // skip saturated stars modeled with a radial profile 
	    if (source->mode2 & PM_SOURCE_MODE2_SATSTAR_PROFILE) continue;

            // skip sources without a psf model
            if (source->modelPSF == NULL) continue;
		
	    if (doObjectExtFit) {
		source->tmpFlags |= PM_SOURCE_TMPF_EXT_FIT;
	    }
            if (doObjectPetrosian) {
                source->tmpFlags |= PM_SOURCE_TMPF_PETRO;
            }

	    // Do the fits if the recipe requests we do extended source fits to everything
	    if (doObjectRadial) {
		source->tmpFlags |=  PM_SOURCE_TMPF_RADIAL_KEEP;
		source->tmpFlags &= ~PM_SOURCE_TMPF_RADIAL_SKIP;
	    } else {
		source->tmpFlags |=  PM_SOURCE_TMPF_RADIAL_SKIP;
		source->tmpFlags &= ~PM_SOURCE_TMPF_RADIAL_KEEP;
	    }
	}
    }

    psLogMsg ("psphot.options", PS_LOG_WARN, "choose analysis options for %ld objects: %f sec\n", objects->n, psTimerMark ("psphot.options"));

    psFree(exptime);
    psFree(zeropt);
    psFree(chips);
    psFree(inputFilters);
    psFree(petroFluxLim);
    psFree(extFitFluxLim);
    psFree(toGal);

    return true;
}

static bool GetGalacticCoords (psSphere *ptGal, psSphere *ptSky, psSphereRot *toGal, pmChip *chip, float xPos, float yPos) {

    pmFPA *fpa = chip->parent;

    if (!chip->toFPA) goto escape;
    if (!fpa->toTPA) goto escape;
    if (!fpa->toSky) goto escape;

    psPlane ptCH, ptFP, ptTP;

    // calculate the astrometry for the coordinate of interest
    ptCH.x = xPos;
    ptCH.y = yPos;
    psPlaneTransformApply (&ptFP, chip->toFPA, &ptCH);
    psPlaneTransformApply (&ptTP, fpa->toTPA, &ptFP);
    psDeproject (ptSky, &ptTP, fpa->toSky);
    psSphereRotApply (ptGal, toGal, ptSky);

    // psSphereRotApply insures that 0 < r < 2PI. We want -PI < b <= PI
    if (ptGal->r > M_PI) {
        ptGal->r -= 2.0 * M_PI;
    }

    return true;

escape:
    // no astrometry calibration, give up
    ptSky->r = NAN;
    ptSky->d = NAN;

    ptGal->r = NAN;
    ptGal->d = NAN;

    return false;
}
