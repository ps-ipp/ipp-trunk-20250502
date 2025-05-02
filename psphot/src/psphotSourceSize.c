# include "psphotInternal.h"

// this structure is only used internally to simplify the function parameters
typedef struct {
    psImageMaskType maskVal;
    psImageMaskType markVal;
    psImageMaskType crMask;
    float ApResid;
    float ApSysErr;
    float nSigmaApResid;
    float nSigmaMoments;
    float nSigmaCR;
    bool altDiffExt;
    float altDiffExtThresh;
    float soft;
    int grow;
    int xtest, ytest;
    bool applyCRmask; // apply CR mask?
    bool dynamicLimitsCR; // apply CR mask?
    float sizeLimitCR;
    float magLimitCR;
    int maxWindowCR;
} psphotSourceSizeOptions;

// local functions:
bool psphotSourceSizePSF (psphotSourceSizeOptions *options, pmReadout *readout, psArray *sources, pmPSF *psf, psMetadata *recipe);
bool psphotDynamicLimitsCR (psphotSourceSizeOptions *options, pmReadout *readout, psArray *sources, pmPSF *psf, psMetadata *recipe);
bool psphotSourceClass (pmReadout *readout, psArray *sources, psMetadata *recipe, pmPSF *psf, psphotSourceSizeOptions *options, pmConfig *config);
bool psphotSourceClassRegion (psRegion *region, pmPSFClump *psfClump, psArray *sources, psMetadata *recipe, pmPSF *psf, psphotSourceSizeOptions *options, pmConfig *config);
bool psphotSourceSelectCR (pmReadout *readout, psArray *sources, psphotSourceSizeOptions *options);
bool psphotMaskCosmicRay (pmReadout *readout, pmSource *source, psImageMaskType maskVal, int maxWindowCR);
int  psphotMaskCosmicRayConnected (int xPeak, int yPeak, psImage *mymask, psImage *myvar, psImage *edges, int binning, float sigma_thresh);
float psphotSourceSizeFindThreshold (psVector *value, psVector *mask, int maskValue, float minValue, float maxValue, float delta, float guess, float fraction);

// save test output images?
# define DUMPPICS 0

// we need to call this function after sources have been fitted to the PSF model and
// subtracted.  

// for now, let's store the detections on the readout->analysis for each readout
bool psphotSourceSize (pmConfig *config, const pmFPAview *view, const char *filerule, bool getPSFsize)
{
    bool status = true;

    fprintf (stdout, "\n");
    psLogMsg ("psphot", PS_LOG_INFO, "--- psphot Source Size ---");

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
        if (!psphotSourceSizeReadout (config, view, filerule, i, recipe, getPSFsize)) {
	    psError (PSPHOT_ERR_CONFIG, false, "failed on source size analysis for %s entry %d", filerule, i);
            return false;
        }
    }
    return true;
}

// this function use an internal flag to mark sources which have already been measured
bool psphotSourceSizeReadout(pmConfig *config, const pmFPAview *view, const char *filerule, int index, psMetadata *recipe, bool getPSFsize)
{
    bool status;
    psphotSourceSizeOptions options;

    psTimerStart ("psphot.size");

    // find the currently selected readout
    pmFPAfile *file = pmFPAfileSelectSingle(config->files, filerule, index); // File of interest
    psAssert (file, "missing file?");

    pmReadout *readout = pmFPAviewThisReadout(view, file->fpa);
    psAssert (readout, "missing readout?");

    pmDetections *detections = psMetadataLookupPtr (&status, readout->analysis, "PSPHOT.DETECTIONS");
    psAssert (detections, "missing detections?");

    psArray *sources = detections->allSources;
    psAssert (sources, "missing sources?");

    if (!sources->n) {
        psLogMsg ("psphot", PS_LOG_INFO, "no sources, skipping source size");
        return true;
    }

    pmPSF *psf = psMetadataLookupPtr (&status, readout->analysis, "PSPHOT.PSF");
    psAssert (psf, "missing psf?");

    // user-defined masks to test for good/bad pixels (build from recipe list if not yet set)
    options.maskVal = psMetadataLookupImageMask(&status, recipe, "MASK.PSPHOT"); // Mask value for bad pixels
    assert (options.maskVal);

    options.markVal = psMetadataLookupImageMask(&status, recipe, "MARK.PSPHOT"); // Mask value for bad pixels
    assert (options.markVal);

    // bit to mask the cosmic-ray pixels
    options.crMask  = pmConfigMaskGet("CR", config); // Mask value for cosmic rays

    options.nSigmaCR = psMetadataLookupF32 (&status, recipe, "PSPHOT.CR.NSIGMA.LIMIT");
    assert (status);

    // XXX recipe name is not great
    options.nSigmaApResid = psMetadataLookupF32 (&status, recipe, "PSPHOT.EXT.NSIGMA.LIMIT");
    assert (status);

    // XXX recipe name is not great (NOTE : not used!)
    options.nSigmaMoments = psMetadataLookupF32 (&status, recipe, "PSPHOT.EXT.NSIGMA.MOMENTS");
    assert (status);

    // Optional algorithm to define if a source is extended (used by DIFF analysis)
    options.altDiffExt = psMetadataLookupBool(&status, recipe, "PSPHOT.EXT.DIFF.ALTERNATE");
    assert (status);

    // Threshold for this alternate method
    options.altDiffExtThresh = psMetadataLookupF32(&status, recipe, "PSPHOT.EXT.DIFF.ALTERNATE.THRESH");
    assert (status);

    // location of a single test source
    options.xtest = psMetadataLookupS32 (&status, recipe, "PSPHOT.CRMASK.XTEST");
    options.ytest = psMetadataLookupS32 (&status, recipe, "PSPHOT.CRMASK.YTEST");
    // These are optional recipe values don't assert (status);

    options.grow = psMetadataLookupS32(&status, recipe, "PSPHOT.CR.GROW"); // Growth size for CRs
    if (!status || options.grow < 0) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "PSPHOT.CR.GROW is not positive.");
        return false;
    }

    options.soft = psMetadataLookupF32(&status, recipe, "PSPHOT.CR.NSIGMA.SOFTEN"); // Softening parameter
    if (!status || !isfinite(options.soft) || options.soft < 0.0) {
        psWarning("PSPHOT.CR.NSIGMA.SOFTEN not set; defaulting to zero.");
        options.soft = 0.0;
    }

    options.applyCRmask = psMetadataLookupBool(&status, recipe, "PSPHOT.CRMASK.APPLY"); // Growth size for CRs
    if (!status) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "PSPHOT.CRMASK.APPLY is not defined.");
        return false;
    }
    options.maxWindowCR =  psMetadataLookupS32 (&status, recipe, "PSPHOT.CR.MAX.WINDOW");
    if (!status) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "PSPHOT.CR.MAX.WINDOW is not defined.");
        return false;
    }

    // determine the distribution of (PSF_mag - KRON_mag) for the PSF sources (saved on readout->analysis)
    psphotSourceSizePSF (&options, readout, sources, psf, recipe);

    // adjust the user-supplied limits based on the distribution of CRs in the (Mminor, mKron) space
    psphotDynamicLimitsCR(&options, readout, sources, psf, recipe);

    // classify the sources based on ApResid and Moments
    // NOTE: only sources not already measured !(source->tmpFlags & PM_SOURCE_TMPF_SIZE_MEASURED)
    psphotSourceClass(readout, sources, recipe, psf, &options, config);

    // attempt to mask the candidate CRs; flag if CR nature is confirmed
    psphotSourceSelectCR(readout, sources, &options);

    // XXX fix this (was source->n  - first)
    psLogMsg ("psphot.size", PS_LOG_WARN, "measure source sizes for %ld sources: %f sec\n", sources->n, psTimerMark ("psphot.size"));

    psphotVisualPlotSourceSize (recipe, readout->analysis, sources);
    psphotVisualPlotSourceSizeAlt (recipe, readout->analysis, sources);
    psphotVisualShowSourceSize (readout, sources);
    psphotVisualPlotApResid (sources, options.ApResid, options.ApSysErr, false);
    psphotVisualShowSatStars (recipe, psf, sources);

    return true;
}

