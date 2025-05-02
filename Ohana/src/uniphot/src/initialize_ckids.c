# include "ckids.h"

void usage_ckids () {
    fprintf (stderr, "USAGE: ckids [options]\n");
    fprintf (stderr, "  options:\n");
    fprintf (stderr, "    -v : verbose mode\n");
    fprintf (stderr, "    -cpt n0000/0000.00.cpt : only repair this cpt file \n");
    fprintf (stderr, "    -region Rmin Rmax Dmin Dmax : only repair this region\n");
    fprintf (stderr, "    -update : actually write results to detections tables\n");
    fprintf (stderr, "    -parallel : run in parallel mode\n\n");

    fprintf (stderr, "    -h     : this help list\n");
    fprintf (stderr, "    -help  : this help list\n");
    fprintf (stderr, "    --h    : this help list\n");
    fprintf (stderr, "    --help : this help list\n");
    
    fprintf (stderr, "    Note that the dvo db can be specified by -D CATDIR (directory)\n");
    exit (2);
}

void initialize_ckids (int argc, char **argv) {

  int N;
  struct stat statbuffer;
  char CatdirPhotcodeFile[256];

  if ((N = get_argument (argc, argv, "-h"))) usage_ckids();
  if ((N = get_argument (argc, argv, "-help"))) usage_ckids();
  if ((N = get_argument (argc, argv, "--h"))) usage_ckids();
  if ((N = get_argument (argc, argv, "--help"))) usage_ckids();

  /* are these set correctly? */
  ConfigInit (&argc, argv);
  args_ckids (argc, argv);

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

int args_ckids (int argc, char **argv) {

  int N;

  VERBOSE = FALSE;
  if ((N = get_argument (argc, argv, "-v"))) {
    VERBOSE = TRUE;
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
  // ckids will simply list the remote command and wait for the user to signal completion
  PARALLEL_MANUAL = FALSE;
  if ((N = get_argument (argc, argv, "-parallel-manual"))) {
    PARALLEL = TRUE; // -parallel-manual implies -parallel
    PARALLEL_MANUAL = TRUE;
    remove_argument (N, &argc, argv);
  }
  // this is a test mode : rather than launching the ckids_client jobs remotely, they are 
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
  // XXX do not allow update until we have added a modification concept
  // if ((N = get_argument (argc, argv, "-update"))) {
  //   remove_argument (N, &argc, argv);
  //   UPDATE = TRUE;
  // }

  if (argc != 1) usage_ckids();

  return (TRUE);
}

void usage_ckids_client () {
  fprintf (stderr, "USAGE: ckids_client -hostID (hostID) -catdir (catdir) -hostdir (hostdir) [options]\n");
  fprintf (stderr, "  options:\n");
  fprintf (stderr, "    -v : verbose mode\n");
  fprintf (stderr, "    -update : actually write results to detections tables\n");
  exit (2);
}

void initialize_ckids_client (int argc, char **argv) {

  int N;
  struct stat statbuffer;
  char CatdirPhotcodeFile[256];

  if ((N = get_argument (argc, argv, "-h"))) usage_ckids_client();
  if ((N = get_argument (argc, argv, "-help"))) usage_ckids_client();
  if ((N = get_argument (argc, argv, "--h"))) usage_ckids_client();
  if ((N = get_argument (argc, argv, "--help"))) usage_ckids_client();

  SetZeroPoint (25.0); // XXX is this needed?
  args_ckids_client (argc, argv);

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

int args_ckids_client (int argc, char **argv) {

  int N;

  VERBOSE = FALSE;
  if ((N = get_argument (argc, argv, "-v"))) {
    VERBOSE = TRUE;
    remove_argument (N, &argc, argv);
  }

  UPDATE = FALSE;
  // if ((N = get_argument (argc, argv, "-update"))) {
  //   remove_argument (N, &argc, argv);
  //   UPDATE = TRUE;
  // }

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

  HOST_ID = 0;
  if ((N = get_argument (argc, argv, "-hostID"))) {
    remove_argument (N, &argc, argv);
    HOST_ID = atoi (argv[N]);
    remove_argument (N, &argc, argv);
  }
  if (!HOST_ID) usage_ckids_client();

  HOSTDIR = NULL;
  if ((N = get_argument (argc, argv, "-hostdir"))) {
    remove_argument (N, &argc, argv);
    HOSTDIR = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }
  if (!HOSTDIR) usage_ckids_client();

  if ((N = get_argument (argc, argv, "-catdir"))) {
    remove_argument (N, &argc, argv);
    CATDIR = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }
  if (!CATDIR) usage_ckids_client();

  if (argc != 1) usage_ckids_client();
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
