# include "psphotInternal.h"

# define PRINT_STUFF 0
# define USE_FOOTPRINTS 1

// measure some properties on the exended objects

bool psphotGalaxyParams (pmConfig *config, const pmFPAview *view, const char *filerule)
{
    bool status = true;

    fprintf (stdout, "\n");
    psLogMsg ("psphot", PS_LOG_INFO, "--- psphot Galaxy Parameters ---");

    // select the appropriate recipe information
    psMetadata *recipe  = psMetadataLookupPtr (&status, config->recipes, PSPHOT_RECIPE);
    psAssert (recipe, "missing recipe?");

    // add this 
    bool doGalaxyParams = psMetadataLookupBool (&status, recipe, "EXTENDED_SOURCE_GALAXY_PARAMS");
    bool doPetrosian = psMetadataLookupBool (&status, recipe, "EXTENDED_SOURCE_PETROSIAN");
    bool doAnnuli    = psMetadataLookupBool (&status, recipe, "EXTENDED_SOURCE_ANNULI");
    bool doExtModels = psMetadataLookupBool (&status, recipe, "EXTENDED_SOURCE_FITS");

    // All of these must be true to measure galaxy parameters
    if (!doGalaxyParams || !doPetrosian || !doAnnuli || !doExtModels) {
	psLogMsg ("psphot", PS_LOG_INFO, "skipping Galaxy parameter source measurements\n");
	return true;
    }

    int num = psphotFileruleCount(config, filerule);

    // loop over the available readouts
    for (int i = 0; i < num; i++) {
	if (!psphotGalaxyParamsReadout (config, view, filerule, i, recipe)) {
	    psError (PSPHOT_ERR_CONFIG, false, "failed on galaxy parameter measurements for %s entry %d", filerule, i);
	    return false;
	}
    }
    return true;
}

// # define COUNT_SKIPS
# ifdef COUNT_SKIPS
/*** for the moment, this test code : it is not thread safe ***/
static int  Nskip1 = 0;
static int  Nskip2 = 0;
static int  Nskip3 = 0;
static int  Nskip4 = 0;
static int  Nskip5 = 0;
static int  Nskip6 = 0;
static int  Nskip7 = 0;
static int  Nskip8 = 0;
# define SKIP(VALUE) { VALUE++; continue; }
# else
# define SKIP(VALUE) {continue;}
# endif //notdef