# define SAVE_PSF_OPTIONS(RO,OPT)					\
    /* save these on readout->analysis (a) for output to the header and (b) to prevent re-calculating in another pass */ \
    psMetadataAddF32(RO->analysis, PS_LIST_TAIL, "PSPHOT.PSF.APRESID", PS_META_REPLACE, "locus of PSF stars in PSF_MAG - KRON_MAG", OPT->ApResid); \
    psMetadataAddF32(RO->analysis, PS_LIST_TAIL, "PSPHOT.PSF.APRESID.SYSERR",  PS_META_REPLACE, "systematic error of PSF_MAG - KRON_MAG",  OPT->ApSysErr);

// model the apmifit distribution for the psf stars:
bool psphotSourceSizePSF (psphotSourceSizeOptions *options, pmReadout *readout, psArray *sources, pmPSF *psf, psMetadata *recipe) {

    // We are using the value PSF_MAG - KRON_MAG as a measure of the extendedness of an object.
    // We need to model this distribution for the PSF stars before we can test the significance
    // for a specific object.

    // NOTE: we require that objects have had moments measured, and we also require psfMags to
    // be calculated.  but, we do not require aperture mags or any other photometry values that
    // require pixel analysis.

    bool status1 = false;
    bool status2 = false;
    options->ApResid = psMetadataLookupF32 (&status1, readout->analysis, "PSPHOT.PSF.APRESID");
    options->ApSysErr = psMetadataLookupF32 (&status2, readout->analysis, "PSPHOT.PSF.APRESID.SYSERR");
    psAssert ((status1 && status2) || (!status1 && !status2), "inconsistent record of PSF ApResid values");

    // if they are saved on readout->analysis, we have already calculated these values
    if (status1 && status2) {
	return true;
    }

    // select stats from the psf stars
    psVector *ApOff = psVectorAllocEmpty (100, PS_TYPE_F32);
    psVector *ApErr = psVectorAllocEmpty (100, PS_TYPE_F32);

    psImageMaskType markVal = options->markVal;
    psImageMaskType maskVal = options->maskVal | options->markVal;

    // with PSFONLY, we do not need modify the pixels
    pmSourcePhotometryMode photMode = PM_SOURCE_PHOT_PSFONLY;

    int num = 0;                        // Number of sources measured
    for (int i = 0; i < sources->n; i++) {
        pmSource *source = sources->data[i];
        if (!(source->mode & PM_SOURCE_MODE_PSFSTAR)) continue;
        num++;

        pmSourceMagnitudes (source, psf, photMode, maskVal, markVal, source->apRadius);

        float kMag = -2.5*log10(source->moments->KronFluxPSF);
        float dMag = source->psfMag - kMag;

        psVectorAppend (ApOff, dMag);
        psVectorAppend (ApErr, source->psfMagErr);
    }
    if (num == 0) {
	// if we cannot determine the PSF distribution, call all objects PSFs...
	options->ApResid = NAN;
	options->ApSysErr = NAN;
        psFree(ApOff);
        psFree(ApErr);
	SAVE_PSF_OPTIONS(readout, options);
        return false;
    }

    // model the distribution as a mean or median value and a systematic error from that value:
    // psStats *stats = psStatsAlloc(PS_STAT_ROBUST_MEDIAN);    
    psStats *stats = psStatsAlloc(PS_STAT_CLIPPED_MEAN);    
    psVectorStats (stats, ApOff, NULL, NULL, 0);

    psVector *dAp = psVectorAlloc (ApOff->n, PS_TYPE_F32);
    for (int i = 0; i < ApOff->n; i++) {
        dAp->data.F32[i] = ApOff->data.F32[i] - stats->clippedMean;
    }

    options->ApResid = stats->clippedMean;
    options->ApSysErr = psVectorSystematicError(dAp, ApErr, 0.1);

    // this is quite arbitrary... a large value means fewer things classified as extended.
    if (!isfinite(options->ApSysErr)) options->ApSysErr = 0.05;
    psLogMsg ("psphot", PS_LOG_DETAIL, "psf - Sum: %f +/- %f\n", options->ApResid, options->ApSysErr);

    psFree (ApOff);
    psFree (ApErr);
    psFree (stats);
    psFree (dAp);

    SAVE_PSF_OPTIONS(readout, options);
    return true;
}

# define SAVE_CR_OPTIONS(RO,OPT)					\
    /* save these on readout->analysis (a) for output to the header and (b) to prevent re-calculating in another pass */  \
    psMetadataAddF32(RO->analysis, PS_LIST_TAIL, "PSPHOT.CR.MAX.SIZE", PS_META_REPLACE, "dynamically-set minor axis size limit for cosmic rays", OPT->sizeLimitCR); \
    psMetadataAddF32(RO->analysis, PS_LIST_TAIL, "PSPHOT.CR.MAX.MAG",  PS_META_REPLACE, "dynamically-set kron magnitude limit for cosmic rays",  OPT->magLimitCR);

