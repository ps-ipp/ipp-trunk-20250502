# include "psphotInternal.h"
// XXX need array of covar factors for each image
// XXX define the 'good' / 'bad' flags?

# define COVAR_FACTOR 1.0

bool psphotFitSourcesLinearStack (pmConfig *config, psArray *objects, bool final) {

    bool status;
    float f;

    // select the appropriate recipe information
    psMetadata *recipe  = psMetadataLookupPtr (&status, config->recipes, PSPHOT_RECIPE);
    assert (recipe);

    psTimerStart ("psphot.linear");

    // bit-masks to test for good/bad pixels
    psImageMaskType maskVal = psMetadataLookupImageMask(&status, recipe, "MASK.PSPHOT");
    assert (maskVal);

    // bit-mask to mark pixels not used in analysis
    psImageMaskType markVal = psMetadataLookupImageMask(&status, recipe, "MARK.PSPHOT");
    assert (markVal);

    // maskVal is used to test for rejected pixels, and must include markVal
    maskVal |= markVal;

    // analysis is done in spatial order (to speed up overlap search)
    objects = psArraySort (objects, pmPhotObjSortByX);

    // storage array for fitSources
    psArray *fitSources = psArrayAllocEmpty (objects->n);

    bool CONSTANT_PHOTOMETRIC_WEIGHTS = psMetadataLookupBool(&status, recipe, "CONSTANT_PHOTOMETRIC_WEIGHTS");
    psAssert (status, "You must provide a value for the BOOL recipe CONSTANT_PHOTOMETRIC_WEIGHTS");

    float MIN_VALID_FLUX = psMetadataLookupF32(&status, recipe, "PSF_FIT_MIN_VALID_FLUX");
    if (!status) {
        MIN_VALID_FLUX = 0.0;
    }
    float MAX_VALID_FLUX = psMetadataLookupF32(&status, recipe, "PSF_FIT_MAX_VALID_FLUX");
    if (!status) {
        MAX_VALID_FLUX = 1e+8;
    }

    // XXX store a local static array of covar factors for each of the images (by image ID)
    // float covarFactor = psImageCovarianceFactorForAperture(readout->covariance, 10.0); // Covariance matrix
    // psLogMsg ("psphot.ensemble", PS_LOG_MINUTIA, "covariance factor: %f\n", covarFactor);

    // select the sources which will be used for the fitting analysis
    for (int i = 0; i < objects->n; i++) {
        pmPhotObj *object = objects->data[i];
        if (!object) continue;
        if (!object->sources) continue;

        // XXX check an element of the group to see if we should use it
        // if (!object->flags & PM_PHOT_OBJ_BAD) continue;

        for (int j = 0; j < object->sources->n; j++) {
          pmSource *source = object->sources->data[j];
          if (!source) continue;

          // turn this bit off and turn it on again if we keep this source
          source->mode &= ~PM_SOURCE_MODE_LINEAR_FIT;

	  // skip non-astronomical objects (very likely defects)
	  if (source->type == PM_SOURCE_TYPE_DEFECT) continue;
	  if (source->type == PM_SOURCE_TYPE_SATURATED) continue;

	  // do not include CRs in the full ensemble fit
	  if (source->mode & PM_SOURCE_MODE_CR_LIMIT) continue;

	  // do not include MOMENTS_FAILURES in the fit
	  if (source->mode & PM_SOURCE_MODE_MOMENTS_FAILURE) continue;

	  if (final) {
	      if (source->tmpFlags & PM_SOURCE_TMPF_SUBTRACTED) continue;
	  } else {
	      // if (source->mode & PM_SOURCE_MODE_BLEND) continue;
	  }

          // generate model for sources without, or skip if we can't
          if (!source->modelFlux) {
            if (!pmSourceCacheModel (source, maskVal)) continue;
          }

	  // check the integral of the model : is it large enough?
	  float modelSum = 0.0;
	  for (int iy = 0; iy < source->modelFlux->numRows; iy++) {
	      for (int ix = 0; ix < source->modelFlux->numCols; ix++) {
		  modelSum += source->modelFlux->data.F32[iy][ix];
	      }
	  }
	  if (modelSum < 0.5) continue; // skip sources with no model constraint (somewhat arbitrary limit)
	  if (modelSum < 0.8) {
	      fprintf (stderr, "low-sig model @ %f, %f (%f sum, %f peak)\n",
		       source->peak->xf, source->peak->yf, modelSum, source->peak->rawFlux);
	  }

	  bool isPSF = false;
	  pmModel *model = pmSourceGetModel (&isPSF, source);

	  // clear the 'mark' pixels and remask on the fit aperture 
	  psImageMaskPixels (source->maskObj, "AND", PS_NOT_IMAGE_MASK(markVal));
	  psImageKeepCircle (source->maskObj, source->peak->x, source->peak->y, model->fitRadius, "OR", markVal);

	  // we call this function multiple times. for the first time, we have only PSF models for all objects
	  // the second time has extended sources.  If we ever fit the PSF model, we should raise this bit
	  source->mode |= PM_SOURCE_MODE_LINEAR_FIT;
	  if (isPSF) {
	      source->mode |= PM_SOURCE_MODE_PSFMODEL;
	  }	    
          psArrayAdd (fitSources, 100, source);
        }
    }
    psLogMsg ("psphot.ensemble", PS_LOG_MINUTIA, "built fitSources: %f sec (%ld objects)\n", psTimerMark ("psphot.linear"), objects->n);

    if (fitSources->n == 0) {
        return true;
    }

    // vectors to store stats for each object
    // psVector *variance = psVectorAlloc (fitSources->n, PS_TYPE_F32);
    psVector *errors = psVectorAlloc (fitSources->n, PS_TYPE_F32);

    // create the sparse matrix
    psSparse *sparse = psSparseAlloc (fitSources->n, 100);

    // fill out the sparse matrix elements and border elements (B)
    // SRCi is the current source of interest
    // SRCj is a possibly overlapping source
    for (int i = 0; i < fitSources->n; i++) {
        pmSource *SRCi = fitSources->data[i];

        // diagonal elements of the sparse matrix (auto-cross-product)
        f = pmSourceModelDotModel (SRCi, SRCi, CONSTANT_PHOTOMETRIC_WEIGHTS, COVAR_FACTOR, maskVal);
        psSparseMatrixElement (sparse, i, i, f);

        // the formal error depends on the weighting scheme
        if (CONSTANT_PHOTOMETRIC_WEIGHTS) {
            float var = pmSourceModelDotModel (SRCi, SRCi, false, COVAR_FACTOR, maskVal);
            errors->data.F32[i] = 1.0 / sqrt(var);
        } else {
            errors->data.F32[i] = 1.0 / sqrt(f);
        }

        // find the image x model value
        f = pmSourceDataDotModel (SRCi, SRCi, CONSTANT_PHOTOMETRIC_WEIGHTS, COVAR_FACTOR, maskVal);
        psSparseVectorElement (sparse, i, f);

        // loop over all other stars following this one
        for (int j = i + 1; j < fitSources->n; j++) {
            pmSource *SRCj = fitSources->data[j];

            // we only need to generate dot terms for source on the same image
            if (SRCj->imageID != SRCi->imageID) { continue; }

            // skip over disjoint source images, break after last possible overlap
            if (SRCj->pixels->row0 + SRCj->pixels->numRows < SRCi->pixels->row0) continue;  // source(i) is above source(j)
            if (SRCi->pixels->row0 + SRCi->pixels->numRows < SRCj->pixels->row0) continue;  // source(i) is below source(j)
            if (SRCj->pixels->col0 + SRCj->pixels->numCols < SRCi->pixels->col0) continue;  // source(i) is right of source(j)
            if (SRCi->pixels->col0 + SRCi->pixels->numCols < SRCj->pixels->col0) break;     // source(i) is left of source(j) [no other source(j) can overlap source(i)]

            // got an overlap; calculate cross-product and add to output array
            f = pmSourceModelDotModel (SRCi, SRCj, CONSTANT_PHOTOMETRIC_WEIGHTS, COVAR_FACTOR, maskVal);
            psSparseMatrixElement (sparse, j, i, f);
        }
    }

    psSparseResort (sparse);
    psLogMsg ("psphot.ensemble", PS_LOG_MINUTIA, "built matrix: %f sec (%d elements)\n", psTimerMark ("psphot.linear"), sparse->Nelem);

    psSparseConstraint constraint;
    constraint.paramMin   = MIN_VALID_FLUX;
    constraint.paramMax   = MAX_VALID_FLUX;
    constraint.paramDelta = 1e7;

    // solve for normalization terms (need include local sky?)
    psVector *norm = NULL;
    norm = psSparseSolve (NULL, constraint, sparse, 5);
    psLogMsg ("psphot.ensemble", PS_LOG_MINUTIA, "solve matrix: %f sec (%d elements)\n", psTimerMark ("psphot.linear"), sparse->Nelem);

    // adjust I0 for fitSources and subtract
    for (int i = 0; i < fitSources->n; i++) {
        pmSource *source = fitSources->data[i];
        pmModel *model = pmSourceGetModel (NULL, source);

        // assign linearly-fitted normalization
        if (isnan(norm->data.F32[i])) {
            psAbort("linear fitted source is nan");
        }

        model->params->data.F32[PM_PAR_I0] = norm->data.F32[i];
        model->dparams->data.F32[PM_PAR_I0] = errors->data.F32[i];

	// clear the 'mark' pixels so the subtraction covers the full window
        psImageMaskPixels (source->maskObj, "AND", PS_NOT_IMAGE_MASK(markVal));

        // subtract object
        pmSourceSub (source, PM_MODEL_OP_FULL, maskVal);
    }
    psLogMsg ("psphot.ensemble", PS_LOG_MINUTIA, "sub models: %f sec (%d elements)\n", psTimerMark ("psphot.linear"), sparse->Nelem);

    // measure chisq for each source
    // for (int i = 0; final && (i < fitSources->n); i++) {
    for (int i = 0; i < fitSources->n; i++) {
        pmSource *source = fitSources->data[i];
        pmModel *model = pmSourceGetModel (NULL, source);
        if (!(source->mode & PM_SOURCE_MODE_NONLINEAR_FIT)) {
	    model->nPar = 1; // LINEAR-only sources have 1 parameter; NONLINEAR sources have their original value
	}
        pmSourceChisq (model, source->pixels, source->maskObj, source->variance, maskVal);
    }
    psLogMsg ("psphot.ensemble", PS_LOG_MINUTIA, "get chisqs: %f sec (%d elements)\n", psTimerMark ("psphot.linear"), sparse->Nelem);

    // psFree (index);
    psFree (sparse);
    psFree (fitSources);
    psFree (norm);
    psFree (errors);

    psLogMsg ("psphot.ensemble", PS_LOG_WARN, "measure ensemble of PSFs: %f sec\n", psTimerMark ("psphot.linear"));
    return true;
}
