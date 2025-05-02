# include "skycells.h"
static void help (void);

int args_skycells (int argc, char **argv) {
  
  int N;
  char *ptr;

  if (argc == 1) goto escape;

  /* check for help request */
  if (get_argument (argc, argv, "-help") ||
      get_argument (argc, argv, "-h")) {
    help ();
  }

  /* extra error messages */
  VERBOSE = FALSE;
  if ((N = get_argument (argc, argv, "-v"))) {
    VERBOSE = TRUE;
    remove_argument (N, &argc, argv);
  }

  /* what type of output files? */
  MODE = SQUARES;
  if ((N = get_argument (argc, argv, "-mode"))) {
    remove_argument (N, &argc, argv);
    if (!strcasecmp (argv[N], "squares")) {
      MODE = SQUARES;
    }
    if (!strcasecmp (argv[N], "triangles")) {
      MODE = SQUARES;
    }
    if (!strcasecmp (argv[N], "local")) {
      MODE = LOCAL;
    }
    if (!strcasecmp (argv[N], "rings")) {
      MODE = RINGS;
    }
    if (!strcasecmp (argv[N], "tamas")) {
      MODE = TAMAS;
    }
    if (!strcasecmp (argv[N], "cfis")) {
      MODE = CFIS;
    }
    remove_argument (N, &argc, argv);
  }

  /* what base solid? */
  SOLID = ICOSAHEDRON;
  if ((N = get_argument (argc, argv, "-solid"))) {
    remove_argument (N, &argc, argv);
    if (!strncasecmp (argv[N], "TETRAHEDRON", MIN(4, strlen(argv[N])))) {
      SOLID = TETRAHEDRON;
    }
    if (!strncasecmp (argv[N], "CUBE", MIN(4, strlen(argv[N])))) {
      SOLID = CUBE;
    }
    if (!strncasecmp (argv[N], "OCTOHEDRON", MIN(4, strlen(argv[N])))) {
      SOLID = OCTOHEDRON;
    }
    if (!strncasecmp (argv[N], "DODECAHEDRON", MIN(4, strlen(argv[N])))) {
      SOLID = DODECAHEDRON;
    }
    if (!strncasecmp (argv[N], "ICOSAHEDRON", MIN(4, strlen(argv[N])))) {
      SOLID = ICOSAHEDRON;
    }
    remove_argument (N, &argc, argv);
  }

  if (MODE == LOCAL) {
    // for local mode, need to define the center and range
    if ((N = get_argument (argc, argv, "-center"))) {
      remove_argument (N, &argc, argv);
      CENTER_RA  = atof (argv[N]);
      remove_argument (N, &argc, argv);
      CENTER_DEC = atof (argv[N]);
      remove_argument (N, &argc, argv);
    } else {
      fprintf (stderr, "missing -center RA DEC for LOCAL mode\n");
      help ();
    }
    // for local mode, need to define the center and range
    if ((N = get_argument (argc, argv, "-size"))) {
      remove_argument (N, &argc, argv);
      RANGE_RA  = atof (argv[N]);
      remove_argument (N, &argc, argv);
      RANGE_DEC = atof (argv[N]);
      remove_argument (N, &argc, argv);
    } else {
      fprintf (stderr, "missing -size dRA dDEC for LOCAL mode\n");
      help ();
    }
    PROJECTION_NUMBER[0] = 0;
    if ((N = get_argument (argc, argv, "-projection-number"))) {
      remove_argument (N, &argc, argv);
      int projection_number = atoi(argv[N]);
      // We store projection number as a string, but range check as a number
      if (projection_number > 10000) {
        fprintf (stderr, "maximum projection-number value is 10000\n");
        help();
      } else if (projection_number < 0) {
        fprintf (stderr, "projection-number value must be > 0\n");
        help();
      }
      strcpy(PROJECTION_NUMBER, argv[N]);
      remove_argument (N, &argc, argv);
    }
  }

  /* what type of output files? */
  FIX_NS = FALSE;
  if ((N = get_argument (argc, argv, "-fix-ns"))) {
    FIX_NS = TRUE;
    remove_argument (N, &argc, argv);
  }

  /* pixel scale (arcsec/pixel) */
  SCALE = 1.0;
  if (MODE == CFIS) {
    SCALE = 0.185768447409; // default CFIS value
  }
  if ((N = get_argument (argc, argv, "-scale"))) {
    remove_argument (N, &argc, argv);
    SCALE = atof (argv[N]);
    remove_argument (N, &argc, argv);
  }

  /* pixel scale (arcsec/pixel) */
  EULER_A = EULER_B = 0.0;
  if ((N = get_argument (argc, argv, "-euler"))) {
    remove_argument (N, &argc, argv);
    EULER_A = RAD_DEG*atof (argv[N]);
    remove_argument (N, &argc, argv);
    EULER_B = RAD_DEG*atof (argv[N]);
    remove_argument (N, &argc, argv);
  }

  /* padding fraction */
  PADDING = 0.0;
  if ((N = get_argument (argc, argv, "-padding"))) {
    remove_argument (N, &argc, argv);
    PADDING = atof (argv[N]);
    remove_argument (N, &argc, argv);
  }

  /* max number of skycells kept in memory */
  NMAX = 200000;
  if ((N = get_argument (argc, argv, "-nmax"))) {
    remove_argument (N, &argc, argv);
    NMAX = atof (argv[N]);
    remove_argument (N, &argc, argv);
  }

  /* max number of skycells kept in memory */
  NX_SUB = NY_SUB = 1;
  if ((N = get_argument (argc, argv, "-nx"))) {
    remove_argument (N, &argc, argv);
    NX_SUB = atof (argv[N]);
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-ny"))) {
    remove_argument (N, &argc, argv);
    NY_SUB = atof (argv[N]);
    remove_argument (N, &argc, argv);
  }

  LEVEL = 8;
  if ((MODE == SQUARES) || (MODE == TRIANGLES)) {
    if ((N = get_argument (argc, argv, "-level"))) {
      remove_argument (N, &argc, argv);
      LEVEL = strtol (argv[N], &ptr, 10);
      remove_argument (N, &argc, argv);
      if ((*ptr != 0) || (LEVEL < 0)) {
	fprintf (stderr, "-level requires an integer (>= 0) argument\n");
	help ();
      }  
    }
  }

  CELLSIZE = 4.0;
  if (MODE == TAMAS) CELLSIZE = 3.955;
  if (MODE == CFIS) CELLSIZE = 0.5;

  int checkCellsize = (MODE == RINGS) || (MODE == TAMAS) || (MODE == CFIS);

  if (checkCellsize && (N = get_argument (argc, argv, "-cellsize"))) {
    remove_argument (N, &argc, argv);
    CELLSIZE = strtod (argv[N], &ptr);
    if ((*ptr != 0) || (CELLSIZE < 0.0)) {
      fprintf (stderr, "-level requires a floating-point argument\n");
      help ();
    }  
    remove_argument (N, &argc, argv);
  }

  OVERLAP_RA = 0;
  OVERLAP_DEC = 0;
  if ((N = get_argument (argc, argv, "-overlap"))) {
    remove_argument (N, &argc, argv);
    OVERLAP_RA  = atof (argv[N]);
    remove_argument (N, &argc, argv);
    OVERLAP_DEC  = atof (argv[N]);
    remove_argument (N, &argc, argv);
  }
  X_PARITY = 1;
  if ((N = get_argument (argc, argv, "-skyparity"))) {
    X_PARITY = -1;
    remove_argument (N, &argc, argv);
  }

  if (argc != 1) goto escape;

  return (TRUE);

escape:
  fprintf (stderr, "USAGE: skycells [-mode mode] [-level level] [-scale arcsec/pix] [-nx (Nx cells)] [-ny (Ny cells)]\n");
  fprintf (stderr, "  [-h for details and other options]\n");
  exit (2);
}