// model the size and magnitude distribution of the Cosmic Rays
// ** CRs are reliably flagged by a combination on Mminor < X && mag (or flux) > Y
bool psphotDynamicLimitsCR (psphotSourceSizeOptions *options, pmReadout *readout, psArray *sources, pmPSF *psf, psMetadata *recipe) {

    /* attempt to describe the CR sources:
       - input parameters are sizeLimit, magLimit
       - first try to refine the sizeLimit:
       -- select objects which meet the magLimit and exceed the sizeLimit by a factor of 1.5
       -- generate the histogram
       -- look for a peak in the histogram
       -- look for the min between valley and upper limit
       -- look for first bin within 5% of the valley floor after peak (new sizeLimit)
       
       - next try to refine the magLimit
       -- select objects which meet the sizeLimit and go fainter than the magLimit by 1.0 mag
       -- generate the histogram
       -- look for a peak in the histogram
       -- look for the min between valley and upper limit
       -- look for first bin within 5% of the valley floor after peak (new magLimit)
    */

    bool status  = false;
    bool status1 = false;
    bool status2 = false;
    options->sizeLimitCR = psMetadataLookupF32 (&status1, readout->analysis, "PSPHOT.CR.MAX.SIZE");
    if (!status1) {
	options->sizeLimitCR = psMetadataLookupF32 (&status, recipe, "PSPHOT.CR.MAX.SIZE");
	if (!status) {
	    options->sizeLimitCR = 1.0;
	}
    } 
    options->magLimitCR = psMetadataLookupF32 (&status2, readout->analysis, "PSPHOT.CR.MAX.MAG");
    if (!status2) {
	options->magLimitCR = psMetadataLookupF32 (&status, recipe, "PSPHOT.CR.MAX.MAG");
	if (!status) {
	    options->magLimitCR = -8.0;
	}
    }
    psAssert ((status1 && status2) || (!status1 && !status2), "inconsistent record of dynamic CR limits");

    // if they are saved on readout->analysis, we have already calculated these values
    if (status1 && status2) {
	return true;
    }

    // if we do not want to dynamically set these, save the user-supplied values and exit
    options->dynamicLimitsCR = psMetadataLookupBool (&status, recipe, "PSPHOT.CR.AUTOSCALE");
    if (!status) {
	options->dynamicLimitsCR = true;
    }
    if (!options->dynamicLimitsCR) {
	SAVE_CR_OPTIONS(readout, options); // macro defined above
	return true;
    }

    psVector *minor = psVectorAllocEmpty (100, PS_TYPE_F32);
    psVector *mKron = psVectorAllocEmpty (100, PS_TYPE_F32);
    psVector *mask  = NULL;

    psImageMaskType markVal = options->markVal;
    psImageMaskType maskVal = options->maskVal | options->markVal;

    // with PSFONLY, we do not need modify the pixels
    pmSourcePhotometryMode photMode = PM_SOURCE_PHOT_PSFONLY;

    // generate vectors for all the objects of possible interest (so we can just access those
    // vectors in the next sections)
    for (int i = 0; i < sources->n; i++) {
        pmSource *source = sources->data[i];

        // XXX can we test if psfMag is set and calculate only if needed?
        pmSourceMagnitudes (source, psf, photMode, maskVal, markVal, source->apRadius);

        // convert to Mmaj, Mmin:
        psF32 Mxx = source->moments->Mxx;
        psF32 Myy = source->moments->Myy;
        psF32 Mxy = source->moments->Mxy;

        float KronMag = -2.5*log10(source->moments->KronFluxPSF);

	float Mminor = 0.5*(Mxx + Myy) - 0.5*sqrt(PS_SQR(Mxx - Myy) + 4.0*PS_SQR(Mxy));

	if (Mminor > options->sizeLimitCR * 1.5) continue;
	if (KronMag > options->magLimitCR + 2.5) continue;

        psVectorAppend (mKron, KronMag);
        psVectorAppend (minor, Mminor);
    }

    // if too few objects meet the criterion, give up..
    if (mKron->n < 50) goto escape;

    // set distinct masks for (minor > sizeLimit) or (mKron > magLimit)
    mask = psVectorAlloc(mKron->n, PS_TYPE_U8);
    psVectorInit(mask, 0);
    for (int i = 0; i < mKron->n; i++) {
	if (mKron->data.F32[i] > options->magLimitCR) {
	    mask->data.U8[i] |= 0x01;
	}
	if (minor->data.F32[i] > options->sizeLimitCR) {
	    mask->data.U8[i] |= 0x02;
	}
    }
	
    float delta1 = PS_MAX(0.02, PS_MIN(0.2, 5.0 * 0.5 / minor->n));
    float newSizeLimit = psphotSourceSizeFindThreshold(minor, mask, 0x01, 0.0, options->sizeLimitCR * 1.5, delta1, options->sizeLimitCR, 0.05);
    if (isfinite(newSizeLimit)) {
	options->sizeLimitCR = newSizeLimit;
    }

    float delta2 = PS_MAX(0.02, PS_MIN(0.2, 5.0 * 2.0 / minor->n));
    float newMagLimit = psphotSourceSizeFindThreshold(mKron, mask, 0x02, -15.0, options->magLimitCR + 2.5, delta2, options->magLimitCR, 0.05);
    if (isfinite(newMagLimit)) {
	options->magLimitCR = newMagLimit;
    }

    psLogMsg ("psphot", PS_LOG_DETAIL, "CR limits : %f mag | %f pix^2\n", options->magLimitCR, options->sizeLimitCR);

    psFree (mKron);
    psFree (minor);
    psFree (mask);

    // save these on readout->analysis (a) for output to the header and (b) to prevent re-calculating in another pass
    SAVE_CR_OPTIONS(readout, options); // macro defined above
    return true;

 escape:
    SAVE_CR_OPTIONS(readout, options); // macro defined above
    return false;
}

// classify sources based on the combination of psf-mag, Mxx, Myy
bool psphotSourceClass (pmReadout *readout, psArray *sources, psMetadata *recipe, pmPSF *psf, psphotSourceSizeOptions *options, pmConfig *config) {

    bool status;
    pmPSFClump psfClump;
    char regionName[64];

    psLogMsg("psModules.objects", PS_LOG_INFO, "Source Size classifications: %4s %4s %4s %4s %4s", "Npsf", "Next", "Nsat", "Ncr", "Nskip");

    if (!psphotSourceClassRegion (NULL, &psfClump, sources, recipe, psf, options, config)) {
	psLogMsg ("psphot", 4, "Failed to determine source classification for full image\n");
    } else {
	psLogMsg ("psphot", 4, "source classification for full image\n");
    }
    return true;
    
    // NOTE : this section is deactivated (EAM : I think we were getting poor boundary effects?)
    int nRegions = psMetadataLookupS32 (&status, readout->analysis, "PSF.CLUMP.NREGIONS");
    for (int i = 0; i < nRegions; i ++) {
        snprintf (regionName, 64, "PSF.CLUMP.REGION.%03d", i);
        psMetadata *regionMD = psMetadataLookupPtr (&status, readout->analysis, regionName);
        psAssert (regionMD, "regions must be defined by earlier call to psphotRoughClassRegion");
	
        psRegion *region = psMetadataLookupPtr (&status, regionMD, "REGION");
        psAssert (region, "regions must be defined by earlier call to psphotRoughClassRegion");
	
        // pull FWHM_X,Y from the recipe, use to define psfClump.X,Y
        psfClump.X  = psMetadataLookupF32 (&status, regionMD, "PSF.CLUMP.X");   psAssert (status, "missing PSF.CLUMP.X");
        psfClump.Y  = psMetadataLookupF32 (&status, regionMD, "PSF.CLUMP.Y");   psAssert (status, "missing PSF.CLUMP.Y");
        psfClump.dX = psMetadataLookupF32 (&status, regionMD, "PSF.CLUMP.DX");  psAssert (status, "missing PSF.CLUMP.DX");
        psfClump.dY = psMetadataLookupF32 (&status, regionMD, "PSF.CLUMP.DY");  psAssert (status, "missing PSF.CLUMP.DY");

        if ((psfClump.X < 0) || (psfClump.Y < 0) || !psfClump.X || !psfClump.Y || isnan(psfClump.X) || isnan(psfClump.Y)) {
            psLogMsg ("psphot", 4, "Failed to find a valid PSF clump for region %f,%f - %f,%f\n", region->x0, region->y0, region->x1, region->y1);
            continue;
        }

        if (!psphotSourceClassRegion (region, &psfClump, sources, recipe, psf, options, config)) {
            psLogMsg ("psphot", 4, "Failed to determine source classification for region %f,%f - %f,%f\n", region->x0, region->y0, region->x1, region->y1);
            continue;
        }
	psLogMsg ("psphot", 4, "source classification for region %f,%f - %f,%f\n", region->x0, region->y0, region->x1, region->y1);
        // psphotVisualPlotSourceSize (recipe, readout->analysis, sources);
    }

    return true;
}

