# include "dvodist.h"

void initialize (int argc, char **argv) {

  /* are these set correctly? */
  if (get_argument (argc, argv, "-h")) usage();
  if (get_argument (argc, argv, "--h")) usage();
  if (get_argument (argc, argv, "-help")) usage();
  if (get_argument (argc, argv, "--help")) usage();

  args (argc, argv);
}

void usage() {

  fprintf (stderr, "USAGE: dvodist (-out | -in) (catdir)\n\n");
  fprintf (stderr, " this program takes an existing DVO database and distributes it to the remote machine locations\n");

  fprintf (stderr, " optional options:\n");
  fprintf (stderr, " -v : verbose mode\n");
  fprintf (stderr, " -params : list the current parameters\n\n");

  fprintf (stderr, " -out-backup (host1) to (host2) : move *.bck files for host1 to host2\n");
  fprintf (stderr, " -use-backup (host1) : use the backup file for host1\n");

  exit (2);
}

int args (int argc, char **argv) {

  int N;

  VERBOSE = FALSE;
  if ((N = get_argument (argc, argv, "-v"))) {
    VERBOSE = TRUE;
    remove_argument (N, &argc, argv);
  }

  /* specify portion of the sky */
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

    if (UserPatch.Rmin == UserPatch.Rmax) {
      fprintf (stderr, "ERROR: selected region is ill-defined: Rmin == Rmax\n");
      exit (2);
    }
    if (UserPatch.Dmin == UserPatch.Dmax) {
      fprintf (stderr, "ERROR: selected region is ill-defined: Dmin == Dmax\n");
      exit (2);
    }
  }

  MODE = MODE_NONE;
  if ((N = get_argument (argc, argv, "-in"))) {
    MODE = MODE_IN;
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-out"))) {
    if (MODE) {
      fprintf (stderr, "ERROR: cannot use both -in and -out options!\n");
      usage();
    }
    MODE = MODE_OUT;
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-fix"))) {
    if (MODE) {
      fprintf (stderr, "ERROR: cannot use -fix with -in or -out options!\n");
      usage();
    }
    MODE = MODE_FIX;
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-out-backup"))) {
    if (MODE) {
      fprintf (stderr, "ERROR: cannot use -fix with -in or -out options!\n");
      usage();
    }
    // usage dvodist -out-backup (srcHost) to (dstHost)
    MODE = MODE_OUT_BACKUP;
    remove_argument (N, &argc, argv);
    srcHostname = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
    remove_argument (N, &argc, argv); // remove 'to'
    dstHostname = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }
  if ((N = get_argument (argc, argv, "-use-backup"))) {
    if (MODE) {
      fprintf (stderr, "ERROR: cannot use -fix with -in or -out options!\n");
      usage();
    }
    // usage dvodist -out-backup (srcHost) to (dstHost)
    MODE = MODE_USE_BACKUP;
    remove_argument (N, &argc, argv);
    srcHostname = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }
  if (!MODE) usage();

  if (argc != 2) usage();

  return (TRUE);
}
