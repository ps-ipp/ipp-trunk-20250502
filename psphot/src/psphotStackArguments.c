# include "psphotStandAlone.h"

static void dumpTemplate(void);
static void usage(pmConfig *config, int exitCode);
static void writeHelpInfo(FILE* ofile);

pmConfig *psphotStackArguments(int argc, char **argv) {

    int N;

    // print help info
    if (psArgumentGet(argc, argv, "-help")) writeHelpInfo(stdout);
    if (psArgumentGet(argc, argv, "-h")) writeHelpInfo(stdout);

    if (psArgumentGet(argc, argv, "-template")) dumpTemplate();

    // load config data from default locations
    pmConfig *config = pmConfigRead(&argc, argv, PSPHOT_RECIPE);
    if (config == NULL) {
      psErrorStackPrint(stderr, "Can't read site configuration");
	exit(PS_EXIT_CONFIG_ERROR);
    }

    // generic arguments (version, dumpconfig)
    PS_ARGUMENTS_GENERIC( psphot, config, argc, argv );

    // thread arguments
    PS_ARGUMENTS_THREADS( psphot, config, argc, argv )

    // save the following additional recipe values based on command-line options
    // these options override the PSPHOT recipe values loaded from recipe files
    psMetadata *options = pmConfigRecipeOptions (config, PSPHOT_RECIPE);

    bool updateMode = false;
    if ((N = psArgumentGet (argc, argv, "-updatemode"))) {
        psArgumentRemove (N, &argc, argv);
        updateMode = true;
    }
    psMetadataAddBool(config->arguments, PS_LIST_TAIL, "PSPHOT.STACK.UPDATEMODE", 0, "update mode flag", updateMode);

    // visual : interactive display mode
    if ((N = psArgumentGet (argc, argv, "-visual"))) {
        psArgumentRemove (N, &argc, argv);
        pmVisualSetVisual(true);
    }

    // memdump : enable memory spot checks
    if ((N = psArgumentGet (argc, argv, "-memdump"))) {
        psArgumentRemove (N, &argc, argv);
        psMemDumpSetState(true);
    }

    // break : used from recipe throughout psphotReadout to stop processing early
    if ((N = psArgumentGet (argc, argv, "-break"))) {
	if (argc <= N+1) {
	  psErrorStackPrint(stderr, "Expected to see an argument for -break");
	  exit(PS_EXIT_CONFIG_ERROR);
	}
        psArgumentRemove (N, &argc, argv);
        psMetadataAddStr (options, PS_LIST_TAIL, "BREAK_POINT", PS_META_REPLACE, "", argv[N]);
        psArgumentRemove (N, &argc, argv);
    }
    if ((N = psArgumentGet (argc, argv, "-ds9regions"))) {
        psArgumentRemove (N, &argc, argv);
        pmSubtractionRegions(true);
    }

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

    if (argc != 2) {
        psError(PSPHOT_ERR_ARGUMENTS, true, "Expected to see one more argument; saw %d", argc - 1);
	usage(config, PS_EXIT_CONFIG_ERROR);
    }

    // output position is fixed
    psMetadataAddStr (config->arguments, PS_LIST_TAIL, "OUTPUT", 0, "", argv[1]);

    psTrace("psphot", 1, "Done with psphotArguments...\n");
    return (config);
}