bool psphotSourceClassRegion (psRegion *region, pmPSFClump *psfClump, psArray *sources, psMetadata *recipe, pmPSF *psf, psphotSourceSizeOptions *options, pmConfig *config) {

    PS_ASSERT_PTR_NON_NULL(sources, false);
    PS_ASSERT_PTR_NON_NULL(recipe, false);

    int Nsat  = 0;
    int Next  = 0;
    int Npsf  = 0;
    int Ncr   = 0;
    int Nskip = 0;

    //initiallise mask values to use later on
    pmSourceMagnitudesInit (config, recipe);

    pmSourceMode noMoments = PM_SOURCE_MODE_MOMENTS_FAILURE | PM_SOURCE_MODE_SKYVAR_FAILURE | PM_SOURCE_MODE_SKY_FAILURE | PM_SOURCE_MODE_BELOW_MOMENTS_SN;
    // request the pixWeight values as well as the magnitudes
    pmSourcePhotometryMode photMode = PM_SOURCE_PHOT_WEIGHT; 

    psImageMaskType markVal = options->markVal;
    psImageMaskType maskVal = options->maskVal | options->markVal;

    // in the ppSub context, do we get sensible values for ApResid?
    float ApResidPSF = options->ApResid;
    if (!isfinite(ApResidPSF)) {
      ApResidPSF = 0.0;
    }
    float ApSysErrPSF = options->ApSysErr;
    if (!isfinite(ApSysErrPSF)) {
      ApSysErrPSF = 0.0;
    }

    for (psS32 i = 0 ; i < sources->n ; i++) {

        pmSource *source = (pmSource *) sources->data[i];

        // psfClumps are found for image subregions:
        // skip sources not in this region
	if (region) {
	    if (source->peak->x <  region->x0) continue;
	    if (source->peak->x >= region->x1) continue;
	    if (source->peak->y <  region->y0) continue;
	    if (source->peak->y >= region->y1) continue;
	}

        // skip source if it was already measured
        if (source->tmpFlags & PM_SOURCE_TMPF_SIZE_MEASURED) {
            psTrace("psphot", 7, "Not calculating source size since it has already been measured\n");
            continue;
        }

        // source must have been subtracted
        if (!(source->tmpFlags & PM_SOURCE_TMPF_SUBTRACTED)) {
            source->mode |= PM_SOURCE_MODE_SIZE_SKIPPED;
            psTrace("psphot", 7, "Not calculating source size since source is not subtracted\n");
            pmSourceMaskEval (source,  source->maskObj, maskVal);
            Nskip ++;
            continue;
        }

	// skip saturated stars modeled with a radial profile 
        if (source->mode2 & PM_SOURCE_MODE2_SATSTAR_PROFILE) continue;

        // we are classifying by moments and PSF_MAG - KRON_MAG
        psAssert (source->moments, "why is this source missing moments?");
        if (source->mode & noMoments) {
            pmSourceMaskEval (source,  source->maskObj, maskVal);
            Nskip ++;
            continue;
        }

        // convert to Mmaj, Mmin:
        psF32 Mxx = source->moments->Mxx;
        psF32 Myy = source->moments->Myy;
        psF32 Mxy = source->moments->Mxy;
	float Mminor = 0.5*(Mxx + Myy) - 0.5*sqrt(PS_SQR(Mxx - Myy) + 4.0*PS_SQR(Mxy));

        // replace object in image to measure mag & psfWeights
        if (source->tmpFlags & PM_SOURCE_TMPF_SUBTRACTED) {
            pmSourceAdd (source, PM_MODEL_OP_FULL, options->maskVal);
        }

        // clear the mask bit and set the circular mask pixels
        // psImageMaskPixels (source->maskObj, "AND", PS_NOT_IMAGE_MASK(options->markVal));
        // psImageKeepCircle (source->maskObj, source->peak->x, source->peak->y, source->apRadius, "OR", options->markVal);
        pmSourceMagnitudes (source, psf, photMode, maskVal, markVal, source->apRadius);

        // clear the mask bit
        // psImageMaskPixels (source->maskObj, "AND", PS_NOT_IMAGE_MASK(options->markVal));

        // re-subtract the object, leave local sky
        pmSourceSub (source, PM_MODEL_OP_FULL, options->maskVal);

        float kMag = -2.5*log10(source->moments->KronFluxPSF);
        float dMag = source->psfMag - kMag;

	// sources without a valid magnitudes cannot have their size measured
	if (!isfinite(kMag) || !isfinite(source->psfMag)) {
            Nskip ++;
            continue;
	}

        // set nSigmaMAG to include both systematic and poisson error terms.  we include a hard
	// floor on the Ap Sys Err (to be a bit generous).  XXX put the floor in the recipe...
        float nSigmaMAG = (dMag - ApResidPSF) / hypot(source->psfMagErr, hypot(ApSysErrPSF, 0.02));
        source->extNsigma = nSigmaMAG;

        // notes to clarify the source size classification rules:
        // * a defect should be functionally equivalent to a cosmic ray
        // * CR & defect should have a faintess limit (min S/N)
        // * SAT stars should not be faint, but defects may?

        // Defects may not always match CRs from peak curvature analysis
        // Defects may also be marked as SATSTAR -- XXX deactivate this flag?
        // XXX this rule is not great
        // XXX only accept brightish detections as CRs
        // (nSigmaMAG < -options->nSigmaApResid) ||

        // saturated star (too many saturated pixels or peak above saturation limit).  These
        // may also be saturated galaxies, or just large saturated regions.  They are never
        // marked as 'extended'
        if (source->mode & PM_SOURCE_MODE_SATSTAR) {
            psTrace("psphotSourceClassRegion.SAT",4,"CLASS: %g %g\t%g %g\t%g %g\t%g %g\t%g SAT\n",
                    source->peak->xf, source->peak->yf, Mminor, kMag, dMag, nSigmaMAG, options->sizeLimitCR, options->magLimitCR, options->nSigmaApResid);
            source->tmpFlags |= PM_SOURCE_TMPF_SIZE_MEASURED;
            Nsat ++;
            continue;
        }

        // any sources missing a large fraction should just be treated as PSFs. They are never
        // marked as 'extended'
        if ((source->pixWeightNotBad < 0.9) || (source->pixWeightNotPoor < 0.9)) {
            psTrace("psphotSourceClassRegion.PSF",4,"CLASS: %g %g\t%g %g  %g %g  %g %g\t%g %g\t%g PSF\t%g %g\n",
                    source->peak->xf,source->peak->yf,Mxx,Myy,psfClump->X,psfClump->Y,psfClump->dX,psfClump->dY,kMag,dMag,nSigmaMAG,
                    options->nSigmaApResid,options->nSigmaMoments);
	    source->tmpFlags |= PM_SOURCE_TMPF_SIZE_MEASURED;
	    Npsf ++;
	    continue;
        }

	// CRs are flagged by a combination on Mminor < options->sizeLimitCR && kmag < options->magLimitCR
	// NOTE: we only flag the CRs here; when we mask them we verify their CR nature (otherwise -> PSF)
        // bool isCR = (kMag < options->magLimitCR) && (Mminor < options->sizeLimitCR);
	// XXX skip if we have already marked it??
        bool isCR = (source->moments->SN > 7.0) && (Mminor < options->sizeLimitCR);
        if (isCR) {
            psTrace("psphotSourceClassRegion.CR",4,"CLASS: %g %g\t%g %g\t%g %g\t%g %g\t%g CR\n",
                    source->peak->xf, source->peak->yf, Mminor, kMag, dMag, nSigmaMAG, options->sizeLimitCR, options->magLimitCR, options->nSigmaApResid);
            source->mode |= PM_SOURCE_MODE_DEFECT;
            source->tmpFlags |= PM_SOURCE_TMPF_SIZE_CR_CANDIDATE;
	    source->tmpFlags |= PM_SOURCE_TMPF_SIZE_MEASURED;
            Ncr ++;
            continue;
        }

        // Likely extended source (PSF_MAG - KRON_MAG is larger than limit)
        bool isEXT = (nSigmaMAG > options->nSigmaApResid);
        if (isEXT) {
            psTrace("psphotSourceClassRegion.EXT",4,"CLASS: %g %g\t%g %g\t%g %g\t%g %g\t%g EXT\n",
                    source->peak->xf, source->peak->yf, Mminor, kMag, dMag, nSigmaMAG, options->sizeLimitCR, options->magLimitCR, options->nSigmaApResid);
	    source->type = PM_SOURCE_TYPE_EXTENDED;
            source->mode |= PM_SOURCE_MODE_EXT_LIMIT;
            source->tmpFlags |= PM_SOURCE_TMPF_SIZE_MEASURED;
            Next ++;
            continue;
        }

	// Alternate extended source limit calculation
	if (options->altDiffExt) {
	  // ratio of major to minor axes
	  // MRV = Major / Minor
	  // MRV = (0.5 * (Mxx + Myy) + 0.5 * sqrt( (Mxx + Myy)^2 + 4 Mxy^2)) /
	  //       (0.5 * (Mxx + Myy) - 0.5 * sqrt( (Mxx + Myy)^2 + 4 Mxy^2))
	  // MRV = (2 * 0.5 * (Mxx + Myy) - Minor) / Minor
	  float momentRatioVeres = (Mxx + Myy - Mminor) / Mminor;
	  bool  isAltEXT = (momentRatioVeres > options->altDiffExtThresh);
	  if (isAltEXT) {
	    psTrace("psphotSourceClassRegion.EXT",4,"CLASS: %g %g\t%g %g\t%g %g\t%g %g\t%g ALTEXT\t%g %g\n",
                    source->peak->xf, source->peak->yf, Mminor, kMag, dMag, nSigmaMAG, options->sizeLimitCR, options->magLimitCR, options->nSigmaApResid,
		    momentRatioVeres,options->altDiffExtThresh);
	    source->type = PM_SOURCE_TYPE_EXTENDED;
            source->mode |= PM_SOURCE_MODE_EXT_LIMIT;
            source->tmpFlags |= PM_SOURCE_TMPF_SIZE_MEASURED;
            Next ++;
            continue;
	  }
	}
	
        // Everything else should just be treated as a PSF
	psTrace("psphotSourceClassRegion.PSF",4,"CLASS: %g %g\t%g %g\t%g %g\t%g %g\t%g PSF\n",
		source->peak->xf, source->peak->yf, Mminor, kMag, dMag, nSigmaMAG, options->sizeLimitCR, options->magLimitCR, options->nSigmaApResid);
	source->tmpFlags |= PM_SOURCE_TMPF_SIZE_MEASURED;
	Npsf ++;
    }

    psLogMsg("psModules.objects", PS_LOG_INFO, "Source Size classifications: %4d %4d %4d %4d %4d", Npsf, Next, Nsat, Ncr, Nskip);

    return true;
}

