# include "uniphot.h"

int args_uniphot (int argc, char **argv) {

  int N;

  /* define time */
  TimeSelect = FALSE;
  if ((N = get_argument (argc, argv, "-time"))) {
    remove_argument (N, &argc, argv);
    if (!ohana_str_to_time (argv[N], &TSTART)) { 
      fprintf (stderr, "ERROR: syntax error\n");
      exit (1);
    }
    remove_argument (N, &argc, argv);
    if (!ohana_str_to_time (argv[N], &TSTOP)) { 
      fprintf (stderr, "ERROR: syntax error\n");
      exit (1);
    }
    remove_argument (N, &argc, argv);
    TimeSelect = TRUE;
  }

  VERBOSE = FALSE;
  if ((N = get_argument (argc, argv, "-v"))) {
    VERBOSE = TRUE;
    remove_argument (N, &argc, argv);
  }

  NLOOP = 8;
  if ((N = get_argument (argc, argv, "-n"))) {
    remove_argument (N, &argc, argv);
    NLOOP = atof (argv[N]);
    remove_argument (N, &argc, argv);
  }

  strcpy (STATMODE, "MEAN");
  if ((N = get_argument (argc, argv, "-statmode"))) {
    remove_argument (N, &argc, argv);
    strcpy (STATMODE, argv[N]);
    remove_argument (N, &argc, argv);
  }

  UPDATE = FALSE;
  if ((N = get_argument (argc, argv, "-update"))) {
    remove_argument (N, &argc, argv);
    UPDATE = TRUE;
  }

  /* max separation for unique space group, in degrees */
  RADIUS = 2.0;
  if ((N = get_argument (argc, argv, "-radius"))) {
    remove_argument (N, &argc, argv);
    RADIUS = atof (argv[N]);
    remove_argument (N, &argc, argv);
  }

  /* max separation for unique time group, in days -> seconds */
  TRANGE = 86400*7.0;
  if ((N = get_argument (argc, argv, "-trange"))) {
    remove_argument (N, &argc, argv);
    TRANGE = 86400*atof (argv[N]);
    remove_argument (N, &argc, argv);
  }

  if (argc != 2) {
    fprintf (stderr, "ERROR: USAGE: uniphot (photcode) [options]\n");
    exit (2);
  } 

  return (TRUE);
}

int args_setfwhm (int argc, char **argv) {

  int N;

  VERBOSE = FALSE;
  if ((N = get_argument (argc, argv, "-v"))) {
    VERBOSE = TRUE;
    remove_argument (N, &argc, argv);
  }

  UPDATE = FALSE;
  if ((N = get_argument (argc, argv, "-update"))) {
    remove_argument (N, &argc, argv);
    UPDATE = TRUE;
  }

  if (argc != 2) {
    fprintf (stderr, "ERROR: USAGE: setfwhm (fwhmfile) [options]\n");
    exit (2);
  } 

  return (TRUE);
}

