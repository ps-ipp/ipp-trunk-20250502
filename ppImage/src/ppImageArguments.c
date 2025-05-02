#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "ppImage.h"

static void usage (void) {
    fprintf(stderr, "USAGE: ppImage [-file INPUT.fits] [-list INPUT.txt] OUTPUT\n\n");
    fprintf(stderr, "Optional arguments:\n");
    fprintf(stderr, "\t-stats STATS.mdc: Output statistics into STATS.mdc\n");
    fprintf(stderr, "\t-isfringe: The input image contains fringe data.\n");
    fprintf(stderr, "\t-isdark: The input image contains dark data.\n");
    fprintf(stderr, "\t-usemask MASKVAL: Use this mask value (override recipe).\n");
    fprintf(stderr, "\t-chip CHIPNUM: Only process this chip number.\n");
    fprintf(stderr, "\t-norm VALUE: Divide through by this value when done.\n");
    fprintf(stderr, "\t-normlist file.mdc: normalizations by class_id.\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Input options (single file / file list):\n");
    fprintf(stderr, "\t-noisemap/-noisemaplist: Noise Map image.\n");
    fprintf(stderr, "\t-bias/-biaslist: Bias image.\n");
    fprintf(stderr, "\t-dark/-darklist: Dark image.\n");
    fprintf(stderr, "\t-shutter/-shutterlist: Shutter image.\n");
    fprintf(stderr, "\t-flat/-flatlist: Flat image.\n");
    fprintf(stderr, "\t-mask/-masklist: Mask image.\n");
    fprintf(stderr, "\t-fringe/-fringelist: Fringe image and data.\n");
    fprintf(stderr, "\t-linearity/-linearlist: linearity correction file.\n");
    fprintf(stderr, "\t-newnonlin/-newnonlinlist: non-linearity correction file (v 2023.01).\n");
    fprintf(stderr, "\n");
    exit (2);
}