// given an object suspected to be a defect, generate a pixel mask using the Lapacian transform
// if enough of the object is detected as 'sharp', consider the object a cosmic ray
bool psphotSourceSelectCR (pmReadout *readout, psArray *sources, psphotSourceSizeOptions *options) {

    psTimerStart ("psphot.cr");

    int nMasked = 0;
    for (int i = 0; i < sources->n; i++) {
        pmSource *source = sources->data[i];

        // only check candidates marked above
        if (!(source->tmpFlags & PM_SOURCE_TMPF_SIZE_CR_CANDIDATE)) {
            psTrace("psphot", 7, "Not calculating source size since it has already been measured\n");
            continue;
        }

	// once we are here, remove the temporary flag (this allows us to try again if we do not mark it now)
	source->tmpFlags &= ~PM_SOURCE_TMPF_SIZE_CR_CANDIDATE;

        // Integer position of peak
        int xPeak = source->peak->xf - source->pixels->col0 + 0.5;
        int yPeak = source->peak->yf - source->pixels->row0 + 0.5;

        // Skip sources which are too close to a boundary.  These are mostly caught as DEFECT
        if (xPeak < 1 || xPeak > source->pixels->numCols - 2 ||
            yPeak < 1 || yPeak > source->pixels->numRows - 2) {
            psTrace("psphot", 7, "Not calculating crNsigma due to edge\n");
            continue;
        }

        // XXX for testing, only CRMASK a single source:
        if (options->xtest && (fabs(source->peak->xf - options->xtest) > 5)) continue;
        if (options->ytest && (fabs(source->peak->yf - options->ytest) > 5)) continue;

        // replace object in image
        if (source->tmpFlags & PM_SOURCE_TMPF_SUBTRACTED) {
            pmSourceAdd (source, PM_MODEL_OP_FULL, options->maskVal);
        }

        // XXX this is running slowly and is too agressive, but it more-or-less works
	// XXX EAM : note that injected sources do not normally have a footprint to use in masking
        psTrace("psphot", 6, "mask cosmic ray at %f, %f\n", source->peak->xf, source->peak->yf);
        if (options->applyCRmask && source->peak->footprint) {
            psphotMaskCosmicRay(readout, source, options->crMask, options->maxWindowCR);
        } else {
            source->mode |= PM_SOURCE_MODE_CR_LIMIT;
        }
        nMasked ++;

        // re-subtract the object, leave local sky
        pmSourceSub (source, PM_MODEL_OP_FULL, options->maskVal);
    }

    // now that we have masked pixels associated with CRs, we can grow the mask
    if (options->grow > 0) {
        bool oldThreads = psImageConvolveSetThreads(true); // Old value of threading for psImageConvolveMask
        psImage *newMask = psImageConvolveMask(NULL, readout->mask, options->crMask, options->crMask, -options->grow, options->grow, -options->grow, options->grow);
        psImageConvolveSetThreads(oldThreads);
        if (!newMask) {
            psError(PS_ERR_UNKNOWN, false, "Unable to grow CR mask");
            return false;
        }
	// Copy the new mask pixel values to the old mask array.  NOTE: we cannot replace
	// the mask pointer because the source->maskView objects point to the old data
	// area
	psImage *oldMask = readout->mask;
	readout->mask = psImageCopy (readout->mask, newMask, PS_TYPE_IMAGE_MASK);
	psAssert (oldMask == readout->mask, "should have copied the values in-situ");
        psFree(newMask);
    }

    psLogMsg ("psphot.cr", PS_LOG_INFO, "mask CR: %d masked in %f sec\n", nMasked, psTimerMark ("psphot.cr"));

    // XXX test : save the mask image
# if (DUMPPICS)
    psphotSaveImage (NULL, readout->mask,   "crmask.fits");
# endif

    return true;
}

