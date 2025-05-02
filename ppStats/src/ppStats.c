# include "ppStatsInternal.h"

int main(int argc, char **argv) {

    psExit status = PS_EXIT_SUCCESS;

    psLibInit(NULL);
    psTimerStart("PPSTATS");

    // Parse the configuration and arguments
    pmConfig *config = pmConfigRead(&argc, argv, PPSTATS_RECIPE);
    if (!config) {
        psErrorStackPrint(stderr, "Unable to read configuration.\n");
        exit(PS_EXIT_CONFIG_ERROR);
    }

    // Get the options, open the files
    ppStatsData *data = ppStatsSetupFromArgs(&argc, argv, config);
    if (!data) {
        psErrorStackPrint(stderr, "Unable to parse command-line arguments.\n");
        exit(PS_EXIT_CONFIG_ERROR);
    }

    // Output filename is optional
    const char *outName = NULL;         // Output file name
    FILE *outFile = stdout;             // Output file
    if (argc == 2) {
        outName = argv[1];
        psString resolved = pmConfigConvertFilename(outName, config, true, true); // Resolved filename

        if (resolved && strlen(resolved) > 0) {
            outFile = fopen(resolved, "w");
            if (!outFile) {
                psLogMsg("ppStats", PS_LOG_ERROR, "Unable to open output file %s\n", resolved);
                psFree(resolved);
                // XXX this could be a system or config error, but not a data error
                status = PS_EXIT_CONFIG_ERROR;
                goto die;
            }
        } else {
            psErrorStackPrint(stderr, "Unable to open output file %s.\n", resolved);
            exit(PS_EXIT_CONFIG_ERROR);
        }
        psFree(resolved);
    }

    // Go through the FPA and do the hard work
    psMetadata *results = ppStatsLoop(&status, data, config);
    if (status != PS_EXIT_SUCCESS) {
        psErrorStackPrint(stderr, "Error in stats loop.\n");
        exit (status);
    }

    if (data->showFormat) {
      char *formatName = psStringCopy (config->formatName);
      if (!formatName) {
	formatName = psStringCopy ("UNKNOWN");
      }
      psMetadataAddStr(results, PS_LIST_HEAD, "FILE.FORMAT", 0, "File format", formatName);
    }

    if (data->showCamera) {
      char *cameraName = psStringCopy (config->cameraName);
      if (!cameraName) {
	cameraName = psStringCopy ("UNKNOWN");
      }
      psMetadataAddStr(results, PS_LIST_HEAD, "CAMERA", 0, "camera name", cameraName);
    }

    // report on the file disposition
    if (data->fileLevel) {
        pmFPALevel level = pmFPAPHULevel(config->format);

        const char *levelName = pmFPALevelToName(level); // Level for file
        psMetadataAddStr(results, PS_LIST_HEAD, "FILE.LEVEL", 0, "File level", levelName);

        char *classID = NULL;
        switch (level) {
          case PM_FPA_LEVEL_FPA:
            assert (data->fpa != NULL);
            assert (data->fileView->chip == -1);
            classID = pmFPANameFromRule ("{FPA.NAME}", data->fpa, data->fileView);
            break;
          case PM_FPA_LEVEL_CHIP:
            assert (data->fpa != NULL);
            assert (data->fileView->chip != -1);
            assert (data->fileView->cell == -1);
            classID = pmFPANameFromRule ("{CHIP.NAME}", data->fpa, data->fileView);
            break;
          case PM_FPA_LEVEL_CELL:
            assert (data->fpa != NULL);
            assert (data->fileView->chip != -1);
            assert (data->fileView->cell != -1);
            classID = pmFPANameFromRule ("{CELL.NAME}", data->fpa, data->fileView);
            break;
          default:
            psErrorStackPrint(stderr, "Error in file level.\n");
            exit (PS_EXIT_CONFIG_ERROR);
        }
        psMetadataAddStr(results, PS_LIST_HEAD, "CLASS.ID", 0, "name for element at file level", classID);
        psFree (classID);
    }

    // did we actually request any data?
    if (psListLength(results->list) == 0) {
        psErrorStackPrint(stderr, "No output.\n");
        exit (status);
    }

    // Format and print the output
    psString output = psMetadataConfigFormat(results);
    if (!output) {
        psErrorStackPrint(stderr, "Unable to generate configuration file with result.\n");
        psFree(results);
        exit(PS_EXIT_CONFIG_ERROR);
    }
    fprintf(outFile, "%s", output);
    psFree(output);

    // Clean up
    psFree(results);
    if (outName) {
        fclose(outFile);
    }

    // Common code for the death.
die:
    if (status) {
        psErrorStackPrint (stderr, "failure in %s", __func__);
    }
    psFree(data);
    psFree(config);
    pmConceptsDone();
    pmConfigDone();
    psLibFinalize();

    return status;
}
