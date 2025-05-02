# include "psphotInternal.h"
// # define DEBUG

# define SKIPSTAR(MSG) { psTrace ("psphot", 3, "invalid : %s", MSG); continue; }
// measure the aperture residual statistics and 2D variations

// for now, let's store the detections on the readout->analysis for each readout
bool psphotApResid (pmConfig *config, const pmFPAview *view, const char *filerule)
{
    bool status = true;

    fprintf (stdout, "\n");
    psLogMsg ("psphot", PS_LOG_INFO, "--- psphot Aperture Residuals ---");

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
        if (!psphotApResidReadout (config, view, filerule, i, recipe)) {
            psError (PSPHOT_ERR_CONFIG, false, "failed to measure aperture residual for %s entry %d", filerule, i);
            return false;
        }
    }
    return true;
}

bool psphotApResidReadout (pmConfig *config, const pmFPAview *view, const char *filerule, int index, psMetadata *recipe)
{
    int Nfail = 0;
    int Nskip = 0;
    int Npsf;
    bool status;
    pmModel *model;
    pmSource *source;

    psTimerStart ("psphot.apresid");

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
        psLogMsg ("psphot", PS_LOG_INFO, "no sources, skipping ap resid");
        return true;
    }

    pmPSF *psf = psMetadataLookupPtr (&status, readout->analysis, "PSPHOT.PSF");
    psAssert (psf, "missing psf?");

    // determine the number of allowed threads
    int nThreads = psMetadataLookupS32(&status, config->arguments, "NTHREADS"); // Number of threads
    if (!status) {
        nThreads = 0;
    }

    bool measureAptrend = psMetadataLookupBool (&status, recipe, "MEASURE.APTREND");
    if (!measureAptrend) {
        // save nan values since these were not calculated
        psMetadataAddF32 (readout->analysis, PS_LIST_TAIL, "APMIFIT",  PS_META_REPLACE, "aperture residual",   NAN);
        psMetadataAddF32 (readout->analysis, PS_LIST_TAIL, "DAPMIFIT", PS_META_REPLACE, "ap residual scatter", NAN);
        psMetadataAddS32 (readout->analysis, PS_LIST_TAIL, "NAPMIFIT", PS_META_REPLACE, "number of apresid stars", 0);
        psMetadataAddF32 (readout->analysis, PS_LIST_TAIL, "APLOSS",   PS_META_REPLACE, "aperture loss (mag)", NAN);
        return true;
    }

    // bit-masks to test for good/bad pixels
    psImageMaskType maskVal = psMetadataLookupImageMask(&status, recipe, "MASK.PSPHOT");
    assert (maskVal);

    // bit-mask to mark pixels not used in analysis
    psImageMaskType markVal = psMetadataLookupImageMask(&status, recipe, "MARK.PSPHOT");
    assert (markVal);

    // maskVal is used to test for rejected pixels, and must include markVal
    maskVal |= markVal;

    // clipping for extreme outliers
    // XXX this is not currently defined in the recipe
    float MAX_AP_OFFSET = psMetadataLookupF32 (&status, recipe, "MAX_AP_OFFSET");

    // options for how the photometry is calculated
    // XXX are these sensible?
    bool IGNORE_GROWTH = psMetadataLookupBool (&status, recipe, "IGNORE_GROWTH");
    bool INTERPOLATE_AP = psMetadataLookupBool (&status, recipe, "INTERPOLATE_AP");

    // XXX is this still needed?  the pmTrend2D stuff should be auto-adjusting...
    int APTREND_NSTAR_MIN = psMetadataLookupS32(&status, recipe, "APTREND.NSTAR.MIN");
    assert (status);

    // maximum order for aperture correction
    int APTREND_ORDER_MAX = psMetadataLookupS32(&status, recipe, "APTREND.ORDER.MAX");
    assert (status);

    if (APTREND_ORDER_MAX < 1) {
        psError(PSPHOT_ERR_CONFIG, true, "APTREND.ORDER.MAX must be 1 or more");
        return false;
    }

    pmSourcePhotometryMode photMode = 0;
    if (!IGNORE_GROWTH) photMode |= PM_SOURCE_PHOT_GROWTH;
    if (INTERPOLATE_AP) photMode |= PM_SOURCE_PHOT_INTERP;

    // set limits on the aperture magnitudes
    pmSourceMagnitudesInit (config, recipe);

    // threaded measurement of the source magnitudes
    // choose Cx, Cy (see psphotThreadTools.c for overview of the concepts)
    int Cx = 1, Cy = 1;
    psphotChooseCellSizes (&Cx, &Cy, readout, nThreads);

    psArray *cellGroups = psphotAssignSources (Cx, Cy, sources);

    for (int i = 0; i < cellGroups->n; i++) {

        psArray *cells = cellGroups->data[i];

        for (int j = 0; j < cells->n; j++) {

            // allocate a job -- if threads are not defined, this just runs the job
            psThreadJob *job = psThreadJobAlloc ("PSPHOT_APRESID_MAGS");

            psArrayAdd(job->args, 1, cells->data[j]); // sources
            psArrayAdd(job->args, 1, psf);
            PS_ARRAY_ADD_SCALAR(job->args, photMode, PS_TYPE_S32);
            PS_ARRAY_ADD_SCALAR(job->args, maskVal,  PS_TYPE_IMAGE_MASK);
            PS_ARRAY_ADD_SCALAR(job->args, markVal,  PS_TYPE_IMAGE_MASK);

            PS_ARRAY_ADD_SCALAR(job->args, 0,        PS_TYPE_S32); // this is used as a return value for Nskip
            PS_ARRAY_ADD_SCALAR(job->args, 0,        PS_TYPE_S32); // this is used as a return value for Nfail

            if (!psThreadJobAddPending(job)) {
                psError(PS_ERR_UNKNOWN, false, "Unable to guess model.");
                return false;
            }
        }

        // wait for the threads to finish and manage results
        if (!psThreadPoolWait (false, true)) {
            psError(PS_ERR_UNKNOWN, false, "Unable to guess model.");
            psFree(cellGroups);
            return false;
        }

        // we have only supplied one type of job, so we can assume the types here
        psThreadJob *job = NULL;
        while ((job = psThreadJobGetDone()) != NULL) {
            if (job->args->n < 1) {
                fprintf (stderr, "error with job\n");
            } else {
                psScalar *scalar = NULL;
                scalar = job->args->data[5];
                Nskip += scalar->data.S32;
                scalar = job->args->data[6];
                Nfail += scalar->data.S32;
            }
            psFree(job);
        }
    }

    psFree (cellGroups);

    // gather the stats to assess the aperture residuals
    psVector *mag     = psVectorAllocEmpty (300, PS_TYPE_F32);
    psVector *xPos    = psVectorAllocEmpty (300, PS_TYPE_F32);
    psVector *yPos    = psVectorAllocEmpty (300, PS_TYPE_F32);
    psVector *apResid = psVectorAllocEmpty (300, PS_TYPE_F32);
    psVector *dMag    = psVectorAllocEmpty (300, PS_TYPE_F32);
    Npsf = 0;