# define LIMIT_XRANGE(X, IMAGE) { X = PS_MIN(PS_MAX(0, X), IMAGE->numCols); }
# define LIMIT_YRANGE(Y, IMAGE) { Y = PS_MIN(PS_MAX(0, Y), IMAGE->numRows); }

// Comments by CZW 20091209 : Mechanics of how to identify CR pixels taken from "Cosmic-Ray
// Rejection by Laplacian Edge Detection" by Pieter van Dokkum, arXiv:astro-ph/0108003.  This
// does no repair or recovery of the CR pixels, it only masks them out.  My test code can be
// found at /data/ipp031.0/watersc1/psphot.20091209/algo_check.c
bool psphotMaskCosmicRay (pmReadout *readout, pmSource *source, psImageMaskType maskVal, int maxWindowCR) {

    // Get the actual images and information about the peak.
    psImage *mask = readout->mask;
    pmPeak *peak = source->peak;
    pmFootprint *footprint = peak->footprint;

    // Bounding boxes are inclusive of final pixel
    int xs = footprint->bbox.x0;
    int xe = footprint->bbox.x1 + 1;
    int ys = footprint->bbox.y0;
    int ye = footprint->bbox.y1 + 1;

    // We occasionally get very large footprints. When the footprint bounding box is large this function will
    // do an incredible amount of work for no benefit.
    // Limit the size of the area we examine.
    if (xe - xs > maxWindowCR) {
        xs = peak->x - maxWindowCR / 2;
        xe = xs + maxWindowCR;
    }
    if (ye - ys > maxWindowCR) {
        ys = peak->y - maxWindowCR / 2;
        ye = ys + maxWindowCR;
    }

    LIMIT_XRANGE(xs, mask);
    LIMIT_XRANGE(xe, mask);
    LIMIT_YRANGE(ys, mask);
    LIMIT_YRANGE(ye, mask);

    // the peak must be contained in the mask..
    if (peak->x < xs) return false;
    if (peak->y < ys) return false;
    if (peak->x >= xe) return false;
    if (peak->y >= ye) return false;

    int dx = xe - xs;
    int dy = ye - ys;

    psImage *image= readout->image;
    psImage *variance = readout->variance;

    int binning = 2;
    float sigma_thresh = 3.0;
    int max_iter = 1; // XXX with isophot masking, we only want to do a single pass

    // Temporary images.
    psImage *mypix  = psImageAlloc(dx,dy,image->type.type);
    psImage *myfix  = psImageAlloc(dx,dy,image->type.type);
    psImage *myvar  = psImageAlloc(dx,dy,image->type.type);
    psImage *binned = psImageAlloc(dx * binning,dy * binning,image->type.type);
    psImage *conved = psImageAlloc(dx * binning,dy * binning,image->type.type);
    psImage *edges  = psImageAlloc(dx,dy,image->type.type);
    psImage *mymask = psImageAlloc(dx,dy,PS_TYPE_IMAGE_MASK);

    // Load my copy of things.
    for (int y = 0; y < dy; y++) {
        for (int x = 0; x < dx; x++) {
            mypix->data.F32[y][x] = image->data.F32[y+ys][x+xs];
            myvar->data.F32[y][x] = variance->data.F32[y+ys][x+xs];
            mymask->data.PS_TYPE_IMAGE_MASK_DATA[y][x] = 0x00;
        }
    }
    // Mask so I can see on the output image where the footprint is.
    for (int i = 0; i < footprint->spans->n; i++) {
        pmSpan *sp = footprint->spans->data[i];
        int y = sp->y - ys;
        if (y < 0 || y >= mymask->numRows) {
            // can we break if y >= numRows?
            continue;
        }
        for (int x = PS_MAX(sp->x0 - xs, 0); x <= PS_MIN(sp->x1 - xs, mymask->numCols-1); x++) {
            mymask->data.PS_TYPE_IMAGE_MASK_DATA[y][x] |= 0x01;
        }
    }

    int nCRpix = 1; // force at least one pass...
    for (int iteration = 0; (iteration < max_iter) && (nCRpix > 0); iteration++) {
        nCRpix = 0;
        psImageInit (binned, 0.0);
        psImageInit (conved, 0.0);
        psImageInit (edges, 0.0);

        // Make subsampled image. Maybe this should be called "unbinned" or something
        for (int y = 0; y < binning * dy; y++) {
            int yraw = y / binning;
            for (int x = 0; x < binning * dx; x++) {
                int xraw = x / binning;
                binned->data.F32[y][x] = mypix->data.F32[yraw][xraw];
            }
        }

        // Apply Laplace transform (kernel = [[0 -0.25 0][-0.25 1 -0.25][0 -0.25 0]]), clipping at zero
        for (int y = 1; y < binning * dy - 1; y++) {
            for (int x = 1; x < binning * dx - 1; x++) {
                float value = binned->data.F32[y][x] - 0.25 *
                    (binned->data.F32[y+0][x-1] + binned->data.F32[y+0][x+1] +
                     binned->data.F32[y-1][x+0] + binned->data.F32[y+1][x+0]);
                value = PS_MAX(0.0, value);

                conved->data.F32[y][x] = value;
            }
        }

        // Create an edge map by rebinning
        for (int y = 0; y < binning * dy; y++) {
            int yraw = y / binning;
            for (int x = 0; x < binning * dx; x++) {
                int xraw = x / binning;
                edges->data.F32[yraw][xraw] += conved->data.F32[y][x];
            }
        }

        // coordinate of peak in subimage pixels:
        int xPeak = peak->x - xs;
        int yPeak = peak->y - ys;

        // Modify my mask if we're above the significance threshold, but only for connected pixels
        nCRpix = psphotMaskCosmicRayConnected (xPeak, yPeak, mymask, myvar, edges, binning, sigma_thresh);

# if DUMPPICS
        psphotSaveImage (NULL, mypix,   "crmask.pix.fits");
# endif

// XXX do not repair the pixels in isophot version
# if 0
        // "Repair" Masked pixels for the next round.
        for (int y = 1; y < dy - 1; y++) {
            for (int x = 1; x < dx - 1; x++) {
                if (!(mymask->data.PS_TYPE_IMAGE_MASK_DATA[y][x] & 0x40)) {
                    myfix->data.F32[y][x] = mypix->data.F32[y][x];
                    continue;
                }
                myfix->data.F32[y][x] = 0.25 *
                    (mypix->data.F32[y+0][x-1] + mypix->data.F32[y+0][x+1] +
                     mypix->data.F32[y-1][x+0] + mypix->data.F32[y+1][x+0]);
            }
        }

        // "Repair" Masked pixels for the next round.
        for (int y = 1; y < dy - 1; y++) {
            for (int x = 1; x < dx - 1; x++) {
                mypix->data.F32[y][x] = myfix->data.F32[y][x];
            }
        }
# endif

# if DUMPPICS
        fprintf (stderr, "CRMASK %d %d %d %d %d\n", xs, ys, dx, dy, iteration);
        psphotSaveImage (NULL, mypix,   "crmask.fix.fits");
        psphotSaveImage (NULL, myvar,   "crmask.var.fits");
        psphotSaveImage (NULL, binned,  "crmask.binn.fits");
        psphotSaveImage (NULL, conved,  "crmask.conv.fits");
        psphotSaveImage (NULL, edges,   "crmask.edge.fits");
        psphotSaveImage (NULL, mymask,  "crmask.mask.fits");
# endif
        psTrace("psphot.czw",2,"Iter: %d Count: %d",iteration, nCRpix);
    }

# if 0
    // A solitary masked pixel is likely a lie. Remove those
    // XXX can't we use nCRpix == 1 to test for these?
    for (int x = 0; x < dx; x++) {
        for (int y = 0; y < dy; y++) {
            if (!(mymask->data.PS_TYPE_IMAGE_MASK_DATA[y][x] & 0x40)) continue;
            if ((x-1 >= 0) && (mymask->data.PS_TYPE_IMAGE_MASK_DATA[y][x-1] & 0x40)) {
                continue;
            }
            if ((y-1 >= 0) && (mymask->data.PS_TYPE_IMAGE_MASK_DATA[y-1][x] & 0x40)) {
                continue;
            }
            if ((x+1 < dx) && (mymask->data.PS_TYPE_IMAGE_MASK_DATA[y][x+1] & 0x40)) {
                continue;
            }
            if ((y+1 < dy) && (mymask->data.PS_TYPE_IMAGE_MASK_DATA[y+1][x] & 0x40)) {
                continue;
            }
            mymask->data.PS_TYPE_IMAGE_MASK_DATA[y][x] ^= 0x40;
        }
    }
# endif

    // transfer temporary mask to real mask & count masked pixels
    nCRpix = 0;
    for (int x = 0; x < dx; x++) {
        for (int y = 0; y < dy; y++) {
            if (mymask->data.PS_TYPE_IMAGE_MASK_DATA[y][x] & 0x40) {
                mask->data.PS_TYPE_IMAGE_MASK_DATA[y+ys+mask->row0][x+xs+mask->col0] |= maskVal;
                nCRpix ++;
            }
        }
    }

    // XXX if we decide this REALLY is a cosmic ray, set the CR_LIMIT bit
    if (nCRpix > 1) {
        source->mode |= PM_SOURCE_MODE_CR_LIMIT;
    }
    // fprintf (stderr, "CRMASK %d %d %d %d %d\n", peak->x, peak->y, dx, dy, nCRpix);

    psFree(mypix);
    psFree(myfix);
    psFree(myvar);
    psFree(binned);
    psFree(conved);
    psFree(edges);
    psFree(mymask);

    return true;
}

