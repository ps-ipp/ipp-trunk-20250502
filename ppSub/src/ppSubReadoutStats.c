#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <pslib.h>
#include <psmodules.h>
#include <ppStats.h>

#include "ppSub.h"


bool ppSubReadoutStats(ppSubData *data)
{
    psAssert(data, "Require processing data");
    pmConfig *config = data->config;    // Configuration
    psAssert(config, "Require configuration");

    if (!data->statsFile) {
        // Nothing to do
        return true;
    }

    pmFPAfile *output = psMetadataLookupPtr(NULL, config->files, "PPSUB.OUTPUT"); // Output file
    if (!output) {
        psError(PPSUB_ERR_PROG, true, "Unable to find file PPSUB.OUTPUT.\n");
        return false;
    }
    psImageMaskType maskValue = pmConfigMaskGet("MASK.VALUE", config);
    pmFPAview *view = ppSubViewReadout(); // View to readout
    ppStatsFPA(data->stats, output->fpa, view, maskValue, config);

    pmReadout *outRO = pmFPAviewThisReadout(view, output->fpa); // Readout of interest
    psFree(view);

    // Statistics on the matching
    psMetadataCopySingle(data->stats, outRO->analysis, PM_SUBTRACTION_ANALYSIS_MODE);
    psMetadataCopySingle(data->stats, outRO->analysis, PM_SUBTRACTION_ANALYSIS_STAMPS);
    psMetadataCopySingle(data->stats, outRO->analysis, PM_SUBTRACTION_ANALYSIS_DEV_MEAN);
    psMetadataCopySingle(data->stats, outRO->analysis, PM_SUBTRACTION_ANALYSIS_DEV_RMS);
    psMetadataCopySingle(data->stats, outRO->analysis, PM_SUBTRACTION_ANALYSIS_NORM);
    psMetadataCopySingle(data->stats, outRO->analysis, PM_SUBTRACTION_ANALYSIS_BGDIFF);
    psMetadataCopySingle(data->stats, outRO->analysis, PM_SUBTRACTION_ANALYSIS_MX);
    psMetadataCopySingle(data->stats, outRO->analysis, PM_SUBTRACTION_ANALYSIS_MY);
    psMetadataCopySingle(data->stats, outRO->analysis, PM_SUBTRACTION_ANALYSIS_MXX);
    psMetadataCopySingle(data->stats, outRO->analysis, PM_SUBTRACTION_ANALYSIS_MXY);
    psMetadataCopySingle(data->stats, outRO->analysis, PM_SUBTRACTION_ANALYSIS_MYY);
    psMetadataCopySingle(data->stats, outRO->analysis, PM_SUBTRACTION_ANALYSIS_DECONV_MAX);
    psMetadataCopySingle(data->stats, outRO->analysis, PM_SUBTRACTION_ANALYSIS_CONVOL_MAX);

    return true;
}
