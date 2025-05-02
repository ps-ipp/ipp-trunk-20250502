#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "ppImage.h"

// calculate stats from headers and concepts
bool ppImageMetadataStats(pmConfig *config, psMetadata *stats, const ppImageOptions *options)
{
    bool mdok;              // Status of MD lookup

    if (!options->doStats) {
        return true;
    }

    // Extract statistics from the last output fpa
    pmFPAfile *outImage  = psMetadataLookupPtr(&mdok, config->files, "PPIMAGE.OUTPUT");
    pmFPAfile *outPhotom = psMetadataLookupPtr(&mdok, config->files, "PSPHOT.OUTPUT");
    pmFPAfile *outAstrom = psMetadataLookupPtr(&mdok, config->files, "PSASTRO.OUTPUT");

    if (!outImage && !outPhotom && !outAstrom) {
        psError(PS_ERR_UNEXPECTED_NULL, true,
                "Unable to find any output file (PPIMAGE.OUTPUT, PSPHOT.OUTPUT, PSASTRO.OUTPUT).");
        return false;
    }

    // get the latest output product available
    pmFPAfile *output = outAstrom;
    if (!output) {
        output = outPhotom;
    }
    if (!output) {
        output = outImage;
    }

    // extract stats for the complete fpa
    pmFPAview *view = pmFPAviewAlloc(0);
    if (!ppStatsMetadata(stats, output->fpa, view, options->maskValue, config)) {
        psError(PS_ERR_UNEXPECTED_NULL, false, "Unable to generate stats for image.");
        psFree(view);
        return false;
    }

    psFree(view);
    return true;
}
