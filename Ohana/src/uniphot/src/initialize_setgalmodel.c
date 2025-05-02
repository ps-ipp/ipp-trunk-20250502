# include "setgalmodel.h"

void usage_setgalmodel () {
    fprintf (stderr, "USAGE: setgalmodel [options]\n");
    fprintf (stderr, "  options:\n");
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

void initialize_setgalmodel (int argc, char **argv) {

  int N;
  struct stat statbuffer;
  char CatdirPhotcodeFile[256];

  if ((N = get_argument (argc, argv, "-h"))) usage_setgalmodel();
  if ((N = get_argument (argc, argv, "-help"))) usage_setgalmodel();
  if ((N = get_argument (argc, argv, "--h"))) usage_setgalmodel();
  if ((N = get_argument (argc, argv, "--help"))) usage_setgalmodel();

  /* are these set correctly? */
  ConfigInit (&argc, argv);
  args_setgalmodel (argc, argv);

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

  // XXX add to config?
  if (!InitGalaxyModel (GALAXY_MODEL)) {
    fprintf (stderr, "failed to init galaxy model %s\n", GALAXY_MODEL);
    exit (2);
  }
}

int args_setgalmodel (int argc, char **argv) {

  int N;

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

  GALAXY_MODEL = NULL;
  if ((N = get_argument (argc, argv, "-galaxy-model"))) {
    remove_argument (N, &argc, argv);
    GALAXY_MODEL = strcreate(argv[N]);
    remove_argument (N, &argc, argv);
  }
  if (!GALAXY_MODEL) GALAXY_MODEL = strcreate ("FEAST-HIPPARCOS");

  TEST_SCALE = 1.0;
  if ((N = get_argument (argc, argv, "-testing"))) {
    remove_argument (N, &argc, argv);
    TEST_SCALE = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }

  PARALLEL = FALSE;
  if ((N = get_argument (argc, argv, "-parallel"))) {
    PARALLEL = TRUE;
    remove_argument (N, &argc, argv);
  }
  // this is a test mode : rather than launching the remote jobs and waiting for completion,
  // setgalmodel will simply list the remote command and wait for the user to signal completion
  PARALLEL_MANUAL = FALSE;
  if ((N = get_argument (argc, argv, "-parallel-manual"))) {
    PARALLEL = TRUE; // -parallel-manual implies -parallel
    PARALLEL_MANUAL = TRUE;
    remove_argument (N, &argc, argv);
  }
  // this is a test mode : rather than launching the setgalmodel_client jobs remotely, they are 
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

  if (argc != 1) usage_setgalmodel();

  return (TRUE);
}

void usage_setgalmodel_client () {
  fprintf (stderr, "USAGE: setgalmodel_client -hostID (hostID) -catdir (catdir) -hostdir (hostdir) [options]\n");
  fprintf (stderr, "  options:\n");
  fprintf (stderr, "    -region Rmin Rmax Dmin Dmax\n");
  fprintf (stderr, "    -update-catformat (format) : change database schema on output\n");
  fprintf (stderr, "    -v : verbose mode\n");
  fprintf (stderr, "    -update : actually write results to detections tables\n");
  exit (2);
}

void initialize_setgalmodel_client (int argc, char **argv) {

  int N;
  struct stat statbuffer;
  char CatdirPhotcodeFile[256];

  if ((N = get_argument (argc, argv, "-h"))) usage_setgalmodel_client();
  if ((N = get_argument (argc, argv, "-help"))) usage_setgalmodel_client();
  if ((N = get_argument (argc, argv, "--h"))) usage_setgalmodel_client();
  if ((N = get_argument (argc, argv, "--help"))) usage_setgalmodel_client();

  SetZeroPoint (25.0); // XXX is this needed?
  args_setgalmodel_client (argc, argv);

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

  // from args
  if (!InitGalaxyModel (GALAXY_MODEL)) {
    fprintf (stderr, "failed to init galaxy model\n");
    exit (2);
  }
}

int args_setgalmodel_client (int argc, char **argv) {

  int N;

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

  GALAXY_MODEL = NULL;
  if ((N = get_argument (argc, argv, "-galaxy-model"))) {
    remove_argument (N, &argc, argv);
    GALAXY_MODEL = strcreate(argv[N]);
    remove_argument (N, &argc, argv);
  }
  if (!GALAXY_MODEL) GALAXY_MODEL = strcreate ("FEAST-HIPPARCOS");

  TEST_SCALE = 1.0;
  if ((N = get_argument (argc, argv, "-testing"))) {
    remove_argument (N, &argc, argv);
    TEST_SCALE = atof(argv[N]);
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
  if (!HOST_ID) usage_setgalmodel_client();

  HOSTDIR = NULL;
  if ((N = get_argument (argc, argv, "-hostdir"))) {
    remove_argument (N, &argc, argv);
    HOSTDIR = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }
  if (!HOSTDIR) usage_setgalmodel_client();

  if ((N = get_argument (argc, argv, "-catdir"))) {
    remove_argument (N, &argc, argv);
    CATDIR = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }
  if (!CATDIR) usage_setgalmodel_client();

  if (argc != 1) usage_setgalmodel_client();
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
