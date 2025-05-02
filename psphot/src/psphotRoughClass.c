# include "psphotInternal.h"

# define CHECK_STATUS(S,MSG) {                                          \
        if (!status) {                                                  \
            psError(PSPHOT_ERR_CONFIG, false, "missing PSF Clump entry: %s\n", MSG); \
            return false;                                               \
        } }

// for now, let's store the detections on the readout->analysis for each readout
bool psphotRoughClass (pmConfig *config, const pmFPAview *view, const char *filerule)
{
    bool status = true;

    fprintf (stdout, "\n");
    psLogMsg ("psphot", PS_LOG_INFO, "--- psphot Rough Class ---");

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
        if (!psphotRoughClassReadout (config, view, filerule, i, recipe)) {
            psError (PSPHOT_ERR_CONFIG, false, "failed on rough classification for %s entry %d", filerule, i);
            return false;
        }
    }
    return true;
}

bool psphotRoughClassReadout (pmConfig *config, const pmFPAview *view, const char *filerule, int index, psMetadata *recipe) {

    bool status;

    psTimerStart ("psphot.rough");

    // find the currently selected readout
    pmFPAfile *file = pmFPAfileSelectSingle(config->files, filerule, index); // File of interest
    psAssert (file, "missing file?");

    pmReadout *readout = pmFPAviewThisReadout(view, file->fpa);
    psAssert (readout, "missing readout?");

    // if we have a PSF, use the existing PSF clump region below
    bool havePSF = false;
    if (psMetadataLookupPtr (&status, readout->analysis, "PSPHOT.PSF")) {
        havePSF = true;
    }

    pmDetections *detections = psMetadataLookupPtr (&status, readout->analysis, "PSPHOT.DETECTIONS");
    psAssert (detections, "missing detections?");

    psArray *sources = detections->newSources;
    psAssert (sources, "missing sources?");

    if (!sources->n) {
        psLogMsg ("psphot", PS_LOG_INFO, "no sources, skipping rough classification");
        return true;
    }

    int NstarsInClump = psMetadataLookupS32 (&status, readout->analysis, "PSF_CLUMP_NSTARS");
    // if NstarsInClump is not defined, use the user-selected option:
    if (!status) {
	NstarsInClump = 1000;
    }

    int ScaleForClump = 1;
    if (NstarsInClump >= 12) ScaleForClump = 2; // 4 cells
    if (NstarsInClump >= 27) ScaleForClump = 3; // 9 cells
    if (NstarsInClump >= 48) ScaleForClump = 4; // 16 cells
    if (NstarsInClump >  75) ScaleForClump = 5; // 25 cells

    // we make this measurement on a NxM grid of regions across the readout
    int NX  = psMetadataLookupS32 (&status, recipe, "PSF_CLUMP_NX");  CHECK_STATUS (status, "PSF_CLUMP_NX");
    int NY  = psMetadataLookupS32 (&status, recipe, "PSF_CLUMP_NY");  CHECK_STATUS (status, "PSF_CLUMP_NY");

    int NXuse, NYuse;

    ScaleForClump = PS_MIN(ScaleForClump, PS_MAX(NX, NY));
    if (NX > NY) {
	NXuse = ScaleForClump;
	NYuse = (int) (ScaleForClump * (NX / NY) + 0.5);
    } else {
	NYuse = ScaleForClump;
	NXuse = (int) (ScaleForClump * (NY / NX) + 0.5);
    }

    psLogMsg ("psphot", 4, "With %d stars, using %d x %d grid for PSF clump\n", NstarsInClump, NXuse, NYuse);

    int dX  = readout->image->numCols / NXuse;
    int dY  = readout->image->numRows / NYuse;

    int nRegion = 0;
    for (int ix = 0; ix < NXuse; ix ++) {
        for (int iy = 0; iy < NYuse; iy ++) {

            psRegion *region = psRegionAlloc (ix*dX, (ix + 1)*dX, iy*dY, (iy + 1)*dY);
            if (!psphotRoughClassRegion (nRegion, region, sources, readout->analysis, recipe, havePSF)) {
                psLogMsg ("psphot", 4, "Failed to determine rough classification for region %f,%f - %f,%f\n",
                         region->x0, region->y0, region->x1, region->y1);

                // If in doubt, it's a PSF
                for (int i = 0; i < sources->n; i++) {
                    pmSource *source = sources->data[i]; // Source of interest
                    if (!source || !source->peak) {
                        continue;
                    }
                    if (source->peak->x <  region->x0) continue;
                    if (source->peak->x >= region->x1) continue;
                    if (source->peak->y <  region->y0) continue;
                    if (source->peak->y >= region->y1) continue;
                    source->type = PM_SOURCE_TYPE_STAR;
                }
                psFree (region);
                continue;
            }
            psFree (region);
            nRegion ++;
        }
    }
    psMetadataAddS32 (readout->analysis, PS_LIST_TAIL, "PSF.CLUMP.NREGIONS",  PS_META_REPLACE, "psf clump regions", nRegion);

    // optional printout of source moments only
    psphotDumpMoments (recipe, sources);

    psLogMsg ("psphot.roughclass", PS_LOG_WARN, "rough classification: %f sec\n", psTimerMark ("psphot.rough"));

    psphotVisualPlotMoments (recipe, readout->analysis, sources);
    psphotVisualShowRoughClass (sources);
    // XXX better visualization: psphotVisualShowFlags (sources);

    return true;
}