# ifdef DEBUG
    FILE *f = fopen ("apresid.dat", "w");
    psAssert (f, "failed open");
# endif

    for (int i = 0; i < sources->n; i++) {
        source = sources->data[i];
        model = source->modelPSF;

        if (source->type != PM_SOURCE_TYPE_STAR) SKIPSTAR ("NOT STAR");
        if (source->mode &  PM_SOURCE_MODE_SATSTAR) SKIPSTAR ("SATSTAR");
        if (source->mode &  PM_SOURCE_MODE_BLEND) SKIPSTAR ("BLEND");
        if (source->mode &  PM_SOURCE_MODE_FAIL) SKIPSTAR ("FAIL STAR");
        if (source->mode &  PM_SOURCE_MODE_POOR) SKIPSTAR ("POOR STAR");

	// skip saturated stars modeled with a radial profile 
        if (source->mode2 & PM_SOURCE_MODE2_SATSTAR_PROFILE) SKIPSTAR ("SATSTAR");

        if (source->mode &  PM_SOURCE_MODE_EXT_LIMIT) SKIPSTAR ("EXTENDED");
        if (source->mode &  PM_SOURCE_MODE_CR_LIMIT) SKIPSTAR ("COSMIC RAY");
        if (source->mode &  PM_SOURCE_MODE_DEFECT) SKIPSTAR ("DEFECT");
        if (source->mode2 & PM_SOURCE_MODE2_MATCHED) SKIPSTAR ("MATCHED");

        if (!isfinite(source->apMag) || !isfinite(source->psfMag)) {
            continue;
        }

        // XXX make this user-configurable?
        if (source->psfMagErr > 0.03) continue;

        // aperture residual for this source
        float dap = source->apMag - source->psfMag;

        // sanity clipping : if the model is very discrepant, put your expectation in the recipe
        if ((MAX_AP_OFFSET > 0) && (fabs(dap) > MAX_AP_OFFSET)) {
            Nfail ++;
            psTrace ("psphot", 3, "fail : bad dap %f %f", dap, MAX_AP_OFFSET);
            continue;
        }

# ifdef DEBUG
        fprintf (f, "%6.1f %6.1f : %6.1f %6.1f : %8.3f %8.3f %8.3f : %f : %f %f %f : %f\n",
                 source->peak->xf, source->peak->yf,
                 source->modelPSF->params->data.F32[PM_PAR_XPOS], source->modelPSF->params->data.F32[PM_PAR_YPOS],
                 source->psfMag, source->apMag, source->psfMagErr,
                 source->modelPSF->params->data.F32[PM_PAR_I0],
                 source->modelPSF->params->data.F32[PM_PAR_SXX], source->modelPSF->params->data.F32[PM_PAR_SXY], source->modelPSF->params->data.F32[PM_PAR_SYY],
                 source->modelPSF->params->data.F32[PM_PAR_7]);
# endif
        if (!isfinite(source->psfMag)) psAbort ("nan in psfMag");
        if (!isfinite(source->psfMagErr)) psAbort ("nan in psfMagErr");
        if (!isfinite(source->apMag)) psAbort ("nan in apMag");
        if (!isfinite(model->params->data.F32[PM_PAR_XPOS])) psAbort ("nan in xPos");
        if (!isfinite(model->params->data.F32[PM_PAR_YPOS])) psAbort ("nan in yPos");

        psVectorAppend (mag, source->psfMag);
        psVectorAppend (dMag,source->psfMagErr);
        psVectorAppend (apResid, dap);
        psVectorAppend (xPos, model->params->data.F32[PM_PAR_XPOS]);
        psVectorAppend (yPos, model->params->data.F32[PM_PAR_YPOS]);
        Npsf ++;
    }

    psLogMsg ("psphot.apresid", PS_LOG_DETAIL, "measure aperture residuals for %d objects (%d skipped, %d failed, %ld invalid)\n",
              Npsf, Nskip, Nfail, sources->n - Npsf - Nskip - Nfail);

