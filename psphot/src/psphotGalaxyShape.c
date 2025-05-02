# include "psphotInternal.h"

bool psphotGalaxyShape (pmConfig *config, const pmFPAview *view, const char *filerule)
{
    bool status = true;

    fprintf (stdout, "\n");
    psLogMsg ("psphot", PS_LOG_INFO, "--- psphot Galaxy Shapes ---");

    // select the appropriate recipe information
    psMetadata *recipe  = psMetadataLookupPtr (&status, config->recipes, PSPHOT_RECIPE);
    psAssert (recipe, "missing recipe?");

    // perform full non-linear fits / extended source analysis?
    // XXX for this to be true for psphotFullForce??
    if (!psMetadataLookupBool (&status, recipe, "GALAXY_SHAPES")) {
	psLogMsg ("psphot", PS_LOG_INFO, "skipping galaxy shape measurements\n");
	return true;
    }

    int num = psphotFileruleCount(config, filerule);

    // skip the chisq image (optionally?)
    int chisqNum = psMetadataLookupS32 (&status, config->arguments, "PSPHOT.CHISQ.NUM");
    if (!status) chisqNum = -1;

    // loop over the available readouts
    for (int i = 0; i < num; i++) {
        if (i == chisqNum) continue; // skip chisq image

        // find the currently selected readout
        pmFPAfile *file = pmFPAfileSelectSingle(config->files, filerule, i); // File of interest
        psAssert (file, "missing file?");

        pmReadout *readout = pmFPAviewThisReadout(view, file->fpa);
        psAssert (readout, "missing readout?");

        pmDetections *detections = psMetadataLookupPtr (&status, readout->analysis, "PSPHOT.DETECTIONS");
        psAssert (detections, "missing detections?");

        psArray *sources = detections->newSources ? detections->newSources : detections->allSources;
        psAssert (sources, "missing sources?");

        pmPSF *psf = psMetadataLookupPtr (&status, readout->analysis, "PSPHOT.PSF");
        // psAssert (psf, "missing psf?");

        if (!psphotGalaxyShapeReadout (config, recipe, view, filerule, readout, sources, psf)) {
            psError (PSPHOT_ERR_CONFIG, false, "failed to measure magnitudes for %s entry %d", filerule, i);
            return false;
        }
    }
    return true;
}

