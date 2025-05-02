# include "fakeastro.h"
void usage (void);
void usage_client (void);

int args (int *argc, char **argv) {

  int N;

  /* possible operations */
  FAKEASTRO_OP = OP_NONE;

  if ((N = get_argument (*argc, argv, "-images"))) {
    remove_argument (N, argc, argv);
    FAKEASTRO_OP = OP_IMAGES;
  }

  if ((N = get_argument (*argc, argv, "-galaxy"))) {
    if (FAKEASTRO_OP != OP_NONE) usage();
    remove_argument (N, argc, argv);
    FAKEASTRO_OP = OP_GALAXY;
  }

  if ((N = get_argument (*argc, argv, "-2mass"))) {
    remove_argument (N, argc, argv);
    FAKEASTRO_OP = OP_2MASS;
  }

  if ((N = get_argument (*argc, argv, "-gaia"))) {
    remove_argument (N, argc, argv);
    FAKEASTRO_OP = OP_GAIA;
  }

  GALAXY_MODEL = NULL;
  if ((N = get_argument (*argc, argv, "-galaxy-model"))) {
    remove_argument (N, argc, argv);
    GALAXY_MODEL = strcreate(argv[N]);
    remove_argument (N, argc, argv);
  }
  if (!GALAXY_MODEL) GALAXY_MODEL = strcreate ("ROESER");

  TEST_SCALE = 1.0;
  if ((N = get_argument (*argc, argv, "-testing"))) {
    remove_argument (N, argc, argv);
    TEST_SCALE = atof(argv[N]);
    remove_argument (N, argc, argv);
  }
  ONE_BIG_CHIP = FALSE;
  if ((N = get_argument (*argc, argv, "-one-big-chip"))) {
    remove_argument (N, argc, argv);
    ONE_BIG_CHIP = TRUE;
  }

  if (FAKEASTRO_OP == OP_NONE) usage();

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
  if ((N = get_argument (*argc, argv, "-catalog"))) {
    remove_argument (N, argc, argv);
    UserPatch.Rmin = atof (argv[N]);
    UserPatch.Rmax = UserPatch.Rmin + 0.001;
    remove_argument (N, argc, argv);
    UserPatch.Dmin = atof (argv[N]);
    UserPatch.Dmax = UserPatch.Dmin + 0.001;
    remove_argument (N, argc, argv);
  }

  // XXX for the moment, make this selection manual.  it needs to be automatic 
  // based on the state of the SkyTable
  PARALLEL = FALSE;
  if ((N = get_argument (*argc, argv, "-parallel"))) {
    PARALLEL = TRUE;
    remove_argument (N, argc, argv);
  }
  // this is a test mode : rather than launching the remote jobs and waiting for completion,
  // fakeastro will simply list the remote command and wait for the user to signal completion
  PARALLEL_MANUAL = FALSE;
  if ((N = get_argument (*argc, argv, "-parallel-manual"))) {
    PARALLEL = TRUE; // -parallel-manual implies -parallel
    PARALLEL_MANUAL = TRUE;
    remove_argument (N, argc, argv);
  }
  // this is a test mode : rather than launching the fakeastro_client jobs remotely, they are 
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

  // MaxDensityUse = FALSE;
  // if ((N = get_argument (*argc, argv, "-max-density"))) {
  //   remove_argument (N, argc, argv);
  //   MaxDensityValue = atof(argv[N]);
  //   remove_argument (N, argc, argv);
  //   MaxDensityUse = TRUE;
  // }

  UNIFORM_RADEC = FALSE;
  if ((N = get_argument (*argc, argv, "-uniform-radec"))) {
    UNIFORM_RADEC = TRUE;
    remove_argument (N, argc, argv);
  }

  MAX_MAG_2MASS = 16.0;
  if ((N = get_argument (*argc, argv, "-2mass-limit"))) {
    remove_argument (N, argc, argv);
    MAX_MAG_2MASS = atof(argv[N]);
    remove_argument (N, argc, argv);
  }

  MAX_MAG_GAIA = 21.0;
  if ((N = get_argument (*argc, argv, "-gaia-limit"))) {
    remove_argument (N, argc, argv);
    MAX_MAG_GAIA = atof(argv[N]);
    remove_argument (N, argc, argv);
  }

  FORCE = FALSE;
  if ((N = get_argument (*argc, argv, "-force"))) {
    FORCE = TRUE;
    remove_argument (N, argc, argv);
  }

  VERBOSE = VERBOSE2 = FALSE;
  if ((N = get_argument (*argc, argv, "-v"))) {
    VERBOSE = TRUE;
    remove_argument (N, argc, argv);
  }
  if ((N = get_argument (*argc, argv, "-vv"))) {
    VERBOSE = VERBOSE2 = TRUE;
    remove_argument (N, argc, argv);
  }

  if (FAKEASTRO_OP == OP_IMAGES) {
    // mandatory arguments to fakeastro -images -input catdir -output -catdir -images images.fits
    if ((N = get_argument (*argc, argv, "-input"))) {
      remove_argument (N, argc, argv);
      CATDIR_INPUT = strcreate (argv[N]);
      remove_argument (N, argc, argv);
    } else {
      fprintf (stderr, "missing -input (catdir)\n");
      exit (2);
    }
    if ((N = get_argument (*argc, argv, "-output"))) {
      remove_argument (N, argc, argv);
      CATDIR_OUTPUT = strcreate (argv[N]);
      remove_argument (N, argc, argv);
    } else {
      fprintf (stderr, "missing -output (catdir)\n");
      exit (2);
    }
    if ((N = get_argument (*argc, argv, "-input-images"))) {
      remove_argument (N, argc, argv);
      IMAGES_INPUT = strcreate (argv[N]);
      remove_argument (N, argc, argv);
    } else {
      fprintf (stderr, "missing -images (images)\n");
      exit (2);
    }
  }

  return TRUE;
}

