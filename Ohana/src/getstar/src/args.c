# include "getstar.h"

void help () {
  fprintf (stderr, "USAGE: \n"
	   "getstar -region ra dec ra dec\n"
	   "getstar -radius ra dec radius\n"
	   "getstar -catalog n0000/0000.cpt\n"
	   "getstar -image name\n"
	   "getstar -immatch partial-name\n\n"
	   " options: \n"
	   " -maglim (mag)    : maximum magnitude returned\n"
	   " -format (format) : output formats (CATALOG, PS1_DEV_0, PS1_DEV_1, PS1_DEV_2)\n"
	   " -photcode (code) : desired photcode for output magnitudes\n"
	   " -o output        : defaults to stdout\n"
	   " -v               : verbose mode\n"
	   " -h / -help       : this list\n"
    );
  exit (2);
}

int args (int argc, char **argv) {
  
  int N;

  /* check for help request */
  if (get_argument (argc, argv, "-help") ||
      get_argument (argc, argv, "-h")) {
    help ();
  }

  /* configuration info */
  ConfigInit (&argc, argv);

  /* check for command line options */
  VERBOSE = FALSE;
  if ((N = get_argument (argc, argv, "-v"))) {
    VERBOSE = TRUE;
    remove_argument (N, &argc, argv);
  }

  MagLimitUse = FALSE;
  MagLimitValue = 100;
  if ((N = get_argument (argc, argv, "-maglim"))) {
    MagLimitUse = TRUE;
    remove_argument (N, &argc, argv);
    MagLimitValue = atof (argv[N]);
    remove_argument (N, &argc, argv);
  }

  MaxDensityUse = FALSE;
  if ((N = get_argument (argc, argv, "-max-density"))) {
    if (MagLimitUse) {
      fprintf (stderr, "-maglim and -max-density are mutually exclusive; ignoring -maglim");
      // MagLimitValue will be overridded by a density-based limit
    }
    MaxDensityUse = TRUE;
    remove_argument (N, &argc, argv);
    MaxDensityValue = atof (argv[N]);
    remove_argument (N, &argc, argv);
  }

  MinMagUse = FALSE;
  MinMagValue = -100;
  if ((N = get_argument (argc, argv, "-minmag"))) {
    MinMagUse = TRUE;
    remove_argument (N, &argc, argv);
    MinMagValue = atof (argv[N]);
    remove_argument (N, &argc, argv);
  }

  /* check for command line options */
  strcpy (OUTPUT, "stdout");
  if ((N = get_argument (argc, argv, "-o"))) {
    remove_argument (N, &argc, argv);
    strcpy (OUTPUT, argv[N]);
    remove_argument (N, &argc, argv);
  }

  /* check for command line options */
  strcpy (OUTFORMAT, "CATALOG");
  if ((N = get_argument (argc, argv, "-format"))) {
    remove_argument (N, &argc, argv);
    strcpy (OUTFORMAT, argv[N]);
    remove_argument (N, &argc, argv);
  }

  // in some cases, we need a photcode
  photcode = GetPhotcodebyNsec (0); // default to first average photcode
  if ((N = get_argument (argc, argv, "-photcode"))) {
    remove_argument (N, &argc, argv);
    photcode = GetPhotcodebyName (argv[N]);
    if (photcode == NULL) {
      fprintf (stderr, "photcode %s not found in photcode table\n", argv[N]);
      exit (1);
    }
    remove_argument (N, &argc, argv);
  }

  /* parse optional entries above. one of the options below is required */
  MODE = BY_NOTHING;
  if ((N = get_argument (argc, argv, "-region"))) {
    double R, D;
    MODE = BY_REGION;
    remove_argument (N, &argc, argv);
    if (argc != 5) help();
    ohana_str_to_radec (&R, &D, argv[N+0], argv[N+1]);
    REGION.Rmin = R;
    REGION.Dmin = D;
    remove_argument (N, &argc, argv);
    remove_argument (N, &argc, argv);

    ohana_str_to_radec (&R, &D, argv[N+0], argv[N+1]);
    REGION.Rmax = R;
    REGION.Dmax = D;
    remove_argument (N, &argc, argv);
    remove_argument (N, &argc, argv);

    // XXX we will have issues at 0,360 boundary...
    // see code in dvo/pmeasure for fixes
    REGION.Rmin = ohana_normalize_angle (REGION.Rmin);
    REGION.Rmax = ohana_normalize_angle (REGION.Rmax);

    if (REGION.Dmax < REGION.Dmin) {
	SWAP (REGION.Dmax, REGION.Dmin);
    }

    R = REGION.Rmax - REGION.Rmin;
    D = REGION.Dmax - REGION.Dmin;

    if ((R <= 0.0) || (D <= 0.0)) {
      fprintf (stderr, "WARNING: selected region [(%f,%f) to (%f,%f)] has zero or negative area\n", 
	       REGION.Rmin, REGION.Dmin, REGION.Rmax, REGION.Dmax);
    }

  }
  if ((N = get_argument (argc, argv, "-radius"))) {
    double R, D, radius;
    fprintf (stderr, "-radius is not recommended\n");
    MODE = BY_RADIUS;
    remove_argument (N, &argc, argv);
    if (argc != 4) help();
    ohana_str_to_radec (&R, &D, argv[N+0], argv[N+1]);
    radius = atof(argv[N+2]);
    REGION.Rmin = R - radius / cos(D*RAD_DEG);
    REGION.Dmin = D - radius;
    REGION.Rmax = R + radius / cos(D*RAD_DEG);
    REGION.Dmax = D + radius;
    remove_argument (N, &argc, argv);
    remove_argument (N, &argc, argv);
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-catalog"))) {
    fprintf (stderr, "-catalog is not implemented\n");
    exit (2);
    MODE = BY_CATALOG;
    remove_argument (N, &argc, argv);
    if (argc != 2) help();
  }
  if ((N = get_argument (argc, argv, "-image"))) {
    MODE = BY_IMAGE;
    remove_argument (N, &argc, argv);
    IMAGENAME = strcreate (argv[N]);
    if (argc != 2) help();
  }
  if ((N = get_argument (argc, argv, "-immatch"))) {
    MODE = BY_IMMATCH;
    remove_argument (N, &argc, argv);
    IMAGENAME = strcreate (argv[N]);
    if (argc != 2) help();
  }
  if (MODE == BY_NOTHING) help ();

  return (TRUE);
}


/* USAGE

getstar -region ra dec ra dec
getstar -radius ra dec radius
getstar -catalog n0000/0000.cpt
getstar -image name [-smp | -smf]
getstar -immatch partial-name [-smp | -smf]
   
* return measurements 
* return average / secfilt table
* return a single image (smf/smp format)

*/
