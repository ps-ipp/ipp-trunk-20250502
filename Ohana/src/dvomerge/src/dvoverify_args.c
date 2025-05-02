# include "dvoverify.h"

int dvoverify_args (int *argc, char **argv) {

  int N;

  NNotSorted = 0;

  VERBOSE = FALSE;
  if ((N = get_argument (*argc, argv, "-v"))) {
    VERBOSE = TRUE;
    remove_argument (N, argc, argv);
  }
  if ((N = get_argument (*argc, argv, "-verbose"))) {
    VERBOSE = TRUE;
    remove_argument (N, argc, argv);
  }

  CHECKSORTED = FALSE;
  if ((N = get_argument (*argc, argv, "-s"))) {
    CHECKSORTED = TRUE;
    remove_argument (N, argc, argv);
  }
  if ((N = get_argument (*argc, argv, "-sorted"))) {
    CHECKSORTED = TRUE;
    remove_argument (N, argc, argv);
  }

  // in some cases, the header sorted state can claim F but is actually T
  IGNORE_SORTED_STATE = FALSE;
  if ((N = get_argument (*argc, argv, "-ignore-sorted-state"))) {
    IGNORE_SORTED_STATE = TRUE;
    remove_argument (N, argc, argv);
  }

  LIST_MISSING = FALSE;
  if ((N = get_argument (*argc, argv, "-list-missing"))) {
    LIST_MISSING = TRUE;
    remove_argument (N, argc, argv);
  }

  if ((N = get_argument (*argc, argv, "-cpt"))) {
    remove_argument (N, argc, argv);
    char *filename = strcreate (argv[N]);
    remove_argument (N, argc, argv);

    if (*argc != 1) {
      fprintf (stderr, "USAGE: dvoverify -cpt filename.cpt\n");
      fprintf (stderr, "OPTIONS: -v -verbose -s -sorted\n");
      fprintf (stderr, "NOTE: -parallel and other options not allowed for -cpt mode\n");
      exit (2);
    }

    CHECK_IMAGE_ID = FALSE;

    int isGood = dvoverify_single (filename);
    free (filename);

    ohana_memcheck (VERBOSE);
    ohana_memdump (VERBOSE);

    if (!isGood) exit (1);
    exit (0);
  }

  CHECK_TOPLEVEL = TRUE;
  if ((N = get_argument (*argc, argv, "-skip-toplevel"))) {
    CHECK_TOPLEVEL = FALSE;
    remove_argument (N, argc, argv);
  }

  /*** dvoverify -cpt should have CHECK_IMAGE_ID = F as default ***/
       
  CHECK_IMAGE_ID = TRUE;
  if ((N = get_argument (*argc, argv, "-skip-image-ids"))) {
    CHECK_IMAGE_ID = FALSE;
    remove_argument (N, argc, argv);
  }

  // restrict to a portion of the sky
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

  HOST_ID = 0;
  HOSTDIR = NULL;

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

  if (*argc != 2) {
    fprintf (stderr, "USAGE: dvoverify (catdir) [-region Rmin Rmax Dmin Dmax] [-v] [-s]\n\n");
    fprintf (stderr, "  (catdir) : database of interest\n");
    fprintf (stderr, "  -v : VERBOSE\n");
    fprintf (stderr, "  -s : checks if sorted, return error if not\n");
    fprintf (stderr, "  -region : limit checks to specified region\n");
    fprintf (stderr, "  -parallel : run in parallel across cluster\n");
    fprintf (stderr, "  -skip-toplevel : do not check top-level files (Images, Photcodes, etc)\n\n");
    fprintf (stderr, "  OR : -cpt (filename)\n");
    exit (2);
  }

  return TRUE;
}

int dvoverify_client_args (int *argc, char **argv) {

  int N;

  VERBOSE = FALSE;
  CHECKSORTED = FALSE;
  NNotSorted = 0;

  if ((N = get_argument (*argc, argv, "-v"))) {
    VERBOSE = TRUE;
    remove_argument (N, argc, argv);
  }
  if ((N = get_argument (*argc, argv, "-s"))) {
    CHECKSORTED = TRUE;
    remove_argument (N, argc, argv);
  }

  LIST_MISSING = FALSE;
  if ((N = get_argument (*argc, argv, "-list-missing"))) {
    LIST_MISSING = TRUE;
    remove_argument (N, argc, argv);
  }

  CHECK_IMAGE_ID = TRUE;
  if ((N = get_argument (*argc, argv, "-skip-image-ids"))) {
    CHECK_IMAGE_ID = FALSE;
    remove_argument (N, argc, argv);
  }

  // restrict to a portion of the sky
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

  // by definition, the client is not parallel 
  PARALLEL = FALSE;
  PARALLEL_MANUAL = FALSE;
  PARALLEL_SERIAL = FALSE;

  HOST_ID = 0;
  if ((N = get_argument (*argc, argv, "-hostID"))) {
    remove_argument (N, argc, argv);
    HOST_ID = atoi (argv[N]);
    remove_argument (N, argc, argv);
  }

  HOSTDIR = NULL;
  if ((N = get_argument (*argc, argv, "-hostdir"))) {
    remove_argument (N, argc, argv);
    HOSTDIR = strcreate (argv[N]);
    remove_argument (N, argc, argv);
  }

  RESULTS = NULL;
  if ((N = get_argument (*argc, argv, "-results"))) {
    remove_argument (N, argc, argv);
    RESULTS = strcreate (argv[N]);
    remove_argument (N, argc, argv);
  }

  // in some cases, the header sorted state can claim F but is actually T
  IGNORE_SORTED_STATE = FALSE;
  if ((N = get_argument (*argc, argv, "-ignore-sorted-state"))) {
    IGNORE_SORTED_STATE = TRUE;
    remove_argument (N, argc, argv);
  }

  if (!HOST_ID || !HOSTDIR || (*argc != 2)) {
    fprintf (stderr, "USAGE: dvoverify_client (catdir) -results (file) -hostID ID -hostdir DIR [-region Rmin Rmax Dmin Dmax] [-v] [-s]\n\n");
    fprintf (stderr, "  (catdir) : database of interest\n");
    fprintf (stderr, "  -v : VERBOSE\n");
    fprintf (stderr, "  -s : checks if sorted, return error if not\n");
    exit (2);
  }

  return TRUE;
}
