# include "addstar.h"
static void help (void);

AddstarClientOptions args_sedstar (int argc, char **argv, AddstarClientOptions options) {
  
  int N;

  /* check for help request */
  if (get_argument (argc, argv, "-help") ||
      get_argument (argc, argv, "-h")) {
    help ();
  }

  // a global used by find_matches_refstars.c (value is 1 except for load2mass)
  NREFSTAR_GROUP = 1;

  /*** check for command line options ***/

  /* basic mode: image, list, refcat */
  options.mode = ADDSTAR_MODE_REFCAT;

  /*** provide additional data ***/ 
  /* restrict to a portion of the sky? (UNUSED) */
  UserPatch.Rmin = 0;
  UserPatch.Rmax= 360;
  UserPatch.Dmin = -90;
  UserPatch.Dmax = +90;
  if ((N = get_argument (argc, argv, "-region"))) {
    remove_argument (N, &argc, argv);
    UserPatch.Rmin = atof (argv[N]);
    remove_argument (N, &argc, argv);
    UserPatch.Rmax = atof (argv[N]);
    remove_argument (N, &argc, argv);
    UserPatch.Dmin = atof (argv[N]);
    remove_argument (N, &argc, argv);
    UserPatch.Dmax = atof (argv[N]);
    remove_argument (N, &argc, argv);
  }

  /* extra error messages */
  VERBOSE = FALSE;
  if ((N = get_argument (argc, argv, "-v"))) {
    VERBOSE = TRUE;
    remove_argument (N, &argc, argv);
  }

  /* extra error messages */
  PLOT = FALSE;
  if ((N = get_argument (argc, argv, "-plot"))) {
    PLOT = TRUE;
    remove_argument (N, &argc, argv);
  }

  /* other defaults */
  options.timeref = 0; 
  options.mosaic = FALSE;
  options.existing_regions = FALSE;
  options.skip_missed = FALSE;
  options.closest = FALSE;
  options.only_match = FALSE;
  options.replace = FALSE;
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

  if (argc != 3) {
    fprintf (stderr, "USAGE: sedstar (sedtable) (outcatalog) [-region Rmin Rmax Dmin Dmax]\n");
    exit (2);
  }
  return (options);
}

static void help () {

  fprintf (stderr, "USAGE\n");
  fprintf (stderr, "  sedstar (SEDtable)");
  fprintf (stderr, "     fit objects to stellar SEDs\n\n");

  fprintf (stderr, "  optional flags:\n");
  fprintf (stderr, "  -region Rmin Rmax Dmin Dmax : sky region for analysis\n");
  fprintf (stderr, "  -v                    	  : verbose mode\n");
  fprintf (stderr, "  -help                 	  : this list\n");
  fprintf (stderr, "  -h                    	  : this list\n\n");
  exit (2);
}
