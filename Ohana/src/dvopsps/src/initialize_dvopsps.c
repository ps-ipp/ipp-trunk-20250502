# include "dvopsps.h"

void usage_dvopsps () {
  fprintf (stderr, "USAGE: dvopsps (mode) [dbinfo] [options]\n");
  fprintf (stderr, "    mysql database info is supplied with these:\n");
  fprintf (stderr, "    -dbhost : database host machine\n");
  fprintf (stderr, "    -dbuser : database username\n");
  fprintf (stderr, "    -dbpass : database password\n");
  fprintf (stderr, "    -dbname : database name\n\n");
  fprintf (stderr, "  options:\n");
  fprintf (stderr, "    -v : verbose mode\n");
  fprintf (stderr, "    -region Rmin Rmax Dmin Dmax\n");
  fprintf (stderr, "    -cpt n0000/0000.00 : limit to the named cpt file\n");
  fprintf (stderr, "    -parallel : run in parallel mode\n");
  fprintf (stderr, "    -insert-remote : in parallel mode, the client sends the data to the dbhost\n");
  fprintf (stderr, "    -time-start YYYY/MM/DD,hh:mm:ss : limit detections to >= this time\n");
  fprintf (stderr, "    -time-end   YYYY/MM/DD,hh:mm:ss : limit detections to <  this time\n");

  fprintf (stderr, "    -photcode-start NN : limit detections to >= this photcode number\n");
  fprintf (stderr, "    -photcode-end   NN : limit detections to <  this photcode number\n");
  fprintf (stderr, "\n");

  fprintf (stderr, " (mode) is one of detections, objects, forced_warp_objects, forced_galaxy_shaoe, skytable\n");

  fprintf (stderr, "\n");
  fprintf (stderr, "    -h     : this help list\n");
  fprintf (stderr, "    -help  : this help list\n");
  fprintf (stderr, "    --h    : this help list\n");
  fprintf (stderr, "    --help : this help list\n");

  fprintf (stderr, "    Note that the dvo db can be specified by -D CATDIR (directory)\n");
  exit (2);
}

