# include "psphotStandAlone.h"

void psphotCleanup (pmConfig *config) {

    // Dump configuration if requested
    bool status;
    psString dump_file = psMetadataLookupStr(&status, config->arguments, "DUMP_CONFIG");
    if (dump_file) {
        (void)pmConfigCamerasCull(config,NULL);
	(void)pmConfigRecipesCull(config,NULL);
        pmConfigDump(config, dump_file);
    }

    // if the program has stats write it out
    psphotStatsFile *statsFile = psphotStatsFileGet();
    if (statsFile) {
        psphotStatsFileSave(config, statsFile);
    }

    psFree (config);

    psphotVisualClose();
    psMemCheckCorruption (stderr, true);
    pmModelClassCleanup ();
    pmSourceFitSetDone ();
    pmConceptsDone ();
    pmConfigDone ();
    pmVisualCleanup ();
    psLibFinalize();
#if (PS_TRACE_ON)
    // don't display memory leaks unless trace is on. 
    // fprintf (stderr, "found %d leaks at %s\n", psMemCheckLeaks (0, NULL, NULL, false), "psphot");
    fprintf (stderr, "found %d leaks at %s\n", psMemCheckLeaks2 (0, NULL, stdout, false, 500), "psphot");
#endif
    return;
}

psExit psphotGetExitStatus (void) {

    // gcc -Wswitch complains here if err is declared as type psErrorCode
    // the collection of ps*ErrorCode values are enums defined separately for 
    // each module (psphot, pswarp, etc).  the lowest type, psErrorCode is only the base set and does
    // not include the possible psphot values

    // for now, to get around this, we just use an int for the switch

    // psErrorCode err = psErrorCodeLast ();
    int err = psErrorCodeLast ();
    switch (err) {
      case PS_ERR_NONE:
        return PS_EXIT_SUCCESS;
      case PSPHOT_ERR_SYS:
        return PS_EXIT_SYS_ERROR;
      case PSPHOT_ERR_CONFIG:
        return PS_EXIT_CONFIG_ERROR;
      case PSPHOT_ERR_PROG:
        return PS_EXIT_PROG_ERROR;
      case PSPHOT_ERR_DATA:
        return PS_EXIT_DATA_ERROR;
      default:
        return PS_EXIT_UNKNOWN_ERROR;
    }
    return PS_EXIT_UNKNOWN_ERROR;
}
