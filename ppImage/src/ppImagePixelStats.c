#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "ppImage.h"

// calculate stats, including MD5
bool ppImagePixelStats(pmConfig *config, psMetadata *stats, const ppImageOptions *options,
                       const pmFPAview *inputView)
{
    bool mdok;              // Status of MD lookup

    // psTimerStart("pixel.stats");

    // loop over the cells/readouts for this chip
    pmFPAview *view = pmFPAviewAlloc(0); // View for local processing
    *view = *inputView;

    // perform the analysis of for this FPA
    pmFPAfile *output = psMetadataLookupPtr(&mdok, config->files, "PPIMAGE.OUTPUT");
    if (!output) {
        psError(PS_ERR_UNEXPECTED_NULL, true, "Unable to find output file (PPIMAGE.OUTPUT).");
        psFree(view);
        return false;
    }

    // select the corresponding chip
    pmChip *chip = pmFPAviewThisChip(view, output->fpa);
    if (!chip) {
        psError(PS_ERR_UNEXPECTED_NULL, false, "Unable to find requested chip.");
        psFree(view);
        return false;
    }

    // Perform statistics for this chip
    if (options->doStats) {
        if (!ppStatsPixels(stats, output->fpa, view, options->maskValue, config)) {
            psError(PS_ERR_UNEXPECTED_NULL, false, "Unable to generate stats for image.");
            psFree(view);
            return false;
        }

        // extract the fringe amplitude from the analysis
        if (options->doFringe && !ppStatsFringe(stats, chip, "FRINGE", "FRINGE.SOLUTION")) {
            psError(PS_ERR_UNEXPECTED_NULL, false, "Unable to extract fringe solution for image.");
            psFree(view);
            return false;
        }

        // extract the fringe residual amplitude from the analysis
        if (options->doFringe && !ppStatsFringe(stats, chip, "FRINGE_RESID", "FRINGE.RESIDUAL.SOLUTION")) {
            psError(PS_ERR_UNEXPECTED_NULL, false, "Unable to extract fringe solution for image.");
            psFree(view);
            return false;
        }
    }


    view->cell = view->readout = -1;
    pmCell *cell = NULL;                // Cell from iteration
    while ((cell = pmFPAviewNextCell(view, output->fpa, 1)) != NULL) {
        if (!cell->process || !cell->file_exists) {
            continue;
        }

        // Add MD5 information for cell
        pmHDU *hdu = pmHDUFromCell(cell); // HDU that owns the cell
        pmReadout *readout = NULL;      // Readout from iteration
        while ((readout = pmFPAviewNextReadout(view, output->fpa, 1)) != NULL) {
            const char *chipName = psMetadataLookupStr(NULL, chip->concepts, "CHIP.NAME");
            const char *cellName = psMetadataLookupStr(NULL, cell->concepts, "CELL.NAME");

            psString headerName = NULL; // Header name for MD5
            psStringAppend(&headerName, "MD5_%s_%s_%d", chipName, cellName, view->readout);

            psVector *md5 = psImageMD5(readout->image); // md5 hash
            psString md5string = psMD5toString(md5); // String
            psFree(md5);
            psMetadataAddStr(hdu->header, PS_LIST_TAIL, headerName, PS_META_REPLACE, "Image MD5", md5string);
            psFree(md5string);
            psFree(headerName);
        }
    }

    // psLogMsg ("ppImage", 5, "measure pixel stats: %f sec\n", psTimerMark ("pixel.stats"));

    psFree (view);
    return true;
}
