#include "ppStatsInternal.h"

// measure only the pixel-related statistics (stats, summary)
psMetadata *ppStatsPixels(psMetadata *out,
			  pmFPA *fpa,         // FPA for which to get statistics
			  pmFPAview *view,    // View for analysis
			  psImageMaskType maskVal, // Value to mask
			  pmConfig *config    // Configuration
    )
{
    PS_ASSERT_PTR_NON_NULL(fpa, NULL)
    PS_ASSERT_PTR_NON_NULL(view, NULL);
    PS_ASSERT_PTR_NON_NULL(config, NULL);

    ppStatsData *data = ppStatsDataAlloc(); // All the input data

    // Get the options, open the files
    if (!ppStatsSetupFromRecipe(data, config)) {
        psError(PS_ERR_UNKNOWN, false, "Unable to get ppStats options from recipe.");
        psFree(data);
        return NULL;
    }

    // drop the 'concept' and 'header' elements of data:
    psFree (data->headers);    
    psFree (data->concepts);
    data->headers = psListAlloc(NULL);
    data->concepts = psListAlloc(NULL);

    // Override recipe mask value 
    data->maskVal = maskVal;

    if (data->fpa) {
        psFree(data->fpa);
    }
    data->fpa = psMemIncrRefCounter(fpa);

    if (data->view) {
        psFree(data->view);
    }
    data->view = psMemIncrRefCounter(view);

    // Go through the FPA and do the hard work
    psExit status;                      // Status of statistics loop
    psMetadata *result = ppStatsLoop(&status, data, config);
    if (status != PS_EXIT_SUCCESS) {
        psError (PS_ERR_UNKNOWN, false, "Not able to measure FPA statistics.\n");
        psFree(result);
        psFree(data);
        return (NULL);
    }

    if (out != NULL) {
	psMetadataOverlay (out, result);
	psFree(result);
	psFree(data);
	return out;
    }

    psFree(data);
    return result;
}
