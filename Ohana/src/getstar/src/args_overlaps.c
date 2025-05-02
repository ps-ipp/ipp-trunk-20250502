# include "dvoImageOverlaps.h"

void help () {
  fprintf (stderr, "USAGE: \n"
	   "dvoImageOverlaps (image)\n"
    );
  exit (2);
}

int args_overlaps (int argc, char **argv) {
  
  int N;

  /* check for help request */
  if (get_argument (argc, argv, "-help") ||
      get_argument (argc, argv, "-h")) {
    help ();
  }

  /* check for command line options */
  WITH_PHU = FALSE;
  if ((N = get_argument (argc, argv, "+phu"))) {
    WITH_PHU = TRUE;
    remove_argument (N, &argc, argv);
  }
  SOLO_PHU = FALSE;
  if ((N = get_argument (argc, argv, "-phu"))) {
    WITH_PHU = TRUE;
    SOLO_PHU = TRUE;
    remove_argument (N, &argc, argv);
  }

  /* check for command line options */
  VERBOSE = FALSE;
  if ((N = get_argument (argc, argv, "-v"))) {
    VERBOSE = TRUE;
    remove_argument (N, &argc, argv);
  }
  /* accept bad header astrometry */
  ACCEPT_ASTROM = FALSE;
  if ((N = get_argument (argc, argv, "-accept"))) {
    ACCEPT_ASTROM = TRUE;
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-accept-astrom"))) {
    ACCEPT_ASTROM = TRUE;
    remove_argument (N, &argc, argv);
  }
  /* provide a mosaic for distortion */
  MOSAIC = NULL;
  if ((N = get_argument (argc, argv, "-mosaic"))) {
    Header header;
    ALLOCATE (MOSAIC, Coords, 1);

    remove_argument (N, &argc, argv);
    if (!gfits_read_header (argv[N], &header)) {
      fprintf (stderr, "ERROR: can't read header for mosaic %s\n", argv[N]);
      exit (1);
    }
    if (!GetCoords (MOSAIC, &header)) {
      fprintf (stderr, "ERROR: no astrometric solution in header\n");
      exit (1);
    }
    if (strcmp(&MOSAIC[0].ctype[4], "-DIS")) {
      fprintf (stderr, "ERROR: not a mosaic distortion header\n");
      exit (1);
    }
    remove_argument (N, &argc, argv);
    gfits_free_header (&header);
  }

  if (argc != 2) help();

  return (TRUE);
}
