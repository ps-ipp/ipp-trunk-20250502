#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "ppImage.h"

// write stats to output file
bool ppImageStatsOutput(pmConfig *config, psMetadata *stats, const ppImageOptions *options)
{
    bool mdok;

    // measure statistics, or ignore?
    if (!options->doStats) {
        return true;
    }

    // get the output stats filename
    const char *statsName = psMetadataLookupStr(&mdok, config->arguments, "STATS"); // Filename for statistics
    if (!statsName || !strlen(statsName)) {
        psError(PS_ERR_UNEXPECTED_NULL, false, "missing STATS entry in arguments list.");
        return false;
    }

    // Write out
    psString resolved = pmConfigConvertFilename(statsName, config, true, true); // Resolved filename
    if (!resolved) {
        psError(psErrorCodeLast(), false, "Unable to resolve statistics file name");
        return false;
    }

    // check for Metadata compression options:
    char *compressMode = NULL;
    bool status = false;
    if (config->camera) {
	// XXX use a different config variable for this output?
	compressMode = psMetadataLookupStr(&status, config->camera, "METADATA.COMPRESSION");
    }

    if (!psMetadataConfigWrite(stats, resolved, compressMode)) {
        psError(psErrorCodeLast(), false, "Unable to serialize stats metadata.\n");
        psFree(resolved);
        return false;
    }
    psFree(resolved);

    pmConfigRunFilenameAddWrite(config, "STATS", statsName);

    return true;
}