bool psphotGalaxyShapeReadout(pmConfig *config, psMetadata *recipe, const pmFPAview *view, const char * filerule, pmReadout *readout, psArray *sources, pmPSF *psf) {

    bool status = false;

    if (!sources->n) {
        psLogMsg ("psphot", PS_LOG_INFO, "no sources, skipping galaxy shapes");
        return true;
    }

    psTimerStart ("psphot.galaxy");

    psphotInitRadiusEXT (recipe, readout);

    // determine the number of allowed threads
    int nThreads = psMetadataLookupS32(&status, config->arguments, "NTHREADS"); // Number of threads
    if (!status) {
        nThreads = 0;
    }

    // bit-masks to test for good/bad pixels
    psImageMaskType maskVal = psMetadataLookupImageMask(&status, recipe, "MASK.PSPHOT");
    assert (maskVal);

    // bit-mask to mark pixels not used in analysis
    psImageMaskType markVal = psMetadataLookupImageMask(&status, recipe, "MARK.PSPHOT");
    assert (markVal);

    maskVal |= markVal;

    psphotGalaxyShapeOptions *opt = psphotGalaxyShapeOptionsAlloc();
    opt->Q = psMetadataLookupF32(&status, recipe, "GALAXY_SHAPES_Q"); 
    psAssert (status, "missing GALAXY_SHAPES_Q");
    opt->NSigma = psMetadataLookupF32(&status, recipe, "GALAXY_SHAPES_NSIGMA"); 
    psAssert (status, "missing GALAXY_SHAPES_NSIGMA");
    opt->clampSN = psMetadataLookupF32(&status, recipe, "GALAXY_SHAPES_CLAMP_SN"); 
    psAssert (status, "missing GALAXY_SHAPES_NSIGMA");
    psString modelTypeToSave = psMetadataLookupStr(&status, recipe, "EXT_MODEL_TYPE_FORCE");
    if (modelTypeToSave && strcmp(modelTypeToSave, "ALL") && strcmp(modelTypeToSave, "BEST")) {
        opt->extModelType = pmModelClassGetType(modelTypeToSave);
    } else {
        opt->extModelType = pmModelClassGetType("PS_MODEL_SERSIC");
    }

#ifdef notdef
    opt->fRmajorMin = psMetadataLookupF32(&status, recipe, "GALAXY_SHAPES_FR_MAJOR_MIN"); if (!status) opt->fRmajorMin = 0.5;
    opt->fRmajorMax = psMetadataLookupF32(&status, recipe, "GALAXY_SHAPES_FR_MAJOR_MAX"); if (!status) opt->fRmajorMax = 2.0;
    opt->fRmajorDel = psMetadataLookupF32(&status, recipe, "GALAXY_SHAPES_FR_MAJOR_DEL"); if (!status) opt->fRmajorDel = 0.1;
    opt->fRminorMin = psMetadataLookupF32(&status, recipe, "GALAXY_SHAPES_FR_MINOR_MIN"); if (!status) opt->fRminorMin = 0.5;
    opt->fRminorMax = psMetadataLookupF32(&status, recipe, "GALAXY_SHAPES_FR_MINOR_MAX"); if (!status) opt->fRminorMax = 2.0;
    opt->fRminorDel = psMetadataLookupF32(&status, recipe, "GALAXY_SHAPES_FR_MINOR_DEL"); if (!status) opt->fRminorDel = 0.1;
#endif

    // what fraction of the PSF is used? (radius in pixels : 2 -> 5x5 box)
    // NOTE: this is only used if we are NOT smoothing with a 1D Gaussian
    int psfSize = psMetadataLookupS32 (&status, recipe, "PCM_BOX_SIZE");
    assert (status);

    float fitNsigmaConv = psMetadataLookupF32 (&status, recipe, "EXT_FIT_NSIGMA_CONV"); // number of sigma for the convolutio
    if (!status || !isfinite(fitNsigmaConv) || fitNsigmaConv <= 0) {
	fitNsigmaConv = 5.0;
    }

    // Define source fitting parameters for extended source fits
    // we are not doing LMM fitting, so most options are irrelevant
    pmSourceFitOptions *fitOptions = pmSourceFitOptionsAlloc();
    fitOptions->mode           = PM_SOURCE_FIT_EXT_AND_SKY;
    fitOptions->covarFactor    = psImageCovarianceFactorForAperture(readout->covariance, 10.0); // Covariance matrix
    fitOptions->nsigma         = fitNsigmaConv;

    // Poisson or Constant weight for chisq tests?
    fitOptions->poissonErrors = psMetadataLookupBool (&status, recipe, "EXTENDED_SOURCE_FITS_POISSON");
    if (!status) fitOptions->poissonErrors = true;

    // source analysis is done in S/N order (brightest first)
    sources = psArraySort (sources, pmSourceSortByFlux);
    if (!sources->n) {
        psLogMsg ("psphot", PS_LOG_INFO, "no sources, skipping blend");
        return true;
    }

    // threaded measurement of the source magnitudes
    // choose Cx, Cy (see psphotThreadTools.c for overview of the concepts)
    int Cx = 1, Cy = 1;
    psphotChooseCellSizes (&Cx, &Cy, readout, nThreads);

    psArray *cellGroups = psphotAssignSources (Cx, Cy, sources);

    for (int i = 0; i < cellGroups->n; i++) {

        psArray *cells = cellGroups->data[i];

        for (int j = 0; j < cells->n; j++) {

            // allocate a job -- if threads are not defined, this just runs the job
            psThreadJob *job = psThreadJobAlloc ("PSPHOT_GALAXY_SHAPES");

            psArrayAdd(job->args, 1, readout);
            psArrayAdd(job->args, 1, cells->data[j]); // sources
            psArrayAdd(job->args, 1, fitOptions);
            psArrayAdd(job->args, 1, opt);
            PS_ARRAY_ADD_SCALAR(job->args, markVal, PS_TYPE_IMAGE_MASK);
            PS_ARRAY_ADD_SCALAR(job->args, maskVal, PS_TYPE_IMAGE_MASK);
            PS_ARRAY_ADD_SCALAR(job->args, psfSize, PS_TYPE_S32);

// set this to 0 to run without threading
# if (1)
            if (!psThreadJobAddPending(job)) {
                psError(PS_ERR_UNKNOWN, false, "Unable to guess model.");
                return false;
            }
# else
	    if (!psphotGalaxyShape_Threaded(job)) {
		psError(PS_ERR_UNKNOWN, false, "Unable to guess model.");
		// psFree(AnalysisRegion);
		return false;
	    }
	    psFree(job);
# endif
        }

        // wait for the threads to finish and manage results
        if (!psThreadPoolWait (false, true)) {
            psError(PS_ERR_UNKNOWN, false, "Unable to guess model.");
            return false;
        }

        // we have only supplied one type of job, so we can assume the types here
        psThreadJob *job = NULL;
        while ((job = psThreadJobGetDone()) != NULL) {
            if (job->args->n < 1) {
                fprintf (stderr, "error with job\n");
            }
            psFree(job);
        }
    }
    psFree (cellGroups);
    psFree (fitOptions);
    psFree (opt);

    psLogMsg ("psphot.galaxy", PS_LOG_WARN, "measure galaxy shapes : %f sec for %ld objects\n", psTimerMark ("psphot.galaxy"), sources->n);
    return true;
}

