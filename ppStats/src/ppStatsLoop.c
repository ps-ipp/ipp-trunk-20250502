# include "ppStatsInternal.h"

psMetadata *ppStatsLoop(psExit *result,
                        ppStatsData *data, // The data
                        pmConfig *config // Configuration
    )
{
    PS_ASSERT_PTR_NON_NULL(data, NULL);
    pmFPA *fpa = data->fpa;             // FPA to analyse
    psFits *fits = data->fits;          // FITS file handle
    PS_ASSERT_PTR_NON_NULL(fpa, NULL);

    *result = PS_EXIT_SUCCESS;

    // allocate or bump the ref counter (so we can just free below)
    pmFPAview *view = (data->view) ? psMemIncrRefCounter(data->view) : pmFPAviewAlloc(0);

    // allocate a new one if needed
    psMetadata *newResults = psMetadataAlloc();

    // check if we can legitimately iterate to the chip level
    psArray *chips = fpa->chips;        // Array of chips
    if (view->chip >= chips->n) {
        psError(PS_ERR_BAD_PARAMETER_VALUE, true, "Desired chip view (%d) doesn't match "
                "number of chips (%ld)\n", view->chip, chips->n);
        return NULL;
    }

    // Iterate over chips (if view->chip is set, skip all others)
    for (int i = 0; i < chips->n; i++) {
        if ((view->chip >= 0) && (i != view->chip)) continue;
        pmChip *chip = chips->data[i];  // Chip of interest
        *result = ppStatsChip(newResults, chip, fits, view, data, config);
        if (*result != PS_EXIT_SUCCESS) {
            psError(PS_ERR_UNKNOWN, false, "trouble with stats for chip %d\n", i);
            psFree (view);
            psFree (newResults);
            return NULL;
        }
    }

    // Iterate through the FPA -- Do this after the chip since we in the SPLIT cases, we
    // only populate the FPA concepts after chips have been read.
    if (psListLength(data->headers) > 0 && fpa->hdu) {
        if (fits && !pmFPAReadHeader(fpa, fits, config)) {
            psError(PS_ERR_IO, false, "Unable to read header for FPA.");
            psFree(view);
            psFree(newResults);
            *result = PS_EXIT_DATA_ERROR;
            return NULL;
        }
        pmHDU *hdu = fpa->hdu;          // HDU for headers
        p_ppStatsGetMetadata(newResults, hdu->header, data->headers);
    }
    if (psListLength(data->concepts) > 0) {
        if (fits && fpa->hdu && !pmFPAReadHeader(fpa, fits, config)) {
            psError(PS_ERR_IO, false, "Unable to read header for FPA.");
            psFree(view);
            psFree(newResults);
            *result = PS_EXIT_DATA_ERROR;
            return NULL;
        }
        pmConceptsReadFPA(fpa, PM_CONCEPT_SOURCE_ALL, false, config);
        p_ppStatsGetMetadata(newResults, fpa->concepts, data->concepts);
    }

    if (fits) {
        pmFPAFreeData(fpa);
    }

    psFree(view);
    return newResults;
}
