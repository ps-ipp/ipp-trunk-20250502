#include "ppSmooth.h"

static void usage (void) {
    fprintf(stderr, "USAGE: ppSmooth [-file INPUT.fits] [-mask mask.fits] [-variance var.fits] OUTPUT\n\n");
    fprintf(stderr, " list alternatives: [-list INPUT.txt] [-masklist MASK.txt] [-variancelist VAR.txt]\n");
    fprintf(stderr, "Optional arguments:\n");
    fprintf(stderr, "  -sigma (sigma): smoothing kernel sigma\n");
    fprintf(stderr, "  -chip (chip): limit operation to specified chip\n");
    fprintf(stderr, "  -version : report version info and exit\n");
    fprintf(stderr, "  -help    : this message\n");
    fprintf(stderr, "\n");
    exit (2);
}

pmConfig *ppSmoothArguments(int argc, char **argv)
{
    int argnum;                         // Argument number of interest

    if (argc == 1) usage();

    if (psArgumentGet (argc, argv, "-help")) usage();

    if (psArgumentGet (argc, argv, "-version")) {
        psString version;
        version = ppSmoothVersionLong();   fprintf (stdout, "%s\n", version); psFree (version);
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

    // Number of threads
    if ((argnum = psArgumentGet(argc, argv, "-threads"))) {
        psArgumentRemove(argnum, &argc, argv);
        int nThreads = atoi(argv[argnum]);
        psMetadataAddS32(config->arguments, PS_LIST_TAIL, "NTHREADS", 0, "number of threads", nThreads);
        psArgumentRemove(argnum, &argc, argv);

        // create the thread pool with number of desired threads, supplying our thread launcher function
        // XXX need to determine the number of threads from the config data
        psThreadPoolInit (nThreads);
    }

    // user-override for the smoothing sigma
    if ((argnum = psArgumentGet(argc, argv, "-sigma"))) {
        psArgumentRemove(argnum, &argc, argv);
        psMetadataAddF32(config->arguments, PS_LIST_TAIL, "SIGMA", PS_META_REPLACE, "smoothing sigma", atof(argv[argnum]));
        psArgumentRemove(argnum, &argc, argv);
    }

    // the input file is a required argument; if not found, we will exit
    pmConfigFileSetsMD (config->arguments, &argc, argv, "INPUT", "-file", "-list");
    pmConfigFileSetsMD (config->arguments, &argc, argv, "MASK", "-mask", "-masklist");
    pmConfigFileSetsMD (config->arguments, &argc, argv, "VARIANCE", "-variance", "-variancelist");

    // chip selection is used to limit chips to be processed
    if ((argnum = psArgumentGet (argc, argv, "-chip"))) {
        psArgumentRemove (argnum, &argc, argv);
        psMetadataAddStr (config->arguments, PS_LIST_TAIL, "CHIP_SELECTIONS", PS_DATA_STRING, "", argv[argnum]);
        psArgumentRemove (argnum, &argc, argv);
    }
    //    XXX ADD this in?
    if ((argnum = psArgumentGet(argc, argv, "-dumpconfig"))) {
        psArgumentRemove(argnum, &argc, argv);
        psMetadataAddStr(config->arguments, PS_LIST_TAIL, "DUMP_CONFIG", PS_META_REPLACE,
                         "Filename for configuration dump", argv[argnum]);
        psArgumentRemove(argnum, &argc, argv);
    }

    if (argc != 2) usage ();

    // Add the input and output images (which remain on the command-line) to the arguments list
    psMetadataAddStr(config->arguments, PS_LIST_TAIL, "OUTPUT", 0, "Name of the output image", argv[1]);



    return config;
}


    // XXX ADD this in? 
    // Number of threads
    // if ((argnum = psArgumentGet(argc, argv, "-threads"))) {
    //     psArgumentRemove(argnum, &argc, argv);
    //     int nThreads = atoi(argv[argnum]);
    //     psMetadataAddS32(config->arguments, PS_LIST_TAIL, "NTHREADS", 0, "number of warp threads", nThreads);
    //     psArgumentRemove(argnum, &argc, argv);
    // 
    //     // create the thread pool with number of desired threads, supplying our thread launcher function
    //     // XXX need to determine the number of threads from the config data
    //     psThreadPoolInit (nThreads);
    // }


