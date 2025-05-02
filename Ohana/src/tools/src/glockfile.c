# include <ohana.h>

int main (int argc, char **argv) {

  char *filename;
  double timeout;
  int N, holdtime, state, type;
  FILE *f;

  timeout = 30.0;
  if ((N = get_argument (argc, argv, "-timeout"))) {
    remove_argument (N, &argc, argv);
    timeout = atof (argv[N]);
    remove_argument (N, &argc, argv);
  }

  if (argc != 4) {
    fprintf (stderr, "USAGE: glock (filename) (type) (holdtime)\n");
    exit (1);
  }

  filename = argv[1];
  holdtime = atoi (argv[3]);

  type = 0;
  if (!strcasecmp (argv[2], "hard")) {
    type = LCK_HARD;
  }
  if (!strcasecmp (argv[2], "soft")) {
    type = LCK_SOFT;
  }
  if (!strcasecmp (argv[2], "xcld")) {
    type = LCK_XCLD;
  }
  if (!strcasecmp (argv[2], "check")) {
    fchecklockfile (filename, LCK_HARD, &state);
    exit (0);
  }

  f = fsetlockfile (filename, timeout, type, &state);
  if (f == NULL) {
    fprintf (stderr, "ERROR: can't lock file %s\n", filename);
    exit (1);
  }

  //  fprintf (stderr, "file is locked\n");
  sleep (holdtime);
  fclearlockfile (filename, f, type, &state);
  exit (0);
}
