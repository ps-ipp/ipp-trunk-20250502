#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "ppImage.h"

// list of the detrend types to free
static char *detrendTypes[] = {
    "PPIMAGE.MASK",
    "PPIMAGE.NOISEMAP",
    "PPIMAGE.BIAS",
    "PPIMAGE.DARK",
    "PPIMAGE.FLAT",
    "PPIMAGE.SHUTTER",
    "PPIMAGE.LINEARITY",
    "PPIMAGE.NEWNONLIN",
    NULL
};

bool ppImageDetrendFree (pmConfig *config, pmFPAview *view) {

    bool status;

    for (int i = 0; detrendTypes[i] != NULL; i++) {

	pmFPAfile *file = psMetadataLookupPtr(&status, config->files, detrendTypes[i]); // File of interest
	psTrace("pmFPAfileFree",1,"Working on %s\n",detrendTypes[i]);
	if (!file) continue; // not all detrends are used in any given run

	// this only returns false on a failure.  if we are not ready to write or close, it is not an error
	if (!pmFPAfileWrite (file, view, config)) {
	    psError(PS_ERR_IO, false, "failed to WRITE %s", file->name);
	    return false;
	}
	if (!pmFPAfileClose(file, view)) {
	    psError(PS_ERR_IO, false, "failed to CLOSE for %s", file->name);
	    return false;
	}
	if (!pmFPAfileFreeData(file, view)) {
	    if (!psMetadataRemoveKey(config->files, file->name)) {
		psError(PS_ERR_IO, false, "failed to remove %s in FPA_AFTER block", file->name);
		return false;
	    }
	}
    }
    return true;
}

bool ppImageFringeFree (pmConfig *config, pmFPAview *view) {

    bool status;

    pmFPAfile *file = psMetadataLookupPtr(&status, config->files, "PPIMAGE.FRINGE"); // File of interest
    if (!file) return true;

    // this only returns false on a failure.  if we are not ready to write or close, it is not an error
    if (!pmFPAfileWrite (file, view, config)) {
	psError(PS_ERR_IO, false, "failed to WRITE %s", file->name);
	return false;
    }
    if (!pmFPAfileClose(file, view)) {
	psError(PS_ERR_IO, false, "failed to CLOSE for %s", file->name);
	return false;
    }
    if (!pmFPAfileFreeData(file, view)) {
	if (!psMetadataRemoveKey(config->files, file->name)) {
	    psError(PS_ERR_IO, false, "failed to remove %s in FPA_AFTER block", file->name);
	    return false;
	}
    }

    return true;
}