# define VERBOSE 0
int psphotMaskCosmicRayConnected (int xPeak, int yPeak, psImage *mymask, psImage *myvar, psImage *edges, int binning, float sigma_thresh) {

    int xLo, xRo;
    int nCRpix = 0;

    float noise_factor = 5.0 / 4.0;  // Intrinsic to the Laplacian making noise spikes spikier.

    // mark the pixels in this row to the left, then the right. stay within footprint
    int xL = xPeak; // find the range of valid pixels in this row
    int xR = xPeak;
    for (int ix = xPeak; (ix >= 0) && (mymask->data.PS_TYPE_IMAGE_MASK_DATA[yPeak][ix] & 0x01); ix--) {
        float noise = binning * sqrt(noise_factor * myvar->data.F32[yPeak][ix]);
        float value = edges->data.F32[yPeak][ix] / noise;
        if (value < sigma_thresh ) break;
        mymask->data.PS_TYPE_IMAGE_MASK_DATA[yPeak][ix] |= 0x40;
        xL = ix;
        nCRpix ++;
        if (VERBOSE) fprintf (stderr, "mark %d,%d (%d) : %d - %d\n", ix, yPeak, nCRpix, xL, xR);
    }
    for (int ix = xPeak; (ix < mymask->numCols) && (mymask->data.PS_TYPE_IMAGE_MASK_DATA[yPeak][ix] & 0x01); ix++) {
        float noise = binning * sqrt(noise_factor * myvar->data.F32[yPeak][ix]);
        float value = edges->data.F32[yPeak][ix] / noise;
        if (value < sigma_thresh ) break;
        mymask->data.PS_TYPE_IMAGE_MASK_DATA[yPeak][ix] |= 0x40;
        xR = ix;
        nCRpix ++;
        if (VERBOSE) fprintf (stderr, "mark %d,%d (%d) : %d - %d\n", ix, yPeak, nCRpix, xL, xR);
    }
    // xL and xR mark the first and last valid pixel in the row

    // for each of the neighboring rows, mark the high pixels if they touch the range xL to xR
    xLo = PS_MAX(xL - 1, 0);
    xRo = PS_MIN(xR + 1, mymask->numCols);

    // first go down:
    for (int iy = yPeak - 1; iy >= 0; iy--) {

        int xLn = -1;
        int xRn = -1;
        int newPix = 0;

        // mark the pixels in the good range
        for (int ix = xLo; ix < xRo; ix++) {
            if (!(mymask->data.PS_TYPE_IMAGE_MASK_DATA[iy][ix] & 0x01)) continue; // only use pixels in the footprint
            float noise = binning * sqrt(noise_factor * myvar->data.F32[iy][ix]);
            float value = edges->data.F32[iy][ix] / noise;
            if (value < sigma_thresh ) continue;
            mymask->data.PS_TYPE_IMAGE_MASK_DATA[iy][ix] |= 0x40;
            if (xLn == -1) xLn = ix; // first valid pixel in this row
            xRn = ix;                // last valid pixel in this row
            nCRpix ++;
            newPix ++;
            if (VERBOSE) fprintf (stderr, "mark C %d,%d (%d) : %d - %d | %d - %d | %d - %d \n", ix, iy, nCRpix, xL, xR, xLo, xRo, xLn, xRn);
        }

        // mark the pixels to the left of the good range
        for (int ix = xLo; (ix >= 0) && (mymask->data.PS_TYPE_IMAGE_MASK_DATA[iy][ix] & 0x01); ix--) {
            float noise = binning * sqrt(noise_factor * myvar->data.F32[iy][ix]);
            float value = edges->data.F32[iy][ix] / noise;
            if (value < sigma_thresh ) break;
            mymask->data.PS_TYPE_IMAGE_MASK_DATA[iy][ix] |= 0x40;
            if (xRn == -1) xRn = ix; // last valid pixel in this row
            xLn = ix;
            nCRpix ++;
            newPix ++;
            if (VERBOSE) fprintf (stderr, "mark L %d,%d (%d) : %d - %d | %d - %d | %d - %d \n", ix, iy, nCRpix, xL, xR, xLo, xRo, xLn, xRn);
        }

        // mark the pixels to the right of the good range
        for (int ix = xRo; (ix < mymask->numCols) && (mymask->data.PS_TYPE_IMAGE_MASK_DATA[iy][ix] & 0x01); ix++) {
            float noise = binning * sqrt(noise_factor * myvar->data.F32[iy][ix]);
            float value = edges->data.F32[iy][ix] / noise;
            if (value < sigma_thresh ) break;
            mymask->data.PS_TYPE_IMAGE_MASK_DATA[iy][ix] |= 0x40;
            if (xLn == -1) xLn = ix; // first valid pixel in this row
            xRn = ix;
            nCRpix ++;
            newPix ++;
            if (VERBOSE) fprintf (stderr, "mark R %d,%d (%d) : %d - %d | %d - %d | %d - %d \n", ix, iy, nCRpix, xL, xR, xLo, xRo, xLn, xRn);
        }
        if (newPix == 0) break;
        xLo = PS_MAX(xLn - 1, 0);
        xRo = PS_MIN(xRn + 1, mymask->numCols);
    }

    xLo = PS_MAX(xL - 1, 0);
    xRo = PS_MIN(xR + 1, mymask->numCols);

    // next go up:
    for (int iy = yPeak + 1; iy < mymask->numRows; iy++) {

        int xLn = -1;
        int xRn = -1;
        int newPix = 0;

        // mark the pixels in the good range
        for (int ix = xLo; ix < xRo; ix++) {
            if (!(mymask->data.PS_TYPE_IMAGE_MASK_DATA[iy][ix] & 0x01)) continue; // only use pixels in the footprint
            float noise = binning * sqrt(noise_factor * myvar->data.F32[iy][ix]);
            float value = edges->data.F32[iy][ix] / noise;
            if (value < sigma_thresh ) continue;
            mymask->data.PS_TYPE_IMAGE_MASK_DATA[iy][ix] |= 0x40;
            if (xLn == -1) xLn = ix; // first valid pixel in this row
            xRn = ix;                // last valid pixel in this row
            nCRpix ++;
            newPix ++;
            if (VERBOSE) fprintf (stderr, "mark C %d,%d (%d) : %d - %d | %d - %d | %d - %d \n", ix, iy, nCRpix, xL, xR, xLo, xRo, xLn, xRn);
        }

        // mark the pixels to the left of the good range
        for (int ix = xLo; (ix >= 0) && (mymask->data.PS_TYPE_IMAGE_MASK_DATA[iy][ix] & 0x01); ix--) {
            float noise = binning * sqrt(noise_factor * myvar->data.F32[iy][ix]);
            float value = edges->data.F32[iy][ix] / noise;
            if (value < sigma_thresh ) break;
            mymask->data.PS_TYPE_IMAGE_MASK_DATA[iy][ix] |= 0x40;
            if (xRn == -1) xRn = ix; // last valid pixel in this row
            xLn = ix;
            nCRpix ++;
            newPix ++;
            if (VERBOSE) fprintf (stderr, "mark L %d,%d (%d) : %d - %d | %d - %d | %d - %d \n", ix, iy, nCRpix, xL, xR, xLo, xRo, xLn, xRn);
        }

        // mark the pixels to the right of the good range
        for (int ix = xRo; (ix < mymask->numCols) && (mymask->data.PS_TYPE_IMAGE_MASK_DATA[iy][ix] & 0x01); ix++) {
            float noise = binning * sqrt(noise_factor * myvar->data.F32[iy][ix]);
            float value = edges->data.F32[iy][ix] / noise;
            if (value < sigma_thresh ) break;
            mymask->data.PS_TYPE_IMAGE_MASK_DATA[iy][ix] |= 0x40;
            if (xLn == -1) xLn = ix; // first valid pixel in this row
            xRn = ix;
            nCRpix ++;
            newPix ++;
            if (VERBOSE) fprintf (stderr, "mark R %d,%d (%d) : %d - %d | %d - %d | %d - %d \n", ix, iy, nCRpix, xL, xR, xLo, xRo, xLn, xRn);
        }
        if (newPix == 0) break;
        xLo = PS_MAX(xLn - 1, 0);
        xRo = PS_MIN(xRn + 1, mymask->numCols);
    }

    return nCRpix;
}