bool psphotGalaxyShape_Threaded (psThreadJob *job) {

    pmReadout *readout      	   = job->args->data[0];
    psArray *sources        	   = job->args->data[1];
    pmSourceFitOptions *fitOptions = job->args->data[2];
    psphotGalaxyShapeOptions *opt  = job->args->data[3];
    psImageMaskType markVal 	   = PS_SCALAR_VALUE(job->args->data[4],PS_TYPE_IMAGE_MASK_DATA);
    psImageMaskType maskVal 	   = PS_SCALAR_VALUE(job->args->data[5],PS_TYPE_IMAGE_MASK_DATA);
    int psfSize             	   = PS_SCALAR_VALUE(job->args->data[6],S32);

    float fitRadius;
    float windowRadius;

    for (int i = 0; i < sources->n; i++) {

	pmSource *source = sources->data[i];
	if (!source->peak) continue; // XXX how can we have a peak-less source?

	// check status of this source's moments
	if (!source->moments) continue;
	if (!(source->tmpFlags & PM_SOURCE_TMPF_MOMENTS_MEASURED)) continue;
	if (source->mode & PM_SOURCE_MODE_MOMENTS_FAILURE) continue;
        if (!source->modelPSF) continue;

	// psphotSetRadiusMomentsExact sets the radius based on Mrf
	if (!isfinite(source->moments->Mrf)) continue;

        // modelFits is allocated if a galaxy fit is requested
        if (!source->modelFits) continue;

	psphotSetRadiusMomentsExact(&fitRadius, &windowRadius, readout, source, markVal); // NOTE : 6 allocs

	// skip saturated stars modeled with a radial profile 
	// XXX worry about this at some point..
	if (source->mode2 & PM_SOURCE_MODE2_SATSTAR_PROFILE) continue;

	// replace object in image
	if (source->tmpFlags & PM_SOURCE_TMPF_SUBTRACTED) {
	    pmSourceAdd (source, PM_MODEL_OP_FULL, maskVal);
	}

	// this function populates moments->Mrf,GalaxyShape,GalaxyShapeErr
	// do the following for a set of shapes (Ex,Ey)
	psphotGalaxyShapeGrid (source, fitOptions, opt, maskVal, psfSize);

	psImageMaskPixels (source->maskObj, "AND", PS_NOT_IMAGE_MASK(markVal));

	pmSourceSub (source, PM_MODEL_OP_FULL, maskVal);
    }

    return true;
}

