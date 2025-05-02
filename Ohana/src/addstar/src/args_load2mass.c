# include "addstar.h"
static void help (void);

AddstarClientOptions args_load2mass (int argc, char **argv, AddstarClientOptions options) {
  
  int N;

  /* check for help request */
  if (get_argument (argc, argv, "-help") ||
      get_argument (argc, argv, "-h")) {
    help ();
  }

  // a global used by find_matches_refstars.c (value is 1 except for load2mass)
  NREFSTAR_GROUP = 3;

  /*** check for command line options ***/

  /* basic mode: image, list, refcat */
  options.mode = ADDSTAR_MODE_REFCAT;

  /*** provide additional data ***/ 
  /* restrict to a portion of the sky? */
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

  /* only add to existing regions */
  options.existing_regions = FALSE;
  if ((N = get_argument (argc, argv, "-existing-regions"))) {
    options.existing_regions = TRUE;
    remove_argument (N, &argc, argv);
  }
  /* only add to existing objects */
  options.only_match = FALSE;
  if ((N = get_argument (argc, argv, "-only-match"))) {
    options.only_match = TRUE;
    remove_argument (N, &argc, argv);
  }
  /* replace measurement, don't duplicate (ref/cat only) */
  options.replace = FALSE;
  if ((N = get_argument (argc, argv, "-replace"))) {
    options.replace = TRUE;
    remove_argument (N, &argc, argv);
  }

  /* extra error messages */
  VERBOSE = FALSE;
  if ((N = get_argument (argc, argv, "-v"))) {
    VERBOSE = TRUE;
    remove_argument (N, &argc, argv);
  }

  /* other addstar options which cannot be used in load2mass */
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

  if (argc != 1) {
    fprintf (stderr, "USAGE: load2mass\n");
    exit (2);
  }
  return (options);
}

static void help () {

  fprintf (stderr, "USAGE: load2mass [options]\n");
  fprintf (stderr, "  add data from 2MASS catalog to fullsky\n\n");

  fprintf (stderr, "  optional flags:\n");
  fprintf (stderr, "  -region Rmin Rmax Dmin Dmax : update only this portion of the sky\n");
  fprintf (stderr, "  -only-match           	  : only add measurements to existing objects\n");
  fprintf (stderr, "  -replace              	  : replace time/photcode measurements (no duplication)\n");
  fprintf (stderr, "  -v                    	  : verbose mode\n");
  fprintf (stderr, "  -help                 	  : this list\n");
  fprintf (stderr, "  -h                    	  : this list\n\n");
  exit (2);
}

# if (0)
  /* select quality flags */
  SELECT_2MASS_QUALITY = NULL;
  if ((N = get_argument (argc, argv, "-2massquality"))) {
    remove_argument (N, &argc, argv);
    SELECT_2MASS_QUALITY = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }
# endif

# if (0)  
  /* override any header PHOTCODE values */
  if ((N = get_argument (argc, argv, "-p"))) {
    remove_argument (N, &argc, argv);
    options.photcode = GetPhotcodeCodebyName (argv[N]);
    remove_argument (N, &argc, argv);
  }
# endif

