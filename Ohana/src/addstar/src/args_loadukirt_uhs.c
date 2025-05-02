# include "addstar.h"
# include "ukirt_uhs.h"

static void help (void);

AddstarClientOptions args_loadukirt_uhs (int *argc, char **argv, AddstarClientOptions options) {
  
  int N;

  /* check for help request */
  if (get_argument (*argc, argv, "-help") ||
      get_argument (*argc, argv, "-h")) {
    help ();
  }

  // UKIRT_MODE is global
  UKIRT_MODE = UKIRT_MODE_NONE;
  if ((N = get_argument (*argc, argv, "-uhs"))) {
    UKIRT_MODE = UKIRT_MODE_UHS;
    UKIRT_NFILTER = 1;
    remove_argument (N, argc, argv);
  }
  if ((N = get_argument (*argc, argv, "-ugcs"))) {
    UKIRT_MODE = UKIRT_MODE_UGCS;
    UKIRT_NFILTER = 6;
    remove_argument (N, argc, argv);
  }
  if ((N = get_argument (*argc, argv, "-ugps"))) {
    UKIRT_MODE = UKIRT_MODE_UGPS;
    UKIRT_NFILTER = 5;
    remove_argument (N, argc, argv);
  }
  if ((N = get_argument (*argc, argv, "-ulas"))) {
    UKIRT_MODE = UKIRT_MODE_ULAS;
    UKIRT_NFILTER = 5;
    remove_argument (N, argc, argv);
  }
  if ((N = get_argument (*argc, argv, "-uhs2022"))) {
    UKIRT_MODE = UKIRT_MODE_UHS2022;
    UKIRT_NFILTER = 2;
    remove_argument (N, argc, argv);
  }
  if (UKIRT_MODE == UKIRT_MODE_NONE) {
    fprintf (stderr, "missing or invalid ukirt mode\n");
    help ();
  }

  /* basic mode: image, list, refcat */
  options.mode = ADDSTAR_MODE_REFCAT;

  /* we do not allow a subset to be extracted -- all or nothing, babe */
  UserPatch.Rmin = 0;
  UserPatch.Rmax= 360;
  UserPatch.Dmin = -90;
  UserPatch.Dmax = +90;

  // XXX for the moment, make this selection manual.  it needs to be automatic 
  // based on the state of the SkyTable
  HOST_ID = 0;
  PARALLEL = FALSE;
  if ((N = get_argument (*argc, argv, "-parallel"))) {
    PARALLEL = TRUE;
    remove_argument (N, argc, argv);
  }
  // this is a test mode : rather than launching the remote jobs and waiting for completion,
  // relphot will simply list the remote command and wait for the user to signal completion
  PARALLEL_MANUAL = FALSE;
  if ((N = get_argument (*argc, argv, "-parallel-manual"))) {
    PARALLEL = TRUE; // -parallel-manual implies -parallel
    PARALLEL_MANUAL = TRUE;
    remove_argument (N, argc, argv);
  }
  // this is a test mode : rather than launching the relphot_client jobs remotely, they are 
  // run in serial via 'system'
  PARALLEL_SERIAL = FALSE;
  if ((N = get_argument (*argc, argv, "-parallel-serial"))) {
    if (PARALLEL_MANUAL) {
      fprintf (stderr, "ERROR: cannot mix -parallel-manual and -parallel-serial\n");
      exit (1);
    }
    PARALLEL = TRUE; // -parallel-serial implies -parallel
    PARALLEL_SERIAL = TRUE;
    remove_argument (N, argc, argv);
  }

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
  /* override any header PHOTCODE values */
  options.photcode = 0;

  /* accept proper-motion (& parallax) data from reference */
  ACCEPT_MOTION = FALSE;
  if ((N = get_argument (*argc, argv, "-accept-motion"))) {
    ACCEPT_MOTION = TRUE;
    remove_argument (N, argc, argv);
  }

  /* provide a time for dataset */
  options.timeref = 0; 
  if ((N = get_argument (*argc, argv, "-time"))) {
    time_t tmp;
    remove_argument (N, argc, argv);
    if (!ohana_str_to_time (argv[N], &tmp)) { 
      fprintf (stderr, "syntax error in time\n");
      exit (1);
    }
    options.timeref = tmp;
    remove_argument (N, argc, argv);
  }

  /* extra error messages */
  VERBOSE = FALSE;
  if ((N = get_argument (*argc, argv, "-v"))) {
    VERBOSE = TRUE;
    remove_argument (N, argc, argv);
  }

  /* other addstar options which cannot be used in loadukirt_uhs */
  // options.timeref = 0; 
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

  if (*argc != 2) {
    fprintf (stderr, "USAGE: loadukirt_uhs [options] (fitsfile)\n");
    exit (2);
  }
  return (options);
}

static void help () {

  fprintf (stderr, "USAGE: loadukirt_uhs [options] (file) [-uhs | -ugcs | -ulas | -ugps | -uhs2022] [..more files]\n");
  fprintf (stderr, "  add data from UKIRT CSV file to DVO\n");
  fprintf (stderr, "  mode is required : -uhs | -ugcs | -ulas | -ugps | -uhs2022\n\n");

  fprintf (stderr, "  optional flags:\n");
  fprintf (stderr, "  -only-match           	  : only add measurements to existing objects\n");
  fprintf (stderr, "  -replace              	  : replace time/photcode measurements (no duplication)\n");
  fprintf (stderr, "  -v                    	  : verbose mode\n");
  fprintf (stderr, "  -help                 	  : this list\n");
  fprintf (stderr, "  -h                    	  : this list\n\n");
  exit (2);
}