# ifdef DEBUG
    fclose (f);
# endif

    // XXX choose a better value here?
    if (Npsf < APTREND_NSTAR_MIN) {
        psWarning("Only %d valid aperture residual sources (need %d), giving up", Npsf, APTREND_NSTAR_MIN);
        goto escape;
    }

    // this is a bit tricky, because we have two cases (MAP vs POLY), and they have a different
    // definition for 'order' (order_MAP = order_POLY + 1).  in addition, we have a
    // user-specified MAX order, which we should respect, regardless of the mode

    // set the max order (0 = constant) which the number of psf stars can support:
    // we require only 3 stars for n = 0, increase stars / cell for higher order
    int MaxOrderForStars = 0;
    if (Npsf >=  16) MaxOrderForStars = 1; // 4 cells
    if (Npsf >=  54) MaxOrderForStars = 2; // 9 cells
    if (Npsf >= 128) MaxOrderForStars = 3; // 16 cells
    if (Npsf >= 300) MaxOrderForStars = 4; // 25 cells
    if (Npsf >  576) MaxOrderForStars = 5; // 36 cells

    pmTrend2DMode mode = PM_TREND_MAP;
    if (mode == PM_TREND_MAP) {
        MaxOrderForStars ++;
    }
    APTREND_ORDER_MAX = PS_MIN (APTREND_ORDER_MAX, MaxOrderForStars);

    psFree (psf->ApTrend);
    psf->ApTrend = NULL;
    float errorFloorMin = FLT_MAX;

    // as we loop over orders, we need to refer to the initial selection, but we modify the
    // option values to match the current guess: save the max values here:
    int NX = readout->image->numCols;
    int NY = readout->image->numRows;
    for (int i = 1; i <= APTREND_ORDER_MAX; i++) {

        int Nx, Ny;
        if (NX > NY) {
            Nx = i;
            Ny = PS_MAX (1, (int)(i * (NY / (float)(NX)) + 0.5));
        } else {
            Ny = i;
            Nx = PS_MAX (1, (int)(i * (NX / (float)(NY)) + 0.5));
        }

        float errorFloor;
        pmTrend2D *apTrend = psphotApResidTrend (&errorFloor, readout, Nx, Ny, xPos, yPos, apResid, dMag);
        if (!apTrend) {
            continue;
        }

        // apply ApTrend results
        // float xc = 0.5*readout->image->numCols + readout->image->col0 + 0.5;
        // float yc = 0.5*readout->image->numRows + readout->image->row0 + 0.5;
        // float ApResid = pmTrend2DEval (psf->ApTrend, xc, yc); // ap-fit at chip center
        // if (!isfinite(ApResid)) psAbort("nan apresid @ center");

        // store the minimum errorFloor and best ApTrend to keep
        if (errorFloor < errorFloorMin) {
            errorFloorMin = errorFloor;
            psFree (psf->ApTrend);
            psf->ApTrend = psMemIncrRefCounter(apTrend);
        }
        psFree (apTrend);
    }
    if (psf->ApTrend == NULL) {
        psWarning("Failed to find a valid aperture residual value");
        goto escape;
    }

    // apply ApTrend results
    float xc = 0.5*readout->image->numCols + readout->image->col0 + 0.5;
    float yc = 0.5*readout->image->numRows + readout->image->row0 + 0.5;

    psf->ApResid  = pmTrend2DEval (psf->ApTrend, xc, yc); // ap-fit at chip center
    psf->dApResid = errorFloorMin;
    psf->nApResid = Npsf;

    // save results for later output
    psMetadataAddF32 (readout->analysis, PS_LIST_TAIL, "APMIFIT",  PS_META_REPLACE, "aperture residual",   psf->ApResid);
    psMetadataAddF32 (readout->analysis, PS_LIST_TAIL, "DAPMIFIT", PS_META_REPLACE, "ap residual scatter", psf->dApResid);
    psMetadataAddS32 (readout->analysis, PS_LIST_TAIL, "NAPMIFIT", PS_META_REPLACE, "number of apresid stars", psf->nApResid);

    // curve-of-growth offset
    if (psf->growth) {
	float gaussSigma = psMetadataLookupF32(&status, readout->analysis, "MOMENTS_GAUSS_SIGMA");
	if (!status) {
	    gaussSigma = psMetadataLookupF32(&status, recipe, "MOMENTS_GAUSS_SIGMA");
	}
	float apScale = psMetadataLookupF32(&status, recipe, "PSF_APERTURE_SCALE");
	float PSF_APERTURE = (int)(apScale*gaussSigma);
	
	float offset = pmGrowthCurveCorrect (psf->growth, PSF_APERTURE);
	psMetadataAddF32 (readout->analysis, PS_LIST_TAIL, "APLOSS",   PS_META_REPLACE, "aperture loss (mag)", psf->growth->apLoss);
	psMetadataAddF32 (readout->analysis, PS_LIST_TAIL, "APREFOFF", PS_META_REPLACE, "offset to ref aperture", offset);
    } else {
	psMetadataAddF32 (readout->analysis, PS_LIST_TAIL, "APLOSS",   PS_META_REPLACE, "aperture loss (mag)", NAN);
	psMetadataAddF32 (readout->analysis, PS_LIST_TAIL, "APREFOFF", PS_META_REPLACE, "offset to ref aperture", NAN);
    }

    psLogMsg ("psphot.apresid", PS_LOG_DETAIL, "aperture residual: %f +/- %f\n", psf->ApResid, psf->dApResid);
    psLogMsg ("psphot.apresid", PS_LOG_WARN, "measure full-frame aperture residuals for %d sources: %f sec\n", Npsf, psTimerMark ("psphot.apresid"));

    psFree (xPos);
    psFree (yPos);
    psFree (apResid);
    psFree (mag);
    psFree (dMag);

    psphotVisualPlotApResid (sources, psf->ApResid, psf->dApResid, true);

    return true;

