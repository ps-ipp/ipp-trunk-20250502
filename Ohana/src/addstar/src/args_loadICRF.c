# include "addstar.h"
# include "loadICRF.h"

static void help (void);
static void help_client (void);

AddstarClientOptions args_loadICRF (int *argc, char **argv, AddstarClientOptions options) {
  
  int N;

  /* check for help request */
  if (get_argument (*argc, argv, "-help") ||
      get_argument (*argc, argv, "-h")) {
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

  /* other addstar options which cannot be used in loadICRF */
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
    fprintf (stderr, "USAGE: loadICRF [options] (file) [..more files]\n");
    exit (2);
  }
  return (options);
}

AddstarClientOptions args_loadICRF_client (int *argc, char **argv, AddstarClientOptions options) {
  
  int N;

  /* check for help request */
  if (get_argument (*argc, argv, "-help") ||
      get_argument (*argc, argv, "-h")) {
    help ();
  }

  PARALLEL = FALSE;
  PARALLEL_MANUAL = FALSE;
  PARALLEL_SERIAL = FALSE;

  HOST_ID = 0;
  if ((N = get_argument (*argc, argv, "-hostID"))) {
    remove_argument (N, argc, argv);
    HOST_ID = atoi (argv[N]);
    remove_argument (N, argc, argv);
  }
  if (!HOST_ID) help_client();

  HOSTDIR = NULL;
  if ((N = get_argument (*argc, argv, "-hostdir"))) {
    remove_argument (N, argc, argv);
    HOSTDIR = strcreate (argv[N]);
    remove_argument (N, argc, argv);
  }
  if (!HOSTDIR) help_client();

  CPT_FILE = NULL;
  if ((N = get_argument (*argc, argv, "-cpt"))) {
    remove_argument (N, argc, argv);
    CPT_FILE = strcreate (argv[N]);
    remove_argument (N, argc, argv);
  }
  if (!CPT_FILE) help_client();
  
  INPUT = NULL;
  if ((N = get_argument (*argc, argv, "-input"))) {
    remove_argument (N, argc, argv);
    INPUT = strcreate (argv[N]);
    remove_argument (N, argc, argv);
  }
  if (!INPUT) help_client();

  /* we do not allow a subset to be extracted -- all or nothing, babe */
  UserPatch.Rmin = 0;
  UserPatch.Rmax= 360;
  UserPatch.Dmin = -90;
  UserPatch.Dmax = +90;

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

  /* other addstar options which cannot be used in loadICRF */
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

  if (*argc != 1) {
    fprintf (stderr, "USAGE: loadICRF_client -cpt (file) -input (file)\n");
    exit (2);
  }
  return (options);
}

static void help () {

  fprintf (stderr, "USAGE: loadICRF [options] (file) [..more files]\n");
  fprintf (stderr, "  add data from ICRF list to DVO\n\n");

  fprintf (stderr, "  optional flags:\n");
  fprintf (stderr, "  -replace              	  : replace time/photcode measurements (no duplication)\n");
  fprintf (stderr, "  -v                    	  : verbose mode\n");
  fprintf (stderr, "  -help                 	  : this list\n");
  fprintf (stderr, "  -h                    	  : this list\n\n");
  exit (2);
}

static void help_client () {

  fprintf (stderr, "USAGE: loadICRF_client -D CATDIR catdir -hostID ID -hostdir (dir) -cpt (filename) -input (input) [options]\n");
  fprintf (stderr, "  add data from stellar parameter file to DVO\n\n");

  fprintf (stderr, "  optional flags:\n");
  fprintf (stderr, "  -replace              	  : replace time/photcode measurements (no duplication)\n");
  fprintf (stderr, "  -v                    	  : verbose mode\n");
  fprintf (stderr, "  -help                 	  : this list\n");
  fprintf (stderr, "  -h                    	  : this list\n\n");
  exit (2);
}
