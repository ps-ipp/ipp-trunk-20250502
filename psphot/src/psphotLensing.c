# include "psphotInternal.h"

// calculate lensing parameters (we have the moments already)
bool psphotLensing (pmConfig *config, const pmFPAview *view, const char *filerule)
{
    bool status = true;

    fprintf (stdout, "\n");
    psLogMsg ("psphot", PS_LOG_INFO, "--- psphot Lensing Parameters ---");

    // select the appropriate recipe information
    psMetadata *recipe  = psMetadataLookupPtr (&status, config->recipes, PSPHOT_RECIPE);
    psAssert (recipe, "missing recipe?");

    // perform full extended source non-linear fits?
    if (!psMetadataLookupBool (&status, recipe, "LENSING_PARAMETERS")) {
        psLogMsg ("psphot", PS_LOG_INFO, "skipping extended source fits\n");
        return true;
    }

    int num = psphotFileruleCount(config, filerule);

    // skip the chisq image (optionally?)
    int chisqNum = psMetadataLookupS32 (&status, config->arguments, "PSPHOT.CHISQ.NUM");
    if (!status) chisqNum = -1;

    // loop over the available readouts
    for (int i = 0; i < num; i++) {
        if (i == chisqNum) continue; // skip chisq image
        if (!psphotLensingReadout (config, view, filerule, i, recipe)) {
            psError (PSPHOT_ERR_CONFIG, false, "failed to measure lensing parameters for %s entry %d", filerule, i);
            return false;
        }
        if (!psphotLensingPSFtrendsReadout (config, view, filerule, i, recipe)) {
            psError (PSPHOT_ERR_CONFIG, false, "failed to measure lensing parameters for %s entry %d", filerule, i);
            return false;
        }
    }
    return true;
}

// this block measures the lensing parameters for all objects
bool psphotLensingReadout (pmConfig *config, const pmFPAview *view, const char *filerule, int index, psMetadata *recipe)
{
    bool status;
    pmSource *source;

    psTimerStart ("psphot.lensing");

    // find the currently selected readout
    pmFPAfile *file = pmFPAfileSelectSingle(config->files, filerule, index); // File of interest
    psAssert (file, "missing file?");

    pmReadout *readout = pmFPAviewThisReadout(view, file->fpa);
    psAssert (readout, "missing readout?");

    // if we have measured the window, we will be saving the modified version of these recipe values on readout->analysis
    float SIGMA = psMetadataLookupF32 (&status, readout->analysis, "MOMENTS_GAUSS_SIGMA");
    if (!status) {
        SIGMA = psMetadataLookupF32 (&status, recipe, "MOMENTS_GAUSS_SIGMA");
    }

    pmDetections *detections = psMetadataLookupPtr (&status, readout->analysis, "PSPHOT.DETECTIONS");
    psAssert (detections, "missing detections?");

    psArray *sources = detections->allSources;
    psAssert (sources, "missing sources?");

    if (!sources->n) {
        psLogMsg ("psphot", PS_LOG_INFO, "no sources, skipping lensing");
        return true;
    }

    for (int i = 0; i < sources->n; i++) {
        source = sources->data[i];

	// skip saturated stars modeled with a radial profile 
        if (source->mode2 & PM_SOURCE_MODE2_SATSTAR_PROFILE) continue;
        if (source->mode  & PM_SOURCE_MODE_SATSTAR) continue;

        if (source->mode &  PM_SOURCE_MODE_CR_LIMIT) continue;
        if (source->mode &  PM_SOURCE_MODE_DEFECT) continue;

        pmMoments *moments = source->moments;

	if (!source->lensingOBJ) {
	  source->lensingOBJ = pmSourceLensingAlloc ();
	}

	// XXX big objects (eg, saturated stars) use 3*SIGMA.  I'm ignoring this for now
	pmSourceLensingShearFromMoments (source->lensingOBJ, moments, SIGMA);
	pmSourceLensingSmearFromMoments (source->lensingOBJ, moments, SIGMA);
    }

    psMetadataAddBool(readout->analysis, PS_LIST_TAIL, "LENS_OBJ", PS_META_REPLACE, "per-object lensing stats measured", true);
    psLogMsg ("psphot.lensing", PS_LOG_DETAIL, "calculate lensing parameters for %d objects\n", (int) sources->n);

    return true;
}

