# include "psphotStandAlone.h"

static void usage(const char *program, psMetadata *arg, pmConfig *config, int exitCode);
static void writeHelpInfo(const char* program, pmConfig* config, FILE* ofile);

pmConfig *psphotFullForceArguments(int argc, char **argv) {

    int N;
    bool status, status1, status2, status3;

    // load config data from default locations
    pmConfig *config = pmConfigRead(&argc, argv, PSPHOT_RECIPE);
    if (config == NULL) {
      psErrorStackPrint(stderr, "Can't read site configuration");
	exit(PS_EXIT_CONFIG_ERROR);
    }

    PS_ARGUMENTS_GENERIC (psphot, config, argc, argv);

    // save the following additional recipe values based on command-line options
    // these options override the PSPHOT recipe values loaded from recipe files
    psMetadata *options = pmConfigRecipeOptions (config, PSPHOT_RECIPE);

    // Number of threads is handled
    PS_ARGUMENTS_THREADS( psphot, config, argc, argv );

    if (psArgumentGet(argc, argv, "-help")) writeHelpInfo(argv[0], config, stdout);
    if (psArgumentGet(argc, argv, "-h"))    writeHelpInfo(argv[0], config, stdout);
      
    // visual : interactive display mode
    if ((N = psArgumentGet (argc, argv, "-visual"))) {
        psArgumentRemove (N, &argc, argv);
        pmVisualSetVisual(true);
    }

    // break : used from recipe throughout psphotReadout
    if ((N = psArgumentGet (argc, argv, "-break"))) {
	if (argc<=N+1) {
	  psErrorStackPrint(stderr, "Expected to see 1 more argument; saw %d", argc - 1);
	  exit(PS_EXIT_CONFIG_ERROR);
	}
        psArgumentRemove (N, &argc, argv);
        psMetadataAddStr (options, PS_LIST_TAIL, "BREAK_POINT", PS_META_REPLACE, "", argv[N]);
        psArgumentRemove (N, &argc, argv);
    }

    // analysis region : overrides recipe value, used in psphotReadout/psphotEnsemblePSF
    if ((N = psArgumentGet (argc, argv, "-region"))) {
	if (argc<=N+1) {
	  psErrorStackPrint(stderr, "Expected to see 1 more argument; saw %d", argc - 1);
	  exit(PS_EXIT_CONFIG_ERROR);
	}
        psArgumentRemove (N, &argc, argv);
        psMetadataAddStr (options, PS_LIST_TAIL, "ANALYSIS_REGION", 0, "", argv[N]);
        psArgumentRemove (N, &argc, argv);
    }

    // chip selection is used to limit chips to be processed
    if ((N = psArgumentGet (argc, argv, "-chip"))) {
	if (argc<=N+1) {
	  psErrorStackPrint(stderr, "Expected to see 1 more argument; saw %d", argc - 1);
	  exit(PS_EXIT_CONFIG_ERROR);
	}
        psArgumentRemove (N, &argc, argv);
        psMetadataAddStr (config->arguments, PS_LIST_TAIL, "CHIP_SELECTIONS", PS_DATA_STRING, "", argv[N]);
        psArgumentRemove (N, &argc, argv);
    }

    if ((N = psArgumentGet(argc, argv, "-stats"))) {
        psArgumentRemove(N, &argc, argv);
        psMetadataAddStr(config->arguments, PS_LIST_TAIL, "STATS", PS_DATA_STRING,
                         "Filename for statistics of output file", argv[N]);
        psArgumentRemove(N, &argc, argv);
    }

    // if these command-line options are supplied, load the file name lists into config->arguments
    // override any configuration-specified source for these files
    //
    pmConfigFileSetsMD (config->arguments, &argc, argv, "MASK",       "-mask",     "-masklist");
    pmConfigFileSetsMD (config->arguments, &argc, argv, "VARIANCE",   "-variance", "-variancelist");

    pmConfigFileSetsMD (config->arguments, &argc, argv, "PSPHOT.PSF", "-psf",      "-psflist");

    status1 = pmConfigFileSetsMD (config->arguments, &argc, argv, "SRC",     "-src",     "-srclist");
    status2 = pmConfigFileSetsMD (config->arguments, &argc, argv, "SRCTEXT", "-srctext", "-srctextlist");
    status3 = pmConfigFileSetsMD (config->arguments, &argc, argv, "FORCE",   "-force",   "-forcelist");
    
    if (!(status1 || status2 || status3)) {
      // XXX require -force version?
        psError(PSPHOT_ERR_ARGUMENTS, true, "No source list is supplied (use one of -force, -forcelist, -src, -srctext, -srclist, or -srctextlist)");
	usage(argv[0], config->arguments, config, PS_EXIT_CONFIG_ERROR);
    }

    if (argc == 1) {
        psError(PSPHOT_ERR_ARGUMENTS, true, "Too few arguments: %d", argc);
	usage(argv[0], config->arguments, config, PS_EXIT_CONFIG_ERROR);
    }

    // the input file is a required argument; if not found, we will exit
    status = pmConfigFileSetsMD (config->arguments, &argc, argv, "INPUT", "-file", "-list");
    if (!status) {
        psError(PSPHOT_ERR_ARGUMENTS, false, "pmConfigFileSetsMD failed to parse arguments");
	usage(argv[0], config->arguments, config, PS_EXIT_CONFIG_ERROR);
    }

    if (argc != 2) {
        psError(PSPHOT_ERR_ARGUMENTS, true, "Expected to see one more argument; saw %d", argc - 1);
	usage(argv[0], config->arguments, config, PS_EXIT_CONFIG_ERROR);
    }

    // output position is fixed
    psMetadataAddStr (config->arguments, PS_LIST_TAIL, "OUTPUT", 0, "", argv[1]);

    psTrace("psphot", 1, "Done with psphotFullForceArguments...\n");
    return (config);
}