static void writeHelpInfo(FILE* ofile)
{
  fprintf(ofile,
	  "Usage: psphotStack -input (INPUTS.mdc) (OUTROOT)\n"
	  "\n"
	  "where INPUTS.mdc contains various METADATAs, each with:\n"
	  "\tIMAGE    : Image filename\n"
	  "\tMASK     : Mask filename\n"
	  "\tVARIANCE : Variance map filename\n"
	  "(use -template to generate a sample input.mdc file)\n"
	  "OUTROOT is the 'root name' for output files\n"
	  "\n"
	  "additional options:\n"
	  "  -chip nn[,nn,...]\n"
	  "     select detector chips to process; default is all.\n"
	  "     Indices correspond to zero-based offset in the FPA metadata table.\n"
	  "  -photcode PhotoCodeName\n"
	  "     specify photocode\n"
	  "  -region RegionString\n"
	  "     specify analysis region.  String is of form '[x0:x1,y0:y1]'\n"
	  "     To use this option you must define a default in psphot.config\n"
	  "  -visual\n"
	  "     turns on interactive display mode\n"
	  "  -dumpconfig CfgFileName\n"
          "     causes config info to be dumped to the named file.\n"
	  "  -break NOTHING|BACKMDL|PEAKS|MOMENTS|PSFMODEL|ENSEMBLE|PASS1\n"
	  "     choose a point at which to exit processing early\n"
	  "  -nthreads n\n"
	  "     set number of parallel threads of execution\n"
	  "  -F OldFileRule ReplacementFileRule\n"
	  "     change file naming rule; e.g. '-F PSPHOT.OUTPUT PSPHOT.OUT.CMF.MEF'\n"
	  "  -D name stringval\n"
	  "     set a string-valued config parameter\n"
	  "  -Di name intval\n"
	  "     set an integer-valued config parameter\n"
	  "  -Df name fval\n"
	  "     set a float-valued config parameter\n"
	  "  -Db name boolval\n"
	  "     set a boolean-valued config parameter\n"
	  "  -v, -vv, -vvv\n"
	  "     set increasing levels of verbosity\n"
	  "  -logfmt FormatString\n"
	  "     set format string used for log messages\n"
	  "  -trace Fac Lvl\n"
	  "     set tracing for facility Fac to integer Lvl, e.g. '-trace err 10'\n"
	  "  -trace-levels\n"
	  "     print current trace levels\n");
    psLibFinalize();
    exit(PS_EXIT_SUCCESS);
}

static void usage(pmConfig *config,      // Configuration
		  int exitCode
		  ) 
{
    fprintf(stderr, 
	    "Usage: psphotStack -input input.mdc outroot\n"
	    "Try 'psphotStack -help' for more options and explanation\n");

    if (exitCode != PS_EXIT_SUCCESS) psErrorStackPrint(stderr, "Error reading arguments\n");

    psFree(config);
    pmConfigDone();
    psLibFinalize();
    exit(exitCode);
}

static void dumpTemplate(void) {

    fprintf (stdout, "# this line is required for multiple INPUT blocks to be accepted\n");
    fprintf (stdout, "INPUT MULTI\n\n");

    fprintf (stdout, "# copy and repeat the following block as needed (one per input image set)\n");
    fprintf (stdout, "# RAW (unconvolved) and CNV (convolved) input images are required\n");
    fprintf (stdout, "# RAW is used for detection and by default for measurements,\n");
    fprintf (stdout, "# CNV is convolved (further) to match target PSF and is used for radial aperture measurements\n");
    fprintf (stdout, "# and if PSPHOT.STACK.USE.RAW = F for measurements\n");
    fprintf (stdout, "# if MASK or VARIANCE images are not supplied, they will be generated\n");
    fprintf (stdout, "# EXPNUM images are optional. If supplied they are uesd to set the value of NFRAMES\n");
    fprintf (stdout, "# PSF may be supplied for the convolution target\n");
    fprintf (stdout, "INPUT METADATA\n");
    fprintf (stdout, "  STACK_ID      S64   123456789      # stack_id for the input image\n");
    fprintf (stdout, "  RAW:IMAGE     STR   file.im.fits   # signal image filename\n");
    fprintf (stdout, "  RAW:MASK      STR   file.mk.fits   # mask image filename\n");
    fprintf (stdout, "  RAW:VARIANCE  STR   file.wt.fits   # variance image filename\n");
    fprintf (stdout, "  RAW:EXPNUM    STR   file.num.fits  # exposure number image filename\n");
    fprintf (stdout, "  RAW:PSF       STR   file.psf.fits  # psf from input unconvolved image\n");

    fprintf (stdout, "  CNV:IMAGE     STR   file.im.fits   # signal image filename\n");
    fprintf (stdout, "  CNV:MASK      STR   file.mk.fits   # mask image filename\n");
    fprintf (stdout, "  CNV:VARIANCE  STR   file.wt.fits   # variance image filename\n");
    fprintf (stdout, "  CNV:EXPNUM    STR   file.num.fits  # exposure number image filename\n");
    fprintf (stdout, "  CNV:PSF       STR   file.psf.fits  # psf from input convolved image\n");

    fprintf (stdout, "  SOURCES       STR   file.cmf       # measured source positions\n");
    fprintf (stdout, "END\n");

    psLibFinalize();
    exit(PS_EXIT_SUCCESS);
}
