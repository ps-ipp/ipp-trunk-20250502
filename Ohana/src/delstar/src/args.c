# include "delstar.h"

void help () {

  fprintf (stderr, "USAGE:\n");
  fprintf (stderr, "  delstar -file (filename)\n");
  fprintf (stderr, "  delstar -name (imagename)\n");
  fprintf (stderr, "  delstar -time (start) (stop/range)\n");
  fprintf (stderr, "  delstar -orphan (region)\n");
  fprintf (stderr, "  delstar -missed (region)\n\n");
  fprintf (stderr, "  delstar -photcodes (list) : delete by photcode\n\n");
  fprintf (stderr, "  delstar -dup-images : delete duplicate images (by externID)\n\n");
  fprintf (stderr, "  delstar -dup-measures : delete duplicate measures (by imageID + detID)\n\n");
  fprintf (stderr, "  delstar -delete-measures-by-match : delete duplicate measures by imageID, photcode, time constratins\n\n");
  fprintf (stderr, "  optional flags:\n");
  fprintf (stderr, "  -v               : verbose mode\n");
  fprintf (stderr, "  -photcode (code) : restrict by photcode\n");
  fprintf (stderr, "  -update : apply changes (-dup-images or -dup-measures only)\n");
  fprintf (stderr, "  -image-details : list info about the deleted images (-dup-images only)\n");
  fprintf (stderr, "  -image-only : only examine the image table (changes are NOT saved; -dup-images only)\n");
  fprintf (stderr, "  -image-only-force : modify only the image table (-dup-images only)\n");
  fprintf (stderr, "  -image-by-obstime : use date/time and photcode (not externID) to find duplicates\n");
  fprintf (stderr, "  -region Rmin Rmax Dmin Dmax : apply changes to this part of the sky\n");
  
  fprintf (stderr, "\n"); 
  exit (2);

}

void usage () {
  fprintf (stderr, "USAGE: delstar (filename) / [optional mode] : -h for help\n");
  exit (2);
}

void delstar_client_usage () {
  fprintf (stderr, "USAGE: delstar_client [options]\n");
  exit (2);
}

