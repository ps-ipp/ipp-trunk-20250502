#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "ppImage.h"

bool ppImageThread_ppImageDetrendReadout (psThreadJob *job) {

    pmConfig *config        = job->args->data[0];
    ppImageOptions *options = job->args->data[1];
    pmFPAview *view         = job->args->data[2];

    bool status = ppImageDetrendReadout(config, options, view);
    return status;
}

bool ppImageThread_ppImageDetrendPatternApplyCell (psThreadJob *job) {

    pmConfig *config        = job->args->data[0];
    pmFPA *fpa              = job->args->data[1];
    pmChip *chip            = job->args->data[2];
    pmCell *cell            = job->args->data[3];
    pmFPAview *view         = job->args->data[4];
    ppImageOptions *options = job->args->data[5];

    bool status = ppImageDetrendPatternApplyCell (config, fpa, chip, cell, view, options);
    return status;
}

bool ppImageSetThreads (void) {

# if (0)
    // *** Detrend Readout (now unused : threading is done at the readout level in pmDetrend)
    {
	psThreadTask *task = psThreadTaskAlloc ("PPIMAGE_DETREND_READOUT", 3);
	task->function = &ppImageThread_ppImageDetrendReadout;
	psThreadTaskAdd (task);
    }
# endif

    // *** Detrend Pattern Row
    {
	psThreadTask *task = psThreadTaskAlloc("PPIMAGE_PATTERN_ROW_CELL", 6);
	task->function = &ppImageThread_ppImageDetrendPatternApplyCell;
	psThreadTaskAdd(task);
	psFree(task);
    }
    return true;
}
