# include "photdbc.h"

int args (int argc, char **argv) {

  int N;

  VERBOSE = FALSE;
  if ((N = get_argument (argc, argv, "-v"))) {
    VERBOSE = TRUE;
    remove_argument (N, &argc, argv);
  }

  SHOW_PARAMS = FALSE;
  if ((N = get_argument (argc, argv, "-params"))) {
    remove_argument (N, &argc, argv);
    SHOW_PARAMS = TRUE;
  }

  SKIP_IMAGES = FALSE;
  if ((N = get_argument (argc, argv, "-skip-images"))) {
    SKIP_IMAGES = TRUE;
    remove_argument (N, &argc, argv);
  }

  ONLY_IMAGES = FALSE;
  if ((N = get_argument (argc, argv, "-only-images"))) {
    if (SKIP_IMAGES) {
      fprintf (stderr, "-skip-images and -only-images cannot be used together\n");
    }
    ONLY_IMAGES = TRUE;
    remove_argument (N, &argc, argv);
  }

  /** allow only certain tables to be copied **/ 
  SKIP_MEASURE = FALSE;
  if ((N = get_argument (argc, argv, "-skip-measure"))) {
    SKIP_MEASURE = TRUE;
    remove_argument (N, &argc, argv);
  }
  SKIP_MISSING = FALSE;
  if ((N = get_argument (argc, argv, "-skip-missing"))) {
    SKIP_MISSING = TRUE;
    remove_argument (N, &argc, argv);
  }
  SKIP_LENSING = FALSE;
  if ((N = get_argument (argc, argv, "-skip-lensing"))) {
    SKIP_LENSING = TRUE;
    remove_argument (N, &argc, argv);
  }
  SKIP_LENSOBJ = FALSE;
  if ((N = get_argument (argc, argv, "-skip-lensobj"))) {
    SKIP_LENSOBJ = TRUE;
    remove_argument (N, &argc, argv);
  }
  SKIP_STARPAR = FALSE;
  if ((N = get_argument (argc, argv, "-skip-starpar"))) {
    SKIP_STARPAR = TRUE;
    remove_argument (N, &argc, argv);
  }
  SKIP_GALPHOT = FALSE;
  if ((N = get_argument (argc, argv, "-skip-galphot"))) {
    SKIP_GALPHOT = TRUE;
    remove_argument (N, &argc, argv);
  }

  ExcludeByInstMag = FALSE;
  if ((N = get_argument (argc, argv, "-instmag"))) {
    ExcludeByInstMag = TRUE;
    remove_argument (N, &argc, argv);
    INST_MAG_MIN = atof(argv[N]);
    remove_argument (N, &argc, argv);
    INST_MAG_MAX = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }

  ExcludeByMinSigma = FALSE;
  if ((N = get_argument (argc, argv, "-min-sigma"))) {
    ExcludeByMinSigma = TRUE;
    remove_argument (N, &argc, argv);
    SIGMA_MIN_KEEP = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }

  ExcludeByMaxMinMag = FALSE;
  if ((N = get_argument (argc, argv, "-maxminmag"))) {
    ExcludeByMaxMinMag = TRUE;
    remove_argument (N, &argc, argv);
    MAX_MIN_MAG = atof(argv[N]);
    remove_argument (N, &argc, argv);
    fprintf (stderr, "warning -maxminmag may not do what you think: check in make_subcatalog.c\n");
  }

  // override input catalog format (PS1_V1, PS1_REF, etc)
  CATFORMAT = NULL;
  if ((N = get_argument (argc, argv, "-set-format"))) {
    remove_argument (N, &argc, argv);
    CATFORMAT = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  // override input catalog mode (raw, mef, split, mysql)
  CATMODE = NULL;
  if ((N = get_argument (argc, argv, "-set-mode"))) {
    remove_argument (N, &argc, argv);
    CATMODE = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  // override input catalog format (PS1_V1, PS1_REF, etc)
  CATCOMPRESS = NULL;
  if ((N = get_argument (argc, argv, "-set-compress"))) {
    remove_argument (N, &argc, argv);
    CATCOMPRESS = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  /* specify portion of the sky */
  REGION.Rmin = 0;
  REGION.Rmax = 360;
  REGION.Dmin = -90;
  REGION.Dmax = +90;
  if ((N = get_argument (argc, argv, "-region"))) {
    remove_argument (N, &argc, argv);
    REGION.Rmin = atof (argv[N]);
    remove_argument (N, &argc, argv);
    REGION.Rmax = atof (argv[N]);
    remove_argument (N, &argc, argv);
    REGION.Dmin = atof (argv[N]);
    remove_argument (N, &argc, argv);
    REGION.Dmax = atof (argv[N]);
    remove_argument (N, &argc, argv);

    if (REGION.Rmin == REGION.Rmax) {
      fprintf (stderr, "ERROR: selected region is ill-defined: Rmin == Rmax\n");
      exit (2);
    }
    if (REGION.Dmin == REGION.Dmax) {
      fprintf (stderr, "ERROR: selected region is ill-defined: Dmin == Dmax\n");
      exit (2);
    }
  }

  // measurements with these photcodes are not copied to the output
  PHOTCODE_DROP_LIST = NULL;
  if ((N = get_argument (argc, argv, "-photcode-drop"))) {
    remove_argument (N, &argc, argv);
    PHOTCODE_DROP_LIST = strcreate(argv[N]);
    remove_argument (N, &argc, argv);
  }

  // measurements with these photcodes are kept ***regardless of quality***
  // -photcode-keep J will keep all J-band measurements of all kinds
  // -photcode-keep GPC1.02.g will keep all g-band measurements from chip XY02
  PHOTCODE_KEEP_LIST = NULL;
  if ((N = get_argument (argc, argv, "-photcode-keep"))) {
    remove_argument (N, &argc, argv);
    PHOTCODE_KEEP_LIST = strcreate(argv[N]);
    remove_argument (N, &argc, argv);
  }

  // XXX for the moment, make this selection manual.  it needs to be automatic 
  // based on the state of the SkyTable
  PARALLEL = FALSE;
  if ((N = get_argument (argc, argv, "-parallel"))) {
    PARALLEL = TRUE;
    remove_argument (N, &argc, argv);
  }
  // this is a test mode : rather than launching the remote jobs and waiting for completion,
  // relphot will simply list the remote command and wait for the user to signal completion
  PARALLEL_MANUAL = FALSE;
  if ((N = get_argument (argc, argv, "-parallel-manual"))) {
    PARALLEL = TRUE; // -parallel-manual implies -parallel
    PARALLEL_MANUAL = TRUE;
    remove_argument (N, &argc, argv);
  }
  // this is a test mode : rather than launching the relphot_client jobs remotely, they are 
  // run in serial via 'system'
  PARALLEL_SERIAL = FALSE;
  if ((N = get_argument (argc, argv, "-parallel-serial"))) {
    if (PARALLEL_MANUAL) {
      fprintf (stderr, "ERROR: cannot mix -parallel-manual and -parallel-serial\n");
      exit (1);
    }
    PARALLEL = TRUE; // -parallel-serial implies -parallel
    PARALLEL_SERIAL = TRUE;
    remove_argument (N, &argc, argv);
  }

  // XXX for the moment, make this selection manual.  it needs to be automatic 
  // based on the state of the SkyTable
  HOSTDIR_OUTPUT = NULL;
  PARALLEL_OUTHOSTS = NULL;
  if ((N = get_argument (argc, argv, "-parallel-output"))) {
    remove_argument (N, &argc, argv);
    PARALLEL_OUTHOSTS = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  if (argc != 2) usage();

  if ((REGION.Rmin == 0) && (REGION.Rmax == 360) && (REGION.Dmin == -90) && (REGION.Dmax == +90)) {
    int i;
    fprintf (stderr, "you have requested a copy of the entire sky in one pass\n");
    fprintf (stderr, "this could be a time consuming operation.  type Ctrl-C within 5 seconds to cancel\n");
    for (i = 5; i > 0; i--) {
      fprintf (stderr, "%d.. ", i);
      usleep (1000000);
    }
    fprintf (stderr, "\n");
  }

  return (TRUE);
}

int args_client (int argc, char **argv) {

  int N;

  PARALLEL = FALSE;
  PARALLEL_MANUAL = FALSE;
  PARALLEL_SERIAL = FALSE;

  HOST_ID = 0;
  if ((N = get_argument (argc, argv, "-hostID"))) {
    remove_argument (N, &argc, argv);
    HOST_ID = atoi (argv[N]);
    remove_argument (N, &argc, argv);
  }
  if (!HOST_ID) usage();

  HOSTDIR = NULL;
  if ((N = get_argument (argc, argv, "-hostdir"))) {
    remove_argument (N, &argc, argv);
    HOSTDIR = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }
  if (!HOSTDIR) usage();

  HOSTDIR_OUTPUT = NULL;
  if ((N = get_argument (argc, argv, "-hostdir-output"))) {
    remove_argument (N, &argc, argv);
    HOSTDIR_OUTPUT = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  VERBOSE = FALSE;
  if ((N = get_argument (argc, argv, "-v"))) {
    VERBOSE = TRUE;
    remove_argument (N, &argc, argv);
  }

  SHOW_PARAMS = FALSE;
  if ((N = get_argument (argc, argv, "-params"))) {
    remove_argument (N, &argc, argv);
    SHOW_PARAMS = TRUE;
  }

  ExcludeByInstMag = FALSE;
  if ((N = get_argument (argc, argv, "-instmag"))) {
    ExcludeByInstMag = TRUE;
    remove_argument (N, &argc, argv);
    INST_MAG_MIN = atof(argv[N]);
    remove_argument (N, &argc, argv);
    INST_MAG_MAX = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }

  ExcludeByMinSigma = FALSE;
  if ((N = get_argument (argc, argv, "-min-sigma"))) {
    ExcludeByMinSigma = TRUE;
    remove_argument (N, &argc, argv);
    SIGMA_MIN_KEEP = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }

  ExcludeByMaxMinMag = FALSE;
  if ((N = get_argument (argc, argv, "-maxminmag"))) {
    ExcludeByMaxMinMag = TRUE;
    remove_argument (N, &argc, argv);
    MAX_MIN_MAG = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }

  /** allow only certain tables to be copied **/ 
  SKIP_MEASURE = FALSE;
  if ((N = get_argument (argc, argv, "-skip-measure"))) {
    SKIP_MEASURE = TRUE;
    remove_argument (N, &argc, argv);
  }
  SKIP_MISSING = FALSE;
  if ((N = get_argument (argc, argv, "-skip-missing"))) {
    SKIP_MISSING = TRUE;
    remove_argument (N, &argc, argv);
  }
  SKIP_LENSING = FALSE;
  if ((N = get_argument (argc, argv, "-skip-lensing"))) {
    SKIP_LENSING = TRUE;
    remove_argument (N, &argc, argv);
  }
  SKIP_LENSOBJ = FALSE;
  if ((N = get_argument (argc, argv, "-skip-lensobj"))) {
    SKIP_LENSOBJ = TRUE;
    remove_argument (N, &argc, argv);
  }
  SKIP_STARPAR = FALSE;
  if ((N = get_argument (argc, argv, "-skip-starpar"))) {
    SKIP_STARPAR = TRUE;
    remove_argument (N, &argc, argv);
  }
  SKIP_GALPHOT = FALSE;
  if ((N = get_argument (argc, argv, "-skip-galphot"))) {
    SKIP_GALPHOT = TRUE;
    remove_argument (N, &argc, argv);
  }

  // override input catalog format (PS1_V1, PS1_REF, etc)
  CATFORMAT = NULL;
  if ((N = get_argument (argc, argv, "-set-format"))) {
    remove_argument (N, &argc, argv);
    CATFORMAT = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  // override input catalog mode (raw, mef, split, mysql)
  CATMODE = NULL;
  if ((N = get_argument (argc, argv, "-set-mode"))) {
    remove_argument (N, &argc, argv);
    CATMODE = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  // override input catalog format (PS1_V1, PS1_REF, etc)
  CATCOMPRESS = NULL;
  if ((N = get_argument (argc, argv, "-set-compress"))) {
    remove_argument (N, &argc, argv);
    CATCOMPRESS = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  /* specify portion of the sky */
  REGION.Rmin = 0;
  REGION.Rmax = 360;
  REGION.Dmin = -90;
  REGION.Dmax = +90;
  if ((N = get_argument (argc, argv, "-region"))) {
    remove_argument (N, &argc, argv);
    REGION.Rmin = atof (argv[N]);
    remove_argument (N, &argc, argv);
    REGION.Rmax = atof (argv[N]);
    remove_argument (N, &argc, argv);
    REGION.Dmin = atof (argv[N]);
    remove_argument (N, &argc, argv);
    REGION.Dmax = atof (argv[N]);
    remove_argument (N, &argc, argv);

    if (REGION.Rmin == REGION.Rmax) {
      fprintf (stderr, "ERROR: selected region is ill-defined: Rmin == Rmax\n");
      exit (2);
    }
    if (REGION.Dmin == REGION.Dmax) {
      fprintf (stderr, "ERROR: selected region is ill-defined: Dmin == Dmax\n");
      exit (2);
    }
  }

  PHOTCODE_DROP_LIST = NULL;
  if ((N = get_argument (argc, argv, "-photcode-drop"))) {
    remove_argument (N, &argc, argv);
    PHOTCODE_DROP_LIST = strcreate(argv[N]);
    remove_argument (N, &argc, argv);
  }

  PHOTCODE_KEEP_LIST = NULL;
  if ((N = get_argument (argc, argv, "-photcode-keep"))) {
    remove_argument (N, &argc, argv);
    PHOTCODE_KEEP_LIST = strcreate(argv[N]);
    remove_argument (N, &argc, argv);
  }

  if (argc != 2) usage();

  if ((REGION.Rmin == 0) && (REGION.Rmax == 360) && (REGION.Dmin == -90) && (REGION.Dmax == +90)) {
    int i;
    fprintf (stderr, "you have requested a copy of the entire sky in one pass\n");
    fprintf (stderr, "this could be a time consuming operation.  type Ctrl-C within 5 seconds to cancel\n");
    for (i = 5; i > 0; i--) {
      fprintf (stderr, "%d.. ", i);
      usleep (1000000);
    }
    fprintf (stderr, "\n");
  }

  return (TRUE);
}

void usage() {

  fprintf (stderr, "USAGE: photdbc (output)\n\n");
  fprintf (stderr, " this program takes an existing DVO database and makes a copy, applying a number of optional\n");
  fprintf (stderr, " filters in the process.  \n\n");

  fprintf (stderr, " photdbc (output)\n\n");

  fprintf (stderr, " parallel options:\n");
  fprintf (stderr, "  -parallel:\n");
  fprintf (stderr, "  -parallel-output HostTable : HostTable should be relative path in OUTPUT directory\n");
  fprintf (stderr, "  -parallel-manual\n");
  fprintf (stderr, "  -parallel-serial\n\n");

  fprintf (stderr, " allowed filters / restrictions include:\n\n");

  fprintf (stderr, " -region Rmin Rmax Dmin Dmax : limit operation to the specified region \n");
  
  fprintf (stderr, " -photcode-drop   : remove these photcodes from the output (REF or DEP only)\n");
  fprintf (stderr, " -photcode-keep  : ignore these photcodes when assessing the validity (keep unless object is dropped)\n");

  fprintf (stderr, " -instmag (min) (max) : range of valid instrumental magnitudes (or measurements are dropped)\n");
  fprintf (stderr, " -min-sigma (sigma)   : object must have one measurement error less than sigma or object is dropped\n");

  fprintf (stderr, " -maxminmag (mag)   : object must have one magnitude less than this or object is dropped\n");

  fprintf (stderr, " option options:\n");
  fprintf (stderr, " -v : verbose mode\n");
  fprintf (stderr, " -params : list the current parameters\n");
  fprintf (stderr, " -skip-images\n");
  fprintf (stderr, " -only-images\n");
  fprintf (stderr, " -set-compress (catcompress) : NONE, GZIP_1, GZIP_2, RICE_1, AUTO\n");
  fprintf (stderr, " -set-format (catmode)\n");
  fprintf (stderr, " -set-mode (catmode)\n");

  fprintf (stderr, "ptolemy.rc config values used by this program:\n");
  fprintf (stderr, " SIGMA_MAX : drop measurements with errors greater than this\n");
  fprintf (stderr, " NMEAS_MIN : drop objects with fewer measurements than this\n");
  fprintf (stderr, " NMEAS_MIN_FILTERED : drop objects with fewer measurements than this after filtering above\n");
  fprintf (stderr, " AVE_SIGMA_LIM : drop objects if all average mags are greater than this\n");

  exit (2);
}

// fprintf (stderr, " -join                 : join measurements between stars using JOIN_RADIUS\n");
// fprintf (stderr, " -ccdregion X Y X Y    : only keep detections within the specified detector region\n");
// fprintf (stderr, "                         (can this be limited to specific photcodes? cameras? detectors?)\n");
// fprintf (stderr, " -photcode_limits code Mmin Mmax : allow multiples of these\n");
