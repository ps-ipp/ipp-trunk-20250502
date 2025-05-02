# include "ppStack.h"


// Option to use the old FWHM rejection code.
# define NEW_REJECTION
// #define OLD_REJECTION


# define RE_PHOTOMETER 0

# define PHOT_SOURCE_MASK (PM_SOURCE_MODE_FAIL | PM_SOURCE_MODE_SATSTAR | PM_SOURCE_MODE_BLEND | \
			   PM_SOURCE_MODE_BADPSF | PM_SOURCE_MODE_DEFECT | PM_SOURCE_MODE_SATURATED | \
			   PM_SOURCE_MODE_CR_LIMIT | PM_SOURCE_MODE_EXT_LIMIT) // Mask to apply to sources

bool ppStackInputPhotometer(const pmReadout *ro, const psArray *sources, const pmConfig *config)
{
    PM_ASSERT_READOUT_NON_NULL(ro, false);
    PS_ASSERT_ARRAY_NON_NULL(sources, false);

    psMetadata *recipe = psMetadataLookupMetadata(NULL, config->recipes, PPSTACK_RECIPE); // ppStack recipe
    psAssert(recipe, "We've thrown an error on this before.");

# if (RE_PHOTOMETER)
    bool mdok;                          // Status of MD lookup

    float zpRadius = psMetadataLookupS32(&mdok, recipe, "PHOT.RADIUS"); // Radius for PHOT measurement
    if (!mdok) {
        psError(PPSTACK_ERR_CONFIG, true, "Unable to find PHOT.RADIUS in recipe");
        return false;
    }
    float zpSigma = psMetadataLookupF32(&mdok, recipe, "PHOT.SIGMA"); // Gaussian sigma for photometry
    if (!mdok) {
        psError(PPSTACK_ERR_CONFIG, true, "Unable to find PHOT.SIGMA in recipe");
        return false;
    }
    float zpFrac = psMetadataLookupF32(&mdok, recipe, "PHOT.FRAC"); // Fraction of good pixels for photometry
    if (!mdok) {
        psError(PPSTACK_ERR_CONFIG, true, "Unable to find PHOT.FRAC in recipe");
        return false;
    }

    psString maskValStr = psMetadataLookupStr(&mdok, recipe, "MASK.VAL"); // Name of bits to mask going in
    if (!mdok || !maskValStr) {
        psError(PPSTACK_ERR_CONFIG, false, "Unable to find MASK.VAL in recipe");
        return false;
    }
    psImageMaskType maskVal = pmConfigMaskGet(maskValStr, config); // Bits to mask

    // Measure background
    psImage *image = ro->image, *mask = ro->mask; // Image and mask from readout
    psRandom *rng = psRandomAlloc(PS_RANDOM_TAUS); // Random number generator
    psStats *stats = psStatsAlloc(PS_STAT_ROBUST_MEDIAN | PS_STAT_ROBUST_STDEV); // Statistics
    if (!psImageBackground(stats, NULL, image, mask, maskVal, rng)) {
        psError(PPSTACK_ERR_DATA, false, "Unable to measure background for image");
        psFree(stats);
        psFree(rng);
        return false;
    }
    float bg = stats->robustMedian; // Background level
    psFree(stats);
    psFree(rng);
# endif

    // Examine the sources (photometer if RE_PHOTOMETER is set)
    int numSources = sources->n; // Number of sources
    int numGood = 0;            // Number of good sources
    float maxMag = -INFINITY;   // Maximum magnitude
    for (int j = 0; j < numSources; j++) {
        pmSource *source = sources->data[j]; // Source of interest
        if (source->mode & PHOT_SOURCE_MASK || !isfinite(source->psfMag)) {
            source->psfMag = NAN;
            continue;
        }

// This code re-measures the magnitude in the assumption that the warp analysis failed It would
// be better to catch and correct such cases.  In any case, we should check on the validity of
// this assumption (eg, compare the mean psf-ap offset) and raise an error on that?

# if (RE_PHOTOMETER)
	
	int numCols = image->numCols, numRows = image->numRows; // Size of image
	float x = source->peak->xf, y = source->peak->yf; // Coordinates of source
        int xCentral = x + 0.5, yCentral = y + 0.5; // Central pixel
        // Range of integration
        int xMin = PS_MAX(0, xCentral - zpRadius), xMax = PS_MIN(numCols - 1, xCentral + zpRadius);
        int yMin = PS_MAX(0, yCentral - zpRadius), yMax = PS_MIN(numRows - 1, yCentral + zpRadius);

        // Integrate
        double sumImage = 0.0, sumKernel = 0.0; // Sums from integration
        int numBadPix = 0;         // Number of bad pixels
        for (int v = yMin; v <= yMax; v++) {
            float dy2 = PS_SQR(y - v); // Distance from centroid
            for (int u = xMin; u <= xMax; u++) {
                if (mask->data.PS_TYPE_IMAGE_MASK_DATA[v][u] & maskVal) {
                    numBadPix++;
                    continue;
                }
                float dx2 = PS_SQR(x - u); // Distance from centroid
                double kernel = exp(-0.5 * (dx2 + dy2) / PS_SQR(zpSigma)); // Kernel value
                sumImage += (image->data.F32[v][u] - bg) * kernel;
                sumKernel += kernel;
            }
        }

        if (numBadPix > zpFrac * M_PI * PS_SQR(zpRadius) || !isfinite(sumImage)) {
            source->psfMag = NAN;
        } else {
            source->psfMag = - 2.5 * log10(sumImage / sumKernel * M_PI * PS_SQR(zpRadius));
            if (isfinite(source->psfMag)) {
                numGood++;
                maxMag = PS_MAX(maxMag, source->psfMag);
            }
        }
# else
	numGood++;
	maxMag = PS_MAX(maxMag, source->psfMag);
# endif
    }
    psLogMsg("ppStack", PS_LOG_INFO, "Photometered %d good sources in image; max mag %f", numGood, maxMag);

    return true;
}


