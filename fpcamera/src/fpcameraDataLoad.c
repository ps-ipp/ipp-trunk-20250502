# include "fpcamera.h"

# define ESCAPE(ERROR,MSG) { psError(ERROR, false, MSG); return false; }
  
/*\brief this loop loads the data from the input files */

bool fpcameraDataLoad (pmConfig *config) {

    psTimerStart ("fpcamera");

    // select the input data sources
    pmFPAfile *input = psMetadataLookupPtr (NULL, config->files, "FPCAMERA.INPUT.ASTROM");
    if (!input) ESCAPE (PS_ERR_UNKNOWN, "FPCAMERA.INPUT.ASTROM not listed in config->files");

    // read the astrometry calibration information (from cmf/smf)
    if (!fpcameraReadAstrometry (input, config)) ESCAPE (PS_ERR_UNKNOWN, "Failure to read astrometry calibration");

    // load the reference stars overlapping the data stars
    if (!fpcameraLoadRefstars(input, config)) ESCAPE (PS_ERR_UNKNOWN, "failed to load reference data");

    psLogMsg ("fpcamera", 3, "load data : %f sec\n", psTimerMark ("fpcamera"));

    return true;
}