static void help () {

  fprintf (stderr, "USAGE\n");
  fprintf (stderr, "  skycells\n\n");

  fprintf (stderr, "  -mode (name)                : define the type of tessellation.  available options are:\n");
  fprintf (stderr, "     SQUARE                   : generate rectangular skycells using base solid (default)\n");
  fprintf (stderr, "     TRIANGLE                 : generate triangular skycells using icosahedron base solid\n");
  fprintf (stderr, "     RINGS                    : generate rectangular skycells using declination strips\n");
  fprintf (stderr, "     CFIS                     : generate rectangular skycells using CFIS declination strips\n");
  fprintf (stderr, "     LOCAL                    : generate a local tessellation around a spot on the sky\n");
  fprintf (stderr, "                                 (Note that this tessellation does not cover the full sky)\n");
  fprintf (stderr, "  -solid (name)               : specify the base solid (default: ICOSAHEDRON)\n");
  fprintf (stderr, "                                value may be one of: TETRAHEDRON, CUBE, OCTOHEDRON, DODECAHEDRON, ICOSAHEDRON\n");
  fprintf (stderr, "                                for convenience, only the first 4 characters are required\n");
  fprintf (stderr, "  -v                          : verbose mode\n");
  fprintf (stderr, "  -triangles                  : save base triangles instead of skycells\n");
  fprintf (stderr, "  -fix-ns                     : orient skycells with y-axis aligned with Dec\n");
  fprintf (stderr, "  -scale (scale)              : set pixel scale in arcsec (default 1.0 arcsec / pixel)\n");
  fprintf (stderr, "  -euler (A) (B)              : define Euler A and B rotation angles (degrees)\n");
  fprintf (stderr, "  -padding (fraction)         : pad skycells by this fraction in each dimension\n");
  fprintf (stderr, "  -nmax Nmax                  : only keep Nmax skycells in memory\n");
  fprintf (stderr, "  -nx Nx                      : subdivide skycell projection in x by Nx\n");
  fprintf (stderr, "  -ny Ny                      : subdivide skycell projection in y by Ny\n");
  fprintf (stderr, "  -overlap Or Od              : overlap between skycells (arcseconds)\n");
  fprintf (stderr, "  -projection-number Np       : set projection-number (local mode only)\n");
  fprintf (stderr, "  -skyparity                  : set wcs for skycells so that east is to the left\n");
  fprintf (stderr, "  -help                       : this list\n");
  fprintf (stderr, "  -h                          : this list\n\n");
  exit (2);
}
