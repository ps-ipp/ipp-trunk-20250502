# include "psphotInternal.h"

// given a pmSource which has been fitted using modelPSF, evaluate the resulting fit: did
// the fit succeed? is this object saturated? return status is TRUE for a successful PSF
// fit, FALSE otherwise.  

// NOTE : 2008.10.12 EAM : This function used to make a measurement of the consistency of
// the source shape with the PSF.  This feature has been superceded with the
// much-better-defined psphotSourceSize.c function

static float SATURATION;
static float PSF_MIN_SN;
static float PSF_MAX_CHI;

bool psphotInitLimitsPSF (psMetadata *recipe, pmReadout *readout) {

    bool status;

    // XXX do we need to set this differently from the value used to mark saturated pixels?
    pmCell *cell     = readout->parent;

    // do not completely trust the values in the header...
    float CELL_SATURATION = psMetadataLookupF32 (&status, cell->concepts, "CELL.SATURATION");
    float MIN_SATURATION = psMetadataLookupF32 (&status, recipe, "DEBLEND_MIN_SATURATION");
    if (!status || !isfinite(MIN_SATURATION)) {
	MIN_SATURATION = 40000.0;
    }
    if (!isfinite(CELL_SATURATION)) {
	SATURATION = MIN_SATURATION;
    } else {
	SATURATION = PS_MAX(MIN_SATURATION, CELL_SATURATION);
    }

    PSF_MIN_SN       = psMetadataLookupF32 (&status, recipe, "PSF_MIN_SN");
    PSF_MAX_CHI      = psMetadataLookupF32 (&status, recipe, "PSF_MAX_CHI");

    return true;
}

// examine the model->status, fit parameters, etc and decide if the model succeeded
// set the source->type and source->mode appropriately
bool psphotEvalPSF (pmSource *source, pmModel *model) { 

    int keep;
    float SN, Chi;

    // do we actually have a valid PSF model?
    if (model == NULL) {
	source->mode &= ~PM_SOURCE_MODE_FITTED;
	return false;
    }

    // was the model actually fitted?
    if (!(model->flags & PM_MODEL_STATUS_FITTED)) {
	source->mode &= ~PM_SOURCE_MODE_FITTED; 
	if (source->mode & PM_SOURCE_MODE_SATSTAR) {
	    psTrace ("psphot", 5, "satstar was not fitted");
	}
	return false;
    }
    // did the model fit fail for one or another reason?
    if (model->flags & (PM_MODEL_STATUS_BADARGS | 
			PM_MODEL_STATUS_NONCONVERGE | 
			PM_MODEL_STATUS_OFFIMAGE)) {
	source->mode |= PM_SOURCE_MODE_FAIL;
	if (source->mode & PM_SOURCE_MODE_SATSTAR) {
	    psTrace ("psphot", 5, "satstar failed fit");
	}
	return false;
    }

    // unless we prove otherwise, this object is a star.
    source->type = PM_SOURCE_TYPE_STAR;

    // the following source->mode information pertains to modelPSF:
    source->mode |= PM_SOURCE_MODE_PSFMODEL;

    // if the object has fitted peak above saturation, label as SATSTAR
    // this is a valid PSF object, but ignore the other quality tests
    // remember: fit does not use saturated pixels (masked)
    // XXX no extended object can saturate and stay extended...

    // XXX this test is wrong : it should add back in the local sky from the model to test
    // against saturation the local sky is saved in source->sky after psphotMagnitudes is called
    if (model->params->data.F32[PM_PAR_I0] >= SATURATION) {
	if (source->mode & PM_SOURCE_MODE_PSFSTAR) {
	    psLogMsg ("psphot", 5, "PSFSTAR marked SATSTAR\n");
	}
	source->mode |=  PM_SOURCE_MODE_SATSTAR;
	return true;
    } 

    // if the object has a fitted peak below 0.02, the source is not viable
    // perhaps the fit did not converge cleanly
    if (model->params->data.F32[PM_PAR_I0] <= 0.02) {
	source->mode |= PM_SOURCE_MODE_FAIL;
	if (source->mode & PM_SOURCE_MODE_SATSTAR) {
	    psTrace ("psphot", 5, "satstar failed fit (peak below 0)");
	}
	return false;
    } 

    // if the source was predicted to be a SATSTAR, but it fitted below saturation, 
    // make a note to the user.
    // XXX do not do this until we have a local measure of the sky in place
    if (false && (source->mode & PM_SOURCE_MODE_SATSTAR)) {
	psLogMsg ("psphot", 5, "SATSTAR marked normal (fitted peak below saturation)\n");
	source->mode &= ~PM_SOURCE_MODE_SATSTAR;
    }

    SN  = model->params->data.F32[PM_PAR_I0]/model->dparams->data.F32[PM_PAR_I0];
    Chi = model->chisqNorm / model->nDOF;

    // assign PM_SOURCE_MODE_GOODSTAR to bright objects within PSF region of dparams[]
    keep = TRUE;
    keep &= (SN > PSF_MIN_SN);
    keep &= (Chi < PSF_MAX_CHI);

    if (source->mode & PM_SOURCE_MODE_SATSTAR) {
	psTrace ("psphot", 5, "satstar fit results: %f, %f  %d :  SN: %f  Chisq: %f\n", 
		     model->params->data.F32[PM_PAR_XPOS], model->params->data.F32[PM_PAR_YPOS], keep, SN, Chi);
    }

    if (keep) {
	if (source->mode & PM_SOURCE_MODE_PSFSTAR) {
	    psTrace ("psphot", 7, "PSFSTAR kept (%f, %f  :  SN: %f Chisq: %f)\n", 
		     model->params->data.F32[PM_PAR_XPOS], model->params->data.F32[PM_PAR_YPOS], SN, Chi);
	}
	return true;
    }

    // this source is not a star, warn if it was a PSFSTAR
    if (source->mode & PM_SOURCE_MODE_PSFSTAR) {
	psTrace ("psphot", 5, "PSFSTAR demoted based on fit quality   (%f, %f  :  SN: %f  Chisq: %f)\n", 
		  model->params->data.F32[PM_PAR_XPOS], model->params->data.F32[PM_PAR_YPOS], SN, Chi);
    } else {
	psTrace ("psphot", 5, "fails PSF fit (%f, %f  :  SN: %f  Chisq: %f)\n", 
		  model->params->data.F32[PM_PAR_XPOS], model->params->data.F32[PM_PAR_YPOS], SN, Chi);
    }

    // poor-quality fit; only keep if nothing else works...
    source->mode |= PM_SOURCE_MODE_POOR;
    return false;
}	