bool psphotGalaxyShapeGrid (pmSource *source, pmSourceFitOptions *fitOptions, psphotGalaxyShapeOptions *opt, psImageMaskType maskVal, int psfSize) {


    for (int iModel = 0 ; iModel < source->modelFits->n; iModel++) {
        pmModel *model = source->modelFits->data[iModel];
        if (!model) return false;

        pmModelType modelType = model->type;
        model->flags = PM_MODEL_STATUS_NONE;

        // we are using fitOptions->mode : be sure this makes sense
        pmPCMdata *pcm = pmPCMinit (source, fitOptions, model, maskVal, psfSize);
        if (!pcm) return false;

        // we are fitting only PM_PAR_I0; the shape elements are generated from a grid
        psF32 *PAR = pcm->modelConv->params->data.F32;

        pmSourceGalaxyFits *galaxyFits = pmSourceGalaxyFitsAlloc();

        // set the options for this source
        if (!psphotGalaxyShapeOptionsSet(source, galaxyFits, opt)) {
            // this source won't work
            psFree(pcm);
            psFree(galaxyFits);
            continue;
        }

        if (!source->galaxyFits) {
            source->galaxyFits = psArrayAllocEmpty(1);
        }
        psArrayAdd(source->galaxyFits, 1, galaxyFits);
        psFree(galaxyFits);

        // I have some source guess (e0, e1, e2)
        psEllipseAxes guessAxes = pmPSF_ModelToAxes (PAR, model->class->useReff);

        float fRmajorBest = NAN;
        float fRminorBest = NAN;
        float chisqBest = NAN;
        for (float fRmajor = galaxyFits->fRmajorMin; fRmajor < galaxyFits->fRmajorMax + 0.5*galaxyFits->fRmajorDel; fRmajor += galaxyFits->fRmajorDel) {
            for (float fRminor = galaxyFits->fRminorMin; fRminor < galaxyFits->fRminorMax + 0.5*galaxyFits->fRminorDel; fRminor += galaxyFits->fRminorDel) {
      
                psEllipseAxes testAxes = guessAxes;
                testAxes.major = guessAxes.major * fRmajor;
                testAxes.minor = guessAxes.minor * fRminor;
                
                pmPSF_AxesToModel (PAR, testAxes, model->class->useReff);
                
                psphotGalaxyShapeSource (pcm, source, galaxyFits, maskVal, psfSize, true);

                int i = galaxyFits->chisq->n - 1;
                float flux = galaxyFits->Flux->data.F32[i];
                if (isfinite(flux)) {
                    float thisChisq = galaxyFits->chisq->data.F32[i];
                    if (isfinite(thisChisq) && isfinite(flux) && (!isfinite(chisqBest) || thisChisq < chisqBest)) {
                        chisqBest = thisChisq;
                        fRmajorBest = fRmajor;
                        fRminorBest = fRminor;
                    }
                }
                // reset I0 to avoid potential problems on the next iteration
                PAR[PM_PAR_I0] = 1.0;
            }
        }

        if (isfinite(chisqBest)) {
            // now save the best fitting model as the source's extended model ...
            psEllipseAxes testAxes = guessAxes;

            // ... unless this macro is defined
#ifndef SAVE_NOMINAL_MODEL
            testAxes.major = guessAxes.major * fRmajorBest;
            testAxes.minor = guessAxes.minor * fRminorBest;
#endif
            
            pmPSF_AxesToModel (PAR, testAxes, model->class->useReff);
                
            psphotGalaxyShapeSource (pcm, source, galaxyFits, maskVal, psfSize, false);

            // Replace modelEXT with the best model from the first of the model fits if one of them is good
            if (isfinite(PAR[PM_PAR_I0]) && modelType == opt->extModelType) {
                psFree (source->modelEXT);

                source->modelEXT = psMemIncrRefCounter (pcm->modelConv);
                source->type = PM_SOURCE_TYPE_EXTENDED;
                source->mode |= PM_SOURCE_MODE_EXTMODEL;
                source->mode |= PM_SOURCE_MODE_NONLINEAR_FIT;

                // cache the model flux
                pmPCMCacheModel (source, maskVal, psfSize, fitOptions->nsigma);
            }
        }

        psFree (pcm);
    }
    return true;
}

