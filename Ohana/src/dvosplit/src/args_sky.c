# include "dvosplit.h"

static void help (void);

int args (int argc, char **argv) {
  
  int N;

  /* check for help request */
  if (get_argument (argc, argv, "-help") ||
      get_argument (argc, argv, "-h")) {
    help ();
  }

  // -region is required for dvosplitsky
  UserPatch.Rmin = NAN;
  UserPatch.Rmax = NAN;
  UserPatch.Dmin = NAN;
  UserPatch.Dmax = NAN;
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
  if (isnan(UserPatch.Rmin)) help();
  if (isnan(UserPatch.Rmax)) help();
  if (isnan(UserPatch.Dmin)) help();
  if (isnan(UserPatch.Dmax)) help();

  /* extra error messages */
  VERBOSE = FALSE;
  if ((N = get_argument (argc, argv, "-v"))) {
    VERBOSE = TRUE;
    remove_argument (N, &argc, argv);
  }

  if (argc == 2) return (TRUE);

  fprintf (stderr, "USAGE: dvosplit (catdir) (newlevel) [-outdir outdir] [-region (Rmin) (Rmax) (Dmin) (Dmax)]\n");
  exit (2);
}

static void help () {

  fprintf (stderr, "USAGE\n");
  fprintf (stderr, "  dvosplitsky (catdir) -region Rmin Rmax Dmin Dmax \n\n");
  fprintf (stderr, "  NOTE : region is required for dvosplitsky\n");
  fprintf (stderr, "  optional flags:\n");
  fprintf (stderr, "  -v                    	  : verbose mode\n");
  fprintf (stderr, "  -help                 	  : this list\n");
  fprintf (stderr, "  -h                    	  : this list\n\n");
  exit (2);
}