pmConfig *ppImageArguments(int argc, char **argv)
{
    int argnum;                         // Argument number of interest

    if (argc == 1) {
        usage();
    }

    if (psArgumentGet (argc, argv, "-version")) {
        psString version;
        version = ppImageVersionLong();   fprintf (stdout, "%s\n", version); psFree (version);
        version = ppStatsVersionLong();   fprintf (stdout, "%s\n", version); psFree (version);
        version = psphotVersionLong();    fprintf (stdout, "%s\n", version); psFree (version);
        version = psastroVersionLong();   fprintf (stdout, "%s\n", version); psFree (version);
        version = psModulesVersionLong(); fprintf (stdout, "%s\n", version); psFree (version);
        version = psLibVersionLong();     fprintf (stdout, "%s\n", version); psFree (version);
        exit (0);
    }

    // load the site-wide configuration information
    pmConfig *config = pmConfigRead(&argc, argv, RECIPE_NAME);
    if (config == NULL) {
        psErrorStackPrint(stderr, "Can't find site configuration!\n");
        exit(EXIT_FAILURE);
    }

    // generic arguments (version -- ignored in this case, dumpconfig)
    PS_ARGUMENTS_GENERIC( psphot, config, argc, argv );

    // thread arguments
    PS_ARGUMENTS_THREADS( psphot, config, argc, argv )

    // save the following additional recipe values based on command-line options
    // these options override the PPIMAGE recipe values loaded from recipe files
    psMetadata *options = pmConfigRecipeOptions (config, RECIPE_NAME);

    // save these recipe options until we have loaded the options
    // psMetadata *options = psMetadataAlloc ();
    // psMetadataAddPtr (config->arguments, PS_LIST_TAIL, "PPIMAGE.OPTIONS",  PS_DATA_METADATA, "", options);

    if ((argnum = psArgumentGet(argc, argv, "-stats"))) {
        psArgumentRemove(argnum, &argc, argv);
        psMetadataAddStr(config->arguments, PS_LIST_TAIL, "STATS", PS_META_REPLACE,
                         "Filename for summary statistics", argv[argnum]);
        psArgumentRemove(argnum, &argc, argv);
    }

    if ((argnum = psArgumentGet(argc, argv, "-isfringe"))) {
        psArgumentRemove(argnum, &argc, argv);
        psMetadataAddBool(config->arguments, PS_LIST_TAIL, "INPUT_IS_FRINGE", PS_META_REPLACE,
                          "Input is fringe image", true);
    }
    if ((argnum = psArgumentGet(argc, argv, "-isdark"))) {
        psArgumentRemove(argnum, &argc, argv);
        psMetadataAddBool(config->arguments, PS_LIST_TAIL, "INPUT_IS_DARK", PS_META_REPLACE,
                          "Input is dark image", true);
    }

    if ((argnum = psArgumentGet(argc, argv, "-visual"))) {
        psArgumentRemove(argnum, &argc, argv);
        pmVisualSetVisual(true);
    }

    // the following options override the PPIMAGE recipe options

    // recipe option: -usemask : override MASK setting in recipe
    if ((argnum = psArgumentGet(argc, argv, "-usemask"))) {
        psArgumentRemove(argnum, &argc, argv);
        psMetadataAddBool(options, PS_LIST_TAIL, "MASK", PS_META_REPLACE, "", true);
        psArgumentRemove(argnum, &argc, argv);
    }

    // XXX add other PPIMAGE recipe options here

    // the input file is a required argument; if not found, we will exit
    pmConfigFileSetsMD (config->arguments, &argc, argv, "INPUT", "-file", "-list");

    // if these command-line options are supplied, load the file name lists into config->arguments
    // override any configuration-specified source for these files
    pmConfigFileSetsMD (config->arguments, &argc, argv, "NOISEMAP", "-noisemap", "-noisemaplist");
    pmConfigFileSetsMD (config->arguments, &argc, argv, "BIAS", "-bias", "-biaslist");
    pmConfigFileSetsMD (config->arguments, &argc, argv, "DARK", "-dark", "-darklist");
    pmConfigFileSetsMD (config->arguments, &argc, argv, "SHUTTER", "-shutter", "-shutterlist");
    pmConfigFileSetsMD (config->arguments, &argc, argv, "FLAT", "-flat", "-flatlist");
    pmConfigFileSetsMD (config->arguments, &argc, argv, "MASK", "-mask", "-masklist");
    pmConfigFileSetsMD (config->arguments, &argc, argv, "FRINGE", "-fringe", "-fringelist");
    pmConfigFileSetsMD (config->arguments, &argc, argv, "LINEARITY", "-linearity", "-linearlist");
    pmConfigFileSetsMD (config->arguments, &argc, argv, "NEWNONLIN", "-newnonlin", "-newnonlinlist");
    pmConfigFileSetsMD (config->arguments, &argc, argv, "PATTERN.ROW.AMP", "-pattern-row-amplitude", "-not-defined");
    pmConfigFileSetsMD (config->arguments, &argc, argv, "PATTERN.DEAD.CELLS", "-pattern-dead-cells", "-not-defined");

    if ((argnum = psArgumentGet(argc, argv, "-burntool"))) {
        psArgumentRemove(argnum, &argc, argv);
        psMetadataAddStr(config->arguments, PS_LIST_TAIL, "BURNTOOL.TABLE", PS_META_REPLACE, "", argv[argnum]);
        psArgumentRemove(argnum, &argc, argv);
    }

    // chip selection is used to limit chips to be processed
    if ((argnum = psArgumentGet (argc, argv, "-chip"))) {
        psArgumentRemove (argnum, &argc, argv);
        psMetadataAddStr (config->arguments, PS_LIST_TAIL, "CHIP_SELECTIONS", PS_DATA_STRING, "", argv[argnum]);
        psArgumentRemove (argnum, &argc, argv);
    }

    // Optional normalization factor
    if ((argnum = psArgumentGet(argc, argv, "-norm"))) {
        psArgumentRemove(argnum, &argc, argv);
        float norm = atof(argv[argnum]);
        psMetadataAddF32(config->arguments, PS_LIST_TAIL, "NORMALIZATION", 0,
                         "Normalisation to apply", norm);
        psArgumentRemove(argnum, &argc, argv);
    }

    // Optional per-class normalization table
    if ((argnum = psArgumentGet(argc, argv, "-normlist"))) {
        psArgumentRemove(argnum, &argc, argv);

        unsigned int nFail = 0;
        psMetadata *normlist = psMetadataConfigRead (NULL, &nFail, argv[argnum], false);
        // XXX allow this file to be in nebulous?

        psMetadataAddMetadata(config->arguments, PS_LIST_TAIL, "NORMALIZATION.TABLE", 0, "Normalization to apply", normlist);
        psFree (normlist);
        psArgumentRemove(argnum, &argc, argv);
    }

    if (argc != 2) usage ();

    // Add the input and output images (which remain on the command-line) to the arguments list
    psMetadataAddStr(config->arguments, PS_LIST_TAIL, "OUTPUT", 0, "Name of the output image", argv[1]);

    return config;
}
