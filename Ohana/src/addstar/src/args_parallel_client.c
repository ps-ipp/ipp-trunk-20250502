# include "addstar.h"

static void help (void);

// options actually respected by addstar_client
//   options.mode
//   options.existing_regions
//   options.nosort

AddstarClientOptions args_parallel_client (int argc, char **argv, AddstarClientOptions options) {
  
  int N;

  NREFSTAR_GROUP = 1; // not used by -resort
  NSTAR_GROUP = 1; // not used by -resort
  NTHREADS = 0; // for now, addstar_client is not threaded

  /* check for help request */
  if (get_argument (argc, argv, "-help") ||
      get_argument (argc, argv, "-h")) {
    help ();
  }

  memset (&options, 0, sizeof(AddstarClientOptions));

  /* basic mode: image, list, refcat */
  options.mode = ADDSTAR_MODE_NONE;
  if ((N = get_argument (argc, argv, "-resort"))) {
    options.mode = ADDSTAR_MODE_RESORT;
    remove_argument (N, &argc, argv);
  }
  if (options.mode == ADDSTAR_MODE_NONE) {
      fprintf (stderr, "error: no valid addstar_client mode is defined\n");
      exit (2);
  }

  /* restrict to a portion of the sky? (REFCAT only) */
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
  if (!HOST_ID) help();

  HOSTDIR = NULL;
  if ((N = get_argument (argc, argv, "-hostdir"))) {
    remove_argument (N, &argc, argv);
    HOSTDIR = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }
  if (!HOSTDIR) help();

  /*** modify behavior ***/
  /* only add to or resort existing objects */
  options.existing_regions = FALSE;
  if ((N = get_argument (argc, argv, "-existing-regions"))) {
    options.existing_regions = TRUE;
    remove_argument (N, &argc, argv);
  }

  // resort even tables that claim to have been sorted
  if ((N = get_argument (argc, argv, "-force-sort"))) {
    options.nosort = 3;  // temporary mode to mean 'force-sort'
    remove_argument (N, &argc, argv);
  }

  OLD_RESORT = FALSE;
  if ((N = get_argument (argc, argv, "-old-resort"))) {
    remove_argument (N, &argc, argv);
    OLD_RESORT = TRUE;
  }

  /* extra error messages */
  VERBOSE = FALSE;
  if ((N = get_argument (argc, argv, "-v"))) {
    VERBOSE = TRUE;
    remove_argument (N, &argc, argv);
  }

  if ((options.mode == ADDSTAR_MODE_RESORT) && (argc == 1)) return (options);

  fprintf (stderr, "USAGE: addstar_client -resort (SkyRegion)\n");
  exit (2);
}

static void help (void) {

  fprintf (stderr, "USAGE\n");
  fprintf (stderr, "  addstar_client -resort (SkyRegion) -hostID (id) -hostdir (dir)");
  fprintf (stderr, "     perform measure sorting for the specified catalog\n\n");

  fprintf (stderr, "  optional flags:\n");
  fprintf (stderr, "  -region ra ra dec dec 	  : only add data in specified region (-ref mode only)\n");
  fprintf (stderr, "  -existing-regions           : only add measurements to existing catalog files\n");
  fprintf (stderr, "  -nosort             	  : don't re-sort the measure entries (improves speed)\n");
  fprintf (stderr, "  -update             	  : only update the new rows (forces -nosort)\n");
  fprintf (stderr, "  -force-sort             	  : \n");
  fprintf (stderr, "  -v                    	  : verbose mode\n");
  fprintf (stderr, "  -help                 	  : this list\n");
  fprintf (stderr, "  -h                    	  : this list\n\n");

  exit (2);
}

