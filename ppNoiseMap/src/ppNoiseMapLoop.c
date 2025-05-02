#include "ppNoiseMap.h"

#define ESCAPE(MESSAGE) { \
  psError(PS_ERR_UNKNOWN, false, MESSAGE); \
  psFree(view); \
  return false; \
}

bool ppNoiseMapLoop(pmConfig *config)
{
    bool status;                        // Status of MD lookup
    pmFPAfile *input = psMetadataLookupPtr(&status, config->files, "PPNOISEMAP.INPUT");
    if (!status) {
        psErrorStackPrint(stderr, "Can't find input data!\n");
        ppNoiseMapCleanup(config);
        exit(PS_EXIT_PROG_ERROR);
    }

    pmConfigCamerasCull(config, NULL);
    pmConfigRecipesCull(config, "PPNOISEMAP,MASKS");

    pmFPAview *view = pmFPAviewAlloc(0);// View for level of interest
    pmHDU *lastHDU = NULL;              // Last HDU that was updated

    // files associated with the science image
    if (!pmFPAfileIOChecks(config, view, PM_FPA_BEFORE)) {
        ESCAPE("load failure for FPA");
    }

    pmChip *chip;                       // Chip from FPA
    while ((chip = pmFPAviewNextChip(view, input->fpa, 1)) != NULL) {
        psLogMsg ("ppNoiseMapLoop", 4, "Chip %d: %x %x\n", view->chip, chip->file_exists, chip->process);
        if (!chip->process || !chip->file_exists) {
            continue;
        }
        if (!pmFPAfileIOChecks(config, view, PM_FPA_BEFORE)) {
            ESCAPE("load failure for Chip");
        }

	// load all cells for this chip before generating output file fpa:
        pmCell *cell;                   // Cell from chip
        while ((cell = pmFPAviewNextCell(view, input->fpa, 1)) != NULL) {
            psLogMsg ("ppNoiseMapLoop", 5, "Cell %d: %x %x\n", view->cell, cell->file_exists, cell->process);
            if (!cell->process || !cell->file_exists) {
                continue;
            }
            if (!pmFPAfileIOChecks(config, view, PM_FPA_BEFORE)) {
                ESCAPE("load failure for Cell");
            }

            // Put version information into the header
            pmHDU *hdu = pmHDUGetHighest(input->fpa, chip, cell);
            if (hdu && hdu != lastHDU) {
                ppNoiseMapVersionHeader(hdu->header);
                lastHDU = hdu;
            }

            // XXX for now, skip the video cells (cell->readouts->n > 1)
            if (cell->readouts->n > 1) {
              psWarning ("Skipping Video Cell for ppNoiseMap");
              continue;
            }

            // process each of the readouts
            pmReadout *readout;         // Readout from cell
            while ((readout = pmFPAviewNextReadout (view, input->fpa, 1)) != NULL) {
                if (!pmFPAfileIOChecks(config, view, PM_FPA_BEFORE)) {
                    ESCAPE("load failure for Readout");
                }
                if (!readout->data_exists) {
                    continue;
                }
	    }
	}

	if (!ppNoiseMapReadout(config, view)) {
	    ESCAPE("Unable to measure noise map for readout");
	}
	
	// close the cells
        while ((cell = pmFPAviewNextCell(view, input->fpa, 1)) != NULL) {
            if (!cell->process || !cell->file_exists) {
                continue;
            }
            if (!pmFPAfileIOChecks(config, view, PM_FPA_AFTER)) {
                ESCAPE("save failure for Cell");
            }
        }

        // Close chip
        if (!pmFPAfileIOChecks(config, view, PM_FPA_AFTER)) {
            ESCAPE("save failure for Chip");
        }
    }

    // Output and Close FPA
    if (!pmFPAfileIOChecks(config, view, PM_FPA_AFTER)) {
        ESCAPE("save failure for FPA");
    }
    psFree(view);

    return true;
}