int args (int argc, char **argv) {
  
  int N;
  double trange;

  /* check for help request */
  if (get_argument (argc, argv, "-help") ||
      get_argument (argc, argv, "-h")) {
    help ();
  }

  VERBOSE = FALSE;
  if ((N = get_argument (argc, argv, "-v"))) {
    VERBOSE = TRUE;
    remove_argument (N, &argc, argv);
  }

  VERBOSE2 = FALSE;
  if ((N = get_argument (argc, argv, "-vv"))) {
    VERBOSE = TRUE;
    VERBOSE2 = TRUE;
    remove_argument (N, &argc, argv);
  }

  UPDATE = FALSE;
  if ((N = get_argument (argc, argv, "-update"))) {
    UPDATE = TRUE;
    remove_argument (N, &argc, argv);
  }

  IMAGE_DETAILS = FALSE;
  if ((N = get_argument (argc, argv, "-image-details"))) {
    IMAGE_DETAILS = TRUE;
    remove_argument (N, &argc, argv);
  }

  IMAGE_DUPLICATES_BY_OBSTIME = FALSE;
  if ((N = get_argument (argc, argv, "-image-by-obstime"))) {
    IMAGE_DUPLICATES_BY_OBSTIME = TRUE;
    remove_argument (N, &argc, argv);
  }

  SKIP_DIFF_PAIRS = FALSE;
  if ((N = get_argument (argc, argv, "-skip-diff-pairs"))) {
    SKIP_DIFF_PAIRS = TRUE;
    remove_argument (N, &argc, argv);
  }

  // We should generally not delete the images before the measures (or we won't know what
  // to delete). -image-only is for testing, -image-only-force -update WILL delete the images 
  IMAGE_ONLY = FALSE;
  if ((N = get_argument (argc, argv, "-image-only"))) {
    IMAGE_ONLY = TRUE;
    UPDATE = FALSE;
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-image-only-force"))) {
    IMAGE_ONLY = TRUE;
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

  SKIP_IMAGES = FALSE;
  if ((N = get_argument (argc, argv, "-skip-images"))) {
    SKIP_IMAGES = TRUE;
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
  // delstar will simply list the remote command and wait for the user to signal completion
  PARALLEL_MANUAL = FALSE;
  if ((N = get_argument (argc, argv, "-parallel-manual"))) {
    PARALLEL = TRUE; // -parallel-manual implies -parallel
    PARALLEL_MANUAL = TRUE;
    remove_argument (N, &argc, argv);
  }
  // this is a test mode : rather than launching the delstar_client jobs remotely, they are 
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

  IMAGENAME = NULL;
  MODE = MODE_NONE;
  if ((N = get_argument (argc, argv, "-name"))) {
    if (MODE != MODE_NONE) usage();
    MODE = MODE_IMAGENAME;
    remove_argument (N, &argc, argv);
    IMAGENAME = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-file"))) {
    if (MODE != MODE_NONE) usage();
    MODE = MODE_IMAGENAME;
    remove_argument (N, &argc, argv);
    IMAGENAME = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-orphan"))) {
    if (MODE != MODE_NONE) usage();
    MODE = MODE_ORPHAN;
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-missed"))) {
    if (MODE != MODE_NONE) usage();
    MODE = MODE_MISSED;
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-dup-images"))) {
    if (MODE != MODE_NONE) usage();
    MODE = MODE_DUP_IMAGES;
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-dup-measures"))) {
    if (MODE != MODE_NONE) usage();
    MODE = MODE_DUP_MEASURES;
    remove_argument (N, &argc, argv);
    SKIP_IMAGES = TRUE; // we do not need to load the images for -dup-measures
  }
  if ((N = get_argument (argc, argv, "-delete-measures-by-match"))) {
    if (MODE != MODE_NONE) usage();
    MODE = MODE_DELETE_MEASURES_BY_MATCH;
    remove_argument (N, &argc, argv);
    SKIP_IMAGES = TRUE; // we do not need to load the images for -dup-measures
  }

  DELLIST_FILENAME = NULL;
  if ((N = get_argument (argc, argv, "-delete-measures-by-detID"))) {
    if (MODE != MODE_NONE) usage();
    MODE = MODE_DELETE_MEASURES_BY_DETID;
    remove_argument (N, &argc, argv);
    SKIP_IMAGES = TRUE; // we do not need to load the images for -dup-measures
    DELLIST_FILENAME = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  DELETE_MIN_DET_ID = 0;
  DELETE_MAX_DET_ID = 0;
  DELETE_MIN_CAT_ID = 0;
  DELETE_MAX_CAT_ID = 0;
  DELETE_MIN_IMAGE_ID = 0;
  DELETE_MAX_IMAGE_ID = 0;
  DELETE_MIN_PHOTCODE = 0;
  DELETE_MAX_PHOTCODE = 0;
  DELETE_MIN_TIME = 0;
  DELETE_MAX_TIME = 0;

  if ((N = get_argument (argc, argv, "-delete-min-detID")))  { remove_argument (N, &argc, argv); DELETE_MIN_DET_ID = atoi(argv[N]); remove_argument (N, &argc, argv); }
  if ((N = get_argument (argc, argv, "-delete-max-detID")))  { remove_argument (N, &argc, argv); DELETE_MAX_DET_ID = atoi(argv[N]); remove_argument (N, &argc, argv); }
  if ((N = get_argument (argc, argv, "-delete-min-catID")))  { remove_argument (N, &argc, argv); DELETE_MIN_CAT_ID = atoi(argv[N]); remove_argument (N, &argc, argv); }
  if ((N = get_argument (argc, argv, "-delete-max-catID")))  { remove_argument (N, &argc, argv); DELETE_MAX_CAT_ID = atoi(argv[N]); remove_argument (N, &argc, argv); }
  if ((N = get_argument (argc, argv, "-delete-min-imageID")))  { remove_argument (N, &argc, argv); DELETE_MIN_IMAGE_ID = atoi(argv[N]); remove_argument (N, &argc, argv); }
  if ((N = get_argument (argc, argv, "-delete-max-imageID")))  { remove_argument (N, &argc, argv); DELETE_MAX_IMAGE_ID = atoi(argv[N]); remove_argument (N, &argc, argv); }
  if ((N = get_argument (argc, argv, "-delete-min-photcode"))) { remove_argument (N, &argc, argv); DELETE_MIN_PHOTCODE = atoi(argv[N]); remove_argument (N, &argc, argv); }
  if ((N = get_argument (argc, argv, "-delete-max-photcode"))) { remove_argument (N, &argc, argv); DELETE_MAX_PHOTCODE = atoi(argv[N]); remove_argument (N, &argc, argv); }
  if ((N = get_argument (argc, argv, "-delete-min-time")))     { remove_argument (N, &argc, argv); DELETE_MIN_TIME = ohana_date_to_sec(argv[N]); remove_argument (N, &argc, argv); }
  if ((N = get_argument (argc, argv, "-delete-max-time")))     { remove_argument (N, &argc, argv); DELETE_MAX_TIME = ohana_date_to_sec(argv[N]); remove_argument (N, &argc, argv); }

  SAVE_DUPLICATES = FALSE;
  if ((N = get_argument (argc, argv, "-save-duplicates"))) {
    SAVE_DUPLICATES = TRUE;
    remove_argument (N, &argc, argv);
  }

  BACKUP_EXTNAME = NULL;
  if ((N = get_argument (argc, argv, "-backup-extname"))) {
    remove_argument (N, &argc, argv);
    BACKUP_EXTNAME = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }
  if (!BACKUP_EXTNAME) BACKUP_EXTNAME = strcreate (".bck");

  SAVE_DELETES = FALSE;
  if ((N = get_argument (argc, argv, "-save-deletes"))) {
    SAVE_DELETES = TRUE;
    remove_argument (N, &argc, argv);
  }

  if ((N = get_argument (argc, argv, "-fix-LAP"))) {
    if (MODE != MODE_NONE) usage();
    MODE = MODE_FIX_LAP;
    remove_argument (N, &argc, argv);
  }
  UNIQUER = NULL;
  if ((N = get_argument (argc, argv, "-fix-LAP-imstats"))) {
    if (MODE != MODE_NONE) usage();
    MODE = MODE_FIX_LAP_STATS;
    remove_argument (N, &argc, argv);
    if (N == argc) {
      fprintf (stderr, "USAGE: delstar -fix-LAP-imstats (uniquer)\n");
      exit (2);
    }
    UNIQUER = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-fix-LAP-edges"))) {
    if (MODE != MODE_NONE) usage();
    MODE = MODE_FIX_LAP_EDGES;
    remove_argument (N, &argc, argv);
    SKIP_IMAGES = TRUE;
  }
  EDGE_DELETIONS = NULL;
  if ((N = get_argument (argc, argv, "-fix-LAP-edges-delete"))) {
    if (MODE != MODE_NONE) usage(); 
    MODE = MODE_FIX_LAP_EDGES_DELETE;
    remove_argument (N, &argc, argv);
    if (N == argc) {
      fprintf (stderr, "USAGE: delstar -fix-LAP-edges-delete (deletions)\n");
      exit (2);
    }
    EDGE_DELETIONS = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
    SKIP_IMAGES = TRUE;
  }
  if ((N = get_argument (argc, argv, "-photcodes"))) {
    if (MODE != MODE_NONE) usage();
    MODE = MODE_PHOTCODES;
    remove_argument (N, &argc, argv);
    PHOTCODE_LIST = strcreate(argv[N]);
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-time"))) {
    if (MODE != MODE_NONE) usage();
    MODE = MODE_TIME;
    remove_argument (N, &argc, argv);

    if (!ohana_str_to_time (argv[N], &START)) usage ();
    remove_argument (N, &argc, argv);

    /* interpret second value */
    if (ohana_str_to_dtime (argv[N], &trange)) { 
      if (trange < 0) {
	END = START;
	START = END + trange;
      } else {
	END = START + trange;
      }
      remove_argument (N, &argc, argv);
      goto goodtime;
    }
    if (ohana_str_to_time (argv[N], &END)) { 
      if (START > END) {
	time_t tmp;
	tmp   = START;
	START = END;
	END   = tmp;
      }
      remove_argument (N, &argc, argv);
      goto goodtime;
    }
    usage ();
  }

  IMSTATS_FILE = NULL;
  if ((MODE == MODE_FIX_LAP) && (!PARALLEL)) {
    IMSTATS_FILE = strcreate ("delstar.fixLAP.stats.fits");
  }

goodtime:

  if ((N = get_argument (argc, argv, "-cpt"))) {
    remove_argument (N, &argc, argv);
    char *filename = strcreate (argv[N]);
    remove_argument (N, &argc, argv);

    switch (MODE) {
      case MODE_IMAGEFILE:
      case MODE_IMAGENAME:
      case MODE_TIME:
      case MODE_ORPHAN:
      case MODE_MISSED:
	fprintf (stderr, "mode not available for single cpt deletion\n");
	break;
      case MODE_PHOTCODES: {
	if (!delete_photcodes_single (filename)) {
	  fprintf (stderr, "failure deleting measurements\n");
	  exit (1);
	}
	exit (0);
      }
      default:
	usage ();
    }
    exit (0);
  }

  /* restrict to a single photcode (not compatible with -image) 
     PHOTCODE = NULL;
     if ((N = get_argument (argc, argv, "-photcode"))) {
     remove_argument (N, &argc, argv);
     PHOTCODE = GetPhotcodebyName (argv[N]);
     remove_argument (N, &argc, argv);
     }*/

  if (MODE == MODE_NONE) usage ();

  if (argc != 1) usage ();
  return (TRUE);
}

void delstar_args_free () {
  FREE (SINGLE_CPT);
  FREE (IMAGENAME);
  FREE (UNIQUER);
  FREE (CATDIR);
  FREE (EDGE_DELETIONS);
  FREE (PHOTCODE_LIST);
  FREE (IMSTATS_FILE);
  FREE (DELLIST_FILENAME);
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
  if (!HOST_ID) delstar_client_usage();

  HOSTDIR = NULL;
  if ((N = get_argument (argc, argv, "-hostdir"))) {
    remove_argument (N, &argc, argv);
    HOSTDIR = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }
  if (!HOSTDIR) delstar_client_usage();

  /* check for help request */
  if (get_argument (argc, argv, "-help") ||
      get_argument (argc, argv, "-h")) {
    help ();
  }

  VERBOSE = FALSE;
  if ((N = get_argument (argc, argv, "-v"))) {
    VERBOSE = TRUE;
    remove_argument (N, &argc, argv);
  }

  VERBOSE2 = FALSE;
  if ((N = get_argument (argc, argv, "-v"))) {
    VERBOSE = TRUE;
    VERBOSE2 = TRUE;
    remove_argument (N, &argc, argv);
  }

  UPDATE = FALSE;
  if ((N = get_argument (argc, argv, "-update"))) {
    UPDATE = TRUE;
    remove_argument (N, &argc, argv);
  }

  IMAGE_DETAILS = FALSE;
  if ((N = get_argument (argc, argv, "-image-details"))) {
    IMAGE_DETAILS = TRUE;
    remove_argument (N, &argc, argv);
  }

  IMAGE_ONLY = FALSE;
  if ((N = get_argument (argc, argv, "-image-only"))) {
    IMAGE_ONLY = TRUE;
    UPDATE = FALSE;
    remove_argument (N, &argc, argv);
  }

  SINGLE_CPT = NULL;
  if ((N = get_argument (argc, argv, "-cpt"))) {
    remove_argument (N, &argc, argv);
    SINGLE_CPT = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

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

  IMAGES = NULL;
  if ((N = get_argument (argc, argv, "-images"))) {
    remove_argument (N, &argc, argv);
    IMAGES = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  MODE = MODE_NONE;
  if ((N = get_argument (argc, argv, "-photcodes"))) {
    if (MODE != MODE_NONE) delstar_client_usage();
    MODE = MODE_PHOTCODES;
    remove_argument (N, &argc, argv);
    PHOTCODE_LIST = strcreate(argv[N]);
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-dup-images"))) {
    if (MODE != MODE_NONE) usage();
    MODE = MODE_DUP_IMAGES;
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-dup-measures"))) {
    if (MODE != MODE_NONE) usage();
    MODE = MODE_DUP_MEASURES;
    remove_argument (N, &argc, argv);
  }

  if ((N = get_argument (argc, argv, "-delete-measures-by-match"))) {
    if (MODE != MODE_NONE) usage();
    MODE = MODE_DELETE_MEASURES_BY_MATCH;
    remove_argument (N, &argc, argv);
    SKIP_IMAGES = TRUE; // we do not need to load the images for -dup-measures
  }

  DELLIST_FILENAME = NULL;
  if ((N = get_argument (argc, argv, "-delete-measures-by-detID"))) {
    if (MODE != MODE_NONE) usage();
    MODE = MODE_DELETE_MEASURES_BY_DETID;
    remove_argument (N, &argc, argv);
    SKIP_IMAGES = TRUE; // we do not need to load the images for -dup-measures
    DELLIST_FILENAME = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  DELETE_MIN_DET_ID = 0;
  DELETE_MAX_DET_ID = 0;
  DELETE_MIN_CAT_ID = 0;
  DELETE_MAX_CAT_ID = 0;
  DELETE_MIN_IMAGE_ID = 0;
  DELETE_MAX_IMAGE_ID = 0;
  DELETE_MIN_PHOTCODE = 0;
  DELETE_MAX_PHOTCODE = 0;
  DELETE_MIN_TIME = 0;
  DELETE_MAX_TIME = 0;

  if ((N = get_argument (argc, argv, "-delete-min-detID")))  { remove_argument (N, &argc, argv); DELETE_MIN_DET_ID = atoi(argv[N]); remove_argument (N, &argc, argv); }
  if ((N = get_argument (argc, argv, "-delete-max-detID")))  { remove_argument (N, &argc, argv); DELETE_MAX_DET_ID = atoi(argv[N]); remove_argument (N, &argc, argv); }
  if ((N = get_argument (argc, argv, "-delete-min-catID")))  { remove_argument (N, &argc, argv); DELETE_MIN_CAT_ID = atoi(argv[N]); remove_argument (N, &argc, argv); }
  if ((N = get_argument (argc, argv, "-delete-max-catID")))  { remove_argument (N, &argc, argv); DELETE_MAX_CAT_ID = atoi(argv[N]); remove_argument (N, &argc, argv); }
  if ((N = get_argument (argc, argv, "-delete-min-imageID")))  { remove_argument (N, &argc, argv); DELETE_MIN_IMAGE_ID = atoi(argv[N]); remove_argument (N, &argc, argv); }
  if ((N = get_argument (argc, argv, "-delete-max-imageID")))  { remove_argument (N, &argc, argv); DELETE_MAX_IMAGE_ID = atoi(argv[N]); remove_argument (N, &argc, argv); }
  if ((N = get_argument (argc, argv, "-delete-min-photcode"))) { remove_argument (N, &argc, argv); DELETE_MIN_PHOTCODE = atoi(argv[N]); remove_argument (N, &argc, argv); }
  if ((N = get_argument (argc, argv, "-delete-max-photcode"))) { remove_argument (N, &argc, argv); DELETE_MAX_PHOTCODE = atoi(argv[N]); remove_argument (N, &argc, argv); }
  if ((N = get_argument (argc, argv, "-delete-min-time")))     { remove_argument (N, &argc, argv); DELETE_MIN_TIME = ohana_date_to_sec(argv[N]); remove_argument (N, &argc, argv); }
  if ((N = get_argument (argc, argv, "-delete-max-time")))     { remove_argument (N, &argc, argv); DELETE_MAX_TIME = ohana_date_to_sec(argv[N]); remove_argument (N, &argc, argv); }

  SAVE_DELETES = FALSE;
  if ((N = get_argument (argc, argv, "-save-deletes"))) {
    SAVE_DELETES = TRUE;
    remove_argument (N, &argc, argv);
  }
  BACKUP_EXTNAME = NULL;
  if ((N = get_argument (argc, argv, "-backup-extname"))) {
    remove_argument (N, &argc, argv);
    BACKUP_EXTNAME = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }
  if (!BACKUP_EXTNAME) BACKUP_EXTNAME = strcreate (".bck");

  if ((N = get_argument (argc, argv, "-fix-LAP"))) {
    if (MODE != MODE_NONE) usage();
    MODE = MODE_FIX_LAP;
    remove_argument (N, &argc, argv);
    if (!IMAGES) delstar_client_usage();
  }
  if ((N = get_argument (argc, argv, "-fix-LAP-edges"))) {
    if (MODE != MODE_NONE) usage();
    MODE = MODE_FIX_LAP_EDGES;
    remove_argument (N, &argc, argv);
  }
  EDGE_DELETIONS = NULL;
  if ((N = get_argument (argc, argv, "-fix-LAP-edges-delete"))) {
    if (MODE != MODE_NONE) usage();
    MODE = MODE_FIX_LAP_EDGES_DELETE;
    remove_argument (N, &argc, argv);
    if (N == argc) {
      fprintf (stderr, "USAGE: delstar -fix-LAP-edges-delete (deletions)\n");
      exit (2);
    }
    EDGE_DELETIONS = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
    SKIP_IMAGES = TRUE;
  }

  SAVE_DUPLICATES = FALSE;
  if ((N = get_argument (argc, argv, "-save-duplicates"))) {
    SAVE_DUPLICATES = TRUE;
    remove_argument (N, &argc, argv);
  }

  MEASURE_EDGE_FILE = NULL;
  if (MODE == MODE_FIX_LAP_EDGES) {
    N = get_argument (argc, argv, "-measures");
    if (N == 0) {
      fprintf (stderr, "delstar_client -fix-LAP-edge needs -measures (FILE)\n");
      exit (2);
    }
    remove_argument (N, &argc, argv);
    if (N == argc) {
      fprintf (stderr, "USAGE: delstar -fix-LAP-edges -measures (FILE) [FILE missing]\n");
      exit (2);
    }
    MEASURE_EDGE_FILE = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  IMSTATS_FILE = NULL;
  if (MODE == MODE_FIX_LAP) {
    N = get_argument (argc, argv, "-imstats");
    if (N == 0) {
      fprintf (stderr, "delstar_client -fix-LAP needs -imstats (FILE)\n");
      exit (2);
    }
    remove_argument (N, &argc, argv);
    IMSTATS_FILE = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  if (MODE == MODE_NONE) delstar_client_usage ();
  return (TRUE);
}

void delstar_client_args_free () {
  FREE (CATDIR);
  FREE (HOSTDIR);
  FREE (SINGLE_CPT);
  FREE (IMAGES);
  FREE (PHOTCODE_LIST);
  FREE (EDGE_DELETIONS);
  FREE (MEASURE_EDGE_FILE);
  FREE (IMSTATS_FILE);
  FREE (DELLIST_FILENAME);
}

