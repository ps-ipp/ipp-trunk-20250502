/** @file ppMerge.c
 *
 *  @brief
 *
 *  @ingroup ppMerge
 *
 *  @author IfA
 *  @version $Revision: 1.26 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-01 21:43:05 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

#include "ppMerge.h"

// Yet to do:
//
// 1. Mask pixels with less than the minimum number of electrons
// 2. Sampling factor for background measurement
// 3. On/off pairs

int main(int argc, char **argv)
{
    psLibInit(NULL);
    psTimerStart(TIMERNAME);

    psExit exitValue = PS_EXIT_SUCCESS; ///< Exit value for program

    pmConfig *config = pmConfigRead(&argc, argv, PPMERGE_RECIPE); ///< Configuration
    if (!config) {
        psErrorStackPrint(stderr, "Error reading configuration.");
        exitValue = PS_EXIT_CONFIG_ERROR;
        goto die;
    }

    if (!ppMergeArguments(argc, argv, config)) {
        psErrorStackPrint(stderr, "Error reading arguments.");
        exitValue = PS_EXIT_CONFIG_ERROR;
        goto die;
    }

    ppMergeVersionPrint();

    ppMergeType type = psMetadataLookupS32(NULL, config->arguments, "TYPE"); ///< Type of frame
    switch (type) {
      case PPMERGE_TYPE_MASK:
        if (!ppMergeMask(config)) {
            psErrorStackPrint(stderr, "Error generating mask.");
            exitValue = PS_EXIT_DATA_ERROR;
            goto die;
        }
        break;
      case PPMERGE_TYPE_BIAS:
      case PPMERGE_TYPE_DARK:
      case PPMERGE_TYPE_SHUTTER:
      case PPMERGE_TYPE_FLAT:
      case PPMERGE_TYPE_FRINGE:
      case PPMERGE_TYPE_CTEMASK:
      case PPMERGE_TYPE_NOISEMAP:
        if (!ppMergeScaleZero(config)) {
            psErrorStackPrint(stderr, "Error getting scale and zero-points.");
            exitValue = PS_EXIT_DATA_ERROR;
            goto die;
        }
        if (!ppMergeLoop(config)) {
            psErrorStackPrint(stderr, "Error performing merge.");
            exitValue = PS_EXIT_PROG_ERROR;
            goto die;
        }
        break;
      default:
        psAbort("Invalid frame type: %x", type);
    }


    // Output the statistics
    bool mdok;                          ///< Status of MD lookup
    psString statsName = psMetadataLookupStr(&mdok, config->arguments, "STATS.NAME"); ///< Statistics file name
    if (mdok && statsName && strlen(statsName) > 0) {
        psString resolved = pmConfigConvertFilename(statsName, config, true, true); ///< Resolved filename
        FILE *statsFile = fopen(resolved, "w"); ///< Output statistics file
        if (!statsFile) {
            psError(PS_ERR_IO, true, "Unable to open statistics file %s for writing.", resolved);
            psFree(resolved);
            exitValue = PS_EXIT_CONFIG_ERROR;
            goto die;
        }
        psFree(resolved);
        psMetadata *stats = psMetadataLookupMetadata(&mdok, config->arguments, "STATS.DATA"); ///< Statistics
        if (!stats) {
            psError(PS_ERR_UNEXPECTED_NULL, false, "Unable to find statistics");
            exitValue = PS_EXIT_PROG_ERROR;
            goto die;
        }
        psString statsOut = psMetadataConfigFormat(stats); ///< String to write out
        fprintf(statsFile, "%s", statsOut);
        psFree(statsOut);
        fclose(statsFile);
        pmConfigRunFilenameAddWrite(config, "STATS", statsName);
    }




#if 0
    // Set various tasks (define optional operations)
    ppMergeOptions *options = ppMergeOptionsParse(config);
    if (!options) {
        psErrorStackPrint(stderr, "Unable to parse options.");
        exit(EXIT_FAILURE);
    }

    // Check the inputs
    ppMergeData *data = ppMergeCheckInputs(options, config);
    if (!data) {
        psErrorStackPrint(stderr, "Not enough valid input files.");
        exit(EXIT_FAILURE);
    }

    if (options->mask) {
        // Generate a mask
        ppMergeMask(data, options, config);
    } else {

        psImage *scale = NULL;              // The scalings
        psImage *zero = NULL;               // The zeros
        psArray *shutters = NULL;           // The shutter correction data

        // Measure the background in each image
        ppMergeScaleZero(&scale, &zero, &shutters, data, options, config);

        // Do the combination and write
        ppMergeCombine(scale, zero, shutters, data, options, config);

        psFree(scale);
        psFree(zero);
        psFree(shutters);
    }

    // Output the statistics
    if (data->statsFile && data->stats) {
        psString statsOut = psMetadataConfigFormat(data->stats); // String to write out
        fprintf(data->statsFile, "%s", statsOut);
        psFree(statsOut);
    }

    // Cleaning up
    pmDarkVisualCleanup();
    psFree(data);
    psFree(options);
#endif

 die:
    psTrace("ppSub", 1, "Finished at %f sec\n", psTimerMark("ppSub"));
    psTimerStop();

    pmDarkVisualCleanup();
    psFree(config);
    pmConfigDone();
    psLibFinalize();

    psLogMsg("ppMerge", PS_LOG_INFO, "Memory leaks: %d\\n", psMemCheckLeaks(0, NULL, stdout, false));

    exit(exitValue);
}
