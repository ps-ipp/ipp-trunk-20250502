/** @file pswarp.c
 *
 *  @brief cleanup the open files & memory then exit
 *  @ingroup pswarp
 *
 *  @author IfA
 *  @version $Revision: 1.15 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-11 23:27:58 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

# include "pswarp.h"

// Lists of file rules which we may need to save here
static char *outputFiles[] = {
    "PSWARP.OUTPUT",
    "PSWARP.OUTPUT.MASK",
    "PSWARP.OUTPUT.VARIANCE",
    "PSWARP.OUTPUT.BKGMODEL",
    "PSWARP.OUTPUT.SOURCES",
    "PSPHOT.OUTPUT",
    "PSPHOT.RESID",
    "PSPHOT.BACKMDL",
    "PSPHOT.BACKMDL.STDEV",
    "PSPHOT.BACKGND",
    "PSPHOT.BACKSUB",
    "PSPHOT.PSF.SAVE",
    "SOURCE.PLOT.MOMENTS",
    "SOURCE.PLOT.PSFMODEL",
    "SOURCE.PLOT.APRESID",
    NULL
};

void pswarpCleanup (pmConfig *config, pswarpStatsFile *statsFile)
{
    psExit exitValue = pswarpExitCode(PS_EXIT_SUCCESS); // Exit code

    // activate all of the relevant output files
    pmFPAfileActivate(config->files, false, NULL);
    for (int i = 0; outputFiles[i] != NULL; i++) {
        pmFPAfileActivate(config->files, true, outputFiles[i]);
    }

    pmFPAfile *output = psMetadataLookupPtr(NULL, config->files, "PSWARP.OUTPUT");
    if (!output) {
        psError(PSWARP_ERR_CONFIG, false, "Can't find output data!\n");
	pmFPAfileFreeSetStrict(false);
	goto DONE;
    }

    pmFPAview *view = pmFPAviewAlloc(0);
    if (!pmFPAfileIOChecks(config, view, PM_FPA_BEFORE)) {
	psError(psErrorCodeLast(), false, "Unable to read files.");
	pmFPAfileFreeSetStrict(false);
	psFree(view);
	goto DONE;
    }

    pmChip *chip;
    while ((chip = pmFPAviewNextChip (view, output->fpa, 1)) != NULL) {
	psTrace ("pswarp", 4, "Chip %d: %x %x\n", view->chip, chip->file_exists, chip->process);
	if (!chip->process || !chip->file_exists) { continue; }
	if (!pmFPAfileIOChecks(config, view, PM_FPA_BEFORE)) {
	    psError(psErrorCodeLast(), false, "Unable to read files.");
	    pmFPAfileFreeSetStrict(false);
	    psFree(view);
	    goto DONE;
	}
	pmCell *cell;
	while ((cell = pmFPAviewNextCell (view, output->fpa, 1)) != NULL) {
	    psTrace ("pswarp", 4, "Cell %d: %x %x\n", view->cell, cell->file_exists, cell->process);
	    if (!cell->process || !cell->file_exists) { continue; }
	    if (!pmFPAfileIOChecks (config, view, PM_FPA_BEFORE) ||
		!pmFPAfileIOChecks (config, view, PM_FPA_AFTER)) {
		psError(psErrorCodeLast(), false, "Unable to read files.");
		pmFPAfileFreeSetStrict(false);
		psFree(view);
		goto DONE;
	    }
	}
	if (!pmFPAfileIOChecks (config, view, PM_FPA_AFTER)) {
	    psError(psErrorCodeLast(), false, "Unable to write files.");
	    pmFPAfileFreeSetStrict(false);
	    psFree(view);
	    goto DONE;
	}
    }

    if (!pmFPAfileIOChecks (config, view, PM_FPA_AFTER)) {
	psError(psErrorCodeLast(), false, "Unable to write files.");
	pmFPAfileFreeSetStrict(false);
	psFree(view);
	goto DONE;
    }
    psFree(view);

    if (!pswarpStatsFileSave (config, statsFile)) {
	psError(psErrorCodeLast(), false, "Unable to save stats file.");
	exitValue = pswarpExitCode(exitValue);
    }

    // Dump configuration
    bool mdok = false;
    psString dump_file = psMetadataLookupStr(&mdok, config->arguments, "DUMP_CONFIG");
    if (dump_file) {
	if (!pmConfigDump(config, dump_file)) {
	    psError(psErrorCodeLast(), false, "Unable to dump configuration");
	    exitValue = pswarpExitCode(exitValue);
	}
    }

    psThreadPoolFinalize();
    psMemCheckCorruption(stderr, true);

    psFree(config);

    psTimerStop();
    pmVisualClose();
    pmModelClassCleanup();
    pmConceptsDone();
    pmConfigDone();
    psLibFinalize();

    psMemBlock **memblocks;
    int Nleaks = psMemCheckLeaks (0, &memblocks, stderr, false);
    fprintf (stderr, "Found %d leaks at %s\n", Nleaks, "ppImage");

DONE:
    exitValue = pswarpExitCode(exitValue);
    exit (exitValue);
}