escape:
    // save nan values since these were not calculated
    psMetadataAddF32 (readout->analysis, PS_LIST_TAIL, "APMIFIT",  PS_META_REPLACE, "aperture residual",   NAN);
    psMetadataAddF32 (readout->analysis, PS_LIST_TAIL, "DAPMIFIT", PS_META_REPLACE, "ap residual scatter", NAN);
    psMetadataAddS32 (readout->analysis, PS_LIST_TAIL, "NAPMIFIT", PS_META_REPLACE, "number of apresid stars", 0);
    psMetadataAddF32 (readout->analysis, PS_LIST_TAIL, "APLOSS",   PS_META_REPLACE, "aperture loss (mag)", NAN);

    psFree (xPos);
    psFree (yPos);
    psFree (apResid);
    psFree (mag);
    psFree (dMag);
    return true;
    // this is a quality error, not a programming error
}

pmTrend2D *psphotApResidTrend (float *apResidSysErr, pmReadout *readout, int Nx, int Ny, psVector *xPos, psVector *yPos, psVector *apResid, psVector *dMag) {

    // the mask marks the values not used to calculate the ApTrend
    psVector *mask = psVectorAlloc(xPos->n, PS_TYPE_VECTOR_MASK);
    psVectorInit (mask, 0);

    // XXX allow user to set this optionally?
    psStats *stats = psStatsAlloc (PS_STAT_SAMPLE_MEDIAN | PS_STAT_SAMPLE_STDEV);

    // measure Trend2D for the current spatial scale
    pmTrend2D *apTrend = pmTrend2DAlloc (PM_TREND_MAP, readout->image, Nx, Ny, stats);

    // XXX somewhat arbitrary: soften the errors so the few bright stars do not totally dominate:
    // XXX use this or not?  probably not, since this is the point of the systematic error analysis
    psVector *dMagSoft = psVectorAlloc (dMag->n, PS_TYPE_F32);
    for (int i = 0; i < dMag->n; i++) {
        dMagSoft->data.F32[i] = hypot(dMag->data.F32[i], 0.005);
    }

    // XXX test for errors here
    bool goodFit = false;
    if (!pmTrend2DFit (&goodFit, apTrend, mask, 0xff, xPos, yPos, apResid, dMagSoft)) {
	// XXX this is probably a real error, and I should exit
        psWarning("Failed to fit trend for %d x %d map", Nx, Ny);
        psFree (apTrend);
        return NULL;
    }
    if (!goodFit) {
        psWarning("Failed to fit trend for %d x %d map", Nx, Ny);
        psFree (apTrend);
        return NULL;
    }
    if (apTrend->mode == PM_TREND_MAP) {
        // p_psImagePrint (2, apTrend->map->map, "ApTrend Before"); // XXX TEST:
        psImageMapRepair (apTrend->map->map);
        // p_psImagePrint (2, apTrend->map->map, "ApTrend After"); // XXX TEST:
    }

    // construct the fitted values and the residuals
    psVector *apResidFit = pmTrend2DEvalVector (apTrend, mask, 0xff, xPos, yPos);
    psVector *apResidRes = (psVector *) psBinaryOp (NULL, (void *) apResid, "-", (void *) apResidFit);

    // measure systematic error
    *apResidSysErr = psVectorSystematicError (apResidRes, dMag, 0.10);
    if (!isfinite(*apResidSysErr)) {
        psWarning("Failed to find systematic error for %d x %d map", Nx, Ny);
        psFree (apTrend);
        return NULL;
    }

    psLogMsg ("psphot.apresid", PS_LOG_INFO, "result of %d x %d grid\n", Nx, Ny);
    psLogMsg ("psphot.apresid", PS_LOG_INFO, "systematic scatter floor: %f\n", *apResidSysErr);

    if (psTraceGetLevel("psphot") >= 4) {
        char filename[64];
        snprintf (filename, 64, "apresid.%dx%d.dat", Nx, Ny);
        FILE *dumpFile = fopen (filename, "w");
        for (int i = 0; i < xPos->n; i++) {
            fprintf (dumpFile, "%f %f  %f %f  %f %f  %x\n",
                     xPos->data.F32[i], yPos->data.F32[i],
                     dMag->data.F32[i], hypot(dMag->data.F32[i], *apResidSysErr),
                     apResid->data.F32[i], apResidRes->data.F32[i],
                     mask->data.PS_TYPE_VECTOR_MASK_DATA[i]);
        }
        fclose (dumpFile);
    }

    psFree (mask);
    psFree (stats);
    psFree (dMagSoft);
    psFree (apResidFit);
    psFree (apResidRes);

    return apTrend;
}

