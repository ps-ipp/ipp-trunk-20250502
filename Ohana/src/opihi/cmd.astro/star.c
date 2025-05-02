# include "astro.h"

int star (int argc, char **argv) {

  int x, y, N, Nborder;
  double max;
  Buffer *buf;
  int VERBOSE;

  VERBOSE = TRUE;
  if ((N = get_argument (argc, argv, "-q"))) {
    VERBOSE = FALSE;
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-quiet"))) {
    VERBOSE = FALSE;
    remove_argument (N, &argc, argv);
  }

  Nborder = 3;
  if ((N = get_argument (argc, argv, "-border"))) {
    remove_argument (N, &argc, argv);
    Nborder  = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }
  Nborder = MAX (Nborder, 1);
  
  max = 60000;
  if ((N = get_argument (argc, argv, "-sat"))) {
    remove_argument (N, &argc, argv);
    max  = atof(argv[N]);
    remove_argument (N, &argc, argv);
  }
  
  int dx = 11;
  int dy = 11;
  int BOX = FALSE;
  if ((N = get_argument (argc, argv, "-box"))) {
    remove_argument (N, &argc, argv);
    dx  = atoi(argv[N]);
    remove_argument (N, &argc, argv);
    dy  = atoi(argv[N]);
    remove_argument (N, &argc, argv);
    BOX = TRUE;
  }

  if ((argc != 4) && (argc != 5)) {
    gprint (GP_ERR, "USAGE: star (buffer) x y [dx] [-border N] [-sat cnts] [-box dx dy]\n");
    gprint (GP_ERR, " dx is the aperture diameter, but is adjusted up to the next odd number\n");
    return (FALSE);
  }
  if ((buf = SelectBuffer (argv[1], OLDBUFFER, TRUE)) == NULL) return (FALSE);

  x = atof (argv[2]);
  y = atof (argv[3]);
  if (argc == 5) {
    dx = atof (argv[4]);
  }

  if (BOX) {
    get_box_stats (&buf[0].matrix, x, y, dx, dy, Nborder, max, VERBOSE);
  } else {
    get_aperture_stats (&buf[0].matrix, x, y, dx, Nborder, max, VERBOSE);
  }
  
  return (TRUE);
}

