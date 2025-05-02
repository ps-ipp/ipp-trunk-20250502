#include "ppSmooth.h"

#define ESCAPE(MESSAGE) { \
  psError(PS_ERR_UNKNOWN, false, MESSAGE); \
  psFree(view); \
  return false; \
}

bool ppSmoothLoop(pmConfig *config)
{
    bool status;                        // Status of MD lookup
    pmFPAfile *input = psMetadataLookupPtr(&status, config->files, "PPSMOOTH.INPUT");
    if (!status) {
        psErrorStackPrint(stderr, "Can't find input data!\n");
        ppSmoothCleanup(config);
        exit(PS_EXIT_PROG_ERROR);
    }

    pmFPAview *view = pmFPAviewAlloc(0);// View for level of interest

    // files associated with the science image
    if (!pmFPAfileIOChecks(config, view, PM_FPA_BEFORE)) {
        ESCAPE("load failure for FPA");
    }

    pmChip *chip;                       // Chip from FPA
    while ((chip = pmFPAviewNextChip(view, input->fpa, 1)) != NULL) {
        psLogMsg ("ppSmoothLoop", 4, "Chip %d: %x %x\n", view->chip, chip->file_exists, chip->process);
        if (!chip->process || !chip->file_exists) {
            continue;
        }
        if (!pmFPAfileIOChecks(config, view, PM_FPA_BEFORE)) {
            ESCAPE("load failure for Chip");
        }

        pmCell *cell;                   // Cell from chip
        while ((cell = pmFPAviewNextCell(view, input->fpa, 1)) != NULL) {
            psLogMsg ("ppSmoothLoop", 5, "Cell %d: %x %x\n", view->cell, cell->file_exists, cell->process);
            if (!cell->process || !cell->file_exists) {
                continue;
            }
            if (!pmFPAfileIOChecks(config, view, PM_FPA_BEFORE)) {
                ESCAPE("load failure for Cell");
            }

            // Put version information into the header
	    ppSmoothVersionUpdateHeader(input->fpa, chip, cell);

            // XXX for now, skip the video cells (cell->readouts->n > 1)
            if (cell->readouts->n > 1) {
              psWarning ("Skipping Video Cell for ppSmoothReadout");
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

                // perform the smoothing
                if (!ppSmoothReadout(config, view)) {
                    ESCAPE("Unable to detrend readout");
                }
		// close readouts if possible
		if (!pmFPAfileIOChecks(config, view, PM_FPA_AFTER)) {
		    ESCAPE("save failure for Cell");
		}
            }
	    // close cells if possible
            if (!pmFPAfileIOChecks(config, view, PM_FPA_AFTER)) {
                ESCAPE("save failure for Cell");
            }
        }
        // close chips if possible
        if (!pmFPAfileIOChecks(config, view, PM_FPA_AFTER)) {
            ESCAPE("save failure for Chip");
        }
    }
    // close FPA if possilbe
    if (!pmFPAfileIOChecks(config, view, PM_FPA_AFTER)) {
        ESCAPE("save failure for FPA");
    }
    psFree(view);

    psString dump_file = psMetadataLookupStr(&status, config->arguments, "DUMP_CONFIG");
    if (dump_file) {
      if (!pmConfigDump(config, dump_file)) {
	ESCAPE("Unable to dump configuration.");
      }
    }


    return true;
}
