/** @file pswarpArguments.c
 *
 *  @brief
 *
 *  @ingroup pswarp
 *
 *  @author IfA
 *  @version $Revision: 1.24 $ $Name: not supported by cvs2svn $
 *  @date $Date: 2009-02-06 03:10:36 $
 *  Copyright 2009 Institute for Astronomy, University of Hawaii
 */

# include "pswarp.h"

static void usage (void) {
    fprintf(stderr, "USAGE: pswarp [-input input.mdc] [-file image(s)] [-list imagelist] [options] (output) (skycell)\n");
    fprintf(stderr, "  options:\n");
    fprintf(stderr, "    [-input input.mdc] : input image information in a metadata file\n");
    fprintf(stderr, "    [-file input.fits[,input.fits]] : input image to be warped\n");
    fprintf(stderr, "    [-astrom astrom.cmp] : provide an alternative astrometry calibration\n");
    fprintf(stderr, "    [-mask mask.fits] : provide a corresponding mask image\n");
    fprintf(stderr, "    [-variance variance.fits] : provide a corresponding variance image\n");
    psErrorStackPrint(stderr, "\n");
    exit(PS_EXIT_CONFIG_ERROR);
}

pmConfig *pswarpArguments (int argc, char **argv) {

    int N;

    if (argc == 1) {
        usage();
    }

    // load config data from default locations
    pmConfig *config = pmConfigRead(&argc, argv, PSWARP_RECIPE);
    if (!config) {
        psError(psErrorCodeLast(), false, "Can't read configuration");
        return NULL;
    }

    // generic arguments (version, dumpconfig)
    PS_ARGUMENTS_GENERIC( pswarp, config, argc, argv );

    // thread arguments
    PS_ARGUMENTS_THREADS( pswarp, config, argc, argv )

    // save the following additional recipe values based on command-line options
    // these options override the PSWARP recipe values loaded from recipe files
    if (!pmConfigRecipeOptions(config, PSWARP_RECIPE)) {
        psError(psErrorCodeLast(), false, "Can't do something with recipes");
        psFree(config);
        return NULL;
    }

    // XXX move to the single group below?
    pmConfigFileSetsMD(config->arguments, &argc, argv, "ASTROM",   "-astrom", "-astromlist");

    // turn on psphot visualization
    if ((N = psArgumentGet(argc, argv, "-psphot-visual"))) {
	psArgumentRemove(N, &argc, argv);
	pmVisualSetVisual(true);
    }

    // chip selection is used to limit chips to be processed
    if ((N = psArgumentGet(argc, argv, "-chip"))) {
        psArgumentRemove(N, &argc, argv);
        psMetadataAddStr(config->arguments, PS_LIST_TAIL, "CHIP_SELECTIONS", PS_DATA_STRING,
                         "Only process these chips", argv[N]);
        psArgumentRemove(N, &argc, argv);
    }

    // Statistics file
    if ((N = psArgumentGet(argc, argv, "-stats"))) {
        psArgumentRemove(N, &argc, argv);
        psMetadataAddStr(config->arguments, PS_LIST_TAIL, "STATS", PS_DATA_STRING,
                         "Filename for statistics of output image", argv[N]);
        psArgumentRemove(N, &argc, argv);
    }

    pswarpSetThreads();

    // there are three mutually exclusive ways of providing the input
    // 1) supply -file (filename) [-mask .. -variance ..] on the command line
    // 2) supply -input (input.mdc) on the command line
    // 3) load inputs from RUN config info

    // below, we check first for -file then for -input.  failure to find either implies use of
    // the configuration metadata file.

    bool singleInput = pmConfigFileSetsMD (config->arguments, &argc, argv, "INPUT", "-file", "-list");
    if (singleInput) {
	if (psArgumentGet(argc, argv, "-input")) {
	    psErrorStackPrint(stderr, "error in arguments : -input and -file / -list are mutually exclusive");
	    exit(PS_EXIT_CONFIG_ERROR);
	}	
	pmConfigFileSetsMD (config->arguments, &argc, argv, "MASK", "-mask", "-masklist");
	pmConfigFileSetsMD (config->arguments, &argc, argv, "VARIANCE", "-variance", "-variancelist");
	pmConfigFileSetsMD (config->arguments, &argc, argv, "BACKGROUND", "-background", "-bkglist");
    } else {
	// find the input data file (an mdc file)
	if ((N = psArgumentGet(argc, argv, "-input"))) {
	    if (argc <= N+1) {
		psErrorStackPrint(stderr, "Expected to see 1 more argument; saw %d", argc - 1);
		exit(PS_EXIT_CONFIG_ERROR);
	    }
	    psArgumentRemove(N, &argc, argv);

	    unsigned int numBad = 0;                     // Number of bad lines
	    psMetadata *inputs = psMetadataConfigRead(NULL, &numBad, argv[N], false); // Input file info
	    if (!inputs || numBad > 0) {
		psErrorStackPrint(stderr, "Unable to cleanly read MDC file with inputs.");
		exit(PS_EXIT_CONFIG_ERROR);
	    }
	    psMetadataAddMetadata(config->arguments, PS_LIST_TAIL, "INPUTS", 0, "Metadata with input details", inputs);
	    psFree(inputs);

	    psArgumentRemove(N, &argc, argv);
	}
    }
    if (argc != 3) {
	usage();
    }
    if (psErrorCodeLast() != PS_ERR_NONE) {
	psErrorStackPrint(stderr, "error in arguments");
	exit(PS_EXIT_CONFIG_ERROR);
    }
    
    psArray *array;

    // output position is fixed
    psMetadataAddStr(config->arguments, PS_LIST_TAIL, "OUTPUT", 0, "", argv[1]);

    // skycell position is fixed
    array = psArrayAlloc(1);
    array->data[0] = psStringCopy(argv[2]);
    psMetadataAddPtr(config->arguments, PS_LIST_TAIL, "SKYCELL", PS_DATA_ARRAY, "", array);
    psFree(array);

    psTrace("pswarp", 1, "Done with pswarpArguments...\n");
    return config;
}
