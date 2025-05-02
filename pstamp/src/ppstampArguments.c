#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <pslib.h>
#include "ppstamp.h"
#include "ppstampOptions.h"
#include <ctype.h>

static void usage (void)
{
    fprintf(stderr, "\n");

    fprintf(stderr, "USAGE: ppstamp -skycenter RA DEC -pixrange dx dy    [-file INPUT.fits] [-list INPUT.txt] OUTPUT\n");
    fprintf(stderr, "       ppstamp -skycenter RA DEC -arcrange dRA dDEC [-file INPUT.fits] [-list INPUT.txt] OUTPUT\n");
    fprintf(stderr, "       ppstamp -pixcenter x y    -pixrange dx dy    [-class_id class_id] [-file INPUT.fits] [-list INPUT.txt] OUTPUT\n");
    fprintf(stderr, "       ppstamp -pixcenter x y    -arcrange dRA dDEC [-class_id class_id] [-file INPUT.fits] [-list INPUT.txt] OUTPUT\n");
    fprintf(stderr, "\n");

    fprintf(stderr, "-skycenter is specified as HH:MM:SS DD:MM:SS or decimal degrees\n");
    fprintf(stderr, "-arcrange values are seconds of arc\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Optional arguments:\n");
    fprintf(stderr, "   [-class_id class_id]  : selects class_id (only used with -pixcenter)\n");
    fprintf(stderr, "   [-astrom astrom.cmp]  : provide an alternative astrometry calibration\n");
    fprintf(stderr, "   [-mask   mk_image]    : mask image\n");
    fprintf(stderr, "   [-variance var_image] : variance image\n");
    fprintf(stderr, "   [-sources sources]    : sources cmf (ignored for chip stage)\n");
    fprintf(stderr, "   [-forheader mdcfile   : metadata config file contaiing keywords to add to fits headers\n");
    fprintf(stderr, "   [-stage stage]        : stage of input image (raw, chip, warp, stack, diff)\n");
    fprintf(stderr, "   [-write_jpeg]         : write a JPEG  format of the image stamp\n");
    fprintf(stderr, "   [-write_cmf]          : create an output cmf with the sources overlapping the stamp\n");
    fprintf(stderr, "   [-wholefile]          : ignore the region of interest and process the entire input image\n");
    fprintf(stderr, "   [-centeroffchip]          : allow center to be off chip boundary and include any pixels in ROI (testing) \n");
    // fprintf(stderr, "   [-no_censor_masked]   : do not set masked pixels to NAN\n");
    fprintf(stderr, "\n");

    exit (2);
}

