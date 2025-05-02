# include "ohana.h"
void usage ();

int main (int argc, char **argv) {

  char *config, *file;
  char word[256];
  int i, N, VERBOSE, status;

  if ((N = get_argument (argc, argv, "-h"))) { usage (); }
  if ((N = get_argument (argc, argv, "-help"))) { usage (); }

  /*** load configuration info ***/
  file = SelectConfigFile (&argc, argv, "ptolemy");

  /* dump raw config file (no interpolation of input files */
  if ((N = get_argument (argc, argv, "-raw"))) {
    config = LoadRawConfigFile (file, TRUE);
    fwrite (config, 1, strlen(config), stdout);
    exit (0);
  }

  /* load complete config info from file(s) */
  config = LoadConfigFile (file);
  if (config == (char *) NULL) {
    fprintf (stderr, "ERROR: can't find configuration file %s\n", file);
    if (file != (char *) NULL) free (file);
    exit (1);
  }

  VERBOSE = FALSE;
  if ((N = get_argument (argc, argv, "-v"))) {
    remove_argument (N, &argc, argv);
    VERBOSE = TRUE;
  }
  if ((N = get_argument (argc, argv, "-q"))) {
    remove_argument (N, &argc, argv);
    VERBOSE = FALSE;
  }

  /* if no keywords, dump entire config file */
  if (argc == 1) {
    fwrite (config, 1, strlen(config), stdout);
    exit (0);
  }

  status = 0;
  for (i = 1; i < argc; i++) {
    if (ScanConfig (config, argv[i], "%s", 0,  word) == (char *) NULL) {
      strcpy (word, "not found");
      status = 1;
    }
    if (VERBOSE) {
      fprintf (stdout, "%s %s\n", argv[i], word);
    } else {
      fprintf (stdout, "%s\n", word);
    }
  }
  exit (status);
}

void usage () {
  fprintf (stderr, "gconfig: print elixir config information\n");
  fprintf (stderr, " USAGE: gconfig [keywords...] [-c file] [-C config] [-D keyword value]\n");
  exit (2);
}
