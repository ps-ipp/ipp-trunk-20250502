# include "dvoImagesAtCoords.h"

void help () {
  fprintf (stderr, "USAGE: \n"
	   "dvoImagesAtCoords [-astrom astrom_file] -coords coords_file\n"
	   "dvoImagesAtCoords [-astrom astrom_file] (ra) (dec))\n"
    );
  exit (2);
}

int args_coords (int argc, char **argv) {
  
  int N;

  /* check for help request */
  if (get_argument (argc, argv, "-help") ||
      get_argument (argc, argv, "-h")) {
    help ();
  }

  coordsFile = NULL;
  if ((N = get_argument(argc, argv, "-coords"))) {
    remove_argument (N, &argc, argv);
    if (argv[N] == NULL) {
      fprintf (stderr, "ERROR: no file provided with -coords\n");
      exit (1);
    }
    coordsFile = strcreate(argv[N]);
    remove_argument (N, &argc, argv);
  }
  astromFile = NULL;
  if ((N = get_argument(argc, argv, "-astrom"))) {
    remove_argument (N, &argc, argv);
    if (argv[N] == NULL) {
      fprintf (stderr, "ERROR: no file provided with -astrom\n");
      exit (1);
    }
    astromFile = strcreate(argv[N]);
    remove_argument (N, &argc, argv);
  }

  /* check for command line options */
  LISTCHIPCOORDS = FALSE;
  if ((N = get_argument (argc, argv, "-listchipcoords"))) {
    LISTCHIPCOORDS = TRUE;
    remove_argument (N, &argc, argv);
  }
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

  fullNames = 0;
  if ((N = get_argument(argc, argv, "-full-names"))) {
    remove_argument (N, &argc, argv);
    fullNames = 1;
  }

  if (coordsFile) {
      if (argc != 1) help();
  } else {
      // expect RA and DEC on command line
      if (argc != 3) help();

      cmd_line_ra = atof(argv[1]);
      cmd_line_dec = atof(argv[2]);
  }

  return (TRUE);
}