float psphotSourceSizeFindThreshold (psVector *value, psVector *mask, int maskValue, float minValue, float maxValue, float delta, float guess, float fraction) {

    // make a histogram of the sources with mKron < magLimit
    int nValue = (int)((maxValue - minValue) / delta) + 1;

    psVector *histogram = psVectorAlloc(nValue, PS_TYPE_S32);
    psVectorInit (histogram, 0);

    for (int i = 0; i < value->n; i++) {
	if (mask->data.U8[i] & maskValue) continue;
	int bin = (value->data.F32[i] - minValue) / delta;
	if (bin < 0.0) continue;
	if (bin > histogram->n - 1) continue;
	histogram->data.S32[bin] ++;
    }

    // find the peak of this histogram, but stop search at guess
    int last = (guess - minValue) / delta;
    int nPeak = 0;
    int vPeak = histogram->data.S32[0];
    for (int i = 1; (i < last) && (i < histogram->n); i++) {
	if (histogram->data.S32[i] < vPeak) continue;
	nPeak = i;
	vPeak = histogram->data.S32[i];
    }

    // start at the peak and find the valley between here and the end of the histogram
    int nValley = nPeak;
    int vValley = histogram->data.S32[nPeak];
    for (int i = nPeak + 1; i < histogram->n; i++) {
	if (histogram->data.S32[i] > vValley) continue;
	nValley = i;
	vValley = histogram->data.S32[i];
    }

    psLogMsg ("psphot", PS_LOG_MINUTIA, "CR limits threshold : peak %d @ %f, valley %d @ %f (%f sigma)\n", 
	      vPeak, delta * nPeak + minValue, vValley, delta * nValley + minValue, vPeak / PS_MAX(1.0, sqrt(vValley)));

    if (nValley == nPeak) {
	psFree (histogram);
	return NAN;
    }

    if (vPeak < 3.0*sqrt(vValley)) {
	psFree (histogram);
	return NAN;
    }

    /// search for the first bin after the peak with f < vValley + 0.05*(vPeak - vValley)
    float vLimit = vValley + fraction*(vPeak - vValley);
    int nLimit = nValley;
    for (int i = nPeak; i < nValley; i++) {
	if (histogram->data.S32[i] > vLimit) continue;
	nLimit = i;
	break;
    }
    float result = nLimit * delta + minValue;

    psFree (histogram);
    
    return result;
}
