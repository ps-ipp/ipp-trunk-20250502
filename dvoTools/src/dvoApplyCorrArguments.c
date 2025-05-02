#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "dvoApplyCorr.h"

static void usage (void) {
    fprintf(stderr, "USAGE: dvoApplyCorr -file INPUT.fits OUTPUT [-corr CORR.fits]\n\n");
    exit (2);
}

pmConfig *dvoApplyCorrArguments(int argc, char **argv)
{
    int argnum;

    if (argc == 1) {
        usage();
    }

    if (psArgumentGet (argc, argv, "-version")) {
	psString version;
	version = dvoApplyCorrVersionLong(); fprintf (stdout, "%s\n", version); psFree (version);
	version = psModulesVersionLong();   fprintf (stdout, "%s\n", version); psFree (version);
	version = psLibVersionLong();       fprintf (stdout, "%s\n", version); psFree (version);
	exit (0);
    }

    // load the site-wide configuration information
    pmConfig *config = pmConfigRead(&argc, argv, RECIPE_NAME);
    if (config == NULL) {
        psErrorStackPrint(stderr, "Can't find site configuration!\n");
        exit(EXIT_FAILURE);
    }

    // save the following additional recipe values based on command-line options
    // these options override the DVOCORR recipe values loaded from recipe files
    // psMetadata *options = pmConfigRecipeOptions (config, RECIPE_NAME);

    // XXX add options from command-line here

    // drop the local view on the options (saved on config->arguments)
    // psFree (options);

    // chip selection is used to limit chips to be processed
    if ((argnum = psArgumentGet (argc, argv, "-chip"))) {
        psArgumentRemove (argnum, &argc, argv);
        psMetadataAddStr (config->arguments, PS_LIST_TAIL, "CHIP_SELECTIONS", PS_DATA_STRING, "",
                          argv[argnum]);
        psArgumentRemove (argnum, &argc, argv);
    }

    // the input file is a required argument; if not found, we will exit
    bool status = pmConfigFileSetsMD (config->arguments, &argc, argv, "INPUT", "-file", "-list");
    if (!status) {
        usage ();
    }

    pmConfigFileSetsMD (config->arguments, &argc, argv, "CORR", "-corr", "-corrlist");

    if (argc != 2) usage ();

    // Add the output image (which remain on the command-line) to the arguments list
    psMetadataAddStr(config->arguments, PS_LIST_TAIL, "OUTPUT", 0, "Name of the output image", argv[1]);

    return config;
}
