#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

# include "ppImage.h"

bool ppImageDefineFile(pmConfig *config, pmFPA *input, char *filerule, char *argname,
                       pmFPAfileType fileType, pmDetrendType detrendType)
{
    bool status;
    pmFPAfile *file = NULL;             // File to be defined

    if (!file) {
        // look for the file on the argument list
        file = pmFPAfileDefineFromArgs(&status, config, filerule, argname);
        if (!status) {
            psError(PS_ERR_UNKNOWN, false, "failed to load file definition ARG LIST");
            return false;
        }
    }
    if (!file) {
        // look for the file on the RUN metadata
        file = pmFPAfileDefineFromRun(&status, NULL, config, filerule);
        if (!status) {
            psError(PS_ERR_UNKNOWN, false, "failed to load file definition RUN");
            return false;
        }
    }
    if (!file) {
        // look for the file in the camera config table
        file = pmFPAfileDefineFromConf(&status, config, filerule);
        if (!status) {
            psError(PS_ERR_UNKNOWN, false, "failed to load file definition CONFIG");
            return false;
        }
    }
    if (!file) {
        // look for the file to be loaded from the detrend database
        file = pmFPAfileDefineFromDetDB(&status, config, filerule, input, detrendType);
        if (!status) {
            psError(PS_ERR_UNKNOWN, false, "failed to load file definition DETREND");
            return false;
        }
    }

    if (!file) {
        return false;
    }

    if (file->type != fileType) {
        psError(PS_ERR_IO, true, "%s is not of type %s", filerule, pmFPAfileStringFromType(fileType));
        return false;
    }
    return true;
}