static void writeHelpInfo(const char* program, pmConfig* config, FILE* ofile)
{
  fprintf(ofile,
	  "Usage: one of the following\n"
	  "%s -file fname1[,fname2,...] -mask maskfile1[,maskfile2,...]\n"
	  "     -variance varfile1[,varfile2,...] OutFileBaseName\n"
	  "\n"
	  "%s -list FileNameList [-masklist MaskFileNameList] \n"
	  "     -variancelist VarFileNameList OutFileBaseName\n"
	  "\n"
	  "%s -help\n"
	  "\n"
	  "%s -version\n"
	  "\n"
	  "where:\n"
	  "  FileNameList is a text file containing filenames, one per line\n"
	  "  MaskFileNameList is a text file of mask filenames, one per line\n"
	  "  VarFileNameList is a text file of variance filenames, one per line\n"
	  "  OutFileBaseName is the 'root name' for output files\n"
	  "also required:\n"
	  "  -src SrcFile1[,SrcFile2,...] or -srclist SrcFileNameList\n"
	  "     specify sources to be measured (required)\n"
	  "\n"
	  "additional options:\n"
	  "  -psf PsfFile1[,PsfFile2,...] or -psflist PsfFileNameList\n"
	  "     specify PSF rather than letting %s estimate it\n"
	  "  -chip nn[,nn,...]\n"
	  "     select detector chips to process; default is all.\n"
	  "     Indices correspond to zero-based offset in the FPA metadata table.\n"
	  "  -region RegionString\n"
	  "     specify analysis region.  String is of form '[x0:x1,y0:y1]'\n"
	  "     To use this option you must define a default in psphot.config\n"
	  "  -visual\n"
	  "     turns on interactive display mode\n"
	  "  -dumpconfig CfgFileName\n"
          "     causes config info to be dumped to the named file.\n"
	  "  -break NOTHING|BACKMDL|PEAKS|MOMENTS|PSFMODEL|ENSEMBLE|PASS1\n"
	  "     choose a point at which to exit processing early\n"
	  "  -threads n\n"
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
	  "     print current trace levels\n",
	  program,program,program,program,program);
    psFree(config);
    pmConfigDone();
    psLibFinalize();
    exit(PS_EXIT_SUCCESS);
}

static void usage(const char *program,  // Name of the program
                  psMetadata *arguments, // Command-line arguments
                  pmConfig *config,      // Configuration
		  int exitCode
		  ) 
{
  fprintf(stderr,
	  "Usage: one of the following\n"
	  "%s -file fname1[,fname2,...] -mask maskfile1[,maskfile2,...]\n"
	  "     -variance varfile1[,varfile2,...] OutFileBaseName\n"
	  "\n"
	  "%s -list FileNameList [-masklist MaskFileNameList] \n"
	  "     -variancelist VarFileNameList OutFileBaseName\n"
	  "also required:\n"
	  "  -src SrcFile1[,SrcFile2,...] or -srclist SrcFileNameList\n"
	  "     specify sources to be measured (required)\n"
	  "\n"
	  "Try '%s -help' for more options and explanation\n",
	  program,program,program);
    if (exitCode != PS_EXIT_SUCCESS)
      psErrorStackPrint(stderr, "Error reading arguments\n");
    psFree(config);
    pmConfigDone();
    psLibFinalize();
    exit(exitCode);
}

