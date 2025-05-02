#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "ppImage.h"

static void usage (void) {
    fprintf (stderr, "USAGE: ppFocus [-file focus.*.fits] [-list INPUT.txt] OUTPUT\n");
    exit (2);
}

pmConfig *ppFocusArguments(int argc, char **argv) {

    int N;
    bool status;

    if (argc == 1) usage ();

    // load the site-wide configuration information
    pmConfig *config = pmConfigRead(&argc, argv, RECIPE_NAME);
    if (config == NULL) {
        psErrorStackPrint(stderr, "Can't find site configuration!\n");
        exit(EXIT_FAILURE);
    }

    // save the following additional recipe values based on command-line options
    // these options override the PPIMAGE recipe values loaded from recipe files
    psMetadata *options = pmConfigRecipeOptions (config, RECIPE_NAME);

    // save these recipe options until we have loaded the options
    // psMetadata *options = psMetadataAlloc ();
    // psMetadataAddPtr (config->arguments, PS_LIST_TAIL, "PPIMAGE.OPTIONS",  PS_DATA_METADATA, "", options);

    // the following options override the PPIMAGE recipe options

    // recipe option: -usemask : override MASK setting in phase2.recipe
    if ((N = psArgumentGet(argc, argv, "-usemask"))) {
        psArgumentRemove (N, &argc, argv);
        psMetadataAddBool (options, PS_LIST_TAIL, "MASK", PS_META_REPLACE, "", true);
        psArgumentRemove (N, &argc, argv);
    }

    // XXX add other PPIMAGE recipe options here

    // the input file is a required argument; if not found, we will exit
    status = pmConfigFileSetsMD (config->arguments, &argc, argv, "INPUT", "-file", "-list");
    if (!status) { usage ();}

    // if these command-line options are supplied, load the file name lists into config->arguments
    // override any configuration-specified source for these files
    pmConfigFileSetsMD (config->arguments, &argc, argv, "BIAS", "-bias", "-biaslist");
    pmConfigFileSetsMD (config->arguments, &argc, argv, "DARK", "-dark", "-darklist");
    pmConfigFileSetsMD (config->arguments, &argc, argv, "FLAT", "-flat", "-flatlist");
    pmConfigFileSetsMD (config->arguments, &argc, argv, "MASK", "-mask", "-masklist");
    pmConfigFileSetsMD (config->arguments, &argc, argv, "FRINGE", "-fringe", "-fringelist");

    // chip selection is used to limit chips to be processed
    if ((N = psArgumentGet (argc, argv, "-chip"))) {
        psArgumentRemove (N, &argc, argv);
        psMetadataAddStr (config->arguments, PS_LIST_TAIL, "CHIP_SELECTIONS", PS_DATA_STRING, "",
                          argv[N]);
        psArgumentRemove (N, &argc, argv);
    }

    if (argc != 2) usage ();

    // Add the input and output images (which remain on the command-line) to the arguments list
    psMetadataAddStr(config->arguments, PS_LIST_TAIL, "OUTPUT", 0, "Name of the output image",
                     argv[1]);

    return config;
}
