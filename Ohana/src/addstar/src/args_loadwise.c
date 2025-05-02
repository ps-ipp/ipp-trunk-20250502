# include "addstar.h"
# include "WISE.h"
static void help (void);

AddstarClientOptions args_loadwise (int *argc, char **argv, AddstarClientOptions options) {
  
  int N;

  /* check for help request */
  if (get_argument (*argc, argv, "-help") ||
      get_argument (*argc, argv, "-h")) {
    help ();
  }

  // a global used by find_matches_refstars.c (value is 1 except for loadwise & loadwise)
  NREFSTAR_GROUP = 4;

  /*** check for command line options ***/

  /* basic mode: image, list, refcat */
  options.mode = ADDSTAR_MODE_REFCAT;

  /* we do not allow a subset to be extracted -- all or nothing, babe */
  UserPatch.Rmin = 0;
  UserPatch.Rmax= 360;
  UserPatch.Dmin = -90;
  UserPatch.Dmax = +90;

  /* only add to existing regions */
  options.existing_regions = FALSE;
  if ((N = get_argument (*argc, argv, "-existing-regions"))) {
    options.existing_regions = TRUE;
    remove_argument (N, argc, argv);
  }
  /* only add to existing objects */
  options.only_match = FALSE;
  if ((N = get_argument (*argc, argv, "-only-match"))) {
    options.only_match = TRUE;
    remove_argument (N, argc, argv);
  }
  /* replace measurement, don't duplicate (ref/cat only) */
  options.replace = FALSE;
  if ((N = get_argument (*argc, argv, "-replace"))) {
    options.replace = TRUE;
    remove_argument (N, argc, argv);
  }

  /* extra error messages */
  VERBOSE = FALSE;
  if ((N = get_argument (*argc, argv, "-v"))) {
    VERBOSE = TRUE;
    remove_argument (N, argc, argv);
  }

  /* accept proper-motion data from reference */
  ACCEPT_MOTION = FALSE;
  if ((N = get_argument (*argc, argv, "-accept-motion"))) {
    ACCEPT_MOTION = TRUE;
    remove_argument (N, argc, argv);
  }

  /* accept proper-motion data from reference */
  RA_SYS_OFFRAW = 0.0;
  DE_SYS_OFFSET = 0.0;
  uRA_SYS_OFFSET = 0.0;
  uDE_SYS_OFFSET = 0.0;
  if ((N = get_argument (*argc, argv, "-sys-astrom-offsets"))) {
    if (*argc < N + 5) {
      fprintf (stderr, "missing args for -sys-astrom-offsets (RA) (DEC) (uRA) (uDEC)\n");
      exit (2);
    }
    remove_argument (N, argc, argv);
    RA_SYS_OFFRAW = atof(argv[N]);
    remove_argument (N, argc, argv);
    DE_SYS_OFFSET = atof(argv[N]);
    remove_argument (N, argc, argv);
    uRA_SYS_OFFSET = atof(argv[N]);
    remove_argument (N, argc, argv);
    uDE_SYS_OFFSET = atof(argv[N]);
    remove_argument (N, argc, argv);
  }

  /* load the prelim data dump */
  MODE = MODE_NONE;
  if ((N = get_argument (*argc, argv, "-mode"))) {
    remove_argument (N, argc, argv);
    if (!strcasecmp(argv[N], "catwise")) MODE = MODE_CATWISE;
    if (!strcasecmp(argv[N], "allwise")) MODE = MODE_ALLWISE;
    if (!strcasecmp(argv[N], "allsky"))  MODE = MODE_ALLSKY;
    if (!strcasecmp(argv[N], "prelim"))  MODE = MODE_PRELIM;
    if (MODE == MODE_NONE) {
      fprintf (stderr, "invalid mode: %s\n", argv[N]);
      exit (2);
    }
    remove_argument (N, argc, argv);
  }

  /* other addstar options which cannot be used in loadwise */
  options.photcode = 0;
  options.timeref = 0; 
  options.mosaic = FALSE;
  options.skip_missed = FALSE;
  options.closest = FALSE;
  options.nosort = FALSE;
  options.update = FALSE;
  options.only_images = FALSE;
  options.calibrate = FALSE;
  options.quality_airmass = FALSE;
  ACCEPT_ASTROM = FALSE;
  FORCE_READ = FALSE;
  TEXTMODE = FALSE;
  SUBPIX = FALSE;
  DUMP = NULL;

  if (*argc < 2) {
    fprintf (stderr, "USAGE: loadwise [options] (wisefile) [..more files]\n");
    exit (2);
  }
  return (options);
}

static void help () {

  fprintf (stderr, "USAGE: loadwise [-mode MODE] [options] (wisefile) [..more files]\n");
  fprintf (stderr, "  add data from WISE catalog to DVO\n\n");

  fprintf (stderr, "  MODE may be : catwise, allwise, allsky, prelim\n");

  fprintf (stderr, "  optional flags:\n");
  fprintf (stderr, "  -only-match           	  : only add measurements to existing objects\n");
  fprintf (stderr, "  -replace              	  : replace time/photcode measurements (no duplication)\n");
  fprintf (stderr, "  -v                    	  : verbose mode\n");
  fprintf (stderr, "  -help                 	  : this list\n");
  fprintf (stderr, "  -h                    	  : this list\n\n");
  exit (2);
}
