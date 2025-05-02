# include "dvomerge.h"

/*** check for command line options ***/
int dvomerge_args (int *argc, char **argv) {
  
  int N;

  HOSTDIR = NULL;
  SINGLE_CPT = NULL;

  /* extra error messages */
  VERBOSE = FALSE;
  if ((N = get_argument (*argc, argv, "-v"))) {
    VERBOSE = TRUE;
    remove_argument (N, argc, argv);
  }

  /* verify merge status of output tables, but do not modify */
  VERIFY = FALSE;
  VERIFY_CATALOG_ONLY = FALSE;
  if ((N = get_argument (*argc, argv, "-verify"))) {
    VERIFY = TRUE;
    remove_argument (N, argc, argv);
  }
  if ((N = get_argument (*argc, argv, "-check-only"))) {
    VERIFY = TRUE;
    remove_argument (N, argc, argv);
  }
  if ((N = get_argument (*argc, argv, "-verify-catalogs"))) {
    VERIFY = TRUE;
    VERIFY_CATALOG_ONLY = TRUE;
    remove_argument (N, argc, argv);
  }

  /* extra error messages */
  IMAGES_ONLY = FALSE;
  if ((N = get_argument (*argc, argv, "-images-only"))) {
    IMAGES_ONLY = TRUE;
    remove_argument (N, argc, argv);
  }
  SKIP_IMAGES = FALSE;
  if ((N = get_argument (*argc, argv, "-skip-images"))) {
    SKIP_IMAGES = TRUE;
    remove_argument (N, argc, argv);
  }

  /* extra error messages */
  MATCHED_TABLES = TRUE;
  if ((N = get_argument (*argc, argv, "-matched-tables"))) {
    MATCHED_TABLES = TRUE;
    remove_argument (N, argc, argv);
  }
  if ((N = get_argument (*argc, argv, "-unmatched-tables"))) {
    MATCHED_TABLES = FALSE;
    remove_argument (N, argc, argv);
  }

  /** allow only certain tables to be merged **/ 
  SKIP_MEASURE = FALSE;
  if ((N = get_argument (*argc, argv, "-skip-measure"))) {
    SKIP_MEASURE = TRUE;
    remove_argument (N, argc, argv);
  }
  SKIP_MISSING = FALSE;
  if ((N = get_argument (*argc, argv, "-skip-missing"))) {
    SKIP_MISSING = TRUE;
    remove_argument (N, argc, argv);
  }
  SKIP_LENSING = FALSE;
  if ((N = get_argument (*argc, argv, "-skip-lensing"))) {
    SKIP_LENSING = TRUE;
    remove_argument (N, argc, argv);
  }
  SKIP_LENSOBJ = FALSE;
  if ((N = get_argument (*argc, argv, "-skip-lensobj"))) {
    SKIP_LENSOBJ = TRUE;
    remove_argument (N, argc, argv);
  }
  SKIP_STARPAR = FALSE;
  if ((N = get_argument (*argc, argv, "-skip-starpar"))) {
    SKIP_STARPAR = TRUE;
    remove_argument (N, argc, argv);
  }
  SKIP_GALPHOT = FALSE;
  if ((N = get_argument (*argc, argv, "-skip-galphot"))) {
    SKIP_GALPHOT = TRUE;
    remove_argument (N, argc, argv);
  }

  if (SKIP_IMAGES && !(SKIP_MEASURE || SKIP_LENSING)) {
    fprintf (stderr, "WARNING: skipping images but merging measure and lensing tables: imageIDs will not be correct\n");
    fprintf (stderr, "type Ctrl-C within 5 seconds to cancel\n");
    for (int i = 5; i > 0; i--) {
      fprintf (stderr, "%d.. ", i);
      usleep (1000000);
    }
    fprintf (stderr, "\n");
  }

  /* extra options */
  RESET_STARPAR = FALSE;
  if ((N = get_argument (*argc, argv, "-reset-starpar"))) {
    RESET_STARPAR = TRUE;
    remove_argument (N, argc, argv);
  }
  RESET_LENSING = FALSE;
  if ((N = get_argument (*argc, argv, "-reset-lensing"))) {
    RESET_LENSING = TRUE;
    remove_argument (N, argc, argv);
  }

  /* match images in the src and tgt database tables by their externID values (otherwise by time and photcode) */
  MATCH_BY_EXTERN_ID = FALSE;
  if ((N = get_argument (*argc, argv, "-match-by-extern-id"))) {
    MATCH_BY_EXTERN_ID = TRUE;
    remove_argument (N, argc, argv);
  }
  /* add objects from input to output database only if they match an existing object */
  ONLY_MATCHES = FALSE;
  if ((N = get_argument (*argc, argv, "-only-matches"))) {
    ONLY_MATCHES = TRUE;
    remove_argument (N, argc, argv);
  }

  /* limit the impact of a dvomerge -parallel */
  MAX_CLIENTS = 10;
  if ((N = get_argument (*argc, argv, "-max-clients"))) {
    remove_argument (N, argc, argv);
    MAX_CLIENTS = atoi(argv[N]);
    remove_argument (N, argc, argv);
  }

  /* accept input database average astrometry motions */
  ACCEPT_MOTION = FALSE;
  if ((N = get_argument (*argc, argv, "-accept-motion"))) {
    remove_argument (N, argc, argv);
    ACCEPT_MOTION = TRUE;
  }
  /* accept input database average astrometry information */
  ACCEPT_ASTROM = FALSE;
  if ((N = get_argument (*argc, argv, "-accept-astrom"))) {
    remove_argument (N, argc, argv);
    ACCEPT_ASTROM = TRUE;
  }

  /* limit the impact of a dvomerge -parallel */
  RETAIN_AVE_PHOTOMETRY = FALSE;
  if ((N = get_argument (*argc, argv, "-retain-ave-photometry"))) {
    remove_argument (N, argc, argv);
    RETAIN_AVE_PHOTOMETRY = TRUE;
    remove_argument (N, argc, argv);
  }

  /* use a different photcode file to define mean values */
  ALTERNATE_PHOTCODE_FILE = NULL;
  if ((N = get_argument (*argc, argv, "-photcode-file"))) {
    remove_argument (N, argc, argv);
    ALTERNATE_PHOTCODE_FILE = strcreate(argv[N]);
    remove_argument (N, argc, argv);
  }

  // merge even if header claims db has already been merged 
  FORCE_MERGE = FALSE;
  if ((N = get_argument (*argc, argv, "-force-merge"))) {
    FORCE_MERGE = TRUE;
    remove_argument (N, argc, argv);
  }

  NCPTLIST = 0;
  CPTLIST = NULL;
  CPTLIST_FILENAME = NULL;
  if ((N = get_argument (*argc, argv, "-restrict-cpt"))) {
    remove_argument (N, argc, argv);
    char *tmppath = strcreate (argv[N]);
    CPTLIST_FILENAME = abspath(tmppath, DVO_MAX_PATH);
    free (tmppath);
    CPTLIST = load_cptlist (CPTLIST_FILENAME, &NCPTLIST);
    remove_argument (N, argc, argv);
  }

  UPDATE_CATFORMAT = NULL;
  if ((N = get_argument (*argc, argv, "-update-catformat"))) {
    remove_argument (N, argc, argv);
    UPDATE_CATFORMAT = strcreate (argv[N]);
    remove_argument (N, argc, argv);
  }
  UPDATE_CATCOMPRESS = NULL;
  if ((N = get_argument (*argc, argv, "-update-catcompress"))) {
    remove_argument (N, argc, argv);
    UPDATE_CATCOMPRESS = strcreate (argv[N]);
    remove_argument (N, argc, argv);
  }

  /* replace measurement, don't duplicate */
  REPLACE_BY_PHOTCODE = FALSE;
  if ((N = get_argument (*argc, argv, "-replace"))) {
    REPLACE_BY_PHOTCODE = TRUE;
    remove_argument (N, argc, argv);
  }

  /* extra error messages */
  ALLOW_MISSING_INPUT_IMAGES = FALSE;
  if ((N = get_argument (*argc, argv, "-allow-missing-input-images"))) {
    ALLOW_MISSING_INPUT_IMAGES = TRUE;
    remove_argument (N, argc, argv);
  }

  /* replace measurement, don't duplicate */
  REPLACE_TYCHO = FALSE;
  if ((N = get_argument (*argc, argv, "-replace-tycho"))) {
    REPLACE_TYCHO = TRUE;
    remove_argument (N, argc, argv);
  }

  REPAIR_BY_OBJID = FALSE;
  if ((N = get_argument (*argc, argv, "-repair-by-objid"))) {
    REPAIR_BY_OBJID = TRUE;
    remove_argument (N, argc, argv);
  }

  NTHREADS = 0;
  if ((N = get_argument (*argc, argv, "-threads"))) {
    remove_argument (N, argc, argv);
    NTHREADS = MAX(0, atoi(argv[N]));
    remove_argument (N, argc, argv);
    myAbort ("threads deprecated for now");
  }

  /*** provide additional data ***/ 
  /* restrict to a portion of the sky? */
  UserPatch.Rmin = 0;
  UserPatch.Rmax= 360;
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

  // is the input database a parallel db?
  PARALLEL_INPUT = FALSE;
  if ((N = get_argument (*argc, argv, "-parallel-input"))) {
    PARALLEL_INPUT = TRUE;
    remove_argument (N, argc, argv);
  }

  // is the output database a parallel db?
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

  if ((*argc < 4) || (*argc > 6)) dvomerge_usage();
  return TRUE;
}