// aperture-like measurements for extended sources
bool psphotGalaxyParamsReadout (pmConfig *config, const pmFPAview *view, const char *filerule, int index, psMetadata *recipe) {

    bool status;
    int Next = 0;
    int Npetro = 0;
    int Nannuli = 0;

    psTimerStart ("psphot.galaxy");

    // find the currently selected readout
    pmFPAfile *file = pmFPAfileSelectSingle(config->files, filerule, index); // File of interest
    psAssert (file, "missing file?");

    pmReadout *readout = pmFPAviewThisReadout(view, file->fpa);
    psAssert (readout, "missing readout?");

    // XXX EAM TEST
    // psphotSaveImage(NULL, readout->image, "dump.v0.fits");

    pmDetections *detections = psMetadataLookupPtr (&status, readout->analysis, "PSPHOT.DETECTIONS");
    psAssert (detections, "missing detections?");

    psArray *sources = detections->allSources;
    psAssert (sources, "missing sources?");

    if (!sources->n) {
	psLogMsg ("psphot", PS_LOG_INFO, "no sources, skipping source size");
	return true;
    }

    // determine the number of allowed threads
    int nThreads = psMetadataLookupS32(&status, config->arguments, "NTHREADS"); // Number of threads
    if (!status) {
	nThreads = 0;
    }

    // get the sky noise from the background analysis; if this is missing, get the user-supplied value
    float skynoise = psMetadataLookupF32 (&status, readout->analysis, "SKY_STDEV");
    if (!status) {
	skynoise = psMetadataLookupF32 (&status, recipe, "SKY.NOISE");
	psWarning ("failed to get sky noise level from background analysis; defaulting to user supplied value of %f\n", skynoise);
    }

    float fitNsigmaConv = psMetadataLookupF32 (&status, recipe, "EXT_FIT_NSIGMA_CONV"); // number of sigma for the convolutio
    if (!status || !isfinite(fitNsigmaConv) || fitNsigmaConv <= 0) {
	fitNsigmaConv = 5.0;
    }
    int psfSize = psMetadataLookupS32 (&status, recipe, "PCM_BOX_SIZE");

    psImage *footprintImage = NULL;
#if USE_FOOTPRINTS
    footprintImage = psImageAlloc(readout->image->numCols, readout->image->numRows, PS_TYPE_S32);
    psImageInit(footprintImage, 0);
    pmSetFootprintArrayIDsForImage(footprintImage, detections->footprints, false);
#endif

    // source analysis is done in S/N order (brightest first)
    sources = psArraySort (sources, pmSourceSortByFlux);

    // option to limit analysis to a specific region
    char *region = psMetadataLookupStr (&status, recipe, "ANALYSIS_REGION");
    psRegion *AnalysisRegion = psRegionAlloc(0,0,0,0);
    *AnalysisRegion = psRegionForImage (readout->image, psRegionFromString (region));
    if (psRegionIsNaN (*AnalysisRegion)) psAbort("analysis region mis-defined");

    // choose Cx, Cy (see psphotThreadTools.c for overview of the concepts)
    int Cx = 1, Cy = 1;
    psphotChooseCellSizes (&Cx, &Cy, readout, nThreads);

    psArray *cellGroups = psphotAssignSources (Cx, Cy, sources);

    for (int i = 0; i < cellGroups->n; i++) {

	psArray *cells = cellGroups->data[i];

	for (int j = 0; j < cells->n; j++) {

	    // allocate a job -- if threads are not defined, this just runs the job
	    psThreadJob *job = psThreadJobAlloc ("PSPHOT_GALAXY_PARAMS");

	    psArrayAdd(job->args, 1, readout);
	    psArrayAdd(job->args, 1, cells->data[j]); // sources
	    psArrayAdd(job->args, 1, AnalysisRegion);
	    psArrayAdd(job->args, 1, recipe);

	    PS_ARRAY_ADD_SCALAR(job->args, skynoise, PS_TYPE_F32);
	    PS_ARRAY_ADD_SCALAR(job->args, fitNsigmaConv, PS_TYPE_F32);
	    PS_ARRAY_ADD_SCALAR(job->args, psfSize, PS_TYPE_F32);
	    psArrayAdd(job->args, 1, footprintImage);

	    PS_ARRAY_ADD_SCALAR(job->args, 0, PS_TYPE_S32); // this is used as a return value for Next
	    PS_ARRAY_ADD_SCALAR(job->args, 0, PS_TYPE_S32); // this is used as a return value for Npetro
	    PS_ARRAY_ADD_SCALAR(job->args, 0, PS_TYPE_S32); // this is used as a return value for Nannuli


// set this to 0 to run without threading
# if (1)	    
	    if (!psThreadJobAddPending(job)) {
		psError(PS_ERR_UNKNOWN, false, "Unable to guess model.");
		psFree(AnalysisRegion);
		return false;
	    } 
# else
	    if (!psphotGalaxyParams_Threaded(job)) {
		psError(PS_ERR_UNKNOWN, false, "Unable to guess model.");
		psFree(AnalysisRegion);
		return false;
	    }
	    psScalar *scalar = NULL;
	    scalar = job->args->data[8];
	    Next += scalar->data.S32;
	    scalar = job->args->data[9];
	    Npetro += scalar->data.S32;
	    scalar = job->args->data[10];
	    Nannuli += scalar->data.S32;
	    psFree(job);
# endif
	}

	// wait for the threads to finish and manage results
	if (!psThreadPoolWait (false, true)) {
	    psError(PS_ERR_UNKNOWN, false, "Unable to guess model.");
	    psFree(AnalysisRegion);
	    return false;
	}

	// we have only supplied one type of job, so we can assume the types here
	psThreadJob *job = NULL;
	while ((job = psThreadJobGetDone()) != NULL) {
	    if (job->args->n < 1) {
		fprintf (stderr, "error with job\n");
	    } else {
		psScalar *scalar = NULL;
		scalar = job->args->data[8];
		Next += scalar->data.S32;
		scalar = job->args->data[9];
		Npetro += scalar->data.S32;
		scalar = job->args->data[10];
		Nannuli += scalar->data.S32;
	    }
	    psFree(job);
	}
    }
    psFree (cellGroups);
    psFree(AnalysisRegion);
    psFree(footprintImage);

    // XXX EAM TEST
    // psphotSaveImage(NULL, readout->image, "dump.v1.fits");

    psLogMsg ("psphot", PS_LOG_WARN, "galaxy parameter analysis: %f sec for %d objects\n", psTimerMark ("psphot.galaxy"), Next);
    psLogMsg ("psphot", PS_LOG_INFO, "  %d petrosian\n", Npetro);
    psLogMsg ("psphot", PS_LOG_INFO, "  %d annuli\n", Nannuli);

// # if (PS_TRACE_ON)
# ifdef COUNT_SKIPS
    fprintf (stderr, "ext analysis skipped @ 1  : %d\n", Nskip1);
    fprintf (stderr, "ext analysis skipped @ 2  : %d\n", Nskip2);
    fprintf (stderr, "ext analysis skipped @ 3  : %d\n", Nskip3);
    fprintf (stderr, "ext analysis skipped @ 4  : %d\n", Nskip4);
    fprintf (stderr, "ext analysis skipped @ 5  : %d\n", Nskip5);
    fprintf (stderr, "ext analysis skipped @ 6  : %d\n", Nskip6);
    fprintf (stderr, "ext analysis skipped @ 7  : %d\n", Nskip7);
    fprintf (stderr, "ext analysis skipped @ 8  : %d\n", Nskip8);
#endif

    // psphotRadialProfileShowSkips ();

    // psphotVisualShowResidualImage (readout, false);

    // psphotVisualShowPetrosians (sources);

    // XXX TEST: SAVE IMAGE:
    // psphotSaveImage (NULL, readout->image, "resid.fits");

    return true;
}