void initialize_dvopsps (int argc, char **argv) {

  int N;
  struct stat statbuffer;
  char CatdirPhotcodeFile[256];

  if ((N = get_argument (argc, argv, "-h"))) usage_dvopsps();
  if ((N = get_argument (argc, argv, "-help"))) usage_dvopsps();
  if ((N = get_argument (argc, argv, "--h"))) usage_dvopsps();
  if ((N = get_argument (argc, argv, "--help"))) usage_dvopsps();

  /* are these set correctly? */
  ConfigInit (&argc, argv);
  args_dvopsps (argc, argv);

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

int args_dvopsps (int argc, char **argv) {

  int N;

  if ((N = get_argument (argc, argv, "-dbhost"))) {
    remove_argument (N, &argc, argv);
    DATABASE_HOST = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  } else usage_dvopsps();

  if ((N = get_argument (argc, argv, "-dbuser"))) {
    remove_argument (N, &argc, argv);
    DATABASE_USER = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  } else usage_dvopsps();

  if ((N = get_argument (argc, argv, "-dbpass"))) {
    remove_argument (N, &argc, argv);
    DATABASE_PASS = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  } else usage_dvopsps();

  if ((N = get_argument (argc, argv, "-dbname"))) {
    remove_argument (N, &argc, argv);
    DATABASE_NAME = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  } else usage_dvopsps();

  RESULT_FILE = NULL; // only used by dvopsps_client
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

  VERBOSE = FALSE;
  if ((N = get_argument (argc, argv, "-v"))) {
    VERBOSE = TRUE;
    remove_argument (N, &argc, argv);
  }

  SAVE_REMOTE = TRUE;
  if ((N = get_argument (argc, argv, "-insert-remote"))) {
    SAVE_REMOTE = FALSE;
    remove_argument (N, &argc, argv);
  }

  TEST_MODE = FALSE;
  if ((N = get_argument (argc, argv, "-test-mode"))) {
    TEST_MODE = TRUE;
    remove_argument (N, &argc, argv);
  }

  TIME_START = NULL;
  if ((N = get_argument (argc, argv, "-time-start"))) {
    remove_argument (N, &argc, argv);
    TIME_START = strcreate(argv[N]);
    remove_argument (N, &argc, argv);
    time_t TIME_START_SEC;
    if (!ohana_str_to_time (TIME_START, &TIME_START_SEC)) {
      fprintf (stderr, "error with starting time given by -time-start: %s\n", TIME_START);
      exit (2);
    }
  }

  TIME_END = NULL;
  if ((N = get_argument (argc, argv, "-time-end"))) {
    remove_argument (N, &argc, argv);
    TIME_END = strcreate(argv[N]);
    remove_argument (N, &argc, argv);
    time_t TIME_END_SEC;
    if (!ohana_str_to_time (TIME_END, &TIME_END_SEC)) {
      fprintf (stderr, "error with starting time given by -time-end: %s\n", TIME_END);
      exit (2);
    }
  }

  PHOTCODE_START = 0;
  if ((N = get_argument (argc, argv, "-photcode-start"))) {
    remove_argument (N, &argc, argv);
    PHOTCODE_START = atoi(argv[N]);
    remove_argument (N, &argc, argv);
  }

  PHOTCODE_END = INT_MAX;
  if ((N = get_argument (argc, argv, "-photcode-end"))) {
    remove_argument (N, &argc, argv);
    PHOTCODE_END = atoi(argv[N]);
    remove_argument (N, &argc, argv);
  }

  HOST_ID = 0;
  PARALLEL = FALSE;
  if ((N = get_argument (argc, argv, "-parallel"))) {
    PARALLEL = TRUE;
    remove_argument (N, &argc, argv);
  }
  // this is a test mode : rather than launching the remote jobs and waiting for completion,
  // dvopsps will simply list the remote command and wait for the user to signal completion
  PARALLEL_MANUAL = FALSE;
  if ((N = get_argument (argc, argv, "-parallel-manual"))) {
    PARALLEL = TRUE; // -parallel-manual implies -parallel
    PARALLEL_MANUAL = TRUE;
    remove_argument (N, &argc, argv);
  }
  // this is a test mode : rather than launching the dvopsps_client jobs remotely, they are 
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

  if (argc != 2) usage_dvopsps();

  return (TRUE);
}

void usage_dvopsps_client () {
  fprintf (stderr, "USAGE: dvopsps_client (mode) [dbinfo] [hostinfo] [options]\n");
  fprintf (stderr, "    dvo host info:\n");
  fprintf (stderr, "    -hostID (ID)   : dvo host ID\n");
  fprintf (stderr, "    -hostdir (dir) : local dvo catdir\n");
  fprintf (stderr, "    -catdir  (dir) : master dvo catdir\n\n");
  fprintf (stderr, "    mysql database info is supplied with these:\n");
  fprintf (stderr, "    -dbhost : database host machine\n");
  fprintf (stderr, "    -dbuser : database username\n");
  fprintf (stderr, "    -dbpass : database password\n");
  fprintf (stderr, "    -dbname : database name\n\n");
  fprintf (stderr, "  options:\n");
  fprintf (stderr, "    -v : verbose mode\n");
  fprintf (stderr, "    -region Rmin Rmax Dmin Dmax\n");
  fprintf (stderr, "    -cpt filename\n");
  fprintf (stderr, "    -parallel : run in parallel mode\n");
  fprintf (stderr, "    -h     : this help list\n");
  fprintf (stderr, "    -help  : this help list\n");
  fprintf (stderr, "    --h    : this help list\n");
  fprintf (stderr, "    --help : this help list\n");
  fprintf (stderr, "    Note that the dvo db can be specified by -D CATDIR (directory)\n");
  exit (2);
}

void initialize_dvopsps_client (int argc, char **argv) {

  int N;
  struct stat statbuffer;
  char CatdirPhotcodeFile[256];

  if ((N = get_argument (argc, argv, "-h"))) usage_dvopsps_client();
  if ((N = get_argument (argc, argv, "-help"))) usage_dvopsps_client();
  if ((N = get_argument (argc, argv, "--h"))) usage_dvopsps_client();
  if ((N = get_argument (argc, argv, "--help"))) usage_dvopsps_client();

  SetZeroPoint (25.0); // XXX is this needed?
  args_dvopsps_client (argc, argv);

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

int args_dvopsps_client (int argc, char **argv) {

  int N;

  // by definition, the client is not parallel 
  PARALLEL = FALSE;
  PARALLEL_MANUAL = FALSE;
  PARALLEL_SERIAL = FALSE;

  if ((N = get_argument (argc, argv, "-dbhost"))) {
    remove_argument (N, &argc, argv);
    DATABASE_HOST = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  } else usage_dvopsps_client();

  if ((N = get_argument (argc, argv, "-dbuser"))) {
    remove_argument (N, &argc, argv);
    DATABASE_USER = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  } else usage_dvopsps_client();

  if ((N = get_argument (argc, argv, "-dbpass"))) {
    remove_argument (N, &argc, argv);
    DATABASE_PASS = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  } else usage_dvopsps_client();

  if ((N = get_argument (argc, argv, "-dbname"))) {
    remove_argument (N, &argc, argv);
    DATABASE_NAME = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  } else usage_dvopsps_client();

  RESULT_FILE = NULL; // only used by dvopsps_client
  if ((N = get_argument (argc, argv, "-save"))) {
    remove_argument (N, &argc, argv);
    RESULT_FILE = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
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

  TIME_START = NULL;
  if ((N = get_argument (argc, argv, "-time-start"))) {
    remove_argument (N, &argc, argv);
    TIME_START = strcreate(argv[N]);
    remove_argument (N, &argc, argv);
    time_t TIME_START_SEC;
    if (!ohana_str_to_time (TIME_START, &TIME_START_SEC)) {
      fprintf (stderr, "error with starting time given by -time-start: %s\n", TIME_START);
      exit (2);
    }
  }

  TIME_END = NULL;
  if ((N = get_argument (argc, argv, "-time-end"))) {
    remove_argument (N, &argc, argv);
    TIME_END = strcreate(argv[N]);
    remove_argument (N, &argc, argv);
    time_t TIME_END_SEC;
    if (!ohana_str_to_time (TIME_END, &TIME_END_SEC)) {
      fprintf (stderr, "error with starting time given by -time-end: %s\n", TIME_END);
      exit (2);
    }
  }

  PHOTCODE_START = 0;
  if ((N = get_argument (argc, argv, "-photcode-start"))) {
    remove_argument (N, &argc, argv);
    PHOTCODE_START = atoi(argv[N]);
    remove_argument (N, &argc, argv);
  }

  PHOTCODE_END = INT_MAX;
  if ((N = get_argument (argc, argv, "-photcode-end"))) {
    remove_argument (N, &argc, argv);
    PHOTCODE_END = atoi(argv[N]);
    remove_argument (N, &argc, argv);
  }

  VERBOSE = FALSE;
  if ((N = get_argument (argc, argv, "-v"))) {
    VERBOSE = TRUE;
    remove_argument (N, &argc, argv);
  }

  SAVE_REMOTE = TRUE;
  if ((N = get_argument (argc, argv, "-insert-remote"))) {
    SAVE_REMOTE = FALSE;
    remove_argument (N, &argc, argv);
  }

  TEST_MODE = FALSE;
  if ((N = get_argument (argc, argv, "-test-mode"))) {
    TEST_MODE = TRUE;
    remove_argument (N, &argc, argv);
  }

  HOST_ID = 0;
  if ((N = get_argument (argc, argv, "-hostID"))) {
    remove_argument (N, &argc, argv);
    HOST_ID = atoi (argv[N]);
    remove_argument (N, &argc, argv);
  }
  if (!HOST_ID) usage_dvopsps_client();

  HOSTDIR = NULL;
  if ((N = get_argument (argc, argv, "-hostdir"))) {
    remove_argument (N, &argc, argv);
    HOSTDIR = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }
  if (!HOSTDIR) usage_dvopsps_client();

  if ((N = get_argument (argc, argv, "-catdir"))) {
    remove_argument (N, &argc, argv);
    CATDIR = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }
  if (!CATDIR) usage_dvopsps_client();

  if (argc != 2) usage_dvopsps_client();
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