void dvomerge_args_free (void) {
  FREE (HOSTDIR);
  FREE (ALTERNATE_PHOTCODE_FILE);
  FREE (UPDATE_CATFORMAT);
  FREE (UPDATE_CATCOMPRESS);
  FREE (SINGLE_CPT);
  FREE (CPTLIST_FILENAME);

  int i;
  for (i = 0; i < NCPTLIST; i++) {
    FREE (CPTLIST[i]);
  }
  FREE (CPTLIST);
}

/*** check for command line options ***/
int dvomerge_client_args (int *argc, char **argv) {
  
  int N;

  // by definition, the client is not parallel 
  PARALLEL = FALSE;
  PARALLEL_MANUAL = FALSE;
  PARALLEL_SERIAL = FALSE;
  IMAGES_ONLY = FALSE;

  // is the input database a parallel db?
  PARALLEL_INPUT = FALSE;
  if ((N = get_argument (*argc, argv, "-parallel-input"))) {
    PARALLEL_INPUT = TRUE;
    remove_argument (N, argc, argv);
  }

  HOST_ID = 0;
  if ((N = get_argument (*argc, argv, "-hostID"))) {
    remove_argument (N, argc, argv);
    HOST_ID = atoi (argv[N]);
    remove_argument (N, argc, argv);
  }
  if (!HOST_ID) dvomerge_client_usage();

  HOSTDIR = NULL;
  if ((N = get_argument (*argc, argv, "-hostdir"))) {
    remove_argument (N, argc, argv);
    HOSTDIR = strcreate (argv[N]);
    remove_argument (N, argc, argv);
  }
  if (!HOSTDIR) dvomerge_client_usage();

  /* extra error messages */
  VERBOSE = FALSE;
  if ((N = get_argument (*argc, argv, "-v"))) {
    VERBOSE = TRUE;
    remove_argument (N, argc, argv);
  }

  /* verify merge status of output tables, but do not modify */
  VERIFY = FALSE;
  if ((N = get_argument (*argc, argv, "-verify"))) {
    VERIFY = TRUE;
    remove_argument (N, argc, argv);
  }
  if ((N = get_argument (*argc, argv, "-check-only"))) {
    VERIFY = TRUE;
    remove_argument (N, argc, argv);
  }
  if ((N = get_argument (*argc, argv, "-verify-catalogs"))) {
    VERIFY = TRUE;
    VERIFY_CATALOG_ONLY = TRUE;
    remove_argument (N, argc, argv);
  }

  // merge even if header claims db has already been merged 
  FORCE_MERGE = FALSE;
  if ((N = get_argument (*argc, argv, "-force-merge"))) {
    FORCE_MERGE = TRUE;
    remove_argument (N, argc, argv);
  }

  /* accept input database average astrometry motions */
  ACCEPT_MOTION = FALSE;
  if ((N = get_argument (*argc, argv, "-accept-motion"))) {
    remove_argument (N, argc, argv);
    ACCEPT_MOTION = TRUE;
  }
  /* accept input database average astrometry information */
  ACCEPT_ASTROM = FALSE;
  if ((N = get_argument (*argc, argv, "-accept-astrom"))) {
    remove_argument (N, argc, argv);
    ACCEPT_ASTROM = TRUE;
  }

  /* limit the impact of a dvomerge -parallel */
  RETAIN_AVE_PHOTOMETRY = FALSE;
  if ((N = get_argument (*argc, argv, "-retain-ave-photometry"))) {
    remove_argument (N, argc, argv);
    RETAIN_AVE_PHOTOMETRY = TRUE;
    remove_argument (N, argc, argv);
  }

  /* extra error messages */
  MATCHED_TABLES = FALSE;
  if ((N = get_argument (*argc, argv, "-matched-tables"))) {
    MATCHED_TABLES = TRUE;
    remove_argument (N, argc, argv);
  }

  /** allow only certain tables to be merged **/ 
  SKIP_MEASURE = FALSE;
  if ((N = get_argument (*argc, argv, "-skip-measure"))) {
    SKIP_MEASURE = TRUE;
    remove_argument (N, argc, argv);
  }
  SKIP_MISSING = FALSE;
  if ((N = get_argument (*argc, argv, "-skip-missing"))) {
    SKIP_MISSING = TRUE;
    remove_argument (N, argc, argv);
  }
  SKIP_LENSING = FALSE;
  if ((N = get_argument (*argc, argv, "-skip-lensing"))) {
    SKIP_LENSING = TRUE;
    remove_argument (N, argc, argv);
  }
  SKIP_LENSOBJ = FALSE;
  if ((N = get_argument (*argc, argv, "-skip-lensobj"))) {
    SKIP_LENSOBJ = TRUE;
    remove_argument (N, argc, argv);
  }
  SKIP_STARPAR = FALSE;
  if ((N = get_argument (*argc, argv, "-skip-starpar"))) {
    SKIP_STARPAR = TRUE;
    remove_argument (N, argc, argv);
  }
  SKIP_GALPHOT = FALSE;
  if ((N = get_argument (*argc, argv, "-skip-galphot"))) {
    SKIP_GALPHOT = TRUE;
    remove_argument (N, argc, argv);
  }

  /* extra error messages */
  RESET_STARPAR = FALSE;
  if ((N = get_argument (*argc, argv, "-reset-starpar"))) {
    RESET_STARPAR = TRUE;
    remove_argument (N, argc, argv);
  }
  RESET_LENSING = FALSE;
  if ((N = get_argument (*argc, argv, "-reset-lensing"))) {
    RESET_LENSING = TRUE;
    remove_argument (N, argc, argv);
  }

  /* match images in the src and tgt database tables by their externID values (otherwise by time and photcode) */
  MATCH_BY_EXTERN_ID = FALSE;
  if ((N = get_argument (*argc, argv, "-match-by-extern-id"))) {
    MATCH_BY_EXTERN_ID = TRUE;
    remove_argument (N, argc, argv);
  }
  /* add objects from input to output database only if they match an existing object */
  ONLY_MATCHES = FALSE;
  if ((N = get_argument (*argc, argv, "-only-matches"))) {
    ONLY_MATCHES = TRUE;
    remove_argument (N, argc, argv);
  }

  /* extra error messages */
  ALLOW_MISSING_INPUT_IMAGES = FALSE;
  if ((N = get_argument (*argc, argv, "-allow-missing-input-images"))) {
    ALLOW_MISSING_INPUT_IMAGES = TRUE;
    remove_argument (N, argc, argv);
  }

  NCPTLIST = 0;
  CPTLIST = NULL;
  if ((N = get_argument (*argc, argv, "-restrict-cpt"))) {
    remove_argument (N, argc, argv);
    CPTLIST = load_cptlist (argv[N], &NCPTLIST);
    remove_argument (N, argc, argv);
  }

  UPDATE_CATFORMAT = NULL;
  if ((N = get_argument (*argc, argv, "-update-catformat"))) {
    remove_argument (N, argc, argv);
    UPDATE_CATFORMAT = strcreate (argv[N]);
    remove_argument (N, argc, argv);
  }
  UPDATE_CATCOMPRESS = NULL;
  if ((N = get_argument (*argc, argv, "-update-catcompress"))) {
    remove_argument (N, argc, argv);
    UPDATE_CATCOMPRESS = strcreate (argv[N]);
    remove_argument (N, argc, argv);
  }

  /* replace measurement, don't duplicate */
  REPLACE_BY_PHOTCODE = FALSE;
  if ((N = get_argument (*argc, argv, "-replace"))) {
    REPLACE_BY_PHOTCODE = TRUE;
    remove_argument (N, argc, argv);
  }

  /* replace measurement, don't duplicate */
  REPLACE_TYCHO = FALSE;
  if ((N = get_argument (*argc, argv, "-replace-tycho"))) {
    REPLACE_TYCHO = TRUE;
    remove_argument (N, argc, argv);
  }

  REPAIR_BY_OBJID = FALSE;
  if ((N = get_argument (*argc, argv, "-repair-by-objid"))) {
    REPAIR_BY_OBJID = TRUE;
    remove_argument (N, argc, argv);
  }

  NTHREADS = 0;
  if ((N = get_argument (*argc, argv, "-threads"))) {
    remove_argument (N, argc, argv);
    NTHREADS = MAX(0, atoi(argv[N]));
    remove_argument (N, argc, argv);
    myAbort ("threads deprecated for now");
  }

  /*** provide additional data ***/ 
  /* restrict to a portion of the sky? */
  UserPatch.Rmin = 0;
  UserPatch.Rmax= 360;
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

  if ((*argc < 4) || (*argc > 6)) dvomerge_client_usage();
  return TRUE;
}