// examine the model->status, fit parameters, etc and decide if the model succeeded
// set the source->type and source->mode appropriately
bool psphotEvalDBL (pmSource *source, pmModel *model) { 

    // do we actually have a valid PSF model?
    if (model == NULL) {
	source->mode &= ~PM_SOURCE_MODE_FITTED;
	return false;
    }

    // was the model actually fitted?
    if (!(model->flags & PM_MODEL_STATUS_FITTED)) {
	source->mode &= ~PM_SOURCE_MODE_FITTED; 
	return false;
    }
    // did the model fit fail for one or another reason?
    if (model->flags & (PM_MODEL_STATUS_BADARGS | 
			PM_MODEL_STATUS_NONCONVERGE | 
			PM_MODEL_STATUS_OFFIMAGE)) {
	source->mode |= PM_SOURCE_MODE_FAIL;
	return false;
    }

    // unless we prove otherwise, this object is a star.
    source->type = PM_SOURCE_TYPE_STAR;

    // the following source->mode information pertains to modelPSF:
    source->mode |= PM_SOURCE_MODE_PSFMODEL;

    // if the object has fitted peak above saturation, label as SATSTAR
    // this is a valid PSF object, but ignore the other quality tests
    // remember: fit does not use saturated pixels (masked)
    // XXX no extended object can saturate and stay extended...
    if (model->params->data.F32[PM_PAR_I0] >= SATURATION) {
	if (source->mode & PM_SOURCE_MODE_PSFSTAR) {
	    psLogMsg ("psphot", 5, "PSFSTAR marked SATSTAR\n");
	}
	source->mode |=  PM_SOURCE_MODE_SATSTAR;
	return true;
    } 

    // if the object has a fitted peak below 0, the fit did not converge cleanly
    if (model->params->data.F32[PM_PAR_I0] <= 0.02) {
	source->mode |= PM_SOURCE_MODE_FAIL;
	return false;
    } 

    // if the source was predicted to be a SATSTAR, but it fitted below saturation, 
    // make a note to the user
    if (source->mode & PM_SOURCE_MODE_SATSTAR) {
	psLogMsg ("psphot", 5, "SATSTAR marked normal (fitted peak below saturation)\n");
	source->mode &= ~PM_SOURCE_MODE_SATSTAR;
    }
    return true;
}	
