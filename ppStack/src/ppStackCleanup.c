#include "ppStack.h"

// ppStackCleanupFiles 
bool ppStackCleanupFiles(ppStackThreadData *stack, ppStackOptions *options, pmConfig *config, ppStackFileList stackFiles, ppStackFileList photFiles, bool closeJPEGs)
{
    psAssert(stack, "Require stack");
    psAssert(options, "Require options");
    psAssert(config, "Require configuration");

    if (!ppStackFilesIterateUp(config)) {
        psError(psErrorCodeLast(), false, "Unable to close files.");
        return false;
    }
    ppStackFileActivation(config, stackFiles, false);
    ppStackFileActivation(config, photFiles, false);

    // Ensure files are freed
    options->outRO->data_exists = false;
    options->outRO->parent->data_exists = false;
    options->outRO->parent->parent->data_exists = false;
    psFree(options->outRO);
    options->outRO = NULL;

    if (options->expRO) {
      options->expRO->data_exists = false;
      if (options->expRO->parent) {
	options->expRO->parent->data_exists = false;
	options->expRO->parent->parent->data_exists = false;
      }
      psFree(options->expRO);
      options->expRO = NULL;
    }

    if (options->bkgRO) {
      options->bkgRO->data_exists = false;
      if (options->bkgRO->parent) {
	options->bkgRO->parent->data_exists = false;
	options->bkgRO->parent->parent->data_exists = false;
      }
      psFree(options->bkgRO);
      options->bkgRO = NULL;
    }
    
    for (int i = 0; i < options->num; i++) {
        pmCellFreeData(options->cells->data[i]);
    }

    if (closeJPEGs) {
	// XXX move these close / free operations to the jpeg creation function
        pmFPAview *view = pmFPAviewAlloc(0);// Pointer into FPA hierarchy
        view->chip = view->cell = 0;        // pmFPAviewFreeData doesn't want to deal with readouts
        pmFPAfile *jpeg1 = pmFPAfileSelectSingle(config->files, "PPSTACK.OUTPUT.JPEG1", 0); // JPEG file
        pmFPAviewFreeData(view, jpeg1);
        pmFPAfile *jpeg2 = pmFPAfileSelectSingle(config->files, "PPSTACK.OUTPUT.JPEG2", 0); // JPEG file
        pmFPAviewFreeData(view, jpeg2);
        pmFPAfile *phot = NULL;         // Photometry file
        if (options->photometry) {
            phot = pmFPAfileSelectSingle(config->files, "PSPHOT.INPUT", 0); // Photometry file
            pmFPAviewFreeData(view, phot);
        }

        view->readout = 0;
        pmReadout *ro1 = pmFPAviewThisReadout(view, jpeg1->fpa); // JPEG readout
        ro1->data_exists = ro1->parent->data_exists = ro1->parent->parent->data_exists = false;
        pmReadout *ro2 = pmFPAviewThisReadout(view, jpeg2->fpa); // JPEG readout
        ro2->data_exists = ro2->parent->data_exists = ro2->parent->parent->data_exists = false;
        if (options->photometry) {
            pmReadout *ro = pmFPAviewThisReadout(view, phot->fpa); // Photometry readout
            ro->data_exists = ro->parent->data_exists = ro->parent->parent->data_exists = false;
        }
        psFree(view);
    }

    return true;
}

bool ppStackCleanup (pmConfig *config, ppStackOptions *options) {
    psExit exitValue = ppStackExitCode(PS_EXIT_SUCCESS); // Exit code
    
    // Ensure everything closes
    if (config) {
	ppStackFileActivation(config, PPSTACK_FILES_PREPARE, true);
	ppStackFileActivation(config, PPSTACK_FILES_CONVOLVE, true);
	ppStackFileActivation(config, PPSTACK_FILES_STACK, true);
	ppStackFileActivation(config, PPSTACK_FILES_UNCONV, true);
	ppStackFileActivation(config, PPSTACK_FILES_PHOT, true);
	ppStackFileActivation(config, PPSTACK_FILES_MEDIAN_IN, true);
	ppStackFileActivation(config, PPSTACK_FILES_MEDIAN_OUT, true);
	if (!ppStackFilesIterateUp(config)) {
	    psError(psErrorCodeLast(), false, "Unable to close files.");
	    exitValue = ppStackExitCode(exitValue);
	    pmFPAfileFreeSetStrict(false);
	}
    }

    // Write out summary statistics
    if (options && options->stats) {

	psMetadataAddS32(options->stats, PS_LIST_TAIL, "QUALITY", PS_META_REPLACE, "Bad data quality flag", options->quality);
	psMetadataAddF32(options->stats, PS_LIST_TAIL, "TIME_STACK", 0, "Total time", psTimerClear("PPSTACK_TOTAL"));

	const char *statsMDC = psMetadataConfigFormat(options->stats);
	if (!statsMDC || strlen(statsMDC) == 0) {
	    psError(PS_ERR_IO, false, "Unable to get statistics MDC file.");
	    exitValue = ppStackExitCode(exitValue);
	    exit(exitValue);
	}
	if (fprintf(options->statsFile, "%s", statsMDC) != strlen(statsMDC)) {
	    psError(PS_ERR_IO, false, "Unable to write statistics MDC file.");
	    exitValue = ppStackExitCode(exitValue);
	    exit(exitValue);
	}
	psFree(statsMDC);
	if (fclose(options->statsFile) == EOF) {
	    psError(PS_ERR_IO, false, "Unable to close statistics MDC file.");
	    exitValue = ppStackExitCode(exitValue);
	    exit(exitValue);
	}
	options->statsFile = NULL;
	pmConfigRunFilenameAddWrite(config, "STATS", psMetadataLookupStr(NULL, config->arguments, "STATS"));
    }
    psFree(options);

    // Dump configuration
    bool mdok;                                                                    // Status of MD lookup
    psString dump = psMetadataLookupStr(&mdok, config->arguments, "DUMP_CONFIG"); // File for config
    if (dump && !pmConfigDump(config, dump)) {
	psError(psErrorCodeLast(), false, "Unable to dump configuration.");
	exitValue = ppStackExitCode(exitValue);
    }

    psTrace("ppStack", 1, "Finished at %f sec\n", psTimerMark("PPSTACK"));
    psLogMsg("ppStack", PS_LOG_INFO, "Complete ppStack run: %f sec\n", psTimerMark("PPSTACK"));
    psTimerStop();

    psFree(config);
    pmModelClassCleanup();
    pmConfigDone();
    psLibFinalize();
    pmVisualClose();
    pmVisualCleanup ();

    exitValue = ppStackExitCode(exitValue);
    exit(exitValue);
}