// fit the given model to the source and find chisq & normalization
// XXX is this a single-component model? sersic with a supplied index, Reff, axis ratio, and theta?
bool psphotGalaxyShapeSource (pmPCMdata *pcm, pmSource *source, pmSourceGalaxyFits *galaxyFits, psImageMaskType maskVal, int psfSize, bool saveResults) {

    PS_ASSERT_PTR_NON_NULL(source, false);
    PS_ASSERT_PTR_NON_NULL(source->peak, false);
    PS_ASSERT_PTR_NON_NULL(source->pixels, false);

    // generate the modelFlux
    // need to reset here each time since we assign below to calculate the flux
    // XXX note that this does not add sky to model
    pmPCMMakeModel (source, pcm->modelConv, pcm->nsigma, maskVal, psfSize);
    pcm->modelConv->params->data.F32[PM_PAR_I0] = 1.0;
	
    int nPix = 0;
    float YY = 0.0;
    float YM = 0.0;
    float MM = 0.0;
    bool usePoisson = pcm->poissonErrors;

    for (int iy = 0; iy < source->pixels->numRows; iy++) {
	for (int ix = 0; ix < source->pixels->numCols; ix++) {
	    // skip masked points
	    if (source->maskObj->data.PS_TYPE_IMAGE_MASK_DATA[iy][ix]) {
		continue;
	    }
	    // skip zero-variance points
	    if (source->variance->data.F32[iy][ix] == 0) {
		continue;
	    }

	    // skip nan value points
	    if (!isfinite(source->pixels->data.F32[iy][ix])) {
		continue;
	    }

	    float fy = source->pixels->data.F32[iy][ix];
	    float fm = source->modelFlux->data.F32[iy][ix];
	    float wt = (usePoisson) ? 1.0 / source->variance->data.F32[iy][ix] : 1.0;

	    YY += PS_SQR(fy) * wt;
	    YM += fm * fy * wt;
	    MM += PS_SQR(fm) * wt;
	    nPix ++;
	}
    }

    float Io = YM / MM;
    float dIo = sqrt (1.0 / MM);
    float Chisq = (YY - 2 * Io * YM + Io * Io * MM) / (float) nPix;
    // NOTE : if !poissonErrors, Chisq is not really the chisq, but is scaled by the flux.

    pcm->modelConv->params->data.F32[PM_PAR_I0] = Io;
    float flux = pcm->modelConv->class->modelFlux (pcm->modelConv->params);
    float dflux = flux * (dIo / Io);


    if (saveResults) {
        psVectorAppend (galaxyFits->Flux, flux);
        psVectorAppend (galaxyFits->dFlux, dflux);
        psVectorAppend (galaxyFits->chisq, Chisq);
        galaxyFits->nPix = nPix;
        galaxyFits->modelType = pcm->modelConv->type;
    }

    return true;
}

/**** support functions ****/

void psphotGalaxyShapeOptionsFree (psphotGalaxyShapeOptions *opt) {
    return;
}

psphotGalaxyShapeOptions *psphotGalaxyShapeOptionsAlloc()
{
    psphotGalaxyShapeOptions *opt = (psphotGalaxyShapeOptions *) psAlloc(sizeof(psphotGalaxyShapeOptions));
    psMemSetDeallocator(opt, (psFreeFunc) psphotGalaxyShapeOptionsFree);
    
    return opt;
}

bool psphotGalaxyShapeOptionsSet(pmSource *source, pmSourceGalaxyFits *galaxyFits, psphotGalaxyShapeOptions *options)
{
    // XXX: put these in recipe
    // doesn't make sense to use too small of a spacing
    float clampedSN = source->extSN < options->clampSN ? source->extSN : options->clampSN;

    float f_del = options->Q / clampedSN;
    float f_min = 1 - options->NSigma * f_del;
    float f_max = 1 + options->NSigma * f_del;

    // if f_min goes negative skip this object
    if (f_min < 0) return false;

    galaxyFits->fRmajorMin = f_min;
    galaxyFits->fRmajorMax = f_max;
    galaxyFits->fRmajorDel = f_del;
    galaxyFits->fRminorMin = f_min;
    galaxyFits->fRminorMax = f_max;
    galaxyFits->fRminorDel = f_del;

    return true;
}

