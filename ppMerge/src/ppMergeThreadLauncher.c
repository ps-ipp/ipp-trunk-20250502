/** @file ppMergeThreadLauncher.c
 *
 *  @brief
 *
 *  @ingroup ppMerge
 *
 *  @author IfA
 *  @version $Revision: 1.4 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-01 21:43:05 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

# include "ppMerge.h"

/**
 * each thread runs this function, starting a new job when it finished with an old one
 * it is called with a (void *) pointer to its own thread pointer
 */
void *ppMergeThreadLauncher (void *data) {

    psThread *self = data;
    psThreadJob *job = NULL;

    while (1) {

	// if we get an error, just wait until we are cleared or killed
	while (self->fault) {
	    usleep (10000);
	}

	// request a new job, if there are none available, sleep a bit
	// we have to lock here so the job queue cannot be empty yet no threads busy
	psThreadLock();
	while ((job = psThreadJobGetPending ()) == NULL) {
	    psThreadUnlock();
	    usleep (10000);
	}
	self->busy = true;
	psThreadUnlock();

	// list all allowed job types here

	// pmReadoutCombine
	if (!strcmp (job->type, "PPMERGE_READOUT_COMBINE")) {
	    psAssert (job->args->n == 5, "invalid number of arguments to pmReadoutCombine");

	    pmReadout *output           = job->args->data[0];
	    ppMergeFileGroup *fileGroup = job->args->data[1];
	    psVector *zero              = job->args->data[2];
	    psVector *scale             = job->args->data[3];
	    pmCombineParams *params     = job->args->data[4];

	    bool status = pmReadoutCombine (output, fileGroup->readouts, zero, scale, params);
	    if (!status) {
		self->fault = true;
	    }

	    // we do not have to lock here because this transition is not tied to the job queue
	    fileGroup->busy = false;
	    self->busy = false;  
	    continue;
	}

	// pmDarkCombine
	if (!strcmp (job->type, "PPMERGE_DARK_COMBINE")) {
	    psAssert (job->args->n == 7, "invalid number of arguments to pmDarkCombine");

	    pmCell *outCell             = job->args->data[0];
	    ppMergeFileGroup *fileGroup = job->args->data[1];
	    psArray *darkOrdinates      = job->args->data[2];
	    psString darkNorm           = job->args->data[3];
	    psScalar *iter     	        = job->args->data[4];
	    psScalar *rej     	        = job->args->data[5];
	    psScalar *maskVal     	= job->args->data[6];

	    bool status = pmDarkCombine(outCell, fileGroup->readouts, darkOrdinates, darkNorm, iter->data.S32, rej->data.F32, maskVal->data.PS_TYPE_IMAGE_MASK_DATA);
	    if (!status) {
		self->fault = true;
	    }
	    // we do not have to lock here because this transition is not tied to the job queue
	    fileGroup->busy = false;
	    self->busy = false;  
	    continue;
	}

	// pmShutterCorrectionGenerate
	if (!strcmp (job->type, "PPMERGE_SHUTTER_CORRECTION")) {
	    psAssert (job->args->n == 7, "invalid number of arguments to pmDarkCombine");

	    pmReadout *output             = job->args->data[0];
	    ppMergeFileGroup *fileGroup   = job->args->data[1];
	    psScalar *shutterRef          = job->args->data[2];
	    pmShutterCorrectionData *data = job->args->data[3];
	    psScalar *iter     	          = job->args->data[4];
	    psScalar *rej     	          = job->args->data[5];
	    psScalar *maskVal     	  = job->args->data[6];

	    bool status = pmShutterCorrectionGenerate(output, NULL, fileGroup->readouts, shutterRef->data.F32, data, iter->data.S32, rej->data.F32, maskVal->data.PS_TYPE_IMAGE_MASK_DATA);
	    if (!status) {
		self->fault = true;
	    }
	    // we do not have to lock here because this transition is not tied to the job queue
	    fileGroup->busy = false;
	    self->busy = false;  
	    continue;
	}
    }  
}
