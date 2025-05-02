# include "gastro2.h"
# define NARGS 2  /* minimum is:  gastro catalog */

void ahelp () {

  fprintf (stderr, "gastro -- astrometry for LONEOS\n");

  fprintf (stderr, "  USAGE: gastro pixscale filename");
  fprintf (stderr, "  optional flags:\n");
  fprintf (stderr, "  -v (verbose mode)\n");
  fprintf (stderr, "  -dump (dump catalog stars, don't complete astrometry)\n");
  fprintf (stderr, "  -mdmp (dump matched catalog stars)\n");
  fprintf (stderr, "\n"); 
  exit (0);

}

void args (int *argc, char **argv, Coords *coords) {
  
  int N;

  if (get_argument (*argc, argv, "-help") ||
      get_argument (*argc, argv, "-h")) {
    ahelp ();
  }

  VERBOSE = FALSE;
  if ((N = get_argument (*argc, argv, "-v"))) {
    VERBOSE = TRUE;
    remove_argument (N, argc, argv);
  }

  /* force read of image database with mismatched NSTARS & size */ 
  TEXTMODE = FALSE;
  if ((N = get_argument (*argc, argv, "-textmode"))) {
    TEXTMODE = TRUE;
    remove_argument (N, argc, argv);
  }

  /* force read of image database with mismatched NSTARS & size */ 
  PTOLEMY_FILL_FACTOR = FALSE;
  if ((N = get_argument (*argc, argv, "-ptolemy-fill-factor"))) {
    PTOLEMY_FILL_FACTOR = TRUE;
    remove_argument (N, argc, argv);
  }

  PLOTSTUFF = FALSE;
  if ((N = get_argument (*argc, argv, "-plot"))) {
    PLOTSTUFF = TRUE;
    remove_argument (N, argc, argv);
  }

  MAGLIMS = TRUE;
  if ((N = get_argument (*argc, argv, "-maglims"))) {
    MAGLIMS = FALSE;
    remove_argument (N, argc, argv);
  }
  if ((N = get_argument (*argc, argv, "-magrange"))) {
    MAGLIMS = TRUE;
    MAGMANUAL = TRUE;
    remove_argument (N, argc, argv);
    MAGLIM_MIN = atof (argv[N]);
    remove_argument (N, argc, argv);
    MAGLIM_MAX = atof (argv[N]);
    remove_argument (N, argc, argv);
  }

  NMAX_STARS = 300;
  if ((N = get_argument (*argc, argv, "-nstars"))) {
    remove_argument (N, argc, argv);
    NMAX_STARS = atof (argv[N]);
    remove_argument (N, argc, argv);
  }

  HEADER[0] = 0;
  if ((N = get_argument (*argc, argv, "-header"))) {
    remove_argument (N, argc, argv);
    strcpy (HEADER, argv[N]);
    remove_argument (N, argc, argv);
  }
  HEADER[0] = 0;
  if ((N = get_argument (*argc, argv, "-head"))) {
    remove_argument (N, argc, argv);
    strcpy (HEADER, argv[N]);
    remove_argument (N, argc, argv);
  }

  FLIPX = FALSE;
  if ((N = get_argument (*argc, argv, "-fx"))) {
    FLIPX = TRUE;
    remove_argument (N, argc, argv);
  }

  FLIPY = FALSE;
  if ((N = get_argument (*argc, argv, "-fy"))) {
    FLIPY = TRUE;
    remove_argument (N, argc, argv);
  }

  FORCE = FALSE;
  if ((N = get_argument (*argc, argv, "-coords"))) {
    FORCE = TRUE;
    remove_argument (N, argc, argv);
    F_RA = atof (argv[N]);
    remove_argument (N, argc, argv);
    F_DEC = atof (argv[N]);
    remove_argument (N, argc, argv);
  }

  /** XXX temporary trick to deal with the very wide-field ASCA images 
      this alters the definition of the reference field boundaries */
  ASCA = FALSE;
  if ((N = get_argument (*argc, argv, "-asca"))) {
    ASCA = TRUE;
    remove_argument (N, argc, argv);
  }

  CATDUMP = FALSE;
  if ((N = get_argument (*argc, argv, "-dump"))) {
    CATDUMP = TRUE;
    remove_argument (N, argc, argv);
  }

  MATCHDUMP = FALSE;
  if ((N = get_argument (*argc, argv, "-mdmp"))) {
    MATCHDUMP = TRUE;
    remove_argument (N, argc, argv);
  }

  NOMATCHDUMP = FALSE;
  if ((N = get_argument (*argc, argv, "-cdmp"))) {
    NOMATCHDUMP = TRUE;
    remove_argument (N, argc, argv);
  }

  // XXX these options are being ignored
  NEWPHOTCODE = FALSE;
  if ((N = get_argument (*argc, argv, "-p"))) {
    NEWPHOTCODE = TRUE;
    remove_argument (N, argc, argv);
    PHOTCODE = strcreate (argv[N]);
    remove_argument (N, argc, argv);
  }
  LONEOS_COORDS = FALSE;
  if ((N = get_argument (*argc, argv, "-loneos"))) {
    LONEOS_COORDS = TRUE;
    remove_argument (N, argc, argv);
  }

  if (*argc != NARGS) {
    fprintf (stderr, "USAGE: gastro filename\n");
    exit (0);
  }

}

