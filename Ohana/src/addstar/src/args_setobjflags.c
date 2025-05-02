# include "addstar.h"
# include "setobjflags.h"

static void help (char *message);
static void help_client (char *message);

int args_setobjflags (int *argc, char **argv) {
  
  int N;

  /* check for help request */
  if (get_argument (*argc, argv, "-help") ||
      get_argument (*argc, argv, "-h")) {
    help (NULL);
  }

  /* specify portion of the sky (useful for testing) */
  UserPatch.Rmin = 0;
  UserPatch.Rmax = 360;
  UserPatch.Dmin = -90;
  UserPatch.Dmax = +90;
  if ((N = get_argument (*argc, argv, "-region"))) {
    remove_argument (N, argc, argv);
    UserPatch.Rmin = atof (argv[N]);
    remove_argument (N, argc, argv);
    UserPatch.Rmax = atof (argv[N]);
    remove_argument (N, argc, argv);
    UserPatch.Dmin = atof (argv[N]);
    remove_argument (N, argc, argv);
    UserPatch.Dmax = atof (argv[N]);
    remove_argument (N, argc, argv);
  }

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

  /* extra error messages */
  VERBOSE = FALSE;
  if ((N = get_argument (*argc, argv, "-v"))) {
    VERBOSE = TRUE;
    remove_argument (N, argc, argv);
  }

  if (*argc != 1) help ("insufficient arguments");
  return TRUE;
}

int args_setobjflags_client (int *argc, char **argv) {
  
  int N;

  /* check for help request */
  if (get_argument (*argc, argv, "-help") ||
      get_argument (*argc, argv, "-h")) {
    help_client (NULL);
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
  if (!HOST_ID) help_client("missing -hostID");

  HOSTDIR = NULL;
  if ((N = get_argument (*argc, argv, "-hostdir"))) {
    remove_argument (N, argc, argv);
    HOSTDIR = strcreate (argv[N]);
    remove_argument (N, argc, argv);
  }
  if (!HOSTDIR) help_client("missing -hostdir");

  // ???
  CPT_FILE = NULL;
  if ((N = get_argument (*argc, argv, "-cpt"))) {
    remove_argument (N, argc, argv);
    CPT_FILE = strcreate (argv[N]);
    remove_argument (N, argc, argv);
  }
  if (!CPT_FILE) help_client("missing cptfile");
  
  INPUT = NULL;
  if ((N = get_argument (*argc, argv, "-input"))) {
    remove_argument (N, argc, argv);
    INPUT = strcreate (argv[N]);
    remove_argument (N, argc, argv);
  }
  if (!INPUT) help_client("missing input file");

  /* we do not allow a subset to be extracted -- all or nothing, babe */
  UserPatch.Rmin = 0;
  UserPatch.Rmax= 360;
  UserPatch.Dmin = -90;
  UserPatch.Dmax = +90;

  /* extra error messages */
  VERBOSE = FALSE;
  if ((N = get_argument (*argc, argv, "-v"))) {
    VERBOSE = TRUE;
    remove_argument (N, argc, argv);
  }

  if (*argc != 1) {
    fprintf (stderr, "USAGE: setobjflags_client -cpt (file) -input (file)\n");
    exit (2);
  }
  return TRUE;
}

static void help (char *message) {

  if (message) fprintf (stderr, "ERROR: %s\n\n", message);

  fprintf (stderr, "USAGE: setobjflags [options]\n");
  fprintf (stderr, "  add flags for specified detection sets\n\n");

  fprintf (stderr, "  options:\n");
  fprintf (stderr, "  -v                    	  : verbose mode\n");
  fprintf (stderr, "  -help                 	  : this list\n");
  fprintf (stderr, "  -h                    	  : this list\n\n");
  exit (2);
}

static void help_client (char *message) {

  if (message) fprintf (stderr, "ERROR: %s\n\n", message);

  fprintf (stderr, "USAGE: setobjflags_client -D CATDIR catdir -hostID ID -hostdir (dir) -cpt (filename) -input (input) [options]\n");
  fprintf (stderr, "  add flags for specified detection sets\n\n");

  fprintf (stderr, "  options:\n");
  fprintf (stderr, "  -v                    	  : verbose mode\n");
  fprintf (stderr, "  -help                 	  : this list\n");
  fprintf (stderr, "  -h                    	  : this list\n\n");
  exit (2);
}