bool psphotApResidMags_Threaded (psThreadJob *job) {

    int Nskip = 0;
    int Nfail = 0;

    psScalar *scalar = NULL;

    psArray *sources                = job->args->data[0];
    pmPSF *psf                      = job->args->data[1];
    pmSourcePhotometryMode photMode = PS_SCALAR_VALUE(job->args->data[2],S32);
    psImageMaskType maskVal         = PS_SCALAR_VALUE(job->args->data[3],PS_TYPE_IMAGE_MASK_DATA);
    psImageMaskType markVal         = PS_SCALAR_VALUE(job->args->data[4],PS_TYPE_IMAGE_MASK_DATA);

    for (int i = 0; i < sources->n; i++) {
        pmSource *source = (pmSource *) sources->data[i];

        if (source->type != PM_SOURCE_TYPE_STAR) SKIPSTAR ("NOT STAR");
        if (source->mode &  PM_SOURCE_MODE_SATSTAR) SKIPSTAR ("SATSTAR");
        if (source->mode &  PM_SOURCE_MODE_BLEND) SKIPSTAR ("BLEND");
        if (source->mode &  PM_SOURCE_MODE_FAIL) SKIPSTAR ("FAIL STAR");
        if (source->mode &  PM_SOURCE_MODE_POOR) SKIPSTAR ("POOR STAR");

        // replace object in image
        if (source->tmpFlags & PM_SOURCE_TMPF_SUBTRACTED) {
            pmSourceAdd (source, PM_MODEL_OP_FULL, maskVal);
        }

        // clear the mask bit and set the circular mask pixels
        psImageMaskPixels (source->maskObj, "AND", PS_NOT_IMAGE_MASK(markVal));
        psImageKeepCircle (source->maskObj, source->peak->x, source->peak->y, source->apRadius, "OR", markVal);

        bool status = pmSourceMagnitudes (source, psf, photMode, maskVal, markVal, source->apRadius);

        // clear the mask bit
        psImageMaskPixels (source->maskObj, "AND", PS_NOT_IMAGE_MASK(markVal));

        // re-subtract the object, leave local sky
        pmSourceSub (source, PM_MODEL_OP_FULL, maskVal);

        if (!status) {
            Nskip ++;
            psTrace ("psphot", 3, "skip : bad source mag");
            continue;
        }

        if (!isfinite(source->apMag) || !isfinite(source->psfMag)) {
            Nfail ++;
            psTrace ("psphot", 3, "fail : %f, %f : nan mags : %f %f", source->peak->xf, source->peak->yf, source->apMag, source->psfMag);
            continue;
        }
        source->mode |= PM_SOURCE_MODE_AP_MAGS;
    }

    // change the value of a scalar on the array (wrap this and put it in psArray.h)
    scalar = job->args->data[5];
    scalar->data.S32 = Nskip;

    scalar = job->args->data[6];
    scalar->data.S32 = Nfail;

    return true;
}
