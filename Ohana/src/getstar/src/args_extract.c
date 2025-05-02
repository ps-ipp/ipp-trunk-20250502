# include "dvoImageExtract.h"

void help () {
  fprintf (stderr, "USAGE: \n"
	   "dvoExtractImages (imageID) [-o output]\n"
    );
  exit (2);
}

int args_extract (int argc, char **argv) {
  
  int N;

  /* check for help request */
  if (get_argument (argc, argv, "-help") ||
      get_argument (argc, argv, "-h")) {
    help ();
  }

  /* check for command line options */
  VERBOSE = FALSE;
  if ((N = get_argument (argc, argv, "-v"))) {
    VERBOSE = TRUE;
    remove_argument (N, &argc, argv);
  }

  /* check for command line options */
  OUTFILE = NULL;
  if ((N = get_argument (argc, argv, "-o"))) {
    remove_argument (N, &argc, argv);
    OUTFILE = strcreate (argv[N]);
    remove_argument (N, &argc, argv);
  }

  if (argc != 2) help();

  return (TRUE);
}