void dvomerge_client_args_free (void) {
  FREE (HOSTDIR);
  FREE (UPDATE_CATFORMAT);
  FREE (UPDATE_CATCOMPRESS);
  FREE (CPTLIST_FILENAME);

  int i;
  for (i = 0; i < NCPTLIST; i++) {
    FREE (CPTLIST[i]);
  }
  FREE (CPTLIST);
}

/*** check for command line options ***/
int dvoconvert_args (int *argc, char **argv) {
  
  int N;

  /* extra error messages */
  VERBOSE = FALSE;
  if ((N = get_argument (*argc, argv, "-v"))) {
    VERBOSE = TRUE;
    remove_argument (N, argc, argv);
  }

  if (*argc != 4) dvoconvert_usage();
  return TRUE;
}

/*** check for command line options ***/
int dvosecfilt_args (int *argc, char **argv) {
  
  int N;

  HOST_ID = 0;

  /* extra error messages */
  VERBOSE = FALSE;
  if ((N = get_argument (*argc, argv, "-v"))) {
    VERBOSE = TRUE;
    remove_argument (N, argc, argv);
  }

  SINGLE_CPT = NULL;
  if ((N = get_argument (*argc, argv, "-cpt"))) {
    remove_argument (N, argc, argv);
    SINGLE_CPT = strcreate (argv[N]);
    remove_argument (N, argc, argv);
  }

  /* specify portion of the sky : allow default of all sky? */
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

  // is the output database a parallel db?
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

  if (*argc != 3) dvosecfilt_usage();
  return TRUE;
}

