# include "dvolens.h"

DvoLensMode args (int argc, char **argv) {

  int N;

  /* specify portion of the sky */
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

  /* specify region file by name (eg n0000/0000.00) */
  UserCatalog = NULL;
  if ((N = get_argument (argc, argv, "-catalog"))) {
    remove_argument (N, &argc, argv);
    UserCatalog = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  VERBOSE = VERBOSE2 = FALSE;
  if ((N = get_argument (argc, argv, "-v"))) {
    VERBOSE = TRUE;
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-vv"))) {
    VERBOSE2 = VERBOSE = TRUE;
    remove_argument (N, &argc, argv);
  }

  REPAIR_LENSING_IDS = FALSE;
  if ((N = get_argument (argc, argv, "-repair-lensing-ids"))) {
    REPAIR_LENSING_IDS = TRUE;
    remove_argument (N, &argc, argv);
  }

  REPAIR_LENSING_IDS_FROM_WARPS = FALSE;
  if ((N = get_argument (argc, argv, "-repair-lensing-ids-from-warps"))) {
    REPAIR_LENSING_IDS_FROM_WARPS = TRUE;
    remove_argument (N, &argc, argv);
  }
  if (REPAIR_LENSING_IDS && REPAIR_LENSING_IDS_FROM_WARPS) {
    fprintf (stderr, "cannot use both -repair-lensing-ids and -repair-lensing-ids-from-warps, exiting\n");
    exit (2);
  }

  NTHREADS = 0;
  if ((N = get_argument (argc, argv, "-threads"))) {
    remove_argument (N, &argc, argv);
    NTHREADS = atof (argv[N]);
    remove_argument (N, &argc, argv);
  }

  // XXX for the moment, make this selection manual.  it needs to be automatic 
  // based on the state of the SkyTable
  HOST_ID = 0;
  PARALLEL = FALSE;
  if ((N = get_argument (argc, argv, "-parallel"))) {
    PARALLEL = TRUE;
    remove_argument (N, &argc, argv);
  }
  // this is a test mode : rather than launching the remote jobs and waiting for completion,
  // dvolens will simply list the remote command and wait for the user to signal completion
  PARALLEL_MANUAL = FALSE;
  if ((N = get_argument (argc, argv, "-parallel-manual"))) {
    PARALLEL = TRUE; // -parallel-manual implies -parallel
    PARALLEL_MANUAL = TRUE;
    remove_argument (N, &argc, argv);
  }
  // this is a test mode : rather than launching the dvolens_client jobs remotely, they are 
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

  DvoLensMode mode = MODE_ERROR;
  if ((N = get_argument (argc, argv, "-update-objects"))) {
    remove_argument (N, &argc, argv);
    mode = MODE_UPDATE_OBJECTS;
  }

  switch (mode) {
    case MODE_UPDATE_OBJECTS:
      if (argc != 1) dvolens_usage();
      break;
      
    default:
      fprintf (stderr, "no valid mode selected\n");
      dvolens_usage();
      break;
  }
  if (argc != 1) dvolens_usage ();

  return mode;
}

int args_client (int argc, char **argv) {

  int N;

  // by definition, the client is not parallel 
  PARALLEL = FALSE;
  PARALLEL_MANUAL = FALSE;
  PARALLEL_SERIAL = FALSE;

  HOST_ID = 0;
  if ((N = get_argument (argc, argv, "-hostID"))) {
    remove_argument (N, &argc, argv);
    HOST_ID = atoi (argv[N]);
    remove_argument (N, &argc, argv);
  }
  if (!HOST_ID) dvolens_client_usage();

  HOSTDIR = NULL;
  if ((N = get_argument (argc, argv, "-hostdir"))) {
    remove_argument (N, &argc, argv);
    HOSTDIR = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }
  if (!HOSTDIR) dvolens_client_usage();

  MODE = MODE_ERROR;
  if ((N = get_argument (argc, argv, "-update-objects"))) {
    MODE = MODE_UPDATE_OBJECTS;
    remove_argument (N, &argc, argv);
  }
  if (!MODE) dvolens_client_usage();

  /* specify portion of the sky */
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

  /* specify region file by name (eg n0000/0000.00) */
  UserCatalog = NULL;
  if ((N = get_argument (argc, argv, "-catalog"))) {
    remove_argument (N, &argc, argv);
    UserCatalog = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  VERBOSE = VERBOSE2 = FALSE;
  if ((N = get_argument (argc, argv, "-v"))) {
    VERBOSE = TRUE;
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-vv"))) {
    VERBOSE2 = VERBOSE = TRUE;
    remove_argument (N, &argc, argv);
  }

  REPAIR_LENSING_IDS = FALSE;
  if ((N = get_argument (argc, argv, "-repair-lensing-ids"))) {
    REPAIR_LENSING_IDS = TRUE;
    remove_argument (N, &argc, argv);
  }

  REPAIR_LENSING_IDS_FROM_WARPS = FALSE;
  if ((N = get_argument (argc, argv, "-repair-lensing-ids-from-warps"))) {
    REPAIR_LENSING_IDS_FROM_WARPS = TRUE;
    remove_argument (N, &argc, argv);
  }
  if (REPAIR_LENSING_IDS && REPAIR_LENSING_IDS_FROM_WARPS) {
    fprintf (stderr, "cannot use both -repair-lensing-ids and -repair-lensing-ids-from-warps, exiting\n");
    exit (2);
  }

  UPDATE = FALSE;
  if ((N = get_argument (argc, argv, "-update"))) {
    remove_argument (N, &argc, argv);
    UPDATE = TRUE;
  }

  if ((MODE == MODE_UPDATE_OBJECTS)  && (argc == 1)) return TRUE;
  if (argc != 2) dvolens_client_usage ();

  return TRUE;
}