bool psphotRoughClassRegion (int nRegion, psRegion *region, psArray *sources, psMetadata *analysis, psMetadata *recipe, const bool havePSF) {

    bool status;
    char regionName[64];
    pmPSFClump psfClump;

    // user-defined masks to test for good/bad pixels (build from recipe list if not yet set)
    psImageMaskType maskSat = psMetadataLookupImageMask(&status, recipe, "MASK.SAT"); // Mask value for bad pixels
    assert (maskSat);

    // the regions are saved on the readout->analysis metadata folder (passed to us as analysis)
    snprintf (regionName, 64, "PSF.CLUMP.REGION.%03d", nRegion);
    psMetadata *regionMD = psMetadataLookupPtr (&status, analysis, regionName);
    if (!regionMD) {
        // allocate the region metadata folder and add this region to it.
        regionMD = psMetadataAlloc();
        psMetadataAddMetadata (analysis, PS_LIST_TAIL, regionName, PS_META_REPLACE, "psf clump region", regionMD);
        psFree (regionMD);
    }
    psMetadataAddPtr (regionMD, PS_LIST_TAIL, "REGION", PS_DATA_REGION | PS_META_REPLACE, "psf clump region", region);

    if (!havePSF) {
        // determine the PSF parameters from the source moment values
        // XXX why not save the psfClump as a PTR?

        float PSF_SN_LIM = psMetadataLookupF32(&status, recipe, "PSF_SN_LIM"); psAssert (status, "missing PSF_SN_LIM");
        float MOMENTS_AR_MAX = psMetadataLookupF32(&status, recipe, "MOMENTS_AR_MAX"); psAssert (status, "missing MOMENTS_AR_MAX");

        float PSF_CLUMP_GRID_SCALE = psMetadataLookupF32(&status, analysis, "PSF_CLUMP_GRID_SCALE");
        if (!status) {
            PSF_CLUMP_GRID_SCALE = psMetadataLookupF32(&status, recipe, "PSF_CLUMP_GRID_SCALE");
            psAssert (status, "missing PSF_CLUMP_GRID_SCALE");
        }
        float MOMENTS_SX_MAX = psMetadataLookupF32(&status, analysis, "MOMENTS_SX_MAX");
        if (!status) {
            MOMENTS_SX_MAX = psMetadataLookupF32(&status, recipe, "MOMENTS_SX_MAX");
            psAssert (status, "missing MOMENTS_SX_MAX");
        }
        float MOMENTS_SY_MAX = psMetadataLookupF32(&status, analysis, "MOMENTS_SY_MAX");
        if (!status) {
            MOMENTS_SY_MAX = psMetadataLookupF32(&status, recipe, "MOMENTS_SY_MAX");
            psAssert (status, "missing MOMENTS_SY_MAX");
        }
        float MOMENTS_SX_MIN = psMetadataLookupF32(&status, analysis, "MOMENTS_SX_MIN");
        if (!status) {
            MOMENTS_SX_MIN = psMetadataLookupF32(&status, recipe, "MOMENTS_SX_MIN");
            if (!status) {
		MOMENTS_SX_MIN = 0.5;
	    }
        }
        float MOMENTS_SY_MIN = psMetadataLookupF32(&status, analysis, "MOMENTS_SY_MIN");
        if (!status) {
            MOMENTS_SY_MIN = psMetadataLookupF32(&status, recipe, "MOMENTS_SY_MIN");
            if (!status) {
		MOMENTS_SY_MIN = 0.5;
	    }
        }

        psfClump = pmSourcePSFClump (NULL, region, sources, PSF_SN_LIM, PSF_CLUMP_GRID_SCALE, MOMENTS_SX_MAX, MOMENTS_SY_MAX, MOMENTS_SX_MIN, MOMENTS_SY_MIN, MOMENTS_AR_MAX);

        psMetadataAddF32 (regionMD, PS_LIST_TAIL, "PSF.CLUMP.X",  PS_META_REPLACE, "psf clump center", psfClump.X);
        psMetadataAddF32 (regionMD, PS_LIST_TAIL, "PSF.CLUMP.Y",  PS_META_REPLACE, "psf clump center", psfClump.Y);
        psMetadataAddF32 (regionMD, PS_LIST_TAIL, "PSF.CLUMP.DX", PS_META_REPLACE, "psf clump center", psfClump.dX);
        psMetadataAddF32 (regionMD, PS_LIST_TAIL, "PSF.CLUMP.DY", PS_META_REPLACE, "psf clump center", psfClump.dY);
    } else {
        // pull FWHM_X,Y from the recipe, use to define psfClump.X,Y
        psfClump.X  = psMetadataLookupF32 (&status, regionMD, "PSF.CLUMP.X");
        if (!status) {
            psLogMsg ("psphot", 4, "No PSF clump defined for region %f,%f - %f,%f\n", region->x0, region->y0, region->x1, region->y1);
            return false;
        }
        psfClump.Y  = psMetadataLookupF32 (&status, regionMD, "PSF.CLUMP.Y");   psAssert (status, "missing PSF.CLUMP.Y");
        psfClump.dX = psMetadataLookupF32 (&status, regionMD, "PSF.CLUMP.DX");  psAssert (status, "missing PSF.CLUMP.DX");
        psfClump.dY = psMetadataLookupF32 (&status, regionMD, "PSF.CLUMP.DY");  psAssert (status, "missing PSF.CLUMP.DY");
    }

    if (psfClump.X < 0) {
        psError(PSPHOT_ERR_PROG, false, "programming error calling pmSourcePSFClump");
        return false;
    }
    if (!psfClump.X || !psfClump.Y || isnan(psfClump.X) || isnan(psfClump.Y)) {
        psLogMsg ("psphot", 4, "Failed to find a valid PSF clump for region %f,%f - %f,%f\n", region->x0, region->y0, region->x1, region->y1);
        return false;
    }
    if (!havePSF) {
	psLogMsg ("psphot", 3, "psf clump  X,  Y: %f, %f : DX, DY: %f, %f : nStars %d of %d\n", psfClump.X, psfClump.Y, psfClump.dX, psfClump.dY, psfClump.nStars, psfClump.nTotal);
    } else {
	psLogMsg ("psphot", 3, "psf clump  X,  Y: %f, %f : DX, DY: %f, %f : loaded from metadata\n", psfClump.X, psfClump.Y, psfClump.dX, psfClump.dY);
    }

    // get basic parameters, or set defaults
    float PSF_SN_LIM = psMetadataLookupF32 (&status, recipe, "PSF_SN_LIM"); psAssert (status, "missing PSF_SN_LIM");
    float PSF_CLUMP_NSIGMA = psMetadataLookupF32 (&status, recipe, "PSF_CLUMP_NSIGMA"); psAssert (status, "missing PSF_CLUMP_NSIGMA");

    // group into STAR, COSMIC, EXTENDED, SATURATED, etc.
    if (!pmSourceRoughClass (region, sources, PSF_SN_LIM, PSF_CLUMP_NSIGMA, psfClump, maskSat)) {
        psError(PSPHOT_ERR_PROG, false, "programming error calling pmSourceRoughClass");
        return false;
    }

    return true;
}