/*** check for command line options ***/
int dvosecfilt_client_args (int *argc, char **argv) {
  
  int N;

  // by definition, the client is not parallel 
  PARALLEL = FALSE;
  PARALLEL_MANUAL = FALSE;
  PARALLEL_SERIAL = FALSE;

  SINGLE_CPT = NULL;
  if ((N = get_argument (*argc, argv, "-cpt"))) {
    remove_argument (N, argc, argv);
    SINGLE_CPT = strcreate (argv[N]);
    remove_argument (N, argc, argv);
  }

  /* specify portion of the sky : allow default of all sky? */
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
  if ((N = get_argument (*argc, argv, "-hostID"))) {
    remove_argument (N, argc, argv);
    HOST_ID = atoi (argv[N]);
    remove_argument (N, argc, argv);
  }
  if (!HOST_ID) dvosecfilt_client_usage();

  HOSTDIR = NULL;
  if ((N = get_argument (*argc, argv, "-hostdir"))) {
    remove_argument (N, argc, argv);
    HOSTDIR = strcreate (argv[N]);
    remove_argument (N, argc, argv);
  }
  if (!HOSTDIR) dvosecfilt_client_usage();

  /* extra error messages */
  VERBOSE = FALSE;
  if ((N = get_argument (*argc, argv, "-v"))) {
    VERBOSE = TRUE;
    remove_argument (N, argc, argv);
  }

  if (*argc != 3) dvosecfilt_client_usage();
  return TRUE;
}