// generate image maps for the Xij,ei elements based only on good stars
bool psphotLensingPSFtrendsReadout (pmConfig *config, const pmFPAview *view, const char *filerule, int index, psMetadata *recipe)
{
    int Nfail = 0;
    bool status;
    pmSource *source;

    psTimerStart ("psphot.lensing");

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
        psLogMsg ("psphot", PS_LOG_INFO, "no sources, skipping lensing psf trends");
        return true;
    }

    // gather the stats to assess the aperture residuals
    int Npsf = 0;
    psVector *psfX11sm = psVectorAllocEmpty (300, PS_TYPE_F32);
    psVector *psfX12sm = psVectorAllocEmpty (300, PS_TYPE_F32);
    psVector *psfX22sm = psVectorAllocEmpty (300, PS_TYPE_F32);
    psVector *psf_e1sm = psVectorAllocEmpty (300, PS_TYPE_F32);
    psVector *psf_e2sm = psVectorAllocEmpty (300, PS_TYPE_F32);
    psVector *psfX11sh = psVectorAllocEmpty (300, PS_TYPE_F32);
    psVector *psfX12sh = psVectorAllocEmpty (300, PS_TYPE_F32);
    psVector *psfX22sh = psVectorAllocEmpty (300, PS_TYPE_F32);
    psVector *psf_e1sh = psVectorAllocEmpty (300, PS_TYPE_F32);
    psVector *psf_e2sh = psVectorAllocEmpty (300, PS_TYPE_F32);
    psVector *psf_e1   = psVectorAllocEmpty (300, PS_TYPE_F32);
    psVector *psf_e2   = psVectorAllocEmpty (300, PS_TYPE_F32);
    psVector *xPos     = psVectorAllocEmpty (300, PS_TYPE_F32);
    psVector *yPos     = psVectorAllocEmpty (300, PS_TYPE_F32);

    for (int i = 0; i < sources->n; i++) {
        source = sources->data[i];

	// only use good stars:
        if (source->type != PM_SOURCE_TYPE_STAR) continue;
        if (source->mode &  PM_SOURCE_MODE_SATSTAR) continue;
        if (source->mode &  PM_SOURCE_MODE_BLEND) continue;
        if (source->mode &  PM_SOURCE_MODE_FAIL) continue;
        if (source->mode &  PM_SOURCE_MODE_POOR) continue;

	// skip saturated stars modeled with a radial profile 
        if (source->mode2 & PM_SOURCE_MODE2_SATSTAR_PROFILE) continue;

        if (source->mode &  PM_SOURCE_MODE_EXT_LIMIT) continue;
        if (source->mode &  PM_SOURCE_MODE_CR_LIMIT) continue;
        if (source->mode &  PM_SOURCE_MODE_DEFECT) continue;
        if (source->mode2 & PM_SOURCE_MODE2_MATCHED) continue; // XXX ???

	// use psf - ap to limit to good stars?
	// XXX need to be sure if this is before or after ap corrections are done
        if (!isfinite(source->apMag)) continue;
	if (!isfinite(source->psfMag)) continue;

        // XXX make this user-configurable?
        if (source->psfMagErr > 0.03) continue;

        // aperture residual for this source
        float dap = source->apMag - source->psfMag;

	// good stars have |dap| < 0.2
        if (fabs(dap) > 0.2) {
            Nfail ++;
            psTrace ("psphot", 3, "fail : bad dap: %f", dap);
            continue;
        }

	pmModel *model = source->modelPSF;

	float R  = source->moments->Mxx + source->moments->Myy;
	source->lensingOBJ->e1 = (source->moments->Mxx - source->moments->Myy) / R;
	source->lensingOBJ->e2 = 2*source->moments->Mxy / R;

        psVectorAppend (xPos, model->params->data.F32[PM_PAR_XPOS]);
        psVectorAppend (yPos, model->params->data.F32[PM_PAR_YPOS]);
        psVectorAppend (psfX11sm, source->lensingOBJ->smear->X11);
        psVectorAppend (psfX12sm, source->lensingOBJ->smear->X12);
        psVectorAppend (psfX22sm, source->lensingOBJ->smear->X22);
        psVectorAppend (psf_e1sm, source->lensingOBJ->smear->e1);
        psVectorAppend (psf_e2sm, source->lensingOBJ->smear->e2);
        psVectorAppend (psfX11sh, source->lensingOBJ->shear->X11);
        psVectorAppend (psfX12sh, source->lensingOBJ->shear->X12);
        psVectorAppend (psfX22sh, source->lensingOBJ->shear->X22);
        psVectorAppend (psf_e1sh, source->lensingOBJ->shear->e1);
        psVectorAppend (psf_e2sh, source->lensingOBJ->shear->e2);
        psVectorAppend (psf_e1,   source->lensingOBJ->e1);
        psVectorAppend (psf_e2,   source->lensingOBJ->e2);
	Npsf ++;
    }

    // this is a bit tricky, because we have two cases (MAP vs POLY), and they have a different
    // definition for 'order' (order_MAP = order_POLY + 1).  in addition, we have a
    // user-specified MAX order, which we should respect, regardless of the mode

    // set the max order (0 = constant) which the number of psf stars can support:
    // we require only 3 stars for n = 0, increase stars / cell for higher order
    if (Npsf < 3) {
        psLogMsg ("psphot", PS_LOG_INFO, "no sources passed selection criteria, skipping lensing psf trends");
        return true;
    }
    int LensingTrendOrder = 0;
    if (Npsf >=  16) LensingTrendOrder = 1; // 4 cells
    if (Npsf >=  54) LensingTrendOrder = 2; // 9 cells
    if (Npsf >= 128) LensingTrendOrder = 3; // 16 cells
    if (Npsf >= 300) LensingTrendOrder = 4; // 25 cells
    if (Npsf >  576) LensingTrendOrder = 5; // 36 cells

    pmTrend2DMode mode = PM_TREND_MAP;
    if (mode == PM_TREND_MAP) {
        LensingTrendOrder ++;
    }

    // Nx,Ny are the superpixel size; but I need to choose them to allow a max of LensingTrendOrder values
    // I'm not sure this is really sensible, but... 
    int NX = readout->image->numCols;
    int NY = readout->image->numRows;
    int Nx, Ny;
    if (NX > NY) {
      Nx = LensingTrendOrder;
      Ny = PS_MAX (1, (int)(LensingTrendOrder * (NY / (float)(NX)) + 0.5));
    } else {
      Ny = LensingTrendOrder;
      Nx = PS_MAX (1, (int)(LensingTrendOrder * (NX / (float)(NY)) + 0.5));
    }

    psLogMsg ("psphot.lensing", PS_LOG_DETAIL, "generate lensing maps using %d objects (%d x %d grid)\n", Npsf, Nx, Ny);

    // XXX allow user to set this optionally?
    psStats *stats = psStatsAlloc (PS_STAT_SAMPLE_MEDIAN | PS_STAT_SAMPLE_STDEV);

    // measure Trend2D for the current spatial scale
    pmTrend2D *trendX11sm = pmTrend2DAlloc (PM_TREND_MAP, readout->image, Nx, Ny, stats);
    pmTrend2D *trendX12sm = pmTrend2DAlloc (PM_TREND_MAP, readout->image, Nx, Ny, stats);
    pmTrend2D *trendX22sm = pmTrend2DAlloc (PM_TREND_MAP, readout->image, Nx, Ny, stats);
    pmTrend2D *trend_e1sm = pmTrend2DAlloc (PM_TREND_MAP, readout->image, Nx, Ny, stats);
    pmTrend2D *trend_e2sm = pmTrend2DAlloc (PM_TREND_MAP, readout->image, Nx, Ny, stats);

    pmTrend2D *trendX11sh = pmTrend2DAlloc (PM_TREND_MAP, readout->image, Nx, Ny, stats);
    pmTrend2D *trendX12sh = pmTrend2DAlloc (PM_TREND_MAP, readout->image, Nx, Ny, stats);
    pmTrend2D *trendX22sh = pmTrend2DAlloc (PM_TREND_MAP, readout->image, Nx, Ny, stats);
    pmTrend2D *trend_e1sh = pmTrend2DAlloc (PM_TREND_MAP, readout->image, Nx, Ny, stats);
    pmTrend2D *trend_e2sh = pmTrend2DAlloc (PM_TREND_MAP, readout->image, Nx, Ny, stats);

    pmTrend2D *trend_e1   = pmTrend2DAlloc (PM_TREND_MAP, readout->image, Nx, Ny, stats);
    pmTrend2D *trend_e2   = pmTrend2DAlloc (PM_TREND_MAP, readout->image, Nx, Ny, stats);

    bool goodFit = false;
    if (!pmTrend2DFit (&goodFit, trendX11sm, NULL, 0xff, xPos, yPos, psfX11sm, NULL)) { psWarning ("failed to measure X11 smear trend"); }
    if (!pmTrend2DFit (&goodFit, trendX12sm, NULL, 0xff, xPos, yPos, psfX12sm, NULL)) { psWarning ("failed to measure X12 smear trend"); }
    if (!pmTrend2DFit (&goodFit, trendX22sm, NULL, 0xff, xPos, yPos, psfX22sm, NULL)) { psWarning ("failed to measure X22 smear trend"); }
    if (!pmTrend2DFit (&goodFit, trend_e1sm, NULL, 0xff, xPos, yPos, psf_e1sm, NULL)) { psWarning ("failed to measure  e1 smear trend"); }
    if (!pmTrend2DFit (&goodFit, trend_e2sm, NULL, 0xff, xPos, yPos, psf_e2sm, NULL)) { psWarning ("failed to measure  e2 smear trend"); }

    if (!pmTrend2DFit (&goodFit, trendX11sh, NULL, 0xff, xPos, yPos, psfX11sh, NULL)) { psWarning ("failed to measure X11 shear trend"); }
    if (!pmTrend2DFit (&goodFit, trendX12sh, NULL, 0xff, xPos, yPos, psfX12sh, NULL)) { psWarning ("failed to measure X12 shear trend"); }
    if (!pmTrend2DFit (&goodFit, trendX22sh, NULL, 0xff, xPos, yPos, psfX22sh, NULL)) { psWarning ("failed to measure X22 shear trend"); }
    if (!pmTrend2DFit (&goodFit, trend_e1sh, NULL, 0xff, xPos, yPos, psf_e1sh, NULL)) { psWarning ("failed to measure  e1 shear trend"); }
    if (!pmTrend2DFit (&goodFit, trend_e2sh, NULL, 0xff, xPos, yPos, psf_e2sh, NULL)) { psWarning ("failed to measure  e2 shear trend"); }

    if (!pmTrend2DFit (&goodFit, trend_e1,   NULL, 0xff, xPos, yPos, psf_e1,   NULL)) { psWarning ("failed to measure  e1 trend"); }
    if (!pmTrend2DFit (&goodFit, trend_e2,   NULL, 0xff, xPos, yPos, psf_e2,   NULL)) { psWarning ("failed to measure  e2 trend"); }

    // evaluate the PSF trend maps at the location of all sources
    for (int i = 0; i < sources->n; i++) {
        source = sources->data[i];
	pmPeak *peak = source->peak;
	if (!peak) continue;

	float xPos = peak->xf;
	float yPos = peak->yf;

	if (!source->lensingPSF) {
	  source->lensingPSF = pmSourceLensingAlloc ();
	}
	pmSourceLensing *lensing = source->lensingPSF;

	if (!lensing->shear) {
	  lensing->shear = pmLensingParsAlloc();
	}
	pmLensingPars *shear = lensing->shear;

	shear->X11 = pmTrend2DEval (trendX11sh, xPos, yPos);
	shear->X12 = pmTrend2DEval (trendX12sh, xPos, yPos);
	shear->X22 = pmTrend2DEval (trendX22sh, xPos, yPos);
	shear->e1  = pmTrend2DEval (trend_e1sh, xPos, yPos);
	shear->e2  = pmTrend2DEval (trend_e2sh, xPos, yPos);
	
	if (!lensing->smear) {
	  lensing->smear = pmLensingParsAlloc();
	}
	pmLensingPars *smear = lensing->smear;

	smear->X11 = pmTrend2DEval (trendX11sm, xPos, yPos);
	smear->X12 = pmTrend2DEval (trendX12sm, xPos, yPos);
	smear->X22 = pmTrend2DEval (trendX22sm, xPos, yPos);
	smear->e1  = pmTrend2DEval (trend_e1sm, xPos, yPos);
	smear->e2  = pmTrend2DEval (trend_e2sm, xPos, yPos);

	lensing->e1  = pmTrend2DEval (trend_e1, xPos, yPos);
	lensing->e2  = pmTrend2DEval (trend_e2, xPos, yPos);
    }

    psFree (trendX11sm);
    psFree (trendX12sm);
    psFree (trendX22sm);
    psFree (trend_e1sm);
    psFree (trend_e2sm);
	              
    psFree (trendX11sh);
    psFree (trendX12sh);
    psFree (trendX22sh);
    psFree (trend_e1sh);
    psFree (trend_e2sh);

    psFree (trend_e1);
    psFree (trend_e2);

    psFree (psfX11sm);
    psFree (psfX12sm);
    psFree (psfX22sm);
    psFree (psf_e1sm);
    psFree (psf_e2sm);

    psFree (psfX11sh);
    psFree (psfX12sh);
    psFree (psfX22sh);
    psFree (psf_e1sh);
    psFree (psf_e2sh);

    psFree (psf_e1);
    psFree (psf_e2);

    psFree (xPos    );
    psFree (yPos    );

    psFree (stats);

    psMetadataAddBool(readout->analysis, PS_LIST_TAIL, "LENS_PSF", PS_META_REPLACE, "psf-trend lensing stats measured", true);
    psLogMsg ("psphot.lensing", PS_LOG_DETAIL, "calculate lensing parameters for %d objects: %f sec\n", (int) sources->n, psTimerMark ("psphot.lensing"));

    return true;
}

