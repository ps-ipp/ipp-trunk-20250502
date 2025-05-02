#include "ppNoiseMap.h"

static void usage (void) {
    fprintf(stderr, "USAGE: ppNoiseMap [-file INPUT.fits] [-list INPUT.txt] OUTPUT\n\n");
    fprintf(stderr, "\n");
    exit (2);
}

pmConfig *ppNoiseMapArguments(int argc, char **argv)
{
    if (argc == 1) {
        usage();
    }

    if (psArgumentGet (argc, argv, "-version")) {
        psString version;
        version = ppNoiseMapVersionLong(); fprintf (stdout, "%s\n", version); psFree (version);
        version = psModulesVersionLong();  fprintf (stdout, "%s\n", version); psFree (version);
        version = psLibVersionLong();      fprintf (stdout, "%s\n", version); psFree (version);
        exit (0);
    }

    // load the site-wide configuration information
    pmConfig *config = pmConfigRead(&argc, argv, RECIPE_NAME);
    if (config == NULL) {
        psErrorStackPrint(stderr, "Can't find site configuration!\n");
        exit(EXIT_FAILURE);
    }

    // the input file is a required argument; if not found, we will exit
    pmConfigFileSetsMD (config->arguments, &argc, argv, "INPUT", "-file", "-list");

    if (argc != 2) usage ();

    // Add the input and output images (which remain on the command-line) to the arguments list
    psMetadataAddStr(config->arguments, PS_LIST_TAIL, "OUTPUT", 0, "Name of the output image", argv[1]);

    return config;
}