bool psphotGalaxyParams_Threaded (psThreadJob *job) {

    // bool status;

    int Next = 0;
    int Npetro = 0;
    int Nannuli = 0;
    int Ngood = 0;

    // arguments: readout, sources, models, region, psfSize, maskVal, markVal
    pmReadout *readout      = job->args->data[0];
    psArray *sources        = job->args->data[1];
    psRegion *region        = job->args->data[2];
    psMetadata *recipe      = job->args->data[3];

#if (!USE_FOOTPRINTS)
    float skynoise          = PS_SCALAR_VALUE(job->args->data[4],F32);
#endif
    float fitNsigmaConv     = PS_SCALAR_VALUE(job->args->data[5],F32);
    float psfSize           = PS_SCALAR_VALUE(job->args->data[6],F32);
    psImage *footprintImage = job->args->data[7];

    bool status;
//    bool doPetrosian    = psMetadataLookupBool (&status, recipe, "EXTENDED_SOURCE_PETROSIAN");

    // user-defined masks to test for good/bad pixels (build from recipe list if not yet set)
    psImageMaskType maskVal = psMetadataLookupImageMask(&status, recipe, "MASK.PSPHOT"); // Mask value for bad pixels
    assert (maskVal);

    // don't try and use bad models
    pmModelStatus badModel = PM_MODEL_STATUS_NONE;
//    badModel |= PM_MODEL_STATUS_NONCONVERGE;  this causes most objects to get unmeasured
    badModel |= PM_MODEL_STATUS_BADARGS;
    badModel |= PM_MODEL_STATUS_OFFIMAGE;
    badModel |= PM_MODEL_STATUS_NAN_CHISQ;
    badModel |= PM_MODEL_SERSIC_PCM_FAIL_GUESS;
    badModel |= PM_MODEL_SERSIC_PCM_FAIL_GRID;
    badModel |= PM_MODEL_PCM_FAIL_GUESS;

    badModel |= PM_MODEL_STATUS_LIMITS;

    pmModelType sersicModelType = pmModelClassGetType("PS_MODEL_SERSIC");

    psVector *bgPixels = NULL;
    psVector *bgPixelsFlip = NULL;
#define dontMAKE_MARK_IMAGE 1
#ifdef MAKE_MARK_IMAGE
    psImage *markImage = NULL;
#endif

#define MARK_SOURCE 0
#define MARK_MASK 1
#define MARK_BG 2
    psRandom *rng = psRandomAlloc(PS_RANDOM_TAUS);

#define dontDUMP_IMAGES 1
#ifdef DUMP_IMAGES
    char *filebase = "/data/ippc18.0/home/bills/debug/psphot/images";
    char filename[80];
    psMetadata *dummyHeader = psMetadataAlloc();
#endif

    // choose the sources of interest
    for (int i = 0; i < sources->n; i++) {

	pmSource *source = sources->data[i];

	// Nall ++;

	bool testObject = FALSE;
	// testObject = testObject || ((fabs(source->peak->xf - 2468) < 10) && (fabs(source->peak->yf - 3955) < 10));
	// testObject = testObject || ((fabs(source->peak->xf -  432) < 10) && (fabs(source->peak->yf -  781) < 10));
	// testObject = testObject || ((fabs(source->peak->xf -  300) < 10) && (fabs(source->peak->yf - 5988) < 10));
	if (testObject) {
	    fprintf (stderr, "test object: %f, %f\n", source->peak->xf, source->peak->yf);
	    testObject = TRUE;
	}

	// rules for measuring petrosian parameters for specific objects are set in
	// psphotChooseAnalysisOptions.c
	if (!(source->tmpFlags & PM_SOURCE_TMPF_PETRO)) SKIP (Nskip1);

	// limit selection by analysis region (XXX move this into psphotChooseAnalysisOption?)
	if (source->peak->x < region->x0) SKIP (Nskip2);
	if (source->peak->y < region->y0) SKIP (Nskip3);
	if (source->peak->x > region->x1) SKIP (Nskip4);
	if (source->peak->y > region->y1) SKIP (Nskip5);

	// Should we skip objects that do not have an extended model?
	if (!source->extpars) SKIP (Nskip6);
	if (!source->modelFits) SKIP (Nskip7);

	// Flux cut to reduce sources to those that match SDSS
	// if (source->moments->KronFlux < 2e5) continue;

	pmModel *sersicModel = NULL;
	for (int j = 0; j < source->modelFits->n; j++) {
	    pmModel *model = source->modelFits->data[j];
	    if (model->type == sersicModelType) {
		sersicModel = model;
		break;
	    }
	}
	if (!sersicModel) SKIP (Nskip8);
	if (sersicModel->flags & badModel) SKIP (Nskip8);

	// check out the model params and skip those that are too large
	psF32 *PAR = sersicModel->params->data.F32;
	psF32 Xo = PAR[PM_PAR_XPOS];
	psF32 Yo = PAR[PM_PAR_YPOS];
	psF32 Io = PAR[PM_PAR_I0];
	//
	// The half light radius to the major axis of the model
	psEllipseAxes axes = pmPSF_ModelToAxes (PAR, sersicModel->class->useReff);
	if (axes.major >= 100) SKIP (Nskip8);

	source->extpars->ghalfLightRadius = axes.major;
	Next ++;

	// if sersicModel is not modelEXT make it so
	pmSourceMode saveMode = source->mode;
	pmSourceType saveType = source->type;
	bool isExtended = saveType == PM_SOURCE_TYPE_EXTENDED;
	pmModel *extModel = source->modelEXT;

	if (source->tmpFlags & PM_SOURCE_TMPF_SUBTRACTED) {
	    pmSourceAdd (source, PM_MODEL_OP_FULL, maskVal);
	}

	if (!isExtended || extModel != sersicModel) {
	    // XXX: I'm not doing this right. For now just look at objects for which
	    source->modelEXT = psMemIncrRefCounter (sersicModel);
	    source->type = PM_SOURCE_TYPE_EXTENDED;
	    source->mode |= PM_SOURCE_MODE_EXTMODEL;
	    // do we need this?
	    source->mode |= PM_SOURCE_MODE_NONLINEAR_FIT;
	}


	// and we will integrate out to 2 * half light radius
	psF32 R = 2 * axes.major;
	psF32 R2 = PS_SQR(R);

	float radius = source->peak->xf - source->pixels->col0;
	radius = PS_MAX (radius, source->peak->yf - source->pixels->row0);
	radius = PS_MAX (radius, source->pixels->numRows - source->peak->yf + source->pixels->row0);
	radius = PS_MAX (radius, source->pixels->numCols - source->peak->xf + source->pixels->col0);
	// Make image at least 10 * half light radius
	radius = PS_MAX (radius, 5*axes.major);
	pmSourceRedefinePixels (source, readout, source->peak->xf, source->peak->yf, 2*radius);

	// RedefinePixels frees modelFlux
	if (source->modelEXT->isPCM) {
	    pmPCMCacheModel (source, maskVal, psfSize, fitNsigmaConv);
	} else {
	    pmSourceCacheModel (source, maskVal);
	}

#ifdef DUMP_IMAGES
	sprintf(filename, "%s/%04d-pix.fits", filebase, source->id);
	psphotSaveImage(dummyHeader, source->pixels, filename);
#endif

	// Create the markImage and accumlate the Flux

	// center of mass in subimage.  Note: the calculation below uses pixel index, so we correct
	// xCM, yCM from pixel coords to pixel index here.
	psF32 xCM = Xo - 0.5 - source->pixels->col0; // coord of peak in subimage
	psF32 yCM = Yo - 0.5 - source->pixels->row0; // coord of peak in subimage

	psF32 **vPix = source->pixels->data.F32;
	psImageMaskType **vMsk = (source->maskObj == NULL) ? NULL : source->maskObj->data.PS_TYPE_IMAGE_MASK_DATA;

	bgPixels = psVectorRecycle(bgPixels, source->pixels->numCols*source->pixels->numRows, PS_TYPE_F32);
	bgPixels->n = 0;
	bgPixelsFlip = psVectorRecycle(bgPixelsFlip, source->pixels->numCols*source->pixels->numRows, PS_TYPE_F32);
	bgPixelsFlip->n = 0;

#ifdef MAKE_MARK_IMAGE
	// The markImage may no longer be needed
	markImage = psImageRecycle(markImage, source->pixels->numCols, source->pixels->numRows, PS_TYPE_U8);

	// assume all pixels are masked to start
	psImageInit(markImage, MARK_MASK);
	psU8 **vMark = markImage->data.U8;
#define SET_MARK(_val) vMark[row][col] = _val
#else
#define SET_MARK(_val)
#endif

#if (USE_FOOTPRINTS)
	psS32 **vFootprint = footprintImage->data.S32;
#else
	// XXX use recipe value instead of hard coded 1.5
	psF32 threshold = skynoise * 3;
#endif

	psF32 sumI = 0;
	psS32 row0 = source->pixels->row0;
	psS32 col0 = source->pixels->col0;
	for (psS32 row = 0; row < source->pixels->numRows ; row++) {
	    psF32 yDiff = row - yCM;
	    // y coordinate of mirror pixel
	    int yFlip = yCM - yDiff;
	    if (yFlip < 0) continue;
	    if (yFlip >= source->pixels->numRows) continue;
	    psS32 yImage = row + row0;
	    for (psS32 col = 0; col < source->pixels->numCols ; col++) {
		// check mask and value for this pixel
		if (vMsk && (vMsk[row][col] & maskVal)) continue;
		psF32 pixel = vPix[row][col];
		if (isnan(pixel)) continue;
		// valid pixel

		psS32 xImage = col + col0;

		psF32 xDiff = col - xCM;

		psF32 r2 = PS_SQR(xDiff) + PS_SQR(yDiff);
		if (r2 <= R2) {
		    sumI += pixel;
		}

#if (USE_FOOTPRINTS)
		// this is a pixel in the object
		if (vFootprint[yImage][xImage] == source->peak->footprint->id) {
		    SET_MARK(MARK_SOURCE);
		    continue;
		}
		// this is a pixel in another object
		if (vFootprint[yImage][xImage] > 0.0) continue;
#else
		// this is a pixel in an object (this or the other)
		if (fabs(pixel) >= threshold) {
		    continue;
		}
#endif
		// we have a background pixel.
		// Check whether it's mirror is a background pixel as well.
	
		// x coordinate of mirror pixel
		int xFlip = xCM - xDiff;
		if (xFlip < 0) continue;
		if (xFlip >= source->pixels->numCols) continue;
		// check mask and value for mirror pixel
		if (vMsk && (vMsk[yFlip][xFlip] & maskVal)) continue;
		psF32 mirrorPixel = vPix[yFlip][xFlip];
		if (isnan(mirrorPixel)) continue;

	
#if (USE_FOOTPRINTS)
		bool goodMirror = (vFootprint[yFlip + row0][xFlip + col0] == 0); 
#else
		bool goodMirror = (fabs(mirrorPixel) < threshold);
#endif
		if (goodMirror) {
		    // This is a good background pixel with a good mirror Save the values 
		    SET_MARK(MARK_BG);
		    psVectorAppend(bgPixels, pixel);
		    psVectorAppend(bgPixelsFlip, mirrorPixel);
		}
	    }
	}
#ifdef DUMP_IMAGES
	sprintf(filename, "%s/%04d-mrk.fits", filebase, source->id);
	psphotSaveImage(dummyHeader, markImage, filename);
#endif

	// subtract model to create the residual image
	pmSourceSub (source, PM_MODEL_OP_FULL, maskVal);
	if (!(source->tmpFlags & PM_SOURCE_TMPF_SUBTRACTED)) {
	    // Hmm something has gone wrong
	    fprintf(stderr, "failed to subtract sersic model for source %d\n", source->id);
	    goto restoreModel;
	}
#ifdef DUMP_IMAGES
	sprintf(filename, "%s/%04d-res.fits", filebase, source->id);
	psphotSaveImage(dummyHeader, source->pixels, filename);
#endif

	// don't know if these pointers could change in the background subtraction so grab them again
	vPix = source->pixels->data.F32;
	vMsk = (source->maskObj == NULL) ? NULL : source->maskObj->data.PS_TYPE_IMAGE_MASK_DATA;
	psF32 **vVar = source->variance->data.F32;
	psF32 **vModel = source->modelFlux->data.F32;
	// Accumulate Rt and Ra
	psF32 sumRt = 0;
	psF32 sumRa = 0;
	int numPixels = 0;
	psF32 sumB = 0;
	psF32 sumModel = 0;
	int numB = 0;
	for (psS32 row = 0; row < source->pixels->numRows; row++) {
	    psF32 yDiff = row - yCM;
	    if (fabs(yDiff) > R) continue;
	    // y coordinate of mirror pixel
	    int yFlip = yCM - yDiff;
	    if (yFlip < 0) continue;
	    if (yFlip >= source->pixels->numRows) continue;
	    for (psS32 col = 0; col < source->pixels->numCols ; col++) {
		// check mask and value for this pixel
		if (vMsk && (vMsk[row][col] & maskVal)) continue;
		psF32 pixel = vPix[row][col];
		if (isnan(pixel)) continue;

		psF32 xDiff = col - xCM;
		psF32 r2 = PS_SQR(xDiff) + PS_SQR(yDiff);
		if (r2 > R2) continue;

		// Measurement of bumpiness parameter excludes the pixels
		// within 2 pixels of the center
		if (r2 > 4) {
		    psF32 variance = vVar[row][col];
		    if (isfinite(variance)) {
			psF32 modelPixel = vModel[row][col];
			if (!isfinite(modelPixel)) {
			    // XXX: Can this happen in a good model?
			    fprintf(stderr, "non-finite modelPixel %4d (%4d, %4d)\n", source->id, col, row);
			}
			numB++;
			sumB += PS_SQR(pixel) - variance;
			sumModel += Io * modelPixel;
		    }
		}

		// x coordinate of mirror pixel
		int xFlip = xCM - xDiff;
		if (xFlip < 0) continue;
		if (xFlip >= source->pixels->numCols) continue;
		// check mask and value for mirror pixel
		if (vMsk && (vMsk[yFlip][xFlip] & maskVal)) continue;
		psF32 mirrorPixel = vPix[yFlip][xFlip];
		if (isnan(mirrorPixel)) continue;

		sumRt += fabs(pixel + mirrorPixel);
		sumRa += fabs(pixel - mirrorPixel);
		++numPixels;
	    }
	}

	if (numB > 0) {
	    source->extpars->gbumpy = sqrt(sumB/numB)  / (sumModel / numB);
	}

	if (bgPixels->n < numPixels) {
	    // fprintf(stderr, "could not find enough background pixels for source %d found %ld need %d\n", source->id, bgPixels->n, numPixels);
	    goto restoreModel;
	}
	Ngood++;

	// and calculate Bt and Ba
	psF32 sumBt = 0;
	psF32 sumBa = 0;
	psF32 *vB = bgPixels->data.F32;
	psF32 *vBFlip = bgPixelsFlip->data.F32;
	long    length = bgPixels->n;
	// Randomly select numPixels background pixels
	for (int n = 0; n < numPixels; n++) {
	    double rand = psRandomUniform(rng);
	    long index = floor(rand * length);
	    if (index >= length) {
		fprintf(stderr, "random index %ld is larger than vector length %ld\n",
			index, length);
		continue;
	    }
	    sumBt += fabs(vB[index] + vBFlip[index]);
	    sumBa += fabs(vB[index] - vBFlip[index]);
	}

	// finally save the results
	source->extpars->gRT = 0.5 * (sumRt - sumBt)/sumI;
	source->extpars->gRA = 0.5 * (sumRa - sumBa)/sumI;
	source->extpars->gS2 = source->extpars->gRT + source->extpars->gRA;

#if (PRINT_STUFF)
	if (Ngood % 40 == 1) {
	    fprintf (stderr, "   id  sumI rHL rad   sumI     gRT   gRA       sumRt      sumBt         numPixels numBGPixels\n");
	}
	fprintf (stderr, "%5d %5.1f %5.1f : %5.2e %4.1f %10.1f %6.3f %6.3f %5.2e %5.2e %10d %6ld\n",
		 source->id, source->peak->xf, source->peak->yf, 
		 sumI, source->extpars->ghalfLightRadius, 
		 radius, 
		 source->extpars->gRT, 
		 source->extpars->gRA, sumRt, sumBt, numPixels, bgPixels->n);
#endif

	// Now put things back the way that they were.
    restoreModel:

	if (testObject) {
	    fprintf (stderr, "done with %d\n", source->id);
	}

# define USE_BEST_FIT TRUE
	// XXX TEST: Keep the sersic model regardless
	if (USE_BEST_FIT && ((extModel != sersicModel) || !isExtended)) {
	    // add the sersic model back in
	    if (source->tmpFlags & PM_SOURCE_TMPF_SUBTRACTED) {
		pmSourceAdd (source, PM_MODEL_OP_FULL, maskVal);
	    }

	    // replace the extended model
	    if (extModel != sersicModel) {
		psMemDecrRefCounter (sersicModel);
		source->modelEXT = extModel;
	    }
	    source->mode = saveMode;
	    source->type = saveType;

	    // RedefinePixels frees modelFlux
	    if (isExtended && source->modelEXT->isPCM) {
		pmPCMCacheModel (source, maskVal, psfSize, fitNsigmaConv);
	    } else {
		pmSourceCacheModel (source, maskVal);
	    }

	    pmSourceSub (source, PM_MODEL_OP_FULL, maskVal);
	}
    }

#ifdef MAKE_MARK_IMAGE
    psFree(markImage);
#endif
#ifdef DUMP_IMAGES
    psFree(dummyHeader);
// psphotSaveImage raises an error
    psErrorClear();
#endif
    psFree(bgPixels)
	psFree(bgPixelsFlip)
	psFree(rng);

    psScalar *scalar = NULL;

// change the value of a scalar on the array (wrap this and put it in psArray.h)
    scalar = job->args->data[8];
    scalar->data.S32 = Next;

    scalar = job->args->data[9];
    scalar->data.S32 = Npetro;

    scalar = job->args->data[10];
    scalar->data.S32 = Nannuli;

    return true;
}

