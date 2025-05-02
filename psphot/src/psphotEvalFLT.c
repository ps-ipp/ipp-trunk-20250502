# include "psphotInternal.h"

bool psphotEvalEXT (pmSource *source, pmModel *model)
{ 
    int keep;

    // do we actually have a valid EXT model?
    if (model == NULL) {
	source->mode &= ~PM_SOURCE_MODE_FITTED;
	psTrace ("psphot", 5, "no model fitted?\n");
	return false;
    }

    // was the model actually fitted?
    if (!(model->flags & PM_MODEL_STATUS_FITTED)) {
	source->mode &= ~PM_SOURCE_MODE_FITTED; 
	psTrace ("psphot", 5, "no model fitted?\n");
	return false;
    }

    // did the model fit fail for one or another reason?
    if (model->flags & (PM_MODEL_STATUS_BADARGS | 
			PM_MODEL_STATUS_NONCONVERGE | 
			PM_MODEL_STATUS_OFFIMAGE)) {
	source->mode |= PM_SOURCE_MODE_FAIL;
	psLogMsg ("psphot", 5, "EXT fail fit\n");
	psTrace ("psphot", 5, "EXT fail fit\n");

	if (model->flags & PM_MODEL_STATUS_OFFIMAGE) {
	  psTrace ("psphot", 5, "off image\n");
	}
	if (model->flags & PM_MODEL_STATUS_BADARGS) {
	  psTrace ("psphot", 5, "bad args\n");
	}
	if (model->flags & PM_MODEL_STATUS_NONCONVERGE) {
	  psTrace ("psphot", 5, "non converge\n");
	}
	return false;
    }

    // unless we prove otherwise, this object is extended
    source->type = PM_SOURCE_TYPE_EXTENDED;

    // the following source->mode information pertains to modelEXT:
    source->mode |= PM_SOURCE_MODE_EXTMODEL;

    // if the object has a fitted peak below 0, the fit did not converge cleanly
    // XXX this limit is fairly arbitrary, and must be > the value is the model limits
    if (model->params->data.F32[PM_PAR_I0] <= 0.02) {
	source->mode |= PM_SOURCE_MODE_FAIL;
	psTrace ("psphot", 5, "model central intensity ~ zero\n");
	return false;
    } 

    keep = model->class->modelFitStatus(model);
    if (keep) return true;

    // poor-quality fit; only keep if nothing else works...
    psLogMsg ("psphot", 5, "EXT poor fit\n");
    psTrace ("psphot", 5, "EXT poor fit\n");

    source->mode |= PM_SOURCE_MODE_POOR;
    return false;
}	
