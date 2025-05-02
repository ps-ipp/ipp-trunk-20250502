# include "fpcamera.h"

# define ESCAPE(ERROR,MSG) { psError(ERROR, true, MSG); psErrorStackPrint(stderr, "exit"); return NULL; }

pmConfig *fpcameraArguments (int argc, char **argv) {

    bool status;
    int N;

    psTimerStart ("complete");		// set an overall timer
    fpcameraErrorRegister();		// register our error codes/messages
    pmModelClassInit();			// model inits are needed in pmSourceIO
    psphotInit();

    if (argc == 1) ESCAPE(FPCAMERA_ERR_ARGUMENTS, "No arguments supplied");

    // load config data from default locations
    pmConfig *config = pmConfigRead(&argc, argv, FPCAMERA_RECIPE);
    if (config == NULL) ESCAPE(FPCAMERA_ERR_CONFIG, "Can't read site configuration");

    // save the following additional recipe values based on command-line options
    // these options override the FPCAMERA recipe values loaded from recipe files
    psMetadata *options = pmConfigRecipeOptions (config, FPCAMERA_RECIPE);

    // photcode : used in output to supplement header data (argument or recipe?)
    if ((N = psArgumentGet (argc, argv, "-photcode"))) {
        psArgumentRemove (N, &argc, argv);
        psMetadataAddStr (options, PS_LIST_TAIL, "PHOTCODE", PS_META_REPLACE, "", argv[N]);
        psArgumentRemove (N, &argc, argv);
    }

    // chip selection is used to limit chips to be processed
    if ((N = psArgumentGet (argc, argv, "-chip"))) {
        psArgumentRemove (N, &argc, argv);
        psMetadataAddStr (config->arguments, PS_LIST_TAIL, "CHIP_SELECTIONS", PS_META_REPLACE, "", argv[N]);
        psArgumentRemove (N, &argc, argv);
    }

    // dump the statistics to a file
    if ((N = psArgumentGet(argc, argv, "-stats"))) {
        psArgumentRemove(N, &argc, argv);
        psMetadataAddStr(config->arguments, PS_LIST_TAIL, "STATS", PS_META_REPLACE, "Filename for summary statistics", argv[N]);
        psArgumentRemove(N, &argc, argv);
    }

    // dump the configuration to a file?
    if ((N = psArgumentGet (argc, argv, "-dumpconfig"))) {
        psArgumentRemove (N, &argc, argv);
        psMetadataAddStr (config->arguments, PS_LIST_TAIL, "DUMP_CONFIG", PS_META_REPLACE, "", argv[N]);
        psArgumentRemove (N, &argc, argv);
    }

    // chip selection is used to limit chips to be processed
    if ((N = psArgumentGet (argc, argv, "-save-resid"))) {
        psArgumentRemove (N, &argc, argv);
        psMetadataAddBool (config->arguments, PS_LIST_TAIL, "SAVE.RESID", PS_META_REPLACE, "", true);
    }

    // specify the input images to process
    status = pmConfigFileSetsMD (config->arguments, &argc, argv, "INPUT",    "-file",     "-list");
    if (!status) ESCAPE(FPCAMERA_ERR_ARGUMENTS, "Missing -file (input) or -list (input)");

    // specify the input images to process
    status = pmConfigFileSetsMD (config->arguments, &argc, argv, "MASK",     "-mask",     "-masklist");
    if (!status) ESCAPE(FPCAMERA_ERR_ARGUMENTS, "Missing -mask (input) or -masklist (input)");

    // specify the input images to process
    status = pmConfigFileSetsMD (config->arguments, &argc, argv, "VARIANCE", "-variance", "-varlist");
    if (!status) ESCAPE(FPCAMERA_ERR_ARGUMENTS, "Missing -variance (input) or -varlist (input)");

    // specify the astrometry calibration
    status = pmConfigFileSetsMD (config->arguments, &argc, argv, "INPUT.ASTROM", "-astrom-file", NULL);
    if (!status) ESCAPE(FPCAMERA_ERR_ARGUMENTS, "Missing -astrom-file (input) : provide an smf or similar");

    // specify the input images to process
    status = pmConfigFileSetsMD (config->arguments, &argc, argv, "INPUT.PSF", "-psf",     "-psflist");
    // user-supplied psf model is optional
    
    if (argc != 2) ESCAPE(FPCAMERA_ERR_ARGUMENTS, "Incorrect number of arguments supplied");

    // output position is fixed
    psMetadataAddStr (config->arguments, PS_LIST_TAIL, "OUTPUT", 0, "", argv[1]);

    psTrace("fpcamera", 1, "Done with fpcameraArguments...\n");
    return (config);
}
