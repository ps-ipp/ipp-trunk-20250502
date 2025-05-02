# include "setastrom.h"

void usage_setastrom () {
    fprintf (stderr, "USAGE: setastrom [options]\n");
    fprintf (stderr, "  options:\n");
    fprintf (stderr, "    -KH (file) : supply Koppenhoefer correction splines\n");
    fprintf (stderr, "    -DCR (file) : supply DCR correction splines\n");
    fprintf (stderr, "    -CAM (file) : supply camera-static correction file\n");
    fprintf (stderr, "    -TYC (file) : supply tycho-fix file (no pm stars)\n");
    fprintf (stderr, "    -v : verbose mode\n");
    fprintf (stderr, "    -region Rmin Rmax Dmin Dmax\n");
    fprintf (stderr, "    -update-catformat (format) : change database schema on output\n");
    fprintf (stderr, "    -update : actually write results to detections tables\n");
    fprintf (stderr, "    -parallel : run in parallel mode\n");
    fprintf (stderr, "    -h     : this help list\n");
    fprintf (stderr, "    -help  : this help list\n");
    fprintf (stderr, "    --h    : this help list\n");
    fprintf (stderr, "    --help : this help list\n");
    fprintf (stderr, "    Note that the dvo db can be specified by -D CATDIR (directory)\n");
    exit (2);
}

void initialize_setastrom (int argc, char **argv) {

  int N;
  struct stat statbuffer;
  char CatdirPhotcodeFile[256];

  if ((N = get_argument (argc, argv, "-h"))) usage_setastrom();
  if ((N = get_argument (argc, argv, "-help"))) usage_setastrom();
  if ((N = get_argument (argc, argv, "--h"))) usage_setastrom();
  if ((N = get_argument (argc, argv, "--help"))) usage_setastrom();

  /* are these set correctly? */
  ConfigInit (&argc, argv);
  args_setastrom (argc, argv);

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
}

