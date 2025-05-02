# include "addstar.h"
static void help (void);

void args_server (int argc, char **argv) {
  
  int N;

  /* check for help request */
  if (get_argument (argc, argv, "-help") ||
      get_argument (argc, argv, "-h")) {
    help ();
  }

  // a global used by find_matches_refstars.c (value is 1 except for load2mass)
  NREFSTAR_GROUP = 1;

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
  
  /* define 2MASS quality flags to keep */
  SELECT_2MASS_QUALITY = NULL;
  if ((N = get_argument (argc, argv, "-2massquality"))) {
    remove_argument (N, &argc, argv);
    SELECT_2MASS_QUALITY = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  /* force read of image database with mismatched NSTARS & size */ 
  FORCE_READ = FALSE;
  if ((N = get_argument (argc, argv, "-force"))) {
    FORCE_READ = TRUE;
    remove_argument (N, &argc, argv);
  }

  /* extra error messages */
  VERBOSE = FALSE;
  if ((N = get_argument (argc, argv, "-v"))) {
    VERBOSE = TRUE;
    remove_argument (N, &argc, argv);
  }

  if (argc != 1) {
    fprintf (stderr, "USAGE: addstard\n");
    exit (2);
  }
}

static void help () {

  fprintf (stderr, "USAGE\n");
  fprintf (stderr, "  addstard\n");
  fprintf (stderr, "  -force                	  : force read of database with inconsistent info\n");
  fprintf (stderr, "  -v                    	  : verbose mode\n");
  fprintf (stderr, "  -help                 	  : this list\n");
  fprintf (stderr, "  -h                    	  : this list\n\n");
  exit (2);
}
