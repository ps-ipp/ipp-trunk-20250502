# include "lightcurve.h"
# define NARGS 1

void args (argc, argv)
int      argc;
char   **argv;
{
  
  int N;

  /** mandatory arguments (see help.c) **/
  if (N = get_argument (argc, argv, "-P")) {
    remove_argument (N, &argc, argv);
    RADIUS = atof (argv[N]);
    PIXELS = TRUE;
    remove_argument (N, &argc, argv);
  }
  else {
    if (N = get_argument (argc, argv, "-A")) {
      remove_argument (N, &argc, argv);
      RADIUS = atof (argv[N]) / 3600.0;
      PIXELS = FALSE;
      remove_argument (N, &argc, argv);
    }
    else 
      help (argv[0]);
  }
  
  if (N = get_argument (argc, argv, "-im")) {
    remove_argument(N, &argc, argv);
    strcpy (IMAGES, argv[N]);
    remove_argument(N, &argc, argv);
  }
  else
    help (argv[0]);

  /**** optional arguments (see help.c) ****/
  if (N = get_argument (argc, argv, "-o")) {
    remove_argument(N, &argc, argv);
    strcpy (OUTFILE, argv[N]);
    remove_argument(N, &argc, argv);
  }
  else
    strcpy (OUTFILE, "-");

  if (N = get_argument (argc, argv, "-midas")) {
    remove_argument(N, &argc, argv);
    MIDAS = TRUE;
  }
  else
    MIDAS = FALSE;

  if (N = get_argument (argc, argv, "-m")) {
    remove_argument(N, &argc, argv);
    MCUTOFF = atof(argv[N]);
    remove_argument(N, &argc, argv);
  }
  else 
    MCUTOFF = 0.05;

  if (N = get_argument (argc, argv, "-X")) {
    remove_argument(N, &argc, argv);
    EXTRASTARS = TRUE;
  }
  else 
    EXTRASTARS = FALSE;

  if (remove_argument(get_argument (argc, argv, "-h"), &argc, argv) ||
      remove_argument(get_argument (argc, argv, "-help"), &argc, argv))
    help(argv[0]);

  if (argc != NARGS) {
    fprintf (stderr,  "%s%s%s", "USAGE: ", 
	     argv[0], " (-P / -A) radius  (-im images) [-o file] [-midas]\n");
    exit (0);
  }
}