int args_setastrom (int argc, char **argv) {

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

  TYC_FILE = NULL;
  if ((N = get_argument (argc, argv, "-TYC"))) {
    remove_argument (N, &argc, argv);
    char *tmpfile = strcreate (argv[N]);
    TYC_FILE = abspath (tmpfile, DVO_MAX_PATH);
    remove_argument (N, &argc, argv);
  }
  TYC_RESET = FALSE;
  if ((N = get_argument (argc, argv, "-TYC-reset"))) {
    remove_argument (N, &argc, argv);
    TYC_RESET = TRUE;
  }

  SINGLE_CPT = NULL;
  if ((N = get_argument (argc, argv, "-cpt"))) {
    remove_argument (N, &argc, argv);
    SINGLE_CPT = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  /* specify portion of the sky : allow default of all sky? */
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

  UPDATE_CATFORMAT = NULL;
  if ((N = get_argument (argc, argv, "-update-catformat"))) {
    remove_argument (N, &argc, argv);
    UPDATE_CATFORMAT = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  VERBOSE = FALSE;
  if ((N = get_argument (argc, argv, "-v"))) {
    VERBOSE = TRUE;
    remove_argument (N, &argc, argv);
  }

  PARALLEL = FALSE;
  if ((N = get_argument (argc, argv, "-parallel"))) {
    PARALLEL = TRUE;
    remove_argument (N, &argc, argv);
  }
  // this is a test mode : rather than launching the remote jobs and waiting for completion,
  // setastrom will simply list the remote command and wait for the user to signal completion
  PARALLEL_MANUAL = FALSE;
  if ((N = get_argument (argc, argv, "-parallel-manual"))) {
    PARALLEL = TRUE; // -parallel-manual implies -parallel
    PARALLEL_MANUAL = TRUE;
    remove_argument (N, &argc, argv);
  }
  // this is a test mode : rather than launching the setastrom_client jobs remotely, they are 
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

  UPDATE = FALSE;
  if ((N = get_argument (argc, argv, "-update"))) {
    remove_argument (N, &argc, argv);
    UPDATE = TRUE;
  }

  if (!KH_FILE && !DCR_FILE && !CAM_ASTROM_FILE && !TYC_FILE) {
    fprintf (stderr, "WARNING: none of -CAM, -KH, -DCR, -TYC supplied\n");
  }

  if (argc != 1) usage_setastrom();

  return (TRUE);
}

void usage_setastrom_client () {
  fprintf (stderr, "USAGE: setastrom_client -hostID (hostID) -catdir (catdir) -hostdir (hostdir) [options]\n");
  fprintf (stderr, "  options:\n");
  fprintf (stderr, "    -KH  (file) : supply Koppenhoefer correction splines\n");
  fprintf (stderr, "    -DCR (file) : supply DCR correction splines\n");
  fprintf (stderr, "    -CAM (file) : supply camera-static correction file\n");
  fprintf (stderr, "    -TYC (file) : supply tycho-fix file (no pm stars)\n");
  fprintf (stderr, "    -region Rmin Rmax Dmin Dmax\n");
  fprintf (stderr, "    -update-catformat (format) : change database schema on output\n");
  fprintf (stderr, "    -v : verbose mode\n");
  fprintf (stderr, "    -update : actually write results to detections tables\n");
  exit (2);
}

void initialize_setastrom_client (int argc, char **argv) {

  int N;
  struct stat statbuffer;
  char CatdirPhotcodeFile[256];

  if ((N = get_argument (argc, argv, "-h"))) usage_setastrom_client();
  if ((N = get_argument (argc, argv, "-help"))) usage_setastrom_client();
  if ((N = get_argument (argc, argv, "--h"))) usage_setastrom_client();
  if ((N = get_argument (argc, argv, "--help"))) usage_setastrom_client();

  SetZeroPoint (25.0); // XXX is this needed?
  args_setastrom_client (argc, argv);

  if (stat (CATDIR, &statbuffer)) {
    fprintf (stderr, "error accessing dvo database directory '%s'\n", CATDIR);
    exit (1);
  }

  // load the photcode table : XXX needed?
  sprintf (CatdirPhotcodeFile, "%s/Photcodes.dat", CATDIR);
  if (!LoadPhotcodes (CatdirPhotcodeFile, NULL, FALSE)) {
    fprintf (stderr, "error loading photcode table %s\n", CatdirPhotcodeFile);
    exit (1);
  }
}

int args_setastrom_client (int argc, char **argv) {

  int N;

  KH_FILE = NULL;
  if ((N = get_argument (argc, argv, "-KH"))) {
    remove_argument (N, &argc, argv);
    KH_FILE = strcreate (argv[N]);
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
    DCR_FILE = strcreate (argv[N]);
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
    CAM_ASTROM_FILE = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }
  CAM_RESET = FALSE;
  if ((N = get_argument (argc, argv, "-CAM-reset"))) {
    remove_argument (N, &argc, argv);
    CAM_RESET = TRUE;
  }

  TYC_FILE = NULL;
  if ((N = get_argument (argc, argv, "-TYC"))) {
    remove_argument (N, &argc, argv);
    char *tmpfile = strcreate (argv[N]);
    TYC_FILE = abspath (tmpfile, DVO_MAX_PATH);
    remove_argument (N, &argc, argv);
  }
  TYC_RESET = FALSE;
  if ((N = get_argument (argc, argv, "-TYC-reset"))) {
    remove_argument (N, &argc, argv);
    TYC_RESET = TRUE;
  }

  /* specify portion of the sky : allow default of all sky? */
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

  UPDATE_CATFORMAT = NULL;
  if ((N = get_argument (argc, argv, "-update-catformat"))) {
    remove_argument (N, &argc, argv);
    UPDATE_CATFORMAT = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  VERBOSE = FALSE;
  if ((N = get_argument (argc, argv, "-v"))) {
    VERBOSE = TRUE;
    remove_argument (N, &argc, argv);
  }

  UPDATE = FALSE;
  if ((N = get_argument (argc, argv, "-update"))) {
    remove_argument (N, &argc, argv);
    UPDATE = TRUE;
  }

  HOST_ID = 0;
  if ((N = get_argument (argc, argv, "-hostID"))) {
    remove_argument (N, &argc, argv);
    HOST_ID = atoi (argv[N]);
    remove_argument (N, &argc, argv);
  }
  if (!HOST_ID) usage_setastrom_client();

  HOSTDIR = NULL;
  if ((N = get_argument (argc, argv, "-hostdir"))) {
    remove_argument (N, &argc, argv);
    HOSTDIR = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }
  if (!HOSTDIR) usage_setastrom_client();

  if ((N = get_argument (argc, argv, "-catdir"))) {
    remove_argument (N, &argc, argv);
    CATDIR = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }
  if (!CATDIR) usage_setastrom_client();

  if (!KH_FILE && !DCR_FILE && !CAM_ASTROM_FILE && !TYC_FILE) {
    fprintf (stderr, "WARNING: none of -CAM, -KH, -DCR, -TYC supplied\n");
  }

  if (argc != 1) usage_setastrom_client();
  return (TRUE);
}

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