// Preparation iteration: Load the sources, and get a target PSF model
bool ppStackPrepare(ppStackOptions *options, pmConfig *config)
{
    psAssert(options, "Require options");
    psAssert(config, "Require configuration");

    psMetadata *recipe = psMetadataLookupMetadata(NULL, config->recipes, PPSTACK_RECIPE); // ppStack recipe
    psAssert(recipe, "We've thrown an error on this before.");

    int num = options->num = psMetadataLookupS32(NULL, config->arguments, "INPUTS.NUM");
    options->sourceLists = psArrayAlloc(num); // Individual lists of sources for matching

    options->inputMask = psVectorAlloc(num, PS_TYPE_VECTOR_MASK); // Mask for inputs
    psVectorInit(options->inputMask, 0);
    options->exposures = psVectorAlloc(options->num, PS_TYPE_F32);

    psVectorInit(options->exposures, NAN);

    pmFPAfileActivate(config->files, false, NULL);
    ppStackFileActivation(config, PPSTACK_FILES_PREPARE, true);
    pmFPAview *view = ppStackFilesIterateDown(config);
    if (!view) {
        return false;
    }

    //MEH -- bscale offset hack for warps 
    options->bscaleOffset = psVectorAlloc(options->num, PS_TYPE_F32);
    psVectorInit(options->bscaleOffset, 0.0);
    bool setbscaleoffset = psMetadataLookupBool(NULL,recipe, "BSCALEOFFSET");

    psArray *psfs = psArrayAlloc(num); // PSFs for PSF envelope
    int numCols = 0, numRows = 0;   // Size of image
    options->sumExposure = 0.0;
    int numWithSources = 0;
    for (int i = 0; i < num; i++) {
        pmFPAfile *file = pmFPAfileSelectSingle(config->files, "PPSTACK.INPUT", i); // File of interest
        pmCell *cell = pmFPAviewThisCell(view, file->fpa); // Cell of interest

        options->exposures->data.F32[i] = psMetadataLookupF32(NULL, cell->concepts, "CELL.EXPOSURE");
	if ((options->exposures->data.F32[i] == 0)||
	    (!(isfinite(options->exposures->data.F32[i])))){
	  options->exposures->data.F32[i] = psMetadataLookupF32(NULL,recipe,"DEFAULT.EXPTIME");
	}
        options->sumExposure += options->exposures->data.F32[i];

        //MEH -- hdu get redefn again below if conv (lots of this happening..) 
        if (setbscaleoffset) {
            pmHDU *hdu = pmHDUFromCell(cell);
            assert(hdu && hdu->header);
            float bscale = psMetadataLookupF32(NULL, hdu->header, "BSCALE");
            options->bscaleOffset->data.F32[i] = 0.5*bscale;
        }
        psLogMsg("ppStack", PS_LOG_INFO, "bscaleOffset: %d %f", i,options->bscaleOffset->data.F32[i]);

        // Get list of PSFs, to determine target PSF
        if (options->convolve) {
            pmChip *chip = pmFPAviewThisChip(view, file->fpa); // The chip: holds the PSF
            pmPSF *psf = psMetadataLookupPtr(NULL, chip->analysis, "PSPHOT.PSF"); // PSF
            if (!psf) {
                psError(PPSTACK_ERR_PROG, false, "Unable to find PSF.");
                psFree(view);
                psFree(psfs);
                return false;
            }
            psfs->data[i] = psMemIncrRefCounter(psf);
            psMetadataRemoveKey(chip->analysis, "PSPHOT.PSF");

            pmCell *cell = pmFPAviewThisCell(view, file->fpa); // Cell of interest
            pmHDU *hdu = pmHDUFromCell(cell);
            assert(hdu && hdu->header);
            int naxis1 = psMetadataLookupS32(NULL, hdu->header, "NAXIS1"); // Number of columns
            int naxis2 = psMetadataLookupS32(NULL, hdu->header, "NAXIS2"); // Number of rows
            if (naxis1 <= 0 || naxis2 <= 0) {
                psError(PPSTACK_ERR_PROG, false, "Unable to determine size of image from PSF.");
                psFree(view);
                psFree(psfs);
                return false;
            }
            if (numCols == 0 && numRows == 0) {
                numCols = naxis1;
                numRows = naxis2;
            }
        }


        bool redoPhot = psMetadataLookupBool(NULL, recipe, "PHOT");

        pmDetections *detections = NULL;
        if (options->convolve || options->matchZPs || options->photometry || redoPhot) {
            pmReadout *ro = pmFPAviewThisReadout(view, file->fpa); // Readout with sources
            detections = psMetadataLookupPtr(NULL, ro->analysis, "PSPHOT.DETECTIONS"); // Sources
            if (!detections || !detections->allSources || !detections->allSources->n) {
                psWarning("No detections found for image %d --- rejecting.", i);
                options->inputMask->data.PS_TYPE_VECTOR_MASK_DATA[i] = PPSTACK_MASK_CAL;
                continue;
            }
            psAssert (detections->allSources, "missing sources?");

            options->sourceLists->data[i] = psMemIncrRefCounter(detections->allSources);
            numWithSources++;
        }

        // Re-do photometry if we don't trust the source lists
        if (redoPhot) {
            psTrace("ppStack", 2, "Photometering input %d of %d....\n", i, num);
            pmFPAfileActivate(config->files, false, NULL);
            ppStackFileActivationSingle(config, PPSTACK_FILES_CONVOLVE, true, i);
            if (options->convolve) {
                pmFPAfileActivate(config->files, true, "PPSTACK.CONV.KERNEL");
            }
            pmFPAfile *file = pmFPAfileSelectSingle(config->files, "PPSTACK.INPUT", i); // File
            pmFPAview *photView = ppStackFilesIterateDown(config);
            if (!photView) {
                psFree(view);
                return false;
            }

            pmReadout *ro = pmFPAviewThisReadout(view, file->fpa); // Readout of interest

            if (!ppStackInputPhotometer(ro, detections->allSources, config)) {
                psError(psErrorCodeLast(), false, "Unable to do photometry on input sources");
                psFree(view);
                psFree(photView);
                return false;
            }

            psFree(photView);
            if (!ppStackFilesIterateUp(config)) {
                psFree(view);
                return false;
            }
            pmFPAfileActivate(config->files, false, NULL);
            ppStackFileActivation(config, PPSTACK_FILES_PREPARE, true);
        }
    }
    if (numWithSources < 2) {
        // This can happen if the inputs have been destreaked
        psErrorStackPrint(stderr, "Not enough inputs have sources");
        psWarning("No inputs have sources --- suspect bad data quality.");
        if (options->quality == 0) {
            options->quality = PPSTACK_ERR_DATA;
        }
        psErrorClear();
        psFree(view);
        return false;
    }

    bool mdok = false;
    bool  simpleClip = psMetadataLookupBool(&mdok, recipe, "PSF.INPUT.CLIP.SIMPLE");
    float maxFWHM = psMetadataLookupF32(&mdok, recipe, "PSF.INPUT.MAX"); // max allowed input fwhm
    float clipFWHMnSig = psMetadataLookupF32(&mdok, recipe, "PSF.INPUT.CLIP.NSIGMA"); // sigma clipping of inputs
    float threshFWHM = psMetadataLookupF32(&mdok, recipe, "PSF.INPUT.THRESH"); // FWHM size that we refuse to clip below
    float asymmetryFWHM = psMetadataLookupF32(&mdok, recipe, "PSF.INPUT.ASYMMETRY"); // max bimodal asymmetry

    
    psString log = psStringCopy("Input seeing FWHMs:\n"); // Log message
    bool havePSFs = false;                                // Do we have any PSFs?
    options->inputSeeing = psVectorAlloc(num, PS_TYPE_F32);
    psVectorInit(options->inputSeeing, NAN);
    for (int i = 0; i < num; i++) {
        pmPSF *psf = psfs->data[i];     // PSF for image
        if (!psf) {
            continue;
        }
        havePSFs = true;

        int xNum = PS_MAX(psf->trendNx, 1), yNum = PS_MAX(psf->trendNy, 1); // Number of realisations
        double sumFWHM = 0.0;           // FWHM for image
        int numFWHM = 0;                // Number of FWHM measurements
        for (int y = 0; y < yNum; y++) {
            float yPos = ((float)y + 0.5) / (float)yNum * numRows;
            for (int x = 0; x < xNum; x++) {
                float xPos = ((float)x + 0.5) / (float)xNum * numCols;
                float fwhm = pmPSFtoFWHM(psf, xPos, yPos); // FWHM for image
                if (isfinite(fwhm)) {
                    sumFWHM += fwhm;
                    numFWHM++;
                }
            }
        }
        if (numFWHM == 0) {
            options->inputSeeing->data.F32[i] = NAN;
            options->inputMask->data.PS_TYPE_VECTOR_MASK_DATA[i] = PPSTACK_MASK_PSF;
            psLogMsg("ppStack", PS_LOG_INFO, "Unable to measure PSF FWHM for image %d --- rejected.", i);
        } else {
            options->inputSeeing->data.F32[i] = sumFWHM / (float)numFWHM;
        }
#ifdef OLD_REJECTION
	// reject any input images which exceed the specified max FWHM
	if (isfinite(maxFWHM) && (options->inputSeeing->data.F32[i] > maxFWHM)) {
            options->inputMask->data.PS_TYPE_VECTOR_MASK_DATA[i] = PPSTACK_MASK_PSF;
            psLogMsg("ppStack", PS_LOG_INFO, "PSF FWHM for image %d is too large (%f vs %f maxFWHM) --- rejected.", i, options->inputSeeing->data.F32[i], maxFWHM);
	}
	// End old rejection fixed limit.
#endif
        psStringAppend(&log, "Input %d: %f\n", i, options->inputSeeing->data.F32[i]);
    }
    if (havePSFs) {
        psLogMsg("ppStack", PS_LOG_INFO, "%s", log);
    }
    psFree(log);

#ifdef NEW_REJECTION
    // Begin new rejection code.
    float limit = INFINITY; // Default to a big value.
    if (!isfinite(threshFWHM)) { // If this isn't set, initialize it to zero.  No inputs may be clipped if smaller than this
      threshFWHM = 0.0;
    }
    if (!isfinite(maxFWHM)) { // No maxFWHM was set, accept all inputs
      maxFWHM = INFINITY;
    }
    limit = maxFWHM; // Initialize the limit to be the maxFWHM
    if (limit < threshFWHM) {
      limit = threshFWHM;
    }
    if (!isfinite(clipFWHMnSig)) { // We cannot do a sigma clip with nSig, so use the maxFWHM to set the limit.
      psLogMsg("ppStack",PS_LOG_INFO,
	       "PSF FWHM distribution: Unable to determine limit based on recipe. Setting to %f. (clipFWHMnSig: %f maxFWHM: %f threshFWHM: %f asymmetryFWHM: %f)",
	       limit,
	       clipFWHMnSig,maxFWHM,threshFWHM,asymmetryFWHM);
    }
    if (simpleClip) { // Do a sigma clip like the old rejection code.
      psStats *fwhmStats = psStatsAlloc(PS_STAT_CLIPPED_MEAN | PS_STAT_CLIPPED_STDEV);
      psVectorStats (fwhmStats, options->inputSeeing, NULL, options->inputMask, 0xff);
      psLogMsg("ppStack", PS_LOG_INFO, "Input FWHMs : %f +/- %f", fwhmStats->clippedMean, fwhmStats->clippedStdev);
      
      options->clippedMean =  fwhmStats->clippedMean;
      options->clippedStdev = fwhmStats->clippedStdev;
      limit = options->clippedMean + clipFWHMnSig * options->clippedStdev;
      if (limit < threshFWHM) {
	limit = threshFWHM;
      }
      if (limit > maxFWHM) {
	limit = maxFWHM;
      }
      psLogMsg("ppStack",PS_LOG_INFO,
	       "PSF FWHM distribution: Used simple clip method.  Limit set to %f.",
	       limit);
      psFree(fwhmStats);
    }
    else { // Do the mixture model rejection.
      if(!isfinite(asymmetryFWHM)) { // Or not, because the parameters aren't set.
	psLogMsg("ppStack",PS_LOG_INFO,
		 "PSF FWHM distribution: Unable to determine limit based on recipe. Setting to %f. (clipFWHMnSig: %f maxFWHM: %f threshFWHM: %f asymmetryFWHM: %f)",
		 limit,
		 clipFWHMnSig,maxFWHM,threshFWHM,asymmetryFWHM);
      }
      else {
	// Do GMM test with two modes to decide where to put the break.
	double Punimodal;
	int m = 2;
	psVector *modes = psVectorAlloc(num,PS_TYPE_F32);
	psVector *means = psVectorAlloc(m,PS_TYPE_F32);
	psVector *S     = psVectorAlloc(m,PS_TYPE_F32);
	psVector *pi    = psVectorAlloc(m,PS_TYPE_F32);
	psImage *P      = psImageAlloc(m,num,PS_TYPE_F32);
	
	// XXX: we should not pass in images to this analysis which have already been rejected by the FWHM limits
	if (!psMM1DClass(options->inputSeeing,
			 options->inputSeeing->n,
			 modes,means,
			 S,pi,P,
			 2,
			 &Punimodal)) {
	  // Handle error here
	  psFree(modes);
	  psFree(means);
	  psFree(S);
	  psFree(pi);
	  psFree(P);
	  return(false);
	}
	//    fprintf(stderr,"means: %g %g\n",means->data.F32[0],means->data.F32[1]);
	//    fprintf(stderr,"sigma: %g %g \n",S->data.F32[0],S->data.F32[1]);
	//    fprintf(stderr,"pi:    %g %g\n",pi->data.F32[0],pi->data.F32[1]);
	
	// Use Gaussian mixture model analysis of the FWHM distribution to decide an optional FWHM limit
	if ((Punimodal > 0.5)|| // This distribution is best represented by a single mode
	    (num <= 4)) {       // Or we have a small number of inputs, making this statistic poor.
	  limit = maxFWHM;
	}
	else {                  // This is a bimodal distribution
	  if ((fabs(pi->data.F32[0] - pi->data.F32[1]) < asymmetryFWHM)||  // However, both modes are equally populated
	      (pi->data.F32[1] > pi->data.F32[0])) {                       // Or the larger FWHM mode is more populated
	    limit = means->data.F32[1] + clipFWHMnSig * S->data.F32[1];
	  }
	  else {                                   // The smaller FWHM mode is more populated
	    limit = means->data.F32[0] + clipFWHMnSig * S->data.F32[0];
	  }
	}
	if (limit > maxFWHM)    { limit = maxFWHM; }    // We should not be larger than our max
	if (limit < threshFWHM) { limit = threshFWHM; } // Nor smaller than our min
	psLogMsg("ppStack",PS_LOG_INFO,
		 "PSF FWHM distribution: limit: %f (%f %f %f) (%f %f %f) %f",
		 limit,means->data.F32[0],S->data.F32[0],pi->data.F32[0],
		 means->data.F32[1],S->data.F32[1],pi->data.F32[1],
		 Punimodal);
	psFree(means);
	psFree(modes);
	psFree(S);
	psFree(pi);
	psFree(P);
      } // End mixture model case
    }
    // Perform rejection using the limit set
    for (int i = 0; i < num; i++) {
      if (options->inputSeeing->data.F32[i] > limit) {
	options->inputMask->data.PS_TYPE_VECTOR_MASK_DATA[i] = PPSTACK_MASK_PSF;
	psLogMsg("ppStack", PS_LOG_INFO,
		 "PSF FWHM for image %d is too large (%f vs %f fwhmlimit) --- rejected",
		 i, options->inputSeeing->data.F32[i],limit);
      }
    }
    // End new rejection code
#endif
    
#ifdef OLD_REJECTION
    // We should have the ability to filter the input list based on the seeing:
    // * reject above some max value and/or min value
    // * reject images out-of-line with the rest of the images

    // measure stats
    psStats *fwhmStats = psStatsAlloc(PS_STAT_CLIPPED_MEAN | PS_STAT_CLIPPED_STDEV);
    psVectorStats (fwhmStats, options->inputSeeing, NULL, options->inputMask, 0xff);
    psLogMsg("ppStack", PS_LOG_INFO, "Input FWHMs : %f +/- %f", fwhmStats->clippedMean, fwhmStats->clippedStdev);

    options->clippedMean =  fwhmStats->clippedMean;
    options->clippedStdev = fwhmStats->clippedStdev;

    if (isfinite(clipFWHMnSig)) {
	float fwhmLimit = fwhmStats->clippedMean + clipFWHMnSig * fwhmStats->clippedStdev;
	fwhmLimit = isfinite(maxFWHM) ? PS_MIN (maxFWHM, fwhmLimit) : fwhmLimit;
	for (int i = 0; i < num; i++) {
            if (options->inputMask->data.PS_TYPE_VECTOR_MASK_DATA[i]) continue;
            if (options->inputSeeing->data.F32[i] > fwhmLimit) {
		options->inputMask->data.PS_TYPE_VECTOR_MASK_DATA[i] = PPSTACK_MASK_PSF;
		psLogMsg("ppStack", PS_LOG_INFO, "PSF FWHM for image %d is too large (%f vs %f fwhmLimit) --- rejected.", i, options->inputSeeing->data.F32[i], fwhmLimit);
	    }
        }
    }
    // End old rejection sigma clip
#endif
    // Generate target PSF
    if (options->convolve) {
        options->psf = ppStackPSF(config, numCols, numRows, psfs, options);
	psFree(psfs);
        if (!options->psf) {
#if 1
            psError(psErrorCodeLast(), false, "Unable to determine output PSF.");
#else
            // This will repair the problem reproted in ticket 1427 but we aren't yet sure
            // why ppStackPSF is failing so we are going to continue to fault for now
            int errorCode = psErrorCodeLast();
            if (errorCode == PPSTACK_ERR_PSF) {
                psErrorStackPrint(stderr, "Unable to determine output PSF.");
                psWarning("Unable to determine output PSF --- suspect bad data quality.");
                if (options->quality == 0) {
                    options->quality = errorCode;
                }
                psErrorClear();
            } else {
                psError(psErrorCodeLast(), false, "Unable to determine output PSF.");
            }
#endif // notyet
            psFree(view);
            return false;
        }
        psMetadataAddPtr(config->arguments, PS_LIST_TAIL, "PSF.TARGET", PS_DATA_UNKNOWN,
                         "Target PSF for stack", options->psf);
        options->targetSeeing = pmPSFtoFWHM(options->psf, 0.5 * numCols, 0.5 * numRows); // FWHM for target


	bool psfTargetAsMax = psMetadataLookupBool(&mdok, recipe, "PSF.TARGET.AS.MAX");
	if (psfTargetAsMax) { // Should we use the largest input as the target?
	  float psfTargetEpsilon = psMetadataLookupF32(&mdok,recipe,"PSF.TARGET.AS.MAX.EPSILON");
	  if (!mdok) { psfTargetEpsilon = 0.0; }
	  options->targetSeeing = 0.0;
	  for (int i = 0; i < num; i++) {
            if (options->inputMask->data.PS_TYPE_VECTOR_MASK_DATA[i]) continue;
	    options->targetSeeing = PS_MAX(options->targetSeeing,options->inputSeeing->data.F32[i]);
	  }
	  psLogMsg("ppStack", PS_LOG_INFO, "Using MAX accepted input FWHM as target (max: %f epsilon: %f target %f)\n",
		   options->targetSeeing,psfTargetEpsilon,options->targetSeeing + psfTargetEpsilon);
	  options->targetSeeing = options->targetSeeing + psfTargetEpsilon;
	}
        psLogMsg("ppStack", PS_LOG_INFO, "Target seeing FWHM: %f\n", options->targetSeeing);	
	
        pmChip *outChip = pmFPAfileThisChip(config->files, view, "PPSTACK.TARGET.PSF"); // Output chip
        psMetadataAddPtr(outChip->analysis, PS_LIST_TAIL, "PSPHOT.PSF", PS_DATA_UNKNOWN,
                         "Target PSF", options->psf);
        outChip->data_exists = true;
    }

    // Zero point calibration
    if (!ppStackSourcesTransparency(options, view, config)) {
        psError(PPSTACK_ERR_DATA, false, "Unable to calculate transparency differences");
        psFree(view);
        return false;
    }
    psFree(view);

    if (!ppStackFilesIterateUp(config)) {
        return false;
    }

    return true;
}
