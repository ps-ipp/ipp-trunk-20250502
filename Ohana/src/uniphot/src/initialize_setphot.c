# include "setphot.h"

static char *ZPT_OFFSETS_FILTERS = NULL;
static char *ZPT_OFFSETS_VALUES = NULL;

void initialize_setphot (int argc, char **argv) {

  struct stat statbuffer;

  char CatdirPhotcodeFile[256];

  /* are these set correctly? */
  ConfigInit (&argc, argv);
  args_setphot (argc, argv);

  if (stat (CATDIR, &statbuffer)) {
    fprintf (stderr, "error accessing dvo database directory '%s'\n", CATDIR);
    exit (1);
  }

  // load the photcode table
  sprintf (CatdirPhotcodeFile, "%s/Photcodes.dat", CATDIR);
  if (!LoadPhotcodes (CatdirPhotcodeFile, NULL, FALSE)) {
    fprintf (stderr, "error loading photcode table %s\n", CatdirPhotcodeFile);
    exit (1);
  }

  // if we have a zpt offset list, parse it here
  parse_zpt_offsets (ZPT_OFFSETS_FILTERS, ZPT_OFFSETS_VALUES);
}

int args_setphot (int argc, char **argv) {

  int N;

  KH_FILE = NULL;
  if ((N = get_argument (argc, argv, "-KH"))) {
    remove_argument (N, &argc, argv);
    char *tmpfile = strcreate (argv[N]);
    KH_FILE = abspath (tmpfile, DVO_MAX_PATH);
    remove_argument (N, &argc, argv);
  }
  KH_RESET = FALSE;
  if ((N = get_argument (argc, argv, "-KH-reset"))) {
    remove_argument (N, &argc, argv);
    KH_RESET = TRUE;
  }

  DCR_FILE = NULL;
  if ((N = get_argument (argc, argv, "-DCR"))) {
    remove_argument (N, &argc, argv);
    char *tmpfile = strcreate (argv[N]);
    DCR_FILE = abspath (tmpfile, DVO_MAX_PATH);
    remove_argument (N, &argc, argv);
  }
  DCR_RESET = FALSE;
  if ((N = get_argument (argc, argv, "-DCR-reset"))) {
    remove_argument (N, &argc, argv);
    DCR_RESET = TRUE;
  }

  CAM_ASTROM_FILE = NULL;
  if ((N = get_argument (argc, argv, "-CAM"))) {
    remove_argument (N, &argc, argv);
    char *tmpfile = strcreate (argv[N]);
    CAM_ASTROM_FILE = abspath (tmpfile, DVO_MAX_PATH);
    remove_argument (N, &argc, argv);
  }
  CAM_RESET = FALSE;
  if ((N = get_argument (argc, argv, "-CAM-reset"))) {
    remove_argument (N, &argc, argv);
    CAM_RESET = TRUE;
  }

  CAM_PHOTOM_FILE = NULL;
  if ((N = get_argument (argc, argv, "-cam-flat"))) {
    remove_argument (N, &argc, argv);
    char *tmpfile = strcreate (argv[N]);
    CAM_PHOTOM_FILE = abspath (tmpfile, DVO_MAX_PATH);
    remove_argument (N, &argc, argv);
  }

  SET_GAL_MODEL = NULL;
  if ((N = get_argument (argc, argv, "-setgalmodel"))) {
    remove_argument (N, &argc, argv);
    SET_GAL_MODEL = strcreate (argv[N]);
    remove_argument (N, &argc, argv);

    if (!InitGalaxyModel (SET_GAL_MODEL)) {
      fprintf (stderr, "failed to init galaxy model %s\n", SET_GAL_MODEL);
      fprintf (stderr, "valid models: ROESER, FEAST-HIPPARCOS\n");
      exit (2);
    }
  }

  VERBOSE = FALSE;
  if ((N = get_argument (argc, argv, "-v"))) {
    VERBOSE = TRUE;
    remove_argument (N, &argc, argv);
  }

  IMAGES_ONLY = FALSE;
  if ((N = get_argument (argc, argv, "-images-only"))) {
    IMAGES_ONLY = TRUE;
    remove_argument (N, &argc, argv);
  }

  RESET = FALSE;
  if ((N = get_argument (argc, argv, "-reset"))) {
    RESET = TRUE;
    remove_argument (N, &argc, argv);
  }

  PHOTCODE_MIN = 0;
  PHOTCODE_MAX = 0;
  if ((N = get_argument (argc, argv, "-photcode-range"))) {
    remove_argument (N, &argc, argv);
    PHOTCODE_MIN = atoi (argv[N]);    
    remove_argument (N, &argc, argv);
    PHOTCODE_MAX = atoi (argv[N]);    
    remove_argument (N, &argc, argv);
  }

  SKIP_EXTRA_EXTENSIONS = FALSE;
  if ((N = get_argument (argc, argv, "-skip-extra-extensions"))) {
    SKIP_EXTRA_EXTENSIONS = TRUE;
    remove_argument (N, &argc, argv);
  }

  REPAIR_BY_OBJID = FALSE;
  if ((N = get_argument (argc, argv, "-repair-by-objid"))) {
    REPAIR_BY_OBJID = TRUE;
    remove_argument (N, &argc, argv);
  }

  SINGLE_CPT = NULL;
  if ((N = get_argument (argc, argv, "-cpt"))) {
    remove_argument (N, &argc, argv);
    SINGLE_CPT = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  // region of interest
  UserPatch.Rmin = 0;
  UserPatch.Rmax = 360;
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

  PARALLEL = FALSE;
  if ((N = get_argument (argc, argv, "-parallel"))) {
    PARALLEL = TRUE;
    remove_argument (N, &argc, argv);
  }
  // this is a test mode : rather than launching the remote jobs and waiting for completion,
  // setphot will simply list the remote command and wait for the user to signal completion
  PARALLEL_MANUAL = FALSE;
  if ((N = get_argument (argc, argv, "-parallel-manual"))) {
    PARALLEL = TRUE; // -parallel-manual implies -parallel
    PARALLEL_MANUAL = TRUE;
    remove_argument (N, &argc, argv);
  }
  // this is a test mode : rather than launching the setphot_client jobs remotely, they are 
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

  UBERCAL = FALSE;
  if ((N = get_argument (argc, argv, "-ubercal"))) {
    remove_argument (N, &argc, argv);
    UBERCAL = TRUE;
    if (!CAM_PHOTOM_FILE) {
      fprintf (stderr, "ubercal now requires a high-res static flat-file: -cam-flat filename\n");
      exit (2);
    }
  }

  NO_METADATA = FALSE;
  if ((N = get_argument (argc, argv, "-no-metadata"))) {
    remove_argument (N, &argc, argv);
    NO_METADATA = TRUE;
  }

  UPDATE = FALSE;
  if ((N = get_argument (argc, argv, "-update"))) {
    remove_argument (N, &argc, argv);
    UPDATE = TRUE;
  }

  // -zpt-offsets code[,code,etc] value[,value,etc]
  // eg: -zpt-offsets g,r,i 0.01,0.02,-0.02
  if ((N = get_argument (argc, argv, "-zpt-offsets"))) {
    remove_argument (N, &argc, argv);
    ZPT_OFFSETS_FILTERS = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
    ZPT_OFFSETS_VALUES = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }
  

  if (argc > 2) {
    fprintf (stderr, "USAGE: setphot (zptfile) [options]\n");
    fprintf (stderr, "  options:\n");
    fprintf (stderr, "    -v : verbose mode\n");
    fprintf (stderr, "    -ubercal : interpret zpt file as a GPC1 ubercal FITS table\n");
    fprintf (stderr, "    -no-metadata : assume the ubercal FITS table has the layout defined by Eddie Schlafly in Dec 2011\n");
    fprintf (stderr, "    -update : actually write results to detections tables\n");
    fprintf (stderr, "    Note that the dvo db can be specified by -D CATDIR (directory)\n");
    exit (2);
  } 

  return (TRUE);
}

void GetConfig (char *config, char *field, char *format, int N, void *ptr);

void ConfigInit (int *argc, char **argv) {

  double ZERO_POINT;
  char  *config, *file;

  /*** load configuration info ***/
  file = SelectConfigFile (argc, argv, "ptolemy");
  config = LoadConfigFile (file);
  if (config == (char *) NULL) {
    fprintf (stderr, "ERROR: can't find configuration file %s\n", file);
    if (file != (char *) NULL) free (file);
    exit (0);
  }
  if (VERBOSE) fprintf (stderr, "loaded config file: %s\n", file);

  // force CATDIR to be absolute (so parallel mode will work)
  char *tmpcatdir = NULL;
  ALLOCATE (tmpcatdir, char, DVO_MAX_PATH);
  GetConfig (config, "CATDIR",                 "%s",  0, tmpcatdir);
  CATDIR = abspath (tmpcatdir, DVO_MAX_PATH);
  free (tmpcatdir);

  sprintf (ImageCat, "%s/Images.dat", CATDIR);

  ScanConfig (config, "ZERO_PT",                "%lf", 0, &ZERO_POINT);
  SetZeroPoint (ZERO_POINT);

  free (config);
  free (file);
}

void GetConfig (char *config, char *field, char *format, int N, void *ptr) {

  char *status;

  status = ScanConfig (config, field, format, N, ptr);
  if (status == NULL) {
    fprintf (stderr, "error in config, cannot find %s\n", field);
    exit (1);
  }
  return;
}