int args_client (int *argc, char **argv) {

  int N;

  /* possible operations */
  FAKEASTRO_OP = OP_NONE;

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
  if (!HOST_ID) usage_client();

  HOSTDIR = NULL;
  if ((N = get_argument (*argc, argv, "-hostdir"))) {
    remove_argument (N, argc, argv);
    HOSTDIR = strcreate (argv[N]);
    remove_argument (N, argc, argv);
  }
  if (!HOSTDIR) usage_client();

  CPT_FILE = NULL;
  if ((N = get_argument (*argc, argv, "-cpt"))) {
    remove_argument (N, argc, argv);
    CPT_FILE = strcreate (argv[N]);
    remove_argument (N, argc, argv);
  }
  if (!CPT_FILE) usage_client();
  
  INPUT = NULL;
  if ((N = get_argument (*argc, argv, "-input"))) {
    remove_argument (N, argc, argv);
    INPUT = strcreate (argv[N]);
    remove_argument (N, argc, argv);
  }
  if (!INPUT) usage_client();

  if ((N = get_argument (*argc, argv, "-images"))) {
    remove_argument (N, argc, argv);
    FAKEASTRO_OP = OP_IMAGES;
  }
  if ((N = get_argument (*argc, argv, "-galaxy"))) {
    if (FAKEASTRO_OP != OP_NONE) usage();
    remove_argument (N, argc, argv);
    FAKEASTRO_OP = OP_GALAXY;
  }
  if (FAKEASTRO_OP == OP_NONE) usage_client();

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
  if ((N = get_argument (*argc, argv, "-catalog"))) {
    remove_argument (N, argc, argv);
    UserPatch.Rmin = atof (argv[N]);
    UserPatch.Rmax = UserPatch.Rmin + 0.001;
    remove_argument (N, argc, argv);
    UserPatch.Dmin = atof (argv[N]);
    UserPatch.Dmax = UserPatch.Dmin + 0.001;
    remove_argument (N, argc, argv);
  }

  // MaxDensityUse = FALSE;
  // if ((N = get_argument (*argc, argv, "-max-density"))) {
  //   remove_argument (N, argc, argv);
  //   MaxDensityValue = atof(argv[N]);
  //   remove_argument (N, argc, argv);
  //   MaxDensityUse = TRUE;
  // }

  GALAXY_MODEL = NULL;
  if ((N = get_argument (*argc, argv, "-galaxy-model"))) {
    remove_argument (N, argc, argv);
    GALAXY_MODEL = strcreate(argv[N]);
    remove_argument (N, argc, argv);
  }
  if (!GALAXY_MODEL) GALAXY_MODEL = strcreate ("FEAST-HIPPARCOS");

  VERBOSE = VERBOSE2 = FALSE;
  if ((N = get_argument (*argc, argv, "-v"))) {
    VERBOSE = TRUE;
    remove_argument (N, argc, argv);
  }
  if ((N = get_argument (*argc, argv, "-vv"))) {
    VERBOSE = VERBOSE2 = TRUE;
    remove_argument (N, argc, argv);
  }

  return TRUE;
}

void usage () {
  fprintf (stderr, "ERROR: USAGE: fakeastro -images [options]\n");
  fprintf (stderr, "       OR:    fakeastro -galaxy [options]\n");

  fprintf (stderr, "  specify one of the following modes: \n");
  fprintf (stderr, "  -images\n");
  fprintf (stderr, "  -galaxy\n");

  fprintf (stderr, " additional options: \n");
  fprintf (stderr, "  -region RA RA DEC DEC\n");
  fprintf (stderr, "  -catalog (ra) (dec)\n");
  fprintf (stderr, "  -testing (scale)\n\n");
  fprintf (stderr, "  -v\n");
  fprintf (stderr, "  -vv\n");
  fprintf (stderr, "  \n");
  exit (2);
} 

void usage_client () {
  fprintf (stderr, "ERROR: USAGE: fakeastro_client -images\n");
  fprintf (stderr, "       OR:    fakeastro_client -galaxy\n");

  fprintf (stderr, "  specify one of the following modes: \n");
  fprintf (stderr, "  -images\n");
  fprintf (stderr, "  -galaxy\n");

  fprintf (stderr, " additional options: \n");
  fprintf (stderr, "  -region Rmin Rmax Dmin Dmax");
  fprintf (stderr, "  -catalog RA DEC");
  fprintf (stderr, "  -v\n");
  fprintf (stderr, "  -vv\n");
  fprintf (stderr, "  \n");
  exit (2);
} 