pmConfig *ppstampArguments(int argc, char **argv, ppstampOptions **pOptions)
{
    int argnum;                         // Argument number of interest
    bool gotCenter = false;
    bool gotRange = false;
    ppstampOptions *options;

    if (argc == 1) {
        usage();
    }

    if (psArgumentGet (argc, argv, "-version")) {
        psString version;
        version = ppstampVersionLong();   fprintf (stdout, "%s\n", version); psFree (version);
        version = psModulesVersionLong(); fprintf (stdout, "%s\n", version); psFree (version);
        version = psLibVersionLong();     fprintf (stdout, "%s\n", version); psFree (version);
        exit (0);
    }

    // load the site-wide configuration information
    pmConfig *config = pmConfigRead(&argc, argv, NULL);
    if (config == NULL) {
        psError(PSTAMP_ERR_CONFIG, false, "Can't read site configuration!\n");
        return(NULL);
    }

    options = ppstampOptionsAlloc();
    *pOptions = options;

    if ((argnum = psArgumentGet(argc, argv, "-centeroffchip"))) {
        psArgumentRemove(argnum, &argc, argv);
        options->centeroffchip = true;
    }
	
    if ((argnum = psArgumentGet(argc, argv, "-wholefile"))) {
        psArgumentRemove(argnum, &argc, argv);
        gotCenter = true;
        gotRange = true;
        options->wholeFile = true;
    } else {
        if (!pstampGetROI(&options->roip, &argc, argv, &gotCenter, &gotRange)) {
            usage();
        }
    }

    if (!gotCenter) {
        psError(PSTAMP_ERR_ARGUMENTS, true, "must specify center of region of interest\n");
        usage();
    }

    if (!gotRange) {
        psError(PSTAMP_ERR_ARGUMENTS, true, "must specify extent of region of interest\n");
        usage();
    }

    if ((argnum = psArgumentGet(argc, argv, "-class_id"))) {
        psArgumentRemove(argnum, &argc, argv);
        options->chipName = psStringCopy(argv[argnum]);
        psArgumentRemove(argnum, &argc, argv);
    }
    if ((argnum = psArgumentGet(argc, argv, "-stage"))) {
        psArgumentRemove(argnum, &argc, argv);
        options->stage = psStringCopy(argv[argnum]);
        psArgumentRemove(argnum, &argc, argv);
    } else {
        // Should we require this? 
        options->stage = psStringCopy("unknown");
    }
    if ((argnum = psArgumentGet(argc, argv, "-write_jpeg"))) {
        psArgumentRemove(argnum, &argc, argv);
        options->writeJPEG = true;
    }
    if ((argnum = psArgumentGet(argc, argv, "-write_cmf"))) {
        psArgumentRemove(argnum, &argc, argv);
        options->writeCMF = true;
    }

    // XXX: Note: the various list options have never been tested with ppstamp to my knowledge
    bool gotAstrom = false;
    if ((argnum = psArgumentGet(argc, argv, "-astrom"))) {
        gotAstrom = true;
        pmConfigFileSetsMD(config->arguments, &argc, argv, "ASTROM", "-astrom", "-astromlist");
    }
    pmConfigFileSetsMD(config->arguments, &argc, argv, "MASK",   "-mask", "-masklist");
    pmConfigFileSetsMD(config->arguments, &argc, argv, "VARIANCE", "-variance", "-variancelist");

    if ((argnum = psArgumentGet(argc, argv, "-sources"))) {
        pmConfigFileSetsMD(config->arguments, &argc, argv, "SOURCES", "-sources", "-sourceslist");
        // supplying a sources file implies that we want to save the output
        options->writeCMF = true;
    } else if (options->writeCMF && !gotAstrom) {
        // if we didn't get a sources file but the user wanted us to write the sources we must
        // have an astrometry file supplied
        // XXX: Is this too restrictive? Could the sources be contained in say the input file?
        psError(PSTAMP_ERR_ARGUMENTS, true, "cannot write cmf file unless input sources are supplied\n");
        usage();
    }

    // the input image file is a required argument; if not found, we will exit
    bool status = pmConfigFileSetsMD (config->arguments, &argc, argv, "INPUT", "-file", "-list");
    if (!status) {
        psError(PSTAMP_ERR_ARGUMENTS, true, "must specify INPUT\n");
        usage();
    }

    if ((argnum = psArgumentGet(argc, argv, "-forheader"))) {
        psArgumentRemove(argnum, &argc, argv);
        if (argnum >= argc) {
            psError(PSTAMP_ERR_ARGUMENTS, true, "metadata config filename value required with option -forheader");
            usage();
        }

        const char *filename = argv[argnum];
        psArgumentRemove(argnum, &argc, argv);

        unsigned int nFail = 0;
        options->headerAdditions = psMetadataConfigRead(NULL, &nFail, filename, false);
        if (nFail || !options->headerAdditions) {
            psError(PS_ERR_UNKNOWN, false, "failed to read metadata from %s", filename);
            return false;
        }
    }
    
    if ((argnum = psArgumentGet(argc, argv, "-no_censor_masked"))) {
        // this is the default. This is for compatiability
        psArgumentRemove(argnum, &argc, argv);
        options->censorMasked = false;
    }
    if ((argnum = psArgumentGet(argc, argv, "-censor_masked"))) {
        // default changed to not censor allow it to be changed back to true
        psArgumentRemove(argnum, &argc, argv);
        options->censorMasked = true;
    }
    if ((argnum = psArgumentGet(argc, argv, "-nocompress"))) {
        psArgumentRemove(argnum, &argc, argv);
        options->nocompress = true;
    }

    // finally the only argument left must be output outroot
    if (argc < 2) {
        psError(PSTAMP_ERR_ARGUMENTS, true, "must specify OUTPUT\n");
        usage();
    } else if (argc > 2) {
        psError(PSTAMP_ERR_ARGUMENTS, true, "unknown argument(s)\n");
        usage();
    }

    psMetadataAddStr(config->arguments, PS_LIST_TAIL, "OUTPUT", 0, "Name of the output image", argv[1]);

    return config;
}
