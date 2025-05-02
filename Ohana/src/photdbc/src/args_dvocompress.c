# include "dvocompress.h"

int args (int *argc, char **argv) {

  int N;

  if (get_argument (*argc, argv, "-h")) usage();
  if (get_argument (*argc, argv, "--h")) usage();
  if (get_argument (*argc, argv, "-help")) usage();
  if (get_argument (*argc, argv, "--help")) usage();

  VERBOSE = FALSE;
  if ((N = get_argument (*argc, argv, "-v"))) {
    VERBOSE = TRUE;
    remove_argument (N, argc, argv);
  }

  // override input catalog format (PS1_V1, PS1_REF, etc)
  UPDATE_CATFORMAT = NULL;
  if ((N = get_argument (*argc, argv, "-set-format"))) {
    remove_argument (N, argc, argv);
    UPDATE_CATFORMAT = strcreate (argv[N]);
    remove_argument (N, argc, argv);
  }

  // override input catalog format (PS1_V1, PS1_REF, etc)
  UPDATE_CATCOMPRESS = NULL;
  if ((N = get_argument (*argc, argv, "-set-compress"))) {
    remove_argument (N, argc, argv);
    UPDATE_CATCOMPRESS = strcreate (argv[N]);
    remove_argument (N, argc, argv);
  }

  SKIP_COMPRESSED = FALSE;
  if ((N = get_argument (*argc, argv, "-skip-compressed"))) {
    remove_argument (N, argc, argv);
    SKIP_COMPRESSED = TRUE;
  }

  if (!UPDATE_CATFORMAT && !UPDATE_CATCOMPRESS) {
    fprintf (stderr, "neither -set-compress nor -set-format are specified : this is a NOP\n");
    exit (2);
  }

  /* specify portion of the sky */
  REGION.Rmin = 0;
  REGION.Rmax = 360;
  REGION.Dmin = -90;
  REGION.Dmax = +90;
  if ((N = get_argument (*argc, argv, "-region"))) {
    remove_argument (N, argc, argv);
    REGION.Rmin = atof (argv[N]);
    remove_argument (N, argc, argv);
    REGION.Rmax = atof (argv[N]);
    remove_argument (N, argc, argv);
    REGION.Dmin = atof (argv[N]);
    remove_argument (N, argc, argv);
    REGION.Dmax = atof (argv[N]);
    remove_argument (N, argc, argv);

    if (REGION.Rmin == REGION.Rmax) {
      fprintf (stderr, "ERROR: selected region is ill-defined: Rmin == Rmax\n");
      exit (2);
    }
    if (REGION.Dmin == REGION.Dmax) {
      fprintf (stderr, "ERROR: selected region is ill-defined: Dmin == Dmax\n");
      exit (2);
    }
  }

  // XXX for the moment, make this selection manual.  it needs to be automatic 
  // based on the state of the SkyTable
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

  if (*argc != 2) usage();

  return (TRUE);
}

int args_client (int *argc, char **argv) {

  int N;

  PARALLEL = FALSE;
  PARALLEL_MANUAL = FALSE;
  PARALLEL_SERIAL = FALSE;

  if (get_argument (*argc, argv, "-h")) usage();
  if (get_argument (*argc, argv, "--h")) usage();
  if (get_argument (*argc, argv, "-help")) usage();
  if (get_argument (*argc, argv, "--help")) usage();

  HOST_ID = 0;
  if ((N = get_argument (*argc, argv, "-hostID"))) {
    remove_argument (N, argc, argv);
    HOST_ID = atoi (argv[N]);
    remove_argument (N, argc, argv);
  }
  if (!HOST_ID) usage();

  HOSTDIR = NULL;
  if ((N = get_argument (*argc, argv, "-hostdir"))) {
    remove_argument (N, argc, argv);
    HOSTDIR = strcreate (argv[N]);
    remove_argument (N, argc, argv);
  }
  if (!HOSTDIR) usage();

  VERBOSE = FALSE;
  if ((N = get_argument (*argc, argv, "-v"))) {
    VERBOSE = TRUE;
    remove_argument (N, argc, argv);
  }
  if ((N = get_argument (*argc, argv, "-vv"))) {
    VERBOSE = 2;
    remove_argument (N, argc, argv);
  }

  // override input catalog format (PS1_V1, PS1_REF, etc)
  UPDATE_CATFORMAT = NULL;
  if ((N = get_argument (*argc, argv, "-set-format"))) {
    remove_argument (N, argc, argv);
    UPDATE_CATFORMAT = strcreate (argv[N]);
    remove_argument (N, argc, argv);
  }

  // override input catalog format (PS1_V1, PS1_REF, etc)
  UPDATE_CATCOMPRESS = NULL;
  if ((N = get_argument (*argc, argv, "-set-compress"))) {
    remove_argument (N, argc, argv);
    UPDATE_CATCOMPRESS = strcreate (argv[N]);
    remove_argument (N, argc, argv);
  }

  SKIP_COMPRESSED = FALSE;
  if ((N = get_argument (*argc, argv, "-skip-compressed"))) {
    remove_argument (N, argc, argv);
    SKIP_COMPRESSED = TRUE;
  }

  if (!UPDATE_CATFORMAT && !UPDATE_CATCOMPRESS) {
    fprintf (stderr, "neither -set-compress nor -set-format are specified : this is a NOP\n");
    exit (2);
  }

  /* specify portion of the sky */
  REGION.Rmin = 0;
  REGION.Rmax = 360;
  REGION.Dmin = -90;
  REGION.Dmax = +90;
  if ((N = get_argument (*argc, argv, "-region"))) {
    remove_argument (N, argc, argv);
    REGION.Rmin = atof (argv[N]);
    remove_argument (N, argc, argv);
    REGION.Rmax = atof (argv[N]);
    remove_argument (N, argc, argv);
    REGION.Dmin = atof (argv[N]);
    remove_argument (N, argc, argv);
    REGION.Dmax = atof (argv[N]);
    remove_argument (N, argc, argv);

    if (REGION.Rmin == REGION.Rmax) {
      fprintf (stderr, "ERROR: selected region is ill-defined: Rmin == Rmax\n");
      exit (2);
    }
    if (REGION.Dmin == REGION.Dmax) {
      fprintf (stderr, "ERROR: selected region is ill-defined: Dmin == Dmax\n");
      exit (2);
    }
  }

  if (*argc != 2) usage();

  return (TRUE);
}

void usage() {

  fprintf (stderr, "USAGE: dvocompress (catdir) [-set-compress name] [-set-format name]\n\n");
  fprintf (stderr, " this program takes an existing DVO database and re-writes the tables in the specified compression / formation\n");

  fprintf (stderr, " dvocompress (catdir)\n\n");

  fprintf (stderr, " parallel options:\n");
  fprintf (stderr, "  -parallel:\n");
  fprintf (stderr, "  -parallel-manual\n");
  fprintf (stderr, "  -parallel-serial\n\n");
  fprintf (stderr, " -region Rmin Rmax Dmin Dmax : limit operation to the specified region \n");
  
  fprintf (stderr, " option options:\n");
  fprintf (stderr, " -v : verbose mode\n");
  fprintf (stderr, " -set-compress (catcompress) : NONE, GZIP_1, GZIP_2, RICE_1, AUTO\n");
  fprintf (stderr, " -set-format (catmode)\n");

  exit (2);
}
